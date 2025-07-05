#!/usr/bin/python3

import os
import sys

def generate_run_script(executable_path, script_path, target, preprocess_cmd, options):
    script_path = os.path.join(script_path, f"run_{target}.sh")
    with open(script_path, 'w') as f:
        #f.write("#!/bin/sh\n")
        f.write(f"{preprocess_cmd}\n")
        f.write(f"nohup {os.path.join(executable_path, target)} {options}\n")
    os.chmod(script_path, 0o755)
    #print(f"Generated {script_path}")

def generate_gdb_run_script(executable_path, script_path, target, preprocess_cmd, options):
    script_path = os.path.join(script_path, f"gdbrun_{target}.sh")
    with open(script_path, 'w') as f:
        #f.write("#!/bin/sh\n")
        f.write(f"{preprocess_cmd}\n")
        f.write(f"gdb --args {os.path.join(executable_path, target)} {options}\n")
    os.chmod(script_path, 0o755)
    #print(f"Generated {script_path}")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: generate_scripts.py <target> <script_path> <config_path> <output_path>")
        sys.exit(1)

    executable_path = sys.argv[1]
    script_path = sys.argv[2]
    target = sys.argv[3]

    preprocess_cmd = "ulimit -s unlimited;"
    options = ""
    gdbrun_options = ""

    # --config <config_path>/<target>.conf
    if len(sys.argv) >= 5:
        config_path = sys.argv[4]
        config_filename = f"{target}.conf"
        options += (f"--config {os.path.join(config_path, config_filename)} ")
        gdbrun_options += (f"--config {os.path.join(config_path, config_filename)} ")
    
    # > <output_path>/<target>.log 2>&1 &
    if len(sys.argv) >= 6:
        output_path = sys.argv[5]
        output_filename = f"{target}.log"
        options += (f"> {os.path.join(output_path, output_filename)} 2>&1 & ")
    
    if not os.path.exists(script_path):
        os.makedirs(script_path)
    generate_run_script(executable_path, script_path, target, preprocess_cmd, options)
    generate_gdb_run_script(executable_path, script_path, target, preprocess_cmd, gdbrun_options)
