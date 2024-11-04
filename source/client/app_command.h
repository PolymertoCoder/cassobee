#pragma once
#include "command.h"
#include "config.h"
#include "server_manager.h"
#include <filesystem>

namespace fs = std::filesystem;

namespace cli
{

class config_command : public cli::command
{
public:
    class config_reload_command : public cli::command
    {
    public:
        config_reload_command(const std::string& name, const std::string& description) : command(name, description) {}
        virtual int do_execute(const std::vector<std::string>& params) override
        {
            if(params.empty())
            {
                auto current_path = fs::current_path();

            }
            else if(params.size() == 1)
            {
                config::get_instance()->init(params[0].data());
                return OK;
            }
            return ERROR;
        }
    };
public:
    config_command(const std::string& name, const std::string& description) : command(name, description)
    {
        add_subcommand("reload", new config_reload_command("reload", "reload config"));
    }
    virtual int do_execute(const std::vector<std::string>& params) override
    {
        if(params.empty())
        {
            
            //printf("Use default config file %s\n", );
            //config::get_instance()->init();
            return OK;
        }
        else if(params.size() == 1)
        {
            printf("Use config file %s\n", params[0].data());
            config::get_instance()->init(params[0].data());
            return OK;
        }
        else
        {
            return ERROR;
        }
        
    }
};

class connect_command : public cli::command
{
public:
    connect_command(const std::string& name, const std::string& description) : command(name, description) {}
    virtual int do_execute(const std::vector<std::string>& params) override
    {
        auto servermgr = server_manager::get_instance();
        servermgr->init();
        client(servermgr);
        return OK;
    }
};

} // namespace cli