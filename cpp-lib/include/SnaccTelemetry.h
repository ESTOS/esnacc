#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include "SnaccROSEInterfaces.h"

/*! Switches for every telemetry stream this library collects.
	Defaults keep today's behavior: invoke records stay on when a callback is installed, and worker-pool sums stay off until a caller turns them on.
	Add a field here when a new stream appears. ConfigureTelemetry keeps the same signature. */
class TelemetryConfig
{
public:
	/*! ROSE invoke records delivered to the installed callback.
		True by default so an existing callback keeps receiving records.
		False skips building a record and skips cloning the invoke context. A record already started is still delivered. */
	bool m_bRoseTelemetryEnabled{true};
	/*! Counters and histograms in ROSE worker pools, copied out by SnaccWorkerPools::Health.
		False by default. Health() then returns no rows.
		Each edge clears the cumulative sums under the pool lock before the next count or copy. Queue depth and running threads stay. */
	bool m_bRoseWorkerPoolTelemetryEnabled{false};
};

/*! Applies every telemetry switch in one call. The new value replaces the previous one for the whole process.
	Invoke records have no sums to clear. Worker-pool sums are cleared on each edge under that pool's lock.
	@param config The full set of switches. Fields left at their defaults keep that stream's default. */
void ConfigureTelemetry(const TelemetryConfig& config);

/*! True when invoke records may be built. Default true. The callback pointer is a separate check. */
bool IsRoseTelemetryEnabled();

/*! True when ROSE worker pools count and Health() returns rows. Default false. */
bool IsRoseWorkerPoolTelemetryEnabled();

/*! Generation of the worker-pool switch. Pools compare it under their own lock and clear sums when it differs. */
std::uint64_t RoseWorkerPoolTelemetryEpoch();

/**
 * Carries telemetry for one ROSE request lifecycle.
 *
 * The object can represent inbound or outbound traffic and covers both invokes and
 * events. It is created when the library starts handling or sending a request, invoke
 * telemetry is enabled, and a telemetry callback is installed. It is finalized once the
 * local lifecycle has completed. When no callback is installed, or invoke telemetry is
 * switched off, the runtime does not create the object.
 *
 * The request metadata captured at construction time is immutable. Completion data
 * such as duration, response size, outcome, and the final ROSE result code are
 * populated by `finalize()`.
 *
 * Duration always represents the local lifecycle time inside this library instance.
 * For inbound requests it is the handling time, for outbound invokes it usually
 * includes waiting for the response, and for outbound events it only reflects the
 * local send/encode time.
 *
 * The object is intended to be shared with telemetry consumers as
 * `std::shared_ptr<const SnaccTelemetryData>` so callbacks can inspect the data
 * without mutating it.
 */
class SnaccTelemetryData
{
public:
	enum class Direction
	{
		INBOUND = 0,
		OUTBOUND = 1
	};

	enum class Outcome
	{
		// The invoked method returned a normal result.
		RESULT = 0,
		// The invoked method returned an application error.
		ERR = 1,
		// The invoke could not be handled and therefore resulted in a reject.
		REJECT = 2,
		// The inbound message was an event and therefore intentionally produced no response.
		EVENT = 3,
		// The local lifecycle completed abnormally (exception, unable to decode, unable to encode, sending failed etc.)
		UNHANDLED = 4,
		// The request was successfully sent, but the caller intentionally skipped waiting for a reply.
		DISPATCHED = 5
	};

	// Stage identifies the lifecycle area in which the telemetry record was finalized.
	enum class Stage
	{
		// Fallback value in case the stage could not be determined.
		UNKNOWN = 0,
		// Finalized while decoding an inbound transport payload.
		INBOUND_DECODE = 1,
		// Finalized while handling an inbound invoke or event.
		INBOUND_INVOKE = 2,
		// Finalized for an inbound result/error/reject message.
		INBOUND_RESPONSE = 3,
		// Finalized while encoding or sending an outbound message.
		OUTBOUND_SEND = 4,
		// Finalized while waiting for the reply to an outbound invoke.
		OUTBOUND_WAIT = 5
	};

	// Reason captures the concrete condition that led to the final outcome.
	enum class Reason
	{
		// No more specific reason was recorded.
		NONE = 0,
		// The local invoke handler produced a normal result.
		LOCAL_RESULT = 1,
		// The local invoke handler produced an application error.
		LOCAL_ERROR = 2,
		// The local side generated and successfully sent a reject.
		LOCAL_REJECT = 3,
		// The local side handled or sent an event without expecting a response.
		LOCAL_EVENT = 4,
		// A normal result message was received from the remote side.
		REMOTE_RESULT = 5,
		// An application error message was received from the remote side.
		REMOTE_ERROR = 6,
		// A reject message was received from the remote side.
		REMOTE_REJECT = 7,
		// The payload could not be decoded.
		DECODE_FAILED = 8,
		// A response could not be encoded.
		ENCODE_FAILED = 9,
		// Sending the payload through the transport failed.
		SEND_FAILED = 10,
		// Waiting for an outbound invoke response timed out.
		TIMEOUT = 11,
		// The lifecycle ended because processing was stopped.
		SHUTDOWN = 12,
		// The received response message did not match the expected shape.
		INVALID_RESPONSE = 13,
		// Waiting for a reply was intentionally skipped.
		WAIT_SKIPPED = 14,
		// The local invoke handler raised an exception and the stub replied with a reject.
		INVOKE_EXCEPTION = 15,
		// The lifecycle ended around a protocol-level reject such as unknown operation
		// or an argument/dispatch mismatch.
		REJECT_PROTOCOL = 16,
		// The lifecycle ended around a session or connection-security precondition reject.
		REJECT_SESSION = 17,
		// The lifecycle ended around an authentication reject.
		REJECT_AUTHENTICATION = 18,
		// Fallback value in case the exact failure reason could not be classified.
		UNKNOWN_FAILURE = 19,
		// Outbound send was skipped because invoke block policy rejected the operation.
		OUTBOUND_BLOCKED = 20
	};

	// Returns a short debug-friendly text for the enum value.
	static const char* GetDebugText(Direction direction);
	static const char* GetDebugText(Outcome outcome);
	static const char* GetDebugText(Stage stage);
	static const char* GetDebugText(Reason reason);

	// Creates the telemetry object in shared ownership so it can be forwarded safely.
	static std::shared_ptr<SnaccTelemetryData> Create(Direction direction, unsigned int uiOperationID, const char* szOperationName, size_t stRequestData, std::chrono::steady_clock::time_point chronoCreated = std::chrono::steady_clock::now());
	static std::shared_ptr<SnaccTelemetryData> CreateFinalized(Direction direction, unsigned int uiOperationID, const char* szOperationName, size_t stRequestData, Outcome outcome, Stage stage, Reason reason, long lRoseResult, std::optional<size_t> stResponseData = std::nullopt, std::shared_ptr<SnaccInvokeContext> pctx = {}, std::chrono::steady_clock::time_point chronoCreated = std::chrono::steady_clock::now());

	// Finalizes the telemetry data once the local request lifecycle has completed.
	// For events pass Outcome::EVENT and leave stResponseData empty.
	void finalize(Outcome outcome, Stage stage, Reason reason, std::optional<long> lRoseResult = std::nullopt, std::optional<size_t> stResponseData = std::nullopt, std::shared_ptr<SnaccInvokeContext> pctx = {});

	// Immutable request metadata captured when the lifecycle starts.
	const Direction m_Direction{};
	const unsigned int m_uiOperationID{};
	const std::string m_strOperationName{};
	const size_t m_stRequestData{};
	const std::chrono::steady_clock::time_point m_ChronoCreated;

	// Outcome data filled once processing completes.
	std::chrono::milliseconds m_Duration{};
	std::optional<size_t> m_stResponseData{};
	std::shared_ptr<const SnaccInvokeContext> m_pctx{};
	Outcome m_Outcome{};
	Stage m_Stage{};
	Reason m_Reason{};
	std::optional<long> m_lRoseResult{};

private:
	bool m_bFinalized{};

	// Use Create() so the object is consistently handed around in shared ownership.
	SnaccTelemetryData(Direction direction, unsigned int uiOperationID, const char* szOperationName, size_t stRequestData, std::chrono::steady_clock::time_point chronoCreated);
};

class SnaccTelemetryCallback
{
public:
	virtual ~SnaccTelemetryCallback() = default;

	/*
	 * Called after a ROSE message lifecycle has been processed by the stub.
	 * The callback receives a read-only telemetry object containing request metadata
	 * and the final processing outcome so implementations can record durations,
	 * payload sizes, result distribution, handled events, and the
	 * direction of the processed message.
	 */
	virtual void OnInvokeProcessed(std::shared_ptr<const SnaccTelemetryData>) = 0;
};
