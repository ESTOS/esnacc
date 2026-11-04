#include <gtest/gtest.h>

#include <RoseSessionSubscriptionStore.h>
#include <SnaccROSEInterfaces.h>

#include <initializer_list>
#include <list>

namespace snacclib
{
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

			ERoseResult SendEvent(SNACC::ROSEInvoke* /*pInvoke*/, const char* /*szOperationName*/, std::shared_ptr<SnaccInvokeContext> /*pCtx*/) override
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

		/* Builds the std::list<int> argument shape SetSubscribedEvents and SetSupportedInvokes take. */
		std::list<int> OpIdList(std::initializer_list<int> opIds)
		{
			return std::list<int>(opIds);
		}
	} // namespace

	TEST(snacclib_RoseSessionSubscription, SnaccROSEComponentForwardsToSender)
	{
		RecordingRoseSender sender;
		TestRoseComponent component(&sender);

		EXPECT_FALSE(component.IsSubscribedEvent(2109));
		component.AddSubscribedEvent(2104, 2109);
		EXPECT_EQ(2104, sender.lastModuleIid);
		EXPECT_TRUE(component.IsSubscribedEvent(2109));
		EXPECT_FALSE(component.IsSubscribedEvent(2170));
	}

	TEST(snacclib_RoseSessionSubscription, TracksSubscribedEvents)
	{
		RoseSessionSubscriptionStore store;
		EXPECT_FALSE(store.IsSubscribedEvent(2109));
		store.AddSubscribedEvent(2104, 2109);
		EXPECT_TRUE(store.IsSubscribedEvent(2109));
		EXPECT_FALSE(store.IsSubscribedEvent(2170));
		store.ClearSubscribedEvents(2104);
		EXPECT_FALSE(store.IsSubscribedEvent(2109));
	}

	TEST(snacclib_RoseSessionSubscription, SetSubscribedEventsReplacesThatModule)
	{
		RoseSessionSubscriptionStore store;
		store.SetSubscribedEvents(2104, OpIdList({2109, 2170}));
		store.AddSubscribedEvent(2104, 2110);

		store.SetSubscribedEvents(2104, OpIdList({2170}));

		EXPECT_FALSE(store.IsSubscribedEvent(2109));
		EXPECT_FALSE(store.IsSubscribedEvent(2110));
		EXPECT_TRUE(store.IsSubscribedEvent(2170));
	}

	TEST(snacclib_RoseSessionSubscription, SetSubscribedEventsLeavesOtherModules)
	{
		RoseSessionSubscriptionStore store;
		store.SetSubscribedEvents(2104, OpIdList({2109}));
		store.SetSubscribedEvents(2200, OpIdList({2170}));

		store.SetSubscribedEvents(2104, OpIdList({2110}));

		EXPECT_FALSE(store.IsSubscribedEvent(2109));
		EXPECT_TRUE(store.IsSubscribedEvent(2110));
		EXPECT_TRUE(store.IsSubscribedEvent(2170));
	}

	TEST(snacclib_RoseSessionSubscription, EmptySetSubscribedEventsClearsThatModuleOnly)
	{
		RoseSessionSubscriptionStore store;
		store.SetSubscribedEvents(2104, OpIdList({2109}));
		store.SetSubscribedEvents(2200, OpIdList({2170}));

		store.SetSubscribedEvents(2104, OpIdList({}));

		EXPECT_FALSE(store.IsSubscribedEvent(2109));
		EXPECT_TRUE(store.IsSubscribedEvent(2170));
	}

	TEST(snacclib_RoseSessionSubscription, ClearSubscribedEventsLeavesSupportedInvokes)
	{
		RoseSessionSubscriptionStore store;
		store.SetSubscribedEvents(2104, OpIdList({2109}));
		store.SetSupportedInvokes(2104, OpIdList({4100}));
		store.SetSupportedInvokes(2200, OpIdList({4200}));

		store.ClearSubscribedEvents(2104);

		EXPECT_FALSE(store.IsSubscribedEvent(2109));
		EXPECT_TRUE(store.IsSupportedInvoke(4100));
		EXPECT_TRUE(store.IsSupportedInvoke(4200));
	}

	TEST(snacclib_RoseSessionSubscription, ClearSupportedInvokesLeavesSubscribedEvents)
	{
		RoseSessionSubscriptionStore store;
		store.SetSubscribedEvents(2104, OpIdList({2109}));
		store.SetSupportedInvokes(2104, OpIdList({4100}));

		store.ClearSupportedInvokes(2104);

		EXPECT_TRUE(store.IsSubscribedEvent(2109));
		EXPECT_FALSE(store.IsSupportedInvoke(4100));
	}

	TEST(snacclib_RoseSessionSubscription, ClearAllSubscriptionsClearsEventsAndInvokes)
	{
		RoseSessionSubscriptionStore store;
		store.SetSubscribedEvents(2104, OpIdList({2109}));
		store.SetSupportedInvokes(2200, OpIdList({4100}));

		store.ClearAllSubscriptions();

		EXPECT_FALSE(store.IsSubscribedEvent(2109));
		EXPECT_FALSE(store.IsSupportedInvoke(4100));
	}

	TEST(snacclib_RoseSessionSubscription, AddSubscribedEventKeepsOtherModules)
	{
		RoseSessionSubscriptionStore store;
		store.SetSubscribedEvents(2104, OpIdList({2109}));
		store.AddSubscribedEvent(2200, 2170);

		store.AddSubscribedEvent(2104, 2110);

		EXPECT_TRUE(store.IsSubscribedEvent(2109));
		EXPECT_TRUE(store.IsSubscribedEvent(2110));
		EXPECT_TRUE(store.IsSubscribedEvent(2170));
	}

	TEST(snacclib_RoseSessionSubscription, SetSupportedInvokesReplacesThatModuleAndLeavesEvents)
	{
		RoseSessionSubscriptionStore store;
		store.SetSubscribedEvents(2104, OpIdList({2109}));
		store.SetSupportedInvokes(2104, OpIdList({4100, 4101}));
		store.SetSupportedInvokes(2200, OpIdList({4200}));

		store.SetSupportedInvokes(2104, OpIdList({4101}));

		EXPECT_TRUE(store.IsSubscribedEvent(2109));
		EXPECT_FALSE(store.IsSupportedInvoke(4100));
		EXPECT_TRUE(store.IsSupportedInvoke(4101));
		EXPECT_TRUE(store.IsSupportedInvoke(4200));
	}

	TEST(snacclib_RoseSessionSubscription, EmptySetSupportedInvokesClearsThatModuleOnly)
	{
		RoseSessionSubscriptionStore store;
		store.SetSupportedInvokes(2104, OpIdList({4100}));
		store.SetSupportedInvokes(2200, OpIdList({4200}));

		store.SetSupportedInvokes(2104, OpIdList({}));

		EXPECT_FALSE(store.IsSupportedInvoke(4100));
		EXPECT_TRUE(store.IsSupportedInvoke(4200));
	}

	TEST(snacclib_RoseSessionSubscription, AddSupportedInvokeKeepsOtherModules)
	{
		RoseSessionSubscriptionStore store;
		store.SetSupportedInvokes(2104, OpIdList({4100}));
		store.AddSupportedInvoke(2200, 4200);

		store.AddSupportedInvoke(2104, 4101);

		EXPECT_TRUE(store.IsSupportedInvoke(4100));
		EXPECT_TRUE(store.IsSupportedInvoke(4101));
		EXPECT_TRUE(store.IsSupportedInvoke(4200));
	}

	TEST(snacclib_RoseSessionSubscription, ClearOfAnUnknownModuleLeavesTheSession)
	{
		RoseSessionSubscriptionStore store;
		store.SetSubscribedEvents(2104, OpIdList({2109}));
		store.SetSupportedInvokes(2104, OpIdList({4100}));

		store.ClearSubscribedEvents(9999);
		store.ClearSupportedInvokes(9998);

		EXPECT_TRUE(store.IsSubscribedEvent(2109));
		EXPECT_TRUE(store.IsSupportedInvoke(4100));
	}

} // namespace snacclib
