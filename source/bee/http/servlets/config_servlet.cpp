#include "config_servlet.h"
#include "httpprotocol.h"

namespace bee
{

int config_servlet::handle(httprequest* req, httpresponse* rsp)
{
    auto cfg = config::get_instance();
    std::string section = req->get_param("section");
    std::string key = req->get_param("key");
    std::string value = cfg->get<std::string>(section, key);
    rsp->set_body(value);
    rsp->set_status(HTTP_STATUS_OK);
    return 0;
}

config_servlet* config_servlet::dup() const
{
    return new config_servlet(*this);
}

} // namespace bee