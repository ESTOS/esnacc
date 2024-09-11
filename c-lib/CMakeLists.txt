# build the static c-lib
# lib name: esnacc_clib

# gather all c files
file(GLOB src_files src/*.c)
file(GLOB header_files include/*.h)
# build all files with a C++ compiler
SET_SOURCE_FILES_PROPERTIES(${src_files} PROPERTIES LANGUAGE C)
# set the build target for the files
add_library(c-lib STATIC ${src_files} ${header_files})
# set the name for the library (the d addon for debug has been specified in the root CMakeLists.txt)
if(C_LIBRARY_OUTPUT_NAME)
	set_target_properties(c-lib PROPERTIES OUTPUT_NAME ${C_LIBRARY_OUTPUT_NAME})
else()
	set_target_properties(c-lib PROPERTIES OUTPUT_NAME "esnacc_c_lib")
endif()

if(C_LIBRARY_OUTPUT_PATH)
	set(OUTPUT_PATH ${C_LIBRARY_OUTPUT_PATH})
else()
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(OUTPUT_PATH ${CMAKE_SOURCE_DIR}/output/libx64)
	else()
		set(OUTPUT_PATH ${CMAKE_SOURCE_DIR}/output/lib)
	endif()
endif()
set_target_properties(c-lib PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${OUTPUT_PATH})
set_target_properties(c-lib PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${OUTPUT_PATH})

# configure compiler specific options
# - warning levels
if(MSVC)
	# -wd4100 - do not complain about unreference parameters
	target_compile_options(c-lib PRIVATE -W4 -wd4100)
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
	target_compile_options(c-lib PRIVATE -Wall)
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "AppleClang")
	target_compile_options(c-lib PRIVATE -Wall)
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU") # GCC
	target_compile_options(c-lib PRIVATE -Wall)
else()
	message(FATAL_ERROR "Unsupported compiler, check how to set the warning level for your compiler")
endif()
