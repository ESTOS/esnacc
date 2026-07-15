# esnacc

**ASN.1 → typed stubs, encoders, and ROSE runtime** — primary targets **TypeScript** and **C++**, plus many additional language backends.

[estos](https://www.estos.de) enhanced SNACC: parse ASN.1 modules, generate client/server stubs (where supported), and link against runtimes that handle BER/JSON encoding, ROSE invoke flow, telemetry, and async completion.

## The three blocks

```
  .asn1 modules          esnacc compiler          your application
       │                       │                        ▲
       └──────►  generated stubs (Invoke_ / OnInvoke_) ─┤
       └──────►  runtime library (cpp-lib / TS glue) ──┘
```

| Block | What it is | Where |
|-------|------------|--------|
| **Compiler** | Parses ASN.1, emits types and ROSE stubs | `compiler/`, `output/bin/esnacc` |
| **Generated output** | Types, encoders, ROSE stubs, or API docs — per target | Your build output directory |
| **Runtime** | Transport hooks, invoke context, encode/decode, telemetry | `cpp-lib/`, TS glue under `compiler/back-ends/ts-gen/` |

## Supported targets

The compiler can emit several kinds of output from the same ASN.1 modules. Feature depth varies by backend; **C++** and **TypeScript** are the fully maintained ROSE stacks.

| Target | Generated output |
|--------|------------------|
| **C** | Structure definitions; BER encoder/decoders |
| **C++** | Structure definitions; JSON and BER encoder/decoders; ROSE client/server stubs |
| **TypeScript** | Structure definitions; JSON and BER encoder/decoders; ROSE client stubs (Node + browser) and server stubs |
| **C#** | Structure definitions |
| **Delphi** | Structure definitions |
| **Java** | Structure definitions |
| **Kotlin** | Structure definitions |
| **JavaScript (JSON)** | Structure definitions |
| **JavaScript (ES6)** | Structure definitions |
| **Swift** | Structure definitions |
| **IDL** | Interface definitions |
| **JSDoc** | JSON documentation from ASN.1 comments |
| **OpenAPI** | OpenAPI JSON from ASN.1 comments — use with [esnacc-openapi-sdk](https://github.com/ESTOS/esnacc-openapi-sdk) ([sample](samples/ts-microservice/openapi)) |

Backends live under `compiler/back-ends/` (`c-gen`, `c++-gen`, `ts-gen`, `cs-gen`, `delphi-gen`, `java-gen`, `kotlin-gen`, `js-gen`, `jses6-gen`, `swift-gen`, `idl-gen`, `jsondoc-gen`, `openapi-gen`, …).

Languages without a bundled ROSE runtime typically emit structures only; serialization is handled in application code (often JSON). See [FAQ.md](FAQ.md) for background on BER vs JSON per target.

[samples/](samples/) shows command lines and expected output per language where samples exist.

## C++ ROSE runtime

Generated stubs call into `SnaccROSESender` / `SnaccROSEBase`. Product code typically:

- overrides **`CreateInvokeContext()`** to supply a derived `SnaccInvokeContext`
- uses **`CreateOutboundInvokeContext()`** for timeout / async callback setup on outbound calls
- passes the **stub operation name** to `SendInvoke` / `SendEvent` (authoritative for send, logging, and telemetry)

Inbound context operation names resolve from **`operationID`** via the lookup map, not from wire `operationName`.

**Spec and tests:** [cpp-lib/tests/runtime_correctness_notes.md](cpp-lib/tests/runtime_correctness_notes.md) — intended semantics for the public C++ runtime API (147 runtime tests in `cpp-lib/tests`).

**Core ROSE ASN.1 module:** regenerate checked-in glue from [ROSE/SNACCROSE.asn1](ROSE/SNACCROSE.asn1) via [ROSE/makesnaccrose.bat](ROSE/makesnaccrose.bat).

## TypeScript

The [ts-microservice sample](samples/ts-microservice/readme.md) is the reference layout: Node server (REST + WebSocket), browser client, and headless integration tests over shared ASN.1 interfaces.

Start here: [samples/readme.md](samples/readme.md).

## Quick start

```shell
mkdir build && cd build
cmake ..
cmake --build .
```

Run C++ runtime tests (after build):

```shell
cmake --build . --config Release --target cpp-lib-sample-runtime-tests
./cpp-lib/tests/Release/cpp-lib-sample-runtime-tests    # path may vary by platform
```

Full build options and IDE workflows: **[docs/build.md](docs/build.md)**.

## Documentation

| Topic | Document |
|-------|----------|
| Repository map and agent routing | [AGENTS.md](AGENTS.md) |
| Build (CMake, VS, CLion, VS Code, macOS) | [docs/build.md](docs/build.md) |
| C++ runtime semantics | [cpp-lib/tests/runtime_correctness_notes.md](cpp-lib/tests/runtime_correctness_notes.md) |
| Samples and integration tests | [samples/readme.md](samples/readme.md) |
| Background, licensing, capabilities | [FAQ.md](FAQ.md) |
| OpenAPI + Swagger UI | [esnacc-openapi-sdk](https://github.com/ESTOS/esnacc-openapi-sdk) · [sample](samples/ts-microservice/openapi) |

## Releases

Pre-release betas are published on [GitHub Releases](https://github.com/ESTOS/esnacc/releases). Tags follow `7.0/<version>.beta.<n>`.

Use a tagged release or build from source, then regenerate stubs when upgrading the compiler or runtime.
