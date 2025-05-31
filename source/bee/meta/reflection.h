// reflection.h - 核心反射数据结构
#pragma once
#include <array>
#include <span>
#include <string_view>
#include <cstddef>
#include <utility>
#include <type_traits>

namespace meta { // 未来替换为 std::meta

// 前置声明
template <typename T>
struct type_info;

// 基础类型描述符
struct base_info {
    template <typename T>
    static consteval const type_info<T>& get() noexcept {
        return type_info<T>::reflect();
    }
};

// 成员变量描述符
struct member_info {
    std::string_view name;
    size_t offset;
    
    template <typename T>
    static consteval const type_info<T>& type() noexcept {
        return type_info<T>::reflect();
    }
};

// 成员方法描述符
struct method_info {
    std::string_view name;
    bool is_constexpr : 1;
    bool is_static : 1;
    bool is_const : 1;
    bool is_virtual : 1;
    
    template <typename Sig>
    static consteval const type_info<Sig>& signature() noexcept {
        return type_info<Sig>::reflect();
    }
};

// 枚举值描述符
struct enumerator_info {
    std::string_view name;
    long long value;
};

// 类型特征模板
template <typename T>
struct type_info {
    // 必须在特化中实现的特征
    static consteval std::string_view name() noexcept;
    static consteval std::span<const member_info> members() noexcept;
    static consteval std::span<const method_info> methods() noexcept;
    static consteval std::span<const base_info> bases() noexcept;
    
    // 可选特征（自动推导）
    static consteval bool is_class() noexcept {
        return !bases().empty() || !members().empty();
    }
    
    static consteval bool is_enum() noexcept {
        return !enumerators().empty();
    }
    
    static consteval std::span<const enumerator_info> enumerators() noexcept {
        return {};
    }

    // 字段访问支持
    template <typename FieldType>
    static consteval FieldType& access(void* obj, std::string_view name) {
        for (const auto& m : members()) {
            if (m.name == name) {
                return *reinterpret_cast<FieldType*>(static_cast<char*>(obj) + m.offset);
            }
        }
        struct field_not_found {};
        throw field_not_found{}; // 编译时错误
    }
};

// 编译时字段访问
template <typename T, typename FieldType>
consteval FieldType& field_access(T& obj, std::string_view name) {
    return type_info<T>::template access<FieldType>(&obj, name);
}

} // namespace meta