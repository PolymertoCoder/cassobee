#include <fstream>
#include <readline/readline.h>
#include <string>
#include "command_line.h"
#include "common.h"
#include "readline.h"
#include "history.h"
#include "log.h"

namespace cli
{

command_line::command_line()
{

}

command_line::~command_line()
{
    
}

void command_line::run()
{
    int retcode = RETCODE::OK;
    while(retcode != RETCODE::QUIT)
    {
        retcode = this->read_line();
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

int command_line::read_line()
{
    char* buffer = readline(_greeting.data());
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
}

auto command_line::get_command(const std::string& command_name) -> cli::command*
{
    auto iter = _commands.find(command_name);
    return iter != _commands.end() ? iter->second : nullptr;
}

void command_line::register_command(const std::string& command_name, cli::command* command)
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

} // namespace cli