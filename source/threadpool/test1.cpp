#include <iostream>
#include <ctime>
#include "threadpool.h"
#include "../util.h"

int add_test(int a)
{
    int temp = 1;
    for(size_t i = 0; i < 10000; ++i){ temp += (a+1) * (a+3) * (a+5) * (a+7) * temp; }
    return temp;
}
 
int main()
{
        {
            auto pool = threadpool::get_instance();
            pool->start();
            GET_TIME_BEGIN()
            for( size_t i = 0; i < 1000000; i++ )
            {
                pool->add_task(i%4, [i](){ add_test(i); });
            }
            pool->stop();
            GET_TIME_END()

            pool->stop();
        }
}
