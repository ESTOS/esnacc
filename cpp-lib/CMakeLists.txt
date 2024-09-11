# WARNING: not really tested yet and has compile errors!
# to improve things, maybe c-lib and compiler CMakeLists.txt can be examples

add_compile_definitions("SNACCDLL_NONE")

file(GLOB src_files src/*.cpp src/tkAppInit.c jsoncpp/*.cpp)
file(GLOB header_files **/*.h)

add_library(cpp-lib STATIC ${src_files} ${header_files})

target_include_directories(cpp-lib PUBLIC "include")
target_compile_definitions(cpp-lib PRIVATE SNACCDLL_NONE)
target_compile_definitions(cpp-lib PRIVATE SNACCLIB_BUILD)

# set the name for the library (the d addon for debug has been specified in the root CMakeLists.txt)
if(CPP_LIBRARY_OUTPUT_NAME)
	set_target_properties(cpp-lib PROPERTIES OUTPUT_NAME ${CPP_LIBRARY_OUTPUT_NAME})
else()
	set_target_properties(cpp-lib PROPERTIES OUTPUT_NAME "esnacc_cpp_lib")
endif()

if(CPP_LIBRARY_OUTPUT_PATH)
	set(OUTPUT_PATH ${CPP_LIBRARY_OUTPUT_PATH})
else()
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(OUTPUT_PATH ${CMAKE_SOURCE_DIR}/output/libx64)
	else()
		set(OUTPUT_PATH ${CMAKE_SOURCE_DIR}/output/lib)
	endif()
endif()
set_target_properties(cpp-lib PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${OUTPUT_PATH})
set_target_properties(cpp-lib PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${OUTPUT_PATH})
set_target_properties(cpp-lib PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${OUTPUT_PATH})

# configure compiler specific options
# - warning levels
if(MSVC)
	# -wd4100 - do not complain about unreference parameters
	target_compile_options(cpp-lib PRIVATE -W4 -wd4100)
	if(MSVC_STATIC_RUNTIME)
		target_compile_options(cpp-lib PRIVATE "/MT$<$<CONFIG:Debug>:d>")
	endif()
	target_compile_options(cpp-lib PRIVATE /Zi)
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
	# -Wno-self-assign -Wno-misleading-indentation -Wno-for-loop-analysis
	target_compile_options(cpp-lib PRIVATE -Wall)
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "AppleClang")
	# -Wno-self-assign -Wno-misleading-indentation -Wno-for-loop-analysis
	target_compile_options(cpp-lib PRIVATE -Wall)
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU") # GCC
	# -Wno-self-assign -Wno-misleading-indentation -Wno-for-loop-analysis
	target_compile_options(cpp-lib PRIVATE -Wall)
else()
	message(FATAL_ERROR "Unsupported compiler, check how to set the warning level for your compiler")
endif()

