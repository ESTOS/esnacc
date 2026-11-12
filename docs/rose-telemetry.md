---
title: ROSE telemetry
scope: cpp-lib, compiler/back-ends/ts-gen/gluecode
owner_repo: esnacc
entry_for:
  - SnaccTelemetryData
  - OnInvokeProcessed
  - onInvokeProcessed
purpose: Human-readable contract for ROSE invoke telemetry, including every justified C++/TypeScript deviation.
read_when:
  - Changing SnaccTelemetryData, OnInvokeProcessed, or the TypeScript telemetry callback
  - Checking C++ and TypeScript ROSE parity before a commit, push, or merge request
related_docs:
  - ../AGENTS.md
  - ../cpp-lib/tests/runtime_correctness_notes.md
  - ../.cursor/rules/rose-cross-language-parity.mdc
  - rose-invoke-context.md
  - rose-invoke-timeout.md
---

# ROSE telemetry

Each ROSE lifecycle produces one record. The record names the direction, the operation, how it ended, and why. `OnInvokeProcessed` / `onInvokeProcessed` delivers it after `finalize`. Duration is the local time inside this runtime.

C++ and TypeScript use the same outcome and reason values, finalized at the same points. The [deviations](#deviations) section is the only place a difference is allowed to exist without being called a bug.

## Shared behavior

- Fire-and-forget (`timeout` 0) is `DISPATCHED` and `WAIT_SKIPPED`. It is not `UNHANDLED`.
- A caller-visible payload decode failure is `UNHANDLED` and `DECODE_FAILED`, not the envelope kind (`RESULT` or `ERR`).
- An envelope that cannot be decoded is `INBOUND_DECODE` and `DECODE_FAILED`.
- A normal result is `RESULT` / `LOCAL_RESULT` on the handler and `RESULT` / `REMOTE_RESULT` on the caller. An application error uses `ERR` / `LOCAL_ERROR` and `ERR` / `REMOTE_ERROR`.
- A peer reject is `REJECT` / `REMOTE_REJECT` on the caller and `INBOUND_RESPONSE`.
- An event is `EVENT` / `LOCAL_EVENT` on the sender (`OUTBOUND_SEND`) and on the receiver (`INBOUND_INVOKE`).
- A closed session gate is `UNHANDLED` / `SHUTDOWN`. A local wait that ends with no answer is `UNHANDLED` / `TIMEOUT`.
- The context stored on the record is a clone. `prepareForTelemetry` runs on that clone. The live context is unchanged.
- No callback means the record is not built. The runtime does not allocate `SnaccTelemetryData` and does not clone the invoke context.

## API

| C++ | TypeScript | Role |
| --- | --- | --- |
| `SnaccTelemetryData::Create` | `SnaccTelemetryData.create` | Start a record |
| `SnaccTelemetryData::CreateFinalized` | `SnaccTelemetryData.createFinalized` | Start and finalize |
| `finalize` | `finalize` | Fill outcome, stage, reason, and the cloned context |
| `Direction`, `Outcome`, `Stage`, `Reason` | The same nested enums | Same numeric values |
| `OnInvokeProcessed` | `onInvokeProcessed` | Deliver one finalized record |
| `SnaccROSEBase::SetTelemetryCallback` | `setTelemetryCallback` | Install or clear the sink |

Sources:

- `cpp-lib/include/SnaccTelemetry.h`
- `cpp-lib/src/SnaccROSEBase.cpp`
- `compiler/back-ends/ts-gen/gluecode/SnaccTelemetry.ts`
- `compiler/back-ends/ts-gen/gluecode/TSASN1Base.ts`
- `compiler/back-ends/ts-gen/gluecode/TSROSEBase.ts`

## Deviations

| Topic | C++ | TypeScript | Why this is allowed |
| --- | --- | --- | --- |
| Callback scope | `SetTelemetryCallback` is one static sink for every stub in the process. | `setTelemetryCallback` is one sink per transport. | A TypeScript process often hosts more than one connection object. Each transport delivers its own records. |
| Async completion | `SendInvokeAsync` finalizes on the callback. | The pending `Promise` is the completion channel. The same outcomes are emitted from `handleInvoke`. | One completion channel in JavaScript. |
| Peer-reject code | `ROSE_REJECT_*` from `GetRejectResultCode`. | A peer reject stores the wire `invokeProblem` when that value is not already an `ERoseResult`. Outcome and reason stay `REJECT` / `REMOTE_REJECT`. | TypeScript answers are the ROSE message. The wire problem is the value the caller already sees. |
| Clock | `std::chrono::steady_clock` | `performance.now()` | Both are a monotonic local duration in milliseconds. |
| Worker pools | Inbound finalize can run on a pool worker. | No pool runtime. Finalize runs on the caller that received the invoke. | TypeScript has no worker pools. See [rose-worker-pools.md](rose-worker-pools.md). |

## Tests

| Behavior | C++ | TypeScript |
| --- | --- | --- |
| Result, error, reject, event | `HappyPathTelemetry*`, `ApplicationErrorTelemetry*`, `RejectTelemetry*`, `EventTelemetry*` | The same names with `(JSON)` and `(BER)` in `TSTelemetry.test.ts` |
| Envelope decode failure | `DecodeFailureTelemetry*` | `DecodeFailureTelemetry (JSON)` / `(BER)` |
| Timeout and fire-and-forget | `TimeoutTelemetry*`, `WaitSkippedTelemetry*` | `TimeoutTelemetry (JSON)` / `(BER)`, `WaitSkippedTelemetry (JSON)` / `(BER)` |
| Payload decode failure | `ResultPayloadDecodeFailureTelemetry*` | `ResultPayloadDecodeFailureTelemetry (JSON)` |
| Context clone | `CustomContextIsClonedAndPrepared*` | `CustomContextIsClonedAndPrepared (JSON)` |
| Shutdown before send | `ShutdownPendingInvokeTelemetry*` | `ShutdownPendingInvokeTelemetry (JSON)` |

## How this file stays current

Update this file in the same change as the code when an outcome, a reason, or a finalize point changes, or when C++ and TypeScript stop doing the same thing. A new difference needs a row in the deviations table and a pair of tests.
