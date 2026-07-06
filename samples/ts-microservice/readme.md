---
title: TypeScript Microservice Sample
scope: samples/ts-microservice
owner_repo: esnacc
entry_for:
  - TypeScript generated stubs
  - Node server and client integration tests
  - browser demo and OpenAPI sample
purpose: Explain the TypeScript microservice sample layout, preparation steps, and verification flow.
read_when:
  - Updating TypeScript code generation samples
  - Running node-server, node-client, browser-client, or OpenAPI examples
related_docs:
  - ../../AGENTS.md
  - ../readme.md
  - node-server/README.md
---

# Typescript based microservice sample
This sample shows how generated TypeScript ASN.1 stubs are used end-to-end: a Node server, a browser demo, and automated integration tests.

Clients and server communicate through REST-like stateless HTTP POST calls or a WebSocket connection that supports server-to-client events.

# interface
The `samples/interface/` folder contains the ASN.1 description shared by all three packages (`ENetUC_Common`, `ENetUC_Settings_Manager`, `ENetUC_Event_Manager`).

Prepare the TypeScript sample packages from the `samples` folder (after building `esnacc`):

1. Build `esnacc` (CMake → `output/bin/`)
2. Run:
   - **Windows:** `prepare.bat`
   - **Linux:** `prepare.sh`

This generates stubs (TS + OpenAPI) and runs `pnpm install` for the `ts-microservice` workspace (`browser-client`, `node-server`, `node-client`). Requires **pnpm 11+** and **Node 24+**. Lockfile: `ts-microservice/pnpm-lock.yaml`.

CMake/CI uses the same `prepare.js` (with `--frozen-lockfile`) via the `snacc-ts-prepare` target before building and running `snacc-ts-integration`.

Optional flags: `--skip-stubs`, `--skip-install`, `--frozen-lockfile`, `--clean` (delete each package's `node_modules` before `pnpm install`; or set `SNACC_PREPARE_CLEAN=1`). On Windows, `prepare.bat` waits 10 seconds on success unless `SNACC_NO_PAUSE=1` is set.

# node-server
The Node backend exposes generated ROSE stubs through Express:

- `POST /rest/*` — stateless invokes (no events)
- `WS /ws` — stateful connection; required for server-to-client events

See [`node-server/README.md`](node-server/README.md) for server internals and configuration.

# browser-client
A Vite-based browser demo with a simple HTML UI to call methods on the server. Use this to explore REST vs WebSocket behaviour interactively.

Depends on generated browser stubs (`-RTS_CLIENT_BROWSER`, `TSASN1BrowserClient`).

# node-client
Headless integration tests for the TypeScript stack. This package is the CI counterpart to the C++ runtime tests in `cpp-lib/tests`.

**What it is**
- A small Node program using the built-in `node:test` runner (no Jest, no browser).
- Uses `TSASN1NodeClient` stubs generated with `-RTS_CLIENT_NODE`.
- Starts `node-server` as a child process, waits until `/echo` responds, runs tests, then shuts the server down.

**What it verifies**
- Fresh `esnacc` TypeScript output compiles (`tsc`).
- REST invoke round-trips (`asnGetSettings`, `asnSetSettings`) over **JSON and BER**.
- WebSocket invoke + inbound events (`asnSettingsChanged`, multiple `asnFancyEvent`) over **JSON and BER**.

**What it does not cover**
- Browser / Vite UI (see `browser-client`).
- OpenAPI generation (`openapi/` sample).

**Run locally** (Node 24, `pnpm`):

```bash
# 1. Build esnacc, then from samples/: prepare.bat or ./prepare.sh
# 2. Build and start is handled by the test runner
cd node-server && pnpm run build
cd ../node-client && pnpm run build && pnpm test
```

In GitHub Actions, the same flow is driven by CMake targets `snacc-ts-prepare`, `snacc-ts-all`, and the `snacc-ts-integration` ctest entry.

## Running the browser demo
* Node 24 and VSCode recommended.
* From `samples/`: run `prepare.bat` / `prepare.sh`.
* **Server:** `cd node-server` → copy `.env.sample` to `.env` → `pnpm run build` → `pnpm run start`
* **Browser client:** `cd browser-client` → copy `.env.sample` to `.env` → open `microservice_browser_client.code-workspace` → `pnpm start` → launch the browser debug configuration in VSCode

# OpenApi
Example usage of https://www.npmjs.com/package/@estos/esnacc-openapi-sdk — see the `openapi/` folder.
