#include "asn_comments.h"
#include "asn_commentparser.h"
#include "asn-stringconvert.h"

extern "C" {

	int ParseFileForComments(FILE* fp, const char* szModuleName, const enum EFILETYPE type)
	{
		EAsnCommentParser parser;
		return parser.ParseFileForComments(fp, szModuleName, type);
	}

	const char* GetFirstModuleLogFileFilter(const char* szModuleName)
	{
		if (!szModuleName)
			return 0;

		auto it = gComments.mapModules.find(szModuleName);
		if (it != gComments.mapModules.end())
		{
			it->second.iCounter = 0;

			auto iter = it->second.strLogFilter.begin();
			if (iter != it->second.strLogFilter.end())
			{
				it->second.iCounter = 1;
				return iter->c_str();
			}
		}
		return NULL;
	}


	const char* GetNextModuleLogFileFilter(const char* szModuleName)
	{
		if (!szModuleName)
			return 0;

		auto it = gComments.mapModules.find(szModuleName);
		if (it != gComments.mapModules.end())
		{
			auto iter = it->second.strLogFilter.begin();
			int iCount = it->second.iCounter;
			while (iCount && iter != it->second.strLogFilter.end())
			{
				iCount--;
				iter++;
			}
			if (iter != it->second.strLogFilter.end())
			{
				it->second.iCounter++;
				return iter->c_str();
			}
		}
		it->second.iCounter = 0;
		return NULL;
	}

	int GetModuleComment_UTF8(const char* szModuleName, asnmodulecomment* pcomment)
	{
		if (!szModuleName)
			return 0;

		auto it = gComments.mapModules.find(szModuleName);
		if (it != gComments.mapModules.end())
		{
			const EModuleComment& comment = it->second;
			pcomment->szModuleName = comment.strTypeName_UTF8.c_str();
			pcomment->szShort = comment.strShort_UTF8.c_str();
			pcomment->szLong = comment.strLong_UTF8.c_str();
			pcomment->szCategory = comment.strCategory_UTF8.c_str();
			pcomment->iPrivate = comment.iPrivate;
			pcomment->lDeprecated = comment.lDeprecated;
			pcomment->szDeprecated = comment.strDeprecated_UTF8.c_str();
			return 1;
		}
		return 0;
	}

	int GetModuleComment_ASCII(const char* szModuleName, asnmodulecomment* pcomment)
	{
		if (!szModuleName)
			return 0;

		auto it = gComments.mapModules.find(szModuleName);
		if (it != gComments.mapModules.end())
		{
			EModuleComment& comment = it->second;
			if (!comment.m_bConvertedToAscii) {
				comment.strTypeName_ASCII = AsnStringConvert::UTF8ToAscii(comment.strTypeName_UTF8.c_str());
				comment.strShort_ASCII = AsnStringConvert::UTF8ToAscii(comment.strShort_UTF8.c_str());
				comment.strLong_ASCII = AsnStringConvert::UTF8ToAscii(comment.strLong_UTF8.c_str());
				comment.strCategory_ASCII = AsnStringConvert::UTF8ToAscii(comment.strCategory_UTF8.c_str());
				comment.strDeprecated_ASCII = AsnStringConvert::UTF8ToAscii(comment.strDeprecated_UTF8.c_str());
				comment.m_bConvertedToAscii = true;
			}
			pcomment->szModuleName = comment.strTypeName_ASCII.c_str();
			pcomment->szShort = comment.strShort_ASCII.c_str();
			pcomment->szLong = comment.strLong_ASCII.c_str();
			pcomment->szCategory = comment.strCategory_ASCII.c_str();
			pcomment->iPrivate = comment.iPrivate;
			pcomment->lDeprecated = comment.lDeprecated;
			pcomment->szDeprecated = comment.strDeprecated_ASCII.c_str();
			return 1;
		}
		return 0;
	}

	int GetOperationComment_UTF8(const char* szModuleName, const char* szOpName, asnoperationcomment* pcomment)
	{
		if (!szModuleName || !szOpName)
			return 0;

		std::string strKey = szModuleName;
		strKey += "::";
		strKey += szOpName;

		auto it = gComments.mapOperations.find(strKey);
		if (it != gComments.mapOperations.end()) 
		{
			const EOperationComment& comment = it->second;
			pcomment->szTypeName = comment.strTypeName_UTF8.c_str();
			pcomment->szShort = comment.strShort_UTF8.c_str();
			pcomment->szLong = comment.strLong_UTF8.c_str();
			pcomment->szCategory = comment.strCategory_UTF8.c_str();
			pcomment->iPrivate = comment.iPrivate;
			pcomment->lDeprecated = comment.lDeprecated;
			pcomment->szDeprecated = comment.strDeprecated_UTF8.c_str();
			return 1;
		}
		return 0;
	}

	int GetOperationComment_ASCII(const char* szModuleName, const char* szOpName, asnoperationcomment* pcomment)
	{
		if (!szModuleName || !szOpName)
			return 0;

		std::string strKey = szModuleName;
		strKey += "::";
		strKey += szOpName;

		auto it = gComments.mapOperations.find(strKey);
		if (it != gComments.mapOperations.end()) 
		{
			EOperationComment& comment = it->second;
			if (!comment.m_bConvertedToAscii) {
				comment.strTypeName_ASCII = AsnStringConvert::UTF8ToAscii(comment.strTypeName_UTF8.c_str());
				comment.strShort_ASCII = AsnStringConvert::UTF8ToAscii(comment.strShort_UTF8.c_str());
				comment.strLong_ASCII = AsnStringConvert::UTF8ToAscii(comment.strLong_UTF8.c_str());
				comment.strCategory_ASCII = AsnStringConvert::UTF8ToAscii(comment.strCategory_UTF8.c_str());
				comment.strDeprecated_ASCII = AsnStringConvert::UTF8ToAscii(comment.strDeprecated_UTF8.c_str());
				comment.m_bConvertedToAscii = true;
			}
			pcomment->szTypeName = comment.strTypeName_ASCII.c_str();
			pcomment->szShort = comment.strShort_ASCII.c_str();
			pcomment->szLong = comment.strLong_ASCII.c_str();
			pcomment->szCategory = comment.strCategory_ASCII.c_str();
			pcomment->iPrivate = comment.iPrivate;
			pcomment->lDeprecated = comment.lDeprecated;
			pcomment->szDeprecated = comment.strDeprecated_ASCII.c_str();
			return 1;
		}
		return 0;
	}	

	//Get Comments for Sequence or other types
	int GetSequenceComment_UTF8(const char* szModuleName, const char* szTypeName, asnsequencecomment* pcomment)
	{
		if (!szModuleName || !szTypeName)
			return 0;

		std::string strKey = szModuleName;
		strKey += "::";
		strKey += szTypeName;

		auto it = gComments.mapSequences.find(strKey);
		if (it != gComments.mapSequences.end())
		{
			const ESequenceComment& comment = it->second;
			pcomment->szTypeName = comment.strTypeName_UTF8.c_str();
			pcomment->szShort = comment.strShort_UTF8.c_str();
			pcomment->szLong = comment.strLong_UTF8.c_str();
			pcomment->szCategory = comment.strCategory_UTF8.c_str();
			pcomment->iPrivate = comment.iPrivate;
			pcomment->lDeprecated = comment.lDeprecated;
			pcomment->szDeprecated = comment.strDeprecated_UTF8.c_str();
			return 1;
		}
		return 0;
	}

	int GetSequenceComment_ASCII(const char* szModuleName, const char* szTypeName, asnsequencecomment* pcomment)
	{
		if (!szModuleName || !szTypeName)
			return 0;

		std::string strKey = szModuleName;
		strKey += "::";
		strKey += szTypeName;

		auto it = gComments.mapSequences.find(strKey);
		if (it != gComments.mapSequences.end())
		{
			ESequenceComment& comment = it->second;
			if (!comment.m_bConvertedToAscii) {
				comment.strTypeName_ASCII = AsnStringConvert::UTF8ToAscii(comment.strTypeName_UTF8.c_str());
				comment.strShort_ASCII = AsnStringConvert::UTF8ToAscii(comment.strShort_UTF8.c_str());
				comment.strLong_ASCII = AsnStringConvert::UTF8ToAscii(comment.strLong_UTF8.c_str());
				comment.strCategory_ASCII = AsnStringConvert::UTF8ToAscii(comment.strCategory_UTF8.c_str());
				comment.strDeprecated_ASCII = AsnStringConvert::UTF8ToAscii(comment.strDeprecated_UTF8.c_str());
				comment.m_bConvertedToAscii = true;
			}
			pcomment->szTypeName = comment.strTypeName_ASCII.c_str();
			pcomment->szShort = comment.strShort_ASCII.c_str();
			pcomment->szLong = comment.strLong_ASCII.c_str();
			pcomment->szCategory = comment.strCategory_ASCII.c_str();
			pcomment->iPrivate = comment.iPrivate;
			pcomment->lDeprecated = comment.lDeprecated;
			pcomment->szDeprecated = comment.strDeprecated_ASCII.c_str();
			return 1;
		}
		return 0;
	}

	//Get Comments for member of sequence
	int GetMemberComment_UTF8(const char* szModuleName, const char* szTypeName, const char* szMemberName, asnmembercomment* pcomment)
	{
		if (!szModuleName || !szTypeName || !szMemberName)
			return 0;

		std::string strKey = szModuleName;
		strKey += "::";
		strKey += szTypeName;

		if(strKey == "ENetUC_Conference::AsnInterfaceVersions")
			printf("");
		auto it = gComments.mapSequences.find(strKey);
		if (it != gComments.mapSequences.end())
		{
			auto it2 = it->second.mapMembers.find(szMemberName);
			if (it2 != it->second.mapMembers.end())
			{
				const EStructMemberComment& comment = it2->second;
				pcomment->szShort = comment.strShort_UTF8.c_str();
				pcomment->szLinkedType = comment.strLinkedType_UTF8.c_str();
				pcomment->iPrivate = comment.iPrivate;
				pcomment->lDeprecated = comment.lDeprecated;
				pcomment->szDeprecated = comment.strDeprecated_UTF8.c_str();
				return 1;
			}
		}
		return 0;
	}

	int GetMemberComment_ASCII(const char* szModuleName, const char* szTypeName, const char* szMemberName, asnmembercomment* pcomment)
	{
		if (!szModuleName || !szTypeName || !szMemberName)
			return 0;

		std::string strKey = szModuleName;
		strKey += "::";
		strKey += szTypeName;

		auto it = gComments.mapSequences.find(strKey);
		if (it != gComments.mapSequences.end())
		{
			auto it2 = it->second.mapMembers.find(szMemberName);
			if (it2 != it->second.mapMembers.end())
			{
				EStructMemberComment& comment = it2->second;
				if (!comment.m_bConvertedToAscii) {
					comment.strShort_ASCII = AsnStringConvert::UTF8ToAscii(comment.strShort_UTF8.c_str());
					comment.strLinkedType_ASCII = AsnStringConvert::UTF8ToAscii(comment.strLinkedType_UTF8.c_str());
					comment.m_bConvertedToAscii = true;
				}
				pcomment->szShort = comment.strShort_ASCII.c_str();
				pcomment->szLinkedType = comment.strLinkedType_ASCII.c_str();
				pcomment->iPrivate = comment.iPrivate;
				pcomment->lDeprecated = comment.lDeprecated;
				pcomment->szDeprecated = comment.strDeprecated_ASCII.c_str();
				return 1;
			}
		}
		return 0;
	}
}

