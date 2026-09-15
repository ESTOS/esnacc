---
title: C++ Runtime Correctness Notes
scope: cpp-lib/tests
owner_repo: esnacc
entry_for:
  - C++ runtime behavior
  - runtime correctness tests
  - ROSE telemetry and shutdown semantics
purpose: Record intended ROSE runtime semantics (C++ primary; TypeScript and future Kotlin/Swift must match) and implementation status for areas exercised by runtime tests.
read_when:
  - Changing cpp-lib runtime behavior, telemetry, shutdown, or decode-error handling
  - Adding or reviewing runtime correctness tests
  - Aligning TypeScript glue or future Kotlin/Swift ROSE runtimes with C++
related_docs:
  - ../../AGENTS.md
  - ../../ReadMe.md
  - ../../.cursor/rules/rose-cross-language-parity.mdc
---

# Runtime Correctness Notes

This note records the intended semantics for selected ROSE runtime behaviors
and whether the current tree implements them. C++ tests are the primary executable
spec today; TypeScript glue tests and future Kotlin/Swift runtimes must implement
the same concepts (see `.cursor/rules/rose-cross-language-parity.mdc`). New sections
should include a **Cross-language parity** bullet naming C++, TS, and planned surfaces.

Primary reference points:
- `cpp-lib/include/SnaccROSEBase.h`
- `cpp-lib/include/SnaccROSEInterfaces.h`
- `cpp-lib/include/SnaccTelemetry.h`
- `cpp-lib/src/SnaccROSEBase.cpp`

## Summary

| Area | Status | Semantics | Primary tests |
| --- | --- | --- | --- |
| Transport-session ROSE gate | Implemented | `PauseRoseProcessing` / `ResumeRoseProcessing` | `PublicApiRuntimeTest.PauseRoseProcessingBlocks*`, `LifecycleRuntimeTest.PauseRoseProcessing*` |
| Fire-and-forget (`iTimeout == 0`) telemetry | Implemented | `Outcome::DISPATCHED` + `Reason::WAIT_SKIPPED`, not `UNHANDLED` | `TelemetryRuntimeTest.WaitSkippedTelemetry*` |
| Response payload decode telemetry | Implemented | Caller-visible `ROSE_RE_DECODE_FAILED` drives `UNHANDLED` + `DECODE_FAILED`, not envelope kind | `TelemetryRuntimeTest.*PayloadDecodeFailureTelemetry*` |
| Inbound decode failures and ROSE rejects | Implemented | Garbage wire silent; targeted reject only after envelope decode | `InvokeContextRuntimeTest.UnparsableInbound*`, section 5 |
| `OnBinaryDataBlockResult()` decode-error hooks | Implemented | `OnRoseDecodeError()` and `bAlreadyTransportLogged` parity with `OnBinaryDataBlock()` | `PublicApiSmokeTest.OnBinaryDataBlockResultDecodeErrorsInvokeHook*` |
| Inbound `ROSEMessage` ownership | Implemented | `unique_ptr` at decode sites; `std::move` through dispatch | Section 6; `InvokeContextRuntimeTest` suite |
| Outbound encode / `Send()` ownership | Implemented | RAII encode helpers detach borrowed arms on scope exit (including encode exceptions) | Section 7; outbound encode-failure tests in `InvokeContextRuntimeTest` |

## 1. Transport-session ROSE processing gate (`PauseRoseProcessing` / `ResumeRoseProcessing`)

### Status: implemented

### Public contract

Preferred API on `SnaccROSEBase`:

- `PauseRoseProcessing()` — end the current transport ROSE session (block new
  work, complete pending ops with `ROSE_TE_SHUTDOWN`).
- `ResumeRoseProcessing()` — reopen processing on the same stub instance (e.g.
  after transport reconnect). Does not resurrect ops from the prior session.

### Intended behavior

Treat `PauseRoseProcessing()` as a transport-session shutdown gate:

1. New outbound invokes and events must fail fast with `ROSE_TE_SHUTDOWN`.
2. Pending operations must still be completed with `ROSE_TE_SHUTDOWN`.
3. New inbound invokes and inbound events must not be dispatched to application
   handlers while shutdown is active.
4. Late inbound responses that arrive after pending operations were force-
   completed may be ignored, but they must not resurrect completed work.
5. `ResumeRoseProcessing()` re-enables processing; callers must invoke it
   explicitly when starting a new transport session on a reused stub (pair with
   `PauseRoseProcessing()` on disconnect).

### Implementation

`PauseRoseProcessing()` clears `m_bProcessingAllowed` and completes all pending
operations with `ROSE_TE_SHUTDOWN`. `ResumeRoseProcessing()` sets
`m_bProcessingAllowed` back to true without touching pending operations.

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

- `PublicApiRuntimeTest.PauseRoseProcessingBlocksNewOutboundInvokesAndEvents`
- `PublicApiRuntimeTest.PauseRoseProcessingBlocksInboundDispatchUntilResumed`
- `LifecycleRuntimeTest.PauseRoseProcessingCompletesPendingInvokeWithShutdownBer`
- `LifecycleRuntimeTest.PauseRoseProcessingCompletesPendingInvokeWithShutdownJson`
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
   `CreateOutboundInvokeContext()` (no name on the context). Pass
   `std::optional<unsigned int>` to set invoke timeout in the same call; omit it for the
   connection default (unset). Generated deprecated
   outbound stubs default to `CreateInvokeContext(SnaccInvokeContextInit(OUTBOUND,
   invoke, operationName))` so `SNACCDeprecated::DeprecatedASN1Method` can read
   `OperationName()` on the context.
2. **Lookup table (UCAAS-1485):** `SnaccRoseOperationLookup` holds operation id/name/interface
   mappings for one listener context. Fill at startup via
   `ENetUC_*ROSE::RegisterOperations(lookup)` (static; no stub instances required),
   then call `Seal()`. Mounting a generated `*ROSE` component does not register operations. `SnaccROSEBase` borrows a const lookup reference for the stub
   lifetime; lookup after seal needs no locking. Outbound stub literals remain authoritative;
   lookup by operationID is the inbound parachute when only the id is known.
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

## 9. ROSEInvoke Wire Timeout (BUILDSYS-645)

### Status: implemented (7.0.18)

### Contract

1. **ASN.1:** optional `ROSEInvoke.timeout` (`[4] IMPLICIT INTEGER`) — relative deadline in
   **milliseconds**. Evaluated on the **server at receive time**:
   `deadline = T_receive + timeout`. If the server cannot process the invoke in time (e.g.
   queued behind resource limits) and the deadline has passed after receipt, product code may
   discard the work without executing the handler and without sending a late
   result/error/reject for that expired invoke.
2. **Outbound encoding:** when `InvokeTimeout()` / `invokeTimeout()` is set and **> 0** on a
   non-event invoke (`invokeID != 99999`), the runtime sets `ROSEInvoke.timeout` before encode.
   Omit the field when unset (connection default), when set to `0` (fire-and-forget), or for events.
3. **Inbound context:** when the wire field is present, copy into `SnaccInvokeContext` /
   `ReceiveInvokeContext` via `InvokeTimeout()` / `invokeTimeout()`; absent field stays unset.
4. **C++ / TypeScript parity:** `SetInvokeTimeout(unsigned)` / `setInvokeTimeout(number)`,
   `ClearInvokeTimeout()` / `clearInvokeTimeout()`, `std::optional<unsigned int> InvokeTimeout()`
   / `invokeTimeout(): number | undefined`, optional constructor param `invokeTimeoutMs` on
   outbound contexts (`CreateOutboundInvokeContext()` / `SendInvokeContext` partial args).
   Three states: unset → connection default; `0` → fire-and-forget; `> 0` → explicit ms (+ wire).
5. **Backward compatibility:** peers that omit `timeout` behave as today.

## 10. Cross-language runtime test matrix

Executable spec today: `cpp-lib/tests/` (~170 cases). TypeScript glue parity:
`compiler/back-ends/ts-gen/tests/` (run via `scripts/run_gluecode_tests.sh`). Integration smoke:
`samples/ts-microservice/node-client/`.

| C++ suite / area | TS glue tests | Status |
| --- | --- | --- |
| `module_registry_tests` | `TSASN1Base.registry.test.ts` | **Aligned** (static registration nuances C++-only) |
| `TSModuleCapabilities` / negotiate helper | `TSModuleCapabilities.test.ts` | **Aligned** |
| `client_invoke_block_policy_tests` | `TSASN1Base.invokeBlockPolicy.test.ts` | **Aligned** (stub gate; loopback runtime C++-only) |
| `server_invoke_block_policy_tests` | `TSASN1Base.invokeBlockPolicy.test.ts` | **Partial** (subscription path only) |
| `rose_session_subscription_tests` | `TSASN1Base.roseSessionSubscription.test.ts` | **Partial** (`SnaccROSEComponent` C++-only) |
| Pause / shutdown gate | `TSASN1Base.pauseRoseProcessing.test.ts` | **Partial** (3 scenarios; lifecycle/async overlap C++-only) |
| `InvokeWireTimeoutTest` / outbound encode | `TSROSEBase.invokeTimeout.test.ts` | **Aligned** (unit + encode path) |
| `InvokeContextInitTest` | `TSInvokeContext.init.test.ts` | **Partial** (wire `operationName` vs lookup differs from C++) |
| `InvokeContextRuntimeTest` (loopback) | `TSCallFlow.loopback.test.ts` (subset) | **Partial** — full orphan/malformed matrix C++-only |
| `OutboundWireInvokeTimeoutReachesInboundHandler*` | — | **Blocked** — TS `receiveHandleROSEMessage` does not copy wire `timeout` into context yet |
| `call_flow_tests` | `TSCallFlow.loopback.test.ts` + integration smoke | **Aligned** (loopback; integration smoke separate) |
| `logical_failure_tests` | `TSLogicalFailure.loopback.test.ts` | **Partial** — TS returns wire `InvokeProblemenum` (not C++ `ROSE_REJECT_*`); BER loopback loses `invokeProblem=1` on reject round-trip (spec failure, see below) |
| `transport_failure_tests` | `TSTransportFailure.loopback.test.ts` | **Partial** (timeout code `requestTimedOut` vs `ROSE_TE_TIMEOUT`) |
| `async_invoke_tests` | — | **Blocked** — Promise model; no `SetAsyncCompletion` surface |
| `logging_tests` | — | **Blocked** — no `ConfigureFileLogging` / transport log-flag API in TS |
| `telemetry_tests` | — | **Blocked** — no telemetry subsystem in TS glue |
| `lifecycle_tests` | — | **Blocked** — partial overlap with pause tests |
| `public_api_tests` (decode hooks, file log) | — | **Blocked** — C++-specific APIs |

**Known TS/C++ behavioral difference (documented, not a test failure):** inbound
`SnaccInvokeContext` resolves `OperationName()` from operationID lookup and **ignores** wire
`operationName` when operationID is set. TypeScript `receiveHandleROSEMessage` keeps wire
`operationName` when present (`TSInvokeContext.init.test.ts`).

**Known TS glue spec failure (BER reject round-trip):** loopback delivery of server
`ROSEReject` with `invokeProblem = unrecognisedOperation` (1) decodes as an empty reject
(`AsnInvokeProblem` value 502 / `emptyRejectMessage`). JSON path and BER rejects with
`invokeProblem = mistypedArgument` (2) round-trip correctly. C++ `logical_failure_tests`
pass BER for unknown operation and missing handler. Fix target: `RejectProblem` BER
encode/decode in `SNACCROSE_Converter.ts` (not test harness).

## 11. TypeScript parity roadmap (implementation blocks)

### Block A — Done in this pass

- Glue tests for invoke timeout, invoke-context init, remote-capability clear/query.
- Shared capture transport (`compiler/back-ends/ts-gen/tests/support/rose_test_transport.ts`).
- Matrix above in this file.

### Block B — Loopback harness (done in 7.0.18 branch)

1. `compiler/back-ends/ts-gen/tests/support/sample_runtime_harness.ts` — loopback transport + sample modules.
2. Sample stubs copied in `scripts/run_gluecode_tests.*` (`ENetUC_Settings_Manager`, `ENetUC_Event_Manager`).
3. `TSCallFlow.loopback.test.ts`, `TSLogicalFailure.loopback.test.ts`, `TSTransportFailure.loopback.test.ts`.
4. CMake `ctest` target `snacc-ts-glue` (requires `bash` + `snacc-ts-glue-prepare`).

### Block C — Telemetry in TypeScript (largest product gap)

Port C++ `SnaccTelemetry.h` / `SnaccTelemetryData` / `SnaccTelemetryCallback` to glue:

| C++ | Proposed TS |
| --- | --- |
| `SnaccTelemetryData::Create` / `CreateFinalized` | `SnaccTelemetryData.create` / `createFinalized` |
| `Direction`, `Stage`, `Outcome`, `Reason` enums | Same names, TS enums |
| `OnInvokeProcessed` callback | `onInvokeProcessed` on transport or logger sink |
| `PrepareForTelemetry()` on invoke context | Clone context for telemetry retention (needs pluggable context — Block D) |
| Outbound/inbound wait, dispatch, decode-failure paths | Hook at same sites as `SnaccROSEBase.cpp` |

Then port `telemetry_tests.cpp` → `TSTelemetry.test.ts` (outcome/reason assertions, not C++ ownership).

### Block D — Invoke context product parity

- Pluggable `createInvokeContext()` on transport (parity with C++ `CreateInvokeContext`).
- `receiveHandleROSEMessage`: copy wire `timeout` into `ReceiveInvokeContext` (unblocks inbound handler timeout E2E).
- Align inbound `operationName` resolution with C++ (lookup wins over wire) **or** lock TS behavior in spec.

### Block E — Logging API

- `configureFileLogging`, `OnBinaryDataBlock` log flags, decode-error hooks → port `logging_tests` / `public_api_tests` logging cases.

### Block F — Kotlin / Swift

When Tier A ROSE glue lands, reuse scenario names from this matrix; implement harness per language.

## Recommended Follow-Up Order

1. ~~Shutdown contract~~ (done)
2. ~~Fire-and-forget telemetry classification~~ (done)
3. ~~Response-payload decode telemetry~~ (done)
4. ~~Inbound decode reject policy and `OnBinaryDataBlockResult()` reject cleanup~~ (done)
5. ~~Inbound and outbound `ROSEMessage` ownership~~ (done)
6. Push branch and open PR for UCAAS-1446.
7. ~~Extract shared decode-failure helpers between `OnBinaryDataBlock()` and `OnBinaryDataBlockResult()`~~ (done)
