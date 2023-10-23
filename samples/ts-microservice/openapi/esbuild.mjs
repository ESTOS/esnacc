/* eslint-disable no-undef */
import * as esbuild from "esbuild";

let ctx = await esbuild.context({
    entryPoints: ["src/index.ts"],
    bundle: true,
    minify: false,
    sourcemap: true,
    jsx: "automatic",
    outfile: "example/esbuild/bundle.js",
});

await ctx.watch();

let { host, port } = await ctx.serve({
    port: 8800,
    servedir: "example",
});

console.log("Build serverd on ", "http://" + host + ":" + port);

