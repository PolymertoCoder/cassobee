#include "config.h"
#include <fstream>
#include "stringfy.h"

void config::init(const char* config_file)
{
    _config_file = config_file;
    if(!reload())
    {
        printf("load config file %s failed...\n", _config_file.data());
        exit(-1);
    }
    printf("load config file %s finished...\n", _config_file.data());
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
    std::ifstream ifs(_config_file);
    if(!ifs.is_open())
    {
        printf("config file %s cannot open?!\n", _config_file.data());
        return false;
    }
    SECTION_MAP old_config;
    old_config.swap(_sections);
    if(!parse(ifs))
    {
        _sections.swap(old_config);
        printf("parse config file:%s failed, an error occured!!!\n", _config_file.data());
        return false;
    }
    ifs.close();
    printf("config %s load finished, %s.\n", _config_file.data(), bee::to_string(_sections).data());
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