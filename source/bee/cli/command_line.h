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
    int  readline();

    // execute
    int execute_command(const std::string& cmd);
    int execute_file(const std::string& filename);

    // mod command
    auto get_command(const std::string& command_name) -> cli::command*;
    void add_command(const std::string& command_name, cli::command* command);
    void remove_command(const std::string& command_name);

    // completions
    auto get_command_completions(const std::string& input, std::vector<std::string>& completions);
    auto get_param_completions(const std::string& command_name, const std::string& param, std::vector<std::string>& completions);

private:
    static void process_errcode(int errcode);
    static auto command_generator(const char* text, int state) -> char*;
    static auto do_command_completion(const char* text, int start, int end) -> char**;
    static auto do_param_completion();

private:
    std::string _greeting;
    std::unordered_map<std::string, cli::command*> _commands;
};

#define REGISTER_CLI_COMMAND(command_name, command) \
    command_line::get_instance()->register_command(command_name, command);

} // namespace cli