#pragma once
#include "command.h"
#include "common.h"
#include <map>
#include <vector>

namespace cli
{

enum RETCODE
{
    ERROR = -1,
    OK    =  0,
    QUIT  =  1,
};

class command_line : public singleton_support<command_line>
{
public:
    command_line();
    ~command_line();
    void run();
    int  read_line();

    // execute
    int execute_command(const std::string& cmd);
    int execute_file(const std::string& filename);

    // mod command
    auto get_command(const std::string& command_name) -> cli::command*;
    void register_command(const std::string& command_name, cli::command* command);
    void remove_command(const std::string& command_name);

    // completions
    auto get_command_completions(const std::string& input) -> std::vector<std::string>;
    auto get_param_completions(const std::string& command_name, const std::string& param) -> std::vector<std::string>;

private:
    void process_errcode(int errcode);

private:
    std::string _greeting;
    std::map<std::string, cli::command*> _commands;
};

#define REGISTER_CLI_COMMAND(command_name, command) \
    command_line::get_instance()->register_command(command_name, command);

} // namespace cli