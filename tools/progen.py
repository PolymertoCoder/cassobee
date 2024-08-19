#!/usr/bin/python3

import xml.etree.ElementTree as ET
import os
import time

protocol_id_counter = 1
protocol_enum_entries = []

# 配置路径
xml_directory = "../progen"
protocol_root_directory = "../source/protocol"
header_output_directory = "../source/protocol/include"
cpp_output_directory = "../source/protocol/source"

def check_regenerate(header_filename, cpp_filename, xml_mtime):
    """检查是否需要重新生成代码"""
    if os.path.exists(header_filename) and os.path.exists(cpp_filename):
        header_mtime = os.path.getmtime(header_filename)
        cpp_mtime = os.path.getmtime(cpp_filename)
        if xml_mtime <= header_mtime and xml_mtime <= cpp_mtime:
            return False
    return True

def generate_class_header(name, base_class, fields, maxsize=None, codefield=None, default_code=None):
    """生成类的头文件内容"""
    header_content = []
    
    header_content.append(f"#pragma once\n\n")
    header_content.append(f'#include "{base_class}.h"\n\n')
    
    # 动态包含必要的头文件
    included_headers = set()
    for _, field_type, _ in fields:
        if "std::vector" in field_type:
            included_headers.add("#include <vector>\n")
        elif "std::map" in field_type:
            included_headers.add("#include <map>\n")
        elif "std::set" in field_type:
            included_headers.add("#include <set>\n")
        elif "std::string" in field_type:
            included_headers.add("#include <string>\n")
        elif "std::pair" in field_type:
            included_headers.add("#include <utility>\n")
        elif "std::unordered_set" in field_type:
            included_headers.add("#include <unordered_set>\n")
        elif "std::unordered_map" in field_type:
            included_headers.add("#include <unordered_map>\n")
    
    header_content.extend(included_headers)
    header_content.append("\n")
    
    header_content.append(f"class {name} : public {base_class}\n")
    header_content.append("{\n")
    header_content.append("public:\n")

    if base_class == "protocol":
        header_content.append(f"    static constexpr PROTOCOLID TYPE = {protocol_id_counter};\n")
    
    if codefield:
        if default_code == "ALL":
            default_code = "ALLFIELDS"
        header_content.append(f"    {codefield} code = {default_code};\n")
        header_content.append("    enum FIELDS\n")
        header_content.append("    {\n")
        for idx, (field_name, _, _) in enumerate(fields):
            header_content.append(f"        FIELDS_{field_name.upper()} = 1 << {idx},\n")
        header_content.append(f"        ALLFIELDS = (1 << {len(fields)}) - 1\n")
        header_content.append("    };\n")
    header_content.append("\n")

    # Default constructor
    header_content.append(f"    {name}() = default;\n")
    
    # Constructor with parameters
    header_content.append(f"    {name}(")
    header_content.append(", ".join([f"{field_type} {field_name}" for field_name, field_type, _ in fields]))
    if codefield:
        header_content.append(f", {codefield} code = {default_code}")
    header_content.append(") : ")
    if codefield:
        header_content.append(f"code(code), ")
    header_content.append(", ".join([f"{field_name}({field_name})" for field_name, _, _ in fields]))
    header_content.append("\n    {}\n")
    
    # Copy and Move constructors
    header_content.append(f"    {name}(const {name}& rhs) = default;\n")
    header_content.append(f"    {name}({name}&& rhs) = default;\n\n")

    # Assignment operator
    header_content.append(f"    {name}& operator=(const {name}& rhs) = default;\n")
    
    # Equality and Inequality operators
    header_content.append(f"    bool operator==(const {name}& rhs) const\n")
    header_content.append("    {\n")
    header_content.append(f"        return ")
    header_content.append(" && ".join([f"{field_name} == rhs.{field_name}" for field_name, _, _ in fields]))
    if codefield:
        header_content.append(" && code == rhs.code")
    header_content.append(";\n    }\n")
    
    header_content.append(f"    bool operator!=(const {name}& rhs) const {{ return !(*this == rhs); }}\n")

    if maxsize:
        header_content.append(f"    virtual PROTOCOLID get_type() const override {{ return TYPE; }}\n")
        header_content.append(f"    virtual size_t maxsize() const override {{ return {maxsize}; }}\n")
        header_content.append(f"    virtual {base_class}* dup() const override {{ return new {name}(*this); }}\n")
        header_content.append("    virtual void run() override;\n\n")
    
    if codefield:
        for field_name, field_type, _ in fields:
            header_content.append(f"    void set_{field_name}()\n")
            header_content.append("    {\n")
            header_content.append(f"        code |= FIELDS_{field_name.upper()};\n")
            header_content.append("    }\n")
            header_content.append(f"    void set_{field_name}(const {field_type}& value)\n")
            header_content.append("    {\n")
            header_content.append(f"        code |= FIELDS_{field_name.upper()};\n")
            header_content.append(f"        {field_name} = value;\n")
            header_content.append("    }\n")
        header_content.append("    void set_all_fields() { code = ALLFIELDS; }\n")
        header_content.append("    void clean_default()\n")
        header_content.append("    {\n")
        for field_name, field_type, default_value in fields:
            header_content.append(f"        if ({field_name} == {default_value}) code &= ~FIELDS_{field_name.upper()};\n")
        header_content.append("    }\n")

    header_content.append("\n    octetsstream& pack(octetsstream& os) const override;\n")
    header_content.append("    octetsstream& unpack(octetsstream& os) override;\n\n")

    header_content.append("public:\n")
    for field_name, field_type, default_value in fields:
        header_content.append(f"    {field_type} {field_name} = {default_value};\n")
    header_content.append("};\n\n")

    return header_content

def generate_class_cpp(name, fields, codefield=None):
    """生成类的实现文件内容"""
    cpp_content = []

    cpp_content.append(f'#include "{name}.h"\n\n')
    cpp_content.append(f"octetsstream& {name}::pack(octetsstream& os) const\n")
    cpp_content.append("{\n")
    if codefield:
        cpp_content.append(f"    os << code;\n")
        for idx, (field_name, _, _) in enumerate(fields):
            cpp_content.append(f"    if (code & FIELDS_{field_name.upper()}) os << {field_name};\n")
    else:
        for field_name, _, _ in fields:
            cpp_content.append(f"    os << {field_name};\n")
    cpp_content.append("    return os;\n")
    cpp_content.append("}\n\n")
    cpp_content.append(f"octetsstream& {name}::unpack(octetsstream& os)\n")
    cpp_content.append("{\n")
    if codefield:
        cpp_content.append(f"    os >> code;\n")
        for idx, (field_name, _, _) in enumerate(fields):
            cpp_content.append(f"    if (code & FIELDS_{field_name.upper()}) os >> {field_name};\n")
    else:
        for field_name, _, _ in fields:
            cpp_content.append(f"    os >> {field_name};\n")
    cpp_content.append("    return os;\n")
    cpp_content.append("}\n\n")

    cpp_content.append(f"void {name}::run() {{}} __attribute__((weak));")

    return cpp_content

def parse_protocol(element, xml_mtime):
    """解析 protocol 元素并生成相应的头文件和实现文件"""
    global protocol_id_counter
    name = element.get('name')
    maxsize = element.get('maxsize')
    codefield = element.get('codefield')
    default_code = element.get('defaulte_code', '0')
    fields = []
    for field in element.findall('field'):
        field_name = field.get('name')
        field_type = field.get('type')
        default_value = field.get('default')
        
        if default_value is None:
            raise ValueError(f"Field '{field_name}' in protocol '{name}' does not have a default value")

        fields.append((field_name, field_type, default_value))

    header_filename = os.path.join(header_output_directory, f"{name}.h")
    cpp_filename = os.path.join(cpp_output_directory, f"{name}.cpp")

    if not check_regenerate(header_filename, cpp_filename, xml_mtime):
        print(f"Skipping generation for {name}, files are up to date.")
        return

    print(f"Generating header and cpp for protocol: {name}")

    header_content = generate_class_header(name, "protocol", fields, maxsize, codefield, default_code)
    cpp_content = generate_class_cpp(name, fields, codefield)

    with open(header_filename, 'w') as header_file:
        header_file.writelines(header_content)

    with open(cpp_filename, 'w') as cpp_file:
        cpp_file.writelines(cpp_content)

    protocol_enum_entries.append(f"PROTOCOL_TYPE_{name.upper()} = {protocol_id_counter}")
    protocol_id_counter += 1

def parse_rpcdata(element, xml_mtime):
    """解析 rpcdata 元素并生成相应的头文件和实现文件"""
    name = element.get('name')
    codefield = element.get('codefield')
    default_code = element.get('defaulte_code', '0')
    fields = []
    for field in element.findall('field'):
        field_name = field.get('name')
        field_type = field.get('type')
        default_value = field.get('default')

        if default_value is None:
            raise ValueError(f"Field '{field_name}' in rpcdata '{name}' does not have a default value")

        fields.append((field_name, field_type, default_value))

    header_filename = os.path.join(header_output_directory, f"{name}.h")
    cpp_filename = os.path.join(cpp_output_directory, f"{name}.cpp")

    if not check_regenerate(header_filename, cpp_filename, xml_mtime):
        print(f"Skipping generation for {name}, files are up to date.")
        return

    print(f"Generating header and cpp for rpcdata: {name}")

    header_content = generate_class_header(name, "rpcdata", fields, codefield=codefield, default_code=default_code)
    cpp_content = generate_class_cpp(name, fields, codefield)

    with open(header_filename, 'w') as header_file:
        header_file.writelines(header_content)

    with open(cpp_filename, 'w') as cpp_file:
        cpp_file.writelines(cpp_content)

def generate_protocol_definitions():
    """生成协议类型定义文件"""
    define_filename = os.path.join(protocol_root_directory, "prot_define.h")
    with open(define_filename, 'w') as define_file:
        define_file.write("#pragma once\n")
        define_file.write("#include \"types.h\"\n\n")
        define_file.write(f"static constexpr PROTOCOLID MAXPROTOCOLID = {len(protocol_enum_entries)};\n\n")
        define_file.write("enum PROTOCOL_TYPE\n")
        define_file.write("{\n")
        for entry in protocol_enum_entries:
            define_file.write(f"    {entry},\n")
        define_file.write("};\n\n")

def main():
    """主函数，解析 XML 文件并生成代码"""
    os.makedirs(header_output_directory, exist_ok=True)
    os.makedirs(cpp_output_directory, exist_ok=True)

    # 删除旧文件
    for file in os.listdir(header_output_directory):
        file_path = os.path.join(header_output_directory, file)
        if os.path.isfile(file_path):
            os.unlink(file_path)
    for file in os.listdir(cpp_output_directory):
        file_path = os.path.join(cpp_output_directory, file)
        if os.path.isfile(file_path):
            os.unlink(file_path)

    xml_files = [file for file in os.listdir(xml_directory) if file.endswith('.xml')]
    for xml_file in xml_files:
        xml_path = os.path.join(xml_directory, xml_file)
        try:
            tree = ET.parse(xml_path)
        except ET.ParseError as e:
            print(f"Error parsing XML file {xml_path}: {e}")
            continue
        root = tree.getroot()
        xml_mtime = os.path.getmtime(xml_path)
        for element in root:
            if element.tag == 'protocol':
                parse_protocol(element, xml_mtime)
            elif element.tag == 'rpcdata':
                parse_rpcdata(element, xml_mtime)
    generate_protocol_definitions()
    print("Protocol definitions generated successfully.")

if __name__ == "__main__":
    main()
