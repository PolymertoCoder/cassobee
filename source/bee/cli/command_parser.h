#pragma once
#include <string>

namespace cli
{

class command_parser
{
public:
    command_parser(const std::string& cmd);

private:
    std::string _cmd;
};

} // namespace cli