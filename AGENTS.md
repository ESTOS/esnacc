# Agent guide for estos esnacc

This repository is the **estos enhanced SNACC ASN.1 compiler** (`esnacc`). It parses ASN.1 modules and generates target-language stubs, runtime helpers, and protocol documentation.

Read this file first. Then follow the documentation routing metadata in markdown files across the repo.

## Documentation routing

Several markdown files begin with YAML front matter:

```yaml
---
title: ...
scope: ...
owner_repo: esnacc
entry_for: [...]
purpose: ...
read_when: [...]
related_docs: [...]
---
```

Use that metadata to choose docs before searching code at random:

1. Match the user task to `read_when` or `entry_for`.
2. Read `purpose` to confirm relevance.
3. Follow `related_docs` for the next file.
4. Respect `scope` — stay inside the described subsystem unless the task clearly spans repos.

Discover routed docs:

```bash
rg -l "^read_when:" --glob "*.md"
```

## Repository map

| Path | What it is | When to work here |
|------|------------|-------------------|
| `compiler/` | ASN.1 parser, core compiler, CLI | Compiler behavior, CLI flags, parsing, shared codegen |
| `compiler/back-ends/` | Per-language code generators (`ts-gen`, `c++-gen`, `c-gen`, `cs-gen`, …) | Generated output for a specific target language |
| `cpp-lib/` | C++ runtime library (`esnacc_cpp_lib`) | BER/JSON codecs, ROSE runtime, C++ consumer APIs |
| `c-lib/` | C runtime support | C target runtime and helpers |
| `ROSE/` | ROSE-related shared material | ROSE protocol/client-server concerns |
| `samples/` | End-to-end usage examples | Verifying generation, integration patterns, sample apps |
| `samples/ts-microservice/` | Main TypeScript sample (server, client, browser, OpenAPI) | TypeScript ROSE/REST/WebSocket workflows |

Build outputs default under `output/` (compiler binary, libraries).

## Primary maintained targets

TypeScript and C++ have the fullest feature set:

- structure definitions
- JSON and BER encoders/decoders
- ROSE client/server stubs

Other backends (C, C#, Java, Kotlin, Swift, Delphi, JavaScript, IDL, JSDoc, OpenAPI) vary in completeness. Check the matching directory under `compiler/back-ends/` and treat samples as the source of truth for what is actually exercised.

## Routed documentation index

| Doc | Use when |
|-----|----------|
| [ReadMe.md](ReadMe.md) | Build instructions, repo overview, supported targets |
| [FAQ.md](FAQ.md) | Compiler background, licensing, capability questions |
| [samples/readme.md](samples/readme.md) | Running or updating samples |
| [samples/ts-microservice/readme.md](samples/ts-microservice/readme.md) | TypeScript microservice layout and verification flow |
| [samples/ts-microservice/node-server/README.md](samples/ts-microservice/node-server/README.md) | Node server handlers, config, generated stub usage |
| [cpp-lib/tests/runtime_correctness_notes.md](cpp-lib/tests/runtime_correctness_notes.md) | Intended C++ runtime semantics and correctness tests |

## Typical task routing

| Task | Start here | Then inspect |
|------|------------|--------------|
| Change TypeScript generation | `ReadMe.md`, `compiler/back-ends/ts-gen/` | `samples/ts-microservice/` |
| Change C++ generation or runtime | `ReadMe.md`, `cpp-lib/tests/runtime_correctness_notes.md` | `compiler/back-ends/c++-gen/`, `cpp-lib/` |
| Add/fix a sample | `samples/readme.md` | The specific sample subdirectory |
| Build or CI for the compiler | `ReadMe.md` | `compiler/CMakeLists.txt`, root `CMakeLists.txt` |
| OpenAPI / JSDoc output | `FAQ.md` | `compiler/back-ends/openapi-gen/`, `compiler/back-ends/jsondoc-gen/` |

## Build quick reference

From repo root:

```shell
mkdir build
cd build
cmake ..
cmake --build .
```

Requirements and IDE-specific steps are in [ReadMe.md](ReadMe.md). CMake minimum version: 3.20.

## Working conventions

- Preserve existing line endings and formatting in edited files.
- Prefer the smallest correct change; match surrounding code style.
- Generated output shape is defined by ASN.1 inputs plus the relevant `compiler/back-ends/*-gen` implementation — read both before changing behavior.
- When documentation and code disagree, verify against `samples/` and tests before updating docs.

## Related repositories

- [esnacc-openapi-sdk](https://github.com/ESTOS/esnacc-openapi-sdk) — Swagger UI integration for generated OpenAPI output
- Consumer projects may vendor generated stubs or runtime libraries built from this repo; confirm the target repo before assuming shared release layout
