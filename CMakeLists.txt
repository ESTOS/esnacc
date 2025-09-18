# Sets the Environmen for the compiler and the c-lib (possible c++-lib too)
cmake_minimum_required(VERSION 3.20...4.1)
project(esnacc_cmake) #  LANGUAGES CXX) - would interpret all files as C++

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU") # GCC
	# Ugly but i see no other option than specifying the compiler directly
	# No matter what i do to tell cmake to use a newer standard than C89 itś
    # always invoking the g89-gcc compiler which complains about -std=c99 -std=c11 etc as not supported
    set(CMAKE_C_STANDARD 99)
	set(CMAKE_C_COMPILER "/usr/bin/c99-gcc")
else()
    set(CMAKE_C_STANDARD 11)
endif()
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_DEBUG_POSTFIX "d")
# set(CMAKE_VERBOSE_MAKEFILE ON)

if(MSVC)
    # Ensures that the __cplusplus macro is set to the proper number correspoding to the cpp version beeing used (otherwise it points to CPP98)
    add_compile_options(/Zc:__cplusplus)
    if(MSVC_STATIC_RUNTIME)
    	message("configuring a static cpp library")
    else()
    	message("configuring a dynamic cpp library")
    endif()
endif()

# set(CMAKE_VERBOSE_MAKEFILE ON)

# define the include directories needed for all projects
include_directories(.)

# DEFINES to set
add_compile_definitions(GNU_SOURCE)
add_compile_definitions(_XOPEN_SOURCE=700)
# for the compiler we should define these (see VS2022 Project):
add_compile_definitions(COMPILER)
# maybe these too? did compile without them yet
add_compile_definitions(_CONSOLE)
add_compile_definitions(USE_GEN_BUF)

# Define if you have the ANSI C header files.
add_compile_definitions(STDC_HEADERS=1)
# Define if lex declares yytext as a char * by default, not a char[].
add_compile_definitions(YYTEXT_POINTER=1)
# use ANSI or K&R style C?
add_compile_definitions(__USE_ANSI_C__=1)
# does the C++ compiler have the bool type built-in?
add_compile_definitions(BOOL_BUILTIN=1)
# The number of bytes in a double.
add_compile_definitions(SIZEOF_DOUBLE=8)
# The number of bytes in a int.
add_compile_definitions(SIZEOF_INT=4)
# The number of bytes in a long.
add_compile_definitions(SIZEOF_LONG=4)
# The number of bytes in a short.
add_compile_definitions(SIZEOF_SHORT=2)
# Define if you have the <string.h> header file.
add_compile_definitions(HAVE_STRING_H=1)
#
add_compile_definitions(IDL=1)

# libs
add_subdirectory(c-lib)
add_subdirectory(cpp-lib)

# compiler itself
add_subdirectory(compiler)

