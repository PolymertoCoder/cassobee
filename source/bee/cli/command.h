#pragma once
#include "types.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace cli
{

class command
{
public:
    command();
    virtual ~command();
    int  execute(const std::vector<std::string>& params);
    void add_option(const std::string& option, const std::string& description);
    void add_subcommand(cli::command* subcommand);

    FORCE_INLINE auto get_name() const { return _name; }
    FORCE_INLINE auto get_description() const { return _description; }

    virtual int  do_execute(const std::vector<std::string>& params) = 0;
    virtual auto get_help() -> std::string;
    virtual void get_param_completions(const std::string& param, std::vector<std::string>& completions);

protected:
    friend class command_line;
    std::string _name;
    std::string _description;
    cli::command* _parent = nullptr;
    std::unordered_map<std::string, cli::command*> _subcommands;
    std::unordered_map<std::string, std::pair<std::string, std::string>> _options;
};

}; // namespace cli