#pragma once
#include "command.h"
#include "common.h"
#include <vector>
#include <unordered_map>

namespace cli
{

class command;

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
    auto get_command(const std::string& command_name) -> command*;
    void add_command(const std::string& command_name, command* command, const std::vector<std::string>& alias = {});
    void remove_command(const std::string& command_name);

    // alias
    int add_alias(const std::string& command_name, const std::string& alias);
    int remove_alias(const std::string& command_name, const std::string& alias);

    // completions
    void get_command_completions(const std::string& input, std::vector<std::string>& completions);
    void get_param_completions(const std::string& command_name, const std::string& param, std::vector<std::string>& completions);

private:
    static void process_errcode(int errcode);
    static auto command_generator(const char* text, int state) -> char*;
    static auto do_command_completion(const char* text, int start, int end) -> char**;
    static auto do_param_completion();

private:
    std::string _greeting;
    std::unordered_map<std::string, command*> _commands;
    std::unordered_map<std::string, std::string> _alias;
};

#define REGISTER_CLI_COMMAND(command_name, command) \
    command_line::get_instance()->register_command(command_name, command);

} // namespace cli