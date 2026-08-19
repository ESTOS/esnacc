#include "test_support/sample_runtime_harness.h"

#include <SnaccModuleCapabilities.h>

#include <gtest/gtest.h>

namespace sample_runtime_tests
{
namespace
{
const char* kSettingsModuleName = "ENetUC_Settings_Manager";

SnaccLoadedModuleMap BuildRemoteSnapshotWithGetSettingsOnly()
{
	const int invokeOpIds[] = {4100};
	const SnaccRemoteModuleDetailInput detail{
		kSettingsModuleName,
		"20240101.0.20240506",
		invokeOpIds,
		1,
		nullptr,
		0,
	};
	SnaccLoadedModuleMap remote;
	SnaccBuildRemoteModuleCapabilities(&detail, 1, remote);
	return remote;
}

SnaccLoadedModuleMap BuildRemoteSnapshotWithoutGetSettings()
{
	const int invokeOpIds[] = {4101};
	const SnaccRemoteModuleDetailInput detail{
		kSettingsModuleName,
		"20240101.0.20240506",
		invokeOpIds,
		1,
		nullptr,
		0,
	};
	SnaccLoadedModuleMap remote;
	SnaccBuildRemoteModuleCapabilities(&detail, 1, remote);
	return remote;
}
} // namespace

class RemoteCapabilityRuntimeTest : public RuntimeTestBase
{
protected:
	void InitializeConnectedEndpoints()
	{
		InitializeEndpoints(TransportEncoding::JSON);
	}

	long InvokeGetSettingsOnClient()
	{
		AsnGetSettingsArgument argument;
		AsnGetSettingsResult result;
		AsnRequestError error;
		return m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error, 250);
	}
};

TEST(RemoteCapabilityModuleHelperTest, BuildRemoteModuleCapabilitiesPopulatesInvokeAndEventOpIds)
{
	const int invokeOpIds[] = {4100, 4101};
	const int eventOpIds[] = {4150};
	const SnaccRemoteModuleDetailInput detail{
		kSettingsModuleName,
		"20240101.0.20240506",
		invokeOpIds,
		2,
		eventOpIds,
		1,
	};

	SnaccLoadedModuleMap remote;
	SnaccBuildRemoteModuleCapabilities(&detail, 1, remote);

	ASSERT_EQ(1u, remote.size());
	const auto& module = remote.at(kSettingsModuleName);
	EXPECT_EQ(kSettingsModuleName, module.m_strModuleName);
	EXPECT_EQ("20240101.0.20240506", module.m_strVersion);
	EXPECT_NE(module.m_invokes.end(), module.m_invokes.find(4100u));
	EXPECT_NE(module.m_invokes.end(), module.m_invokes.find(4101u));
	EXPECT_NE(module.m_events.end(), module.m_events.find(4150u));
}

TEST_F(RemoteCapabilityRuntimeTest, EnabledWithoutSnapshotDoesNotGateInvoke)
{
	InitializeConnectedEndpoints();
	m_client.SetRemoteCapabilityMode(SnaccRemoteCapabilityMode::Enabled);

	const long roseResult = InvokeGetSettingsOnClient();
	EXPECT_EQ(ROSE_NOERROR, roseResult);
	EXPECT_GE(m_client.Transport().TransportSendCount(), 1u);
}

TEST_F(RemoteCapabilityRuntimeTest, EnabledWithUnsupportedOpIdReturnsRemoteNotCapable)
{
	InitializeConnectedEndpoints();
	m_client.ApplyRemoteModuleCapabilities(BuildRemoteSnapshotWithoutGetSettings());
	m_client.SetRemoteCapabilityMode(SnaccRemoteCapabilityMode::Enabled);

	const long roseResult = InvokeGetSettingsOnClient();
	EXPECT_EQ(ROSE_REJECT_REMOTENOTCAPABLE, roseResult);
	EXPECT_EQ(0u, m_server.InboundObservation().TransportSendCount());
}

TEST_F(RemoteCapabilityRuntimeTest, EnabledWithSupportedOpIdSendsInvoke)
{
	InitializeConnectedEndpoints();
	m_client.ApplyRemoteModuleCapabilities(BuildRemoteSnapshotWithGetSettingsOnly());
	m_client.SetRemoteCapabilityMode(SnaccRemoteCapabilityMode::Enabled);

	const long roseResult = InvokeGetSettingsOnClient();
	EXPECT_EQ(ROSE_NOERROR, roseResult);
	EXPECT_GE(m_client.Transport().TransportSendCount(), 1u);
}

TEST_F(RemoteCapabilityRuntimeTest, DisabledWithSnapshotDoesNotGateInvoke)
{
	InitializeConnectedEndpoints();
	m_client.ApplyRemoteModuleCapabilities(BuildRemoteSnapshotWithoutGetSettings());
	m_client.SetRemoteCapabilityMode(SnaccRemoteCapabilityMode::Disabled);

	const long roseResult = InvokeGetSettingsOnClient();
	EXPECT_EQ(ROSE_NOERROR, roseResult);
	EXPECT_GE(m_client.Transport().TransportSendCount(), 1u);
}

TEST_F(RemoteCapabilityRuntimeTest, ClearRemoteCapabilitiesStopsGating)
{
	InitializeConnectedEndpoints();
	m_client.ApplyRemoteModuleCapabilities(BuildRemoteSnapshotWithoutGetSettings());
	m_client.SetRemoteCapabilityMode(SnaccRemoteCapabilityMode::Enabled);
	m_client.ClearRemoteModuleCapabilities();

	const long roseResult = InvokeGetSettingsOnClient();
	EXPECT_EQ(ROSE_NOERROR, roseResult);
	EXPECT_GE(m_client.Transport().TransportSendCount(), 1u);
}

TEST_F(RemoteCapabilityRuntimeTest, IsSupportedOperationReflectsAppliedSnapshot)
{
	SnaccRoseOperationLookup lookup;
	RuntimeEndpoint endpoint{L"RemoteCapabilityQuery", "remote-capability-query", lookup};
	ENetUC_Settings_ManagerROSE::RegisterOperations(lookup);

	endpoint.ApplyRemoteModuleCapabilities(BuildRemoteSnapshotWithGetSettingsOnly());
	EXPECT_TRUE(endpoint.HasRemoteModuleCapabilities());
	EXPECT_TRUE(endpoint.IsSupportedOperation(4100u));
	EXPECT_FALSE(endpoint.IsSupportedOperation(4101u));
}

} // namespace sample_runtime_tests
