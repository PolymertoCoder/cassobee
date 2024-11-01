#include "command.h"
#include "command_line.h"
#include "common.h"
#include <strstream>

namespace cli
{

command::command(const std::string& name, const std::string& description)
    : _name(name), _description(description)
{

}

command::~command()
{

}

int command::execute(std::vector<std::string>& params)
{
    if(params.size())
    {
        if(startswith(params[0], "-")) // short name option
        {
            if(auto* option = get_option(params[0].substr(1)))
            {
                return params.erase(params.begin()), option->execute(params);
            }
            else
            {
                printf("Unknown options -%s.\n", params[0].data());
                return ERROR;
            }
        }
        else if(startswith(params[0], "--")) // long name option
        {
            if(auto* option = get_option(params[0].substr(2)))
            {
                return params.erase(params.begin()), option->execute(params);
            }
            else
            {
                printf("Unknown options --%s.\n", params[0].data());
                return ERROR;
            }
        }
        else if(auto* subcommand = get_subcommand(params[0])) // subcommand
        {
            return params.erase(params.begin()), subcommand->execute(params);
        }
        else // param
        {
            return params.erase(params.begin()), do_execute(params);
        }
    }
    else
    {
        return do_execute(params);
    }
}

auto command::get_option(const std::string& option_name) -> cli::command*
{
    auto iter = _options.find(option_name);
    return iter != _options.end() ? iter->second : nullptr;
}

int command::add_option(const std::string& short_name, const std::string& long_name, cli::command* option)
{
    if(short_name.size() && !startswith(short_name, "-"))
    {
        printf("Command %s short name option %s failed, must starts with \'-\'.\n", _name.data(), short_name.data());
        return ERROR;
    }
    if(long_name.size() && !startswith(long_name, "--"))
    {
        printf("Command %s long name option %s failed, must starts with \'--\'.\n", _name.data(), long_name.data());
        return ERROR;
    }
    if(short_name.size() && !_options.emplace(short_name, option).second)
    {
        printf("Command %s add short name option failed, option %s repeated.\n", _name.data(), short_name.data());
        return ERROR;
    }
    if(long_name.size() && !_options.emplace(long_name, option).second)
    {
        printf("Command %s add long name option failed, option %s repeated.\n", _name.data(), long_name.data());
        return ERROR;
    }
    return OK;
}

int command::remove_option(const std::string& option_name)
{
    if(auto iter = _options.find(option_name); iter != _options.end())
    {
        delete iter->second;
        _options.erase(iter);
    }
}

auto command::get_subcommand(const std::string& subcommand_name) -> cli::command*
{
    auto iter = _subcommands.find(subcommand_name);
    return iter != _subcommands.end() ? iter->second : nullptr;
}

void command::add_subcommand(const std::string& subcommand_name, cli::command* subcommand)
{
    remove_subcommand(subcommand_name);
    _subcommands.emplace(subcommand_name, subcommand);
}

void command::remove_subcommand(const std::string& subcommand_name)
{
    if(auto iter = _subcommands.find(subcommand_name); iter != _subcommands.end())
    {
        delete iter->second;
        _subcommands.erase(iter);
    }
}

void command::print_help()
{
    std::ostrstream os;
    os << "Usage: " << _name << " [option][subcommand]\n";
    os << _description << "\n";
    if(_options.size())
    {
        os << "Options:" << "\n";
        for(const auto& [option_name, option] : _options)
        {
            os << "  " << option_name << "\t" << option->get_description() << "\n";
        }
    }
    if(_subcommands.size())
    {
        os << "Subcommand:" << "\n";
        for(const auto& [subcommand_name, subcommand] : _subcommands)
        {
            os << "  " << subcommand_name << "\t" << subcommand->get_description() << "\n";
        }
    }
    if(_alias.size())
    {
        os << "Alias:" << "\n";
        for(const auto& alias : _alias)
        {
            os << "  " << alias << "\n";
        }
    }
}

void command::get_param_completions(const std::string& param, std::vector<std::string>& completions)
{

}

}