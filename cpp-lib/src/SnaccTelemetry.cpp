#include "../include/SnaccTelemetry.h"
#include "snacc-assert.h"
#include <stdexcept>

const char* SnaccTelemetryData::GetDebugText(Direction direction)
{
	switch (direction)
	{
		case Direction::INBOUND:
			return "INBOUND";
		case Direction::OUTBOUND:
			return "OUTBOUND";
		default:
			ASSERT(0);
			return "INVALID_DIRECTION";
	}
}

const char* SnaccTelemetryData::GetDebugText(Outcome outcome)
{
	switch (outcome)
	{
		case Outcome::RESULT:
			return "RESULT";
		case Outcome::ERR:
			return "ERR";
		case Outcome::REJECT:
			return "REJECT";
		case Outcome::EVENT:
			return "EVENT";
		case Outcome::UNHANDLED:
			return "UNHANDLED";
		default:
			ASSERT(0);
			return "INVALID_OUTCOME";
	}
}

const char* SnaccTelemetryData::GetDebugText(Stage stage)
{
	switch (stage)
	{
		case Stage::UNKNOWN:
			return "UNKNOWN";
		case Stage::INBOUND_DECODE:
			return "INBOUND_DECODE";
		case Stage::INBOUND_INVOKE:
			return "INBOUND_INVOKE";
		case Stage::INBOUND_RESPONSE:
			return "INBOUND_RESPONSE";
		case Stage::OUTBOUND_SEND:
			return "OUTBOUND_SEND";
		case Stage::OUTBOUND_WAIT:
			return "OUTBOUND_WAIT";
		default:
			ASSERT(0);
			return "INVALID_STAGE";
	}
}

const char* SnaccTelemetryData::GetDebugText(Reason reason)
{
	switch (reason)
	{
		case Reason::NONE:
			return "NONE";
		case Reason::LOCAL_RESULT:
			return "LOCAL_RESULT";
		case Reason::LOCAL_ERROR:
			return "LOCAL_ERROR";
		case Reason::LOCAL_REJECT:
			return "LOCAL_REJECT";
		case Reason::LOCAL_EVENT:
			return "LOCAL_EVENT";
		case Reason::REMOTE_RESULT:
			return "REMOTE_RESULT";
		case Reason::REMOTE_ERROR:
			return "REMOTE_ERROR";
		case Reason::REMOTE_REJECT:
			return "REMOTE_REJECT";
		case Reason::DECODE_FAILED:
			return "DECODE_FAILED";
		case Reason::ENCODE_FAILED:
			return "ENCODE_FAILED";
		case Reason::SEND_FAILED:
			return "SEND_FAILED";
		case Reason::TIMEOUT:
			return "TIMEOUT";
		case Reason::SHUTDOWN:
			return "SHUTDOWN";
		case Reason::INVALID_RESPONSE:
			return "INVALID_RESPONSE";
		case Reason::WAIT_SKIPPED:
			return "WAIT_SKIPPED";
		case Reason::INVOKE_EXCEPTION:
			return "INVOKE_EXCEPTION";
		case Reason::UNKNOWN_FAILURE:
			return "UNKNOWN_FAILURE";
		default:
			ASSERT(0);
			return "INVALID_REASON";
	}
}

std::shared_ptr<SnaccTelemetryData> SnaccTelemetryData::Create(Direction direction, unsigned int uiOperationID, const char* szOperationName, size_t stRequestData, std::chrono::steady_clock::time_point chronoCreated /*= std::chrono::steady_clock::now()*/)
{
	return std::shared_ptr<SnaccTelemetryData>(new SnaccTelemetryData(direction, uiOperationID, szOperationName, stRequestData, chronoCreated));
}

std::shared_ptr<SnaccTelemetryData> SnaccTelemetryData::CreateFinalized(Direction direction, unsigned int uiOperationID, const char* szOperationName, size_t stRequestData, Outcome outcome, Stage stage, Reason reason, long lRoseResult, std::optional<size_t> stResponseData /*= std::nullopt*/, std::shared_ptr<SnaccInvokeContext> pctx /*= {}*/, std::chrono::steady_clock::time_point chronoCreated /*= std::chrono::steady_clock::now()*/)
{
	auto telemetry = Create(direction, uiOperationID, szOperationName, stRequestData, chronoCreated);
	telemetry->finalize(outcome, stage, reason, lRoseResult, stResponseData, pctx);
	return telemetry;
}

void SnaccTelemetryData::finalize(Outcome outcome, Stage stage, Reason reason, std::optional<long> lRoseResult /*= std::nullopt*/, std::optional<size_t> stResponseData /*= std::nullopt*/, std::shared_ptr<SnaccInvokeContext> pctx /*= {}*/)
{
	ASSERT(!m_bFinalized);
	if (m_bFinalized)
		throw std::logic_error("SnaccTelemetryData::finalize may only be called once");

	std::shared_ptr<SnaccInvokeContext> pTelemetryctx;
	if (pctx)
	{
		pTelemetryctx = pctx->Clone();
		pTelemetryctx->PrepareForTelemetry();
	}

	m_Duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_ChronoCreated);
	m_Outcome = outcome;
	m_Stage = stage;
	m_Reason = reason;
	m_lRoseResult = lRoseResult;
	m_stResponseData = stResponseData;
	m_pctx = std::move(pTelemetryctx);
	m_bFinalized = true;
}

SnaccTelemetryData::SnaccTelemetryData(Direction direction, unsigned int uiOperationID, const char* szOperationName, size_t stRequestData, std::chrono::steady_clock::time_point chronoCreated)
	: m_Direction(direction),
	  m_uiOperationID(uiOperationID),
	  m_strOperationName(szOperationName ? szOperationName : ""),
	  m_stRequestData(stRequestData),
	  m_ChronoCreated(chronoCreated)
{
}
