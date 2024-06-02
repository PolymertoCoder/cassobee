#include "config.h"

void config::init(const char* config_path)
{

}

bool config::parse(const char* filename)
{
    return true;
}

bool config::reload(const char* filename)
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