#ifndef ASN_COMMENT_H
#define ASN_COMMENT_H

#include <stdio.h>
#include "filetype.h"

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct _asnmodulecomment
	{
		const char* szModuleName;
		const char* szCategory;
		const char* szShort;
		const char* szLong;
		int iPrivate;
		int iDeprecated;
		const char* szDeprecated;
	} asnmodulecomment;

	typedef struct _asnoperationcomment
	{
		const char* szTypeName;
		const char* szCategory;
		const char* szShort;
		const char* szLong;
		int iPrivate;
		int iDeprecated;
		const char* szDeprecated;
	} asnoperationcomment;

	typedef struct _asnsequencecomment
	{
		const char* szTypeName;
		const char* szCategory;
		const char* szShort;
		const char* szLong;
		int iPrivate;
		int iDeprecated;
		const char* szDeprecated;
	} asnsequencecomment;

	typedef struct _asnmembercomment
	{
		const char* szShort;
		const char* szLinkedType;		//Link to a related Type for example the enum used in the integer
		int iPrivate;
		int iDeprecated;
	} asnmembercomment;

	//Parse for Comments
	extern int ParseFileForComments(FILE* fp, const char* szModuleName, const enum EFILETYPE type);

	//Get LogFilter Attributes (if any) call recurring until it returns 0
	extern const char* GetFirstModuleLogFileFilter(const char* szModuleName);
	extern const char* GetNextModuleLogFileFilter(const char* szModuleName);
	//Get Comments for Module (returns 1 for success)
	extern int GetModuleComment_UTF8(const char* szModuleName, asnmodulecomment* pcomment);
	extern int GetModuleComment_ASCII(const char* szModuleName, asnmodulecomment* pcomment);
	//Get Comments for Operation (returns 1 for success)
	extern int GetOperationComment_UTF8(const char* szOpName, asnoperationcomment* pcomment);
	extern int GetOperationComment_ASCII(const char* szOpName, asnoperationcomment* pcomment);
	//Get Comments for Sequence or other types (returns 1 for success)
	extern int GetSequenceComment_UTF8(const char* szTypeName, asnsequencecomment* pcomment);
	extern int GetSequenceComment_ASCII(const char* szTypeName, asnsequencecomment* pcomment);
	//Get Comments for member of sequence (returns 1 for success)
	extern int GetMemberComment_UTF8(const char* szTypeName, const char* szMemberName, asnmembercomment* pcomment);
	extern int GetMemberComment_ASCII(const char* szTypeName, const char* szMemberName, asnmembercomment* pcomment);

#ifdef __cplusplus
}
#endif

#endif