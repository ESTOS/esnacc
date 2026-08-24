#include <gtest/gtest.h>

#include <SnaccROSEInterfaces.h>

namespace
{
class RecordingRoseSender : public SnaccROSESender
{
public:
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

	void AddSubscribedEvent(int moduleIid, unsigned int uiEventOpId) override
	{
		lastModuleIid = moduleIid;
		subscribedEventOpId = uiEventOpId;
	}

	int lastModuleIid = 0;
	unsigned int subscribedEventOpId = 0;
};

class TestRoseComponent : public SnaccROSEComponent
{
public:
	explicit TestRoseComponent(SnaccROSESender* pSender)
		: SnaccROSEComponent(pSender)
	{
	}
};
} // namespace

TEST(RoseSessionSubscription, SnaccROSESenderDefaultsArePermissive)
{
	struct MinimalSender : public SnaccROSESender
	{
		std::shared_ptr<SnaccInvokeContext> CreateInvokeContext(const SnaccInvokeContextInit& init) override
		{
			(void)init;
			return {};
		}
		long GetNextInvokeID() override
		{
			return 0;
		}
		SNACC::EAsnLogLevel GetLogLevel(const bool) override
		{
			return SNACC::EAsnLogLevel::DISABLED;
		}
		bool LogTransportData(const bool, const SNACC::TransportEncoding, const char*, const char*, const size_t, const SNACC::ROSEMessage*, const SJson::Value*) override
		{
			return false;
		}
		long SendInvoke(SNACC::ROSEInvoke*, SNACC::AsnType*, SNACC::AsnType*, const char*, std::shared_ptr<SnaccInvokeContext>) override
		{
			return ROSE_NOERROR;
		}
		long SendInvokeAsync(SNACC::ROSEInvoke*, SNACC::AsnType*, SNACC::AsnType*, const char*, std::shared_ptr<SnaccInvokeContext>) override
		{
			return ROSE_NOERROR;
		}
		long HandleInvokeResult(long, const SNACC::ROSEMessage&, SNACC::AsnType*, SNACC::AsnType*, SnaccInvokeContext&) override
		{
			return ROSE_NOERROR;
		}
		long HandleOnInvokeResult(SNACC::InvokeResult, const SNACC::ROSEInvoke&, SnaccInvokeContext&, std::string&, SNACC::AsnType*, SNACC::AsnType*) override
		{
			return ROSE_NOERROR;
		}
		long DecodeInvoke(const SNACC::ROSEMessage&, SNACC::AsnType*) override
		{
			return ROSE_NOERROR;
		}
		long SendEvent(SNACC::ROSEInvoke*, const char*, std::shared_ptr<SnaccInvokeContext>) override
		{
			return ROSE_NOERROR;
		}
		long EncodeResult(unsigned int, const SNACC::AsnType*, std::string&, const wchar_t*) override
		{
			return ROSE_NOERROR;
		}
		long EncodeError(unsigned int, const SNACC::AsnType*, std::string&, const wchar_t*) override
		{
			return ROSE_NOERROR;
		}
	} sender;

	EXPECT_TRUE(sender.IsSubscribedEvent(1234));
	EXPECT_TRUE(sender.IsSupportedInvoke(5678));
}

TEST(RoseSessionSubscription, SnaccROSEComponentForwardsToSender)
{
	RecordingRoseSender sender;
	TestRoseComponent component(&sender);

	EXPECT_FALSE(component.IsSubscribedEvent(2109));
	component.AddSubscribedEvent(2104, 2109);
	EXPECT_EQ(2104, sender.lastModuleIid);
	EXPECT_TRUE(component.IsSubscribedEvent(2109));
	EXPECT_FALSE(component.IsSubscribedEvent(2170));
}
