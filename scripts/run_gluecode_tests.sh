#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
NODE_MODULES="$REPO_ROOT/samples/ts-microservice/node-client/node_modules"

if [[ ! -d "$NODE_MODULES/@estos/asn1ts" ]]; then
	echo "error: run scripts/setup-snacc-tests.sh first." >&2
	exit 1
fi

exec node "$REPO_ROOT/scripts/run-ts-glue-test-suite.mjs"
