#!/usr/bin/python3

import xml.etree.ElementTree as ET
import os
import argparse
from collections import OrderedDict, defaultdict

modified_xml_files = []

basic_types = ["bool", "char", "int8_t", "uint8_t", "short", "int16_t", "uint16_t", "int", "int32_t", "uint32_t", "float", "double", "long", "long long", "int64_t", "uint64_t"]
stl_types = ["std::vector", "std::map", "std::set", "std::string", "std::pair", "std::unordered_set", "std::unordered_map"]

# 数据缓存结构
class ProtocolInfo:
    __slots__ = ['name', 'base_class', 'fields', 'type', 'maxsize', 'codefield', 'default_code', 'argument_type', 'result_type', 'includes']
    
    def __init__(self):
        self.name = ""
        self.base_class = ""
        self.fields = []
        self.type = None
        self.maxsize = None
        self.codefield = None
        self.default_code = '0'
        self.argument_type = None
        self.result_type = None
        self.includes = set()

def check_regenerate(xmlpath, header_output_directory, cpp_output_directory):
    """Check if regeneration of code is necessary."""
    any_change_detected = False
    header_mtime = os.path.getmtime(header_output_directory)
    cpp_mtime = os.path.getmtime(cpp_output_directory)
    
    def check_mtime(mtime):
        return mtime > header_mtime or mtime > cpp_mtime

    script_path = os.path.abspath(__file__)
    script_mtime = os.path.getmtime(script_path)
    if check_mtime(script_mtime):
        any_change_detected = True

    xml_files = [file for file in os.listdir(xmlpath) if file.endswith('.xml')]
    for xml_file in xml_files:
        xml_path = os.path.join(xmlpath, xml_file)
        xml_mtime = os.path.getmtime(xml_path)

        if check_mtime(xml_mtime):
            any_change_detected = True
            modified_xml_files.append(xml_file)

    return any_change_detected

def create_directories(header_output_directory, cpp_output_directory, state_output_directory):
    """Create necessary directories."""
    os.makedirs(header_output_directory, exist_ok=True)
    os.makedirs(cpp_output_directory, exist_ok=True)
    os.makedirs(state_output_directory, exist_ok=True)

def clean_old_files(xmlpath, header_output_directory, cpp_output_directory, force):
    """If force is True, delete old files."""
    if force:
        print("Force to regenerate protocol")
    elif not check_regenerate(xmlpath, header_output_directory, cpp_output_directory):
        print("No need to regenerate protocol because there are no xml files were modified")
        exit(0)
    
    for xml_file in modified_xml_files:
        print(f"{xml_file} has modified")

    for directory in [header_output_directory, cpp_output_directory]:
        for file in os.listdir(directory):
            file_path = os.path.join(directory, file)
            if os.path.isfile(file_path):
                os.unlink(file_path)

class XMLProcessor:
    def __init__(self):
        self.protocol_cache = OrderedDict()  # 有序字典保证生成顺序
        self.state_cache = defaultdict(list)  # state缓存
        self.protocol_enum_entries = OrderedDict() # 协议号枚举
        self.protocol_name2id = OrderedDict() # 名称到协议号的映射
        self.protocol_ids = set() # 所有的协议类型

    def preprocess_xml(self, xmlpath):
        """预处理阶段：解析所有XML文件并缓存数据"""
        xml_files = [f for f in os.listdir(xmlpath) if f.endswith('.xml')]
        main_protocol = None
        
        # 首先处理protocol.xml
        if 'protocol.xml' in xml_files:
            main_protocol = os.path.join(xmlpath, 'protocol.xml')
            self._parse_protocol_file(main_protocol)
            xml_files.remove('protocol.xml')

        # 处理其他XML文件
        for xml_file in xml_files:
            xml_path = os.path.join(xmlpath, xml_file)
            self._parse_general_xml(xml_path)

    def _parse_protocol_file(self, xml_path):
        """解析protocol.xml特殊处理"""
        tree = ET.parse(xml_path)
        root = tree.getroot()
        
        for element in root:
            if element.tag == "state":
                state_name = element.get('name')
                for protocol in element.findall('protocol'):
                    self.state_cache[state_name].append(protocol.get('name'))

    def _parse_general_xml(self, xml_path):
        """解析普通XML文件"""
        tree = ET.parse(xml_path)
        root = tree.getroot()
        
        for element in root:
            info = ProtocolInfo()
            info.name = element.get('name') # type: ignore
            info.base_class = element.tag
            
            # 解析公共属性
            if info.base_class == "protocol" or info.base_class == "rpc":
                info.type = element.get('type') # type: ignore
                info.maxsize = element.get('maxsize') # type: ignore
                if not info.type or not info.maxsize:
                    raise ValueError(f"Protocol {info.name} missing type/maxsize")
                
                # 记录protocol类型映射
                self.protocol_enum_entries[info.type] = f"PROTOCOL_TYPE_{info.name.upper()}"
            
            # 解析字段和包含
            info.fields = self._parse_fields(element)
            info.includes = self._parse_includes(element)
            if info.base_class == "rpc":
                [info.argument_type, info.result_type] = self._parse_rpc_info(element)
            
            # 解析codefield相关属性
            info.codefield = element.get('codefield') # type: ignore
            info.default_code = element.get('default_code', '0')
            
            # 缓存协议信息
            self.protocol_cache[info.name] = info
            if info.base_class == "protocol" or info.base_class == "rpc":
                self.protocol_name2id[info.name] = info.type
                if info.type in self.protocol_ids:
                    raise ValueError(f"Duplicate protocol type {info.type} in {info.name}")
                self.protocol_ids.add(info.type)

    def _parse_fields(self, element):
        """解析字段信息"""
        fields = []
        for field in element.findall('field'):
            field_name = field.get('name')
            field_type = field.get('type')
            default_value = field.get('default')
            
            if not field_name or not field_type or not default_value:
                raise ValueError(f"Invalid field definition in {element.get('name')}")
            
            fields.append((field_name, field_type, default_value))
        return fields

    def _parse_includes(self, element):
        """解析包含的头文件"""
        includes = set()
        for include in element.findall('include'):
            includes.add(include.get('name'))
        return includes
    
    def _parse_rpc_info(self, element):
        """解析RPC相关信息"""
        argument = element.find('argument')
        result = element.find('result')
        if argument is None or result is None:
            raise ValueError(f"Invalid RPC definition in {element.get('name')}")

        argument_type = argument.get('type')
        result_type = result.get('type')
        if argument_type is None or result_type is None:
            raise ValueError(f"Invalid RPC definition in {element.get('name')}")

        if argument_type not in self.protocol_cache:
            raise ValueError(f"Argument type {argument_type} not found in protocol cache")
        if result_type not in self.protocol_cache:
            raise ValueError(f"Result type {result_type} not found in protocol cache")
        return argument_type, result_type
    
    def _is_rpcdata(self, field_type):
        """检查字段类型是否为rpcdata"""
        return field_type in self.protocol_cache and self.protocol_cache[field_type].base_class == "rpcdata"

    def generate_code(self, root_dir, header_dir, cpp_dir, state_dir):
        """生成阶段：根据缓存数据生成代码文件"""
        self._generate_protocol_files(header_dir, cpp_dir)
        self._generate_state_files(state_dir)
        self._generate_protocol_definitions(root_dir)

    def _generate_protocol_files(self, header_dir, cpp_dir):
        """生成协议相关的.h和.cpp文件"""
        
        for protocol in self.protocol_cache.values():
            # 生成头文件
            header_content = self._generate_header_content(protocol)
            header_path = os.path.join(header_dir, f"{protocol.name}.h")
            self._write_file(header_path, header_content)
            
            # 生成cpp文件
            cpp_content = self._generate_cpp_content(protocol)
            cpp_path = os.path.join(cpp_dir, f"{protocol.name}.cpp")
            self._write_file(cpp_path, cpp_content)

    def _generate_constructors(self, protocol):
        """Generate constructors for the class."""

        def __generate_default_constructor_params(protocol):
            """Generate default constructor parameters."""
            constructor_content = ""
            if protocol.base_class == "rpc":
                constructor_content += f"    {protocol.name}()\n"
                constructor_content += "    {    \n"
                constructor_content += f"        _argument = new {protocol.argument_type};\n"
                constructor_content += f"        _result = new {protocol.result_type};\n"
                constructor_content += "    }\n"
            else:
                constructor_content += f"    {protocol.name}() = default;\n"
            if protocol.base_class == "protocol":
                constructor_content += f"    {protocol.name}(PROTOCOLID type) : {protocol.base_class}(type)\n    {{}}\n"
            elif protocol.base_class == "rpc":
                constructor_content += f"    {protocol.name}(PROTOCOLID type) : {protocol.base_class}(type)\n"
                constructor_content += "    {\n"
                constructor_content += f"        _argument = new {protocol.argument_type};\n"
                constructor_content += f"        _result = new {protocol.result_type};\n"
                constructor_content += "    }\n"
            return constructor_content

        def __generate_non_basic_type_constructors(protocol):
            """Generate constructors with const& and && parameters for non-basic types."""
            if len(protocol.fields) == 0:
                return ""
            constructor_content = ""
            non_basic_fields = [(field_name, field_type) for field_name, field_type, _ in protocol.fields if field_type not in basic_types]

            # Generate constructors with const& parameters
            params = ", ".join([f"const {field_type}& _{field_name}" if field_type not in basic_types else f"{field_type} _{field_name}" for field_name, field_type, _ in protocol.fields])
            initializers = ", ".join([f"{field_name}(_{field_name})" for field_name, _, _ in protocol.fields])
            constructor_content += f"    {protocol.name}({params}) : "
            if protocol.codefield:
                constructor_content += f"code(ALLFIELDS), "
            constructor_content += f"{initializers}\n    {{}}\n"

            # Generate constructors with && parameters
            if non_basic_fields:
                params = ", ".join([f"{field_type}&& _{field_name}" if field_type not in basic_types else f"{field_type} _{field_name}" for field_name, field_type, _ in protocol.fields])
                initializers = ", ".join([f"{field_name}(std::move(_{field_name}))" if field_type not in basic_types else f"{field_name}(_{field_name})" for field_name, field_type, _ in protocol.fields])
                constructor_content += f"    {protocol.name}({params}) : "
                if protocol.codefield:
                    constructor_content += f"code(ALLFIELDS), "
                constructor_content += f"{initializers}\n    {{}}\n"

            return constructor_content

        constructor_content = ""
        constructor_content += __generate_default_constructor_params(protocol)
        constructor_content += __generate_non_basic_type_constructors(protocol)

        constructor_content += f"    {protocol.name}(const {protocol.name}& rhs) = default;\n"
        constructor_content += f"    {protocol.name}({protocol.name}&& rhs) = default;\n"
        if protocol.base_class != "rpc":
            constructor_content += f"    {protocol.name}& operator=(const {protocol.name}& rhs) = default;\n"

        return constructor_content
    
    def _generate_operator_overloads(self, protocol):
        """Generate operator overloads for the class."""
        operator_content = f"    bool operator==(const {protocol.name}& rhs) const\n    {{\n        return "
        if protocol.codefield:
            operator_content += "code == rhs.code"
            if len(protocol.fields):
                operator_content += " && "
        elif len(protocol.fields) == 0:
            operator_content += "true"
        operator_content += " && ".join([f"{field_name} == rhs.{field_name}" for field_name, _, _ in protocol.fields])
        operator_content += ";\n    }\n"
        operator_content += f"    bool operator!=(const {protocol.name}& rhs) const {{ return !(*this == rhs); }}\n"
        return operator_content

    def _generate_virtual_methods(self, protocol):
        """Generate virtual methods for the class."""
        virtual_methods = ""
        if protocol.base_class == "protocol":
            virtual_methods += f"    virtual PROTOCOLID get_type() const override {{ return TYPE; }}\n"
            virtual_methods += f"    virtual const char* get_name() const override {{ return \"{protocol.name}\"; }}\n"
            virtual_methods += f"    virtual size_t maxsize() const override {{ return {protocol.maxsize}; }}\n"
            virtual_methods += f"    virtual {protocol.base_class}* dup() const override {{ return new {protocol.name}(*this); }}\n"
            virtual_methods += "    virtual void run() override;\n"
            virtual_methods += f"    virtual ostringstream& dump(ostringstream& out) const override;\n\n"
        elif protocol.base_class == "rpc":
            virtual_methods += f"    virtual PROTOCOLID get_type() const override {{ return TYPE; }}\n"
            virtual_methods += f"    virtual const char* get_name() const override {{ return \"{protocol.name}\"; }}\n"
            virtual_methods += f"    virtual size_t maxsize() const override {{ return {protocol.maxsize}; }}\n"
            virtual_methods += f"    virtual {protocol.base_class}* dup() const override {{ return new {protocol.name}(*this); }}\n"
            virtual_methods += f"    virtual bool server(rpcdata* argument, rpcdata* result) override;\n"
            virtual_methods += f"    virtual void client(rpcdata* argument, rpcdata* result) override;\n"
            virtual_methods += f"    virtual void timeout(rpcdata* argument) override;\n"
        elif protocol.base_class == "rpcdata":
            virtual_methods += f"    virtual {protocol.base_class}* dup() const override {{ return new {protocol.name}(*this); }}\n"
            virtual_methods += f"    virtual ostringstream& dump(ostringstream& out) const override;\n\n"
        return virtual_methods

    def _generate_codefield_methods(self, protocol):
        """Generate methods to handle codefield."""
        codefield_methods = "public:\n"
        for field_name, field_type, default_value in protocol.fields:
            codefield_methods += f"    void set_{field_name}()\n    {{\n        code |= FIELDS_{field_name.upper()};\n    }}\n"
            codefield_methods += f"    void set_{field_name}(const {field_type}& value)\n    {{\n        code |= FIELDS_{field_name.upper()};\n        {field_name} = value;\n    }}\n"
        codefield_methods += "    void set_all_fields() { code = ALLFIELDS; }\n"
        codefield_methods += "    void clean_default()\n    {\n"
        for field_name, field_type, default_value in protocol.fields:
            if default_value == "{}":
                codefield_methods += f"        if({field_name} == {field_type}()) code &= ~FIELDS_{field_name.upper()};\n"
            else:
                codefield_methods += f"        if({field_name} == {default_value}) code &= ~FIELDS_{field_name.upper()};\n"
        codefield_methods += "    }\n"
        return codefield_methods

    def _generate_pack_unpack_methods(self, protocol):
        """Generate pack and unpack methods for the class."""
        pack_unpack_methods = f"    virtual octetsstream& pack(octetsstream& os) const override;\n"
        pack_unpack_methods += "    virtual octetsstream& unpack(octetsstream& os) override;\n\n"
        return pack_unpack_methods

    def _generate_public_fields(self, protocol):
        """Generate public fields for the class."""
        if not protocol.codefield and not protocol.fields:
            return ""

        public_fields = "\npublic:\n"
        if protocol.codefield:
            public_fields += f"    {protocol.codefield} code = 0;\n"
        for field_name, field_type, default_value in protocol.fields:
            if default_value == "{}":
                public_fields += f"    {field_type} {field_name};\n"
            else:
                public_fields += f"    {field_type} {field_name} = {default_value};\n"
        return public_fields

    def _generate_pack_method(self, protocol):
        """Generate the pack method for the class."""
        pack_method = f"octetsstream& {protocol.name}::pack(octetsstream& os) const\n{{\n"
        if protocol.codefield:
            pack_method += f"    os << code;\n"
            for idx, (field_name, _, _) in enumerate(protocol.fields):
                pack_method += f"    if(code & FIELDS_{field_name.upper()}) os << {field_name};\n"
        else:
            for field_name, _, _ in protocol.fields:
                pack_method += f"    os << {field_name};\n"
        pack_method += "    return os;\n}\n\n"
        return pack_method

    def _generate_unpack_method(self, protocol):
        """Generate the unpack method for the class."""
        unpack_method = f"octetsstream& {protocol.name}::unpack(octetsstream& os)\n{{\n"
        if protocol.codefield:
            unpack_method += f"    os >> code;\n"
            for idx, (field_name, _, _) in enumerate(protocol.fields):
                unpack_method += f"    if(code & FIELDS_{field_name.upper()}) os >> {field_name};\n"
        else:
            for field_name, _, _ in protocol.fields:
                unpack_method += f"    os >> {field_name};\n"
        unpack_method += "    return os;\n}\n\n"
        return unpack_method

    def _generate_dump_method(self, protocol):
        """Generate the dump method for the class."""
        dump_method = f"ostringstream& {protocol.name}::dump(ostringstream& out) const\n{{\n"
        dump_method += f"    {protocol.base_class}::dump(out);\n"
        if protocol.codefield:
            dump_method += f"    out << \"code: \" << code;\n"
        for field_name, _, _ in protocol.fields:
            dump_method += f"    out << \"{field_name}: \" << {field_name};\n"
        dump_method += "    return out;\n}\n"
        return dump_method

    def _generate_header_content(self, protocol):
        """生成头文件内容"""
        lines = []
        lines.append("#pragma once\n")
        
        # 生成包含头文件的内容
        lines.append(self._generate_includes(protocol))
        
        lines.append("\nnamespace bee\n{\n\n")
        lines.append(f"class {protocol.name} : public {protocol.base_class}\n{{\npublic:\n")
        
        # 生成类内容
        if protocol.base_class == "protocol":
            lines.append(f"    static constexpr PROTOCOLID TYPE = {protocol.type};\n")
        elif protocol.base_class == "rpc":
            lines.append(f"    static constexpr PROTOCOLID TYPE = {protocol.type};\n")
            lines.append(f"    using argument_type = {protocol.argument_type};\n")
            lines.append(f"    using result_type = {protocol.result_type};\n")

        if protocol.codefield:
            lines.append(self._generate_enum_fields(protocol))

        # if protocol.fields:
        #     lines.append(self._generate_constructors(protocol))
        # else:
        #     lines.append(f"    {protocol.name}() = default;\n")

        lines.append(self._generate_constructors(protocol))

        if protocol.base_class != "rpc":
            lines.append(self._generate_operator_overloads(protocol))
        lines.append(f"    virtual ~{protocol.name}() override = default;\n\n")
    
        if protocol.base_class != "rpc":
            lines.append(self._generate_pack_unpack_methods(protocol))
        lines.append(self._generate_virtual_methods(protocol))

        if protocol.base_class == "rpc":
            lines.append(f"    static {protocol.base_class}* call(const {protocol.argument_type}& argument = {{}}) {{ return {protocol.base_class}::call(TYPE, argument); }}\n")

        if protocol.codefield:
            lines.append(self._generate_codefield_methods(protocol))

        lines.append(self._generate_public_fields(protocol))
        
        lines.append("};\n\n} // namespace bee")
        return "".join(lines)

    def _generate_cpp_content(self, protocol):
        """生成cpp文件内容"""
        lines = []
        lines.append(f'#include "{protocol.name}.h"\n\n')
        lines.append("namespace bee\n{\n\n")
        
        if protocol.base_class == "protocol":
            lines.append(self._generate_pack_method(protocol))
            lines.append(self._generate_unpack_method(protocol))
            lines.append(self._generate_dump_method(protocol))
            lines.append(f"\n__attribute__((weak)) void {protocol.name}::run() {{}}\n")
        elif protocol.base_class == "rpc":
            lines.append(f"\n__attribute__((weak)) bool {protocol.name}::server(rpcdata* argument, rpcdata* result) {{ return false; }}")
            lines.append(f"\n__attribute__((weak)) void {protocol.name}::client(rpcdata* argument, rpcdata* result) {{}}")
            lines.append(f"\n__attribute__((weak)) void {protocol.name}::timeout(rpcdata* argument) {{}}\n")
        elif protocol.base_class == "rpcdata":
            lines.append(self._generate_pack_method(protocol))
            lines.append(self._generate_unpack_method(protocol))
            lines.append(self._generate_dump_method(protocol))
        
        lines.append("\n} // namespace bee")
        return "".join(lines)

    def _generate_includes(self, protocol):
        lines = []

        # 添加STL头文件
        stl_headers = set()
        rpcdata_headers = set()
        for _, field_type, _ in protocol.fields:
            for stl in stl_types:
                if stl in field_type:
                    stl_headers.add(f"#include <{stl.split('::')[-1]}>\n")
            if self._is_rpcdata(field_type):
                rpcdata_headers.add(f"#include \"{field_type}.h\"\n")

        lines.extend(sorted(stl_headers))

        # 添加基础头文件
        lines.append(f'#include "{protocol.base_class}.h"\n')
        if protocol.base_class == "rpc":
            lines.append(f'#include "{protocol.argument_type}.h"\n')
            lines.append(f'#include "{protocol.result_type}.h"\n')
        
        # rpcdata头文件
        lines.extend(sorted(rpcdata_headers))
        
        # 添加自定义头文件
        for include in protocol.includes:
            lines.append(f'#include "{include}"\n')
        return "".join(lines)

    def _generate_enum_fields(self, protocol):
        """生成枚举字段"""
        lines = ["    enum FIELDS\n    {\n"]
        for idx, (field_name, _, _) in enumerate(protocol.fields):
            lines.append(f"        FIELDS_{field_name.upper()} = 1 << {idx},\n")
        lines.append(f"        ALLFIELDS = (1 << {len(protocol.fields)}) - 1\n    }};\n\n")
        return "".join(lines)

    def _generate_state_files(self, state_dir):
        """生成状态文件"""
        for state_name, protocols in self.state_cache.items():
            content = []
            register_lines = []
            
            for proto_name in protocols:
                content.append(f"#include \"{proto_name}.h\"\n")
                register_lines.append(
                    f"static {proto_name} __register_{proto_name}({self.protocol_cache[proto_name].type});\n"
                )

            content.append("\nnamespace bee\n{\n\n")
            content.extend(register_lines)
            content.append("\n} // namespace bee")
            state_path = os.path.join(state_dir, f"state_{state_name}.cpp")
            self._write_file(state_path, content)

    def _generate_protocol_definitions(self, root_dir):
        """生成协议定义文件"""
        if not self.protocol_enum_entries:
            return

        lines = [
            "#pragma once\n",
            "#include \"types.h\"\n\n",
            "namespace bee\n{\n\n",
            f"static constexpr PROTOCOLID MAXPROTOCOLID = {list(self.protocol_enum_entries.keys())[-1]};\n\n",
            "enum PROTOCOL_TYPE\n{\n"
        ]
        
        for type_id, enum_name in self.protocol_enum_entries.items():
            lines.append(f"    {enum_name} = {type_id},\n")
        
        lines.append("};\n\n} // namespace bee\n")
        define_path = os.path.join(root_dir, "prot_define.h")
        self._write_file(define_path, lines)

    def _write_file(self, path, content):
        """写文件工具方法"""
        # os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, 'w') as f:
            if isinstance(content, list):
                f.writelines(content)
            else:
                f.write(content)

def main():
    """Main function, parses command line arguments and generates code."""
    parser = argparse.ArgumentParser(description="Generate C++ protocol and RPC data classes from XML definitions.")
    parser.add_argument('--xmlpath', required=True, help='Directory containing XML files')
    parser.add_argument('--outpath', required=True, help='Root directory for protocol definitions')
    parser.add_argument('--force', action='store_true', help='Force regeneration of all files')

    args = parser.parse_args()

    root_dir = os.path.join(args.outpath, "")
    header_dir = os.path.join(args.outpath, "include")
    cpp_dir = os.path.join(args.outpath, "source")
    state_dir = os.path.join(args.outpath, "state")

    # 创建必要的文件夹
    create_directories(header_dir, cpp_dir, state_dir)

    # 是否需要重新生成
    clean_old_files(args.xmlpath, header_dir, cpp_dir, args.force)

    processor = XMLProcessor()
    
    # 预处理阶段
    processor.preprocess_xml(args.xmlpath)
    
    # 生成阶段
    processor.generate_code(root_dir, header_dir, cpp_dir, state_dir)
    
    print("Code generation completed successfully.")

if __name__ == "__main__":
    main()