---
title: C++ Runtime Correctness Notes
scope: cpp-lib/tests
owner_repo: esnacc
entry_for:
  - C++ runtime behavior
  - runtime correctness tests
  - ROSE telemetry and shutdown semantics
purpose: Record intended C++ runtime semantics before tests or implementation are tightened.
read_when:
  - Changing cpp-lib runtime behavior, telemetry, shutdown, or decode-error handling
  - Adding or reviewing runtime correctness tests
related_docs:
  - ../../ReadMe.md
---

# Runtime Correctness Notes

This note records the intended semantics for selected `cpp-lib` runtime behaviors
before tightening tests or changing the implementation. The goal is to align the
runtime to the public API contract and to common operator expectations rather
than simply preserving whatever behavior exists today.

Primary reference points:
- `cpp-lib/include/SnaccROSEBase.h`
- `cpp-lib/include/SnaccROSEInterfaces.h`
- `cpp-lib/include/SnaccTelemetry.h`
- `cpp-lib/src/SnaccROSEBase.cpp`

## Summary

| Area | Current Behavior | Intended Behavior |
| --- | --- | --- |
| `StopProcessing()` | Completes pending operations, but does not block new work in practice | Shutdown must refuse new outbound work and stop inbound invoke/event dispatch |
| `iTimeout == 0` telemetry | Reported as `Outcome::UNHANDLED` with `WAIT_SKIPPED` | Treated as a successful fire-and-forget dispatch, not as a failure |
| Response payload decode after a valid reply envelope | Caller sees decode failure, telemetry still looks like remote result/error | Telemetry must primarily reflect the caller-visible outcome |
| `OnBinaryDataBlockResult()` decode failures | Logs and telemetry are emitted, but `OnRoseDecodeError()` parity is missing | Result-path decode failure handling should mirror `OnBinaryDataBlock()` |

## 1. `StopProcessing()` Shutdown Contract

### Public contract

`SnaccROSEBase` documents shutdown as a hard stop:

```152:156:cpp-lib/include/SnaccROSEBase.h
	/*! Shutdown.
		Call this function to stop processing any more Invokes.
		All pending operations will be completed and new function calls will be blocked.
		All Functions return a ROSE_TE_SHUTDOWN */
	void StopProcessing(bool bStop = true);
```

### Current behavior

The implementation only toggles `m_bProcessingAllowed` and completes pending
operations. In the current tree, the flag is not enforced by the send or receive
paths.

```330:338:cpp-lib/src/SnaccROSEBase.cpp
void SnaccROSEBase::StopProcessing(bool bStop /*= true*/)
{
	{
		std::lock_guard<std::mutex> guard(m_InternalProtectMutex);
		m_bProcessingAllowed = bStop ? false : true;
	}

	CompleteAllPendingOperations();
}
```

```346:352:cpp-lib/src/SnaccROSEBase.cpp
void SnaccROSEBase::CompleteAllPendingOperations()
{
	std::lock_guard<std::mutex> guard(m_InternalProtectMutex);

	for (auto it = m_PendingOperations.begin(); it != m_PendingOperations.end(); it++)
		it->second->CompleteOperation(ROSE_TE_SHUTDOWN, NULL);
}
```

`SendInvoke()` does not check `m_bProcessingAllowed` before creating a pending
operation, sending an invoke, or waiting for a response:

```1500:1506:cpp-lib/src/SnaccROSEBase.cpp
	auto& pendingOP = AddPendingOperation(pinvoke->invokeID, pinvoke->operationID, szResolvedOperationName);

	size_t stRequestData = 0;
	long lRoseResult = Send(pinvoke, szOperationName, ctx, &stRequestData);
	pendingOP.m_pTelemetry = SnaccTelemetryData::Create(SnaccTelemetryData::Direction::OUTBOUND, pinvoke->operationID, szResolvedOperationName, stRequestData, chronoCreated);

	if (lRoseResult == 0)
```

### Why this is inconsistent

- The public header promises that new calls are blocked after shutdown.
- The implementation only completes already pending work.
- Callers and operators cannot rely on shutdown to be a real safety boundary.

### Intended behavior

Treat `StopProcessing(true)` as a real runtime shutdown gate:

1. New outbound invokes and events must fail fast with `ROSE_TE_SHUTDOWN`.
2. Pending operations must still be completed with `ROSE_TE_SHUTDOWN`.
3. New inbound invokes and inbound events must not be dispatched to application
   handlers while shutdown is active.
4. Late inbound responses that arrive after pending operations were force-
   completed may be ignored, but they must not resurrect completed work.
5. `StopProcessing(false)` may re-enable the runtime if that is part of the
   supported API, but the behavior should be explicit and documented as a
   restart of processing, not an incidental side effect.

### Tests that should enforce this later

- `SendInvoke()` after shutdown returns `ROSE_TE_SHUTDOWN` without creating a
  new pending operation.
- `SendEvent()` after shutdown returns `ROSE_TE_SHUTDOWN`.
- An inbound invoke fed through `OnBinaryDataBlock()` during shutdown does not
  reach application handlers.
- Existing pending operations complete with `ROSE_TE_SHUTDOWN`.
- If restart remains supported, `StopProcessing(false)` has one explicit test
  proving the intended resumed behavior.

### Likely code paths to change

- `SnaccROSEBase::SendInvoke()`
- `SnaccROSEBase::SendEvent()`
- potentially `SnaccROSEBase::Send()` as the shared outbound choke point
- `SnaccROSEBase::OnBinaryDataBlock()`
- `SnaccROSEBase::OnBinaryDataBlockResult()`
- possibly `SnaccROSEBase::OnROSEMessage()` for central inbound gating

## 2. Fire-And-Forget Invoke Telemetry (`iTimeout == 0`)

### Current behavior

When the caller explicitly chooses not to wait, `SendInvoke()` treats the call
as locally successful and stores `ROSE_NOERROR`:

```1529:1533:cpp-lib/src/SnaccROSEBase.cpp
		else
		{
			lRoseResult = ROSE_NOERROR;
			pendingOP.m_lRoseResult = lRoseResult;
		}
```

Later, telemetry finalization sees "no answer message" plus `ROSE_NOERROR` and
records the lifecycle as `Outcome::UNHANDLED` with `Reason::WAIT_SKIPPED`.

```299:306:cpp-lib/src/SnaccROSEBase.cpp
	if (m_lRoseResult != ROSE_NOERROR)
	{
		m_pTelemetry->finalize(SnaccTelemetryData::Outcome::UNHANDLED, GetOutboundUnhandledStageFromResult(m_lRoseResult), GetUnhandledReasonFromResult(m_lRoseResult), m_lRoseResult, std::nullopt, std::move(pctx));
		return;
	}

	m_pTelemetry->finalize(SnaccTelemetryData::Outcome::UNHANDLED, SnaccTelemetryData::Stage::OUTBOUND_WAIT, SnaccTelemetryData::Reason::WAIT_SKIPPED, m_lRoseResult, std::nullopt, std::move(pctx));
```

### Why this is inconsistent

- `WAIT_SKIPPED` is intentional caller behavior, not a failure condition.
- `Outcome::UNHANDLED` is otherwise used for abnormal paths such as transport,
  timeout, shutdown, invalid response, and decode failure.
- A caller intentionally using fire-and-forget currently looks like a failure in
  telemetry aggregations.

### Intended behavior

Fire-and-forget should be treated as a successful local dispatch of an invoke
whose remote outcome is intentionally unknown to this runtime instance.

Preferred model:

1. Introduce a dedicated successful outcome such as `DISPATCHED` or `PENDING`
   for "sent, not awaited".
2. Keep `Reason::WAIT_SKIPPED` to preserve the explicit cause.
3. Keep `Stage::OUTBOUND_WAIT` or rename it later if a more precise stage model
   is introduced.

If the enum surface cannot be changed immediately, the minimum semantic rule
should still be:

- do not classify `WAIT_SKIPPED` under the same top-level failure bucket used
  for true unhandled faults.

### Tests that should enforce this later

- `SendInvoke(..., iTimeout = 0)` produces a non-failure telemetry outcome.
- The telemetry reason remains `WAIT_SKIPPED`.
- The runtime reports local send success and does not fabricate a remote result.

### Likely code paths to change

- `SnaccROSEPendingOperation::FinalizeTelemetry()`
- `SnaccTelemetryData::Outcome` in `cpp-lib/include/SnaccTelemetry.h`
- any debug-text helpers in `cpp-lib/src/SnaccTelemetry.cpp`

## 3. Outbound Telemetry After Response Payload Decode Failure

### Current behavior

`HandleInvokeResult()` can turn a reply that had a valid envelope into a final
caller-visible decode failure when the embedded result or error payload cannot
be decoded.

```1745:1747:cpp-lib/src/SnaccROSEBase.cpp
	lRoseResult = DecodeResponse(pResponseMsg, &pResult, &pError, ctx);

	if (lRoseResult == ROSE_NOERROR)
```

```1787:1796:cpp-lib/src/SnaccROSEBase.cpp
			catch (const SnaccException& ex)
			{
				SJson::Value err;
				err["exception"] = ex.what();
				err["method"] = __FUNCTION__;
				err["error"] = (int)ex.m_errorCode;
				std::string strError = getPrettyPrinted(err);
				PrintJSONToLog(false, true, nullptr, strError.c_str(), strError.length());

				lRoseResult = ROSE_RE_DECODE_FAILED;
			}
```

But outbound telemetry finalization uses the received envelope kind stored in
`m_pAnswerMessage`, not the final caller-visible result returned after payload
decode:

```280:292:cpp-lib/src/SnaccROSEBase.cpp
	if (m_pAnswerMessage)
	{
		switch (m_pAnswerMessage->choiceId)
		{
			case ROSEMessage::resultCid:
				m_pTelemetry->finalize(SnaccTelemetryData::Outcome::RESULT, SnaccTelemetryData::Stage::OUTBOUND_WAIT, SnaccTelemetryData::Reason::REMOTE_RESULT, m_lRoseResult, m_stResponseData, pctx);
				break;
			case ROSEMessage::errorCid:
				m_pTelemetry->finalize(SnaccTelemetryData::Outcome::ERR, SnaccTelemetryData::Stage::OUTBOUND_WAIT, SnaccTelemetryData::Reason::REMOTE_ERROR, m_lRoseResult, m_stResponseData, pctx);
				break;
```

### Why this is inconsistent

- The public caller gets a decode failure.
- Telemetry still says the operation completed as a clean remote result or
  remote error.
- This makes telemetry disagree with the API result that application code sees.

### Intended behavior

For outbound invoke telemetry, the final caller-visible result must be the
authoritative classification.

That means:

1. If the response envelope was received but payload decode fails, telemetry
   should finalize as `Outcome::UNHANDLED`.
2. The reason should be `DECODE_FAILED`.
3. The result code should be `ROSE_RE_DECODE_FAILED`.
4. If later needed, the runtime may keep a secondary field for "remote envelope
   kind received", but that must not override the primary outcome.

### Tests that should enforce this later

- A malformed result payload yields `ROSE_RE_DECODE_FAILED` to the caller and
  telemetry `UNHANDLED + DECODE_FAILED`.
- A malformed error payload yields the same shape.
- The telemetry record must no longer be `REMOTE_RESULT` or `REMOTE_ERROR` in
  those cases.

### Likely code paths to change

- `SnaccROSEBase::HandleInvokeResult()`
- `SnaccROSEPendingOperation::FinalizeTelemetry()`
- potentially `SnaccROSEPendingOperation::CompleteOperation()` if more state is
  needed than envelope kind plus raw result code

## 4. `OnBinaryDataBlockResult()` Decode-Error Hook and Logging Parity

### Current behavior

`OnBinaryDataBlock()` invokes `OnRoseDecodeError()` on BER, JSON decode, JSON
parse, and unknown-encoding failures. It also updates `bLogTransportData` with
the actual return value from `LogTransportData()` before calling the hook.

```780:792:cpp-lib/src/SnaccROSEBase.cpp
					catch (const SnaccException& ex)
					{
						if (bLogTransportData)
							bLogTransportData = LogTransportData(false, m_eTransportEncoding, nullptr, lpBytes, ulSize, nullptr, nullptr);

						SJson::Value error;
						error["exception"] = ex.what();
						error["method"] = __FUNCTION__;
						error["error"] = (int)ex.m_errorCode;
						std::string strError = getPrettyPrinted(error);
						PrintJSONToLog(false, true, nullptr, strError.c_str(), strError.length());

						OnRoseDecodeError(bLogTransportData, SNACC::TransportEncoding::BER, lpBytes, ulSize, ex.what());
```

`OnBinaryDataBlockResult()` performs similar logging and telemetry work, but it
never calls `OnRoseDecodeError()` in the corresponding failure cases.

```493:503:cpp-lib/src/SnaccROSEBase.cpp
					catch (const SnaccException& ex)
					{
						if (bLogTransportData)
							LogTransportData(false, m_eTransportEncoding, nullptr, lpBytes, lSize, nullptr, nullptr);

						SJson::Value error;
						error["exception"] = ex.what();
						error["method"] = __FUNCTION__;
						error["error"] = (int)ex.m_errorCode;
						std::string strError = getPrettyPrinted(error);
						PrintJSONToLog(false, true, nullptr, strError.c_str(), strError.length());
```

### Why this is inconsistent

- `OnRoseDecodeError()` is the dedicated extension point for decode failures.
- Result-path decode failures are still decode failures, but the override hook is
  bypassed there.
- Consumers overriding that hook cannot get symmetric behavior for the two
  inbound entry points.
- The `bAlreadyTransportLogged` signal is only maintained on the
  `OnBinaryDataBlock()` side.

### Intended behavior

`OnBinaryDataBlockResult()` should mirror `OnBinaryDataBlock()` for decode
failure handling unless there is an explicitly documented reason not to.

That means:

1. invoke `OnRoseDecodeError()` for comparable decode-failure classes
2. pass the real `bAlreadyTransportLogged` state derived from
   `LogTransportData()`
3. preserve any legitimate differences in reply-specific protocol handling, but
   not differences in error-hook visibility

### Tests that should enforce this later

- A test endpoint overriding `OnRoseDecodeError()` sees BER result-path decode
  failures.
- The same endpoint sees JSON result-path decode failures.
- The hook receives the same "already transport logged" semantics as the normal
  inbound path.

### Likely code paths to change

- `SnaccROSEBase::OnBinaryDataBlockResult()`
- possibly shared helper extraction between `OnBinaryDataBlock()` and
  `OnBinaryDataBlockResult()` to avoid drift

## Recommended Follow-Up Order

1. Fix and test the shutdown contract first, because it affects public API
   guarantees and can change how the rest of the runtime is exercised.
2. Fix fire-and-forget telemetry classification so dashboards stop reporting
   intentional behavior as a failure.
3. Fix response-payload decode telemetry so caller-visible failures and
   telemetry agree.
4. Bring `OnBinaryDataBlockResult()` decode-error hooks into parity with
   `OnBinaryDataBlock()` to avoid continuing divergence.
