#pragma once
#include <string>

namespace bee
{

enum ERRCODE
{
    OK    = 0,
    ERROR = 1,
    QUIT  = 2,

    ERR_TOO_MUCH_PARAMS_COUNT = 4,
};

inline std::string to_error_string(int errcode)
{
    switch(errcode)
    {
        case ERRCODE::OK:    { return "OK";    }
        case ERRCODE::ERROR: { return "Error"; }
        case ERRCODE::QUIT:  { return "Quit";  }
        case ERR_TOO_MUCH_PARAMS_COUNT: { return "It is too much parameters."; }
    }
    return "Unkonwn Error";
}

} // namespace bee