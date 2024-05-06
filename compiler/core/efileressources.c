#include "efileressources.h"

#include <assert.h>
#include <stdio.h>
#include "../../snacc.h"
#include "cpp_c_helper.h"

#ifdef _WIN32
#include <Windows.h>
#else // _WIN32
#define INCBIN_SILENCE_BITCODE_WARNING
#include "incbin.h"
INCBIN(BIN_CONVERTER_BASE, "compiler/back-ends/ts-gen/gluecode/TSConverterBase.ts");
INCBIN(BIN_OPTIONALPARAM_CONVERTER, "compiler/back-ends/ts-gen/gluecode/TSOptionalParamConverter.ts");
INCBIN(BIN_ASN1_BASE, "compiler/back-ends/ts-gen/gluecode/TSASN1Base.ts");
INCBIN(BIN_ASN1_CLIENT, "compiler/back-ends/ts-gen/gluecode/TSASN1Client.ts");
INCBIN(BIN_ASN1_NODE_CLIENT, "compiler/back-ends/ts-gen/gluecode/TSASN1NodeClient.ts");
INCBIN(BIN_ASN1_BROWSER_CLIENT, "compiler/back-ends/ts-gen/gluecode/TSASN1BrowserClient.ts");
INCBIN(BIN_ASN1_SERVER, "compiler/back-ends/ts-gen/gluecode/TSASN1Server.ts");
INCBIN(BIN_ROSE_BASE, "compiler/back-ends/ts-gen/gluecode/TSROSEBase.ts");
INCBIN(BIN_SNACCROSE, "compiler/back-ends/ts-gen/gluecode/SNACCROSE.ts");
INCBIN(BIN_SNACCROSE_CONVERTER, "compiler/back-ends/ts-gen/gluecode/SNACCROSE_Converter.ts");
INCBIN(BIN_INVOKE_CONTEXT, "compiler/back-ends/ts-gen/gluecode/TSInvokeContext.ts");
INCBIN(BIN_DEPRECATED_CALLBACK, "compiler/back-ends/ts-gen/gluecode/TSDeprecatedCallback.ts");

INCBIN(BIN_EDELPHI_ASN1_YPES, "compiler/back-ends/delphi-gen/gluecode/DelphiAsn1Types.pas");
#endif // _WIN32

#ifndef _WIN32
void SaveIncBinToFile(const unsigned char* szData, const unsigned int size, const char* szFileName)
{
	writeFile( (const char *)szData, size, szFileName, genTSESMCode ? true : false, gNodeVersion);
}
#endif

extern int genTSESMCode;

void SaveResourceToFile(enum EFILERESSOURCE resourceID, const char* szFileName)
{
#ifdef _WIN32
	HINSTANCE hInst = GetModuleHandle(NULL);
	HRSRC hRes = FindResource(hInst, MAKEINTRESOURCE(resourceID), RT_RCDATA);
	if (hRes)
	{
		HGLOBAL hMem = LoadResource(hInst, hRes);
		if (hMem)
		{
			DWORD size = SizeofResource(hInst, hRes);
			const char* buffer = (const char*)LockResource(hMem);
			writeFile(buffer, size, szFileName, genTSESMCode ? true : false, gNodeVersion);
			FreeResource(hMem);
		}
		else
		{
			assert(FALSE);
		}
	}
	else
	{
		assert(FALSE);
	}
#else  // _WIN32

	switch (resourceID)
	{
		case ETS_CONVERTER_BASE:
			SaveIncBinToFile(gBIN_CONVERTER_BASEData, gBIN_CONVERTER_BASESize, szFileName);
			break;
		case ETS_ASN1_BASE:
			SaveIncBinToFile(gBIN_ASN1_BASEData, gBIN_ASN1_BASESize, szFileName);
			break;
		case ETS_ASN1_CLIENT:
			SaveIncBinToFile(gBIN_ASN1_CLIENTData, gBIN_ASN1_CLIENTSize, szFileName);
			break;
		case ETS_ASN1_NODE_CLIENT:
			SaveIncBinToFile(gBIN_ASN1_NODE_CLIENTData, gBIN_ASN1_NODE_CLIENTSize, szFileName);
			break;
		case ETS_ASN1_BROWSER_CLIENT:
			SaveIncBinToFile(gBIN_ASN1_BROWSER_CLIENTData, gBIN_ASN1_BROWSER_CLIENTSize, szFileName);
			break;
		case ETS_ASN1_SERVER:
			SaveIncBinToFile(gBIN_ASN1_SERVERData, gBIN_ASN1_SERVERSize, szFileName);
			break;
		case ETS_ROSE_BASE:
			SaveIncBinToFile(gBIN_ROSE_BASEData, gBIN_ROSE_BASESize, szFileName);
			break;
		case ETS_SNACCROSE:
			SaveIncBinToFile(gBIN_SNACCROSEData, gBIN_SNACCROSESize, szFileName);
			break;
		case ETS_SNACCROSE_CONVERTER:
			SaveIncBinToFile(gBIN_SNACCROSE_CONVERTERData, gBIN_SNACCROSE_CONVERTERSize, szFileName);
			break;
		case ETS_OPTIONALPARAM_CONVERTER:
			SaveIncBinToFile(gBIN_OPTIONALPARAM_CONVERTERData, gBIN_OPTIONALPARAM_CONVERTERSize, szFileName);
			break;
		case ETS_DEPRECATED_CALLBACK:
			SaveIncBinToFile(gBIN_DEPRECATED_CALLBACKData, gBIN_DEPRECATED_CALLBACKSize, szFileName);
			break;
		case ETS_INVOKE_CONTEXT:
			SaveIncBinToFile(gBIN_INVOKE_CONTEXTData, gBIN_INVOKE_CONTEXTSize, szFileName);
			break;
		case EDELPHI_ASN1_TYPES:
			SaveIncBinToFile(gBIN_EDELPHI_ASN1_YPESData, gBIN_EDELPHI_ASN1_YPESSize, szFileName);
			break;
		default:
			// Trying to write an unknown ressource type as gluecode file
			assert(0);
			break;
	}
#endif // _WIN32
}
