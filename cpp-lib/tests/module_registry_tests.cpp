#include "test_support/sample_runtime_harness.h"

#include <gtest/gtest.h>

namespace sample_runtime_tests
{
namespace
{
const char* kSettingsModuleName = "ENetUC_Settings_Manager";

void ExpectOpInfo(const SnaccOpVersionInfo& info, const bool bExpectAdded, const bool bExpectDeprecated)
{
	if (bExpectAdded)
		EXPECT_GT(info.m_ullAddedUnix, 0u);
	else
		EXPECT_EQ(0u, info.m_ullAddedUnix);

	if (bExpectDeprecated)
		EXPECT_GT(info.m_ullDeprecatedUnix, 0u);
	else
		EXPECT_EQ(0u, info.m_ullDeprecatedUnix);
}
} // namespace

TEST(ModuleRegistryTest, GeneratedModuleRegistersOnlyOnMountedStub)
{
	RuntimeEndpoint endpointA{L"RegistryEndpointA", "registry-a"};
	RuntimeEndpoint endpointB{L"RegistryEndpointB", "registry-b"};

	ENetUC_Settings_ManagerROSE settingsOnA(&endpointA);

	EXPECT_TRUE(endpointA.HasRegisteredOperations());
	EXPECT_FALSE(endpointB.HasRegisteredOperations());

	const auto& modulesA = endpointA.GetLoadedModules();
	ASSERT_EQ(1u, modulesA.size());
	const auto moduleIt = modulesA.find(kSettingsModuleName);
	ASSERT_NE(modulesA.end(), moduleIt);
	EXPECT_FALSE(moduleIt->second.m_strVersion.empty());
	EXPECT_GE(moduleIt->second.m_invokes.size(), 3u);
	EXPECT_EQ(1u, moduleIt->second.m_events.size());
	EXPECT_NE(moduleIt->second.m_events.end(), moduleIt->second.m_events.find(4150u));

	endpointA.ClearRegisteredOperations();
	EXPECT_FALSE(endpointA.HasRegisteredOperations());
	EXPECT_TRUE(endpointA.GetLoadedModules().empty());
}

TEST(ModuleRegistryTest, RegisteredMetadataMatchesLoadedModuleSnapshot)
{
	RuntimeEndpoint endpoint{L"RegistryMetadata", "registry-metadata"};
	ENetUC_Settings_ManagerROSE settings(&endpoint);

	const auto& modules = endpoint.GetLoadedModules();
	ASSERT_EQ(1u, modules.size());
	const auto& module = modules.begin()->second;

	EXPECT_EQ(kSettingsModuleName, module.m_strModuleName);
	EXPECT_FALSE(module.m_strVersion.empty());

	ASSERT_NE(module.m_invokes.end(), module.m_invokes.find(4100u));
	ASSERT_NE(module.m_invokes.end(), module.m_invokes.find(4101u));
	ASSERT_NE(module.m_invokes.end(), module.m_invokes.find(4102u));
	ExpectOpInfo(module.m_invokes.at(4100u), true, false);
	ExpectOpInfo(module.m_invokes.at(4101u), false, false);
	ExpectOpInfo(module.m_invokes.at(4102u), false, true);

	ASSERT_NE(module.m_events.end(), module.m_events.find(4150u));
	ExpectOpInfo(module.m_events.at(4150u), false, false);

	EXPECT_EQ(4100u, endpoint.LookUpID("asnGetSettings"));
	EXPECT_STREQ("asnGetSettings", endpoint.LookUpName(4100u));
	EXPECT_EQ(ENetUC_Settings_ManagerROSE::m_iid, endpoint.LookUpInterfaceID(4100u));
}

TEST(ModuleRegistryTest, RuntimeFixtureKeepsClientAndServerRegistriesSeparate)
{
	RuntimeEndpoint server{L"RegistryServer", "registry-server"};
	RuntimeEndpoint client{L"RegistryClient", "registry-client"};
	server.ConnectTo(client);
	client.ConnectTo(server);

	SettingsServiceModule serverSettings(server);
	SettingsClientModule clientSettings(client);

	EXPECT_TRUE(server.HasRegisteredOperations());
	EXPECT_TRUE(client.HasRegisteredOperations());

	const auto& serverModules = server.GetLoadedModules();
	const auto& clientModules = client.GetLoadedModules();

	EXPECT_FALSE(serverModules.empty());
	EXPECT_FALSE(clientModules.empty());
	EXPECT_NE(serverModules.find(kSettingsModuleName), serverModules.end());
	EXPECT_NE(clientModules.find(kSettingsModuleName), clientModules.end());

	server.ClearRegisteredOperations();
	EXPECT_TRUE(server.GetLoadedModules().empty());
	EXPECT_FALSE(client.GetLoadedModules().empty());
}

} // namespace sample_runtime_tests
