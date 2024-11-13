import os
import sys
import subprocess
import shutil
from pathlib import Path
import platform
import logging
import random
import string
import time

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='[%(levelname)s] %(message)s'
)

# Global common defines
COMMON_DEFINES = [
    '-DDEPTH_CASCADES_COUNT=3',
    '-DDEBUG_COLOR_BLEND=0',
    '-DGBUFFER_COUNT=5',
    '-DLIGHT_SOURCES_MAX_COUNT=256',
    '-DVOXEL_SIZE=128'
]

def generate_random_hex(length=8):
    """Generate a random hexadecimal string."""
    return ''.join(random.choices('0123456789abcdef', k=length))

class BuildSettings:
    """Class to hold build settings based on the platform and build type."""

    def __init__(self, build_type):
        self.build_type = build_type.lower()
        self.validate_build_type()
        self.os_name = platform.system()
        self.is_windows = self.os_name == 'Windows'
        self.is_linux = self.os_name == 'Linux'
        self.settings = {}
        self.configure_environment()
        self.configure_compiler_settings()

    def validate_build_type(self):
        if self.build_type not in ['debug', 'release']:
            logging.error(f"Unknown build type '{self.build_type}'. Use 'debug' or 'release'.")
            sys.exit(1)

    def configure_environment(self):
        """Set up Vulkan SDK paths and other environment variables."""
        vulkan_sdk = os.environ.get('VULKAN_SDK')
        if not vulkan_sdk:
            if self.is_windows:
                logging.error("VULKAN_SDK environment variable is not set.")
                sys.exit(1)
            else:
                # On Linux, use standard paths
                self.vulkan_inc = Path('/usr/include')
                self.vulkan_lib = Path('/usr/lib')
                if not (self.vulkan_inc / 'vulkan' / 'vulkan.h').exists():
                    logging.error("Error: Vulkan headers not found in standard include path.")
                    sys.exit(1)
        else:
            if self.is_windows:
                self.vulkan_inc = Path(vulkan_sdk) / 'Include'
                self.vulkan_lib = Path(vulkan_sdk) / 'Lib'
            else:
                self.vulkan_inc = Path(vulkan_sdk) / 'include'
                self.vulkan_lib = Path(vulkan_sdk) / 'lib'
                if not (self.vulkan_inc / 'vulkan' / 'vulkan.h').exists():
                    logging.error("Error: Vulkan headers not found in VULKAN_SDK include path.")
                    sys.exit(1)

        self.settings['VulkanInc'] = str(self.vulkan_inc)
        self.settings['VulkanLib'] = str(self.vulkan_lib)

    def configure_compiler_settings(self):
        """Configure compiler and linker settings based on the platform."""
        if self.is_windows:
            self.configure_windows_settings()
        elif self.is_linux:
            self.configure_linux_settings()
        else:
            logging.error(f"Unsupported operating system: {self.os_name}")
            sys.exit(1)

    def configure_windows_settings(self):
        """Configure settings specific to Windows."""
        vs_install_dir = os.environ.get('VSINSTALLDIR')
        if not vs_install_dir:
            logging.error("VSINSTALLDIR environment variable is not set.")
            sys.exit(1)
        
        vcvarsall = Path(vs_install_dir) / 'VC' / 'Auxiliary' / 'Build' / 'vcvarsall.bat'
        if not vcvarsall.exists():
            logging.error("Visual Studio build tools are not found.")
            sys.exit(1)
        
        # Initialize Visual Studio environment using 'call'
        cmd = f'cmd /c "call \"{vcvarsall}\" x64 && set"'
        result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
        if result.returncode != 0:
            logging.error("Failed to set up Visual Studio environment.")
            logging.error(result.stderr)
            sys.exit(1)
        
        # Update environment variables
        for line in result.stdout.splitlines():
            if '=' in line:
                key, value = line.split('=', 1)
                os.environ[key] = value

        # Compiler and linker flags
        self.common_comp_flags = [
            '/std:c++latest', '/Zc:__cplusplus', '/fp:fast', '/nologo', '/EHsc',
            '/Oi', '/WX-', '/W4', '/GR', '/Gm-', '/GS', '/FC', '/Zi', '/D_MBCS',
            '/wd4005', '/wd4100', '/wd4127', '/wd4189', '/wd4201', '/wd4238',
            '/wd4244', '/wd4267', '/wd4315', '/wd4324', '/wd4505', '/wd4715'
        ]

        self.common_link_flags = ['/opt:ref', '/incremental:no', '/SUBSYSTEM:console', '/ignore:4099']

        self.platform_cpp_files = [str(Path('..') / 'src' / 'win32_main.cpp')]
        self.compiler_cmd = 'cl'

        self.include_dirs = [
            self.settings['VulkanInc'],
            str(Path('..') / 'src'),
            str(Path('..') / 'src' / 'core' / 'vendor')
        ]

        # Build-specific flags
        if self.build_type == 'debug':
            self.common_comp_flags.extend(['/MDd', '/Od', '/DCE_DEBUG'])
            self.build_suffix = 'd'
        else:
            self.common_comp_flags.extend(['/MD', '/O2'])
            self.build_suffix = ''

        self.libraries = [
            'user32.lib', 'kernel32.lib', 'gdi32.lib', 'shell32.lib',
            'd3d12.lib', 'dxgi.lib', 'dxguid.lib', 'd3dcompiler.lib',
            str(Path('..') / 'libs' / 'dxcompiler.lib'), 'vulkan-1.lib',
            f'glslang{self.build_suffix}.lib', f'HLSL{self.build_suffix}.lib',
            f'OGLCompiler{self.build_suffix}.lib', f'OSDependent{self.build_suffix}.lib',
            f'MachineIndependent{self.build_suffix}.lib', f'SPIRV{self.build_suffix}.lib',
            f'SPIRV-Tools{self.build_suffix}.lib', f'SPIRV-Tools-opt{self.build_suffix}.lib',
            f'GenericCodeGen{self.build_suffix}.lib', f'glslang-default-resource-limits{self.build_suffix}.lib',
            f'SPVRemapper{self.build_suffix}.lib', f'spirv-cross-core{self.build_suffix}.lib',
            'spirv-cross-cpp.lib', f'spirv-cross-glsl{self.build_suffix}.lib', f'spirv-cross-hlsl{self.build_suffix}.lib',
            str(Path('..') / 'libs' / 'assimp-vc143-mt.lib')
        ]

    def configure_linux_settings(self):
        """Configure settings specific to Linux."""
        self.common_comp_flags = [
            '-std=c++2a', '-ffast-math', '-w', '-Wall', '-Wextra',
            '-Wno-error', '-pthread', '-MD', '-frtti', '-fPIC', '-march=native'
        ]

        self.common_link_flags = ['-pthread']

        self.platform_cpp_files = ['../src/linux_main.cpp']
        self.compiler_cmd = 'g++'

        self.include_dirs = [
            self.settings['VulkanInc'],
            '../src',
            '../src/core/vendor'
        ]

        # Build-specific flags
        if self.build_type == 'debug':
            self.common_comp_flags.extend(['-O0', '-g3', '-DCE_DEBUG'])
        else:
            self.common_comp_flags.extend(['-O3'])

        self.libraries = [
            'dxcompiler', 'dl', 'glfw', 'glslang', 'vulkan',
            'MachineIndependent', 'OSDependent', 'GenericCodeGen',
            'OGLCompiler', 'SPIRV', 'SPIRV-Tools', 'SPIRV-Tools-opt',
            'glslang-default-resource-limits', 'SPVRemapper',
            'spirv-cross-core', 'spirv-cross-cpp', 'spirv-cross-glsl',
            'spirv-cross-hlsl', 'assimp', 'pthread'
        ]

        self.build_suffix = ''

def main():
    start_time = time.perf_counter()  # **Capture the start time**
    try:
        if len(sys.argv) < 2:
            logging.error("Usage: python build.py [debug|release]")
            sys.exit(1)

        build_type = sys.argv[1].lower()
        settings = BuildSettings(build_type)

        # Create necessary directories
        build_dir = Path('..') / 'build'
        scenes_dir = build_dir / 'scenes'
        build_dir.mkdir(parents=True, exist_ok=True)
        scenes_dir.mkdir(parents=True, exist_ok=True)

        clean_old_pdb_files(build_dir, scenes_dir)

        # Build game scenes
        build_game_scenes(scenes_dir, settings)

        # Build and run reflection program
        build_generate_exe(build_dir, settings)
        run_generate_exe(build_dir, settings)

        # Build main executable
        build_main_executable(build_dir, settings)

        logging.info("Build completed successfully.")
    except Exception as e:
        logging.error(f"Build failed: {e}")
        sys.exit(1)
    finally:
        end_time = time.perf_counter()  # **Capture the end time**
        elapsed_time = end_time - start_time
        formatted_time = format_elapsed_time(elapsed_time)
        logging.info(f"Total build time: {formatted_time}")

def clean_old_pdb_files(build_dir, scenes_dir):
    """Remove old PDB files from build and scenes directories."""
    for pdb_file in scenes_dir.glob('*.pdb'):
        pdb_file.unlink()
    for pdb_file in build_dir.glob('*.pdb'):
        pdb_file.unlink()

def build_game_scenes(scenes_dir, settings):
    """Compile all game scene CPP files."""
    game_scenes_dir = Path('..') / 'src' / 'game_scenes'
    for cpp_file in game_scenes_dir.glob('*.cpp'):
        base_name = cpp_file.stem
        export_name = process_tokens(base_name) + 'Create'
        relative_cpp_file = os.path.relpath(cpp_file.resolve(), start=scenes_dir.resolve())

        include_dirs = [
            str(Path('..') / '..' / 'src'),
            str(Path('..') / '..' / 'src' / 'core' / 'vendor')
        ]

        cmd = construct_compile_command(
            compiler_cmd=settings.compiler_cmd,
            common_comp_flags=settings.common_comp_flags,
            include_dirs=include_dirs,
            cpp_file=relative_cpp_file,
            base_name=base_name,  # Pass base_name to the function
            libraries=settings.libraries,
            common_link_flags=settings.common_link_flags,
            is_windows=settings.is_windows,
            common_defines=COMMON_DEFINES,
            export_name=export_name,
            build_suffix=settings.build_suffix
        )

        logging.info(f"Building scene: {cpp_file.name}")
        try:
            subprocess.run(cmd, check=True, cwd=scenes_dir)
        except subprocess.CalledProcessError as e:
            logging.error(f"Failed to build scene {cpp_file.name}: {e}")
            sys.exit(1)

def construct_compile_command(
    compiler_cmd, common_comp_flags, include_dirs, cpp_file, base_name, 
    libraries, common_link_flags, is_windows, common_defines, export_name, build_suffix
):
    """Construct the compile command based on the platform."""
    if is_windows:
        pdb_file = f'ce_{base_name}_{generate_random_hex(8)}.pdb'
        cmd = [
            compiler_cmd,
            *common_comp_flags,
            *[f'/I{dir}' for dir in include_dirs],
            cpp_file,
            str(Path('..') / 'libs' / 'assimp-vc143-mt.lib'),
            '/LD',
            *common_defines,
            '/DENGINE_EXPORT_CODE',
            '/link',
            *common_link_flags,
            f'/EXPORT:{export_name}',
            f'/LIBPATH:{str(Path("..") / ".." / "libs")}',
            f'/PDB:{pdb_file}'
        ]
    else:
        # For Linux, cpp_file is a string; base_name is used to name the output
        cmd = [
            compiler_cmd,
            *common_comp_flags,
            *[f'-I{dir}' for dir in include_dirs],
            cpp_file,
            '-lassimp',
            '-shared',
            '-o', f'{base_name}.lscene',
            *common_defines,
            '-DENGINE_EXPORT_CODE'
        ]
    return cmd

def build_generate_exe(build_dir, settings):
    """Build the generate executable."""
    if settings.is_windows:
        build_generate_exe_windows(build_dir, settings)
    elif settings.is_linux:
        build_generate_exe_linux(build_dir, settings)
    else:
        logging.error("Unsupported platform")
        sys.exit(1)

def build_generate_exe_windows(build_dir, settings):
    """Build generate.exe on Windows using clang++."""
    try:
        result = subprocess.run(
            ['python', 'get_llvm_flags_clang.py'],
            capture_output=True, text=True, check=True
        )
        llvm_flags = result.stdout.strip()
        if not llvm_flags:
            logging.error("LLVM flags are empty. Please check get_llvm_flags_clang.py.")
            sys.exit(1)
    except subprocess.CalledProcessError as e:
        logging.error("Error fetching LLVM flags:")
        logging.error(e.stderr)
        sys.exit(1)

    # Construct the command as a single string
    cmd = (
        f'clang++ -fms-runtime-lib=dll "..\\generate.cpp" -o "generate.exe" '
        f'-lversion -lmsvcrt -lclangAPINotes -lclangEdit -lclangBasic '
        f'-lclangTooling -lclangFrontendTool -lclangCodeGen -lclangARCMigrate '
        f'-lclangRewrite -lclangRewriteFrontend -lclangASTMatchers -lclangSerialization '
        f'-lclangSema -lclangStaticAnalyzerFrontend -lclangStaticAnalyzerCheckers '
        f'-lclangStaticAnalyzerCore -lclangAnalysis -lclangDriver -lclangFrontend '
        f'-lclangParse -lclangAST -lclangLex -lclangSupport '
        f'{llvm_flags} -Xlinker /NODEFAULTLIB:libcmt.lib'
    )

    # Optional: Log the command for debugging
    logging.debug(f"Clang++ command: {cmd}")

    logging.info("Building generate.exe with clang++")
    try:
        subprocess.run(cmd, check=True, cwd=build_dir, shell=True)
    except subprocess.CalledProcessError as e:
        logging.error(f"Failed to build generate.exe: {e}")
        sys.exit(1)

def build_generate_exe_linux(build_dir, settings):
    """Build generate executable on Linux."""
    # Construct the command as a single string to handle backticks properly
    cmd_str = (
        'clang++ ../generate.cpp -o generate '
        '`llvm-config --cxxflags` -Wl,--start-group '
        '`llvm-config --ldflags --system-libs --libs all` '
        '-lclangAPINotes -lclangEdit -lclangBasic -lclangTooling -lclangFrontendTool '
        '-lclangCodeGen -lclangARCMigrate -lclangRewrite -lclangRewriteFrontend '
        '-lclangASTMatchers -lclangSerialization -lclangSema -lclangStaticAnalyzerFrontend '
        '-lclangStaticAnalyzerCheckers -lclangStaticAnalyzerCore -lclangAnalysis '
        '-lclangDriver -lclangFrontend -lclangParse -lclangAST -lclangLex '
        '-lclangSupport -lclangIndex -lclang -Wl,--end-group'
    )

    logging.info("Building generate executable on Linux")
    try:
        subprocess.run(cmd_str, check=True, cwd=build_dir, shell=True, executable='/bin/bash')
    except subprocess.CalledProcessError as e:
        logging.error("Error building generate executable:")
        sys.exit(1)

def run_generate_exe(build_dir, settings):
    """Run the generate executable with appropriate arguments."""
    src_include = str(Path('..') / 'src')
    vendor_include = str(Path('..') / 'src' / 'core' / 'vendor')

    if settings.is_windows:
        extra_args = [
            '-std=c++20',
            '-w',
            '-Wall',
            '-Wextra',
            '-Wno-error',
            '-Wno-narrowing',
            *COMMON_DEFINES,
            f'-I{settings.settings["VulkanInc"]}',
            f'-I{src_include}',
            f'-I{vendor_include}'
        ]
        generate_exe = str(Path(build_dir) / 'generate.exe')
    elif settings.is_linux:
        extra_args = [
            '-resource-dir=/home/deck/llvm-build/lib/clang/20',  # Update as necessary
            '-march=native',
            '-std=c++20',
            '-w', 
            '-Wall', 
            '-Wextra', 
            '-Wno-error', 
            '-Wno-narrowing', 
            *COMMON_DEFINES,
            '-I/usr/include/vulkan',  # Assuming Vulkan is in /usr/include/vulkan
            f'-I{src_include}',
            f'-I{vendor_include}'
        ]
        generate_exe = str(Path(build_dir) / 'generate')
    else:
        logging.error("Unsupported platform")
        sys.exit(1)

    if not Path(generate_exe).exists():
        logging.error(f"Error: generate executable not found at {generate_exe}")
        sys.exit(1)

    main_cpp = str(Path('..') / 'src' / 'main.cpp')
    if not Path(main_cpp).exists():
        logging.error(f"Error: main.cpp not found at {main_cpp}")
        sys.exit(1)

    cmd = [generate_exe] + [f'--extra-arg-before={arg}' for arg in extra_args] + [main_cpp]

    logging.info("Running generate executable")
    logging.debug(' '.join(cmd))
    try:
        subprocess.run(cmd, check=True)
    except subprocess.CalledProcessError as e:
        logging.error(f"Error running generate executable: {e}")
        sys.exit(1)

def build_main_executable(build_dir, settings):
    """Build the main executable based on the platform."""
    if settings.is_windows:
        build_main_executable_windows(build_dir, settings)
    elif settings.is_linux:
        build_main_executable_linux(build_dir, settings)
    else:
        logging.error("Unsupported platform")
        sys.exit(1)

def build_main_executable_windows(build_dir, settings):
    """Build the main executable on Windows."""
    build_suffix = settings.build_suffix
    common_comp_flags = settings.common_comp_flags
    common_link_flags = settings.common_link_flags
    include_dirs = settings.include_dirs
    libraries = settings.libraries
    compiler_cmd = settings.compiler_cmd
    platform_cpp_files = settings.platform_cpp_files

    source_files = [
        str(Path('..') / 'src' / 'main.cpp'),
        *platform_cpp_files
    ]

    include_dirs_str = ' '.join([f'/I"{dir}"' for dir in include_dirs])
    source_files_str = ' '.join([f'"{file}"' for file in source_files])
    libraries_str = ' '.join(libraries)
    common_comp_flags_str = ' '.join(common_comp_flags)
    common_link_flags_str = ' '.join(common_link_flags)

    pdb_file = f'ce_{generate_random_hex(8)}.pdb'
    cmd = (
        f'{compiler_cmd} {common_comp_flags_str} {include_dirs_str} {source_files_str} '
        f'{" ".join(COMMON_DEFINES)} '
        f'/Fe"Cynosure Engine.exe" {libraries_str} /link {common_link_flags_str} '
        f'/LIBPATH:"{settings.settings["VulkanLib"]}" /PDB:{pdb_file}'
    )

    logging.info("Building main executable on Windows")
    try:
        subprocess.run(cmd, check=True, cwd=build_dir, shell=True)
    except subprocess.CalledProcessError as e:
        logging.error(f"Failed to build main executable: {e}")
        sys.exit(1)

def build_main_executable_linux(build_dir, settings):
    """Build the main executable on Linux."""
    common_comp_flags = settings.common_comp_flags
    common_link_flags = settings.common_link_flags
    include_dirs = settings.include_dirs
    libraries = settings.libraries
    compiler_cmd = settings.compiler_cmd
    platform_cpp_files = settings.platform_cpp_files

    source_files = [
        '../src/main.cpp',
        *platform_cpp_files
    ]

    libraries_with_l = [f'-l{lib}' for lib in libraries]

    cmd = [
        compiler_cmd,
        *common_comp_flags,
        *[f'-I{dir}' for dir in include_dirs],
        *source_files,
        '-Wl,-rpath=../libs/',
        '-L../libs/',
        '-o', 'Cynosure Engine',
        *libraries_with_l,
        *COMMON_DEFINES
    ]

    logging.info("Building main executable on Linux")
    try:
        subprocess.run(cmd, check=True, cwd=build_dir)
    except subprocess.CalledProcessError as e:
        logging.error(f"Failed to build main executable: {e}")
        sys.exit(1)

def process_tokens(base_name):
    """Process tokens from the base name to create an export name."""
    tokens = base_name.split('_')
    export_name = ''.join(token.capitalize() for token in tokens)
    return export_name

def format_elapsed_time(elapsed_seconds):
    """Format elapsed time into hours, minutes, and seconds."""
    hours, remainder = divmod(elapsed_seconds, 3600)
    minutes, seconds = divmod(remainder, 60)
    if hours:
        return f"{int(hours)}h {int(minutes)}m {seconds:.2f}s"
    elif minutes:
        return f"{int(minutes)}m {seconds:.2f}s"
    else:
        return f"{seconds:.2f}s"

if __name__ == '__main__':
    main()
