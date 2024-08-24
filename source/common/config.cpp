#include "config.h"
#include <filesystem>
#include <fstream>
#include "stringfy.h"

void config::init(const char* config_path)
{
    _config_path = config_path;
    if(!reload())
    {
        printf("load config %s failed...\n", _config_path.data());
        exit(-1);
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
            CHECK_BUG(_sections[section].emplace(key, value).second, printf("section %s emplace key %s repeat.", section.data(), key.data()); return false;);
        }
    }
    return true;
}

bool config::reload()
{
    for(const auto& entry : std::filesystem::directory_iterator(_config_path))
    {
        std::string filepath = entry.path().string();
        std::ifstream ifs(filepath);
        if(!ifs.is_open())
        {
            printf("config file %s cannot open?!\n", filepath.data());
            continue;
        }
        if(!parse(ifs))
        {
            printf("parse config file:%s failed, an error occured!!!\n", filepath.data());
            return false;
        }
        ifs.close();
        printf("config %s load finished, %s.\n", entry.path().filename().c_str(), cassobee::to_string(_sections).data());
    }
    return true;
}
    
std::string config::get(const std::string section, const std::string item, const std::string& default_value)
{
    if(auto section_iter = _sections.find(section); section_iter != _sections.end())
    {
        if(auto item_iter = section_iter->second.find(item); item_iter != section_iter->second.end())
        {
            return item_iter->second;
        }
    }
    return default_value;
}