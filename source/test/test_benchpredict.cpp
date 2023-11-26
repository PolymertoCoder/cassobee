#include <vector>
#include "../util.h"
#include "../macros.h"

int main()
{
#define VECLEN 999999
#define TESTLEN ((VECLEN) / (10))
    std::vector<int> vec(VECLEN, 0);
    for(size_t i = 0; i < VECLEN; ++i)
    {
        vec[i] = i <= TESTLEN ? 1 : 0;
    }

    {
        GET_TIME_BEGIN();
        for(size_t i = 0; i < VECLEN; ++i)
        {
            if((vec[i]))
            {
                for(size_t j = 0; j < 20; ++j)
                {
                    int a = vec[i] * 9999 * 8888 * 7777 * 6666 * 5555 * 4444 *3333 * 2222 * 1111;
                }
            }
            else
            {
                for(size_t j = 0; j < 999; ++j)
                {
                    int a = vec[i] * 9999 * 8888 * 7777 * 6666 * 5555 * 4444 *3333 * 2222 * 1111;
                }
            }
        }
        GET_TIME_END();
    }

    {
        GET_TIME_BEGIN();
        for(size_t i = 0; i < VECLEN; ++i)
        {
            if(PREDICT_FALSE(vec[i]))
            {
                for(size_t j = 0; j < 20; ++j)
                {
                    int a = vec[i] * 9999 * 8888 * 7777 * 6666 * 5555 * 4444 *3333 * 2222 * 1111;
                }
            }
            else
            {
                for(size_t j = 0; j < 999; ++j)
                {
                    int a = vec[i] * 9999 * 8888 * 7777 * 6666 * 5555 * 4444 *3333 * 2222 * 1111;
                }
            }
        }
        GET_TIME_END();
    }

    return 0;
}
