#pragma once
#include <string>
#include <vector>

namespace cli
{

class command
{
public:
    virtual ~command();
    int  execute(const std::vector<std::string>& params);
    void add_option(const std::string& opt, const std::string& discription);
    void add_subcommand(cli::command* command);

    virtual int  do_execute(const std::vector<std::string>& params) = 0;
    virtual auto get_help() -> std::string = 0;


protected:
    std::vector<cli::command*> _sub_commands;
};

}; // namespace cli