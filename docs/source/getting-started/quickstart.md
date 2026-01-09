# Quickstart

## Setting up an example project

If you don't have `portal-tool` insalled, please refer to [Installation](installation.md)  

`portal-tool` has an interactive wizard to help you setup your project.  
Simply run:
```console
portal-tool init
```

## Init Wizard Questions

Calling `portal-tool` will ask you a few questions to help you setup your project.

| Question            | Description                                                                                     | Default                            | Example       |
|---------------------|-------------------------------------------------------------------------------------------------|------------------------------------|---------------|
| Project Name        | The name of the project                                                                         | No Default                         | `My Project`  |
| Base Location       | The path to create that project at                                                              | `CWD` / Project name as snake case | `my-project`  |
| Use Example?        | Whether or not to use and example to configure the project                                      | Y/N question                       | y             |
| Example to use      | If answered yes, shows a list of examples to use                                                | `engine_test`                      | `engine_test` |
| Use External VCPKG? | If installed, will ask if you want to use an <br/>externally installed vcpkg to add a submodule | Y/N question                       | n             |
| Choose Compiler     | Will show you a list of installed valid compilers and ask you which one you want to use         | `clang` if exists                  | `clang`       |


## Project Structure

After `init` is finished, you will have a project structure similar to this:
```text
project-name
├───cmake/
│   └───overlay-triplets/     # Triplets overload for build system
├───resources/                # Resources for this project
├───source/                   # Source code for this project
├───CMakeLists.txt            # CMakeLists.txt with basic setup
├───CMakePresets.json         # Common presets
├───settings.json             # Settings for portal framework
├───vcpkg.json                # vcpkg manifest
└───vcpkg-configurations.json # vcpkg registry configurations
```

## Building And Running

The easiest way to build the project is using the presets.

### Configuration

The default cmake generator is `Ninja Multi-Config`. to run it, use:
```console
cmake --preset ninja-multi
```

This will create a `build` folder with the configurations for all build presets

### Building

The default build presets are `debug`, `development` and `dist`.
* debug - debug build will build the project in debug with asserts and debugging symbols enabled
* development - development build will build the project in release with asserts and debugging symbols enabled
* dist - dist build will build the project in release with asserts disabled and debugging symbols disabled

To build a specific preset:
```console
cmake --build build --preset <debug/development/dist>
```

### Packaging

#### Installer Package
The installer requires the QT Installer Framework, you can download it [here](https://doc.qt.io/qtinstallerframework/index.html)

You need to configure the installer using `portal_game_configure_installer` cmake command.
Then run:
```
cmake --workflow package-installer
```

This will create an installer package in the `dist` folder.

```{note}
Make sure to add the Qt Installer Framework to your PATH!
```

#### Zip Package
There are no additional steps required to create a zip package, 
Simply run:

```
cmake --workflow package-zip
```

This will create a zip with the binaries in the `dist` folder.