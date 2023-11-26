
#include <iostream>
#include "threadpool.h"
 
int main()
{
    ThreadPool pool(4);
    auto result = pool.enqueue([](int answer) { return answer;  }, 42);
    std::cout << result.get() << std::endl; 
}
