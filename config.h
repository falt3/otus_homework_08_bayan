#pragma once

#include <boost/program_options.hpp>
#include <boost/filesystem/operations.hpp>
// #include <filesystem>
#include <iostream>
#include <string>
#include "hash.h"
#include <memory>

namespace opt = boost::program_options;
// namespace fs = std::filesystem;
namespace fs = boost::filesystem;

struct Config {
    bool parse (int argc, const char* argv[]);

    std::vector<std::string>  dirs_in;
    std::vector<std::string>  dirs_ignore;
    int level       = 0;
    int minSize     = 1;
    int blockSize   = 100;
    std::string mask;
    std::unique_ptr<IHash> hash;
};

bool Config::parse(int argc, const char* argv[]) 
{
    // -- разбор параметров
    opt::options_description desc("All options");
    desc.add_options()
        ("dir,d", opt::value<std::vector<std::string>>(), "список директорий для сканирования")
        ("ignore,i", opt::value<std::vector<std::string>>(), "список директорий для исключения из сканирования")
        ("level,l", opt::value<int>()->default_value(0), "уровень сканирования")
        ("size,s", opt::value<int>()->default_value(1), "минимальный размер файла")
        ("mask", opt::value<std::string>()->default_value("*"), "маска имен файлов, разрешенных для сравнения")
        ("block,b", opt::value<int>()->default_value(100), "размер блока, которым производится чтения файлов")
        ("hash,H", opt::value<std::string>()->default_value("md5"), "алгоритм хэширования (md5, crc32)")
        ("help,h", "справка")
    ;
    opt::variables_map vm;
    opt::store(opt::parse_command_line(argc, argv, desc), vm);
    opt::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 1;
    }

    std::vector<std::string>  dirs_in_tmp;
    if (vm.count("dir")) 
        dirs_in_tmp = vm["dir"].as<std::vector<std::string>>();
    for (auto &el : dirs_in_tmp) {
        fs::path path(el);
        if (fs::is_directory(path)) 
            dirs_in.push_back(fs::canonical(path).string());
    }        

    std::vector<std::string>  dirs_ignore_tmp;
    if (vm.count("ignore"))
        dirs_ignore_tmp = vm["ignore"].as<std::vector<std::string>>();
    for (auto &el : dirs_ignore_tmp) {
        fs::path path(el);
        if (fs::is_directory(path))
            dirs_ignore.push_back(fs::canonical(path).string());
    }
    
    if (vm.count("level"))
         level = vm["level"].as<int>();

    if (vm.count("size"))
        minSize = vm["size"].as<int>();

    if (vm.count("block"))
        blockSize = vm["block"].as<int>();
    
    std::string mask_tmp;
    if (vm.count("mask"))
        mask_tmp = vm["mask"].as<std::string>();
    for (auto ch : mask_tmp) {
        // std::cout << ch;
        if (ch == '*') mask.append(".*");
        else if (ch == '?') mask.append(".");
        else if (ch == '.') mask.append("\\.");
        else mask.push_back(ch);
    }
    // std::cout << std::endl << "mask: " << mask << std::endl;

    if (vm.count("hash")) {
        std::string s = vm["hash"].as<std::string>();
        if (s == "crc32") hash = std::make_unique<Hash_crc32>();
        else if (s == "md5") hash = std::make_unique<Hash_md5>();
        else return false;
    }
    else return false;

    return true;
}

