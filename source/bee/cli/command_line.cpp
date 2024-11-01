#include <fstream>
#include <readline/readline.h>
#include <string>
#include "command_line.h"
#include "errcode.h"
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
    int retcode = OK;
    while(retcode != QUIT)
    {
        retcode = this->readline();
        if(retcode == OK)
        {
            _greeting = "<";
        }
        else if(retcode == ERROR)
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
        return QUIT;
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
    if(inputs.empty()) return OK;

    auto* command = get_command(inputs[0]);
    if(!command)
    {
        printf("Command %s not found.\n", inputs[0].data());
        return ERROR;
    }
    return inputs.erase(inputs.begin()), command->execute(inputs);
}

int command_line::execute_file(const std::string& filename)
{
    std::ifstream is(filename);
    if(!is)
    {
        printf("Could not find the specified file to execute.\n");
        return ERROR;
    }

    std::string cmd;
    int counter = 0, result = OK;
    while(std::getline(is, cmd))
    {
        if(cmd[0] == '#') continue;
        result = execute_command(cmd);
        if(result != OK) return result;
        printf("[%d]\n", counter++);
    }
    return OK;
}

auto command_line::get_command(const std::string& command_name) -> cli::command*
{
    auto iter = _commands.find(command_name);
    return iter != _commands.end() ? iter->second : nullptr;
}

void command_line::add_command(const std::string& command_name, cli::command* command, const std::vector<std::string>& alias)
{
    auto [iter, inserted] = _commands.emplace(command_name, command);
    if(inserted)
    {
        for(const auto& a : alias)
        {
            if(_alias.emplace(a, command_name).second)
            {
                iter->second->add_alias(a);
            }
            else
            {
                printf("Command %s alias %s is repeated.\n", command_name.data(), a.data());
            }
        }
    }
    else
    {
        printf("Command %s already be registed.\n", command_name.data());
    }
}

void command_line::remove_command(const std::string& command_name)
{
    if(auto iter = _commands.find(command_name); iter != _commands.end())
    {
        for(const auto& alias : iter->second->_alias)
        {
            _alias.erase(alias);
        }
        delete iter->second;
        _commands.erase(iter);
    }
    else
    {
        printf("No need to remove command %s, command not found.\n", command_name.data());
    }
}

int command_line::add_alias(const std::string& command_name, const std::string& alias)
{
    if(auto iter = _commands.find(command_name); iter == _commands.end())
    {
        printf("Add command alias %s failed, command %s not found.\n", alias.data(), command_name.data());
        return ERROR;
    }
    else if(_alias.emplace(alias, command_name).second)
    {
        iter->second->add_alias(alias);
    }
    return OK;
}

int command_line::remove_alias(const std::string& command_name, const std::string& alias)
{
    if(auto iter = _commands.find(command_name); iter == _commands.end())
    {
        printf("Remove command alias %s failed, command %s not found\n", alias.data(), command_name.data());
        return ERROR;
    }
    else if(_alias.erase(alias))
    {
        iter->second->remove_alias(alias);
    }
    else
    {
        printf("No need to remove command %s alias %s\n", command_name.data(), alias.data());
    }
    return OK;
}

void command_line::get_command_completions(const std::string& input, std::vector<std::string>& completions)
{

}

void command_line::get_param_completions(const std::string& command_name, const std::string& param, std::vector<std::string>& completions)
{

}

void command_line::process_errcode(int errcode)
{
    if(errcode == ERRCODE::OK) return;
    printf("%s\n", to_error_string(errcode).data());
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