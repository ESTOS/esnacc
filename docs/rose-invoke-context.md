---
title: ROSE invoke context
scope: cpp-lib, compiler/back-ends/ts-gen/gluecode
owner_repo: esnacc
entry_for:
  - SnaccInvokeContext
  - CreateInvokeContext
  - createReceiveInvokeContext
  - createSendInvokeContext
  - ReceiveInvokeContext
  - SendInvokeContext
  - BaseInvokeContext
purpose: Human-readable contract for the ROSE invoke context, including every justified C++/TypeScript deviation.
read_when:
  - Changing SnaccInvokeContext, CreateInvokeContext, or the TypeScript receive or send context
  - Checking C++ and TypeScript ROSE parity before a commit, push, or merge request
related_docs:
  - ../AGENTS.md
  - ../cpp-lib/tests/runtime_correctness_notes.md
  - ../.cursor/rules/rose-cross-language-parity.mdc
  - rose-invoke-timeout.md
---

# ROSE invoke context

The invoke context is the object a handler and a sender share for one ROSE exchange. It carries the operation identity, the optional timeout, and any product fields a derived type adds.

C++ builds it with `CreateInvokeContext`. TypeScript builds a receive context with `createReceiveInvokeContext` and a send context with `createSendInvokeContext`. A product overrides the factory for that side to supply a derived type. The runtime then uses that object for the handler or for the outbound send.

The wire timeout rules live in [rose-invoke-timeout.md](rose-invoke-timeout.md). This page is the context that holds the value.

## Shared behavior

- Inbound operation name comes from the operation-id lookup on this stub. A wire `operationName` does not replace that lookup. An unknown id leaves the name empty.
- When the inbound invoke has operation id 0 and a wire name, the id is resolved from the lookup before the name is chosen.
- A present `ROSEInvoke.timeout`, including 0, is stored on the inbound context. An absent field leaves the timeout unset.
- Outbound stores an operation name only when the caller passed an explicit one. The stub literal used for send and telemetry is a separate argument in C++. TypeScript also stores that literal on the outbound context because the context is the only carrier the send path keeps.
- `createReceiveInvokeContext` and `createSendInvokeContext` are the construction paths the runtime uses for inbound dispatch and outbound encode. C++ uses `CreateInvokeContext` for both. A product override is the object the handler or the sender sees.

## API

Casing follows the naming map in `.cursor/rules/rose-cross-language-parity.mdc`.

| C++ | TypeScript | Role |
| --- | --- | --- |
| `CreateInvokeContext` | `createReceiveInvokeContext` | Inbound factory. Override to return a derived receive context |
| `CreateInvokeContext` | `createSendInvokeContext` | Outbound factory. Override to return a derived send context |
| `SnaccInvokeContext` | `BaseInvokeContext` | Shared identity, timeout, connection, and headers. Receive and send extend it |
| `OperationName` | `operationName` | Canonical inbound name, or the explicit outbound name |
| `SetInvokeTimeout` / `InvokeTimeout` | `setInvokeTimeout` / `invokeTimeout` | Stored timeout, empty when unset |
| `PrepareForTelemetry` | `prepareForTelemetry` | Runs on the telemetry clone only |

Sources:

- `cpp-lib/include/SnaccROSEInterfaces.h`
- `cpp-lib/src/SnaccROSEBase.cpp`
- `compiler/back-ends/ts-gen/gluecode/TSASN1Base.ts`
- `compiler/back-ends/ts-gen/gluecode/TSROSEBase.ts`

## Deviations

| Topic | C++ | TypeScript | Why this is allowed |
| --- | --- | --- | --- |
| Outbound name on the context | Empty unless `SnaccInvokeContextInit` was given a name. Send uses the stub literal argument. | `encodeInvoke` passes the stub literal into `createSendInvokeContext`, so the outbound context holds that name. | TypeScript has no separate operation-name argument at send time. The lookup rule for inbound names is the same. |
| Context class | One `SnaccInvokeContext`. Products derive it. | `BaseInvokeContext` holds the shared fields. `ReceiveInvokeContext` and `SendInvokeContext` add the side-specific fields. Products derive those. | The two classes carry the HTTP and REST fields that exist only on the TypeScript transport. |
| Factory | One `CreateInvokeContext`. `SnaccInvokeDirection` selects the side. | `createReceiveInvokeContext` returns `ReceiveInvokeContext`. `createSendInvokeContext` returns `SendInvokeContext`. | One TypeScript method would return a union, and every caller would narrow it. C++ has one context class, so one method is enough. |
| Method casing | `PascalCase` | `camelCase` | Language naming map. |

## Tests

| Behavior | C++ | TypeScript |
| --- | --- | --- |
| Lookup wins over a wire name | `InboundInitResolvesOperationNameFromInvokeLookup` | `InboundReceiveResolvesOperationNameFromLookupWhenWireNamePresent` |
| Wire timeout, including 0, reaches the handler | `OutboundWireInvokeTimeoutReachesInboundHandlerJson` / `Ber` | `OutboundWireInvokeTimeoutReachesInboundHandler (JSON)` / `(BER)` |
| Absent wire timeout stays unset | `InboundInitWithoutWireTimeoutLeavesUnset` | `InboundReceiveLeavesTimeoutUnsetWhenWireFieldAbsent` |
| Product factory reaches the handler | `AssertInboundHandlerReceivesCustomInvokeContext` | `CreateReceiveInvokeContextOverrideReachesInboundHandler` |

## How this file stays current

Update this file in the same change as the code when the context's identity, timeout, or factory changes, or when C++ and TypeScript stop doing the same thing. A new difference needs a row in the deviations table and a pair of tests.
