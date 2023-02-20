#!/bin/env python

# This script agenerates the .vscode/c_cpp_properties.json needed for VSCode C/C++ extension.
# Run from project home folder.
# Expects cmake to generate build folder.

import re
import json
import os

# Run cmake to generate compile_commands.json
res = os.system("cd build && cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 . && cd ..")

if res > 0:
    print("Oh no, CMake threw an error!")
    exit()

# Extract compiler path, defines and include directories from compile_commands.json
with open("build/compile_commands.json") as f:
    # Rely on info for first source file, which is usually main.c
    c = json.load(f)[0]

compiler = re.findall(r"^([^ ]+)", c["command"])[0]
defines = re.findall(r"-D([^ ]+)", c["command"])
includes = re.findall(r"-I([^ ]+)", c["command"])

# Craft new c_cpp_properties.json
properties = {
    "configurations": [
    {
        "name": "PicoAutogen",
        "includePath": sorted(includes),
        "defines": sorted(defines),
        "compilerPath": compiler,
        "cStandard": "gnu17",
        "cppStandard": "gnu++14",
        "intelliSenseMode": "linux-gcc-arm",
        "compilerArgs": []
    }], 
    "version": 4
}

# Write new JSON to file
with open(".vscode/c_cpp_properties.json", "w") as f:
    json.dump(obj = properties, fp = f, sort_keys = False, indent = 4)

print("New c_cpp_properties.json has been written to folder .vscode")