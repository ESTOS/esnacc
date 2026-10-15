// Shared rewrite of Node's junit reporter so GitHub lists every test().
// Node puts <testcase> directly under <testsuites>. dorny/test-reporter only
// lists cases that sit inside <testsuite>. Both TypeScript runners call this
// after node --test so the check and the editor stay on the same cases.
import { dirname } from "node:path";

/**
 * Turns a Node junit file attribute into a path relative to the repo.
 * GitHub annotations match that path against the checked-out sources.
 * @param {string} file
 * @param {string} root
 * @returns {string}
 */
function repoRelativeTestFile(file, root) {
	const normalized = file.replace(/\\/g, "/");
	const rootPosix = root.replace(/\\/g, "/").replace(/\/$/, "");
	if (normalized.toLowerCase().startsWith(rootPosix.toLowerCase())) {
		return normalized.slice(rootPosix.length).replace(/^\//, "");
	}
	for (const marker of ["/typescript/", "/samples/"]) {
		const at = normalized.toLowerCase().indexOf(marker);
		if (at >= 0) {
			return normalized.slice(at + 1);
		}
	}
	return normalized;
}

/**
 * Escapes a value placed in a double-quoted XML attribute.
 * @param {string} value
 * @returns {string}
 */
function escapeXmlAttribute(value) {
	return value.replace(/&/g, "&amp;").replace(/"/g, "&quot;").replace(/</g, "&lt;");
}

/**
 * Wraps Node's junit output in one testsuite per source file.
 * Called by the glue suite and the microservice runner after node --test.
 * @param {string} xml
 * @param {string} root
 * @returns {string}
 */
export function wrapNodeJUnitForGitHub(xml, root) {
	const cases = xml.match(/<testcase\b[^>]*\/>|<testcase\b[^>]*>[\s\S]*?<\/testcase>/g) ?? [];
	if (cases.length === 0) {
		return xml;
	}
	/** @type {Map<string, string[]>} */
	const groups = new Map();
	for (const block of cases) {
		const fileMatch = /file="([^"]*)"/.exec(block);
		const relative = repoRelativeTestFile(fileMatch ? fileMatch[1] : dirname(root), root);
		const escaped = escapeXmlAttribute(relative);
		const rewritten = block
			.replace(/file="[^"]*"/, `file="${escaped}"`)
			.replace(/classname="[^"]*"/, `classname="${escaped}"`);
		const list = groups.get(relative) ?? [];
		list.push(rewritten);
		groups.set(relative, list);
	}
	const suites = [];
	for (const [name, blocks] of groups) {
		const failures = blocks.filter((block) => block.includes("<failure")).length;
		const time = blocks.reduce((sum, block) => {
			const match = /time="([^"]*)"/.exec(block);
			return sum + (match ? Number(match[1]) : 0);
		}, 0);
		suites.push(
			`<testsuite name="${escapeXmlAttribute(name)}" tests="${blocks.length}" failures="${failures}" time="${time.toFixed(6)}">\n${blocks.join("\n")}\n</testsuite>`,
		);
	}
	return `<?xml version="1.0" encoding="utf-8"?>\n<testsuites tests="${cases.length}">\n${suites.join("\n")}\n</testsuites>\n`;
}
