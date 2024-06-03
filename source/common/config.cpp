#include "config.h"
#include <cassert>
#include <filesystem>
#include <fstream>

void config::init(const char* config_path)
{
    _config_path = config_path;

    for(const auto& entry : std::filesystem::directory_iterator(_config_path))
    {
        std::string filepath = entry.path().string();
        if(!entry.is_regular_file())
        {
            printf("config file %s is not a regular file\n", filepath.data());
            continue;
        }
        std::ifstream ifs(filepath);
        if(!ifs.is_open())
        {
            printf("config file %s cannot open?!\n", filepath.data());
            continue;
        }
        if(!parse(ifs))
        {
            printf("parse config file:%s failed, an error occured!!!\n", filepath.data());
        }
        ifs.close();
    }
    printf("load config %s finished...\n", _config_path.data());
}

bool config::parse(std::ifstream& filestream)
{
    std::string section, line;
    while(std::getline(filestream, line))
    {
        if(line.empty()) continue;
        if(line.front() == '[' && line.back() == ']')
        {
            section = line.substr(1, line.length() - 2);
        }
        else if(auto pos = line.find('='); pos != std::string::npos)
        {
            std::string key   = trim(line.substr(0, pos));
            std::string value = trim(line.substr(pos + 1));
            assert(_sections[section].emplace(key, value).second);
        }
    }
    return true;
}

bool config::reload()
{
    return true;
}
    
std::string config::get(const std::string section, const std::string item)
{
    if(auto section_iter = _sections.find(section); section_iter != _sections.end())
    {
        if(auto item_iter = section_iter->second.find(item); item_iter != section_iter->second.end())
        {
            return item_iter->second;
        }
    }
    return "";
}