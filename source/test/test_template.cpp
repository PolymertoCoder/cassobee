#include <iostream>
#include <set>
#include <map>
#include <string>
#include <type_traits>

namespace cassobee
{

template<typename T>
std::string ToString(const std::set<T>& val);
template<typename T1, typename T2>
std::string ToString(const std::map<T1, T2>& val);

template<typename T>
auto ToString(T val) -> typename std::enable_if<std::is_arithmetic<T>::value, std::string>::type
{
    return std::to_string(val);
}

template<typename T1, typename T2>
std::string ToString(const std::pair<T1, T2>& val)
{
    std::string str("<");
    str.append(ToString(val.first) + ", ");
    str.append(ToString(val.second) + ">");
    return str;
}

template<typename STL_CONTAINER>
std::string ContainerToString(const STL_CONTAINER& container, const std::string_view& prefix)
{
    std::string str(prefix.data(), prefix.size());
    size_t i = 0, sz = container.size();
    for(typename STL_CONTAINER::const_iterator iter = container.begin(); iter != container.end(); ++iter, ++i)
    {
        str.append(ToString(*iter));
        if(i != sz-1){ str.append(", "); }
    }
    return str.append("}");
}

template<typename T1, typename T2>
std::string ToString(const std::map<T1, T2>& val)
{
    return ContainerToString(val, "std::map:{");
}
template<typename T>
std::string ToString(const std::set<T>& val)
{
    return ContainerToString(val, "std::set:{");
}

}

void test()
{
#if 0
    std::map<int, std::set<int>> temp;
    temp[1000].insert(2000);
    temp[1000].insert(3000);
    temp[4000].insert(5000);
    temp[4000].insert(6000);
    std::cout << ToString(temp) << std::endl;
#elif 0
    std::map<int, int> temp;
    temp[1000] = 2000;
    temp[3000] = 4000;
    temp[5000] = 6000;
    std::cout << ToString(temp) << std::endl;
#elif 1
    std::map<int, std::map<int, std::map<int, int>>> temp;
    temp[1][2][1] = 1;
    temp[1][2][2] = 2;
    temp[1][2][3] = 3;
    temp[4][5][6] = 2;
    std::cout << ToString(temp) << std::endl;
#endif
}

int main()
{
    test();
    return 0;
}
