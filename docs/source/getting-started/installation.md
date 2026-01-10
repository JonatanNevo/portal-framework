# Installation

## Platform Support

| Platform                         | Compiler               |
|----------------------------------|------------------------|
| Windows                          | MSVC 2022+ / Clang 20+ |
| Linux (Only Ubuntu24+ is tested) | GCC 14+ / Clang 20+    |
| macOS                            | Clang 20+              |

## Prerequisites

Before installing `portal-tool` and the engine, make sure you have the following installed:

- Python 3.12 or later (for portal tool)
- CMake 3.30 or later
- Ninja build system
- Git
- Vulkan SDK version 1.4+
- C++ 23+ compiler

### Prerequisites Installation Steps
::::{tab-set}
:sync-group: platform

:::{tab-item} Windows
:sync: windows

#### Python
You can download python from [here](https://www.python.org/downloads/).  

#### Git
You can download Git from [here](https://git-scm.com/downloads).
Or using winget:
```bash
winget install Git.Git
```

#### CMake
You can download CMake from [here](https://cmake.org/download/).  
Or using winget:
```bash
winget install cmake
```

#### Ninja
You can download Ninja from [here](https://github.com/ninja-build/ninja/releases).
Or using winget:
```bash
winget install Ninja-build.Ninja
```

#### Vulkan SDK
You can download the Vulkan SDK from [here](https://vulkan.lunarg.com/sdk/home).

#### C++ Compiler
Windows has two available, well supported c++ compilers, either MSVC or Clang.  

##### MSVC
You can download MSVC from [here](https://visualstudio.microsoft.com/downloads/).  

##### Clang
```{note}
Make sure you install clang version 20+
```
You can download Clang from [here](https://github.com/llvm/llvm-project/releases).  
Or using winget:
```bash
winget install llvm
```

:::

:::{tab-item} Linux
:sync: linux


### Debian/Ubuntu

#### Using Apt

```console
sudo apt update
sudo apt install git cmake ninja-build python3
```

#### C++ Compiler
If you are using Ubuntu 25+, both GCC 14+ and Clang 20+ are available in apt.  
If not:

##### Clang 20+
Install clang from installer script:
```console
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh <version 20/21> all
```

Then update alternatives:
```
sudo update-alternatives --install /usr/bin/clang clang /usr/bin/clang-<version> 100
sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-<version> 100
sudo update-alternatives --set clang /usr/bin/clang-<version>
sudo update-alternatives --set clang++ /usr/bin/clang++-<version>
```

##### GCC 14+
Install GCC 14 from PPA:
```console
sudo add-apt-repository ppa:ubuntu-toolchain-r/test -y
sudo apt update
sudo apt install gcc-<version> g++<version> -y
```

Then update alternatives:
```console
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-<version> 100 
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-<version> 100
sudo update-alternatives --set gcc /usr/bin/gcc-<version>
sudo update-alternatives --set g++ /usr/bin/g++-<version>
```

#### Vulkan SDK
You can download the Vulkan SDK from [here](https://vulkan.lunarg.com/sdk/home).

### Other Linux Distros
Portal Engine is not yet tested on other linux distros.
:::

:::{tab-item} macOS
:sync: macos

### Brew
Make sure you have brew installed, if not you can find an installation [here](https://brew.sh/).

### Using Brew
```console
brew install git cmake ninja python llvm
```

```{note}
If you have xcode installed, you don't need to install llvm separately.
```

#### Vulkan SDK
You can download the Vulkan SDK from [here](https://vulkan.lunarg.com/sdk/home).

:::
::::

## Using Portal Tool (Recommended)
Portal Tool is a companion tool for building and managing Portal Engine projects.  
You can install it using pip:

```console
pip install portal-tool
```

````{tip}
It is recommended to install shell completions for portal-tool.

```console
portal-tool --install-completion
```
````

### Installing Required System Packages
Some platforms require additional system packages to be installed before building Portal Engine.

`portal-tool` can automatically install these packages and make sure you have the correct compilers installed.
```console
portal-tool install
```

## Manual Installation

If you don't want to install `portal-tool` you can install the required packages manually.
```{warning}
I do not have an up to date list of all required packages per platform.  
In order to find it, you can try to build the engine, and follow the instruction when vcpkg fails.
```