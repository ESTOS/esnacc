#include "test_support/sample_runtime_harness.h"

namespace sample_runtime_tests
{

// Covers protocol and logical failure scenarios where the request reaches the
// server-side glue but should result in a reject or application error.
class LogicalFailureRuntimeTest : public RuntimeTestBase
{
protected:
	// Verifies that an unknown operation id is rejected by the server.
	void AssertUnknownOperationReturnsReject(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);
		const long roseResult = m_clientSettingsModule.InvokeUnknownOperation();
		EXPECT_EQ(ROSE_REJECT_UNKNOWNOPERATION, roseResult);
	}

	// Verifies that a missing handler implementation yields a function-missing reject.
	void AssertMissingHandlerReturnsFunctionMissingReject(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);
		HandlerModes handlerModes;
		handlerModes.implementGetSettings = false;
		ConfigureServerHandlers(handlerModes);

		AsnGetSettingsArgument argument;
		AsnGetSettingsResult result;
		AsnRequestError error;

		const long roseResult = m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error);
		EXPECT_EQ(ROSE_REJECT_FUNCTIONMISSING, roseResult);
	}

	// Verifies that a handler can deliberately return a ROSE application error.
	void AssertHandlerCanReturnApplicationError(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);
		HandlerModes handlerModes;
		handlerModes.getSettingsReturnsError = true;
		ConfigureServerHandlers(handlerModes);

		AsnGetSettingsArgument argument;
		AsnGetSettingsResult result;
		AsnRequestError error;

		const long roseResult = m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error);
		EXPECT_EQ(ROSE_ERROR_VALUE, roseResult);
		EXPECT_EQ(7001, error.iErrorDetail.GetLongLong());
		EXPECT_EQ("get settings handler returned error", error.u8sErrorString.getASCII());
	}

	// Verifies that omitting a required argument is rejected during decode/dispatch.
	void AssertMissingArgumentIsRejected(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);
		AsnSetSettingsResult result;
		AsnRequestError error;

		const long roseResult = m_clientSettingsModule.InvokeSetSettingsWithoutArgument(&result, &error);
		EXPECT_EQ(ROSE_REJECT_MISTYPEDARGUMENT, roseResult);
	}

	// Verifies that using an argument of the wrong ASN.1 type leads to a reject.
	void AssertMistypedArgumentIsRejected(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);
		AsnCreateFancyEventsArgument wrongArgument;
		wrongArgument.iEventDelay = 1;
		wrongArgument.iEventCount = 2;
		AsnSetSettingsResult result;
		AsnRequestError error;

		const long roseResult = m_clientSettingsModule.InvokeSetSettingsWithWrongArgument(&wrongArgument, &result, &error);
		EXPECT_NE(ROSE_NOERROR, roseResult);
		EXPECT_TRUE(ISROSE_REJECT(roseResult));
	}
};

// Runs the unknown-operation reject scenario over BER.
TEST_F(LogicalFailureRuntimeTest, UnknownOperationReturnsRejectBer)
{
	AssertUnknownOperationReturnsReject(TransportEncoding::BER);
}

// Runs the missing-handler reject scenario over BER.
TEST_F(LogicalFailureRuntimeTest, MissingHandlerReturnsFunctionMissingRejectBer)
{
	AssertMissingHandlerReturnsFunctionMissingReject(TransportEncoding::BER);
}

// Runs the application-error scenario over BER.
TEST_F(LogicalFailureRuntimeTest, HandlerCanReturnApplicationErrorBer)
{
	AssertHandlerCanReturnApplicationError(TransportEncoding::BER);
}

// Runs the missing-argument reject scenario over BER.
TEST_F(LogicalFailureRuntimeTest, MissingArgumentIsRejectedBer)
{
	AssertMissingArgumentIsRejected(TransportEncoding::BER);
}

// Runs the mistyped-argument reject scenario over BER.
TEST_F(LogicalFailureRuntimeTest, MistypedArgumentIsRejectedBer)
{
	AssertMistypedArgumentIsRejected(TransportEncoding::BER);
}

// Runs the unknown-operation reject scenario over JSON.
TEST_F(LogicalFailureRuntimeTest, UnknownOperationReturnsRejectJson)
{
	AssertUnknownOperationReturnsReject(TransportEncoding::JSON);
}

// Runs the missing-handler reject scenario over JSON.
TEST_F(LogicalFailureRuntimeTest, MissingHandlerReturnsFunctionMissingRejectJson)
{
	AssertMissingHandlerReturnsFunctionMissingReject(TransportEncoding::JSON);
}

// Runs the application-error scenario over JSON.
TEST_F(LogicalFailureRuntimeTest, HandlerCanReturnApplicationErrorJson)
{
	AssertHandlerCanReturnApplicationError(TransportEncoding::JSON);
}

// Runs the missing-argument reject scenario over JSON.
TEST_F(LogicalFailureRuntimeTest, MissingArgumentIsRejectedJson)
{
	AssertMissingArgumentIsRejected(TransportEncoding::JSON);
}

// Runs the mistyped-argument reject scenario over JSON.
TEST_F(LogicalFailureRuntimeTest, MistypedArgumentIsRejectedJson)
{
	AssertMistypedArgumentIsRejected(TransportEncoding::JSON);
}

} // namespace sample_runtime_tests
