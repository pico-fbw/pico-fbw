# This file is heavily inspired by Prusa's build.py/bootstrap.py scripts, thanks!
# https://github.com/prusa3d/Prusa-Firmware-Buddy/blob/master/utils/build.py

import os
import shutil
import stat
import sys
import tarfile
from pathlib import Path
from urllib.request import urlretrieve
import zipfile

def install_esp_idf():
    if os.sys.platform.lower() == "windows":
        # TODO: test on windows
        os.system("install.bat")
    else:
        os.chmod("install.sh", os.stat("install.sh").st_mode | stat.S_IEXEC)
        os.system("./install.sh")
    os.chdir("..")

def install_ninja():
    os.chmod("ninja", os.stat("ninja").st_mode | stat.S_IEXEC)

def install_pico_sdk():
    os.chdir("pico-sdk")
    os.system("git submodule update --init")
    os.chdir("..")

dependencies = {
    'arm-none-eabi-gcc': {
        'version': '10.3.1',
        'url': {
            'linux': 'https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2',
            'windows': 'https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-win32.zip',
            'darwin': 'https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-mac.tar.bz2',
        }
    },
    'cmake': {
        'version': '3.28.3',
        'url': {
            'linux': 'https://github.com/Kitware/CMake/releases/download/v3.28.3/cmake-3.28.3-linux-x86_64.tar.gz',
            'windows': 'https://github.com/Kitware/CMake/releases/download/v3.28.3/cmake-3.28.3-windows-x86_64.zip',
            'darwin': 'https://github.com/Kitware/CMake/releases/download/v3.28.3/cmake-3.28.3-macos-universal.tar.gz',
        },
    },
    'ESP-IDF': {
        'version': '5.2.1',
        'url': {
            'linux': 'https://github.com/espressif/esp-idf/releases/download/v5.2.1/esp-idf-v5.2.1.zip',
            'windows': 'https://github.com/espressif/esp-idf/releases/download/v5.2.1/esp-idf-v5.2.1.zip',
            'darwin': 'https://github.com/espressif/esp-idf/releases/download/v5.2.1/esp-idf-v5.2.1.zip',
        },
        'install': install_esp_idf,
    },
    'ninja': {
        'version': '1.11.1',
        'url': {
            'linux': 'https://github.com/ninja-build/ninja/releases/download/v1.11.1/ninja-linux.zip',
            'windows': 'https://github.com/ninja-build/ninja/releases/download/v1.11.1/ninja-win.zip',
            'darwin': 'https://github.com/ninja-build/ninja/releases/download/v1.11.1/ninja-mac.zip',
        },
        'install': install_ninja,
    },
    'pico-sdk': {
        'version': '1.5.1',
        'cmd': 'git clone https://github.com/raspberrypi/pico-sdk.git --branch 1.5.1 --depth 1',
        'install': install_pico_sdk,
    },
}

platforms = {
    1: "pico",
    2: "esp32"
}

def find_single_subdir(path: Path):
    members = list(path.iterdir())
    if path.is_dir() and len(members) > 1:
        return path
    elif path.is_dir() and len(members) == 1:
        return find_single_subdir(members[0]) if members[0].is_dir() else path
    else:
        raise RuntimeError
    
def download_progress(count, block_size, total_size):
    percent = count * block_size * 100 // total_size
    sys.stdout.write("\rDownload progress: %d%%" % percent)
    sys.stdout.flush()

def download_and_extract(url: str, directory: Path):
    extract_dir = directory.with_suffix('.temp')
    shutil.rmtree(directory, ignore_errors=True)
    shutil.rmtree(extract_dir, ignore_errors=True)

    print('Downloading ' + directory.name)
    # TODO: handle error (RemoteDisconnected)
    f, _ = urlretrieve(url, filename=None, reporthook=download_progress)
    print()
    print('Extracting ' + directory.name)
    if '.tar.bz2' in url or '.tar.gz' in url or '.tar.xz' in url:
        obj = tarfile.open(f)
    else:
        obj = zipfile.ZipFile(f, 'r')
    obj.extractall(path=str(extract_dir))

    subdir = find_single_subdir(extract_dir)
    shutil.move(str(subdir), str(directory))
    shutil.rmtree(extract_dir, ignore_errors=True)

def install_dependency(dep: str):
    dir = Path(f"{dep}-{dependencies[dep]['version']}")
    os.mkdir(dir)
    os.chdir(dir)
    if 'cmd' in dependencies[dep]:
        # A command needs to be run to install the dependency, create its directory and run the command
        os.system(dependencies[dep]['cmd'])
    elif 'url' in dependencies[dep]:
        # A URL is provided to download the dependency, download and extract it
        download_and_extract(dependencies[dep]['url'][os.sys.platform.lower()], Path(os.getcwd()))
    else:
        print(f"Dependency {dep} has no installation method")
        exit(1)
    # No clue why but the dir has to be cycled to get the correct path
    os.chdir("..")
    os.chdir(dir)

    if 'install' in dependencies[dep]:
        print(Path(os.getcwd()))
        # The dependency requires some post-installation steps, run them
        dependencies[dep]['install']()
    os.chdir("..")

# TODO: check version of installed dependencies and update if necessary
def find_dependency(program: str):
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
        for path in (rootDir / "build" / "deps").iterdir():
            if path.name.startswith(program):
                if program == "arm-none-eabi-gcc":
                    return path / "bin" # Return the entire bin directory to have access to all tools
                elif program == "cmake":
                    return path / "bin" / "cmake"
                elif program == "ninja":
                    return path / "ninja"
                return path
    except FileNotFoundError:
        pass
    # Couldn't find the dependency
    return None

rootDir = Path(__file__).resolve().parent
platform = None
# If the argument to the command is clean, remove the build directory
if len(os.sys.argv) > 1 and os.sys.argv[1] == "clean":
    shutil.rmtree("build", ignore_errors=True)
    exit(0)
# Otherwise, interpret the argument as the platform to build for
elif len(os.sys.argv) > 1:
    platform = os.sys.argv[1]
# If no argument is given...
if platform == None:
    # If the cmake cache is present, pull the platform from there
    if os.path.exists("build/CMakeCache.txt"):
        with open("build/CMakeCache.txt") as f:
            for line in f:
                if "FBW_PLATFORM" in line:
                    platform = line.split("=")[1].strip()
                    break
    # Otherwise, prompt the user to select a platform
    else:
        platform = int(input("Select platform (1: Pico, 2: ESP32): "))
        platform = platforms[platform]
print(f"Building for platform: {platform}")

# Find which dependencies we need based on the platform
if platform == "pico" or platform == "pico_w":
    needed = ["ninja", "cmake", "arm-none-eabi-gcc", "pico-sdk"]
elif platform == "esp32":
    needed = ["ninja", "cmake", "ESP-IDF"]
else:
    print("Invalid platform")
    exit(1)

print("Indexing dependencies...")
# Check what dependencies (if any) are installed and remove them from the list
for dep in needed.copy():
    if find_dependency(dep) is not None:
        print(f"Depencency {dep} found at {find_dependency(dep)}")
        needed.remove(dep)
if needed:
    # Create the deps directory if it doesn't exist
    if not os.path.exists(rootDir / "build"):
        os.mkdir("build")
    if not os.path.exists(rootDir / "build" / "deps"):
        os.chdir(rootDir / "build")
        os.mkdir("deps")
    os.chdir(rootDir / "build" / "deps")
    print("Need to install: " + ", ".join(needed))
    for dep in needed:
        if dep in dependencies:
            install_dependency(dep)
        else:
            print(f"Dependency {dep} not found in the list of dependencies")
            exit(1)
        # Sanity check to make sure we can find the dependency after installing it
        if not find_dependency(dep):
            print(f"Failed to install {dep}")
            exit(1)
os.chdir(rootDir)
print("All dependencies installed")

# Run the build
if platform == "pico" or platform == "pico_w":
    os.environ["PICO_SDK_PATH"] = str(find_dependency("pico-sdk"))
    os.environ["PATH"] += os.pathsep + str(find_dependency("arm-none-eabi-gcc"))
    cmake_command = ' '.join([str(find_dependency("cmake")),
                    "-B", "build", f"-DFBW_PLATFORM={platform}", "-DCMAKE_BUILD_TYPE=Release", 
                    f"-DCMAKE_MAKE_PROGRAM={str(find_dependency('ninja'))}", "-GNinja"])
elif platform == "esp32":
    os.environ["IDF_PATH"] = str(find_dependency("ESP-IDF"))
    cmake_command = ' '.join([f". $IDF_PATH/export.sh &&", str(find_dependency("cmake")),
                    "-B", "build", f"-DFBW_PLATFORM={platform}", "-DCMAKE_BUILD_TYPE=Release", 
                    f"-DCMAKE_MAKE_PROGRAM={str(find_dependency('ninja'))}", "-GNinja"])
cmake_command += " && " + " ".join([str(find_dependency("cmake")), "--build", "build", "--parallel"])
print("Running CMake command:", cmake_command)
os.system(cmake_command)