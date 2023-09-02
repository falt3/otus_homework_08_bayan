/**
 * 
 */

/**
 * @file main.cpp
 * @author lipatkin dmitry
 * @brief 
 * @date 2023-08-30
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <iostream>
#include <boost/program_options.hpp>

namespace opt = boost::program_options;


int main(int argc, const char* argv[]) 
{
    // if (argc > 1) std::cout << argv[1];
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
    if (vm.count("dir")) {
        std::vector<std::string>  vv = vm["dir"].as<std::vector<std::string>>();
        std::for_each(vv.cbegin(), vv.cend(), [](auto el){std::cout << " " << el;});        
        // std::cout << "\n";
        // std::cout << "Dir: " << vm["dir"].as< std::vector<std::string> >() << "\n";
        // std::cout << "Dir: " << vv << "\n";
        std::cout << vm.count("dir") << "\n";
    }

    std::cout << "exit\n";

    
    return 0;
}