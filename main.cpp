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
#include "config.h"
#include <string>
#include <fstream>
#include <algorithm>
#include <boost/regex.hpp>



/**
 * @brief Структура для хранения информации о файле
 * 
 */
struct TFileInf 
{
    TFileInf(std::size_t size_, const std::string& fn_, int group_) {
        size = size_;
        fn = fn_;
        group = group_;
    }
    TFileInf(TFileInf&& el) : size(el.size), fn(std::move(el.fn)), group (el.group), hash(std::move(el.hash)) {}
    TFileInf& operator=(TFileInf&& el) {
        size = el.size;
        fn = std::move(el.fn);
        group = el.group;
        hash = std::move(el.hash);
        return *this;
    }
    std::size_t size;               ///< размер файла
    std::string fn;                 ///< полное название файла
    int group       = 0;            ///< групирование файлов по содержимому
    std::vector<uint8_t> hash;      ///< текущий результат hash функции
};

//----------------------------------------------------------------
/**
 * @brief Поиск файлов в директории по заданным критериям выбора
 * 
 * @param [in] dir_in        Директория для сканирования
 * @param [in] dirs_ignore   Список директорий для исключения из сканирования
 * @param [in] level         Уровень сканирования
 * @param [in] minSize       Минимальный размер файла
 * @param [in] mask          Маска имен файлов разрешенных для сравнения
 * @param [out] files        Список файлов удовлетворяющих критерию выбора
 */
void scan(const std::string& dir_in, const std::vector<std::string>& dirs_ignore, 
             int level, std::size_t minSize, const std::string& mask,
             std::vector<TFileInf>& files)
{
    // std::cout << "scan: " << dir_in << "\n";
    if (fs::is_directory(fs::path(dir_in))) {
        for (auto& el : dirs_ignore) 
            if (dir_in == el) return;

        fs::path pathDir(dir_in);
        for (const auto& entry : fs::directory_iterator{ pathDir })         
        {
            if (fs::is_directory(entry.path())) {
                if (level > 0)
                    scan(entry.path().string(), dirs_ignore, level - 1, minSize, mask, files);
            }
            else if (fs::is_regular_file(entry.path())) {
                if (fs::file_size(entry.path()) >= minSize) {
                    boost::regex re(mask);
                    bool matched = boost::regex_match(entry.path().filename().string(), re);
                    // std::cout << "\tpath part: " << " = " << entry << " " << matched << std::endl;
                    if (matched)
                        files.push_back({fs::file_size(entry.path()), fs::canonical(entry).string(), 0});
                }
            }
        }
    }
}

//----------------------------------------------------------------
/**
 * @brief   Функция сравнения двух файлов поблочно
 * 
 * @param [in] begin        Итератор начала диапазона списка файлов для сравнения
 * @param [in] end          Итератор конца диапазона списка файлов для сравнения
 * @param [in] blockSize    Размер блока чтения из файла
 * @param [in] bytesRead    Количество прочитанных байт в файле 
 * @param [in] group        Номер группы для файлов с одинаковым содержимым
 * @details В функцию передается диапазон вектора в котором файлы одинаковы на текущем этапе сравнения.
 * Для каждого файла находится хэш. Файлы сортируются по найденному хэшу, чтобы одинаковые находились рядом.
 * Файлы с одинаковыми хэшами, если не проверились полностью отправляются рекурсивно опять в эту функцию.
 * А файлам, которые полностью проверились и у которых совпал хэш, присваевается уникальный номер группы.
 *  */
void compare(std::vector<TFileInf>::iterator begin, std::vector<TFileInf>::iterator end, std::size_t blockSize, std::size_t bytesRead, int& group, IHash& hash)
{
    assert(begin != end && "compare: begin == end");
    assert(blockSize > 0 && "compare: blocksize <= 0");

    // -- чтение из файла и расчет hash
    std::string ss;             // буфер для чтения
    ss.resize(blockSize, 0);

    bool fl_readLast = (bytesRead + blockSize) > (*begin).size ? true : false;  // признак окончания файла
    int blockRead =  fl_readLast ? ((*begin).size - bytesRead) : blockSize;     // кол-во байт которое нужно прочитать

    // std::cout << "Compare: " << end - begin << " " << blockSize << " " << fl_readLast << " " << blockRead << " " << bytesRead << "\n";

    for (auto it = begin; it != end; ++it) {
        // std::cout << "    " << (*it).size << "  " << (*it).fn << " " << (*it).hash.size() << std::endl;
        std::ifstream fs((*it).fn);
        if (fs.is_open()) {
            fs.seekg(bytesRead);
            fs.read(ss.data(), blockRead);
            fs.close();
        }
        else 
            std::cout << "    no opened\n";
            
        hash.hash(ss, (*it).hash);       // расчет hash
    }

    bytesRead += blockRead;


    // -- сортировка по hash
    std::sort(begin, end, [](TFileInf& a, TFileInf& b) {
        for (std::size_t i = 0; i < a.hash.size(); ++i) {
            if (b.hash[i] > a.hash[i]) return true;
        }
        return false;
    });


    // -- нахождение одинаковых файлов для продолжения сравнения
    int count = 1;                
    std::vector<uint8_t> &val = (*begin).hash;

    for (auto it = begin + 1; it < end; ++it) {
        int flagCount = 0;
        if (equal(val.cbegin(), val.cend(), (*it).hash.cbegin())) {
            count++;
            if (it == end - 1) {    // Проверяем, что это последний элемент 
                it++;               // переставляем на end, чтобы два случая группировки одинаково обработать
                flagCount = count;
            }
        }
        else {
            if (count > 1) 
                flagCount = count;
            val = (*it).hash;
            count = 1;
        }

        if (flagCount > 0) {
            if (fl_readLast) {  // файл закончился
                group++;
                for (auto it2 = it - flagCount; it2 != it; ++it2)
                    (*it2).group = group;
            }            
            else 
                compare(it - flagCount, it, blockSize, bytesRead, group, hash);
        }
    }
}

//----------------------------------------------------------------

int main(int argc, const char* argv[]) 
{
    // -- разбор входных параметров 
    Config config;
    if (!config.parse(argc, argv))
        return 1;


    // -- формирование списка файлов
    std::vector<TFileInf> files;    
    for (const auto& el : config.dirs_in)
        scan(el, config.dirs_ignore, config.level, config.minSize, config.mask, files);


    // -- сортировка списка по размеру файла (первое группирование файлов идет по размеру)
    std::sort(files.begin(), files.end(), [](TFileInf& a, TFileInf& b) {
        return (b.size > a.size);
    });


    // -- нахождение одинаковых файлов
    int group = 0;              // всем одинаковым файлам присваивается номер группы
    std::size_t val = -1;
    int count = 0;

    for (auto it = files.begin(); it != files.end(); ++it) {
        if (val != (*it).size) {
            if (count > 1) {
                (*(it-1)).hash.resize(config.hash->size());
                compare(it - count, it, config.blockSize, 0, group, *config.hash);
            }
            val = (*it).size;
            count = 1;
        }
        else {
            ++count;
            (*(it-1)).hash.resize(config.hash->size());
        }
    }
    if (count != 0) {
        (*(files.end()-1)).hash.resize(config.hash->size());
        compare(files.end() - count, files.end(), config.blockSize, 0, group, *config.hash);
    }


    // -- вывод списков с одинаковым содержимым файлов
    int group_ = 0;
    for (auto& el : files) {
        if (el.group != 0) {
            if (el.group != group_ && group_ != 0) // вывод пустой строки между группами файлов
                std::cout << std::endl;
            std::cout << el.fn << std::endl;
            group_ = el.group;
        }
    }
        
    
    return 0;
}