#include <gtest/gtest.h>

#include <SnaccROSEInterfaces.h>

namespace
{
	class RecordingSubscriptionSender : public SnaccROSESender
	{
	public:
		unsigned int subscribedEventOpId = 0;
		unsigned int supportedInvokeOpId = 0;

		std::shared_ptr<SnaccInvokeContext> CreateInvokeContext(const SnaccInvokeContextInit& init) override
		{
			(void)init;
			return {};
		}

		long GetNextInvokeID() override
		{
			return 1;
		}

		SNACC::EAsnLogLevel GetLogLevel(const bool /*bOutbound*/) override
		{
			return SNACC::EAsnLogLevel::DISABLED;
		}

		bool LogTransportData(const bool /*bOutbound*/, const SNACC::TransportEncoding /*encoding*/, const char* /*szOperationName*/, const char* /*szData*/, const size_t /*size*/, const SNACC::ROSEMessage* /*pMsg*/, const SJson::Value* /*pParsedValue*/) override
		{
			return false;
		}

		long SendInvoke(SNACC::ROSEInvoke* /*pInvoke*/, SNACC::AsnType* /*pResult*/, SNACC::AsnType* /*pError*/, const char* /*szOperationName*/, std::shared_ptr<SnaccInvokeContext> /*pCtx*/) override
		{
			return ROSE_NOERROR;
		}

		long SendInvokeAsync(SNACC::ROSEInvoke* /*pInvoke*/, SNACC::AsnType* /*pResult*/, SNACC::AsnType* /*pError*/, const char* /*szOperationName*/, std::shared_ptr<SnaccInvokeContext> /*pCtx*/) override
		{
			return ROSE_NOERROR;
		}

		long HandleInvokeResult(long /*lRoseResult*/, const SNACC::ROSEMessage& /*responseMsg*/, SNACC::AsnType* /*pResult*/, SNACC::AsnType* /*pError*/, SnaccInvokeContext& /*ctx*/) override
		{
			return ROSE_NOERROR;
		}

		long HandleOnInvokeResult(SNACC::InvokeResult /*invokeResult*/, const SNACC::ROSEInvoke& /*invoke*/, SnaccInvokeContext& /*ctx*/, std::string& /*strResponse*/, SNACC::AsnType* /*pResult*/, SNACC::AsnType* /*pError*/) override
		{
			return ROSE_NOERROR;
		}

		long DecodeInvoke(const SNACC::ROSEMessage& /*invokeMessage*/, SNACC::AsnType* /*pArgument*/) override
		{
			return ROSE_NOERROR;
		}

		long SendEvent(SNACC::ROSEInvoke* /*pInvoke*/, const char* /*szOperationName*/, std::shared_ptr<SnaccInvokeContext> /*pCtx*/) override
		{
			return ROSE_NOERROR;
		}

		long EncodeResult(unsigned int /*uiInvokeID*/, const SNACC::AsnType* /*pResult*/, std::string& /*strResponse*/, const wchar_t* /*szSessionID*/) override
		{
			return ROSE_NOERROR;
		}

		long EncodeError(unsigned int /*uiInvokeID*/, const SNACC::AsnType* /*pError*/, std::string& /*strResponse*/, const wchar_t* /*szSessionID*/) override
		{
			return ROSE_NOERROR;
		}

		bool IsSubscribedEvent(unsigned int uiEventOpId) const override
		{
			return uiEventOpId == subscribedEventOpId;
		}

		bool IsSupportedInvoke(unsigned int uiInvokeOpId) const override
		{
			return uiInvokeOpId == supportedInvokeOpId;
		}
	};

	TEST(ServerInvokeBlockPolicyTest, BlockWithoutStateDoesNotBlock)
	{
		RecordingSubscriptionSender sender;
		sender.SetOperationBlockPolicy(SnaccOperationBlockPolicy::BlockUnsupportedOperations);
		EXPECT_FALSE(sender.IsOperationBlocked(2109, true));
		EXPECT_FALSE(sender.IsOperationBlocked(2109, false));
	}

	TEST(ServerInvokeBlockPolicyTest, BlockWithStateBlocksUnsubscribedEvent)
	{
		RecordingSubscriptionSender sender;
		sender.SetOperationBlockPolicy(SnaccOperationBlockPolicy::BlockUnsupportedOperations);
		sender.MarkSessionSubscriptionStateSet();
		sender.subscribedEventOpId = 2170;
		EXPECT_TRUE(sender.IsOperationBlocked(2109, true));
		EXPECT_FALSE(sender.IsOperationBlocked(2170, true));
	}

	TEST(ServerInvokeBlockPolicyTest, BlockWithStateBlocksUnsupportedInvoke)
	{
		RecordingSubscriptionSender sender;
		sender.SetOperationBlockPolicy(SnaccOperationBlockPolicy::BlockUnsupportedOperations);
		sender.MarkSessionSubscriptionStateSet();
		sender.supportedInvokeOpId = 3001;
		EXPECT_TRUE(sender.IsOperationBlocked(3002, false));
		EXPECT_FALSE(sender.IsOperationBlocked(3001, false));
	}

	TEST(ServerInvokeBlockPolicyTest, NeverBlockDoesNotBlock)
	{
		RecordingSubscriptionSender sender;
		sender.SetOperationBlockPolicy(SnaccOperationBlockPolicy::NeverBlock);
		sender.MarkSessionSubscriptionStateSet();
		EXPECT_FALSE(sender.IsOperationBlocked(2109, true));
		EXPECT_FALSE(sender.IsOperationBlocked(2109, false));
	}
} // namespace
