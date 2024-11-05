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
                return config::get_instance()->reload() ? OK : ERROR;
            }
            else if(params.size() == 1)
            {
                config::get_instance()->init(params[0].data());
                return OK;
            }
            else
            {
                return ERR_TOO_MUCH_PARAMS_COUNT;
            }
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
            auto root_path   = fs::current_path().parent_path();
            auto config_file = root_path/"config"/"client.conf";
            printf("Use default config file \"%s\" because not specify config file.", config_file.c_str());
            config::get_instance()->init(config_file.c_str());
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
            return ERR_TOO_MUCH_PARAMS_COUNT;
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