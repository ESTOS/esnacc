#ifndef _SnaccRoseOperationLookup_h_
#define _SnaccRoseOperationLookup_h_

/*! UCAAS-1485: per-listener operation lookup table (immutable after Seal). */

#include "SnaccROSEInterfaces.h"

#include <map>
#include <string>
#include <unordered_map>

/*! Per-operation @added / @deprecated metadata (unix seconds; 0 = not annotated). */
struct SnaccOpVersionInfo
{
	unsigned long long m_ullAddedUnix = 0;
	unsigned long long m_ullDeprecatedUnix = 0;
};

/*! Loaded module snapshot for negotiate / introspection on one lookup table. */
struct SnaccLoadedModuleInfo
{
	std::string m_strModuleName;
	std::string m_strVersion;
	std::unordered_map<unsigned int, SnaccOpVersionInfo> m_invokes;
	std::unordered_map<unsigned int, SnaccOpVersionInfo> m_events;
};

using SnaccLoadedModuleMap = std::unordered_map<std::string, SnaccLoadedModuleInfo>;

/*! Immutable operation-id lookup table after Seal().
	Fill at listener startup; share one instance across all connections on that listener.
	Lookup is read-only and needs no locking once sealed. */
class SnaccRoseOperationLookup
{
public:
	SnaccRoseOperationLookup() = default;

	/*! Populates module version metadata before Seal(). Used by generated RegisterOperations(). */
	void RegisterModuleVersion(const char* szModuleName, const char* szVersion);

	/*! Registers one invoke or event before Seal(). Duplicate operation ids are ignored. */
	void RegisterOperation(
		unsigned int uiOpID,
		const char* szOpName,
		unsigned int uiInterfaceID,
		const char* szModuleName,
		bool bIsEvent = false,
		unsigned long long ullAddedUnix = 0,
		unsigned long long ullDeprecatedUnix = 0);

	/*! Test-only reset before Seal(). Production tables should Seal() after startup registration. */
	void ClearRegisteredOperations();

	/*! Freezes the table. Further Register* calls assert in debug and are ignored in release. */
	void Seal();

	/*! True after Seal() was called. */
	bool IsSealed() const
	{
		return m_bSealed;
	}

	bool HasRegisteredOperations() const;
	const SnaccLoadedModuleMap& GetLoadedModules() const;

	const char* LookUpName(unsigned int uiOpID) const;
	unsigned int LookUpID(const char* szOpName) const;
	unsigned int LookUpInterfaceID(unsigned int uiOpID) const;

private:
	bool m_bSealed = false;
	std::map<std::string, unsigned int> m_mapOpToID;
	std::map<unsigned int, std::string> m_mapIDToOp;
	std::map<unsigned int, unsigned int> m_mapIDToInterface;
	SnaccLoadedModuleMap m_loadedModules;
	std::unordered_map<unsigned int, std::pair<std::string, bool>> m_mapIDToModuleKind;
};

/*! Minimal SnaccROSESender used only to drive generated RegisterOperations() at startup.
	Pass to generated *ROSE(&host) while filling a listener-owned SnaccRoseOperationLookup. */
class SnaccRoseOperationLookupRegistrationHost : public SnaccROSESender
{
public:
	explicit SnaccRoseOperationLookupRegistrationHost(SnaccRoseOperationLookup& operationLookup);

	SnaccRoseOperationLookup& OperationLookup()
	{
		return m_operationLookup;
	}

	std::shared_ptr<SnaccInvokeContext> CreateInvokeContext(const SnaccInvokeContextInit& init) override;
	long GetNextInvokeID() override;
	SNACC::EAsnLogLevel GetLogLevel(const bool bOutbound) override;
	bool LogTransportData(const bool bOutbound, const SNACC::TransportEncoding encoding, const char* szOperationName, const char* szData, const size_t size, const SNACC::ROSEMessage* pMSg, const SJson::Value* pParsedValue = nullptr) override;
	long SendInvoke(SNACC::ROSEInvoke* pInvoke, SNACC::AsnType* pResult, SNACC::AsnType* pError, const char* szOperationName, std::shared_ptr<SnaccInvokeContext> pCtx = {}) override;
	long SendInvokeAsync(SNACC::ROSEInvoke* pInvoke, SNACC::AsnType* pResult, SNACC::AsnType* pError, const char* szOperationName, std::shared_ptr<SnaccInvokeContext> pCtx = {}) override;
	long HandleInvokeResult(long lRoseResult, const SNACC::ROSEMessage& responseMsg, SNACC::AsnType* pResult, SNACC::AsnType* pError, SnaccInvokeContext& ctx) override;
	long HandleOnInvokeResult(SNACC::InvokeResult invokeResult, const SNACC::ROSEInvoke& invoke, SnaccInvokeContext& ctx, std::string& strResponse, SNACC::AsnType* pResult, SNACC::AsnType* pError) override;
	long DecodeInvoke(const SNACC::ROSEMessage& invokeMessage, SNACC::AsnType* pArgument) override;
	long SendEvent(SNACC::ROSEInvoke* pInvoke, const char* szOperationName, std::shared_ptr<SnaccInvokeContext> pCtx = {}) override;
	long EncodeResult(unsigned int uiInvokeID, const SNACC::AsnType* pResult, std::string& strResponse, const wchar_t* szSessionID = nullptr) override;
	long EncodeError(unsigned int uiInvokeID, const SNACC::AsnType* pError, std::string& strResponse, const wchar_t* szSessionID = nullptr) override;

private:
	SnaccRoseOperationLookup& m_operationLookup;
	long m_lNextInvokeId = 1;
};

#endif // _SnaccRoseOperationLookup_h_
