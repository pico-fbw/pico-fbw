# This file is heavily inspired by Prusa's build.py/bootstrap.py scripts, thanks!
# https://github.com/prusa3d/Prusa-Firmware-Buddy/blob/master/utils/build.py

# Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
# Licensed under the GNU AGPL-3.0

import os
import random
import shutil
import ssl
import stat
import subprocess
import sys
import tarfile
from pathlib import Path
from urllib.request import urlretrieve
import zipfile

host = os.sys.platform.lower() # OS of host machine
root_dir = Path(__file__).resolve().parent # Root directory of the project
ssl._create_default_https_context = ssl._create_stdlib_context

# === Install functions for dependencies ===

def install_esp_idf():
    if host == "win32":
        subprocess.check_call(["install.bat"])
    else:
        os.chmod("install.sh", os.stat("install.sh").st_mode | stat.S_IEXEC)
        subprocess.check_call(["./install.sh"])
    os.chdir("..")

def install_ninja():
    ninja = "ninja"
    if host == "win32":
        ninja += ".exe"
    os.chmod(ninja, os.stat(ninja).st_mode | stat.S_IEXEC)

def install_node():
    os.environ["PATH"] += os.pathsep + (str(get_dependency_dir("node") / "bin") if host != "win32" else str(get_dependency_dir("node")))
    corepack = Path("bin/corepack")
    if host == "win32":
        corepack = Path("corepack.cmd")
    # Enable corepack to install yarn
    subprocess.check_call([str(corepack), "enable"])

def install_pico_sdk():
    os.chdir("pico-sdk")
    # Finish installing by cloning submodules
    subprocess.check_call(["git", "submodule", "update", "--init", "--progress"])
    os.chdir("..")
    # Move everything in pico-sdk subdirectory back to the root
    for item in Path("pico-sdk").iterdir():
        shutil.move(str(item), str(Path(os.getcwd())))

# Dependencies needed for the build process and details on how to install them
dependencies = {
    'arm-none-eabi-gcc': {
        'version': '10.3.1',
        'url': {
            'linux': 'https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2',
            'win32': 'https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-win32.zip',
            'darwin': 'https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-mac.tar.bz2',
        },
        'add_to_path': 'bin',
    },
    'cmake': {
        'version': '3.30.5',
        'url': {
            'linux': 'https://github.com/Kitware/CMake/releases/download/v3.30.5/cmake-3.30.5-linux-x86_64.tar.gz',
            'win32': 'https://github.com/Kitware/CMake/releases/download/v3.30.5/cmake-3.30.5-windows-x86_64.zip',
            'darwin': 'https://github.com/Kitware/CMake/releases/download/v3.30.5/cmake-3.30.5-macos-universal.tar.gz',
        },
    },
    'ESP-IDF': {
        'version': '5.3.1',
        'independent': True,
        'url': 'https://github.com/espressif/esp-idf/releases/download/v5.3.1/esp-idf-v5.3.1.zip',
        'install': install_esp_idf,
    },
    'ninja': {
        'version': '1.12.1',
        'url': {
            'linux': 'https://github.com/ninja-build/ninja/releases/download/v1.12.1/ninja-linux.zip',
            'win32': 'https://github.com/ninja-build/ninja/releases/download/v1.12.1/ninja-win.zip',
            'darwin': 'https://github.com/ninja-build/ninja/releases/download/v1.12.1/ninja-mac.zip',
        },
        'install': install_ninja,
        'add_to_path': '',
    },
    'node': {
        'version': '20.18.0',
        'url': {
            'linux': 'https://nodejs.org/dist/v20.18.0/node-v20.18.0-linux-x64.tar.xz',
            'win32': 'https://nodejs.org/dist/v20.18.0/node-v20.18.0-win-x64.zip',
            'darwin': 'https://nodejs.org/dist/v20.18.0/node-v20.18.0-darwin-x64.tar.gz',
        },
        'install': install_node,
        # node is actually added to PATH but has to be handled as an edge case (in run_prebuild_tasks)
    },
    'pico-sdk': {
        'version': '2.0.0',
        'cmd': 'git clone https://github.com/raspberrypi/pico-sdk.git --branch 2.0.0 --depth 1',
        'install': install_pico_sdk,
    },
}

# Platforms that can be built for
platforms = [
    "esp32",
    "host",
    "pico",
    "pico2",
    "pico_w",
]

# Dependencies needed for each platform
platform_dependencies = {
    "esp32": ["ninja", "cmake", "node", "ESP-IDF"],
    "host": ["ninja", "cmake"],
    "pico": ["ninja", "cmake", "node", "arm-none-eabi-gcc", "pico-sdk"],
    "pico2": ["ninja", "cmake", "node", "arm-none-eabi-gcc", "pico-sdk"],
    "pico_w": ["ninja", "cmake", "node", "arm-none-eabi-gcc", "pico-sdk"],
}

def find_innermost_subdir(path: Path) -> Path:
    """Find the innermost subdirectory of the specified path.
    :param path: the path to search
    :return: the innermost subdirectory
    """
    members = list(path.iterdir())
    if path.is_dir() and len(members) > 1:
        return path
    elif path.is_dir() and len(members) == 1:
        return find_innermost_subdir(members[0]) if members[0].is_dir() else path
    else:
        raise RuntimeError

def download_progress(count, block_size, total_size):
    """Hook for urlretrieve to display download progress.
    :param count: the number of blocks downloaded
    :param block_size: the size of each block
    :param total_size: the total size of the file
    """
    percent = min(count * block_size * 100 / total_size, 100)
    bar_length = 40
    filled_length = int(bar_length * percent // 100)
    bar = '=' * filled_length + '-' * (bar_length - filled_length)
    sys.stdout.write(f"\r|{bar}| {percent:.1f}%")
    sys.stdout.flush()

def download_and_extract(url: str, directory: Path):
    """Download and extract a file to the specified directory.
    :param url: the URL to download the file from
    :param directory: the directory to extract the file to
    """
    print(f"Downloading '{directory.name}'")
    f, _ = urlretrieve(url, filename=None, reporthook=download_progress)
    print()
    print(f"Extracting '{directory.name}'")
    extract_dir = directory / (directory.name + ".tmp")
    if any(x in url for x in [".tar.bz2", ".tar.gz", ".tar.xz"]):
        with tarfile.open(f) as obj:
            obj.extractall(path=str(extract_dir))
    else:
        with zipfile.ZipFile(f, 'r') as obj:
            obj.extractall(path=str(extract_dir))
    subdir = find_innermost_subdir(extract_dir)
    for item in Path(subdir).iterdir():
        shutil.move(str(item), str(directory))
    shutil.rmtree(extract_dir)

def get_dependency_dir(dep: str):
    """Get the directory for the specified dependency.
    :param dep: the name of the dependency
    :return: the path to the dependency directory
    """
    return (root_dir / "build" / "deps" / Path(f"{dep}-{dependencies[dep]['version']}")).absolute()

def install_dependency(dep: str):
    """Install the specified dependency.
    :param dep: the name of the dependency to install
    """
    print(f"-- Installing '{dep}'")
    dir = get_dependency_dir(dep)
    os.mkdir(dir)
    os.chdir(dir)
    if "cmd" in dependencies[dep]:
        # A command needs to be run to install the dependency, create its directory and run the command
        try:
            subprocess.check_call(dependencies[dep]['cmd'], shell=True)
        except subprocess.CalledProcessError:
            print(f"Failed to install '{dep}'. Is '{dependencies[dep]['cmd'].split()[0]}' installed?")
            exit(1)
    elif "url" in dependencies[dep]:
        # A URL is provided to download the dependency, download and extract it
        if dependencies[dep].get('independent', False):
            download_and_extract(dependencies[dep]['url'], Path(os.getcwd()))
        else:
            download_and_extract(dependencies[dep]['url'][host], Path(os.getcwd()))
    else:
        print(f"Dependency '{dep}' has no installation method")
        exit(1)
    # No clue why but the dir has to be cycled to get the correct path
    os.chdir("..")
    os.chdir(dir)

    if "install" in dependencies[dep]:
        print(f"Running post-installation steps for '{dep}'")
        # The dependency requires some post-installation steps, run them
        dependencies[dep]['install']()
    os.chdir("..")

# TODO: check version of installed dependencies and update if necessary
def find_dependency(program: str) -> Path | None:
    """Find the path to the specified dependency.
    :param program: the name of the dependency to find
    :return: the path to the dependency, or None if it couldn't be found
    """
    for path in os.environ['PATH'].split(os.pathsep):
        path = Path(path)
        if (path / program).exists():
            return path / program
    # Not found in PATH, handle some special cases
    if program == "pico-sdk" and os.environ.get("PICO_SDK_PATH") is not None:
        return Path(os.environ["PICO_SDK_PATH"])
    if program == "ESP-IDF" and os.environ.get("IDF_PATH") is not None:
        return Path(os.environ["IDF_PATH"])

    # Dependency isn't already installed, now check the deps directory
    try:
        for path in (root_dir / "build" / "deps").iterdir():
            if path.name.startswith(program):
                if program == "arm-none-eabi-gcc":
                    return path / "bin" # Return the entire bin directory to have access to all tools
                elif program == "cmake":
                    if host == "win32":
                        return path / "bin" / "cmake.exe"
                    return path / "bin" / "cmake"
                elif program == "ninja":
                    if host == "win32":
                        return path / "ninja.exe"
                    return path / "ninja"
                elif program == "node":
                    if host == "win32":
                        return path / "node.exe"
                    return path / "bin" / "node"
                return path
    except FileNotFoundError:
        pass
    # Couldn't find the dependency
    return None

def setup_host_tools():
    """Install any necessary build tools for the host platform."""
    if host == "linux":
        if not shutil.which("gcc"):
            print("-- Installing build tools")
            # Try to install build-essential (or equivalent) via the system's package manager
            if shutil.which("apt"):
                subprocess.check_call(["sudo", "apt", "install", "build-essential", "-y"])
            elif shutil.which("dnf"):
                subprocess.check_call(["sudo", "dnf", "groupinstall", "development-tools", "-y"])
            elif shutil.which("pacman"):
                subprocess.check_call(["sudo", "pacman", "-S", "base-devel", "--noconfirm"])
            elif shutil.which("zypper"):
                subprocess.check_call(["sudo", "zypper", "install", "-t", "pattern", "devel_basis"])
            else:
                print("Unable to install build tools, please install them manually.")
                exit(1)
    elif host == "win32":
        if not (Path(os.path.abspath(os.sep)) / "msys64").exists():
            # Install tools via msys2
            print("-- Setting up MSYS2")
            print("Downloading MSYS2")
            download = "msys2.exe"
            f, _ = urlretrieve("https://github.com/msys2/msys2-installer/releases/download/2024-07-27/msys2-x86_64-20240727.exe", filename=download, reporthook=download_progress)
            print()
            print("Installing MSYS2")
            subprocess.check_call(["msys2.exe", "in", "--confirm-command", "--accept-messages", "--root", "C:\\msys64"])
            os.remove(download)
            # Disable key refresh
            post_file = Path(os.path.abspath(os.sep)) / "msys64" / "etc" / "post-install" / "07-pacman-key.post"
            with open(str(post_file), "r", encoding="utf-8") as f:
                content = f.read()
            new_content = content.replace("--refresh-keys", "--version")
            with open(str(post_file), "w", encoding="utf-8") as f:
                f.write(new_content)
            print("Installing build tools")
            subprocess.check_call(["C:\\msys64\\usr\\bin\\pacman", "-S", "base-devel", "mingw-w64-ucrt-x86_64-toolchain", "--needed", "--noconfirm"])
        os.environ["PATH"] += os.pathsep + "C:\\msys64\\ucrt64\\bin" + os.pathsep + "C:\\msys64\\usr\\bin" # Ensure packages are in PATH during build
    elif host == "darwin":
        if not (Path(os.path.abspath(os.sep)) / "Library/Developer/CommandLineTools").exists():
            # Install tools via Xcode
            print("-- Setting up Xcode command line tools")
            subprocess.check_call(["xcode-select", "--install"])
            subprocess.check_call(["softwareupdate", "-i", "-a"])
            print("Please finish installing Xcode command line tools via System Preferences -> Software Update, and then run this script again.") # Just to be safe
            exit(1)

def run_prebuild_tasks(platform: str):
    """Run any necessary pre-build tasks for the specified platform.
    :param platform: the platform to build for
    """
    for dep in dependencies:
        # Add needed dependencies to PATH
        if "add_to_path" in dependencies[dep]:
            os.environ["PATH"] += os.pathsep + str(get_dependency_dir(dep) / Path(dependencies[dep]["add_to_path"]))
    # Edge case because node is weird (shocker)
    if find_dependency("node") is not None:
        os.environ["PATH"] += os.pathsep + (str(get_dependency_dir("node") / "bin") if host != "win32" else str(get_dependency_dir("node")))
        os.environ["COREPACK_ENABLE_DOWNLOAD_PROMPT"] = "0" # Also disable download prompt for corepack
    # Some platforms need to set environment variables
    if platform in ["pico", "pico2", "pico_w"]:
        os.environ["PICO_SDK_PATH"] = str(find_dependency("pico-sdk"))
    elif platform == "esp32":
        os.environ["IDF_PATH"] = str(find_dependency("ESP-IDF"))

def construct_cmake_command(platform: str) -> tuple[str, str]:
    """Construct the CMake configure and build commands based on the provided platform.
    :param platform: the platform to build for
    :return: a tuple containing the configure and build commands
    """
    source_dir = str(root_dir)
    build_dir = "build"
    # Base configure command 
    configure = str(find_dependency("cmake")) + " -S" + source_dir + " -B" + build_dir + f" -DFBW_PLATFORM={platform}"+ " -DCMAKE_BUILD_TYPE=Release"
    # If ninja is in PATH, use it as the build tool
    if find_dependency("ninja") is not None:
        configure += f" -DCMAKE_MAKE_PROGRAM={str(find_dependency('ninja'))}" + " -GNinja"
    # Platform-specific modifications
    if platform == "esp32":
        if host == "win32":
            configure = str(find_dependency("ESP-IDF") / "export.bat") + " && " + configure
        else:
            configure = ". $IDF_PATH/export.sh && " + configure
        
    # Base build command
    build = str(find_dependency("cmake")) + " --build " + build_dir + " --parallel"
    # More platform-specific modifications for build
    if platform == "esp32":
        if host == "win32":
            build = str(find_dependency("ESP-IDF") / "export.bat") + " && " + build
        else:
            build = ". $IDF_PATH/export.sh && " + build

    return configure, build

def get_platform() -> str:
    """:return: the platform to build for"""
    # If an argument is given, use that as the platform
    if len(os.sys.argv) > 1:
        return str(os.sys.argv[1])
    # If the cmake cache is present, pull the platform from there
    if os.path.exists("build/CMakeCache.txt"):
        with open("build/CMakeCache.txt") as f:
            for line in f:
                if "FBW_PLATFORM" in line:
                    print("Auto-detected platform from previous build")
                    print(f"To build for a different platform, clean the previous build ('{os.sys.argv[0]} clean') and run this script again.")
                    return line.split("=")[1].strip()
    # No platform could be found; prompt the user to select one
    else:
        for index, platform in enumerate(platforms):
            print(f"{index + 1}: {platform}")
        choice = int(input("Select platform: ")) - 1
        if choice < 0 or choice >= len(platforms):
            print("Invalid choice")
            exit(1)
        return platforms[choice]

def main():
    # Clean the build directory if requested
    if len(os.sys.argv) > 1 and os.sys.argv[1] == "clean":
        print("-- Cleaning build directory")
        for item in (root_dir / "build").iterdir():
            if item.is_dir() and item.name != "deps":
                shutil.rmtree(item)
        exit(0)
    # Get the platform to build for
    platform = get_platform()
    print(f"-- Building for platform '{platform}'")
    if platform not in platforms:
        print(f"Invalid platform '{platform}'")
        exit(1)

    # Find which dependencies we need based on the provided platform
    needed = platform_dependencies[platform].copy()
    print("-- Indexing dependencies...")
    # Check what dependencies (if any) are installed and remove them from the list
    for dep in needed.copy():
        if find_dependency(dep) is not None:
            print(f"Depencency '{dep}' found at '{find_dependency(dep)}'")
            needed.remove(dep)
    if needed:
        # Create the deps directory if it doesn't exist
        if not os.path.exists(root_dir / "build"):
            os.mkdir("build")
        if not os.path.exists(root_dir / "build" / "deps"):
            os.chdir(root_dir / "build")
            os.mkdir("deps")
        os.chdir(root_dir / "build" / "deps")
        print("Need to install:", ", ".join(needed))
        for dep in needed:
            if dep in dependencies:
                install_dependency(dep)
            else:
                print(f"Dependency '{dep}' not found in the list of dependencies")
                exit(1)
            # Sanity check to make sure we can find the dependency after installing it
            if not find_dependency(dep):
                print(f"Failed to install '{dep}'")
                exit(1)
    os.chdir(root_dir)
    # Check for host build tools; cmake likes to use host compilers for a lot of things
    setup_host_tools()
    print("-- All dependencies installed")

    # Run pre-build tasks and then build the project
    print("-- Running pre-build tasks")
    run_prebuild_tasks(platform)
    configure_cmd, build_cmd = construct_cmake_command(platform)
    print("-- Starting build")
    subprocess.check_call(configure_cmd, shell=True)
    subprocess.check_call(build_cmd, shell=True)
    print("\n-- Build complete!", random.choice(["🎉", "🚀", "👍", "🥳", "🎈"]))

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n-- Build interrupted by user")
        exit(1)
    except Exception as e:
        print(f"\n\n-- Build failed!", random.choice(["😢", "😭", "😞", "😟", "😦"]))
        raise e
