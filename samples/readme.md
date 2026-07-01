---
title: esnacc Samples Overview
scope: samples
owner_repo: esnacc
entry_for:
  - sample applications
  - TypeScript microservice sample
  - generated runtime code examples
purpose: Route readers to the sample projects that demonstrate generated esnacc code.
read_when:
  - Running or updating esnacc samples
  - Changing TypeScript sample package layout or integration-test flow
related_docs:
  - ../ReadMe.md
  - ts-microservice/readme.md
---

# Samples
This folder contains samples that show how to use the snacc compiler and the generated runtime code.

## ts-microservice
A TypeScript microservice sample with three packages under `samples/ts-microservice/`:

| Package | Purpose |
| --- | --- |
| `node-server` | Express backend with REST (`/rest`) and WebSocket (`/ws`) entry points for the generated ASN.1 stubs |
| `browser-client` | Vite-based browser demo with a small HTML UI |
| `node-client` | Headless Node integration tests (`node:test`) — the CI gate for TypeScript codegen |

`browser-client` and `node-client` talk to `node-server` over the same ASN.1 interface (`samples/interface/*.asn1`). Communication is either REST-like stateless HTTP POST calls or a stateful WebSocket connection for server-to-client events.

### node-client
`node-client` is not a demo app. It is an automated test runner — similar in spirit to `cpp-lib/tests` for the C++ runtime, but exercising the real HTTP/WebSocket stack:

1. Build `esnacc` and run `samples/prepare.*` (same script CMake uses: stubs + `pnpm install`).
2. Compile `node-server` and `node-client`.
3. `node-client` starts `node-server`, then runs `node:test` cases for REST invokes and WebSocket events.

Current coverage includes `asnGetSettings`, `asnSetSettings` round-trips, `asnSettingsChanged` events, and ordered `asnFancyEvent` delivery from `asnCreateFancyEvents`.

See [`ts-microservice/readme.md`](ts-microservice/readme.md) for setup steps (Node 24, `pnpm`, `prepare.*`).
