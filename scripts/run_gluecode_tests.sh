#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
TEST_DIR="$REPO_ROOT/typescript"
NODE_MODULES="$REPO_ROOT/samples/ts-microservice/node-client/node_modules"

if [[ ! -d "$NODE_MODULES/@estos/asn1ts" ]]; then
	echo "error: run scripts/setup-snacc-tests.sh first." >&2
	exit 1
fi

export NODE_PATH="$NODE_MODULES"

cleanup() {
	rm -rf "$TEST_DIR/stub"
}
trap cleanup EXIT

node "$REPO_ROOT/scripts/prepare-ts-glue-stub.mjs" --clean

for test_file in \
	TSASN1Base.registry.test.ts \
	TSASN1Base.invokeBlockPolicy.test.ts \
	TSASN1Base.roseSessionSubscription.test.ts \
	TSASN1Base.pauseRoseProcessing.test.ts \
	TSModuleCapabilities.test.ts \
	TSROSEBase.invokeTimeout.test.ts \
	TSInvokeContext.init.test.ts \
	TSCallFlow.loopback.test.ts \
	TSLogicalFailure.loopback.test.ts \
	TSTransportFailure.loopback.test.ts
do
	echo "Running $test_file ..."
	node "$REPO_ROOT/scripts/run-ts-glue-test-file.mjs" "$test_file"
done
