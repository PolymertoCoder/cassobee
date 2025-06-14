#pragma once
#include <deque>
#include <random>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <string>
#include <algorithm>

namespace bee
{

// 随机数生成器类，提供多种随机数生成方法
// 线程安全，支持多种分布和类型
// 使用单例模式，确保每个线程有独立的随机数生成器实例
// 支持整数、浮点数、正态分布、指数分布、伽马分布、泊松分布等
// 支持从容器中随机选择元素、打乱容器顺序、生成随机字符串、颜色、日期等

class random
{
public:
    // 初始化（可指定种子）
    static void init_seed(uint64_t seed = std::random_device{}());

    // 基础随机数生成（简化模板特化）
    template<typename T>
    static T generate()
    {
        auto& inst = get_instance();
        if constexpr(std::is_integral_v<T>)
        {
            return inst.template generate_impl<T>();
        }
        else if constexpr(std::is_floating_point_v<T>)
        {
            return inst.template generate_impl<T>();
        }
        else
        {
            static_assert(sizeof(T) == 0, "Unsupported type for random generation");
        }
    }

    // 范围随机数（统一接口）
    template<typename T>
    static T range(T min, T max)
    {
        auto& inst = get_instance();
        return inst.template range_impl<T>(min, max);
    }

    // 正态分布
    template<typename T>
    static T normal(T mean, T stddev)
    {
        static_assert(std::is_floating_point_v<T>, 
                     "Normal distribution requires floating point type");
        std::normal_distribution<T> dist(mean, stddev);
        return dist(get_instance()._engine);
    }

    // 指数分布
    template<typename T>
    static T exponential(T lambda)
    {
        static_assert(std::is_floating_point_v<T>, 
                     "Exponential distribution requires floating point type");
        std::exponential_distribution<T> dist(lambda);
        return dist(get_instance()._engine);
    }

    // 伽马分布
    template<typename T>
    static T gamma(T alpha, T beta)
    {
        static_assert(std::is_floating_point_v<T>, 
                     "Gamma distribution requires floating point type");
        std::gamma_distribution<T> dist(alpha, beta);
        return dist(get_instance()._engine);
    }

    // 泊松分布
    static int poisson(double mean);

    // 伯努利分布
    static bool bernoulli(double p = 0.5);

    // 从容器中随机选择一个元素
    template<typename Container>
    static auto choice(const Container& container) -> typename Container::value_type
    {
        if(container.empty())
        {
            return typename Container::value_type(); // 返回默认值
        }
        
        auto& inst = get_instance();
        std::uniform_int_distribution<size_t> dist(0, container.size() - 1);
        size_t index = dist(inst._engine);
        
        auto it = std::begin(container);
        std::advance(it, index);
        return *it;
    }

    // 从初始化列表中随机选择
    template<typename T>
    static T choice(std::initializer_list<T> list)
    {
        return choice(std::vector<T>(list));
    }

    // 从C风格数组中随机选择
    template<typename T, size_t N>
    static T choice(T (&array)[N])
    {
        std::uniform_int_distribution<size_t> dist(0, N - 1);
        return array[dist(get_instance()._engine)];
    }

    // 从容器中随机选择指定数量的元素
    template<typename Container>
    static auto select(const Container& container, size_t count) 
        -> std::vector<typename Container::value_type>
    {
        if(count == 0 || container.empty())
        {
            return {};
        }
        
        auto& inst = get_instance();
        std::vector<typename Container::value_type> result;
        std::sample(std::begin(container), std::end(container), 
                    std::back_inserter(result), count, inst._engine);
        return result;
    }

    // 从初始化列表中随机选择指定数量的元素
    template<typename T>
    static T select(std::initializer_list<T> list, size_t count)
    {
        return select(std::vector<T>(list), count);
    }

    // 从C风格数组中随机选择指定数量的元素
    template<typename T, size_t N>
    static T select(T (&array)[N], size_t count)
    {
        if(count == 0 || N == 0)
        {
            return {};
        }
        
        auto& inst = get_instance();
        std::vector<T> result;
        std::sample(array, array + N, std::back_inserter(result), 
                    count, inst._engine);
        return result;
    }

    // 打乱容器元素顺序
    template<typename Container>
    static void shuffle(Container& container)
    {
        std::shuffle(std::begin(container), std::end(container), 
                     get_instance()._engine);
    }

    // 返回打乱后的新容器
    template<typename Container>
    static Container shuffled(const Container& container)
    {
        Container copy = container;
        shuffle(copy);
        return copy;
    }

    // 生成随机字符串
    static std::string random_string(size_t length, bool include_upper = true, bool include_lower = true, bool include_digits = true, bool include_special = false);

    // 生成随机文件名
    static std::string random_filename(size_t length = 12, const std::string& extension = "");

    // 生成随机颜色（RGB格式）
    static std::tuple<uint8_t, uint8_t, uint8_t> random_color();

    // 生成随机日期（范围：1900-01-01 到 2100-12-31）
    static std::tuple<int, int, int> random_date();

    // 概率判断（返回true的概率为p）
    static bool probability(double p);

    // 生成唯一ID（UUID格式）
    static std::string uuid();

    // 随机字节序列
    static std::vector<uint8_t> random_bytes(size_t count);

    // 随机权重选择
    template<typename T>
    static const T& weighted_choice(const std::vector<T>& items, const std::vector<double>& weights)
    {
        if(items.size() != weights.size() || items.empty())
        {
            return items.empty() ? T() : items.front(); // 返回默认值或第一个元素
        }
        
        double total_weight = 0.0;
        for(double w : weights)
        {
            total_weight += w;
        }
        
        auto& eng = get_instance()._engine;
        std::uniform_real_distribution<double> dist(0.0, total_weight);
        double r = dist(eng);
        
        double cumulative = 0.0;
        for (size_t i = 0; i < weights.size(); ++i)
        {
            cumulative += weights[i];
            if(r <= cumulative) 
            {
                return items[i];
            }
        }
        
        return items.back();
    }

private:
    std::mt19937_64 _engine;

    random();

    static random& get_instance();

    void initialize(uint64_t seed);

    // 统一的基础生成实现
    template<typename T>
    T generate_impl()
    {
        if constexpr(std::is_integral_v<T>)
        {
            std::uniform_int_distribution<T> dist;
            return dist(_engine);
        }
        else if constexpr(std::is_floating_point_v<T>)
        {
            std::uniform_real_distribution<T> dist(0, 1);
            return dist(_engine);
        }
    }

    // 统一的范围生成实现
    template<typename T>
    T range_impl(T min, T max)
    {
        if constexpr(std::is_integral_v<T>)
        {
            std::uniform_int_distribution<T> dist(min, max);
            return dist(_engine);
        }
        else if constexpr(std::is_floating_point_v<T>)
        {
            std::uniform_real_distribution<T> dist(min, max);
            return dist(_engine);
        }
    }

    // 字符串生成辅助函数
    static std::string generate_string(size_t length, const std::string& charset);

    // 闰年判断
    static bool is_leap_year(int year);
};

// 独立随机引擎
class random_engine 
{
public:
    explicit random_engine(uint64_t seed = std::random_device{}());

    // 重新播种
    void seed(uint64_t seed);

    // 基础随机数生成
    template<typename T>
    T generate()
    {
        if constexpr(std::is_integral_v<T>)
        {
            std::uniform_int_distribution<T> dist;
            return dist(_engine);
        }
        else if constexpr(std::is_floating_point_v<T>)
        {
            std::uniform_real_distribution<T> dist(0, 1);
            return dist(_engine);
        }
    }

    // 范围随机数
    template<typename T>
    T range(T min, T max)
    {
        if constexpr(std::is_integral_v<T>)
        {
            std::uniform_int_distribution<T> dist(min, max);
            return dist(_engine);
        }
        else if constexpr(std::is_floating_point_v<T>)
        {
            std::uniform_real_distribution<T> dist(min, max);
            return dist(_engine);
        }
    }

    // 正态分布
    template<typename T>
    T normal(T mean, T stddev)
    {
        return random::normal(mean, stddev);
    }

    // 指数分布
    template<typename T>
    T exponential(T lambda)
    {
        return random::exponential(lambda);
    }

private:
    std::mt19937_64 _engine;
};

} // namespace bee