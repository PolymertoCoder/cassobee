#!/usr/bin/python3

import xml.etree.ElementTree as ET
import os
import argparse
from collections import OrderedDict

protocol_enum_entries = dict()

modified_xml_files = []

basic_types = ["bool", "char", "int8_t", "uint8_t", "short", "int16_t", "uint16_t", "int", "int32_t", "uint32_t", "float", "double", "long", "long long", "int64_t", "uint64_t"]
stl_types = ["std::vector", "std::map", "std::set", "std::string", "std::pair", "std::unordered_set", "std::unordered_map"]

def check_regenerate(xmlpath, header_output_directory, cpp_output_directory):
    """Check if regeneration of code is necessary."""
    any_change_detected = False
    header_mtime = os.path.getmtime(header_output_directory)
    cpp_mtime = os.path.getmtime(cpp_output_directory)

    xml_files = [file for file in os.listdir(xmlpath) if file.endswith('.xml')]
    for xml_file in xml_files:
        xml_path = os.path.join(xmlpath, xml_file)
        xml_mtime = os.path.getmtime(xml_path)

        if xml_mtime > header_mtime or xml_mtime > cpp_mtime:
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

def generate_header_content(name, base_class, fields, type, maxsize, codefield=None, default_code=None):
    """Generate the content of the header file for the class."""
    header_content = []

    header_content.append(f"#pragma once\n")
    header_content.append(f'#include "{base_class}.h"\n\n')

    included_headers = generate_included_headers(fields)
    header_content.extend(included_headers)

    header_content.append("\nnamespace bee\n{\n\n")
    header_content.append(f"class {name} : public {base_class}\n{{\npublic:\n")

    if base_class == "protocol":
        header_content.append(f"    static constexpr PROTOCOLID TYPE = {type};\n")

    if codefield:
        header_content.append(generate_enum_fields(fields, default_code))

    if fields:
        header_content.append(generate_constructors(name, base_class, fields, codefield, default_code))
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

    header_content.append("\n}; // namespace bee\n")

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
        if field_type not in basic_types and not any(stl in field_type for stl in stl_types):
            included_headers.add(f'#include "{field_type}.h"\n')
    return included_headers

def generate_enum_fields(fields, default_code):
    """Generate enum fields for codefield."""
    enum_content = "    enum FIELDS\n    {\n"
    for idx, (field_name, _, _) in enumerate(fields):
        enum_content += f"        FIELDS_{field_name.upper()} = 1 << {idx},\n"
    enum_content += f"        ALLFIELDS = (1 << {len(fields)}) - 1\n    }};\n\n"
    return enum_content

def generate_constructors(name, base_class, fields, codefield, default_code):
    """Generate constructors for the class."""
    constructor_content = ""
    constructor_content += generate_default_constructor_params(name, base_class, fields, codefield, default_code)
    constructor_content += generate_non_basic_type_constructors(name, fields, codefield, default_code)

    constructor_content += f"    {name}(const {name}& rhs) = default;\n"
    constructor_content += f"    {name}({name}&& rhs) = default;\n\n"
    constructor_content += f"    {name}& operator=(const {name}& rhs) = default;\n"

    return constructor_content

def generate_default_constructor_params(name, base_class, fields, codefield, default_code):
    """Generate default constructor parameters."""
    constructor_content = ""
    constructor_content += f"    {name}() = default;\n"
    if base_class == "protocol":
        constructor_content += f"    {name}(PROTOCOLID type) : protocol(type)\n    {{}}\n"
    return constructor_content

def generate_non_basic_type_constructors(name, fields, codefield, default_code):
    """Generate constructors with const& and && parameters for non-basic types."""
    constructor_content = ""
    non_basic_fields = [(field_name, field_type) for field_name, field_type, _ in fields if field_type not in basic_types]

    # Generate constructors with const& parameters
    params = ", ".join([f"const {field_type}& _{field_name}" if field_type not in basic_types else f"{field_type} _{field_name}" for field_name, field_type, _ in fields])
    initializers = ", ".join([f"{field_name}(_{field_name})" for field_name, _, _ in fields])
    constructor_content += f"    {name}({params}) : "
    if codefield:
        constructor_content += f"code({default_code}), "
    constructor_content += f"{initializers}\n    {{}}\n"

    # Generate constructors with && parameters
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
    """Generate methods to handle codefield."""
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
    public_fields += "};\n"
    return public_fields

def generate_cpp_content(name, fields, base_class, codefield=None):
    """Generate the content of the cpp file for the class."""
    cpp_content = []

    cpp_content.append(f'#include "{name}.h"\n\n')
    cpp_content.append("namespace bee\n{\n\n")
    cpp_content.append(generate_pack_method(name, fields, codefield))
    cpp_content.append(generate_unpack_method(name, fields, codefield))
    if base_class == "protocol":
        cpp_content.append(f"\n__attribute__((weak)) void {name}::run() {{}}\n")
    cpp_content.append("\n} // namespace bee")

    return cpp_content

def generate_pack_method(name, fields, codefield):
    """Generate the pack method for the class."""
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
    """Generate the unpack method for the class."""
    unpack_method = f"octetsstream& {name}::unpack(octetsstream& os)\n{{\n"
    if codefield:
        unpack_method += f"    os >> code;\n"
        for idx, (field_name, _, _) in enumerate(fields):
            unpack_method += f"    if (code & FIELDS_{field_name.upper()}) os >> {field_name};\n"
    else:
        for field_name, _, _ in fields:
            unpack_method += f"    os >> {field_name};\n"
    unpack_method += "    return os;\n}\n"
    return unpack_method

def parse_element(element, base_class, header_output_directory, cpp_output_directory, type=None, codefield=None, default_code=None):
    """Parse protocol or rpcdata element and generate corresponding header and cpp files."""
    global protocol_enum_entries
    name = element.get('name')
    maxsize = element.get('maxsize')

    if base_class == "protocol":
        if type is None:
            raise ValueError(f"Protocol '{name}' must define 'type' attribute")
        if maxsize is None:
            raise ValueError(f"Protocol '{name}' must define 'maxsize' attribute")

    fields = parse_fields(element, base_class, name)
    header_filename = os.path.join(header_output_directory, f"{name}.h")
    cpp_filename = os.path.join(cpp_output_directory, f"{name}.cpp")

    print(f"Generating {base_class} header and cpp files: {name}...")

    header_content = generate_header_content(name, base_class, fields, type, maxsize, codefield, default_code)
    cpp_content = generate_cpp_content(name, fields, base_class, codefield)

    write_file(header_filename, header_content)
    write_file(cpp_filename, cpp_content)

    if base_class == "protocol":
        if protocol_enum_entries.get(type) != None:
            raise ValueError(f"Protocol '{name}' has duplicate type: {type}")
        protocol_enum_entries[type] = f"PROTOCOL_TYPE_{name.upper()}"

def parse_fields(element, base_class, name):
    """Parse fields from XML element."""
    fields = []
    for field in element.findall('field'):
        field_name = field.get('name')
        field_type = field.get('type')
        default_value = field.get('default')

        if default_value is None:
            raise ValueError(f"{base_class} '{name}' field '{field_name}' has no default value")

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
    xml_files.remove("protocol.xml")

    for xml_file in xml_files:
        xml_path = os.path.join(xmlpath, xml_file)
        try:
            tree = ET.parse(xml_path)
        except ET.ParseError as e:
            print(f"Error parsing XML file {xml_path}: {e}")
            continue
        root = tree.getroot()

        for element in root:
            parse_element(element, element.tag, header_output_directory, cpp_output_directory, element.get('type'), element.get('codefield'), element.get('default_code', '0'))

def generate_protocol_definitions(outpath):
    """Generate protocol type definition file."""
    if not protocol_enum_entries:
        print("No protocol enum entries to generate.")
        return

    sorted_protocol_enum_entries = sorted(protocol_enum_entries.items())
    define_filename = os.path.join(outpath, "prot_define.h")
    try:
        with open(define_filename, 'w') as define_file:
            define_file.write("#pragma once\n")
            define_file.write("#include \"types.h\"\n\n")
            lasttype, lastenum = sorted_protocol_enum_entries[-1]
            define_file.write(f"static constexpr PROTOCOLID MAXPROTOCOLID = {lasttype};\n\n")
            define_file.write("enum PROTOCOL_TYPE\n")
            define_file.write("{\n")
            for type, enum in sorted_protocol_enum_entries:
                define_file.write(f"    {enum} = {type},\n")
            define_file.write("};\n\n")
    except IOError as e:
        print(f"Error writing to file {define_filename}: {e}")

def parse_state(xmlpath, state_output_directory, force):
    """Parse XML files and generate corresponding header and cpp files."""
    xml_file = "protocol.xml"
    if xml_file.endswith('.xml'):
        xml_path = os.path.join(xmlpath, xml_file)
        tree = ET.parse(xml_path)
        root = tree.getroot()

        for element in root:
            if element.tag == "state":
                """Parse state element and generate corresponding state_xxx.cpp file."""
                state_name = element.get('name')
                if state_name is None:
                    raise ValueError("State must define 'name' attribute")

                state_filename = os.path.join(state_output_directory, f"state_{state_name}.cpp")
                state_headers_content = "#include \"protocol.h\"\n"
                register_content = ""

                enum2type = {}
                for protocol_type, protocol_enum in protocol_enum_entries.items():
                    if protocol_enum not in enum2type:
                        enum2type[protocol_enum] = protocol_type
                    else:
                        raise ValueError(f"Protocol name'{protocol_enum}' duplicated")

                for protocol in element.findall('protocol'):
                    protocol_name = str(protocol.get('name'))
                    protocol_enum = f"PROTOCOL_TYPE_{protocol_name.upper()}"
                    if protocol_enum not in enum2type.keys():
                        raise ValueError(f"Protocol '{protocol_name}' not defined")
                    protocol_type = enum2type[protocol_enum]
                    state_headers_content += f"#include \"{protocol_name}.h\"\n"
                    register_content += f"static bee::{protocol_name} __register_{protocol_name}_{protocol_type}({protocol_type});\n"

                state_content = f"{state_headers_content}\n{register_content}"
                write_file(state_filename, state_content)
            else:
                raise ValueError(f"protocol.xml element.tag {element.tag} must be state")

def main():
    """Main function, parses command line arguments and generates code."""
    parser = argparse.ArgumentParser(description="Generate C++ protocol and RPC data classes from XML definitions.")
    parser.add_argument('--xmlpath', required=True, help='Directory containing XML files')
    parser.add_argument('--outpath', required=True, help='Root directory for protocol definitions')
    parser.add_argument('--force', action='store_true', help='Force regeneration of all files')

    args = parser.parse_args()

    header_output_directory = os.path.join(args.outpath, "include")
    cpp_output_directory = os.path.join(args.outpath, "source")
    state_output_directory = os.path.join(args.outpath, "state")

    create_directories(header_output_directory, cpp_output_directory, state_output_directory)
    clean_old_files(args.xmlpath, header_output_directory, cpp_output_directory, args.force)
    parse_xml_files(args.xmlpath, header_output_directory, cpp_output_directory, args.force)
    generate_protocol_definitions(args.outpath)
    parse_state(args.xmlpath, state_output_directory, args.force)
    print("Protocol definitions generated successfully.")

if __name__ == "__main__":
    main()
