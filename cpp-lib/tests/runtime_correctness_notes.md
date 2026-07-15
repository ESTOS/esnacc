---
title: C++ Runtime Correctness Notes
scope: cpp-lib/tests
owner_repo: esnacc
entry_for:
  - C++ runtime behavior
  - runtime correctness tests
  - ROSE telemetry and shutdown semantics
purpose: Record intended C++ runtime semantics and implementation status for ROSE correctness areas exercised by cpp-lib runtime tests.
read_when:
  - Changing cpp-lib runtime behavior, telemetry, shutdown, or decode-error handling
  - Adding or reviewing runtime correctness tests
related_docs:
  - ../../AGENTS.md
  - ../../ReadMe.md
---

# Runtime Correctness Notes

This note records the intended semantics for selected `cpp-lib` runtime behaviors
and whether the current tree implements them. The goal is to align the runtime
to the public API contract and to common operator expectations rather than
simply preserving whatever behavior exists today.

Primary reference points:
- `cpp-lib/include/SnaccROSEBase.h`
- `cpp-lib/include/SnaccROSEInterfaces.h`
- `cpp-lib/include/SnaccTelemetry.h`
- `cpp-lib/src/SnaccROSEBase.cpp`

## Summary

| Area | Status | Semantics | Primary tests |
| --- | --- | --- | --- |
| `StopProcessing()` shutdown gate | Implemented | Refuse new outbound work; block inbound handler dispatch; complete pending ops with `ROSE_TE_SHUTDOWN` | `PublicApiRuntimeTest.StopProcessingBlocks*`, `LifecycleRuntimeTest.StopProcessing*` |
| Fire-and-forget (`iTimeout == 0`) telemetry | Implemented | `Outcome::DISPATCHED` + `Reason::WAIT_SKIPPED`, not `UNHANDLED` | `TelemetryRuntimeTest.WaitSkippedTelemetry*` |
| Response payload decode telemetry | Implemented | Caller-visible `ROSE_RE_DECODE_FAILED` drives `UNHANDLED` + `DECODE_FAILED`, not envelope kind | `TelemetryRuntimeTest.*PayloadDecodeFailureTelemetry*` |
| Inbound decode failures and ROSE rejects | Implemented | Garbage wire silent; targeted reject only after envelope decode | `InvokeContextRuntimeTest.UnparsableInbound*`, section 5 |
| `OnBinaryDataBlockResult()` decode-error hooks | Implemented | `OnRoseDecodeError()` and `bAlreadyTransportLogged` parity with `OnBinaryDataBlock()` | `PublicApiSmokeTest.OnBinaryDataBlockResultDecodeErrorsInvokeHook*` |
| Inbound `ROSEMessage` ownership | Implemented | `unique_ptr` at decode sites; `std::move` through dispatch | Section 6; `InvokeContextRuntimeTest` suite |
| Outbound encode / `Send()` ownership | Implemented | RAII encode helpers detach borrowed arms on scope exit (including encode exceptions) | Section 7; outbound encode-failure tests in `InvokeContextRuntimeTest` |

## 1. `StopProcessing()` Shutdown Contract

### Status: implemented

### Public contract

`SnaccROSEBase` documents shutdown as a hard stop:

```152:156:cpp-lib/include/SnaccROSEBase.h
	/*! Shutdown.
		Call this function to stop processing any more Invokes.
		All pending operations will be completed and new function calls will be blocked.
		All Functions return a ROSE_TE_SHUTDOWN */
	void StopProcessing(bool bStop = true);
```

### Intended behavior

Treat `StopProcessing(true)` as a real runtime shutdown gate:

1. New outbound invokes and events must fail fast with `ROSE_TE_SHUTDOWN`.
2. Pending operations must still be completed with `ROSE_TE_SHUTDOWN`.
3. New inbound invokes and inbound events must not be dispatched to application
   handlers while shutdown is active.
4. Late inbound responses that arrive after pending operations were force-
   completed may be ignored, but they must not resurrect completed work.
5. `StopProcessing(false)` re-enables processing; callers must treat that as an
   explicit restart, not an incidental side effect.

### Implementation

`StopProcessing(true)` clears `m_bProcessingAllowed` and completes all pending
operations with `ROSE_TE_SHUTDOWN`:

```605:628:cpp-lib/src/SnaccROSEBase.cpp
void SnaccROSEBase::StopProcessing(bool bStop /*= true*/)
{
	{
		std::lock_guard<std::mutex> guard(m_InternalProtectMutex);
		m_bProcessingAllowed = bStop ? false : true;
	}

	if (bStop)
		CompleteAllPendingOperations();
}
...
	for (auto it = m_PendingOperations.begin(); it != m_PendingOperations.end(); it++)
		it->second->CompleteOperation(ROSE_TE_SHUTDOWN);
```

Outbound choke points check `IsProcessingAllowed()` before creating pending
operations or sending:

```1604:1610:cpp-lib/src/SnaccROSEBase.cpp
	if (!IsProcessingAllowed())
	{
		auto telemetry = SnaccTelemetryData::Create(...);
		telemetry->finalize(..., SnaccTelemetryData::Reason::SHUTDOWN, ROSE_TE_SHUTDOWN, ...);
		OnInvokeProcessed(telemetry);
		return ROSE_TE_SHUTDOWN;
	}
```

`SendEvent()` uses the same gate and returns `ROSE_TE_SHUTDOWN` without sending.

Inbound invoke/event dispatch is blocked in `OnInvokeMessage()`:

```1324:1325:cpp-lib/src/SnaccROSEBase.cpp
	if (!IsProcessingAllowed())
		lResult = ROSE_TE_SHUTDOWN;
```

Wire data may still be decoded on the receive path; handlers are not reached
while shutdown is active.

### Tests that enforce this

- `PublicApiRuntimeTest.StopProcessingBlocksNewOutboundInvokesAndEvents`
- `PublicApiRuntimeTest.StopProcessingBlocksInboundDispatchUntilReEnabled`
- `LifecycleRuntimeTest.StopProcessingCompletesPendingInvokeWithShutdownBer`
- `LifecycleRuntimeTest.StopProcessingCompletesPendingInvokeWithShutdownJson`
- `LifecycleRuntimeTest.PendingInvokeCanRecoverAfterShutdownOnNextFixtureSetupBer`
- `LifecycleRuntimeTest.PendingInvokeCanRecoverAfterShutdownOnNextFixtureSetupJson`

## 2. Fire-And-Forget Invoke Telemetry (`iTimeout == 0`)

### Status: implemented

### Intended behavior

Fire-and-forget is a successful local dispatch of an invoke whose remote outcome
is intentionally unknown to this runtime instance:

1. `Outcome::DISPATCHED` for "sent, not awaited".
2. `Reason::WAIT_SKIPPED` preserves the explicit cause.
3. `Stage::OUTBOUND_WAIT` is acceptable for now; a finer stage taxonomy is
   deferred until async invokes that complete via callback reshape outbound
   lifecycle telemetry anyway.

`WAIT_SKIPPED` must not be classified under the same top-level failure bucket as
transport errors, timeouts, shutdown, invalid responses, or decode failures.

### Implementation

When `iTimeout == 0`, `SendInvoke()` records local success (`ROSE_NOERROR`) and
does not wait for a response. `FinalizeTelemetry()` then classifies the
lifecycle as dispatched, not unhandled:

```581:581:cpp-lib/src/SnaccROSEBase.cpp
	m_pTelemetry->finalize(SnaccTelemetryData::Outcome::DISPATCHED, SnaccTelemetryData::Stage::OUTBOUND_WAIT, SnaccTelemetryData::Reason::WAIT_SKIPPED, m_lRoseResult, std::nullopt, std::move(pctx));
```

`SnaccTelemetryData::Outcome::DISPATCHED` and its debug text are defined in
`cpp-lib/include/SnaccTelemetry.h` and `cpp-lib/src/SnaccTelemetry.cpp`.

### Tests that enforce this

- `TelemetryRuntimeTest.WaitSkippedTelemetryBer`
- `TelemetryRuntimeTest.WaitSkippedTelemetryJson`

## 3. Outbound Telemetry After Response Payload Decode Failure

### Status: implemented

### Intended behavior

For outbound invoke telemetry, the final caller-visible result is the
authoritative classification:

1. If the response envelope was received but payload decode fails, telemetry
   finalizes as `Outcome::UNHANDLED`.
2. The reason is `DECODE_FAILED`.
3. The result code is `ROSE_RE_DECODE_FAILED`.
4. Envelope kind (`result` vs `error`) must not override the primary outcome.

### Implementation

`HandleInvokeResult()` can return `ROSE_RE_DECODE_FAILED` after a valid envelope
when the embedded result or error payload cannot be decoded. `FinalizeTelemetry()`
compares the stored pending-op result with the final caller-visible result and
prefers the final outcome when they differ:

```549:552:cpp-lib/src/SnaccROSEBase.cpp
	if (m_pAnswerMessage && lFinalRoseResult != m_lRoseResult)
	{
		m_pTelemetry->finalize(SnaccTelemetryData::Outcome::UNHANDLED, GetOutboundUnhandledStageFromResult(lFinalRoseResult), GetUnhandledReasonFromResult(lFinalRoseResult), lFinalRoseResult, m_stResponseData, std::move(pctx));
		return;
	}
```

When payload decode succeeds, envelope kind still drives `RESULT`, `ERR`, or
`REJECT` telemetry as before.

### Tests that enforce this

- `TelemetryRuntimeTest.ResultPayloadDecodeFailureTelemetryBer`
- `TelemetryRuntimeTest.ResultPayloadDecodeFailureTelemetryJson`
- `TelemetryRuntimeTest.ErrorPayloadDecodeFailureTelemetryBer`
- `TelemetryRuntimeTest.ErrorPayloadDecodeFailureTelemetryJson`

## 4. `OnBinaryDataBlockResult()` Decode-Error Hook and Logging Parity

### Status: implemented

Both inbound entry points call `OnRoseDecodeError()` for comparable decode
failure classes (BER envelope decode, JSON envelope decode, JSON parse failure,
unknown encoding). Both pass the real `bAlreadyTransportLogged` value derived
from `LogTransportData()` return value before invoking the hook.

Shared private methods on `SnaccROSEBase` centralize logging, hook invocation,
optional reject, and telemetry:

| Method | Role |
| --- | --- |
| `HandleInboundEnvelopeSnaccDecodeFailure` | `SnaccException` after BER `BDec` or JSON `JDec` |
| `HandleInboundJsonParseDecodeFailure` | `SJson::Reader::parse` failure |
| `HandleInboundUnknownEncodingDecodeFailure` | Unknown `m_eTransportEncoding` |
| `HandleInboundOuterDecodeFailure` | Outer `catch` around the encoding switch |
| `EmitInboundDecodeFailureTelemetry` | `OnInvokeProcessed` for decode failures |

`OnBinaryDataBlock()` passes `bSendReject=true` into the envelope helper;
`OnBinaryDataBlockResult()` passes `bSendReject=false`.

### Tests that enforce this

- `PublicApiSmokeTest.OnBinaryDataBlockResultDecodeErrorsInvokeHookBer`
- `PublicApiSmokeTest.OnBinaryDataBlockResultDecodeErrorsInvokeHookJson`

## 5. Inbound Decode Layers, Reject Policy, and BER vs JSON

### Why BER and JSON are not symmetric at the wire layer

**BER** is decoded incrementally as nested TLVs. The runtime can fail at
different depths on the same buffer:

1. **Wire garbage** — not even a decodable `ROSEMessage` (for example truncated
   tag/length).
2. **Envelope incomplete** — some bytes consumed, but `ROSEMessage::BDec` did not
   finish; generated CHOICE `choiceId` must not be trusted (codegen defers
   `choiceId` until the selected arm decodes successfully).
3. **Envelope OK, payload bad** — invoke envelope is valid; operation argument
   decode fails later in `OnInvokeMessage`.

**JSON** does not offer an envelope-only parse for invalid wire text.
`SJson::Reader::parse` is all-or-nothing on the payload after the `J` length
prefix:

- If parse fails, there is no `SJson::Value` tree and no partial ROSE structure
  to inspect.
- Wire failure and “not a ROSE JSON object” collapse into one step for malformed
  syntax.
- Only after parse succeeds does `ROSEMessage::JDec` run field-by-field (layer 2
  above).

So BER admits **layered** failure classification at runtime; JSON only admits
layers **after** syntactically valid JSON exists.

### Intended reject policy (implemented)

Outbound ROSE rejects must be **correlatable and semantically honest**. Do not
claim `mistypedArgument` when no invoke was successfully decoded.

| Layer | What failed | `bRoseEnvelopeDecoded` | Outbound ROSE reject? |
| --- | --- | --- | --- |
| Wire / syntax | BER garbage, JSON `parse` fail, unknown encoding | n/a (no envelope) | **No** — log, `OnRoseDecodeError`, telemetry only |
| Envelope | `BDec` / `JDec` on `ROSEMessage` did not complete | `false` | **No** |
| Envelope OK, invoke path | Decode or dispatch failed after envelope succeeded | `true` and `invoke` present | **Yes** on `OnBinaryDataBlock()` — `mistypedArgument` with real `invokeID` |
| Argument | Operation argument decode in handler path | n/a (handler stage) | **Yes** — `OnInvokeMessage` / handler reject path |

Garbage wire therefore gets **no response** on the application ROSE layer (common
RPC practice: the caller times out; an uncorrelated `invokednull` reject does not
help a pending client invoke).

### Intentional asymmetry: `OnBinaryDataBlock()` vs `OnBinaryDataBlockResult()`

| Entry point | Role | Reject on decode failure? |
| --- | --- | --- |
| `OnBinaryDataBlock()` | Inbound invokes/events (server receive path) | **May** send targeted `mistypedArgument` when envelope decode succeeded and `invoke` is known |
| `OnBinaryDataBlockResult()` | Inbound results/errors/rejects (client response path) | **Must not** send rejects for decode failures; log + hook + telemetry only |

Hook and logging parity between the two paths is required. **Reject parity is not**
— the response path must not fabricate server-side rejects when a reply cannot be
decoded.

Legacy reject branches were removed from `OnBinaryDataBlockResult()` decode
catches; that path is telemetry-only on decode failure.

### Tests that enforce this

- `InvokeContextRuntimeTest.UnparsableInboundDoesNotReachHandlerBer`
- `InvokeContextRuntimeTest.UnparsableInboundDoesNotReachHandlerJson`
- CHOICE `choiceId` deferral: `compiler/back-ends/c++-gen/gen-code.c` (regenerated
  `SNACCROSE.cpp`)

### Likely code paths

- `SnaccROSEBase::OnBinaryDataBlock()`
- `SnaccROSEBase::OnBinaryDataBlockResult()`
- `SnaccROSEBase::OnInvokeMessage()` (argument-layer rejects)
- `compiler/back-ends/c++-gen/gen-code.c` (CHOICE decode / `choiceId`)

## 6. `ROSEMessage` Ownership on Inbound Decode Paths

### Status: implemented

### Contract

Inbound decode paths allocate with `std::make_unique<ROSEMessage>()`. After a
successful envelope decode (`BDec` / `JDec`), reject-relevant invoke fields are
snapshotted into `InboundInvokeRejectContext` (invoke ID, operation ID,
operation name). Ownership then moves into `OnROSEMessage()` via `std::move`.
If dispatch throws, the outer `catch` uses the snapshot for targeted
`mistypedArgument` rejects — not the moved-away message.

`OnROSEMessage()` takes `std::unique_ptr<ROSEMessage>`:

| Stage | Owner |
| --- | --- |
| Before envelope decode | Local `unique_ptr` in the decode `try` block |
| After envelope decode, before `OnROSEMessage` | Local `unique_ptr` + optional `InboundInvokeRejectContext` snapshot |
| Invoke/event dispatch | `OnInvokeMessage(std::unique_ptr)` — destroyed after dispatch |
| Matched result/error/reject | `CompletePendingOperation(std::move)` → `m_pAnswerMessage` |
| Orphan result/error/reject | `CompletePendingOperation()` when lookup fails |
| `SnaccException` before envelope decode | Local `unique_ptr` destroyed on scope exit |
| `SnaccException` after envelope decode | `rejectCtx` snapshot drives reject/telemetry; `unique_ptr` destroyed on scope exit |

Reject policy in decode `catch` blocks:

| Entry point | Send `mistypedArgument` reject? |
| --- | --- |
| `OnBinaryDataBlock()` | Yes, when `rejectCtx` is present and invoke ID ≠ 99999 |
| `OnBinaryDataBlockResult()` | No — telemetry only |

### Likely code paths

- `SnaccROSEBase::OnBinaryDataBlock()`
- `SnaccROSEBase::OnBinaryDataBlockResult()`
- `SnaccROSEBase::OnROSEMessage()`
- `SnaccROSEBase::CompletePendingOperation()`
- `SnaccROSEPendingOperation::CompleteOperation()`

## 7. Outbound Encode and `Send()` Ownership

### Status: implemented

### Problem

Outbound encoding builds temporary `ROSEMessage` trees that borrow caller-owned
invoke, result, error, or reject objects. Without explicit detach before
destruction, `~ROSEMessage` can delete borrowed values. The previous pattern
used manual `// prevent delete` nulling on the happy path only, which was fragile
when encode threw.

### Contract

1. Caller retains ownership of invoke arguments, result/error payloads, and reject
   objects passed into `Send()`, `EncodeResult()`, `EncodeError()`, and
   `EncodeReject()`.
2. Stack `ROSEMessage` envelopes used for encoding must detach borrowed arms in
   all exit paths, including encode exceptions.
3. `ScopedInvokeOperationName` may allocate a temporary `operationName` on the
   caller invoke for JSON encoding only; it removes that allocation on scope exit.

### Implementation

File-local RAII helpers in `SnaccROSEBase.cpp` (anonymous namespace):

| Helper | Role |
| --- | --- |
| `RoseEncodeRejectBorrow` | Binds stack `ROSEReject` into `ROSEMessage` for `EncodeReject` |
| `RoseEncodeResultEnvelope` | Owns stack `ROSEResult` + encode allocations; borrows result payload |
| `RoseEncodeErrorEnvelope` | Owns stack `ROSEError` + `AsnAny` wrapper; borrows error payload |
| `ScopedEncodeInvokeBorrow` | Binds caller `ROSEInvoke` into outbound `ROSEMessage` for `Send` |
| `ScopedInvokeOperationName` | Adds/removes temporary JSON `operationName` on caller invoke |

Each helper detaches borrowed pointers in its destructor.

### Tests that exercise this

- Happy-path outbound invoke/response flows across the runtime test suite
- `InvokeContextRuntimeTest.OutboundEncodeFailureKeepsCallerContextJson` (encode
  failure must not corrupt caller-owned invoke context)

## 8. Operation Name Resolution

### Status: implemented (7.0.9)

### Contract

1. **Outbound invokes and events:** the generated stub literal passed to
   `SendInvoke` / `SendEvent` is the authoritative operation name for telemetry,
   logging, and transport encoding. Normal outbound contexts use
   `CreateOutboundInvokeContext()` (no name on the context). Generated deprecated
   outbound stubs default to `CreateInvokeContext(SnaccInvokeContextInit(OUTBOUND,
   invoke, operationName))` so `SNACCDeprecated::DeprecatedASN1Method` can read
   `OperationName()` on the context.
2. **Lookup map:** `SnaccRoseOperationLookup::LookUpName()` is a parachute when
   the stub name is absent on outbound paths. Registration in the lookup map is
   optional but required for inbound context naming when only `operationID` is known.
3. **Inbound invokes:** `operationID` is authoritative for dispatch. When the client
   sends `operationID: 0` with `operationName`, `PrepareInboundInvokeOperationId`
   resolves the ID via `LookUpID` before stub dispatch and before the invoke context
   is built. The context name is then set via `LookUpName(operationID)` — never from
   the wire string. Synthetic inbound paths without a decoded invoke may pass an
   explicit name to `SnaccInvokeContextInit`.
4. **Context factory:** runtime and generated stubs must create contexts only through
   `SnaccROSESender::CreateInvokeContext()` (including helpers such as
   `CreateOutboundInvokeContext()`). Never call `SnaccInvokeContext::Create()` from
   product/runtime code except the default `CreateInvokeContext` implementation.
5. **`SnaccInvokeContext::OperationName()`:** inbound from `LookUpName(operationID)`.
   Outbound only when explicitly passed to `SnaccInvokeContextInit` (deprecated stubs).
6. **`SnaccInvokeContextInit::m_strOperationName`:** mirrors the same rule.

## Recommended Follow-Up Order

1. ~~Shutdown contract~~ (done)
2. ~~Fire-and-forget telemetry classification~~ (done)
3. ~~Response-payload decode telemetry~~ (done)
4. ~~Inbound decode reject policy and `OnBinaryDataBlockResult()` reject cleanup~~ (done)
5. ~~Inbound and outbound `ROSEMessage` ownership~~ (done)
6. Push branch and open PR for UCAAS-1446.
7. ~~Extract shared decode-failure helpers between `OnBinaryDataBlock()` and `OnBinaryDataBlockResult()`~~ (done)
