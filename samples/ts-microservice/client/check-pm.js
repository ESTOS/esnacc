const userAgent = process.env.npm_config_user_agent || "";
if (!userAgent.startsWith("pnpm"))
{
  console.error("\nERROR: You must use pnpm (preferably via Corepack).\n\n");
  process.exit(1);
}