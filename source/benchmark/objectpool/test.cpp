#include <set>
#include <map>
#include <vector>
#include <cstdio>
#include "../common/util.h"
#include "../common/objectpool.h"

class bigtestobj
{
public:
    std::map<int, int> _1;
    std::map<int, int> _2;
    std::map<int, int> _3;
    std::map<int, int> _4;
    std::map<int, int> _5;
    std::set<int> _6;
    std::set<int> _7;
    std::set<int> _8;
    std::set<int> _9;
    std::set<int> _10;
};

class smalltestobj
{
public:
    std::vector<int> _1;
    std::vector<int> _2;
};

#define TESTCOUNT 1000000

int main()
{
    {
        auto* objpool = new objectpool<smalltestobj>(TESTCOUNT);
        std::vector<size_t> vec(TESTCOUNT);
        GET_TIME_BEGIN();
        for(size_t i = 0; i < TESTCOUNT; ++i)
        {
            auto retpair = objpool->alloc();
            vec.push_back(retpair.first);
        }
        for(size_t i = 0; i < TESTCOUNT; ++i)
        {
            if(vec[i]){ objpool->free(vec[i]); }
        }
        vec.clear();
        for(size_t i = 0; i < TESTCOUNT; ++i)
        {
            auto retpair = objpool->alloc();
            vec.push_back(retpair.first);
        }
        for(size_t i = 0; i < TESTCOUNT; ++i)
        {
            if(vec[i]){ objpool->free(vec[i]); }
        }
        GET_TIME_END();
    }
    {
        std::vector<smalltestobj*> vec(TESTCOUNT);
        GET_TIME_BEGIN();
        for(size_t i = 0; i < TESTCOUNT; ++i)
        {
            smalltestobj* obj = new smalltestobj;
            vec.push_back(obj);
        }
        for(size_t i = 0; i < TESTCOUNT; ++i)
        {
            if(vec[i]){ delete vec[i]; }
        }
        vec.clear();
        for(size_t i = 0; i < TESTCOUNT; ++i)
        {
            smalltestobj* obj = new smalltestobj;
            vec.push_back(obj);
        }
        for(size_t i = 0; i < TESTCOUNT; ++i)
        {
            if(vec[i]){ delete vec[i]; }
        }
        GET_TIME_END();
    }
    {
        auto* objpool = new objectpool<bigtestobj>(TESTCOUNT);
        std::vector<size_t> vec(TESTCOUNT);
        GET_TIME_BEGIN();
        for(size_t i = 0; i < TESTCOUNT; ++i)
        {
            auto retpair = objpool->alloc();
            vec.push_back(retpair.first);
        }
        for(size_t i = 0; i < TESTCOUNT; ++i)
        {
            if(vec[i]){ objpool->free(vec[i]); }
        }
        vec.clear();
        for(size_t i = 0; i < TESTCOUNT; ++i)
        {
            auto retpair = objpool->alloc();
            vec.push_back(retpair.first);
        }
        for(size_t i = 0; i < TESTCOUNT; ++i)
        {
            if(vec[i]){ objpool->free(vec[i]); }
        }
        GET_TIME_END();
    }
    {
        std::vector<bigtestobj*> vec(TESTCOUNT);
        GET_TIME_BEGIN();
        for(size_t i = 0; i < TESTCOUNT; ++i)
        {
            bigtestobj* obj = new bigtestobj;
            vec.push_back(obj);
        }
        for(size_t i = 0; i < TESTCOUNT; ++i)
        {
            if(vec[i]){ delete vec[i]; }
        }
        vec.clear();
        for(size_t i = 0; i < TESTCOUNT; ++i)
        {
            bigtestobj* obj = new bigtestobj;
            vec.push_back(obj);
        }
        for(size_t i = 0; i < TESTCOUNT; ++i)
        {
            if(vec[i]){ delete vec[i]; }
        }
        GET_TIME_END();
    }
    return 0;
}
