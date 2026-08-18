#include "../include/SnaccRoseOperationLookup.h"
#include "../include/SNACCROSE.h"
#include "snacc-assert.h"

#include <cstdio>
#include <mutex>
#include <set>

namespace
{
void AssertNotSealed(const SnaccRoseOperationLookup& lookup)
{
#ifdef _DEBUG
	if (lookup.IsSealed())
	{
		ASSERT(0);
	}
#endif
	(void)lookup;
}
} // namespace

void SnaccRoseOperationLookup::RegisterModuleVersion(const char* szModuleName, const char* szVersion)
{
	if (!szModuleName || !szVersion || m_bSealed)
		return;

	AssertNotSealed(*this);

	auto& module = m_loadedModules[szModuleName];
	module.m_strModuleName = szModuleName;
	module.m_strVersion = szVersion;
}

void SnaccRoseOperationLookup::RegisterOperation(
	unsigned int uiOpID,
	const char* szOpName,
	unsigned int uiInterfaceID,
	const char* szModuleName,
	bool bIsEvent,
	unsigned long long ullAddedUnix,
	unsigned long long ullDeprecatedUnix)
{
	if (!szOpName || !szModuleName || m_bSealed)
		return;

	AssertNotSealed(*this);

	if (m_mapIDToOp.find(uiOpID) != m_mapIDToOp.end())
		return;

#ifdef _DEBUG
	if (m_mapOpToID.find(szOpName) != m_mapOpToID.end())
	{
		ASSERT(0);
	}
#endif

	m_mapOpToID[szOpName] = uiOpID;
	m_mapIDToOp[uiOpID] = szOpName;
	m_mapIDToInterface[uiOpID] = uiInterfaceID;
	m_mapIDToModuleKind[uiOpID] = {szModuleName, bIsEvent};

	auto& module = m_loadedModules[szModuleName];
	module.m_strModuleName = szModuleName;

	SnaccOpVersionInfo info;
	info.m_ullAddedUnix = ullAddedUnix;
	info.m_ullDeprecatedUnix = ullDeprecatedUnix;
	if (bIsEvent)
		module.m_events[uiOpID] = info;
	else
		module.m_invokes[uiOpID] = info;
}

void SnaccRoseOperationLookup::ClearRegisteredOperations()
{
	if (m_bSealed)
		return;

	AssertNotSealed(*this);

	m_mapOpToID.clear();
	m_mapIDToOp.clear();
	m_mapIDToInterface.clear();
	m_mapIDToModuleKind.clear();
	m_loadedModules.clear();
}

void SnaccRoseOperationLookup::Seal()
{
	m_bSealed = true;
}

bool SnaccRoseOperationLookup::HasRegisteredOperations() const
{
	return !m_mapIDToOp.empty();
}

const SnaccLoadedModuleMap& SnaccRoseOperationLookup::GetLoadedModules() const
{
	return m_loadedModules;
}

const char* SnaccRoseOperationLookup::LookUpName(unsigned int uiOpID) const
{
	const auto it = m_mapIDToOp.find(uiOpID);
	if (it != m_mapIDToOp.end())
		return it->second.c_str();

#ifdef _DEBUG
	static std::mutex s_mutex;
	static std::set<int> s_unknownOpIDsAlreadyNotified;
	std::lock_guard<std::mutex> lock(s_mutex);
	if (!s_unknownOpIDsAlreadyNotified.contains(uiOpID))
	{
		s_unknownOpIDsAlreadyNotified.insert(uiOpID);
		fprintf(stderr, "An unknown operation with operationID %d was called.\n", uiOpID);
	}
#endif

	return nullptr;
}

unsigned int SnaccRoseOperationLookup::LookUpID(const char* szOpName) const
{
	if (!szOpName)
		return 0;

	const auto it = m_mapOpToID.find(szOpName);
	if (it != m_mapOpToID.end())
		return it->second;

	return 0;
}

unsigned int SnaccRoseOperationLookup::LookUpInterfaceID(unsigned int uiOpID) const
{
	const auto it = m_mapIDToInterface.find(uiOpID);
	if (it != m_mapIDToInterface.end())
		return it->second;

	return 0;
}

SnaccRoseOperationLookupRegistrationHost::SnaccRoseOperationLookupRegistrationHost(SnaccRoseOperationLookup& operationLookup)
	: m_operationLookup(operationLookup)
{
}

std::shared_ptr<SnaccInvokeContext> SnaccRoseOperationLookupRegistrationHost::CreateInvokeContext(const SnaccInvokeContextInit& init)
{
	return SnaccInvokeContext::Create(init);
}

long SnaccRoseOperationLookupRegistrationHost::GetNextInvokeID()
{
	return m_lNextInvokeId++;
}

SNACC::EAsnLogLevel SnaccRoseOperationLookupRegistrationHost::GetLogLevel(const bool /* bOutbound */)
{
	return SNACC::EAsnLogLevel::DISABLED;
}

bool SnaccRoseOperationLookupRegistrationHost::LogTransportData(const bool /* bOutbound */, const SNACC::TransportEncoding /* encoding */, const char* /* szOperationName */, const char* /* szData */, const size_t /* size */, const SNACC::ROSEMessage* /* pMSg */, const SJson::Value* /* pParsedValue */)
{
	return false;
}

long SnaccRoseOperationLookupRegistrationHost::SendInvoke(SNACC::ROSEInvoke* /* pInvoke */, SNACC::AsnType* /* pResult */, SNACC::AsnType* /* pError */, const char* /* szOperationName */, std::shared_ptr<SnaccInvokeContext> /* pCtx */)
{
	return ROSE_TE_SHUTDOWN;
}

long SnaccRoseOperationLookupRegistrationHost::SendInvokeAsync(SNACC::ROSEInvoke* /* pInvoke */, SNACC::AsnType* /* pResult */, SNACC::AsnType* /* pError */, const char* /* szOperationName */, std::shared_ptr<SnaccInvokeContext> /* pCtx */)
{
	return ROSE_TE_SHUTDOWN;
}

long SnaccRoseOperationLookupRegistrationHost::HandleInvokeResult(long /* lRoseResult */, const SNACC::ROSEMessage& /* responseMsg */, SNACC::AsnType* /* pResult */, SNACC::AsnType* /* pError */, SnaccInvokeContext& /* ctx */)
{
	return ROSE_TE_SHUTDOWN;
}

long SnaccRoseOperationLookupRegistrationHost::HandleOnInvokeResult(SNACC::InvokeResult /* invokeResult */, const SNACC::ROSEInvoke& /* invoke */, SnaccInvokeContext& /* ctx */, std::string& /* strResponse */, SNACC::AsnType* /* pResult */, SNACC::AsnType* /* pError */)
{
	return ROSE_TE_SHUTDOWN;
}

long SnaccRoseOperationLookupRegistrationHost::DecodeInvoke(const SNACC::ROSEMessage& /* invokeMessage */, SNACC::AsnType* /* pArgument */)
{
	return ROSE_TE_SHUTDOWN;
}

long SnaccRoseOperationLookupRegistrationHost::SendEvent(SNACC::ROSEInvoke* /* pInvoke */, const char* /* szOperationName */, std::shared_ptr<SnaccInvokeContext> /* pCtx */)
{
	return ROSE_TE_SHUTDOWN;
}

long SnaccRoseOperationLookupRegistrationHost::EncodeResult(unsigned int /* uiInvokeID */, const SNACC::AsnType* /* pResult */, std::string& /* strResponse */, const wchar_t* /* szSessionID */)
{
	return ROSE_TE_SHUTDOWN;
}

long SnaccRoseOperationLookupRegistrationHost::EncodeError(unsigned int uiInvokeID, const SNACC::AsnType* /* pError */, std::string& /* strResponse */, const wchar_t* /* szSessionID */)
{
	(void)uiInvokeID;
	return ROSE_TE_SHUTDOWN;
}
