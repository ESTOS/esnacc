# Shared GoogleTest setup for snacclib7 C++ tests.
include(FetchContent)

# Keep test TU CRT settings aligned with snacc_cpp_lib (/MT when MSVC_STATIC_RUNTIME is ON).
if(MSVC AND MSVC_STATIC_RUNTIME)
	add_compile_options("/MT$<$<CONFIG:Debug>:d>")
endif()

find_package(GTest CONFIG QUIET)

if(NOT GTest_FOUND AND NOT TARGET gtest)
	if(MSVC AND MSVC_STATIC_RUNTIME)
		set(gtest_force_shared_crt OFF CACHE BOOL "" FORCE)
	else()
		set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
	endif()
	FetchContent_Declare(
		googletest
		URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
	)
	FetchContent_MakeAvailable(googletest)
endif()
