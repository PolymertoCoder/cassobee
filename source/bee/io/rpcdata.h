#pragma once
#include "format.h"
#include "marshal.h"

namespace bee
{
class ostringstream;

struct rpcdata : public marshal
{
    rpcdata() = default;
    virtual ~rpcdata() = default;
    virtual void dump(ostringstream& out) const;
};

ostringstream& operator<<(ostringstream& oss, const rpcdata& data);

} // namespace bee