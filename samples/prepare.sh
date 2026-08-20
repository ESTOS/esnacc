#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
eval "$("${SCRIPT_DIR}/../scripts/ensure_compiler.sh" --export --quiet)"
exec node "${SCRIPT_DIR}/prepare.js" "$@"
