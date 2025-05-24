#pragma once
#include "format.h"
#include "marshal.h"

namespace bee
{
class ostringstream;

class rpcdata : public marshal
{
public:
    rpcdata() = default;
    rpcdata(const rpcdata&) = default;
    bool operator==(const rpcdata& rhs) { return true; }
    virtual ~rpcdata() = default;

    virtual rpcdata* dup() const = 0;
    virtual ostringstream& dump(ostringstream& out) const;
};

ostringstream& operator<<(ostringstream& oss, const rpcdata& data);

} // namespace bee