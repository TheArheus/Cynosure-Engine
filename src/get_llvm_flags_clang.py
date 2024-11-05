import os
import re
import subprocess

# Run the llvm-config command and get the output
llvm_output = subprocess.check_output("llvm-config --cxxflags --ldflags --system-libs --libs all", shell=True, text=True).strip().splitlines()
llvm_output[1] = " ".join((llvm_output[1], "-LIBPATH:C:\\vcpkg\\installed\\x64-windows\\lib"))

clang_flags_replacements = {
    '/EHs-c-': '-fexceptions',  # C++ exceptions support
    '/GR-': '-fno-rtti',        # Disable Run-Time Type Info
    '-std:c++17': '-std=c++17'  # Change colon to equals
}

for original, replacement in clang_flags_replacements.items():
    llvm_output[0] = llvm_output[0].replace(original, replacement)

# Replace function to add quotes to the path if necessary
def wrap_with_quotes(match):
    flag = match.group(1)
    path = match.group(2)
    
    # Check if the path contains spaces and is not already quoted
    if ' ' in path and not (path.startswith('"') and path.endswith('"')):
        return f'{flag}"{path}"'
    else:
        return match.group(0)  # Return as is if already quoted or no spaces

# Substitute the matched patterns with the quoted equivalents
llvm_output[0] = re.sub(r'(-I\s*)([^\s]+(?:\s[^\s-][^\s]*)*)', wrap_with_quotes, llvm_output[0])

# Modify the second string to format the LIBPATH correctly
lib_names = re.findall(r'-LIBPATH:(.*?)(?=\s-|$)', llvm_output[1])
adjusted_lib_names = ' '.join(f'-L"{lib.strip()}"' for lib in lib_names)
llvm_output[1] = adjusted_lib_names

# Modify the third string to correctly format paths with -l and quotes
# Use regex to capture full paths
lib_names = re.findall(r'[^\\]+(?=\.lib)', llvm_output[2])
adjusted_lib_names = ' '.join(f'-l{lib}' for lib in lib_names)
# lib_names = re.findall(r'([A-Z]:[\\][^"]+?\.lib)', llvm_output[2])
# adjusted_lib_names = ' '.join(f'"{lib}"' for lib in lib_names)
llvm_output[2] = adjusted_lib_names

# Modify the fourth string to add -l prefix to each lib
lib_names = [lib.replace('.lib', '') for lib in llvm_output[3].split()]
adjusted_lib_names = ' '.join(f"-l{lib}" for lib in lib_names)
llvm_output[3] = adjusted_lib_names

llvm_output_string = ' '.join(llvm_output)

# Print the final combined string
print(llvm_output_string)
