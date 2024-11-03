#pragma once
#include <functional>
#include <sstream>
#include "command.h"

namespace cli
{

class global_help_command : public command
{
public:
    global_help_command(const std::string& name, const std::string& description) : command(name, description, false) {}
    virtual int do_execute(const std::vector<std::string>& params) override
    {
        std::ostringstream os;
        os << "Usage:" << "\n";
        for(const auto& [command_name, command] : command_line::get_instance()->get_commands())
        {
            os << "  " << command_name << " = " << command->get_description() << "\n";
        }
        printf("%s", os.str().data());
        return OK;
    }
};

class help_command : public command
{
public:
    help_command(const std::string& name, const std::string& description) : command(name, description, false) {}
    virtual int do_execute(const std::vector<std::string>& params) override
    {
        _parent->print_help();
        return OK;
    }
};

class exit_command : public command
{
public:
    exit_command(const std::string& name, const std::string& description) : command(name, description) {}
    virtual int do_execute(const std::vector<std::string>& params) override { return QUIT; }
};

template<typename... Args>
class function_command : public command
{
public:
    using function_type = std::function<int(Args...)>;
    function_command(function_type func, const std::string& name, const std::string& description) : _func(func), command(name, description) {}
    virtual int do_execute(const std::vector<std::string>& params) override
    {
        if(_func)
        {
            return _func();
        }
        printf("Function command _func is null\n");
        return ERROR;
    }

private:
     function_type _func;
};

} // namespace cli