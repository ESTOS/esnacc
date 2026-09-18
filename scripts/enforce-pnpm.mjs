// Blocks npm/yarn install; this repository uses pnpm (see packageManager in package.json).
const userAgent = process.env.npm_config_user_agent ?? "";
if (userAgent.startsWith("pnpm/")) {
	process.exit(0);
}

const lines = [
	"",
	"This repository uses pnpm, not npm or yarn.",
	"",
	"  corepack enable",
	"  pnpm install",
	"",
];
console.error(lines.join("\n"));
process.exit(1);
