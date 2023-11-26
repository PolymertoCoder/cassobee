#include <unistd.h>
#include "threadpool.h"

int main()
{
    auto tpool = threadpool::get_instance();
    tpool->start();

    for(int i = 0; i < 20; ++i)
    {
        tpool->add_task(1, [&i]()
                {
                    i = i*(i-1)*(i-2)*(i-3)*(i-4);
                    printf("result:%d.\n", i);
                }); 
    }

    //sleep(5);

    tpool->stop();

    return 0;
}
