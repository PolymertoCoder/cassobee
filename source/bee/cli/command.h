#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <set>
#include "command_line.h"
#include "types.h"
#include "errcode.h"

namespace cli
{

class command
{
public:
    command(const std::string& name, const std::string& description);
    virtual ~command();
    int  execute(std::vector<std::string>& params);

    // option
    auto get_option(const std::string& option_name) -> command*;
    int  add_option(const std::string& short_name, const std::string& long_name, command* option);
    int  remove_option(const std::string& option);

    // subcommand
    auto get_subcommand(const std::string& subcommand_name) -> command*;
    void add_subcommand(const std::string& subcommand_name, command* subcommand);
    void remove_subcommand(const std::string& subcommand_name);

    // alias
    FORCE_INLINE bool add_alias(const std::string& alias) { return _alias.insert(alias).second; }
    FORCE_INLINE bool remove_alias(const std::string& alias) { return _alias.erase(alias); }

    FORCE_INLINE void set_name(const std::string& name) { _name = name; }
    FORCE_INLINE auto get_name() const { return _name; }
    FORCE_INLINE void set_description(const std::string& description) { _description = description; }
    FORCE_INLINE auto get_description() const { return _description; }

    virtual int  do_execute(const std::vector<std::string>& params) { return OK; };
    virtual void print_help();
    virtual void get_param_completions(const std::string& param, std::vector<std::string>& completions);

protected:
    friend class command_line;
    std::string _name;
    std::string _description;
    cli::command* _parent = nullptr;
    std::set<std::string> _alias;
    std::unordered_map<std::string, cli::command*> _subcommands;
    std::unordered_map<std::string, cli::command*> _options;
};

}; // namespace cli