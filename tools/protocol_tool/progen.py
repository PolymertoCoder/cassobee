#!/usr/bin/python3

import xml.etree.ElementTree as ET
import os
import argparse

protocol_id_counter = 1
protocol_enum_entries = []

basic_types = ["bool", "char", "int8_t", "uint8_t", "short", "int16_t", "uint16_t", "int", "int32_t", "uint32_t", "float", "double", "long", "long long", "int64_t", "uint64_t"]

def check_regenerate(header_filename, cpp_filename, xml_mtime):
    """Check if regeneration of code is necessary."""
    if os.path.exists(header_filename) and os.path.exists(cpp_filename):
        header_mtime = os.path.getmtime(header_filename)
        cpp_mtime = os.path.getmtime(cpp_filename)
        if xml_mtime <= header_mtime and xml_mtime <= cpp_mtime:
            return False
    return True

def create_directories(header_output_directory, cpp_output_directory):
    """Create necessary directories."""
    os.makedirs(header_output_directory, exist_ok=True)
    os.makedirs(cpp_output_directory, exist_ok=True)

def clean_old_files(header_output_directory, cpp_output_directory, force):
    """Delete old files if force is True."""
    if force:
        for file in os.listdir(header_output_directory):
            file_path = os.path.join(header_output_directory, file)
            if os.path.isfile(file_path):
                os.unlink(file_path)
        for file in os.listdir(cpp_output_directory):
            file_path = os.path.join(cpp_output_directory, file)
            if os.path.isfile(file_path):
                os.unlink(file_path)

def generate_class_header(name, base_class, fields, maxsize=None, codefield=None, default_code=None):
    """Generate header content for the class."""
    header_content = []
    
    header_content.append(f"#pragma once\n")
    header_content.append(f'#include "{base_class}.h"\n\n')
    
    included_headers = generate_included_headers(fields)
    header_content.extend(included_headers)
    
    header_content.append("\n")
    header_content.append(f"class {name} : public {base_class}\n")
    header_content.append("{\npublic:\n")

    if base_class == "protocol":
        header_content.append(f"    static constexpr PROTOCOLID TYPE = {protocol_id_counter};\n")
    
    if codefield:
        header_content.append(generate_enum_fields(fields, default_code))
    
    if fields:
        header_content.append(generate_constructors(name, fields, codefield, default_code))
        header_content.append(generate_operator_overloads(name, fields, codefield))
    else:
        header_content.append(f"    {name}() = default;\n")

    header_content.append(f"    virtual ~{name}() override = default;\n")
    
    if maxsize:
        header_content.append(generate_virtual_methods(name, maxsize, base_class))
    
    if codefield:
        header_content.append(generate_codefield_methods(fields, codefield))
    
    header_content.append(generate_pack_unpack_methods(name))
    header_content.append(generate_public_fields(fields, codefield))
    
    return header_content

def generate_included_headers(fields):
    """Generate necessary include headers."""
    included_headers = set()
    for _, field_type, _ in fields:
        if "std::vector" in field_type:
            included_headers.add("#include <vector>\n")
        if "std::map" in field_type:
            included_headers.add("#include <map>\n")
        if "std::set" in field_type:
            included_headers.add("#include <set>\n")
        if "std::string" in field_type:
            included_headers.add("#include <string>\n")
        if "std::pair" in field_type:
            included_headers.add("#include <utility>\n")
        if "std::unordered_set" in field_type:
            included_headers.add("#include <unordered_set>\n")
        if "std::unordered_map" in field_type:
            included_headers.add("#include <unordered_map>\n")
        if field_type not in basic_types and not any(stl in field_type for stl in ["std::vector", "std::map", "std::set", "std::string", "std::pair", "std::unordered_set", "std::unordered_map"]):
            included_headers.add(f'#include "{field_type}.h"\n')
    return included_headers

def generate_enum_fields(fields, default_code):
    """Generate enum fields for codefield."""
    enum_content = "    enum FIELDS\n    {\n"
    for idx, (field_name, _, _) in enumerate(fields):
        enum_content += f"        FIELDS_{field_name.upper()} = 1 << {idx},\n"
    enum_content += f"        ALLFIELDS = (1 << {len(fields)}) - 1\n    }};\n\n"
    return enum_content

def generate_constructors(name, fields, codefield, default_code):
    """Generate constructors for the class."""
    constructor_content = ""
    default_params, default_initializers = generate_default_constructor_params(fields)
    constructor_content += f"    {name}({', '.join(default_params)}) : "
    if codefield:
        constructor_content += f"code({default_code})"
        if default_initializers:
            constructor_content += ", "
    constructor_content += ", ".join(default_initializers)
    constructor_content += "\n    {}\n"

    constructor_content += generate_non_basic_type_constructors(name, fields, codefield, default_code)

    constructor_content += f"    {name}(const {name}& rhs) = default;\n"
    constructor_content += f"    {name}({name}&& rhs) = default;\n\n"
    constructor_content += f"    {name}& operator=(const {name}& rhs) = default;\n"
    
    return constructor_content

def generate_default_constructor_params(fields):
    """Generate default constructor parameters."""
    default_params = []
    default_initializers = []
    for field_name, field_type, default_value in fields:
        if default_value == "{}":
            default_params.append(f"{field_type} _{field_name} = {field_type}()")
        else:
            default_params.append(f"{field_type} _{field_name} = {default_value}")
        default_initializers.append(f"{field_name}(_{field_name})")
    return default_params, default_initializers

def generate_non_basic_type_constructors(name, fields, codefield, default_code):
    """Generate constructors for non-basic types with const& and && parameters."""
    constructor_content = ""
    non_basic_fields = [(field_name, field_type) for field_name, field_type, _ in fields if field_type not in basic_types]

    # Generate constructor with const& parameters for non-basic types
    params = ", ".join([f"const {field_type}& _{field_name}" if field_type not in basic_types else f"{field_type} _{field_name}" for field_name, field_type, _ in fields])
    initializers = ", ".join([f"{field_name}(_{field_name})" for field_name, _, _ in fields])
    constructor_content += f"    {name}({params}) : "
    if codefield:
        constructor_content += f"code({default_code}), "
    constructor_content += f"{initializers}\n    {{}}\n"

    # Generate constructor with && parameters for non-basic types
    if non_basic_fields:
        params = ", ".join([f"{field_type}&& _{field_name}" if field_type not in basic_types else f"{field_type} _{field_name}" for field_name, field_type, _ in fields])
        initializers = ", ".join([f"{field_name}(std::move(_{field_name}))" if field_type not in basic_types else f"{field_name}(_{field_name})" for field_name, field_type, _ in fields])
        constructor_content += f"    {name}({params}) : "
        if codefield:
            constructor_content += f"code({default_code}), "
        constructor_content += f"{initializers}\n    {{}}\n"

    return constructor_content

def generate_operator_overloads(name, fields, codefield):
    """Generate operator overloads for the class."""
    operator_content = f"    bool operator==(const {name}& rhs) const\n    {{\n        return "
    if codefield:
        operator_content += "code == rhs.code"
        if len(fields):
            operator_content += " && "
        else:
            operator_content += "\n"
    operator_content += " && ".join([f"{field_name} == rhs.{field_name}" for field_name, _, _ in fields])
    operator_content += ";\n    }\n"
    operator_content += f"    bool operator!=(const {name}& rhs) const {{ return !(*this == rhs); }}\n"
    return operator_content

def generate_virtual_methods(name, maxsize, base_class):
    """Generate virtual methods for the class."""
    virtual_methods = f"    virtual PROTOCOLID get_type() const override {{ return TYPE; }}\n"
    virtual_methods += f"    virtual size_t maxsize() const override {{ return {maxsize}; }}\n"
    virtual_methods += f"    virtual {base_class}* dup() const override {{ return new {name}(*this); }}\n"
    if base_class == "protocol":
        virtual_methods += "    virtual void run() override;\n\n"
    return virtual_methods

def generate_codefield_methods(fields, codefield):
    """Generate methods for handling codefield."""
    codefield_methods = ""
    for field_name, field_type, default_value in fields:
        codefield_methods += f"    void set_{field_name}()\n    {{\n        code |= FIELDS_{field_name.upper()};\n    }}\n"
        codefield_methods += f"    void set_{field_name}(const {field_type}& value)\n    {{\n        code |= FIELDS_{field_name.upper()};\n        {field_name} = value;\n    }}\n"
    codefield_methods += "    void set_all_fields() { code = ALLFIELDS; }\n"
    codefield_methods += "    void clean_default()\n    {\n"
    for field_name, field_type, default_value in fields:
        if default_value == "{}":
            codefield_methods += f"        if ({field_name} == {field_type}()) code &= ~FIELDS_{field_name.upper()};\n"
        else:
            codefield_methods += f"        if ({field_name} == {default_value}) code &= ~FIELDS_{field_name.upper()};\n"
    codefield_methods += "    }\n"
    return codefield_methods

def generate_pack_unpack_methods(name):
    """Generate pack and unpack methods for the class."""
    pack_unpack_methods = f"    octetsstream& pack(octetsstream& os) const override;\n"
    pack_unpack_methods += "    octetsstream& unpack(octetsstream& os) override;\n\n"
    return pack_unpack_methods

def generate_public_fields(fields, codefield):
    """Generate public fields for the class."""
    public_fields = "public:\n"
    if codefield:
        public_fields += f"    {codefield} code = 0;\n"
    for field_name, field_type, default_value in fields:
        if default_value == "{}":
            public_fields += f"    {field_type} {field_name};\n"
        else:
            public_fields += f"    {field_type} {field_name} = {default_value};\n"
    public_fields += "};\n\n"
    return public_fields


def generate_class_cpp(name, fields, base_class, codefield=None):
    """Generate implementation file content for the class."""
    cpp_content = []

    cpp_content.append(f'#include "{name}.h"\n\n')
    cpp_content.append(generate_pack_method(name, fields, codefield))
    cpp_content.append(generate_unpack_method(name, fields, codefield))
    if base_class == "protocol":
        cpp_content.append(f"__attribute__((weak)) void {name}::run() {{}}")

    return cpp_content

def generate_pack_method(name, fields, codefield):
    """Generate pack method for the class."""
    pack_method = f"octetsstream& {name}::pack(octetsstream& os) const\n{{\n"
    if codefield:
        pack_method += f"    os << code;\n"
        for idx, (field_name, _, _) in enumerate(fields):
            pack_method += f"    if (code & FIELDS_{field_name.upper()}) os << {field_name};\n"
    else:
        for field_name, _, _ in fields:
            pack_method += f"    os << {field_name};\n"
    pack_method += "    return os;\n}\n\n"
    return pack_method

def generate_unpack_method(name, fields, codefield):
    """Generate unpack method for the class."""
    unpack_method = f"octetsstream& {name}::unpack(octetsstream& os)\n{{\n"
    if codefield:
        unpack_method += f"    os >> code;\n"
        for idx, (field_name, _, _) in enumerate(fields):
            unpack_method += f"    if (code & FIELDS_{field_name.upper()}) os >> {field_name};\n"
    else:
        for field_name, _, _ in fields:
            unpack_method += f"    os >> {field_name};\n"
    unpack_method += "    return os;\n}\n\n"
    return unpack_method

def parse_element(element, xml_mtime, base_class, header_output_directory, cpp_output_directory, force, codefield=None, default_code=None):
    """Parse protocol or RPC data element and generate corresponding header and implementation files."""
    global protocol_id_counter
    name = element.get('name')
    maxsize = element.get('maxsize')
    
    if base_class == "protocol" and maxsize is None:
        raise ValueError(f"Protocol '{name}' must have 'maxsize' attribute defined")

    fields = parse_fields(element, base_class, name)
    header_filename = os.path.join(header_output_directory, f"{name}.h")
    cpp_filename = os.path.join(cpp_output_directory, f"{name}.cpp")

    if not force and not check_regenerate(header_filename, cpp_filename, xml_mtime):
        print(f"Skipping generation for {name}, files are up to date.")
        return

    print(f"Generating header and cpp for {base_class}: {name}...")

    header_content = generate_class_header(name, base_class, fields, maxsize, codefield, default_code)
    cpp_content = generate_class_cpp(name, fields, base_class, codefield)

    write_file(header_filename, header_content)
    write_file(cpp_filename, cpp_content)

    if base_class == "protocol":
        protocol_enum_entries.append(f"PROTOCOL_TYPE_{name.upper()} = {protocol_id_counter}")
        protocol_id_counter += 1

def parse_fields(element, base_class, name):
    """Parse fields from XML element."""
    fields = []
    for field in element.findall('field'):
        field_name = field.get('name')
        field_type = field.get('type')
        default_value = field.get('default')
        
        if default_value is None:
            raise ValueError(f"Field '{field_name}' in {base_class} '{name}' does not have a default value")

        fields.append((field_name, field_type, default_value))
    return fields

def write_file(filename, content):
    """Write content to file."""
    try:
        with open(filename, 'w') as file:
            file.writelines(content)
    except IOError as e:
        print(f"Error writing to file {filename}: {e}")

def parse_xml_files(xmlpath, header_output_directory, cpp_output_directory, force):
    """Parse XML files and generate code."""
    xml_files = [file for file in os.listdir(xmlpath) if file.endswith('.xml')]
    for xml_file in xml_files:
        xml_path = os.path.join(xmlpath, xml_file)
        try:
            tree = ET.parse(xml_path)
        except ET.ParseError as e:
            print(f"Error parsing XML file {xml_path}: {e}")
            continue
        root = tree.getroot()
        xml_mtime = os.path.getmtime(xml_path)
        for element in root:
            if element.tag == 'protocol':
                parse_element(element, xml_mtime, "protocol", header_output_directory, cpp_output_directory, force, element.get('codefield'), element.get('default_code', '0'))
            elif element.tag == 'rpcdata':
                parse_element(element, xml_mtime, "rpcdata", header_output_directory, cpp_output_directory, force, element.get('codefield'), element.get('default_code', '0'))

def generate_protocol_definitions(outpath):
    """Generate protocol type definition file."""
    define_filename = os.path.join(outpath, "prot_define.h")
    try:
        with open(define_filename, 'w') as define_file:
            define_file.write("#pragma once\n")
            define_file.write("#include \"types.h\"\n\n")
            define_file.write(f"static constexpr PROTOCOLID MAXPROTOCOLID = {len(protocol_enum_entries)};\n\n")
            define_file.write("enum PROTOCOL_TYPE\n")
            define_file.write("{\n")
            for entry in protocol_enum_entries:
                define_file.write(f"    {entry},\n")
            define_file.write("};\n\n")
    except IOError as e:
        print(f"Error writing to file {define_filename}: {e}")

def main():
    """Main function, parses command line arguments and generates code."""
    parser = argparse.ArgumentParser(description="Generate C++ protocol and RPC data classes from XML definitions.")
    parser.add_argument('--xmlpath', required=True, help='Directory containing XML files')
    parser.add_argument('--outpath', required=True, help='Root directory for protocol definitions')
    parser.add_argument('--force', action='store_true', help='Force regeneration of all files')

    args = parser.parse_args()

    header_output_directory = os.path.join(args.outpath, "include")
    cpp_output_directory = os.path.join(args.outpath, "source")

    create_directories(header_output_directory, cpp_output_directory)
    clean_old_files(header_output_directory, cpp_output_directory, args.force)
    parse_xml_files(args.xmlpath, header_output_directory, cpp_output_directory, args.force)
    generate_protocol_definitions(args.outpath)
    print("Protocol definitions generated successfully.")

if __name__ == "__main__":
    main()
