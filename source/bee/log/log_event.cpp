#include "log_event.h"

namespace bee
{

octetsstream& log_event::pack(octetsstream& os) const
{
    os << code;
    if (code & FIELDS_PROCESS_NAME) os << process_name;
    if (code & FIELDS_FILENAME) os << filename;
    if (code & FIELDS_LINE) os << line;
    if (code & FIELDS_TIMESTAMP) os << timestamp;
    if (code & FIELDS_THREADID) os << threadid;
    if (code & FIELDS_FIBERID) os << fiberid;
    if (code & FIELDS_ELAPSE) os << elapse;
    if (code & FIELDS_CONTENT) os << content;
    return os;
}

octetsstream& log_event::unpack(octetsstream& os)
{
    os >> code;
    if (code & FIELDS_PROCESS_NAME) os >> process_name;
    if (code & FIELDS_FILENAME) os >> filename;
    if (code & FIELDS_LINE) os >> line;
    if (code & FIELDS_TIMESTAMP) os >> timestamp;
    if (code & FIELDS_THREADID) os >> threadid;
    if (code & FIELDS_FIBERID) os >> fiberid;
    if (code & FIELDS_ELAPSE) os >> elapse;
    if (code & FIELDS_CONTENT) os >> content;
    return os;
}

} // namespace bee