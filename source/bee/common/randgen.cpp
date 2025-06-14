#include "randgen.h"

namespace bee
{

void random::init_seed(uint64_t seed)
{
    get_instance().initialize(seed);
}

int random::poisson(double mean)
{
    std::poisson_distribution<int> dist(mean);
    return dist(get_instance()._engine);
}

bool random::bernoulli(double p)
{
    std::bernoulli_distribution dist(p);
    return dist(get_instance()._engine);
}

std::string random::random_string(size_t length, bool include_upper, bool include_lower, bool include_digits, bool include_special)
{
    static const char* upper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static const char* lower = "abcdefghijklmnopqrstuvwxyz";
    static const char* digits = "0123456789";
    static const char* special = "!@#$%^&*()_+-=[]{}|;:,.<>?";
    
    std::string charset;
    if(include_upper) charset.append(upper);
    if(include_lower) charset.append(lower);
    if(include_digits) charset.append(digits);
    if(include_special) charset.append(special);
    
    if(charset.empty())
    {
        return ""; // 如果没有指定字符集，返回空字符串
    }
    
    return generate_string(length, charset);
}

std::string random::random_filename(size_t length, const std::string& extension)
{
    static const char safe_chars[] = "abcdefghijklmnopqrstuvwxyz0123456789";
    std::string name = generate_string(length, safe_chars);
    return extension.empty() ? name : name + "." + extension;
}

std::tuple<uint8_t, uint8_t, uint8_t> random::random_color()
{
    auto& eng = get_instance()._engine;
    std::uniform_int_distribution<uint16_t> dist(0, 255);
    return {static_cast<uint8_t>(dist(eng)),
            static_cast<uint8_t>(dist(eng)),
            static_cast<uint8_t>(dist(eng))};
}

std::tuple<int, int, int> random::random_date()
{
    auto& eng = get_instance()._engine;
    std::uniform_int_distribution<int> year_dist(1900, 2100);
    std::uniform_int_distribution<int> month_dist(1, 12);
    
    int year = year_dist(eng);
    int month = month_dist(eng);
    
    // 处理不同月份的天数
    static constexpr int days_in_month[] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };
    
    int day_max = days_in_month[month - 1];
    if(month == 2 && is_leap_year(year)) {
        day_max = 29;
    }
    
    std::uniform_int_distribution<int> day_dist(1, day_max);
    return {year, month, day_dist(eng)};
}

bool random::probability(double p)
{
    return bernoulli(p);
}

std::string random::uuid()
{
    auto& eng = get_instance()._engine;
    std::uniform_int_distribution<int> hex_dist(0, 15);
    std::string uuid = "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx";
    
    for(char& c : uuid)
    {
        if(c == 'x')
        {
            c = "0123456789abcdef"[hex_dist(eng)];
        }
        else if(c == 'y')
        {
            c = "89ab"[hex_dist(eng) & 0x3]; // 8,9,A,B
        }
    }
    return uuid;
}

std::vector<uint8_t> random::random_bytes(size_t count)
{
    std::vector<uint8_t> bytes(count);
    auto& eng = get_instance()._engine;
    std::uniform_int_distribution<uint16_t> dist(0, 255);
    
    for(auto& byte : bytes)
    {
        byte = static_cast<uint8_t>(dist(eng));
    }
    return bytes;
}

random::random()
{
    init_seed();
}

random& random::get_instance()
{
    thread_local random instance;
    return instance;
}

void random::initialize(uint64_t seed)
{
    _engine.seed(seed);
}

std::string random::generate_string(size_t length, const std::string& charset)
{
    auto& eng = get_instance()._engine;
    std::uniform_int_distribution<size_t> dist(0, charset.size() - 1);
    std::string result;
    result.reserve(length);
    
    for(size_t i = 0; i < length; ++i)
    {
        result += charset[dist(eng)];
    }
    
    return result;
}

bool random::is_leap_year(int year)
{
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

random_engine::random_engine(uint64_t seed) 
        : _engine(seed) {}

void random_engine::seed(uint64_t seed)
{
    _engine.seed(seed);
}

} // namespace bee