#include <clang-c/Index.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <cstdlib>
#include <unordered_map>
#include <algorithm>

namespace fs = std::filesystem;

// 工具函数：转换CXString到std::string
std::string to_std_string(CXString cxstr) {
    if (const char* cstr = clang_getCString(cxstr)) {
        std::string str(cstr);
        clang_disposeString(cxstr);
        return str;
    }
    return "";
}

// 获取完全限定名
std::string get_qualified_name(CXCursor cursor) {
    std::string result = to_std_string(clang_getCursorSpelling(cursor));
    CXCursor parent = clang_getCursorSemanticParent(cursor);

    while (!clang_Cursor_isNull(parent) &&
           clang_getCursorKind(parent) != CXCursor_TranslationUnit) {
        std::string parentName = to_std_string(clang_getCursorSpelling(parent));
        if (!parentName.empty()) {
            result = parentName + "::" + result;
        }
        parent = clang_getCursorSemanticParent(parent);
    }

    return result;
}

// 提取类型名称（处理模板特化）
std::string get_type_name(CXType type) {
    std::string name = to_std_string(clang_getTypeSpelling(type));
    
    // 简化标准库模板实例
    size_t pos = name.find('<');
    if (pos != std::string::npos) {
        std::string base = name.substr(0, pos);
        if (base == "std::basic_string") {
            return "std::string";
        } else if (base == "std::vector") {
            return "std::vector<" + get_type_name(clang_Type_getTemplateArgumentAsType(type, 0)) + ">";
        }
    }
    
    // 移除多余的空格
    name.erase(std::remove(name.begin(), name.end(), ' '), name.end());
    return name;
}

// 提取属性
struct attribute {
    std::string name;
    std::string args;
};

std::vector<attribute> extract_attributes(CXCursor cursor) {
    std::vector<attribute> attrs;
    
    clang_visitChildren(cursor, [](CXCursor c, CXCursor, CXClientData data) {
        if (clang_getCursorKind(c) == CXCursor_AnnotateAttr) {
            std::string attr = to_std_string(clang_getCursorSpelling(c));
            auto* vec = static_cast<std::vector<attribute>*>(data);
            
            size_t pos = attr.find('(');
            if (pos != std::string::npos) {
                std::string name = attr.substr(0, pos);
                std::string args = attr.substr(pos + 1, attr.size() - pos - 2);
                vec->push_back({name, args});
            } else {
                vec->push_back({attr, ""});
            }
        }
        return CXChildVisit_Continue;
    }, &attrs);
    
    return attrs;
}

// 反射生成器类
class ReflectionGenerator {
    std::ostream& out;
    CXTranslationUnit tu;
    std::unordered_map<std::string, std::string> generated_types;
    
public:
    ReflectionGenerator(std::ostream& os, CXTranslationUnit tu)
        : out(os), tu(tu) {}
    
    void generate() {
        out << "#pragma once\n#include \"reflection.h\"\n\n";
        out << "namespace meta {\n\n";
        
        CXCursor root = clang_getTranslationUnitCursor(tu);
        clang_visitChildren(root, [](CXCursor cursor, CXCursor, CXClientData data) {
            auto* self = static_cast<ReflectionGenerator*>(data);
            self->process_cursor(cursor);
            return CXChildVisit_Recurse;
        }, this);
        
        out << "} // namespace meta\n";
    }
    
private:
    void process_cursor(CXCursor cursor) {
        const CXCursorKind kind = clang_getCursorKind(cursor);
        
        // 处理类和结构体
        if (kind == CXCursor_ClassDecl || kind == CXCursor_StructDecl) {
            process_record(cursor);
        }
        // 处理枚举
        else if (kind == CXCursor_EnumDecl) {
            process_enum(cursor);
        }
    }
    
    void process_record(CXCursor cursor) {
        if (!clang_isCursorDefinition(cursor)) return;
        
        auto attrs = extract_attributes(cursor);
        bool should_reflect = std::any_of(attrs.begin(), attrs.end(), 
            [](const attribute& attr) { return attr.name == "reflect"; });
        
        if (!should_reflect) return;
        
        std::string name = get_qualified_name(cursor);
        if (generated_types.find(name) != generated_types.end()) return;
        
        generated_types[name] = name;
        
        out << "template<>\n";
        out << "struct type_info<" << name << "> {\n";
        
        // 类名
        out << "    static consteval std::string_view name() noexcept {\n";
        out << "        return \"" << name << "\";\n";
        out << "    }\n\n";
        
        // 基类
        out << "    static consteval std::span<const base_info> bases() noexcept {\n";
        out << "        static constexpr std::array bases{\n";
        
        clang_visitChildren(cursor, [](CXCursor c, CXCursor, CXClientData data) {
            if (clang_getCursorKind(c) == CXCursor_CXXBaseSpecifier) {
                CXType baseType = clang_getCursorType(c);
                std::string baseName = get_type_name(baseType);
                auto* out = static_cast<std::ostream*>(data);
                *out << "            base_info{},\n";
            }
            return CXChildVisit_Continue;
        }, &out);
        
        out << "        };\n";
        out << "        return bases;\n";
        out << "    }\n\n";
        
        // 成员变量
        out << "    static consteval std::span<const member_info> members() noexcept {\n";
        out << "        static constexpr std::array members{\n";
        
        clang_visitChildren(cursor, [](CXCursor c, CXCursor parent, CXClientData data) {
            if (clang_getCursorKind(c) == CXCursor_FieldDecl) {
                auto* self = static_cast<ReflectionGenerator*>(data);
                self->process_field(c, parent);
            }
            return CXChildVisit_Continue;
        }, this);
        
        out << "        };\n";
        out << "        return members;\n";
        out << "    }\n\n";
        
        // 成员函数
        out << "    static consteval std::span<const method_info> methods() noexcept {\n";
        out << "        static constexpr std::array methods{\n";
        
        clang_visitChildren(cursor, [](CXCursor c, CXCursor, CXClientData data) {
            if (clang_getCursorKind(c) == CXCursor_CXXMethod) {
                auto* self = static_cast<ReflectionGenerator*>(data);
                self->process_method(c);
            }
            return CXChildVisit_Continue;
        }, this);
        
        out << "        };\n";
        out << "        return methods;\n";
        out << "    }\n";
        
        out << "};\n\n";
    }
    
    void process_field(CXCursor field, CXCursor parent) {
        std::string name = to_std_string(clang_getCursorSpelling(field));
        CXType type = clang_getCursorType(field);
        std::string type_name = get_type_name(type);
        
        // 计算偏移量
        long long offset_bits = clang_Type_getOffsetOf(clang_getCursorType(parent), name.c_str());
        size_t offset_bytes = offset_bits / 8;
        
        out << "            member_info{\n";
        out << "                .name = \"" << name << "\",\n";
        out << "                .offset = " << offset_bytes << ",\n";
        out << "            },\n";
    }
    
    void process_method(CXCursor method) {
        std::string name = to_std_string(clang_getCursorSpelling(method));
        CXType result_type = clang_getResultType(clang_getCursorType(method));
        std::string return_type = get_type_name(result_type);
        
        // bool is_constexpr = clang_CXXMethod_isConstexpr(method);
        bool is_static = clang_CXXMethod_isStatic(method);
        bool is_const = clang_CXXMethod_isConst(method);
        bool is_virtual = clang_CXXMethod_isVirtual(method);
        
        out << "            method_info{\n";
        out << "                .name = \"" << name << "\",\n";
        // out << "                .is_constexpr = " << (is_constexpr ? "true" : "false") << ",\n";
        out << "                .is_static = " << (is_static ? "true" : "false") << ",\n";
        out << "                .is_const = " << (is_const ? "true" : "false") << ",\n";
        out << "                .is_virtual = " << (is_virtual ? "true" : "false") << ",\n";
        out << "            },\n";
    }
    
    void process_enum(CXCursor enum_cursor) {
        auto attrs = extract_attributes(enum_cursor);
        bool should_reflect = std::any_of(attrs.begin(), attrs.end(), 
            [](const attribute& attr) { return attr.name == "reflect"; });
        
        if (!should_reflect) return;
        
        std::string name = get_qualified_name(enum_cursor);
        if (generated_types.find(name) != generated_types.end()) return;
        
        generated_types[name] = name;
        
        out << "template<>\n";
        out << "struct type_info<" << name << "> {\n";
        
        // 枚举名
        out << "    static consteval std::string_view name() noexcept {\n";
        out << "        return \"" << name << "\";\n";
        out << "    }\n\n";
        
        // 枚举值
        out << "    static consteval std::span<const enumerator_info> enumerators() noexcept {\n";
        out << "        static constexpr std::array enumerators{\n";
        
        clang_visitChildren(enum_cursor, [](CXCursor c, CXCursor, CXClientData data) {
            if (clang_getCursorKind(c) == CXCursor_EnumConstantDecl) {
                std::string name = to_std_string(clang_getCursorSpelling(c));
                long long value = clang_getEnumConstantDeclValue(c);
                auto* out = static_cast<std::ostream*>(data);
                *out << "            enumerator_info{\"" << name << "\", " << value << "},\n";
            }
            return CXChildVisit_Continue;
        }, &out);
        
        out << "        };\n";
        out << "        return enumerators;\n";
        out << "    }\n";
        
        out << "};\n\n";
    }
};

// 获取编译命令参数
std::vector<std::string> get_compile_args(const std::string& filename) {
    std::vector<std::string> args = {
        "-std=c++2b",
        "-xc++",
        "-I/usr/include",
        "-I/usr/local/include"
    };
    
    // 添加系统特定的包含路径
#if defined(__linux__)
    args.push_back("-I/usr/include/c++/11");
    args.push_back("-I/usr/include/x86_64-linux-gnu/c++/11");
#elif defined(__APPLE__)
    args.push_back("-I/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1");
#endif
    
    return args;
}

// 主函数
// int main(int argc, char** argv) {
//     if (argc < 3) {
//         std::cerr << "Usage: " << argv[0] << " <input_file> <output_file>\n";
//         return 1;
//     }
    
//     const char* input_file = argv[1];
//     const char* output_file = argv[2];
    
//     // 创建索引
//     CXIndex index = clang_createIndex(0, 0);
    
//     // 获取编译参数
//     auto args = get_compile_args(input_file);
//     std::vector<const char*> c_args;
//     for (const auto& arg : args) {
//         c_args.push_back(arg.c_str());
//     }
    
//     // 解析翻译单元
//     CXTranslationUnit tu = clang_parseTranslationUnit(
//         index,
//         input_file,
//         c_args.data(),
//         static_cast<int>(c_args.size()),
//         nullptr, 0,
//         CXTranslationUnit_None
//     );
    
//     if (!tu) {
//         std::cerr << "Failed to parse translation unit\n";
//         clang_disposeIndex(index);
//         return 1;
//     }
    
//     // 生成反射代码
//     std::ofstream out(output_file);
//     if (!out.is_open()) {
//         std::cerr << "Failed to open output file: " << output_file << "\n";
//         clang_disposeTranslationUnit(tu);
//         clang_disposeIndex(index);
//         return 1;
//     }
    
//     ReflectionGenerator generator(out, tu);
//     generator.generate();
    
//     // 清理资源
//     clang_disposeTranslationUnit(tu);
//     clang_disposeIndex(index);
    
//     std::cout << "Reflection data generated: " << output_file << "\n";
//     return 0;
// }