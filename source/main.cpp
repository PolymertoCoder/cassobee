#include <list>
#include <string>
#include <utility>

#include "reactor.h"
#include "threadpool.h"
#include "delegate.h"

SINGLE_DELEGATE_DECLEAR(test_single_t, int, int, int)
MULTICAST_DELEGATE_DECLEAR(test_multi_t, int, int)
NUMEROUS_MULTICAST_DELEGATE_DECLEAR(test_nmrs_multi_t, int, int)

int main()
{
    test_single_t test_single;
    bool ret = test_single.bind([](int a, int b){ return a+b; });
    if(ret)
    {
        printf("single_delegate result:%d.\n", test_single.execute(10, 20));
    }

    test_multi_t test_multi;
    int id1 = test_multi.bind([](int a, int b){ printf("test_multi result1:%d\n", a+b); });
    int id2 = test_multi.bind([](int a, int b){ printf("test_multi result2:%d\n", a-b); });
    int id3 = test_multi.bind([](int a, int b){ printf("test_multi result3:%d\n", a*b); });
    int id4 = test_multi.bind([](int a, int b){ printf("test_multi result4:%d\n", a/b); });
    printf("test_multi add %d %d %d %d\n", id1, id2, id3, id4);
    test_multi.broadcast(10, 20);

    test_nmrs_multi_t test_nmrs_multi;
    id1 = test_nmrs_multi.bind([](int a, int b){ printf("test_multi result1:%d\n", a+b); });
    id2 = test_nmrs_multi.bind([](int a, int b){ printf("test_multi result2:%d\n", a-b); });
    id3 = test_nmrs_multi.bind([](int a, int b){ printf("test_multi result3:%d\n", a*b); });
    id4 = test_nmrs_multi.bind([](int a, int b){ printf("test_multi result4:%d\n", a/b); });
    printf("test_multi add %d %d %d %d\n", id1, id2, id3, id4);
    test_nmrs_multi.broadcast(100, 200);

    return 0;
}
