#include "rpcdata.h"

namespace bee
{

void rpcdata::dump(ostringstream& out) const
{
}

ostringstream& operator<<(ostringstream& oss, const rpcdata& data)
{
    data.dump(oss);
    return oss;
}

} // namespace bee