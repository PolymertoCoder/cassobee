#include "influxlog_event.h"

namespace bee
{

octetsstream& influxlog_event::pack(octetsstream& os) const
{
    os << measurement;
    os << tags;
    os << fields;
    os << timestamp;
    return os;
}

octetsstream& influxlog_event::unpack(octetsstream& os)
{
    os >> measurement;
    os >> tags;
    os >> fields;
    os >> timestamp;
    return os;
}

ostringstream& influxlog_event::dump(ostringstream& out) const
{
    rpcdata::dump(out);
    out << "measurement: " << measurement;
    out << "tags: " << tags;
    out << "fields: " << fields;
    out << "timestamp: " << timestamp;
    return out;
}

} // namespace bee