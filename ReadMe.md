# estos Enhanced Sample Neufeld ASN C Compiler
This is the estos enhanced sample neufeld asn c compiler. 
It is an functional enriched fork of the original enhanced sample neufeld asn c compiler offering:
* Documented source code, based on the documentation from the asn1 source files
* Creating documentation based on the documentation from the asn1 source files
* Additional supported target languages
  * Typescript (main maintained language)
    * Structure definitions
    * JSON and BER encoder/decoders
    * ROSE client implementation (complete stubs) for node and the browser side
    * ROSE server implementation (complete stubs)
  * C++ (main maintained language)
    * Structure definitions
    * JSON and BER encoder/decoders
    * ROSE client/server implementation (complete stubs)
  * Delphi
    * Structure definitions
  * JAVA
    * Structure definitions
  * JavaScript JSON
    * Structure definitions
  * SWIFT
    * Structure definitions
  * JSDOC
    * JSON formatted Documentation generated based on the documentation inside the asn1 files
  * OpenApi
    * OpenApi JSON Documentation generated based on the documentation inside the asn1 files
    * Should be used with our swagger-ui project https://github.com/ESTOS/esnacc-openapi-sdk

# Functionality
Based on asn1 files the compiler transcodes the information into matching representations in different languages. Different target languages have different feature sets implemented. The currently two main languages are typescript and c++ which offer the most features.
Those two languages offer JSON and BER encoding for the transport layer, a complete ROSE (Remote Operations Service Element) implementation for both sides, the server and the client side.
Other languages just offer to get created structures which need to get serialized / deserialized in JSON with other functions of the language.

# Getting started
You need to compile the compiler or use a precompiled version of it.
The samples folder offers examples on the usage for the different supported languages. These folders show some sample asn1 files, the command line and the expected output of the compiler.

# Building the compiler
## Prerequesites to compile the compiler
* Have cmake installed 
  * https://cmake.org
  * at least V3.21
  * available in the path
  
## Command line
* From the root path of the repo, create a directory where all intermemdiate and build files will be written to
  ```shell
  mkdir build
  ```
* Navigate into this directory and generate the needed build files with cmake
  ```shell
  cd build
  cmake ..
    ```
* Build the compiler and libraries  
  ```shell
  cmake --build .
  ```

You may use the following cmake commandline variables to parameterize the output:

* __CPP_LIBRARY_OUTPUT_PATH__
  * the output path where debug and release builds of the cpp library will be written to
  * by default the libraries are written to /output/libx64 for x64 and /output/lib for x86

* __CPP_LIBRARY_OUTPUT_NAME__
  - the name of the library (e.g. if you want to specify a version like snacclib_1_2)
  - by default the name is "esnacc_cpp_lib"
  - a "d" is always added for debug builds

* __COMPILER_OUTPUT_PATH__
  - the output path where debug and release builds of the compiler will be written to
  - by default the compiler is written to /output/bin

* __COMPILER_OUTPUT_NAME__
  - the name of the compiler (e.g. if you want to specify a version like esnacc_1_2)
  - by default the name is "esnacc"
  - the .exe for windows is automatically added

* __COMPILER_OUTPUT_NAME_DEBUG__
  - the name of the compiler (e.g. if you want to specify a version like esnacc_1_2d)
  - by default the name is "esnaccd"
  - the .exe for windows is automatically added

## Visual Studio 2022
1. Open Visual Studio
2. Instead of "Open Project..." use "Open local folder" for Project-Selection
3. Choose the "code" folder of the Repository - The folder that contains the root CMakeLists.txt
4. Visual Studio will recognize the CMakeFiles.txt and starts configuring using cmake.
   This will generate a subdirectory 'out', the equivalent of "build" from above
5. Select "esnacc.exe" as the run configuration to debug the binary
6. Hit F7 or CTRL-SHIFT-B to compile the compiler

## CLion
1. Open CLion with the code directory as the project directory.
2. Make sure, that the correct toolchain is set in the settings (Visual Studio or build-in MingW).
3. Build
4. For debugging, use run configuration "compiler"

## Visual Studio Code
* Have ninja installed
  * https://github.com/ninja-build/ninja/releases
  * at least v1.11.1
  * available in the path

1. Open Visual Studio Code using the esnacc.code-workspace file in the code folder
2. If not already done, install the suggested extension
  * The extensions are advertised with the extensions.json in the .vscode sub folder
  * C/C++
  * C/C++ Extension Pack (brings along the C/C++ Themes)
  * Cmake Tools
  * CodeLLDB
  * Clang-Format
3. Visual Studio Code will now ask you to specify the top cmake config file which is located in the code folder CMakeLists.txt
  * In case visual studio is not asking you use <Ctrl>+<Shift>+<P> **Cmake: Configure** -> ${workspaceFolder}././CmakeLists.txt
4. In the bottom bar of the IDE from the left to the right you will find the settings how and what to compile / debug
  * Check the tooltip on what option the different entry is supposed to configure as the text will be environment specific different
  1. Element - Info about errors and warnings
  2. Element - **Click to select the build variant** -> **Cmake: [Debug]: Ready**
  3. Element - **Click to change the active kit** -> **Clang xx.x.x** not clang-clang-cl
  4. Element - **Build selected target** -> builds the option on the right
  5. Element - **Select the default build target** -> **[esnacc]**
  * Click the build button to build the compiler

## macOS
1. Make sure all development tools are installed (XCode + commandline tools)
2. Install cmake (e.g. with homebrew)
3. Now proceed like it is described in "Command line"
   ```shell
   mkdir build
   cd build
   cmake ..
   cmake --build .
   ```

The Compiler will be generated in ```output/bin```

