#pragma once
#include "servlet.h"
#include "config.h"

namespace bee
{

class config_servlet : public servlet
{
public:
    config_servlet() : servlet("config") {}
    ~config_servlet() override = default;

    virtual int handle(httprequest* req, httpresponse* rsp, httpsession* ses) override;
    virtual config_servlet* dup() const override;
};

} // namespace