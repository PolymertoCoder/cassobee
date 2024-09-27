#pragma once
#include "marshal.h"

struct rpcdata : public marshal
{
    virtual ~rpcdata() = default;
};