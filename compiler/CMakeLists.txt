# Build the compiler

# cmake debug message
# message("Current src: " ${CMAKE_CURRENT_SOURCE_DIR})

# ChatGPT says: - nice, but stask overflow has a better matching anser out of the box
## Get a list of all subdirectories in a directory
#file(GLOB subdirs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/path/to/subdir ${CMAKE_CURRENT_SOURCE_DIR}/path/to/subdir/*)
#
## Filter out files and keep only directories
#foreach(dir ${subdirs})
#    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/path/to/subdir/${dir})
#        list(APPEND dirs ${dir})
#    endif()
#endforeach()
#
## Store the list of directories in a variable for later use
#set(dir_list ${dirs} CACHE INTERNAL "List of directories")

# Stackoverflow says:
# A Macro is a sort of function which does not get an own environment; it changes the variables of the caller!
# but thats exactly what we need here
MACRO(SUBDIRLIST result curdir)
    FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
    SET(dirlist "")
    FOREACH(child ${children})
        IF(IS_DIRECTORY ${curdir}/${child})
            LIST(APPEND dirlist ${child})
        ENDIF()
    ENDFOREACH()
    SET(${result} ${dirlist})
ENDMACRO()

# add all compiler sources
if(WIN32)
    file(GLOB src_files core/*.c core/*.cpp back-ends/*.c back-ends/**/*.c compiler.rc)
else()
    file(GLOB src_files core/*.c core/*.cpp back-ends/*.c back-ends/**/*.c)
endif()
file(GLOB header_files core/*.h back-ends/*.h back-ends/**/*.h)

# Set the compiler (binary) output path
if(COMPILER_OUTPUT_PATH)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${COMPILER_OUTPUT_PATH})
else()
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/output/bin)
endif()
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# build executable and link it with c-static-lib and back-end
add_executable(compiler ${src_files} ${header_files})
target_link_libraries (compiler LINK_PUBLIC c-lib)

# Get include paths the compiler needs
# get all subdriectories of back-ends
SUBDIRLIST(subdirs ${CMAKE_CURRENT_SOURCE_DIR}/back-ends)

# put them in the compiler include path
foreach(subdi ${subdirs})
    target_include_directories(compiler PUBLIC ${subdi})
endforeach()


if(COMPILER_OUTPUT_NAME)
	set_target_properties(compiler PROPERTIES OUTPUT_NAME ${COMPILER_OUTPUT_NAME})
else()
	set_target_properties(compiler PROPERTIES OUTPUT_NAME esnacc)
endif()
if(COMPILER_OUTPUT_NAME_DEBUG)
	set_target_properties(compiler PROPERTIES OUTPUT_NAME_DEBUG ${COMPILER_OUTPUT_NAME_DEBUG})
else()
	set_target_properties(compiler PROPERTIES OUTPUT_NAME_DEBUG esnaccd)
endif()


# configure compiler specific options
# - warning levels
if(MSVC)
	# -wd4100 - do not complain about unreference parameters
	target_compile_options(compiler PRIVATE -W4 -wd4100)
	# Get rid of strange compiler warnings concerning interlocked functions the compiler is not usign anyway
	add_definitions(-DMICROSOFT_WINDOWS_WINBASE_H_DEFINE_INTERLOCKED_CPLUSPLUS_OVERLOADS=0)
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    target_compile_options(compiler PRIVATE -Wall)
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "AppleClang")
    target_compile_options(compiler PRIVATE -Wall)
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU") # GCC
    target_compile_options(compiler PRIVATE -Wall)
else()
	message(FATAL_ERROR "Unsupported compiler, check how to set the warning level for your compiler")
endif()
