#include <fstream>
#include <readline/readline.h>
#include <string>
#include "command_line.h"
#include "common.h"
#include "readline.h"
#include "history.h"

namespace cli
{

command_line::command_line()
{
    rl_attempted_completion_function = command_line::do_command_completion;
    rl_attempted_completion_over = 1;
}

command_line::~command_line()
{
    
}

void command_line::run()
{
    int retcode = RETCODE::OK;
    while(retcode != RETCODE::QUIT)
    {
        retcode = this->readline();
        if(retcode == RETCODE::OK)
        {
            _greeting = "<";
        }
        else if(retcode == RETCODE::ERROR)
        {
            _greeting = "!<";
        }
        else
        {
            process_errcode(retcode);
        }
    }

    exit(0);
}

int command_line::readline()
{
    char* buffer = ::readline(_greeting.data());
    if(!buffer)
    {
        printf("\n");
        return RETCODE::QUIT;
    }

    if(buffer[0] != '\0')
    {
        add_history(buffer);
    }

    std::string cmd(buffer); free(buffer);
    return execute_command(cmd);
}

int command_line::execute_command(const std::string& cmd)
{
    std::vector<std::string> inputs = split(cmd, " ");
    if(inputs.empty()) return RETCODE::OK;

    auto* command = get_command(inputs[0]);
    if(!command)
    {
        printf("Command %s not found.\n", inputs[0].data());
        return RETCODE::ERROR;
    }
    return inputs.erase(inputs.begin()), command->execute(inputs);
}

int command_line::execute_file(const std::string& filename)
{
    std::ifstream is(filename);
    if(!is)
    {
        printf("Could not find the specified file to execute.\n");
        return RETCODE::ERROR;
    }

    std::string cmd;
    int counter = 0, result = RETCODE::OK;
    while(std::getline(is, cmd))
    {
        if(cmd[0] == '#') continue;
        result = execute_command(cmd);
        if(result != RETCODE::OK) return result;
        printf("[%d]\n", counter++);
    }
    return RETCODE::OK;
}

auto command_line::get_command(const std::string& command_name) -> cli::command*
{
    auto iter = _commands.find(command_name);
    return iter != _commands.end() ? iter->second : nullptr;
}

void command_line::add_command(const std::string& command_name, cli::command* command)
{
    remove_command(command_name);
    _commands.emplace(command_name, command);
}

void command_line::remove_command(const std::string& command_name)
{
    if(auto iter = _commands.find(command_name); iter != _commands.end())
    {
        delete iter->second;
        _commands.erase(iter);
    }
}

void command_line::process_errcode(int errcode)
{

}

char* command_line::command_generator(const char* text, int state)
{
    thread_local std::vector<std::string> matches;
    thread_local size_t match_index;

    if(state == 0)
    {
        matches.clear();
        std::string text_str(text);
        for(const auto& [command_name, command] : get_instance()->_commands)
        {
            if(command_name.find(text_str) == 0)
            {
                matches.push_back(command_name);
            }
        }
        match_index = 0;
    }

    if(match_index < matches.size())
    {
        return strdup(matches[match_index++].data());
    }
    return nullptr;
}

char** command_line::do_command_completion(const char* text, int start, int end)
{
    return rl_completion_matches(text, command_line::command_generator);
}

} // namespace cli