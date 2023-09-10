/**
 * 
 */

/**
 * @file main.cpp
 * @author lipatkin Dmitry
 * @brief 
 * @date 2023-08-30
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <iostream>
#include <vector>
#include <boost/program_options.hpp>
#include <filesystem>
#include <string>
#include <fstream>
#include <algorithm>
// #include <boost/filesystem/operations.hpp>

namespace opt = boost::program_options;
namespace fs = std::filesystem;

/**
 * @brief Структура для хранения информации о файле
 * 
 */
struct TFileInf 
{
    std::size_t size;               ///< размер файла
    std::string fn;                 ///< название файла
    int group       = 0;            ///< групирование файлов по содержимому
    std::vector<uint8_t> hash;      ///< hash 
};

/**
 * @brief Список разрешенных hash алгоритмов 
 * 
 */
enum class THash 
{
    none,           ///< алгоритм не указан
    crc32,          ///< CRC32
    md5,            ///< MD5
};

//----------------------------------------------------------------

std::string printV(std::vector<uint8_t> &val) 
{
    std::ostringstream ss;
    ss << std::hex << "0x";
    for (auto el : val) {
        ss.width(2);
        ss.fill('0');
        ss << (uint32_t)el;
    }
    ss << std::dec;
    return ss.str();
}


//----------------------------------------------------------------
/**
 * @brief Поиск файлов в директории по заданным критериям выбора
 * 
 * @param [in] dir_in       Директория для сканирования
 * @param [in] dir_ignore   Список директорий для исключения из сканирования
 * @param [in] level        Уровень сканирования
 * @param [in] minSize      Минимальный размер файла
 * @param [in] mask         Маска имен файлов разрешенных для сравнения
 * @param [out] mas         Список файлов удовлетворяющих критерию выбора
 */
void scan(const std::string& dir_in, const std::vector<std::string>& dir_ignore, 
             int level, std::size_t minSize, const std::string& ,
             std::vector<TFileInf>& mas)
{
    std::cout << "scan: " << dir_in << "\n";
    if (fs::is_directory(dir_in)) {
        fs::path pathDir(dir_in);
        std::cout << "  2: " << dir_in << "\t" << fs::canonical(pathDir) << "\n";
        for (const auto& entry : fs::directory_iterator{ pathDir })         
        {
            if (entry.is_directory()) {
                if (level > 0)
                    scan(entry.path(), dir_ignore, level - 1, minSize, "", mas);
            }
            else if (entry.is_regular_file()) {
                if (entry.file_size() >= minSize) {
                    std::cout << "\tpath part: " << " = " << entry << std::endl;
                    mas.push_back({entry.file_size(), fs::canonical(entry), 0, {}});
                }
            }
        }
    }
}

//----------------------------------------------------------------

void hash_sum(const std::string& data, std::vector<uint8_t>& h) 
{
    uint sum = 0;
    for (auto el : data) {
        sum += (uint8_t)el;
        // std::cout << "sum: " << sum << std::endl;
    }
    h[0] = (sum >> 24) & 0xff;
    h[1] = (sum >> 16) & 0xff;
    h[2] = (sum >> 8) & 0xff;
    h[3] = (sum >> 0) & 0xff;
    // std::cout << "    hash0: " << std::hex << (uint32_t)h[0] << " " << (uint32_t)h[1] << " " << (uint32_t)h[2] << " " << (uint32_t)h[3] << std::dec << std::endl;
}

/**
 * @brief   Функция сравнения двух файлов поблочно
 * 
 * @param [in] begin        Итератор начала диапозона списка файлов для сравнения
 * @param [in] end          Итератор конца диапозона списка файлов для сравнения
 * @param [in] blockSize    Размер блока чтения из файла
 * @param [in] bytesRead    Количество прочитанных байт в файле 
 * @param [in] group        Текущий номер группы для файлов с одинаковым содержимым
 */
void compare(std::vector<TFileInf>::iterator begin, std::vector<TFileInf>::iterator end, std::size_t blockSize, std::size_t bytesRead, int& group, THash hashFunc)
{
    assert(begin != end && "compare: 001");
    assert(blockSize > 0);

    // -- чтение из файла и расчет hash
    std::string ss;             // буфер для чтения
    ss.resize(blockSize, 0);

    bool fl_readLast = (bytesRead + blockSize) > (*begin).size ? true : false;  // последнее чтение из файла
    int blockRead =  fl_readLast ? ((*begin).size - bytesRead) : blockSize;     // кол-во байт которое нужно прочитать

    // -- вывод на экран
    std::cout << "Compare: " << end - begin << " " << blockSize << " " << fl_readLast << " " << blockRead << " " << bytesRead << "\n";

    for (auto it = begin; it != end; ++it) {
        std::cout << "    " << (*it).size << "  " << (*it).fn << " " << (*it).hash.size() << std::endl;
        std::ifstream fs((*it).fn);
        if (fs.is_open()) {
            fs.seekg(bytesRead);
            fs.read(ss.data(), blockRead);
            fs.close();
        }
        else 
            std::cout << "    no opened\n";
            
        hash_sum(ss, (*it).hash);       // расчет hash
    }

    bytesRead += blockRead;


    // -- сортировка по hash
    std::sort(begin, end, [](TFileInf& a, TFileInf& b) {
        for (std::size_t i = 0; i < a.hash.size(); ++i) {
            if (b.hash[i] > a.hash[i]) return true;
        }
        return false;
    });


    // -- нахождение одинаковых файлов
    int count = 1;                
    std::vector<uint8_t> &val = (*begin).hash;
    std::cout << "  hash: " << printV(val) << std::endl;

    for (auto it = begin + 1; it < end; ++it) {
        int count_tmp = 0;
        if (equal(val.cbegin(), val.cend(), (*it).hash.cbegin())) {
            count++;
            if (it == end - 1) {    // Проверяем, что это последний элемент 
                it++;               // переставляем на end, чтобы два случая одинаково обрабатывать
                count_tmp = count;
            }
        }
        else {
            if (count > 1) 
                count_tmp = count;
            val = (*it).hash;
            count = 1;
        }

        if (count_tmp > 0) {
            if (fl_readLast) {  
                group++;
                for (auto it2 = it - count_tmp; it2 != it; ++it2)
                    (*it2).group = group;
            }            
            else 
                compare(it - count_tmp, it, blockSize, bytesRead, group, hashFunc);
        }
    }
}

//----------------------------------------------------------------

int main(int argc, const char* argv[]) 
{
    // -- разбор параметров
    opt::options_description desc("All options");
    desc.add_options()
        ("dir,d", opt::value<std::vector<std::string>>(), "директории для сканирования")
        ("ignore,i", opt::value<std::string>(), "директории для исключения из сканирования")
        ("level,l", opt::value<int>()->default_value(0), "уровень сканирования")
        ("size,s", opt::value<int>()->default_value(1), "минимальный размер файла")
        ("mask,m", opt::value<std::string>()->default_value("*"), "маски имен файлов разрешенных для сравнения")
        ("block,b", opt::value<int>()->default_value(100), "размер блока, которым производится чтения файлов")
        ("hash,H", opt::value<std::string>(), "один из имеющихся алгоритмов хэширования")
        ("help,h", "справка")
    ;
    opt::variables_map vm;
    opt::store(opt::parse_command_line(argc, argv, desc), vm);
    opt::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 1;
    }

    std::vector<std::string>  dir_in;
    if (vm.count("dir")) {
        dir_in = vm["dir"].as<std::vector<std::string>>();
    } 

    std::vector<std::string>  dir_ignore;
    if (vm.count("ignore"))
        dir_ignore = vm["ignore"].as<std::vector<std::string>>();
    
    int level = 0;
    if (vm.count("level"))
         level = vm["level"].as<int>();

    int minSize = 1;
    if (vm.count("size"))
        minSize = vm["size"].as<int>();

    int blockSize = 100;
    if (vm.count("block"))
        blockSize = vm["block"].as<int>();

    THash hash = THash::none;
    if (vm.count("hash")) {
        std::string s = vm["hash"].as<std::string>();
        if (s == "crc32") hash = THash::crc32;
        else if (s == "md5") hash = THash::md5;
    }


    // -- формирование списка файлов
    std::vector<TFileInf> mas;    
    for (const auto& el : dir_in)
        scan(el, dir_ignore, level, minSize, "*", mas);


    // -- сортировка списка по размеру файла 
    std::sort(mas.begin(), mas.end(), [](TFileInf& a, TFileInf& b) {
        return (b.size > a.size);
    });


    // -- вывод начального списка файлов 
    // std::cout << "Массив файлов:\n";
    // for (auto el : mas) {
    //     std::cout << el.size << "\t" << el.fn << std::endl;
    // }


    // -- нахождение одинаковых файлов
    int group = 0;
    std::size_t val = -1;
    int count = 0;

    for (auto it = mas.begin(); it != mas.end(); ++it) {
        if (val != (*it).size) {
            if (count > 1) {
                (*(it-1)).hash.resize(4);
                compare(it - count, it, blockSize, 0, group, hash);
            }
            val = (*it).size;
            count = 1;
        }
        else {
            ++count;
            (*(it-1)).hash.resize(4);
        }
    }
    if (count != 0) {
        (*(mas.end()-1)).hash.resize(4);
        compare(mas.end() - count, mas.end(), blockSize, 0, group, hash);
    }

    
    // std::cout << "Массив файлов:\n";
    // for (auto el : mas) {
    //     std::cout << el.size << "  " << el.group << "  "<< el.fn << std::endl;
    // }


    // -- вывод списков с одинаковым содержимым файлов
    int group_ = 0;
    for (auto el : mas) {
        if (el.group != 0) {
            if (el.group != group_) 
                std::cout << std::endl;
            std::cout << el.fn << std::endl;
            group_ = el.group;
        }
    }
        
    
    return 0;
}