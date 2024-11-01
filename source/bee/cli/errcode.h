#pragma once
#include <string>

namespace cli
{

enum ERRCODE
{
    OK    = 0,
    ERROR = 1,
    QUIT  = 2,
};

inline std::string to_error_string(int errcode)
{
    switch(errcode)
    {
        case ERRCODE::OK:    { return "OK";    }
        case ERRCODE::ERROR: { return "Error"; }
        case ERRCODE::QUIT:  { return "Quit";  }
    }
    return "Unkonwn Error";
}

} // namespace cli