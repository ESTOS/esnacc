#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
GLUE_DIR="$REPO_ROOT/compiler/back-ends/ts-gen/gluecode"
TEST_DIR="$REPO_ROOT/compiler/back-ends/ts-gen/tests"
STUB_DIR="$REPO_ROOT/samples/ts-microservice/node-client/src/stub"
NODE_MODULES="$REPO_ROOT/samples/ts-microservice/node-client/node_modules"

if [[ ! -d "$NODE_MODULES/@estos/asn1ts" ]]; then
	echo "error: run samples/prepare.sh first to install node-client dependencies." >&2
	exit 1
fi

export NODE_PATH="$NODE_MODULES"
COPIED_FIXTURE=0
cleanup() {
	if [[ "$COPIED_FIXTURE" -eq 1 ]]; then
		rm -f "$GLUE_DIR/ENetUC_Common.ts" "$GLUE_DIR/ENetUC_Common_Converter.ts"
	fi
}
trap cleanup EXIT

for fixture in ENetUC_Common.ts ENetUC_Common_Converter.ts; do
	if [[ ! -f "$GLUE_DIR/$fixture" ]]; then
		cp "$STUB_DIR/$fixture" "$GLUE_DIR/$fixture"
		COPIED_FIXTURE=1
	fi
done

for test_file in \
	TSASN1Base.registry.test.ts \
	TSASN1Base.remoteCapability.test.ts \
	TSModuleCapabilities.test.ts
do
	echo "Running $test_file ..."
	npx --yes tsx "$TEST_DIR/$test_file"
done
