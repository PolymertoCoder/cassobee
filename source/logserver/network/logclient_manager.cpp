#include "logclient_manager.h"

namespace bee
{

void logclient_manager::on_add_session(SID sid)
{
    printf("logclient_manager on_add_session sid=%lu\n", sid);
}

void logclient_manager::on_del_session(SID sid)
{
    printf("logclient_manager on_del_session sid=%lu\n", sid);
}

} // namespace bee