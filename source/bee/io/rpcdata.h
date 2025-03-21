#pragma once
#include "marshal.h"

namespace bee
{

struct rpcdata : public marshal
{
    virtual ~rpcdata() = default;
};

} // namespace bee