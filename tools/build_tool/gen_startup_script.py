#!/usr/bin/python3

import os
import sys

def generate_run_script(target, output_dir):
    script_path = os.path.join(output_dir, f"run_{target}.sh")
    with open(script_path, 'w') as f:
        #f.write("#!/bin/sh\n")
        f.write(f"nohup {os.path.join(output_dir, target)} --config > \n")
    os.chmod(script_path, 0o755)
    print(f"Generated {script_path}")

def generate_gdb_run_script(target, output_dir):
    script_path = os.path.join(output_dir, f"gdbrun_{target}.sh")
    with open(script_path, 'w') as f:
        #f.write("#!/bin/sh\n")
        f.write(f"gdb --args {os.path.join(output_dir, target)}\n")
    os.chmod(script_path, 0o755)
    print(f"Generated {script_path}")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: generate_scripts.py <target> <output_dir>")
        sys.exit(1)

    target = sys.argv[1]
    output_dir = sys.argv[2]
    
    generate_run_script(target, output_dir)
    generate_gdb_run_script(target, output_dir)
