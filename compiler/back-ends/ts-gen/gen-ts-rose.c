#include "../../../snacc.h"
#include "gen-ts-code.h"
#include "gen-ts-rose.h"
#include "gen-ts-combined.h"
#include "../str-util.h"
#include "../comment-util.h"
#include "../structure-util.h"
#include "../../core/efileressources.h"
#include "../str-util.h"
#include "../../core/asn_comments.h"
#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <inttypes.h>

void PrintTSROSEHeader(FILE* src, Module* m, const bool bInterface)
{
	fprintf(src, "// [%s]\n", __FUNCTION__);
	fprintf(src, "/**\n");
	if (bInterface)
		fprintf(src, " * %sROSE_Interface\n", RemovePath(m->baseFilePath));
	else
		fprintf(src, " * %sROSE\n", RemovePath(m->baseFilePath));

	if (bInterface)
		fprintf(src, " * \"%s\" ASN.1 interfaces.\n", m->modId->name);
	else
		fprintf(src, " * \"%s\" ASN.1 stubs.\n", m->modId->name);

	write_snacc_header(src, " * ");
	fprintf(src, " */\n\n");
	fprintf(src, "// prettier-ignore\n");
	fprintf(src, ESLINT_DISABLE);
}

void SaveTSROSEFilesToOutputDirectory(const int genRoseStubs, const char* szPath)
{
	if (genRoseStubs)
	{
		{
			char szFileName[_MAX_PATH] = {0};
			strcpy_s(szFileName, _MAX_PATH, szPath);
			strcat_s(szFileName, _MAX_PATH, "TSASN1Base.ts");
			SaveResourceToFile(ETS_ASN1_BASE, szFileName);
		}
		if (genRoseStubs & 0x01)
		{
			char szFileName[_MAX_PATH] = {0};
			strcpy_s(szFileName, _MAX_PATH, szPath);
			strcat_s(szFileName, _MAX_PATH, "TSASN1Server.ts");
			SaveResourceToFile(ETS_ASN1_SERVER, szFileName);
		}
		if (genRoseStubs & 0x02 || genRoseStubs & 0x04)
		{
			char szFileName[_MAX_PATH] = {0};
			strcpy_s(szFileName, _MAX_PATH, szPath);
			strcat_s(szFileName, _MAX_PATH, "TSASN1Client.ts");
			SaveResourceToFile(ETS_ASN1_CLIENT, szFileName);
		}
		if (genRoseStubs & 0x02)
		{
			char szFileName[_MAX_PATH] = {0};
			strcpy_s(szFileName, _MAX_PATH, szPath);
			strcat_s(szFileName, _MAX_PATH, "TSASN1NodeClient.ts");
			SaveResourceToFile(ETS_ASN1_NODE_CLIENT, szFileName);
		}
		if (genRoseStubs & 0x04)
		{
			char szFileName[_MAX_PATH] = {0};
			strcpy_s(szFileName, _MAX_PATH, szPath);
			strcat_s(szFileName, _MAX_PATH, "TSASN1BrowserClient.ts");
			SaveResourceToFile(ETS_ASN1_BROWSER_CLIENT, szFileName);
		}
		{
			char szFileName[_MAX_PATH] = {0};
			strcpy_s(szFileName, _MAX_PATH, szPath);
			strcat_s(szFileName, _MAX_PATH, "TSROSEBase.ts");
			SaveResourceToFile(ETS_ROSE_BASE, szFileName);
		}
		{
			char szFileName[_MAX_PATH] = {0};
			strcpy_s(szFileName, _MAX_PATH, szPath);
			strcat_s(szFileName, _MAX_PATH, "SNACCROSE.ts");
			SaveResourceToFile(ETS_SNACCROSE, szFileName);
		}
		{
			char szFileName[_MAX_PATH] = {0};
			strcpy_s(szFileName, _MAX_PATH, szPath);
			strcat_s(szFileName, _MAX_PATH, "SNACCROSE_Converter.ts");
			SaveResourceToFile(ETS_SNACCROSE_CONVERTER, szFileName);
		}
	}
	{
		char szFileName[_MAX_PATH] = {0};
		strcpy_s(szFileName, _MAX_PATH, szPath);
		strcat_s(szFileName, _MAX_PATH, "TSOptionalParamConverter.ts");
		SaveResourceToFile(ETS_OPTIONALPARAM_CONVERTER, szFileName);
	}
	{
		char szFileName[_MAX_PATH] = {0};
		strcpy_s(szFileName, _MAX_PATH, szPath);
		strcat_s(szFileName, _MAX_PATH, "TSDeprecatedCallback.ts");
		SaveResourceToFile(ETS_DEPRECATED_CALLBACK, szFileName);
	}
	{
		char szFileName[_MAX_PATH] = {0};
		strcpy_s(szFileName, _MAX_PATH, szPath);
		strcat_s(szFileName, _MAX_PATH, "TSInvokeContext.ts");
		SaveResourceToFile(ETS_INVOKE_CONTEXT, szFileName);
	}
}

void PrintTSROSEImports(FILE* src, ModuleList* mods, Module* mod)
{
	fprintf(src, "// [%s]\n", __FUNCTION__);

	fprintf(src, "// Global imports\n");
	fprintf(src, "import { I%s, I%s_Handler } from \"./%s_Interface%s\";\n", mod->ROSEClassName, mod->ROSEClassName, mod->ROSEClassName, getCommonJSFileExtension());
	fprintf(src, "import { ROSEError, ROSEInvoke, ROSEReject, ROSEResult } from \"./SNACCROSE%s\";\n", getCommonJSFileExtension());
	fprintf(src, "import { AsnInvokeProblem, AsnInvokeProblemEnum, createInvokeReject, IASN1Transport, IASN1LogData, IReceiveInvokeContext, IInvokeHandler, ELogSeverity, ROSEBase } from \"./TSROSEBase%s\";\n", getCommonJSFileExtension());
	fprintf(src, "import { ISendInvokeContextParams } from \"./TSInvokeContext%s\";\n", getCommonJSFileExtension());

	fprintf(src, "// Local imports\n");
	fprintf(src, "import * as %s from \"./%s%s\";\n", GetNameSpace(mod), mod->moduleName, getCommonJSFileExtension());
	fprintf(src, "import * as Converter from \"./%s_Converter%s\";\n", mod->moduleName, getCommonJSFileExtension());

	PrintTSImports(src, mods, mod, true, false, true);
}

void PrintTSROSETypeDefCode(FILE* src, ModuleList* mods, Module* m, TypeDef* td)
{
}

bool PrintTSROSEOperationDefine(FILE* src, Module* m, ValueDef* v)
{
	/* just do ints */
	if (v->value->basicValue->choiceId != BASICVALUE_INTEGER)
		return false;

	if (v->value->type->basicType->choiceId != BASICTYPE_MACROTYPE)
		return false;

	if (v->value->type->basicType->a.macroType->choiceId != MACROTYPE_ROSOPERATION)
		return false;

	if (IsDeprecatedNoOutputOperation(m, v->definedName))
		return false;

	fprintf(src, "\tOPID_%s = %d", v->definedName, v->value->basicValue->a.integer);
	return true;
}

void PrintTSROSEOperationDefines(FILE* src, Module* mod, ValueDefList* defs)
{
	fprintf(src, "\n// [%s]\n", __FUNCTION__);
	fprintf(src, "export enum OperationIDs {\n");
	bool bElementAdded = false;
	ValueDef* vd;
	FOR_EACH_LIST_ELMT(vd, defs)
	{
		if (bElementAdded)
			fprintf(src, ",\n");
		bElementAdded = PrintTSROSEOperationDefine(src, mod, vd);
	}
	fprintf(src, "\n}\n");
}

void PrintTSROSEInterfaceEntry(FILE* src, ModuleList* mods, ValueDef* vd, bool bAsComment, Module* m)
{
	const char* pszArgument = NULL;
	const char* pszResult = NULL;
	const char* pszError = NULL;

	if (GetROSEDetails(m, vd, &pszArgument, &pszResult, &pszError, NULL, NULL, NULL, true))
	{
		Module* argumentMod = GetImportModuleRefByClassName(pszArgument, mods, m);
		const char* argumentNS = GetNameSpace(argumentMod);
		const char* pszFunction = vd->definedName;
		if (pszResult)
		{
			Module* resultMod = GetImportModuleRefByClassName(pszResult, mods, m);
			const char* resultNS = GetNameSpace(resultMod);
			fprintf(src, bAsComment ? "public " : "\t");
			fprintf(src, "invoke_%s(argument: %s.%s, invokeContext?: ISendInvokeContextParams): Promise<%s.%s", pszFunction, argumentNS, pszArgument, resultNS, pszResult);
			if (pszError)
			{
				Module* errorMod = GetImportModuleRefByClassName(pszError, mods, m);
				const char* pszErrorNS = GetNameSpace(errorMod);
				if (pszErrorNS)
					fprintf(src, " | %s.%s", pszErrorNS, pszError);
			}

			fprintf(src, " | AsnInvokeProblem>");
			fprintf(src, bAsComment ? " {\n\treturn undefined;\n}\n" : ";\n");
		}
		else if (!pszResult)
		{
			fprintf(src, bAsComment ? "public " : "\t");
			fprintf(src, "event_%s(argument: %s.%s, invokeContext?: ISendInvokeContextParams): void", pszFunction, argumentNS, pszArgument);
			fprintf(src, bAsComment ? " {\n}\n" : ";\n");
		}
	}
}

/**
 * Prints an entry of the ROSE table
 * @param src - the file to write into
 * @param mods - the list of laoded modules
 * @param m - the current module we are processing
 * @param vd - the operation inside of m we are processing
 * @param bAsComment - writes the method as comment (for copy paste code)
 * @param bInvokes - true = writes only invokes, false = writes only events
 * @param bAddNewLine - true to add an additional newline in case the content will be written
 */
bool PrintTSROSEHandlerInterfaceEntry(FILE* src, ModuleList* mods, Module* m, ValueDef* vd, bool bAsComment, bool bInvokes, bool bAddNewLine)
{
	const char* pszArgument = NULL;
	const char* pszResult = NULL;
	const char* pszError = NULL;

	if (!GetROSEDetails(m, vd, &pszArgument, &pszResult, &pszError, NULL, NULL, NULL, true))
		return bAddNewLine;

	if ((!bInvokes && pszResult) || (bInvokes && !pszResult))
		return bAddNewLine;

	if (bAddNewLine)
		fprintf(src, "\n");

	const char* pszFunction = vd->definedName;
	if (bAsComment)
	{
		asnoperationcomment operationComment;
		if (GetOperationComment_UTF8(m->moduleName, pszFunction, &operationComment))
		{
			fprintf(src, "/**\n");
			bool bHasShort = strlen(operationComment.szShort) ? true : false;
			bool bHasLong = strlen(operationComment.szLong) ? true : false;
			if (bHasShort)
				printComment(src, " *", operationComment.szShort, "\n");
			if (bHasShort && bHasLong)
				fprintf(src, " *\n");
			if (bHasLong)
				printComment(src, " *", operationComment.szLong, "\n");
			if (!bHasShort && !bHasLong)
				fprintf(src, " * -\n");
			fprintf(src, " *\n");
			asnsequencecomment argumentComment;
			if (GetSequenceComment_UTF8(m->moduleName, pszArgument, &argumentComment))
			{
				fprintf(src, " * @param argument -");
				char* szComment = getNakedCommentDupped(argumentComment.szShort);
				if (szComment)
				{
					if (strlen(szComment))
						fprintf(src, " %s", szComment);
					free(szComment);
				}
				fprintf(src, "\n");
			}
			fprintf(src, " * @param invokeContext - Invokecontext from the asn.1 lib (containing invoke related data)\n");
			if (pszResult)
				fprintf(src, " * @returns - %s on success, %s on error or undefined if the function is not implemented\n", pszResult, pszError);
			fprintf(src, " */\n");
		}
		fprintf(src, "/*\n");
	}
	if (pszResult)
	{
		fprintf(src, bAsComment ? "public async " : "\t");
		Module* argumentMod = GetImportModuleRefByClassName(pszArgument, mods, m);
		const char* argumentNS = GetNameSpace(argumentMod);
		Module* resultMod = GetImportModuleRefByClassName(pszResult, mods, m);
		const char* resultNS = GetNameSpace(resultMod);
		fprintf(src, "onInvoke_%s(argument: %s.%s, invokeContext: IReceiveInvokeContext): Promise<%s.%s", pszFunction, argumentNS, pszArgument, resultNS, pszResult);
		if (pszError)
		{
			Module* errorMod = GetImportModuleRefByClassName(pszError, mods, m);
			const char* errorNS = GetNameSpace(errorMod);
			if (errorNS)
				fprintf(src, " | %s.%s", errorNS, pszError);
		}
		fprintf(src, " | undefined>");
		fprintf(src, bAsComment ? " {\n\treturn undefined;\n}\n" : ";\n");
	}
	else if (!pszResult)
	{
		Module* argumentMod = GetImportModuleRefByClassName(pszArgument, mods, m);
		const char* argumentNS = GetNameSpace(argumentMod);
		fprintf(src, bAsComment ? "public " : "\t");
		fprintf(src, "onEvent_%s(argument: %s.%s, invokeContext: IReceiveInvokeContext): void", pszFunction, argumentNS, pszArgument);
		fprintf(src, bAsComment ? " {\n}\n" : ";\n");
	}
	if (bAsComment)
		fprintf(src, "*/\n");

	return true;
}

void PrintTSROSEImport(FILE* src, ModuleList* mods, Module* mod)
{
	fprintf(src, "// [%s]\n", __FUNCTION__);

	fprintf(src, "import { IReceiveInvokeContext, AsnInvokeProblem } from \"./TSROSEBase%s\";\n", getCommonJSFileExtension());
	fprintf(src, "import { ISendInvokeContextParams } from \"./TSInvokeContext%s\";\n", getCommonJSFileExtension());
	fprintf(src, "// Local imports\n");
	fprintf(src, "import * as %s from \"./%s%s\";\n", GetNameSpace(mod), mod->moduleName, getCommonJSFileExtension());

	PrintTSImports(src, mods, mod, false, false, false);
}

void PrintTSROSEInterface(FILE* src, ModuleList* mods, Module* m)
{
	fprintf(src, "\n// [%s]\n", __FUNCTION__);

	fprintf(src, "export interface I%s {\n", m->ROSEClassName);

	ValueDef* vd;
	FOR_EACH_LIST_ELMT(vd, m->valueDefs)
	{
		if (vd->value->type->basicType->choiceId == BASICTYPE_MACROTYPE)
		{
			if (IsDeprecatedNoOutputOperation(m, vd->definedName))
				continue;
			PrintTSROSEInterfaceEntry(src, mods, vd, false, m);
		}
	}
	fprintf(src, "}\n");
}

void PrintTSROSEHandlerInterface(FILE* src, ModuleList* mods, Module* m)
{
	fprintf(src, "\n// [%s]\n", __FUNCTION__);

	fprintf(src, "// Contains all invokes of the interface (normally the server side)\n");
	fprintf(src, "export interface I%s_Invoke_Handler {\n", m->ROSEClassName);
	fprintf(src, "\t// Allows the implementer to (globally) implement an async local storage (thread local storage) for calls inside the called environment)\n");
	fprintf(src, "\tsetLogContext?(argument: unknown, invokeContext: IReceiveInvokeContext): void;\n");
	ValueDef* vd;
	FOR_EACH_LIST_ELMT(vd, m->valueDefs)
	{
		if (vd->value->type->basicType->choiceId == BASICTYPE_MACROTYPE)
		{
			if (IsDeprecatedNoOutputOperation(m, vd->definedName))
				continue;
			PrintTSROSEHandlerInterfaceEntry(src, mods, m, vd, false, true, false);
		}
	}
	fprintf(src, "}\n\n");

	fprintf(src, "// Contains all events of the interface (normally the client side)\n");
	fprintf(src, "export interface I%s_Event_Handler {\n", m->ROSEClassName);
	fprintf(src, "\t// Allows the implementer to (globally) implement an async local storage (thread local storage) for calls inside the called environment)\n");
	fprintf(src, "\tsetLogContext?(argument: unknown, invokeContext: IReceiveInvokeContext): void;\n");
	FOR_EACH_LIST_ELMT(vd, m->valueDefs)
	{
		if (vd->value->type->basicType->choiceId == BASICTYPE_MACROTYPE)
		{
			if (IsDeprecatedNoOutputOperation(m, vd->definedName))
				continue;
			PrintTSROSEHandlerInterfaceEntry(src, mods, m, vd, false, false, false);
		}
	}
	fprintf(src, "}\n\n");

	fprintf(src, "// Contains all invokes and events of the interface\n");
	fprintf(src, "export type I%s_Handler = I%s_Invoke_Handler & I%s_Event_Handler;\n\n", m->ROSEClassName, m->ROSEClassName, m->ROSEClassName);
}

void PrintTSROSEServerCopyPasteInterface(FILE* src, ModuleList* mods, Module* m)
{
	fprintf(src, "// [%s]\n", __FUNCTION__);

	fprintf(src, "/* Copy paste code for the import statement\n");
	fprintf(src, "import { IReceiveInvokeContext } from \"./TSROSEBase%s\";\n", getCommonJSFileExtension());
	fprintf(src, "import * as ENetUC_Common from \"./ENetUC_Common%s\";\n", getCommonJSFileExtension());
	fprintf(src, "import { %s } from \"./%s%s\";\n", GetNameSpace(m), RemovePath(m->baseFilePath), getCommonJSFileExtension());
	fprintf(src, "*/\n");

	fprintf(src, "\n/**\n");
	fprintf(src, " * Allows to set the log context for an invoke.\n");
	fprintf(src, " * This method is called in advanced of methods handled inside this handler\n");
	fprintf(src, " * The idea is to implement a async local storage based on the provided data from the argument or invokeContext\n");
	fprintf(src, " *\n");
	fprintf(src, " * @param argument - the snacc rose argument\n");
	fprintf(src, " * @param invokeContext - the invoke context\n");
	fprintf(src, " */\n");
	fprintf(src, "/*\n");
	fprintf(src, "public setLogContext(argument: unknown, invokeContext: IReceiveInvokeContext): void {\n");
	fprintf(src, "}\n");
	fprintf(src, "*/\n\n");

	ValueDef* vd;
	bool bAddNewLine = false;
	// Writes invokes
	FOR_EACH_LIST_ELMT(vd, m->valueDefs)
	{
		if (vd->value->type->basicType->choiceId == BASICTYPE_MACROTYPE)
		{
			if (IsDeprecatedNoOutputOperation(m, vd->definedName))
				continue;
			bAddNewLine = PrintTSROSEHandlerInterfaceEntry(src, mods, m, vd, true, true, bAddNewLine);
		}
	}

	// Writes events
	FOR_EACH_LIST_ELMT(vd, m->valueDefs)
	{
		if (vd->value->type->basicType->choiceId == BASICTYPE_MACROTYPE)
		{
			if (IsDeprecatedNoOutputOperation(m, vd->definedName))
				continue;
			bAddNewLine = PrintTSROSEHandlerInterfaceEntry(src, mods, m, vd, true, false, bAddNewLine);
		}
	}
}

void PrintTSROSEConstructor(FILE* src, Module* m)
{
	fprintf(src, "\n\t// [%s]\n", __FUNCTION__);
	fprintf(src, "\t/**\n");
	fprintf(src, "\t * Constructs the invoke and oninvoke object targeting all the ROSE related parts.\n");
	fprintf(src, "\t *\n");
	fprintf(src, "\t * @param transport - The transport is the connection to the other side. It takes care of delivering the invoke\n");
	fprintf(src, "\t * to us as well as to send invokes and events to the other side. It also holds the logger.\n");
	fprintf(src, "\t * @param handleEvents - Set this to true if you want to receive events or false if the stub should cached them\n");
	fprintf(src, "\t * until you call dispatchEvents();\n");
	fprintf(src, "\t * @param handler - The handler takes care of handling methods that are exposed through the ASN1 file\n");
	fprintf(src, "\t * The outer ROSE envelop specifies the function that is called. The server looks for an appropriate handler\n");
	fprintf(src, "\t * and calls the handler for the operation. Inside the operation the argument is decoded. Once the handling of the\n");
	fprintf(src, "\t * operation is done the result (error) is encoded and handed back to the callee, embedded in the ROSE envelop and send\n");
	fprintf(src, "\t * back to the other side. If a certain function is not register the function call will fail with not function not implemented\n");
	fprintf(src, "\t */\n");

	fprintf(src, "\tpublic constructor(transport: IASN1Transport, handleEvents: boolean, handler?: Partial<I%s_Handler>) {\n", m->ROSEClassName);
	fprintf(src, "\t\tsuper(transport, handleEvents);\n");

	fprintf(src, "\n\t\tthis.logFilter = [");

	const char* szBaseName = RemovePath(m->baseFilePath);
	const char* szFilter = GetFirstModuleLogFileFilter(szBaseName);
	if (szFilter)
	{
		do
		{
			fprintf(src, "\n\t\t\"%s\"", szFilter);
			szFilter = GetNextModuleLogFileFilter(szBaseName);
			if (szFilter)
				fprintf(src, ",");
		} while (szFilter);
		fprintf(src, "\n\t");
	}
	fprintf(src, "];\n\n");

	fprintf(src, "\t\tif (handler)\n");
	fprintf(src, "\t\t\tthis.setHandler(handler);\n");
	fprintf(src, "\t}\n");
}

void PrintTSROSESetHandler(FILE* src, Module* m)
{
	fprintf(src, "\n\t// [%s]\n", __FUNCTION__);
	fprintf(src, "\t/**\n");
	fprintf(src, "\t * Sets the handler and registers the operations with it\n");
	fprintf(src, "\t *\n");
	fprintf(src, "\t * @param handler - The handler takes care of handling methods that are exposed through the ASN1 file\n");
	fprintf(src, "\t * The outer ROSE envelop specifies the function that is called. The server looks for an appropriate handler\n");
	fprintf(src, "\t * and calls the handler for the operation. Inside the operation the argument is decoded. Once the handling of the\n");
	fprintf(src, "\t * operation is done the result (error) is encoded and handed back to the callee, embedded in the ROSE envelop and send\n");
	fprintf(src, "\t * back to the other side. If a certain function is not register the function call will fail with not function not implemented\n");
	fprintf(src, "\t */\n");

	fprintf(src, "\tpublic setHandler(handler: Partial<I%s_Handler>): void {\n", m->ROSEClassName);
	ValueDef* vd;
	FOR_EACH_LIST_ELMT(vd, m->valueDefs)
	{
		if (IsROSEValueDef(m, vd))
			fprintf(src, "\t\tthis.transport.registerOperation(this, handler, OperationIDs.OPID_%s, \"%s\");\n", vd->definedName, vd->definedName);
	}

	if (gMajorInterfaceVersion >= 0)
	{
		long long lMinorVersion = GetModuleMinorVersion(m->moduleName);
		fprintf(src, "\t\tthis.transport.registerModuleVersion(\"%s\", %i, %lld);\n", m->moduleName, gMajorInterfaceVersion, lMinorVersion);
	}

	fprintf(src, "\t}\n");
}

void PrintTSROSEOnInvokeswitchCaseEntry(FILE* src, ModuleList* mods, int bEvents, ValueDef* vd, Module* m)
{
	const char* pszArgument = NULL;
	const char* pszResult = NULL;
	const char* pszError = NULL;
	if (GetROSEDetails(m, vd, &pszArgument, &pszResult, &pszError, NULL, NULL, NULL, true))
	{
		Module* argumentMod = GetImportModuleRefByClassName(pszArgument, mods, m);
		const char* szArgumentNS = GetNameSpace(argumentMod);

		Module* resultMod = NULL;
		const char* szResultNS = NULL;
		if (pszResult)
		{
			resultMod = GetImportModuleRefByClassName(pszResult, mods, m);
			szResultNS = GetNameSpace(resultMod);
		}

		const char* pszFunction = vd->definedName;
		bool bDeprecated = IsDeprecatedFlaggedOperation(m, pszFunction);

		if (pszResult && !bEvents)
		{
			fprintf(src, "\t\t\tcase OperationIDs.OPID_%s:\n", pszFunction);
			if (bDeprecated)
			{
				asnoperationcomment comment;
				GetOperationComment_UTF8(m->moduleName, vd->definedName, &comment);
				fprintf(src, "\t\t\t\tTSDeprecatedCallback.deprecatedMethod(%lld, this.getLogData().className, \"%s\", \"IN\", invokeContext);\n", comment.i64Deprecated, pszFunction);
			}
			fprintf(src, "\t\t\t\treturn await this.handleOnInvoke(invoke, ");
			fprintf(src, "OperationIDs.OPID_%s, ", pszFunction);
			fprintf(src, "%s.%s, ", szArgumentNS, pszArgument);
			if (argumentMod == m)
				fprintf(src, "Converter.%s_Converter, ", pszArgument);
			else
				fprintf(src, "%s_Converter.%s_Converter, ", szArgumentNS, pszArgument);
			if (resultMod == m)
				fprintf(src, "Converter.%s_Converter, ", pszResult);
			else
				fprintf(src, "%s_Converter.%s_Converter, ", szResultNS, pszResult);
			fprintf(src, "handler, ");
			fprintf(src, "handler.onInvoke_%s, ", pszFunction);
			fprintf(src, "invokeContext);\n");
		}
		else if (!pszResult && bEvents)
		{
			fprintf(src, "\t\t\tcase OperationIDs.OPID_%s:\n", pszFunction);
			if (bDeprecated)
			{
				asnoperationcomment comment;
				GetOperationComment_UTF8(m->moduleName, vd->definedName, &comment);
				fprintf(src, "\t\t\t\tTSDeprecatedCallback.deprecatedMethod(%lld, this.getLogData().className, \"%s\", \"IN\", invokeContext);\n", comment.i64Deprecated, pszFunction);
			}
			fprintf(src, "\t\t\t\treturn await this.handleOnEvent(invoke, ");
			fprintf(src, "OperationIDs.OPID_%s, ", pszFunction);
			fprintf(src, "%s.%s, ", szArgumentNS, pszArgument);
			if (argumentMod == m)
				fprintf(src, "Converter.%s_Converter, ", pszArgument);
			else
				fprintf(src, "%s_Converter.%s_Converter, ", szArgumentNS, pszArgument);
			fprintf(src, "handler, ");
			fprintf(src, "handler.onEvent_%s, ", pszFunction);
			fprintf(src, "invokeContext);\n");
		}
	}
}

void PrintTSROSEOnInvokeswitchCase(FILE* src, ModuleList* mods, Module* m)
{
	fprintf(src, "\n\t// [%s]\n", __FUNCTION__);
	fprintf(src, "\t/**\n");
	fprintf(src, "\t * This is the central onInvoke method that is called whenever a method of this module is called.\n");
	fprintf(src, "\t * Based on the operationID we step into the decoding of the method argument and call the method in the handler.\n");
	fprintf(src, "\t * The result is then again encoded and send to the other side.\n");
	fprintf(src, "\t *\n");
	fprintf(src, "\t * @param invoke - The (ROSE) decoded invoke which also contains the function argument (not yet decoded). The\n");
	fprintf(src, "\t * operationID is the one that defines which function we call. In the switch case we decode the methods argument\n");
	fprintf(src, "\t * and call the metho in the handler.\n");
	fprintf(src, "\t * @param invokeContext - The invoke related contextual data (see IReceiveInvokeContext)\n");
	fprintf(src, "\t * @param handler - This object is handling the invoke after having successfully decoded the argument.\n");
	fprintf(src, "\t * it contains the methods as defined in the asn.1 files.\n");
	fprintf(src, "\t * @returns ROSEReject if the request was not handled, ROSEResult for the invoke result, ROSEError for an error or undefined if an event was called\n");
	fprintf(src, "\t */\n");
	fprintf(src, "\tpublic async onInvoke(invoke: ROSEInvoke, invokeContext: IReceiveInvokeContext, handler: I%s_Handler): Promise<ROSEReject | ROSEResult | ROSEError | undefined> {\n", m->ROSEClassName);
	fprintf(src, "\t\tswitch (invoke.operationID) {\n");

	ValueDef* vd;
	FOR_EACH_LIST_ELMT(vd, m->valueDefs)
	{
		if (vd->value->type->basicType->choiceId == BASICTYPE_MACROTYPE)
		{
			if (IsDeprecatedNoOutputOperation(m, vd->definedName))
				continue;
			PrintTSROSEOnInvokeswitchCaseEntry(src, mods, 0, vd, m);
		}
	}

	int addedOne = 0;
	FOR_EACH_LIST_ELMT(vd, m->valueDefs)
	{
		if (vd->value->type->basicType->choiceId == BASICTYPE_MACROTYPE)
		{
			if (IsDeprecatedNoOutputOperation(m, vd->definedName))
				continue;
			const char* pszFunction = vd->definedName;

			const char* pszResult = NULL;
			GetROSEDetails(m, vd, NULL, &pszResult, NULL, NULL, NULL, NULL, true);
			if (!pszResult)
			{
				addedOne = 1;
				fprintf(src, "\t\t\tcase OperationIDs.OPID_%s:\n", pszFunction);
			}
		}
	}
	if (addedOne)
		fprintf(src, "\t\t\t\treturn this.onEvent(invoke, invokeContext, handler);\n");

	fprintf(src, "\t\t\tdefault:\n");
	fprintf(src, "\t\t\t\t// If you land here stub of client and server are incompatible...\n");
	fprintf(src, "\t\t\t\tdebugger;\n");
	fprintf(src, "\t\t\t\treturn createInvokeReject(invoke, AsnInvokeProblemEnum.unrecognisedOperation, `${invoke.operationID} (\"${invoke.operationName}\") is not a function of %s`);\n", m->ROSEClassName);
	fprintf(src, "\t\t}\n");
	fprintf(src, "\t}\n");
}

void PrintTSROSEOnEventSwitchCase(FILE* src, ModuleList* mods, Module* m)
{
	fprintf(src, "\n\t// [%s]\n", __FUNCTION__);
	fprintf(src, "\t/**\n");
	fprintf(src, "\t * This is the onEvent method that is called whenever an event is called in this module.\n");
	fprintf(src, "\t * It is called from the onInvoke in case of an event is being called.\n");
	fprintf(src, "\t * Depending on the handleEvents flag the event is either handled or cached.\n");
	fprintf(src, "\t *\n");
	fprintf(src, "\t * @param invoke - The (ROSE) decoded invoke which also contains the function argument (not yet decoded). The\n");
	fprintf(src, "\t * operationID is the one that defines which function we call. In the switch case we decode the methods argument\n");
	fprintf(src, "\t * and call the method in the handler.\n");
	fprintf(src, "\t * @param invokeContext - The invoke related contextual data (see IReceiveInvokeContext)\n");
	fprintf(src, "\t * @param handler - This object is handling the invoke after having successfully decoded the argument.\n");
	fprintf(src, "\t * it contains the methods as defined in the asn.1 files.\n");
	fprintf(src, "\t * @returns ROSEReject if the request was not handled or undefined\n");
	fprintf(src, "\t */\n");
	fprintf(src, "\tprivate async onEvent(invoke: ROSEInvoke, invokeContext: IReceiveInvokeContext, handler: I%s_Handler): Promise<ROSEReject | undefined> {\n", m->ROSEClassName);
	fprintf(src, "\t\t// If the class says do not handle events and the override flag in the invokeContext has not been set, add the event to the que, otherwise we dispatch it\n");
	fprintf(src, "\t\tif (!this.handleEvents && !invokeContext?.handleEvent) {\n");
	fprintf(src, "\t\t\tthis.transport.log(ELogSeverity.debug, \"Adding event to queue\", \"onEvent\", this, { operationName: invoke.operationName, operationID: invoke.operationID });\n");
	fprintf(src, "\t\t\tthis.cachedEvents.push({ invoke, invokeContext, handler });\n");
	fprintf(src, "\t\t\treturn;\n");
	fprintf(src, "\t\t}\n\n");

	fprintf(src, "\t\tswitch (invoke.operationID) {\n");

	ValueDef* vd;
	FOR_EACH_LIST_ELMT(vd, m->valueDefs)
	{
		if (vd->value->type->basicType->choiceId == BASICTYPE_MACROTYPE)
		{
			if (IsDeprecatedNoOutputOperation(m, vd->definedName))
				continue;
			PrintTSROSEOnInvokeswitchCaseEntry(src, mods, 1, vd, m);
		}
	}

	fprintf(src, "\t\t\tdefault:\n");
	fprintf(src, "\t\t\t\t// If you land here stub of client and server are incompatible...\n");
	fprintf(src, "\t\t\t\tdebugger;\n");
	fprintf(src, "\t\t\t\treturn createInvokeReject(invoke, AsnInvokeProblemEnum.unrecognisedOperation, `${invoke.operationID} (\"${invoke.operationName}\") is not a function of %s`);\n", m->ROSEClassName);
	fprintf(src, "\t\t}\n");
	fprintf(src, "\t}\n");
}

bool PrintTSROSEInvokeMethod(FILE* src, ModuleList* mods, int bEvents, ValueDef* vd, Module* m)
{
	const char* pszArgument = NULL;
	const char* pszResult = NULL;
	const char* pszError = NULL;
	if (GetROSEDetails(m, vd, &pszArgument, &pszResult, &pszError, NULL, NULL, NULL, true))
	{
		Module* argumentMod = GetImportModuleRefByClassName(pszArgument, mods, m);
		const char* szArgumentNS = GetNameSpace(argumentMod);

		const char* szResultNS = NULL;
		Module* resultMod = NULL;
		if (pszResult)
		{
			resultMod = GetImportModuleRefByClassName(pszResult, mods, m);
			szResultNS = GetNameSpace(resultMod);
		}

		Module* errorMod = NULL;
		if (pszError)
			errorMod = GetImportModuleRefByClassName(pszError, mods, m);

		const char* pszFunction = vd->definedName;
		bool bDeprecated = false;

		if ((pszResult && !bEvents) || (!pszResult && bEvents))
		{
			fprintf(src, "\n\t// [%s]\n", __FUNCTION__);
			{
				asnoperationcomment operationComment;
				if (GetOperationComment_UTF8(m->moduleName, pszFunction, &operationComment))
				{
					fprintf(src, "\t/**\n");
					bool bHasShort = strlen(operationComment.szShort) ? true : false;
					bool bHasLong = strlen(operationComment.szLong) ? true : false;
					if (bHasShort)
						printComment(src, "\t *", operationComment.szShort, "\n");
					if (bHasLong)
						printComment(src, "\t *", operationComment.szLong, "\n");
					if (bHasShort || bHasLong)
						fprintf(src, "\t *\n");
					if (operationComment.i64Deprecated || operationComment.i64Added || operationComment.iPrivate)
					{
						if (operationComment.i64Deprecated)
						{
							fprintf(src, "\t * @deprecated %s\n", getDeprecated(operationComment.szDeprecated, COMMENTSTYLE_TYPESCRIPT));
							bDeprecated = true;
						}
						if (operationComment.i64Added)
						{
							char* szTime = ConvertUnixTimeToReadable(operationComment.i64Added);
							fprintf(src, "\t * @added %s\n", szTime);
							free(szTime);
						}
						if (operationComment.iPrivate)
							fprintf(src, "\t * @private\n");
						fprintf(src, "\t *\n");
					}

					fprintf(src, "\t * @param argument - An %s object containing all the relevant parameters for the call\n", pszArgument);
					fprintf(src, "\t * @param invokeContext - Invoke related contextual data (e.g. a clientConnectionID)\n");
					if (pszResult && !bEvents)
						fprintf(src, "\t * @returns a Promise resolving into %s, an AsnRequestError or AsnInvokeProblem object\n", pszResult);
					else if (bEvents)
						fprintf(src, "\t * @returns undefined or, if bSendEventSynchronous has been set true when the event was sent\n");
					fprintf(src, "\t */\n");
				}
			}

			if (pszResult && !bEvents)
			{
				fprintf(src, "\tpublic async invoke_%s(argument: %s.%s, invokeContext?: ISendInvokeContextParams): Promise<%s.%s", pszFunction, szArgumentNS, pszArgument, szResultNS, pszResult);
				const char* szErrorNS = GetNameSpace(errorMod);
				if (szErrorNS && pszError)
					fprintf(src, " | %s.%s", szErrorNS, pszError);
				fprintf(src, " | AsnInvokeProblem> {\n");
				if (bDeprecated)
				{
					asnoperationcomment comment;
					GetOperationComment_UTF8(m->moduleName, vd->definedName, &comment);
					fprintf(src, "\t\tTSDeprecatedCallback.deprecatedMethod(%lld, this.getLogData().className, \"%s\", \"OUT\", invokeContext);\n", comment.i64Deprecated, pszFunction);
				}
				fprintf(src, "\t\treturn this.handleInvoke(argument, ");
				fprintf(src, "%s.%s, ", szResultNS, pszResult);
				fprintf(src, "OperationIDs.OPID_%s, ", pszFunction);
				fprintf(src, "\"%s\", ", pszFunction);
				if (argumentMod == m)
					fprintf(src, "Converter.%s_Converter, ", pszArgument);
				else
					fprintf(src, "%s_Converter.%s_Converter, ", szArgumentNS, pszArgument);
				if (resultMod == m)
					fprintf(src, "Converter.%s_Converter, ", pszResult);
				else
					fprintf(src, "%s_Converter.%s_Converter, ", szResultNS, pszResult);
				fprintf(src, "invokeContext");

				if (pszError && strcmp(pszError, "AsnRequestError") != 0)
				{
					// Custom error, so we need to advertise the converter
					if (errorMod == m)
						fprintf(src, ", Converter.%s_Converter", pszError);
					else
						fprintf(src, ", %s_Converter.%s_Converter", szErrorNS, pszError);
				}
				fprintf(src, ");\n");

				fprintf(src, "\t}\n");
			}
			else if (!pszResult && bEvents)
			{
				fprintf(src, "\tpublic event_%s(argument: %s.%s, invokeContext?: ISendInvokeContextParams): undefined | boolean {\n", pszFunction, szArgumentNS, pszArgument);
				if (bDeprecated)
				{
					asnoperationcomment comment;
					GetOperationComment_UTF8(m->moduleName, vd->definedName, &comment);
					fprintf(src, "\t\tTSDeprecatedCallback.deprecatedMethod(%lld, this.getLogData().className, \"%s\", \"OUT\", invokeContext);\n", comment.i64Deprecated, pszFunction);
				}
				fprintf(src, "\t\treturn this.handleEvent(argument, ");
				fprintf(src, "OperationIDs.OPID_%s, ", pszFunction);
				fprintf(src, "\"%s\", ", pszFunction);
				if (argumentMod == m)
					fprintf(src, "Converter.%s_Converter, ", pszArgument);
				else
					fprintf(src, "%s_Converter.%s_Converter, ", szArgumentNS, pszArgument);
				fprintf(src, "invokeContext);\n");
				fprintf(src, "\t}\n");
			}
			return true;
		}
	}
	return false;
}

void PrintTSROSEInvokeMethods(FILE* src, ModuleList* mods, Module* m)
{
	fprintf(src, "\n\t// [%s]\n", __FUNCTION__);

	ValueDef* vd;
	FOR_EACH_LIST_ELMT(vd, m->valueDefs)
	{
		if (vd->value->type->basicType->choiceId == BASICTYPE_MACROTYPE)
		{
			if (IsDeprecatedNoOutputOperation(m, vd->definedName))
				continue;
			PrintTSROSEInvokeMethod(src, mods, 0, vd, m);
		}
	}
	FOR_EACH_LIST_ELMT(vd, m->valueDefs)
	{
		if (vd->value->type->basicType->choiceId == BASICTYPE_MACROTYPE)
		{
			if (IsDeprecatedNoOutputOperation(m, vd->definedName))
				continue;
			PrintTSROSEInvokeMethod(src, mods, 1, vd, m);
		}
	}
}

void PrintTSROSEInterfaceCode(FILE* src, ModuleList* mods, Module* m)
{
	fprintf(src, "// [%s]\n", __FUNCTION__);

	// Comments
	PrintTSROSEHeader(src, m, true);

	// Import definition
	PrintTSROSEImport(src, mods, m);

	// Root types
	PrintTSRootTypes(src, m, "ROSEInterface");

	// ClientInterface definition
	PrintTSROSEInterface(src, mods, m);

	// Server Interface definition
	PrintTSROSEHandlerInterface(src, mods, m);

	// Server Interface copy paste code definition
	PrintTSROSEServerCopyPasteInterface(src, mods, m);
}

void PrintTSROSEModuleComment(FILE* src, Module* m)
{
	fprintf(src, "\n// [%s]\n", __FUNCTION__);
	printModuleComment(src, m->moduleName, COMMENTSTYLE_TYPESCRIPT);
}

bool hasEvents(Module* m)
{
	bool bHasEvents = false;
	ValueDef* vd;
	FOR_EACH_LIST_ELMT(vd, m->valueDefs)
	{
		if (vd->value->type->basicType->choiceId == BASICTYPE_MACROTYPE)
		{
			const char* pszResult = NULL;
			GetROSEDetails(m, vd, NULL, &pszResult, NULL, NULL, NULL, NULL, true);
			if (!pszResult)
			{
				bHasEvents = true;
				break;
			}
		}
	}
	return bHasEvents;
}

void PrintTSROSEClass(FILE* src, ModuleList* mods, Module* m)
{
	fprintf(src, "\n// [%s]\n", __FUNCTION__);
	fprintf(src, "export class %s extends ROSEBase implements IInvokeHandler, I%s {\n", m->ROSEClassName, m->ROSEClassName);

	fprintf(src, "\t/**\n");
	fprintf(src, "\t * Contains the attributes that have to be filtered from logging\n");
	fprintf(src, "\t * Use logfilter property;property inside the asn1 root comments to specify this list\n");
	fprintf(src, "\t */\n");
	fprintf(src, "\tpublic readonly logFilter: string[];\n\n");

	fprintf(src, "\t/**\n");
	fprintf(src, "\t * The Loggers getLogData callback (used in all the log methods called in this class, add the classname to every log entry)\n");
	fprintf(src, "\t *\n");
	fprintf(src, "\t * @returns - an ILogData log data object provided additional data for all the logger calls in this class\n");
	fprintf(src, "\t */\n");
	fprintf(src, "\tpublic getLogData(): IASN1LogData {\n");
	fprintf(src, "\t\treturn {\n");
	fprintf(src, "\t\t\tclassName: MODULE_NAME\n");
	fprintf(src, "\t\t};\n");
	fprintf(src, "\t}\n\n");

	fprintf(src, "\t/**\n");
	fprintf(src, "\t * Returns the operationName for an operationID\n");
	fprintf(src, "\t *\n");
	fprintf(src, "\t * @param id - the id we want to have the name for\n");
	fprintf(src, "\t * @returns - the name or undefined if not found\n");
	fprintf(src, "\t */\n");
	fprintf(src, "\tpublic getNameForOperationID(id: OperationIDs): string | undefined {\n");
	fprintf(src, "\t\tswitch (id) {\n");
	ValueDef* v;
	FOR_EACH_LIST_ELMT(v, m->valueDefs)
	{
		/* just do ints */
		if (v->value->basicValue->choiceId != BASICVALUE_INTEGER)
			continue;
		if (v->value->type->basicType->choiceId != BASICTYPE_MACROTYPE)
			continue;
		if (v->value->type->basicType->a.macroType->choiceId != MACROTYPE_ROSOPERATION)
			continue;
		if (IsDeprecatedNoOutputOperation(m, v->definedName))
			continue;

		fprintf(src, "\t\t\tcase OperationIDs.OPID_%s:\n", v->definedName);
		fprintf(src, "\t\t\t\treturn \"%s\";\n", v->definedName);
	}
	fprintf(src, "\t\t\tdefault:\n");
	fprintf(src, "\t\t\t\treturn undefined;\n");
	fprintf(src, "\t\t}\n");
	fprintf(src, "\t}\n\n");

	fprintf(src, "\t/**\n");
	fprintf(src, "\t * Returns the operationID for an operationName\n");
	fprintf(src, "\t *\n");
	fprintf(src, "\t * @param name - the name we want to have the id for\n");
	fprintf(src, "\t * @returns - the id or undefined if not found\n");
	fprintf(src, "\t */\n");
	fprintf(src, "\tpublic getIDForOperationName(name: string): OperationIDs | undefined {\n");
	fprintf(src, "\t\tswitch (name) {\n");
	FOR_EACH_LIST_ELMT(v, m->valueDefs)
	{
		/* just do ints */
		if (v->value->basicValue->choiceId != BASICVALUE_INTEGER)
			continue;
		if (v->value->type->basicType->choiceId != BASICTYPE_MACROTYPE)
			continue;
		if (v->value->type->basicType->a.macroType->choiceId != MACROTYPE_ROSOPERATION)
			continue;
		if (IsDeprecatedNoOutputOperation(m, v->definedName))
			continue;

		fprintf(src, "\t\t\tcase \"%s\":\n", v->definedName);
		fprintf(src, "\t\t\t\treturn OperationIDs.OPID_%s;\n", v->definedName);
	}
	fprintf(src, "\t\t\tdefault:\n");
	fprintf(src, "\t\t\t\treturn undefined;\n");
	fprintf(src, "\t\t}\n");
	fprintf(src, "\t}\n");

	PrintTSROSEConstructor(src, m);
	PrintTSROSESetHandler(src, m);
	PrintTSROSEInvokeMethods(src, mods, m);
	PrintTSROSEOnInvokeswitchCase(src, mods, m);
	if (hasEvents(m))
		PrintTSROSEOnEventSwitchCase(src, mods, m);
	fprintf(src, "}\n");
}

void PrintTSROSECode(FILE* src, ModuleList* mods, Module* m)
{
	PrintTSROSEHeader(src, m, false);
	PrintTSROSEImports(src, mods, m);
	PrintTSRootTypes(src, m, "ROSE");
	PrintTSROSEOperationDefines(src, m, m->valueDefs);
	PrintTSROSEModuleComment(src, m);
	PrintTSROSEClass(src, mods, m);
}