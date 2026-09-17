---
title: ROSE HTTP status mapping (TypeScript glue)
scope: compiler/back-ends/ts-gen/gluecode
owner_repo: esnacc
entry_for:
  - TypeScript ROSE REST/fetch transport
  - HTTP status on ROSE invoke responses
purpose: Document how TSASN1Base maps ROSE outcomes to HTTP status for REST/fetch; ROSEMessage body is unchanged.
read_when:
  - Changing handleResult or fetch/REST response status in ts-gen gluecode
  - Adding ROSE transport tests that assert HTTP status
related_docs:
  - ../../../../AGENTS.md
  - ../../../../cpp-lib/tests/runtime_correctness_notes.md
---

# ROSE HTTP status mapping

REST and `fetch` transports return a full encoded **`ROSEMessage`** in the body on every invoke response. HTTP status is **transport metadata only** — callers must read the ROSE arm (`result`, `error`, `reject`) and encoded payload for the real outcome.

Implementation: `httpStatusFromRoseOutcome()` in `TSROSEBase.ts`, used from `TSASN1Base.handleResult()`.

## Rules

1. **Never map `AsnRequestError.iErrorDetail` / `ROSEError.error_value` to HTTP.** Application error codes stay in the body.
2. **`500` is reserved for `ROSEError` only** (handler returned `AsnRequestError`). Rejects must not use `500`.
3. **Reject fallback:** any `ROSEReject` without a specific mapping → **`502`**.
4. **Do not invent custom HTTP codes.** Use registered status codes only.

## Top-level ROSE arms

| ROSE arm | HTTP | Notes |
|----------|------|-------|
| `ROSEResult` | **200** | Success |
| `ROSEError` | **500** | Application error; detail in `error` / `AsnRequestError` |
| `ROSEReject` | see below | Protocol / stub / transport rejection |

## `invokeProblem` (standard ROSE)

| Case | HTTP |
|------|------|
| `duplicateInvocation` (0) | **409** |
| `unrecognisedOperation` (1) | **501** |
| `mistypedArgument` (2) | **400** |
| `resourceLimitation` (3) | **503** |
| `initiatorReleasing` (4) | **408** |
| `unrecognisedLinkedID` (5) | **502** |
| `linkedResponseUnexpected` (6) | **502** |
| `unexpectedChildOperation` (7) | **502** |
| `invalidSessionID` (8) | **401** |
| `authenticationIncomplete` (9) | **401** |
| `authenticationFailed` (10) | **401** |

## Custom `invokeProblem` values (`CustomInvokeProblemEnum`)

These reuse the `invokeProblem` slot with extension codes. HTTP status follows **semantics**, not the numeric enum value.

| Custom code | HTTP |
|-------------|------|
| `missingResponse` (444) | **504** |
| `serviceUnavailable` (503) | **503** |
| `requestTimedOut` (504) | **504** |
| `internalError` (500) | **502** |
| `messageTooBig` (501) | **413** |
| `emptyRejectMessage` (502) | **502** |
| `remoteNotCapable` (0xE00) | **501** |

## `generalProblem`

| Case | HTTP |
|------|------|
| `unrecognisedAPDU` / `mistypedAPDU` / `badlyStructuredAPDU` | **400** |

## `returnResultProblem` / `returnErrorProblem`

Rare in TypeScript-to-TypeScript flows → **502** (reject fallback).

## Client usage

- **`200`:** decode `result`.
- **`500`:** decode `error` (`AsnRequestError` or operation-specific error type).
- **Any other non-200:** decode `reject` (`AsnInvokeProblem` / `invokeProblem`).

Loopback, WebSocket, and TCP transports may ignore HTTP status; REST/fetch should set it on the response.

## Cross-language parity

HTTP mapping is **TypeScript REST/fetch glue** today. C++ ROSE runtimes without HTTP front-ends do not use this table. If another language adds REST ROSE, reuse the same mapping and add paired tests.
