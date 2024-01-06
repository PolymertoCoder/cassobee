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
            // create thread pool with 4 worker threads
            thread_pool pool(4);
            pool.init();
              
            GET_TIME_BEGIN()
            std::vector<std::future<int>> ress;
            for( size_t i = 0; i < 1000000; i++ )
            {
                // enqueue and store future
                //auto result = pool.add_task([](int answer) { return answer; }, 42);
                //auto result1 = pool.add_task([](int answer) { return answer; }, 96);
                ress.emplace_back( pool.add_task(add_test, i) );
                //auto result3 = pool.add_task([](int a, int b, int c) { return a+b+c; }, temp, 10, i);
                               
                // get result from future, print 42
                //std::cout << result.get() << std::endl; 
                //std::cout << result1.get() << std::endl; 
                //std::cout << temp << std::endl; 
                //std::cout << result3.get() << std::endl; 
            }
            for(size_t i = 0; i < 1000000; i++)
            {
                int temp = ress[i].get();
            }

            if( pool.wait_for_all_done(0) )
            {
                pool.stop();
            }
            GET_TIME_END()
        }
        //{
        //    GET_TIME_BEGIN()
        //    for( size_t i = 0; i < 1000000; i++ )
        //    {
        //        int temp = add_test(i);
        //    }
        //    GET_TIME_END()
        //}
}
