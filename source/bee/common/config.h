#pragma once
#include <stdlib.h>
#include <map>
#include <iosfwd>
#include <string>

#include "common.h"

namespace bee
{

class config : public singleton_support<config>
{
public:
    void init(const char* config_path);
    bool parse(std::ifstream& filestream);
    bool reload();
    std::string get(const std::string section, const std::string item, const std::string& default_value = "");

    template<typename return_type = std::string>
    return_type get(const std::string section, const std::string item, return_type default_value = {})
    {
        std::string value = get(section, item);
        if constexpr(std::is_same_v<return_type, bool>)
        {
            if(value == "true" || value == "1")
            {
                return true;
            }
            if(value == "false" || value == "0")
            {
                return false;
            }
            return false;
        }
        if constexpr(std::is_integral_v<return_type>)
        {
            return atoi(value.data());
        }
        if constexpr(std::is_floating_point_v<return_type>)
        {
            return atof(value.data());
        }
        return default_value;
    }

private:
    std::string _config_file;
    using ITEM_MAP    = std::map<std::string, std::string>;
    using SECTION_MAP = std::map<std::string, ITEM_MAP>;
    SECTION_MAP _sections;
};

} // namespace bee