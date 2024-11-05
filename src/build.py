import os
import sys
import subprocess
import shutil
from pathlib import Path
import platform

def main():
    if len(sys.argv) < 2:
        print("Usage: python build.py [debug|release]")
        sys.exit(1)

    build_type = sys.argv[1].lower()
    if build_type not in ['debug', 'release']:
        print(f"Unknown build type '{build_type}'. Use 'debug' or 'release'.")
        sys.exit(1)

    # Detect the operating system
    os_name = platform.system()
    is_windows = os_name == 'Windows'
    is_linux = os_name == 'Linux'

    if not (is_windows or is_linux):
        print(f"Unsupported operating system: {os_name}")
        sys.exit(1)

    # Ensure necessary environment variables are set
    vulkan_sdk = os.environ.get('VULKAN_SDK')
    if not vulkan_sdk:
        print("Error: VULKAN_SDK environment variable is not set.")
        sys.exit(1)

    # Set up compiler and linker flags based on OS and build type
    if is_windows:
        setup_environment_windows()
    elif is_linux:
        setup_environment_linux()

    # Common settings
    depth_cascades = '-DDEPTH_CASCADES_COUNT=3'
    use_debug_color_blend = '-DDEBUG_COLOR_BLEND=0'
    gbuffer_count = '-DGBUFFER_COUNT=5'
    light_sources_max = '-DLIGHT_SOURCES_MAX_COUNT=256'
    voxel_grid_size = '-DVOXEL_SIZE=128'

    # Build-specific settings
    if build_type == 'debug':
        if is_windows:
            common_comp_flags.extend(['-MDd', '-Od', '-DCE_DEBUG'])
            build_suffix = 'd'
        else:
            common_comp_flags.extend(['-O0', '-g3', '-DCE_DEBUG'])
    else:
        if is_windows:
            common_comp_flags.extend(['-MD', '-O2'])
            build_suffix = ''
        else:
            common_comp_flags.extend(['-O3'])

    # Create necessary directories
    build_dir = Path('..') / 'build'
    scenes_dir = build_dir / 'scenes'
    build_dir.mkdir(parents=True, exist_ok=True)
    scenes_dir.mkdir(parents=True, exist_ok=True)

    # Clean up old files
    for pdb_file in scenes_dir.glob('*.pdb'):
        pdb_file.unlink()
    for pdb_file in build_dir.glob('*.pdb'):
        pdb_file.unlink()

    # Build game scenes
    build_game_scenes(scenes_dir, build_type, depth_cascades, is_windows)

    # Build and run reflection program
    build_generate_exe(build_dir)
    run_generate_exe(build_dir)

    # Build main executable
    build_main_executable(build_dir, build_type, is_windows, depth_cascades, use_debug_color_blend,
                          gbuffer_count, light_sources_max, voxel_grid_size)

    print("Build completed successfully.")

def setup_environment_windows():
    global common_comp_flags, common_link_flags, platform_cpp_files, compiler_cmd
    # Call vcvarsall.bat to set up the Visual Studio environment
    vs_install_dir = os.environ.get('VSINSTALLDIR', '')
    if not vs_install_dir:
        print("Error: VSINSTALLDIR environment variable is not set.")
        sys.exit(1)
    vcvarsall = os.path.join(vs_install_dir, 'VC', 'Auxiliary', 'Build', 'vcvarsall.bat')
    if not os.path.exists(vcvarsall):
        print("Error: Visual Studio build tools are not found.")
        sys.exit(1)
    os.system(f'"{vcvarsall}" x64 >nul')

    # Set up environment variables
    os.environ['VulkanInc'] = os.path.join(os.environ["VULKAN_SDK"], "Include")
    os.environ['VulkanLib'] = os.path.join(os.environ["VULKAN_SDK"], "Lib")

    # Compiler and linker flags
    common_comp_flags = [
        '/std:c++latest', '/Zc:__cplusplus', '-fp:fast', '-nologo', '-EHsc',
        '-Oi', '-WX-', '-W4', '-GR', '-Gm-', '-GS', '-FC', '-Zi', '-D_MBCS',
        '-wd4005', '-wd4100', '-wd4127', '-wd4189', '-wd4201', '-wd4238',
        '-wd4244', '-wd4267', '-wd4315', '-wd4324', '-wd4505', '-wd4715'
    ]
    common_link_flags = ['-opt:ref', '-incremental:no', '/SUBSYSTEM:console', '/ignore:4099']
    platform_cpp_files = [os.path.join('..', 'src', 'win32_main.cpp')]
    compiler_cmd = 'cl'

def setup_environment_linux():
    global common_comp_flags, common_link_flags, platform_cpp_files, compiler_cmd
    # Set up environment variables
    os.environ['VulkanInc'] = os.path.join(os.environ["VULKAN_SDK"], "Include")
    os.environ['VulkanLib'] = os.path.join(os.environ["VULKAN_SDK"], "Lib")

    # Compiler and linker flags
    common_comp_flags = [
        '-std=c++2a', '-ffast-math', '-w', '-Wall', '-Wextra',
        '-Wno-error', '-pthread', '-MD', '-frtti', '-fPIC', '-march=native'
    ]
    common_link_flags = ['-pthread']
    platform_cpp_files = ['../src/linux_main.cpp']
    compiler_cmd = 'g++'

def build_game_scenes(scenes_dir, build_type, depth_cascades, is_windows):
    game_scenes_dir = Path('..') / 'src' / 'game_scenes'
    for cpp_file in game_scenes_dir.glob('*.cpp'):
        base_name = cpp_file.stem
        export_name = process_tokens(base_name) + 'Create'

        # Compute the path to the cpp file relative to scenes_dir
        relative_cpp_file = os.path.relpath(cpp_file.resolve(), start=scenes_dir.resolve())

        if is_windows:
            include_dirs = [
                os.path.join('..', '..', 'src'),
                os.path.join('..', '..', 'src', 'core', 'vendor')
            ]
            cmd = [
                'cl',
                *common_comp_flags,
                *[f'/I{include_dir}' for include_dir in include_dirs],
                relative_cpp_file,
                os.path.join('..', 'libs', 'assimp-vc143-mt.lib'),
                '/LD',
                f'/Fe{base_name}',
                depth_cascades,
                '-DENGINE_EXPORT_CODE',
                '/link',
                *common_link_flags,
                f'/EXPORT:{export_name}',
                f'/LIBPATH:{os.path.join("..", "..", "libs")}',
                f'-PDB:ce_{base_name}_{os.urandom(4).hex()}.pdb'
            ]
        else:
            include_dirs = [
                '../../src',
                '../../src/core/vendor'
            ]
            cmd = [
                'g++',
                *common_comp_flags,
                *[f'-I{include_dir}' for include_dir in include_dirs],
                relative_cpp_file,
                '-lassimp',
                '-shared',
                '-o', f'{base_name}.lscene',
                depth_cascades,
                '-DENGINE_EXPORT_CODE'
            ]
        # print(' '.join(cmd))
        subprocess.run(cmd, check=True, cwd=scenes_dir)

def build_generate_exe(build_dir):
    # Windows-specific function to build generate.exe
    try:
        result = subprocess.run(
            ['python', 'get_llvm_flags_clang.py'],
            capture_output=True, text=True, check=True
        )
        llvm_flags = result.stdout.strip()
    except subprocess.CalledProcessError as e:
        print("Error fetching LLVM flags:", e)
        sys.exit(1)

    cmd = f'clang++ -fms-runtime-lib=dll ..\\generate.cpp -o generate.exe ' \
           '-lversion -lmsvcrt -lclangAPINotes -lclangEdit -lclangBasic ' \
           '-lclangTooling -lclangFrontendTool -lclangCodeGen -lclangARCMigrate ' \
           '-lclangRewrite -lclangRewriteFrontend -lclangASTMatchers -lclangSerialization ' \
           '-lclangSema -lclangStaticAnalyzerFrontend -lclangStaticAnalyzerCheckers ' \
           '-lclangStaticAnalyzerCore -lclangAnalysis -lclangDriver -lclangFrontend ' \
           '-lclangParse -lclangAST -lclangLex -lclangSupport ' \
           f'{llvm_flags} -Xlinker /NODEFAULTLIB:libcmt.lib'
    # print(cmd)
    subprocess.run(cmd, check=True, cwd=build_dir)

def run_generate_exe(build_dir):
    vulkan_sdk_include = os.path.join(os.environ['VULKAN_SDK'], 'Include')
    src_include = os.path.abspath(os.path.join('..', 'src'))
    vendor_include = os.path.abspath(os.path.join('..', 'src', 'core', 'vendor'))

    extra_args = [
        '-std=c++20',
        '-w',
        '-Wall',
        '-Wextra',
        '-Wno-error',
        '-Wno-narrowing',
        '-DDEPTH_CASCADES_COUNT=3',
        '-DDEBUG_COLOR_BLEND=0',
        '-DGBUFFER_COUNT=5',
        '-DLIGHT_SOURCES_MAX_COUNT=256',
        '-DVOXEL_SIZE=128',
        f'-I{vulkan_sdk_include}',
        f'-I{src_include}',
        f'-I{vendor_include}'
    ]

    generate_exe = os.path.abspath(os.path.join(build_dir, 'generate.exe'))

    if not os.path.exists(generate_exe):
        print(f"Error: generate.exe not found at {generate_exe}")
        sys.exit(1)

    main_cpp = os.path.abspath(os.path.join('..', 'src', 'main.cpp'))

    if not os.path.exists(main_cpp):
        print(f"Error: main.cpp not found at {main_cpp}")
        sys.exit(1)

    cmd = [generate_exe]
    cmd += [f'--extra-arg-before={arg}' for arg in extra_args]
    cmd.append(main_cpp)

    # print(' '.join(cmd))
    subprocess.run(cmd, check=True)

def build_main_executable(build_dir, build_type, is_windows, depth_cascades, use_debug_color_blend,
                          gbuffer_count, light_sources_max, voxel_grid_size):
    if is_windows:
        build_main_executable_windows(build_dir, build_type, depth_cascades, use_debug_color_blend,
                                      gbuffer_count, light_sources_max, voxel_grid_size)
    else:
        build_main_executable_linux(build_dir, build_type, depth_cascades, use_debug_color_blend,
                                    gbuffer_count, light_sources_max, voxel_grid_size)

def build_main_executable_windows(build_dir, build_type, depth_cascades, use_debug_color_blend,
                                  gbuffer_count, light_sources_max, voxel_grid_size):
    build_suffix = 'd' if build_type == 'debug' else ''
    libraries = [
        'user32.lib', 'kernel32.lib', 'gdi32.lib', 'shell32.lib',
        'd3d12.lib', 'dxgi.lib', 'dxguid.lib', 'd3dcompiler.lib',
        '..\\libs\\dxcompiler.lib', 'vulkan-1.lib',
        f'glslang{build_suffix}.lib', f'HLSL{build_suffix}.lib',
        f'OGLCompiler{build_suffix}.lib', f'OSDependent{build_suffix}.lib',
        f'MachineIndependent{build_suffix}.lib', f'SPIRV{build_suffix}.lib',
        f'SPIRV-Tools{build_suffix}.lib', f'SPIRV-Tools-opt{build_suffix}.lib',
        f'GenericCodeGen{build_suffix}.lib', f'glslang-default-resource-limits{build_suffix}.lib',
        f'SPVRemapper{build_suffix}.lib', f'spirv-cross-core{build_suffix}.lib',
        f'spirv-cross-cpp{build_suffix}.lib', f'spirv-cross-glsl{build_suffix}.lib',
        f'spirv-cross-hlsl{build_suffix}.lib', '..\\libs\\assimp-vc143-mt.lib'
    ]

    include_dirs = [
        os.environ["VulkanInc"],
        os.path.join('..', 'src'),
        os.path.join('..', 'src', 'core', 'vendor')
    ]

    source_files = [
        os.path.join('..', 'src', 'main.cpp'),
        *platform_cpp_files
    ]

    common_comp_flags_str = ' '.join(common_comp_flags)
    include_dirs_str = ' '.join([f'/I"{dir}"' for dir in include_dirs])
    source_files_str = ' '.join([f'"{file}"' for file in source_files])
    libraries_str = ' '.join(libraries)
    common_link_flags_str = ' '.join(common_link_flags)

    cmd = f'cl {common_comp_flags_str} {include_dirs_str} {source_files_str} ' \
          f'{use_debug_color_blend} {depth_cascades} {gbuffer_count} ' \
          f'{light_sources_max} {voxel_grid_size} ' \
          f'/Fe"Cynosure Engine.exe" {libraries_str} /link {common_link_flags_str} ' \
          f'/LIBPATH:"{os.environ["VulkanLib"]}" -PDB:ce_{os.urandom(4).hex()}.pdb'

    # print(cmd)
    subprocess.run(cmd, check=True, cwd=build_dir, shell=True)

def build_main_executable_linux(build_dir, build_type, depth_cascades, use_debug_color_blend,
                                gbuffer_count, light_sources_max, voxel_grid_size):
    libraries = [
        '-ldxcompiler', '-ldl', '-lglfw', '-lglslang', '-lvulkan',
        '-lMachineIndependent', '-lOSDependent', '-lGenericCodeGen',
        '-lOGLCompiler', '-lSPIRV', '-lSPIRV-Tools', '-lSPIRV-Tools-opt',
        '-lglslang-default-resource-limits', '-lSPVRemapper',
        '-lspirv-cross-core', '-lspirv-cross-cpp', '-lspirv-cross-glsl',
        '-lspirv-cross-hlsl', '-lassimp', '-lpthread'
    ]

    include_dirs = [
        '../src',
        '../src/core/vendor'
    ]

    source_files = [
        '../src/main.cpp',
        *platform_cpp_files
    ]

    cmd = [
        'g++',
        *common_comp_flags,
        *source_files,
        *[f'-I{include_dir}' for include_dir in include_dirs],
        '-Wl,-rpath=../libs/', '-L../libs/',
        '-o', '"Cynosure Engine"',
        *libraries,
        use_debug_color_blend, depth_cascades, gbuffer_count,
        light_sources_max, voxel_grid_size
    ]
    # print(' '.join(cmd))
    subprocess.run(cmd, check=True, cwd=build_dir)

def process_tokens(base_name):
    tokens = base_name.split('_')
    export_name = ''
    for token in tokens:
        export_name += token.capitalize()
    return export_name

if __name__ == '__main__':
    main()
