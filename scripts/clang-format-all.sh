#!/usr/bin/env bash
# Formats all in-repo C/C++ sources with clang-format (repo root .clang-format).
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

if ! command -v clang-format >/dev/null 2>&1; then
	echo "error: clang-format not found on PATH (need major version 18+)" >&2
	exit 1
fi

version_line="$(clang-format --version | head -n 1)"
if [[ ! "$version_line" =~ version[[:space:]]+([0-9]+) ]]; then
	echo "error: could not parse clang-format version from: $version_line" >&2
	exit 1
fi
if (( "${BASH_REMATCH[1]}" < 18 )); then
	echo "error: clang-format major version ${BASH_REMATCH[1]} < 18" >&2
	exit 1
fi

is_excluded() {
	local rel="$1"
	[[ "$rel" == cpp-lib/jsoncpp/* ]] && return 0
	[[ "$rel" =~ ^samples/ts-microservice/.+/src/stub/ ]] && return 0
	[[ "$rel" == compiler/core/asn_commentparser.cpp ]] && return 0
	[[ "$rel" == *"/node_modules/"* ]] && return 0
	return 1
}

count=0
while IFS= read -r -d '' file; do
	rel="${file#"${REPO_ROOT}/"}"
	if is_excluded "$rel"; then
		continue
	fi
	clang-format -i "$file"
	count=$((count + 1))
done < <(find "$REPO_ROOT" \( -name '*.cpp' -o -name '*.h' -o -name '*.hpp' -o -name '*.c' -o -name '*.cc' -o -name '*.cxx' \) -type f -print0)

echo "clang-format baseline complete (${count} files)."
