#include "glog.h"
#include "logger.h"
#include "remotelog.h"
#include "logserver_manager.h"

namespace bee
{

thread_local bee::remotelog g_remotelog;

void logclient::set_logserver(logserver_manager* logserver)
{
    _logserver = logserver;
}

void logclient::commit_log(LOG_LEVEL level, const log_event& event)
{
    g_remotelog.loglevel = level;
    g_remotelog.logevent = event;
    if(_is_logserver)
    {
        g_remotelog.run();
        return;
    }
    if(_logserver && _logserver->is_connect())
    {
        _logserver->send(g_remotelog);
    }
    else
    {
        _console_logger->log(level, event);
    }
}

} // namespace bee