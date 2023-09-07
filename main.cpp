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
#include <string>
#include <boost/program_options.hpp>
#include <filesystem>
#include <fstream>
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

void hash_sum(const std::vector<uint8_t>& data, std::vector<uint8_t>& h) 
{
    uint sum = 0;
    for (auto el : data)
        sum += el;
    h[0] = (sum >> 24) && 0xff;
    h[1] = (sum >> 16) && 0xff;
    h[2] = (sum >> 8) && 0xff;
    h[3] = (sum >> 0) && 0xff;
}

/**
 * @brief 
 * 
 * @param [in] begin 
 * @param [in] end 
 * @param [in] block 
 */
void compare(std::vector<TFileInf>::iterator begin, std::vector<TFileInf>::iterator end, int block, THash )
{
    // -- вывод на экран
    std::cout << "Compare: " << sizeof(begin) << "\n";
    for (auto it = begin; it != end; ++it) {
        std::cout << "    " << (*it).size << "  " << (*it).fn << std::endl;
    }   


    // -- чтение из файла и расчет hash
    std::vector<char> mr(block);
    for (auto it = begin; it != end; ++it) {
        // std::fill_n(mr.begin(), block);
        std::ifstream ifs((*it).fn, std::ios_base::in);
        ifs.read(mr.begin, block);
    }

    // std::fstream fs("./test_files/ip_filter.tsv", std::fstream::in);
    // BOOST_TEST(fs.is_open());
    // if (fs.is_open()) {
    //     inputData(ipMas, fs);
    //     fs.close();
    // }

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
        // std::for_each(dir_in.cbegin(), dir_in.cend(), [](auto el){std::cout << " " << el;});        
    }    // if (vm.count("level"))

    std::vector<std::string>  dir_ignore;
    if (vm.count("ignore"))
        dir_ignore = vm["ignore"].as<std::vector<std::string>>();
    
    int level = 0;
    if (vm.count("level"))
         level = vm["level"].as<int>();

    int minSize = 1;
    if (vm.count("size"))
        minSize = vm["size"].as<int>();

    int block = 100;
    if (vm.count("block"))
        block = vm["block"].as<int>();

    THash hash = THash::none;
    if (vm.count("hash")) {
        std::string s = vm["hash"].as<std::string>();
        if (s == "crc32") hash = THash::crc32;
        else if (s == "md5") hash = THash::md5;
    }


    std::cout << "Init: " << level << " " << minSize << std::endl;


    // -- формирование списка файлов
    std::vector<TFileInf> mas;    
    for (const auto& el : dir_in)
        scan(el, dir_ignore, level, minSize, "*", mas);


    // -- сортировка списка по размеру файла 
    std::sort(mas.begin(), mas.end(), [](TFileInf& a, TFileInf& b) {
        return (b.size > a.size);
    });


    // -- вывод начального списка файлов 
    std::cout << "Массив файлов:\n";
    for (auto el : mas) {
        std::cout << el.size << "\t" << el.fn << std::endl;
    }


    // -- нахождение одинаковых файлов/ -- вывод списков с одинаковыми файлам
    std::size_t val = -1;
    int count = 0;
    for (auto it = mas.begin(); it != mas.end(); ++it) {
        if (val != (*it).size) {
            if (count > 1) {
                (*(it-1)).hash.resize(4);
                compare(it - count, it, 0, hash);
            }
            val = (*it).size;
            count = 1;
        }
        else {
            ++count;
            (*(it-1)).hash.resize(4);
        }
    }
    if (count != 0) 
        compare(mas.end() - count, mas.end(), block, hash);

    // -- вывод списков с одинаковыми файлами

    
    return 0;
}