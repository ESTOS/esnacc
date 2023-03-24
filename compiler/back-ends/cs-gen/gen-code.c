/*
 *   compiler/back_ends/cs-gen/gen_code.c - routines for printing C# code from type trees
 *
 *
 */

#include "gen-code.h"

#include "../../../c-lib/include/print.h"
#include "../tag-util.h"  /* get GetTags/FreeTags/CountTags/TagByteLen */
#include "../str-util.h"

#if META
#include "meta.h"
#endif

int GetROSECSDetails(ValueDef *vd, char** ppszArgument, char** ppszResult, char** ppszError)
{
	int bRetval = 0;
	if (vd->value->type != NULL)
	{
		if (vd->value->type->basicType->choiceId == BASICTYPE_MACROTYPE)
		{
			if (vd->value->type->basicType->a.macroType->choiceId == MACROTYPE_ROSOPERATION)
			{
				if (vd->value->type->basicType->a.macroType->a.rosOperation->arguments &&
					((vd->value->type->basicType->a.macroType->a.rosOperation->arguments->type->basicType->choiceId == BASICTYPE_LOCALTYPEREF &&
					vd->value->type->basicType->a.macroType->a.rosOperation->arguments->type->basicType->a.localTypeRef->typeName) ||
					(vd->value->type->basicType->a.macroType->a.rosOperation->arguments->type->basicType->choiceId == BASICTYPE_IMPORTTYPEREF &&
					vd->value->type->basicType->a.macroType->a.rosOperation->arguments->type->basicType->a.importTypeRef->typeName)
					))
				{
					//there is an argument
					if (vd->value->type->basicType->a.macroType->a.rosOperation->arguments->type->basicType->choiceId == BASICTYPE_LOCALTYPEREF &&
						vd->value->type->basicType->a.macroType->a.rosOperation->arguments->type->basicType->a.localTypeRef->typeName)
					{
						*ppszArgument = vd->value->type->basicType->a.macroType->a.rosOperation->arguments->type->basicType->a.localTypeRef->typeName;
					}
					else if (vd->value->type->basicType->a.macroType->a.rosOperation->arguments->type->basicType->choiceId == BASICTYPE_IMPORTTYPEREF &&
						vd->value->type->basicType->a.macroType->a.rosOperation->arguments->type->basicType->a.importTypeRef->typeName)
					{
						*ppszArgument = vd->value->type->basicType->a.macroType->a.rosOperation->arguments->type->basicType->a.importTypeRef->typeName;
					}


					*ppszResult = NULL;
					*ppszError = NULL;

					if (vd->value->type->basicType->a.macroType->a.rosOperation->result)
					{
						if (vd->value->type->basicType->a.macroType->a.rosOperation->result->type->basicType->choiceId == BASICTYPE_LOCALTYPEREF &&
							vd->value->type->basicType->a.macroType->a.rosOperation->result->type->basicType->a.localTypeRef->typeName)
						{
							*ppszResult = vd->value->type->basicType->a.macroType->a.rosOperation->result->type->basicType->a.localTypeRef->typeName;
						}
						else if (vd->value->type->basicType->a.macroType->a.rosOperation->result->type->basicType->choiceId == BASICTYPE_IMPORTTYPEREF &&
							vd->value->type->basicType->a.macroType->a.rosOperation->result->type->basicType->a.importTypeRef->typeName)
						{
							*ppszResult = vd->value->type->basicType->a.macroType->a.rosOperation->result->type->basicType->a.importTypeRef->typeName;
						}
					}

					if (vd->value->type->basicType->a.macroType->a.rosOperation->errors && 
						vd->value->type->basicType->a.macroType->a.rosOperation->errors->count)
					{
						TypeOrValue *first = (TypeOrValue*)FIRST_LIST_ELMT (vd->value->type->basicType->a.macroType->a.rosOperation->errors);
						if (first->choiceId == TYPEORVALUE_TYPE)
						{
							if (first->a.type->basicType->choiceId == BASICTYPE_LOCALTYPEREF && 
								first->a.type->basicType->a.localTypeRef->typeName)
							{
								//local defined
								*ppszError = first->a.type->basicType->a.localTypeRef->typeName;
							} 
							else if (first->a.type->basicType->choiceId == BASICTYPE_IMPORTTYPEREF && 
								first->a.type->basicType->a.importTypeRef->typeName)
							{
								//imported
								*ppszError = first->a.type->basicType->a.importTypeRef->typeName;
							}
						}
					}
					bRetval = 1;
				}
			}
		}
	}
	return bRetval;
}


static void PrintCSValueDefsName(FILE *f, ValueDef *v)
{
	char *cName;
	cName = Asn1ValueName2CValueName (v->definedName);
	fprintf (f, "%s", cName);
	Free (cName);
}

void PrintROSECSOperationDefines(FILE *hdr, ValueDef *v)
{
    /* just do ints */
    if (v->value->basicValue->choiceId != BASICVALUE_INTEGER) {
        return;
	}

	if (v->value->type->basicType->choiceId != BASICTYPE_MACROTYPE) {
		return;
	}

	if (v->value->type->basicType->a.macroType->choiceId != MACROTYPE_ROSOPERATION) {
		return;
	}

	/*
	* put instantiation in hdr file
	*/
	fprintf (hdr, "\t\t\tpublic const int OPID_");
	PrintCSValueDefsName (hdr, v);
	fprintf (hdr, " = %d;\n", v->value->basicValue->a.integer);
}

/*
 * prints PrintROSEOnInvokeCS
 */
static void PrintROSEOnInvokeCS(FILE *src,
    int bEvents,
    ValueDef *vd)
{
	char* pszArgument = NULL;
	char* pszResult = NULL;
	char* pszError = NULL;
	char* source = NULL;
	int i = 0;

	if (GetROSECSDetails(vd, &pszArgument, &pszResult, &pszError))
	{
		if (pszResult && !bEvents)
		{
			//there is a result -> it is a Funktion
			//Header
			if (pszError)
			{
				fprintf(src, "\tpublic virtual InvokeResult OnInvoke_%s(%s argument, %s result, %s error) { return InvokeResult.returnReject; }\n", vd->definedName,
								pszArgument, pszResult, pszError);
			}
			else
			{
				fprintf(src, "\tpublic virtual InvokeResult OnInvoke_%s(%s argument, %s result) { return InvokeResult.returnReject; }\n", vd->definedName,
								pszArgument, pszResult);
			}
		}
		else if (!pszResult && bEvents)
		{
			//there is no result -> it is an Event
			//Header
			//fprintf(src, "\tpublic virtual void OnEvent_%s(%s argument) { }\n", vd->definedName, szArgument);
			fprintf(src, "\tpublic delegate void %s(object sender, %s argument);\n", vd->definedName, pszArgument);
			
			source = vd->definedName;
			for(i=0;i<3;i++) 
			{
				source++;
			}

			fprintf(src, "\tpublic event %s %s;\n", vd->definedName, source);
			fprintf(src, "\tprivate void OnEvent_%s(%s argument)\n", vd->definedName, pszArgument);
			fprintf(src, "\t{\n");
			fprintf(src, "\t\t%s handler = %s;\n",vd->definedName, source);
			fprintf(src, "\t\tif (null != handler)\n");
			fprintf(src, "\t\t{\n");
			fprintf(src, "\t\t\tforeach (%s singleCast in handler.GetInvocationList())\n", vd->definedName);
			fprintf(src, "\t\t\t{\n");
			fprintf(src, "\t\t\t\tISynchronizeInvoke syncInvoke = singleCast.Target as ISynchronizeInvoke;\n");
			fprintf(src, "\t\t\t\ttry\n");
			fprintf(src, "\t\t\t\t{\n");
			fprintf(src, "\t\t\t\t\tif ((null != syncInvoke) && (syncInvoke.InvokeRequired))\n");
			fprintf(src, "\t\t\t\t\t\t\tsyncInvoke.Invoke(singleCast, new object[] { this, argument });\n");
			fprintf(src, "\t\t\t\t\telse\n");
			fprintf(src, "\t\t\t\t\t\t\tsingleCast(this, argument);\n");
			fprintf(src, "\t\t\t\t}\n");
			fprintf(src, "\t\t\t\tcatch (Exception ex) { m_Log.ErrorException(\"OnEvent_%s\", ex); }\n", vd->definedName);
			fprintf(src, "\t\t\t}\n");
			fprintf(src, "\t\t}\n");
			fprintf(src, "\t}\n\n");


		}
	}
} /* PrintROSEOnInvokeCS */


/*
 * prints PrintROSEOnInvokeswitchCase
 */
static void PrintROSEOnInvokeswitchCaseCS(FILE *src, int bEvents, ValueDef *vd)
{
	char* pszArgument = NULL;
	char* pszResult = NULL;
	char* pszError = NULL;

	if (GetROSECSDetails(vd, &pszArgument, &pszResult, &pszError))
	{
		//there is an argument
		if (pszResult && !bEvents)
		{
			//there is a result -> it is a Funktion
			//Source here
			
			fprintf(src, "\t\t\tcase OPID_%s:\n", vd->definedName);
			fprintf(src, "\t\t\t\t{\n");
			fprintf(src, "\t\t\t\t\t%s argument = new %s();\n", pszArgument, pszArgument);
			fprintf(src, "\t\t\t\t\t%s result = new %s();\n", pszResult, pszResult);
			if (pszError) {
				fprintf(src, "\t\t\t\t\t%s error = new %s();\n", pszError, pszError);
			}
			
			fprintf(src, "\t\t\t\t\targument.Decode(decodeBuffer);\n");
			fprintf(src, "\t\t\t\t\tDateTime dtStart = DateTime.UtcNow;\n");
			fprintf(src, "\t\t\t\t\tif (m_Log.IsDebugEnabled) { m_Log.Debug(\"%s InvokeID:\" + invoke.invokeID.mValue.ToString()); }\n", vd->definedName);
			//fprintf(src, "\t\t\t\t\tm_Log.Debug(\"%s\",invoke, argument);\n", vd->definedName);
			if (pszError)
			{
				fprintf(src, "\t\t\t\t\tInvokeResult invokeResult = OnInvoke_%s (argument, result, error);\n",vd->definedName);
				fprintf(src, "\t\t\t\t\tlRoseResult = InvokeParseResult(invokeResult,OPID_%s , invoke.invokeID, result, error);\n",vd->definedName);
			}
			else
			{
				fprintf(src, "\t\t\t\t\tInvokeResult invokeResult = OnInvoke_%s (argument, result);\n",vd->definedName);
				fprintf(src, "\t\t\t\t\tlRoseResult = InvokeParseResult(invokeResult,OPID_%s , invoke.invokeID, result);\n",vd->definedName);
			}
			fprintf(src, "\t\t\t\t\tDateTime dtEnd = DateTime.UtcNow;\n");
			fprintf(src, "\t\t\t\t\tif (m_Log.IsDebugEnabled) { m_Log.Debug(\"%s InvokeID:{0} {1}ms\", invoke.invokeID.mValue.ToString(), (dtEnd-dtStart).TotalMilliseconds); }\n", vd->definedName);
			fprintf(src, "\t\t\t\t}\n");
			fprintf(src, "\t\t\tbreak;\n");
		}
		else if (!pszResult && bEvents)
		{
			//there is no result -> it is an Event
			//Source here
			fprintf(src, "\t\t\tcase OPID_%s:\n", vd->definedName);
			fprintf(src, "\t\t\t\t{\n");
			fprintf(src, "\t\t\t\t\t%s argument = new %s();\n", pszArgument, pszArgument);
			fprintf(src, "\t\t\t\t\tDateTime dtStart = DateTime.UtcNow;\n");
			fprintf(src, "\t\t\t\t\tif (m_Log.IsDebugEnabled) { m_Log.Debug(\"%s InvokeID:\" + invoke.invokeID.mValue.ToString()); }\n", vd->definedName);
			//fprintf(src, "\t\t\t\t\tm_Log.Debug(\"%s InvokeID:\" + invoke.invokeID.mValue.ToString());\n", vd->definedName);
			fprintf(src, "\t\t\t\t\targument.Decode(decodeBuffer);\n");		
			fprintf(src, "\t\t\t\t\tOnEvent_%s(argument);\n", vd->definedName);
			fprintf(src, "\t\t\t\t\tDateTime dtEnd = DateTime.UtcNow;\n");
			fprintf(src, "\t\t\t\t\tif (m_Log.IsDebugEnabled) { m_Log.Debug(\"%s InvokeID:{0} {1}ms\", invoke.invokeID.mValue.ToString(), (dtEnd-dtStart).TotalMilliseconds); }\n", vd->definedName);
			fprintf(src, "\t\t\t\t\tlRoseResult = ROSE_NOERROR;\n");
			fprintf(src, "\t\t\t\t}\n");
			fprintf(src, "\t\t\t\tbreak;\n");
		}
	}
} /* PrintROSEOnInvokeswitchCaseCS */

/*
 * prints PrintROSEInvokeCS
 */
static void PrintROSEInvokeCS(FILE *hdr, FILE *src, Module *m, int bEvents, ValueDef *vd)
{
	char* pszArgument = NULL;
	char* pszResult = NULL;
	char* pszError = NULL;
	if (GetROSECSDetails(vd, &pszArgument, &pszResult, &pszError))
	{
		if (pszResult && !bEvents)
		{
			//there is a result -> it is a Funktion
			//Are there errors inside?
			if (pszError)
			{

				//Source
				fprintf(src, "\t\tpublic long Invoke_%s(%s argument, %s result, %s error, int iTimeout)\n", vd->definedName,
									pszArgument, pszResult, pszError);
				fprintf(src, "\t\t{\n");
				fprintf(src, "\t\t\treturn AsnInvoke(argument, result, error, iTimeout, OPID_%s);\n", vd->definedName);
				fprintf(src, "\t\t}\n");
			}
			else
			{
				//no special errors
				//Source
				fprintf(src, "\t\tpublic long Event_%s(%s argument, %s result)\n", vd->definedName,
									pszArgument, pszResult);
				fprintf(src, "\t\t{\n");
				fprintf(src, "\t\t\treturn AsnEvent(argument,  OPID_%s);\n", vd->definedName);
				fprintf(src, "\t\t}\n");
				fprintf(src, "\n");
			}

		}
		else if (!pszResult && bEvents)
		{
			//there is no result -> it is an Event
			//Header
			/*fprintf(hdr, "\tlong Event_%s(%s* argument);\n", vd->definedName,
								pszArgument);*/

			//Source
			fprintf(src, "\t\tpublic long Event_%s(%s argument)\n", vd->definedName,
								pszArgument);
			fprintf(src, "\t\t{\n");
			
			fprintf(src, "\t\t\treturn AsnEvent(argument, OPID_%s);\n",  vd->definedName);

			fprintf(src, "\t\t}\n");
			fprintf(src, "\n");
		}
	}
} /* PrintROSEInvoke */


void PrintROSECSCode(FILE *src, ModuleList *mods, Module *m)
{
    ValueDef *vd;
	// time_t now = time (NULL);
	fprintf(src, "// %s - class member functions for ASN.1 module %s\n", RemovePath(m->ROSESrcCSFileName), m->modId->name);
	fprintf(src, "//\n");
	write_snacc_header(src, "// ");
	fprintf(src, "\n");

    fprintf(src, "using System;\n");
	fprintf(src, "using System.Collections.Generic;\n");
	fprintf(src, "using System.Text;\n");
	fprintf(src, "using System.ComponentModel;\n");
	fprintf(src, "using Com.Objsys.Asn1.Runtime;\n");
	fprintf(src, "using ENetCLRLib;\n");
	fprintf(src, "using NLog;\n");
	
    fprintf(src, "\n");    //RWC; PRINT before possible "namespace" designations.
    fprintf(src, "\n");

    /* 7-09-2001 Pierce Leonberger
     *   Added code to include all SNACC generated code in the SNACC namespace.
     *   If the namespace name was overridden with the '-ns' switch then
     *   use the name specified.  If the '-nons' switch was used then don't
     *   use namespaces for SNACC generated code.
     */
    if (gNO_NAMESPACE == 0)
    {

       if (gAlternateNamespaceString)
       {
          fprintf(src,"namespace %s \n", gAlternateNamespaceString);
		  fprintf(src,"{\n"); 
       }
       else if (m->namespaceToUse)
       {           
          fprintf(src,"namespace %s \n", m->namespaceToUse);
		  fprintf(src,"{\n"); 
       }
       else
       {
	      fprintf(src,"namespace ENetASNLib\n"); 
		  fprintf(src,"{\n"); 
       }
    }

	fprintf (src, "\tpublic abstract partial class ENetROSEInterfaceROSE : SnaccROSEBase");
	fprintf(src,"\t{\n\n"); 



	//print Operation defines
	
    fprintf (src, "\t\t\t//------------------------------------------------------------------------------\n");
	//fprintf (src, "\t\t\tprivate static NLog.Logger Log = NLog.LogManager.GetCurrentClassLogger(); \n\n");
    fprintf (src, "\t\t\t// Operation defines\n\n");
	fprintf (src,"\t\t\t#region Operation defines\n");
    FOR_EACH_LIST_ELMT (vd, m->valueDefs)
	{
		PrintROSECSOperationDefines(src, vd);	
	}

        
    fprintf (src, "\n");
	fprintf (src,"\t\t\t#endregion\n");
    fprintf (src, "//------------------------------------------------------------------------------\n");


    //fprintf (src, "//------------------------------------------------------------------------------\n");
    fprintf (src, "// class declarations:\n\n");

    //PrintCxxAnyCode (src, src, r, mods, m);
	fprintf (src, "\t\tpublic long OnInvoke%s(InvokePDU invoke)\n", m->ROSEClassName);
	fprintf (src, "\t\t{\n");
	fprintf (src, "\t\t\tlong lRoseResult = ROSE_REJECT_UNKNOWNOPERATION; // Event Default\n");
	fprintf (src, "\t\t\tint iOperationID = int.Parse(invoke.operationValue.ToString());\n");
	fprintf (src, "\t\t\tif (invoke.argument == null)  return lRoseResult;\n");
	fprintf (src, "\t\t\tAsn1BerDecodeBuffer decodeBuffer = new Asn1BerDecodeBuffer(invoke.argument.mValue);\n");
	fprintf (src, "\t\t\tswitch (iOperationID)\n");
	fprintf (src, "\t\t\t{\n");
	fprintf (src, "\t\t\t// OnInvoke\n");

    FOR_EACH_LIST_ELMT (vd, m->valueDefs)
	{
		if (vd->value->type->basicType->choiceId == BASICTYPE_MACROTYPE)
		{
			PrintROSEOnInvokeswitchCaseCS(src, 0, vd);
		}
	}

    FOR_EACH_LIST_ELMT (vd, m->valueDefs)
	{
		if (vd->value->type->basicType->choiceId == BASICTYPE_MACROTYPE)
		{
			PrintROSEOnInvokeswitchCaseCS(src, 1, vd);
		}
	}

	fprintf (src, "\t\t\t}\n");
	fprintf (src, "\t\treturn lRoseResult;\n");
	fprintf (src, "\t}\n");
	fprintf (src, "\n");

	fprintf (src, "\n");
	fprintf (src, "\t//Invoke Messages\n");
	fprintf (src, "\t#region Invoke Messages\n");
	//Now generate the invoke messages
    FOR_EACH_LIST_ELMT (vd, m->valueDefs)
	{
		if (vd->value->type->basicType->choiceId == BASICTYPE_MACROTYPE)
		{
			PrintROSEInvokeCS(src, src, m, 0, vd);
		}
	}
	fprintf (src, "\t#endregion\n");

	fprintf (src, "\t//Event Messages\n");
	//Now generate the invoke messages
	fprintf (src, "\t#region Event Messages\n");

    FOR_EACH_LIST_ELMT (vd, m->valueDefs)
	{
		if (vd->value->type->basicType->choiceId == BASICTYPE_MACROTYPE)
		{
			PrintROSEInvokeCS(src, src, m, 1, vd);
		}
	}
	fprintf (src, "\t#endregion\n");


	fprintf (src, "\t//Invoke Handler Messages\n");
	fprintf (src, "\t#region Invoke Handler Messages\n");
	//Now generate the invoke handler messages
    FOR_EACH_LIST_ELMT (vd, m->valueDefs)
	{
		if (vd->value->type->basicType->choiceId == BASICTYPE_MACROTYPE)
		{
			PrintROSEOnInvokeCS(src, 0, vd);
		}
	}
	fprintf (src, "\t#endregion\n");
	
	fprintf (src, "\t//Event Handler Messages\n");
	fprintf (src, "\t#region Event Handler Messages\n");
	//Now generate the Event handler messages
    FOR_EACH_LIST_ELMT (vd, m->valueDefs)
	{
		if (vd->value->type->basicType->choiceId == BASICTYPE_MACROTYPE)
		{
			PrintROSEOnInvokeCS(src, 1, vd);
		}
	}
	fprintf (src, "\t#endregion\n");
	fprintf (src, "\n");
	fprintf (src, "\t}\n");
	fprintf (src, "}\n");
} /* PrintROSECSCode */
