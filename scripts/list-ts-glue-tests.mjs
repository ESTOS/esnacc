// Lists top-level node:test cases in typescript/*.test.ts for CTest registration.
// CMake runs this at configure time when SNACC_CTEST_REGISTER_TS=ON. Each stdout line is
// "<file>\t<exact test name>". Names must be string literals on the test() line so the
// VS Code Test Explorer and CI register the same cases. Dynamic test() calls fail the script.
import { readdirSync, readFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const testDir = join(dirname(fileURLToPath(import.meta.url)), "..", "typescript");

// test("name", ...) or test('name', ...) with the literal on the same line.
const literalPattern = /^\s*test\(\s*(?:"((?:\\.|[^"\\])*)"|'((?:\\.|[^'\\])*)')\s*,/;
// test(expression) is invisible to the static list, so CI would drop it.
const dynamicPattern = /^\s*test\(\s*(?!["'])/;
const describePattern = /^\s*describe\s*\(/;

/**
 * Reads typescript/*.test.ts and returns each top-level test() case.
 * Called by this script's main when CMake configures the CI test list.
 * Throws when a file uses describe() or a non-literal test() name.
 * @param {string} directory
 * @returns {{ file: string, name: string }[]}
 */
function listGlueTests(directory) {
	const files = readdirSync(directory).filter((name) => name.endsWith(".test.ts")).sort();
	const cases = [];
	for (const file of files) {
		const lines = readFileSync(join(directory, file), "utf8").split(/\r?\n/);
		for (let index = 0; index < lines.length; index++) {
			const line = lines[index];
			const where = `${file}:${index + 1}`;
			if (describePattern.test(line)) {
				throw new Error(`${where}: describe() is not supported; CTest matches the top-level test() name`);
			}
			if (dynamicPattern.test(line)) {
				throw new Error(`${where}: test() must use a string literal so CTest can register it`);
			}
			const match = literalPattern.exec(line);
			if (!match) {
				continue;
			}
			const name = (match[1] ?? match[2]).replace(/\\(["'\\])/g, "$1");
			if (/[\t\r\n;]/.test(name)) {
				throw new Error(`${where}: test name cannot contain a tab, newline, or semicolon`);
			}
			cases.push({ file, name });
		}
	}
	return cases;
}

/**
 * Prints the glue test list for CMake. One case per line, file and name separated by a tab.
 * Exits 1 when discovery finds nothing or two cases in one file share a name.
 */
function main() {
	let cases;
	try {
		cases = listGlueTests(testDir);
	} catch (error) {
		console.error(error instanceof Error ? error.message : error);
		process.exit(1);
	}
	if (cases.length === 0) {
		console.error("no TypeScript glue tests found");
		process.exit(1);
	}
	const seen = new Set();
	for (const entry of cases) {
		const key = `${entry.file}\0${entry.name}`;
		if (seen.has(key)) {
			console.error(`duplicate test ${entry.file}: ${entry.name}`);
			process.exit(1);
		}
		seen.add(key);
		process.stdout.write(`${entry.file}\t${entry.name}\n`);
	}
}

main();
