#!/usr/bin/env bash
# Ensures the esnacc compiler is current and prints/exports its CMake output path.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="${SNACCLIB7_ROOT:-$(cd "$SCRIPT_DIR/.." && pwd)}"
CONFIGURATION="${SNACC_CONFIGURATION:-Release}"
QUIET=0
WRITE_PATH=0
EXPORT_ENV=0

while [[ $# -gt 0 ]]; do
	case "$1" in
		--quiet) QUIET=1; shift ;;
		--write-path) WRITE_PATH=1; shift ;;
		--export) EXPORT_ENV=1; shift ;;
		*) echo "Unknown argument: $1" >&2; exit 2 ;;
	esac
done

log() {
	if [[ "$QUIET" -eq 0 ]]; then
		echo "$1"
	fi
}

read_cache_value() {
	local cache_file="$1"
	local key="$2"
	local line value
	line="$(grep -m1 "^${key}:" "$cache_file" || true)"
	if [[ -z "$line" ]]; then
		return 1
	fi
	value="${line#*=}"
	if [[ "$value" == UNINITIALIZED=* ]]; then
		echo "${value#UNINITIALIZED=}"
	else
		echo "$value"
	fi
}

find_cmake_build_dir() {
	local candidate
	if [[ -n "${SNACC_CMAKE_BUILD_DIR:-}" ]]; then
		echo "$REPO_ROOT/$SNACC_CMAKE_BUILD_DIR"
		return 0
	fi
	for candidate in build/x64_vc145 build/win32_vc145 build/release build/debug build; do
		if [[ -f "$REPO_ROOT/$candidate/CMakeCache.txt" ]]; then
			echo "$REPO_ROOT/$candidate"
			return 0
		fi
	done
	echo "$REPO_ROOT/build"
}

default_output_dir() {
	echo "$REPO_ROOT/output/bin"
}

compiler_from_cache() {
	local cache_file="$1"
	local output_dir output_name
	output_dir="$(read_cache_value "$cache_file" COMPILER_OUTPUT_PATH || default_output_dir)"
	output_name="$(read_cache_value "$cache_file" COMPILER_OUTPUT_NAME || echo esnacc)"
	echo "$output_dir/$output_name"
}

resolve_existing_compiler() {
	local candidate name dir
	for candidate in "${SNACC_COMPILER:-}" "${ESNACC_EXECUTABLE:-}" "${CMAKE_COMPILER_TARGET:-}"; do
		if [[ -n "$candidate" && -f "$candidate" ]]; then
			echo "$(cd "$(dirname "$candidate")" && pwd)/$(basename "$candidate")"
			return 0
		fi
	done
	for dir in "$(default_output_dir)" "$REPO_ROOT/samples/bin"; do
		for name in esnacc esnaccd esnacc7 esnacc7d; do
			if [[ -x "$dir/$name" ]]; then
				echo "$dir/$name"
				return 0
			fi
		done
	done
	for name in esnacc esnaccd esnacc7 esnacc7d; do
		if command -v "$name" >/dev/null 2>&1; then
			command -v "$name"
			return 0
		fi
	done
	return 1
}

ensure_cmake_configured() {
	local build_dir="$1"
	local cache_file="$build_dir/CMakeCache.txt"
	if [[ -f "$cache_file" ]]; then
		echo "$cache_file"
		return 0
	fi

	log "Configuring esnacc CMake build in $build_dir"
	mkdir -p "$build_dir"
	local output_dir
	output_dir="$(default_output_dir)"
	local -a cmake_args=(
		"-S" "$REPO_ROOT"
		"-B" "$build_dir"
		"-DMSVC_STATIC_RUNTIME=ON"
		"-DBUILD_TESTING=OFF"
		"-DCOMPILER_OUTPUT_PATH=$output_dir"
		"-DCOMPILER_OUTPUT_NAME=esnacc"
	)
	if [[ -n "${SNACC_CMAKE_GENERATOR:-}" ]]; then
		cmake_args=("-G" "$SNACC_CMAKE_GENERATOR" "${cmake_args[@]}")
	fi
	cmake "${cmake_args[@]}"
	echo "$cache_file"
}

if [[ "${SNACC_SKIP_COMPILER_BUILD:-}" == "1" ]]; then
	compiler="$(resolve_existing_compiler)"
elif [[ ( -n "${SNACC_COMPILER:-}" || -n "${CMAKE_COMPILER_TARGET:-}" ) && "${SNACC_FORCE_COMPILER_BUILD:-}" != "1" ]]; then
	compiler="$(resolve_existing_compiler)"
else
	build_dir="$(find_cmake_build_dir)"
	cache_file="$(ensure_cmake_configured "$build_dir")"
	log "Building esnacc compiler ($CONFIGURATION) via CMake"
	cmake --build "$build_dir" --config "$CONFIGURATION" --target compiler
	compiler="$(compiler_from_cache "$cache_file")"
	if [[ ! -f "$compiler" ]]; then
		echo "esnacc compiler not found at $compiler" >&2
		exit 1
	fi
fi

export SNACC_COMPILER="$compiler"
if [[ "$EXPORT_ENV" -eq 1 ]]; then
	printf 'export SNACC_COMPILER=%q\n' "$compiler"
	printf 'export PATH=%q:$PATH\n' "$(dirname "$compiler")"
elif [[ "$WRITE_PATH" -eq 1 ]]; then
	echo "$compiler"
else
	log "Using esnacc compiler: $compiler"
fi
