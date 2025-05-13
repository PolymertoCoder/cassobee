#include "rpcdata.h"

namespace bee
{

ostringstream& rpcdata::dump(ostringstream& out) const
{
    return out;
}

ostringstream& operator<<(ostringstream& oss, const rpcdata& data)
{
    data.dump(oss);
    return oss;
}

} // namespace bee