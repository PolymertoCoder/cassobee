#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>
#include "command_line.h"
#include "app_commands.h"
#include "errcode.h"
#include "common.h"
#include "readline.h"
#include "history.h"
#include "config.h"

namespace cli
{

command_line::command_line()
{
    _greeting = "$ ";

    rl_attempted_completion_function = command_line::do_command_completion;
    rl_attempted_completion_over = 1;

    add_command("help", new global_help_command("help", "Enter \"help\" or \"h\" to print usage."), {"h"});
    add_command("quit", new exit_command("exit", "Enter \"q\" or \"quit\" for exit."), {"q", "quit"});

    auto current_path = std::filesystem::current_path();
    for(const auto& entry : std::filesystem::recursive_directory_iterator(current_path))
    {
        if(entry.is_regular_file() && entry.path().filename() == ".cliinit")
        {
            printf("Found %s\n", entry.path().c_str());
            execute_file(entry.path());
        }
    }
}

command_line::~command_line()
{
    for(auto& [_, command] : _commands)
    {
        delete command;
    }
    _commands.clear();
    _alias.clear();
}

void command_line::run()
{
    int retcode = OK;
    while(retcode != QUIT)
    {
        retcode = this->readline();
        if(retcode == OK)
        {
            _greeting = "$ ";
        }
        else if(retcode == ERROR)
        {
            _greeting = "!$ ";
        }
        else
        {
            _greeting = "!$ ";
            process_errcode(retcode);
        }
    }
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
    auto name = command_name;
    if(auto aiter = _alias.find(name); aiter != _alias.end())
    {
        name = aiter->second;
    }
    if(auto iter = _commands.find(name); iter != _commands.end())
    {
        return iter->second;
    }
    else
    {
        return nullptr;
    }
}

int command_line::add_command(const std::string& command_name, cli::command* command, const std::vector<std::string>& alias)
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
                return ERROR;
            }
        }
    }
    else
    {
        printf("Command %s already be registed.\n", command_name.data());
        return ERROR;
    }
    return OK;
}

template<typename... Args>
int command_line::add_command(const std::string& command_name, std::function<int(Args...)> func, const std::string& name, const std::string& description, const std::vector<std::string>& alias)
{
    return add_command(command_name, new function_command<Args...>(func, name, description), alias);
}

int command_line::remove_command(const std::string& command_name)
{
    auto name = command_name;
    if(auto aiter = _alias.find(name); aiter != _alias.end())
    {
        name = aiter->second;
    }
    if(auto iter = _commands.find(name); iter != _commands.end())
    {
        for(const auto& alias : iter->second->_alias)
        {
            _alias.erase(alias);
        }
        delete iter->second;
        _commands.erase(iter);
        return OK;
    }
    else
    {
        printf("No need to remove command %s, command not found.\n", command_name.data());
        return ERROR;
    }
}

int command_line::add_alias(const std::string& command_name, const std::string& alias)
{
    auto name = command_name;
    if(auto aiter = _alias.find(name); aiter != _alias.end())
    {
        name = aiter->second;
    }
    if(auto iter = _commands.find(name); iter != _commands.end())
    {
        if(_alias.emplace(alias, command_name).second)
        {
            iter->second->add_alias(alias);
            return OK;
        }
        else
        {
            printf("Add command alias failed, alias %s is registed.\n", alias.data());
            return ERROR;
        }
    }
    else
    {
        printf("Add command alias %s failed, command %s not found.\n", alias.data(), command_name.data());
        return ERROR;
    }
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

auto command_line::parse_command(const std::vector<std::string>& tokens, std::vector<std::string>& params, std::unordered_map<std::string, std::string>& options) -> const cli::command*
{
    const command* command  = nullptr;
    const auto*    commands = &_commands;
    for(size_t i = 0; i < tokens.size(); ++i)
    {
        const auto& token = tokens[i];
        if(token[0] == '-')
        {
            if(i + 1 < tokens.size() && tokens[i + 1][0] != '-')
            {
                options[token] = token[++i];
            }
            else
            {
                options[token] = "";
            }
        }
        else
        {
            auto name = token;
            if(auto aiter = _alias.find(token); aiter != _alias.end())
            {
                name = aiter->second;
            }
            if(auto iter = commands->find(name); iter != commands->end())
            {
                command  = iter->second;
                commands = &(command->_subcommands);
            }
            else
            {
                params.push_back(token);
            }
        }
    }
    return command;
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

        // Get the current input line and cursor position
        std::string line(rl_line_buffer);
        const size_t cursor_pos = rl_point;

        command_line* cli = get_instance();
        std::vector<std::string> tokens = split(line);
        std::vector<std::string> params;
        std::unordered_map<std::string, std::string> options;
        const auto* command = cli->parse_command(tokens, params, options);

        if(command)
        {
            size_t token_start = 0;
            for(size_t i = 0; i < tokens.size(); ++i)
            {
                if(token_start + tokens[i].size() >= cursor_pos)
                {
                    // We are in the middle of this token
                    if(tokens[i][0] == '-')
                    {
                        // Option completion
                        for(const auto& [option_name, _] : command->_options)
                        {
                            if(startswith(option_name, text_str))
                            {
                                matches.push_back(option_name);
                            }
                        }
                    }
                    else
                    {
                        // Argument or subcommand completion
                        for(const auto& [subcommand_name, _]: command->_subcommands)
                        {
                            if(startswith(subcommand_name, text_str))
                            {
                                matches.push_back(subcommand_name);
                            }
                        }
                    }
                    break;
                }
                token_start += tokens[i].size() + 1; // +1 for the space
            }
            if(matches.empty())
            {

            }
            // if(params.empty())
            // {
            //     // Command name completion
            //     for(const auto& [subcommand_name, _] : command->_subcommands)
            //     {
            //         if(startswith(subcommand_name, text_str))
            //         {
            //             matches.push_back(subcommand_name);
            //         }
            //     }
            // }
            // else
            // {
            //     // Command argument completion
            //     for (const auto& [subcommand_name, _] : command->_subcommands)
            //     {
            //         if(startswith(subcommand_name, params.back()))
            //         {
            //             matches.push_back(subcommand_name);
            //         }
            //     }
            // }
        }
        else
        {
            // Top-level command completion
            for(const auto& [command_name, _] : cli->_commands)
            {
                if(startswith(command_name, text_str))
                {
                    matches.push_back(command_name);
                }
            }
             // Alias completion
             for(const auto& [alias, _] : cli->_alias)
             {
                if(startswith(alias, text_str))
                {
                    matches.push_back(alias);
                }
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