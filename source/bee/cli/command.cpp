#include "command.h"
#include "app_commands.h"
#include "command_line.h"
#include "common.h"
#include <memory>

namespace bee
{

command::command(const std::string& name, const std::string& description, bool init)
    : _name(name), _description(description)
{
    if(init)
    {
        init_default();
    }
}

command::~command()
{
    delete _parent;
    _subcommands.clear();
    _options.clear();
}

void command::init_default()
{
    add_option("-h", "--help", new help_command("help", "Print usage information and exit."));
}

int command::execute(std::vector<std::string>& params)
{
    if(params.size())
    {
        if(startswith(params[0], "--")) // long name option
        {
            if(auto* option = get_option(params[0]))
            {
                return params.erase(params.begin()), option->execute(params);
            }
            else
            {
                printf("Unknown options %s.\n", params[0].data());
                return ERROR;
            }
        }
        else if(startswith(params[0], "-")) // short name option
        {
            if(auto* option = get_option(params[0]))
            {
                return params.erase(params.begin()), option->execute(params);
            }
            else
            {
                printf("Unknown options %s.\n", params[0].data());
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

auto command::get_option(const std::string& option_name) -> bee::command*
{
    auto iter = _options.find(option_name);
    return iter != _options.end() ? iter->second.get() : nullptr;
}

int command::add_option(const std::string& short_name, const std::string& long_name, bee::command* option)
{
    std::shared_ptr<bee::command> opt(option);
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
    if(short_name.size() && !_options.emplace(short_name, opt).second)
    {
        printf("Command %s add short name option failed, option %s repeated.\n", _name.data(), short_name.data());
        return ERROR;
    }
    if(long_name.size() && !_options.emplace(long_name, opt).second)
    {
        printf("Command %s add long name option failed, option %s repeated.\n", _name.data(), long_name.data());
        return ERROR;
    }
    opt->_parent = this;
    return OK;
}

int command::remove_option(const std::string& option_name)
{
    if(auto iter = _options.find(option_name); iter != _options.end())
    {
        iter->second->_parent = nullptr;
        _options.erase(iter);
        return OK;
    }
    printf("Command %s remove option failed, option %s not found.\n", _name.data(), option_name.data());
    return ERROR;
}

auto command::get_subcommand(const std::string& subcommand_name) -> bee::command*
{
    auto iter = _subcommands.find(subcommand_name);
    return iter != _subcommands.end() ? iter->second : nullptr;
}

void command::add_subcommand(const std::string& subcommand_name, bee::command* subcommand)
{
    remove_subcommand(subcommand_name);
    _subcommands.emplace(subcommand_name, subcommand);
    subcommand->_parent = this;
}

void command::remove_subcommand(const std::string& subcommand_name)
{
    if(auto iter = _subcommands.find(subcommand_name); iter != _subcommands.end())
    {
        iter->second->_parent = nullptr;
        delete iter->second;
        _subcommands.erase(iter);
    }
}

void command::print_help()
{
    bee::ostringstream os;
    os << "Usage: " << _name << " [option][subcommand]\n";
    os << _description << "\n";
    if(_options.size())
    {
        os << "Options:" << "\n";
        for(const auto& [option_name, option] : _options)
        {
            os << format_string("  %-8s = %s\n", option_name.data(), option->get_description().data());
        }
    }
    if(_subcommands.size())
    {
        os << "Subcommand:" << "\n";
        for(const auto& [subcommand_name, subcommand] : _subcommands)
        {
            os << format_string("  %-8s = %s\n", subcommand_name.data(), subcommand->get_description().data());
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
    printf("%s", os.str().data());
}

void command::get_param_completions(const std::string& param, std::vector<std::string>& completions)
{

}

} // namespace bee