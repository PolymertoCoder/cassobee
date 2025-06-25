#include "glog.h"
#include "logger.h"
#include "remotelog.h"
#include "remoteinfluxlog.h"
#include "logserver_manager.h"
#ifdef _REENTRANT
#include "threadpool.h"
#endif

namespace bee
{

thread_local bee::remotelog g_remotelog;
thread_local bee::remoteinfluxlog g_influxremotelog;

void logclient::set_logserver(logserver_manager* logserver)
{
    _logserver = logserver;
}

void logclient::commit_log(LOG_LEVEL level, const log_event& event)
{
    if(_is_logserver)
    {
        g_remotelog.loglevel = level;
        g_remotelog.logevent = event;
    #ifdef _REENTRANT
        threadpool::get_instance()->add_task(g_remotelog.thread_group_idx(), g_remotelog.dup());
    #else
        g_remotelog.run();
    #endif
    }
    else // logclient
    {
        if(_logserver && _logserver->is_connect())
        {
            g_remotelog.loglevel = level;
            g_remotelog.logevent = event;
            _logserver->send(g_remotelog);
        }
        else
        {
            _console_logger->log(level, event);
        }
    }
}

void logclient::commit_influxlog(const influxlog_event& event)
{
    if(_is_logserver)
    {
        g_influxremotelog.logevent = event;
    #ifdef _REENTRANT
        threadpool::get_instance()->add_task(g_remotelog.thread_group_idx(), g_influxremotelog.dup());
    #else
        g_influxremotelog.run();
    #endif
    }
    else // logclient
    {
        if(_logserver && _logserver->is_connect())
        {
            g_influxremotelog.logevent = event;
            _logserver->send(g_remotelog);
        }
    }
}

} // namespace bee