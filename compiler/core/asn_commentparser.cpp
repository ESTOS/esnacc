#include "asn_commentparser.h"
#include "asn-stringconvert.h"
#include "filetype.h"
#include "../../snacc.h"
#include "time_helpers.h"
#include <stdio.h>
#include <sstream>
#include <string>
#include <list>
#include <string.h>
#include <time.h>
#include <vector>
#ifndef _WIN32
#include <unistd.h>
#endif

const std::string WHITESPACE = " \n\r\t\f\v";

bool isFiltered(const ETypeComment& comment)
{
	if (!gPrivateSymbols && comment.iPrivate)
		return true;

	if (comment.i64Deprecated && gi64NoDeprecatedSymbols)
	{
		if (gi64NoDeprecatedSymbols == 1)
			return true;
		else if (gi64NoDeprecatedSymbols >= comment.i64Deprecated)
			return true;
	}

	return false;
}

std::string ltrim(const std::string& s)
{
	size_t start = s.find_first_not_of(WHITESPACE);
	return (start == std::string::npos) ? "" : s.substr(start);
}

std::string rtrim(const std::string& s)
{
	size_t end = s.find_last_not_of(WHITESPACE);
	return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

std::string trim(const std::string& s)
{
	return rtrim(ltrim(s));
}

/**
 * Converts a unix time into something readable
 *
 * Returns a pointer to a buffer that needs to get released with Free
 */
char* ConvertUnixTimeToReverseTimeString(const long long tmUnixTime)
{
	char* szBuffer = (char*)malloc(128);
	if (!szBuffer)
		return szBuffer;

#ifdef _WIN32
	struct tm timeinfo;
	localtime_s(&timeinfo, &tmUnixTime);
	strftime(szBuffer, 128, "%Y%m%d", &timeinfo);
#else
	struct tm* timeinfo;
	timeinfo = localtime((const time_t*)&tmUnixTime);
	strftime(szBuffer, 128, "%Y%m%d", timeinfo);
#endif

	return szBuffer;
}

void EDeprecated::handleDeprecated(const std::string& strParsedLine)
{
	std::string strComment = trim(strParsedLine);
	// Check is ther a date in the value?
	// @deprecated 1.1.2023 Some comment
	auto pos = strComment.find(" ");
	if (pos == std::string::npos)
		pos = strComment.length();
	// Longest is 31.12.2023 (10), shortest is 1.1.2000 (8)
	if (pos >= 8 && pos <= 10)
	{
		// Okay, let's see if this is timestamp value...
		std::string strDate = strComment.substr(0, pos);
		long long i64UnixTime = ConvertDateToUnixTime(trim(strDate).c_str());
		if (i64UnixTime <= 0)
			fprintf(stderr, "WARNING - @deprecated flag is missing or has a broken timestamp. '%s'", strParsedLine.c_str());
		if (i64UnixTime > 0)
		{
			i64Deprecated = i64UnixTime;
			strComment = trim(strComment.substr(strDate.length()));
		}
	}
	if (strComment.length())
		strDeprecated_UTF8 = escapeJsonString(strComment);

	if (i64Deprecated <= 0)
	{
		fprintf(stderr, "WARNING - @deprecated flag is missing or has a broken timestamp. '%s'", strComment.c_str());
		i64Deprecated = 1;
	}
}

void EAdded::handleAdded(const std::string& strParsedLine)
{
	i64Added = ConvertDateToUnixTime(trim(strParsedLine).c_str());
	if (i64Added <= 0)
		fprintf(stderr, "WARNING - @added flag is missing or has a broken timestamp. '%s'", strParsedLine.c_str());
}

void replaceAll(std::string& str, const char* szSearch, const char* szReplace)
{
	if (!szSearch)
		return;
	auto searchLen = strlen(szSearch);
	if (!searchLen)
		return;
	auto replaceLen = strlen(szReplace);
	size_t start_pos = 0;
	while ((start_pos = str.find(szSearch, start_pos)) != std::string::npos)
	{
		str.replace(start_pos, searchLen, szReplace);
		start_pos += replaceLen; // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}
}

void AsciiToUTF8(const char* szAscii, std::string& strUTF8)
{
}

EAsnComments gComments;

extern "C"
{
	extern FILE* errFileG;
}

// std::set SORTIERT, das darf aber bei dem explode nicht sein!
std::vector<std::string> explode(std::string const& s, char delim, const bool bSkipEmpty = true)
{
	std::vector<std::string> result;
	std::istringstream iss(s);

	for (std::string token; std::getline(iss, token, delim);)
	{
		// leere tokens auslassen, brauchen wir nicht
		auto element = trim(token);
		if (!bSkipEmpty || element.size())
			result.push_back(element);
	}

	return result;
}

std::string escapeJsonString(const std::string& input)
{
	std::ostringstream ss;
	for (auto iter = input.cbegin(); iter != input.cend(); iter++)
	{
		// C++98/03:
		// for (std::string::const_iterator iter = input.begin(); iter != input.end(); iter++) {
		switch (*iter)
		{
			case '\\':
				ss << "\\\\";
				break;
			case '"':
				ss << "\\\"";
				break;
			case '/':
				ss << "\\/";
				break;
			case '\b':
				ss << "\\b";
				break;
			case '\f':
				ss << "\\f";
				break;
			case '\n':
				ss << "\\n";
				break;
			case '\r':
				ss << "\\r";
				break;
			case '\t':
				ss << "\\t";
				break;
			case '<':
				ss << "&lt;";
				break;
			case '>':
				ss << "&gt;";
				break;
			default:
				ss << *iter;
				break;
		}
	}
	return ss.str();
}

void convertCommentList(std::list<std::string>& commentList, ETypeComment* pType)
{
	if (!pType)
	{
		fprintf(stderr, "\nFatal error in convertCommentList\n");
		fprintf(stderr, "Invalid parameter, pType == NULL\n");
		exit(200);
	}
	bool bInLong = true;
	bool bInBrief = false;
	int nEmptyLines = 0;
	for (auto el = commentList.begin(); el != commentList.end(); el++)
	{
		std::string strLine = *el;
		if (strLine.substr(0, 6) == "@brief")
		{
			nEmptyLines = 0;
			bInLong = false;
			bInBrief = true;
			strLine = trim(strLine.substr(6));
			pType->strShort_UTF8 += escapeJsonString(strLine);
		}
		else if (strLine.substr(0, 5) == "@long")
		{
			nEmptyLines = 0;
			bInBrief = false;
			bInLong = true;
			strLine = trim(strLine.substr(5));
			pType->strLong_UTF8 += escapeJsonString(strLine);
		}
		else if (strLine.substr(0, 8) == "@private")
		{
			nEmptyLines = 0;
			pType->iPrivate = 1;
			// We do not change the flags here, the keyword @private may lead or follow any comment
			// bInLong = false;
			// bInBrief = false;
		}
		else if (strLine.substr(0, 11) == "@deprecated")
		{
			nEmptyLines = 0;
			pType->handleDeprecated(strLine.substr(11));
			// We do not change the flags here, the keyword @deprecated may lead or follow any comment
			// bInLong = false;
			// bInBrief = false;
		}
		else if (strLine.substr(0, 6) == "@added")
		{
			nEmptyLines = 0;
			pType->handleAdded(strLine.substr(6));
			// We do not change the flags here, the keyword @deprecated may lead or follow any comment
			// bInLong = false;
			// bInBrief = false;
		}
		else if (strLine.substr(0, 9) == "@category")
		{
			nEmptyLines = 0;
			strLine = trim(strLine.substr(9));
			// strLine += "\n";
			pType->strCategory_UTF8 = escapeJsonString(strLine);
			bInLong = false;
			bInBrief = false;
		}
		else if (strLine.substr(0, 10) == "@logfilter")
		{
			nEmptyLines = 0;
			EModuleComment* pModuleComment = static_cast<EModuleComment*>(pType);
			if (pModuleComment)
			{
				strLine = trim(strLine.substr(10));
				pModuleComment->strLogFilter = explode(strLine, ';');
			}
		}
		else
		{
			strLine = trim(strLine);
			if (bInLong)
			{
				if (!pType->strLong_UTF8.empty())
				{
					nEmptyLines++;
					if (!strLine.empty())
					{
						while (nEmptyLines)
						{
							pType->strLong_UTF8 += escapeJsonString("\n");
							nEmptyLines--;
						}
					}
				}
				pType->strLong_UTF8 += escapeJsonString(strLine);
			}
			else if (bInBrief)
			{
				if (!pType->strShort_UTF8.empty())
				{
					nEmptyLines++;
					if (!strLine.empty())
					{
						while (nEmptyLines)
						{
							pType->strShort_UTF8 += escapeJsonString("\n");
							nEmptyLines--;
						}
					}
				}
				pType->strShort_UTF8 += escapeJsonString(strLine);
			}
		}
	}
	if (!pType)
		return;
	if (!pType->strLong_UTF8.empty())
		pType->strLong_UTF8 += escapeJsonString("\n");
	if (!pType->strShort_UTF8.empty())
		pType->strShort_UTF8 += escapeJsonString("\n");
}

void convertMemberCommentList(std::list<std::string>& commentList, EStructMemberComment* pType)
{
	enum class eLast
	{
		_unknown = 0,
		_brief = 1,
		_private = 2,
		_deprecated = 3,
		_added = 4,
		_linked = 5,
	};

	eLast last = eLast::_unknown;

	for (auto el = commentList.begin(); el != commentList.end(); el++)
	{
		std::string strLine = *el;
		if (strLine.substr(0, 6) == "@brief")
		{
			last = eLast::_brief;
			strLine = trim(strLine.substr(6));
			pType->strShort_UTF8 += escapeJsonString(strLine);
		}
		else if (strLine.substr(0, 8) == "@private")
		{
			last = eLast::_private;
			pType->iPrivate = 1;
		}
		else if (strLine.substr(0, 11) == "@deprecated")
		{
			last = eLast::_deprecated;
			pType->handleDeprecated(strLine.substr(11));
		}
		else if (strLine.substr(0, 6) == "@added")
		{
			last = eLast::_added;
			pType->handleAdded(strLine.substr(6));
		}
		else if (strLine.substr(0, 7) == "@linked")
		{
			last = eLast::_linked;
			strLine = trim(strLine.substr(7));
			pType->strLinkedType_UTF8 += escapeJsonString(strLine);
		}
		else if (strLine.substr(0, 5) == "@long")
		{
			// in case someone added a long comment to a member variable we add the content to the short
			last = eLast::_brief;
			strLine = trim(strLine.substr(5));
			if (!pType->strShort_UTF8.empty())
				pType->strShort_UTF8 += escapeJsonString("\n");
			pType->strShort_UTF8 += escapeJsonString(strLine);
		}
		else
		{
			strLine = trim(strLine);
			if (last == eLast::_brief)
			{
				if (!pType->strShort_UTF8.empty())
					pType->strShort_UTF8 += escapeJsonString("\n");

				pType->strShort_UTF8 += escapeJsonString(strLine);
			}
			else if ((last == eLast::_unknown) && pType->strShort_UTF8.empty() && commentList.size() > 1)
			{
				// es ist noch kein Modus gesetzt, es gibt mehrere Zeilen, wir sehen gerade die erste Zeile
				pType->strShort_UTF8 += escapeJsonString(strLine);
				last = eLast::_brief;
			}
			else if ((last != eLast::_brief) && pType->strShort_UTF8.empty())
			{
				// Wenn das letzte Element ein linked, deprecated oder private ist erlaubten wir den Kommentar der hinter dem Element steht
				// @linked OtherElementType
				// iElement Number -- Beschreibung von iElement, auch wenn es ein linked type ist
				pType->strShort_UTF8 += escapeJsonString(strLine);
			}
		}
	}
}

int EAsnStackElementFile::ProcessLine(const char* szModuleName, const char* szRawSourceLine, std::string& szLine, std::string& szComment, EElementState& state)
{
	m_strRawSourceFileIncrement += szRawSourceLine;

	if (szLine.empty())
	{
		if (!szComment.empty())
		{
			if (szComment.substr(0, 6) == "@clear")
				m_CollectComments.clear();
			else
				m_CollectComments.push_back(szComment);
		}
		return 0;
	}

	if (szLine.length() >= 5)
	{
		std::string strBegin = szLine.substr(szLine.length() - 5);
		if (strBegin == "BEGIN")
		{
			EAsnStackElementModule* el = new EAsnStackElementModule(m_pParser);
			el->SetModuleProperties(m_strModuleName.c_str(), m_strModuleName.c_str(), m_CollectComments);
			m_CollectComments.clear();
			m_pParser->m_stack.push_back(el);

			return 0;
		}
	}
	if (szLine == "END")
	{
		fprintf(errFileG, "WARNING - EAsnCommentParser END before START\n");
		return 1;
	}

	return 0;
}

void EAsnStackElementModule::SetModuleProperties(const char* szTypeName, const char* szCategory, std::list<std::string>& listComments)
{
	m_ModuleComment.strTypeName_UTF8 = szTypeName;
	m_ModuleComment.strCategory_UTF8 = szCategory;
	convertCommentList(listComments, &m_ModuleComment);
}

int EAsnStackElementModule::ProcessLine(const char* szModuleName, const char* szRawSourceLine, std::string& szLine, std::string& szComment, EElementState& state)
{
	m_strRawSourceFileIncrement += szRawSourceLine;

	if (szLine == "END")
	{
		// end of module
		std::string strKey = szModuleName;
		const auto pos = strKey.find_first_of(".");
		if (pos != std::string::npos)
			strKey = strKey.substr(0, pos);

		m_ModuleComment.setModuleName(strKey.c_str());
		gComments.mapModules[strKey] = m_ModuleComment;

		if (isFiltered(m_ModuleComment))
			state = EElementState::end_and_filtered;
		else
		{
			m_strFilteredFileContent += m_strRawSourceFileIncrement;
			state = EElementState::end;
		}
		m_strRawSourceFileIncrement.clear();

		return 0;
	}

	if (szLine.empty())
	{
		if (!szComment.empty())
		{
			if (szComment.substr(0, 6) == "@clear")
				m_CollectComments.clear();
			else
				m_CollectComments.push_back(szComment);
		}
		return 0;
	}

	auto tokens = explode(szLine, ' ');
	auto iterTokens = tokens.begin();
	if (iterTokens != tokens.end())
	{
		if (*iterTokens == "IMPORTS")
		{
			iterTokens = tokens.erase(iterTokens);
			m_bWaitForSemiColon = true;
			if (szLine.find(";") != std::string::npos)
				m_bWaitForSemiColon = false;
			return 0;
		}
		if (*iterTokens == "EXPORTS")
		{
			iterTokens = tokens.erase(iterTokens);
			m_bWaitForSemiColon = true;
			if (szLine.find(";") != std::string::npos)
				m_bWaitForSemiColon = false;
			return 0;
		}
	}
	if (m_bWaitForSemiColon)
	{
		if (szLine.find(";") != std::string::npos)
			m_bWaitForSemiColon = false;

		m_strFilteredFileContent += m_strRawSourceFileIncrement;
		m_strRawSourceFileIncrement.clear();

		return 0;
	}

	char szFirstChar = szLine[0];
	if (szFirstChar >= 'A' && szFirstChar <= 'Z')
	{
		// Upper Letter Identifier
		std::string strType = iterTokens != tokens.end() ? *iterTokens : "";
		iterTokens++;
		if (iterTokens == tokens.end() || *iterTokens != "::=")
		{
			fprintf(errFileG, "WARNING - EAsnCommentParser invalid syntax\n");
			fprintf(errFileG, "szLine: %s\n", szLine.c_str());
			fprintf(errFileG, "szComment: %s\n", szComment.c_str());

			return 1;
		}
		iterTokens++;
		std::string strBasicType1 = iterTokens != tokens.end() ? *iterTokens : "";
		if (iterTokens != tokens.end())
			iterTokens++;
		std::string strBasicType2 = iterTokens != tokens.end() ? *iterTokens : "";
		if (iterTokens != tokens.end())
			iterTokens++;
		std::string strBasicType3 = iterTokens != tokens.end() ? *iterTokens : "";
		if (iterTokens != tokens.end())
			iterTokens++;

		if (strBasicType1 == "SEQUENCE" && strBasicType2 == "OF")
		{
			// SEQUENCE OF is usually only one line of code
			EAsnStackElementSequenceOf* el = new EAsnStackElementSequenceOf(m_pParser);
			el->m_comment.strTypeName_UTF8 = strType;
			el->m_comment.strCategory_UTF8 = m_ModuleComment.strCategory_UTF8;
			el->commentsBefore = m_CollectComments;

			szLine.clear();
			EElementState elementState = EElementState::not_yet_ended;
			el->ProcessLine(szModuleName, m_strRawSourceFileIncrement.c_str(), szLine, szComment, elementState);

			if (!isFiltered(el->m_comment))
				m_strFilteredFileContent += m_strRawSourceFileIncrement;
			m_strRawSourceFileIncrement.clear();
			// we assume the element has ended
			// m_pParser->m_stack.push_back(el);
			delete el;

			m_CollectComments.clear();
			return 0;
		}
		else if (strBasicType1 == "SEQUENCE" && (strBasicType2 == "" || strBasicType2 == "{"))
		{
			EAsnStackElementSequence* el = new EAsnStackElementSequence(m_pParser);
			el->SetSequenceProperties(strBasicType2 == "{", strType.c_str(), &m_ModuleComment, m_CollectComments);

			m_pParser->m_stack.push_back(el);

			m_CollectComments.clear();
			return 0;
		}
		else if (strBasicType1 == "ENUMERATED" && (strBasicType2 == "" || strBasicType2 == "{"))
		{
			EAsnStackElementSequence* el = new EAsnStackElementSequence(m_pParser);
			el->SetSequenceProperties(strBasicType2 == "{", strType.c_str(), &m_ModuleComment, m_CollectComments);

			m_pParser->m_stack.push_back(el);

			m_CollectComments.clear();
			return 0;
		}
		else if (strBasicType1 == "BIT" && strBasicType2 == "STRING")
		{
			EAsnStackElementSequence* el = new EAsnStackElementSequence(m_pParser);
			el->SetSequenceProperties(strBasicType2 == "{", strType.c_str(), &m_ModuleComment, m_CollectComments);

			m_pParser->m_stack.push_back(el);

			m_CollectComments.clear();
			return 0;
		}
		else if (strBasicType1 == "CHOICE" && (strBasicType2 == "" || strBasicType2 == "{"))
		{
			EAsnStackElementSequence* el = new EAsnStackElementSequence(m_pParser);
			el->SetSequenceProperties(strBasicType2 == "{", strType.c_str(), &m_ModuleComment, m_CollectComments);

			m_pParser->m_stack.push_back(el);

			m_CollectComments.clear();
			return 0;
		}
		else
		{
			// alias typedef
			ESequenceComment comment;
			comment.strTypeName_UTF8 = strType;
			comment.strCategory_UTF8 = m_ModuleComment.strCategory_UTF8;
			convertCommentList(m_CollectComments, &comment);

			std::string strKey = szModuleName;
			const auto pos = strKey.find_first_of(".");
			if (pos != std::string::npos)
				strKey = strKey.substr(0, pos);
			strKey += "::";
			strKey += comment.strTypeName_UTF8;
			gComments.mapSequences[strKey] = comment;

			m_CollectComments.clear();
			return 0;
		}
	}
	else if (szFirstChar >= 'a' && szFirstChar <= 'z')
	{
		// Lower Letter Identifier
		std::string strType = iterTokens != tokens.end() ? *iterTokens : "";
		if (iterTokens != tokens.end())
			iterTokens++;

		if (iterTokens != tokens.end() && *iterTokens == "OPERATION")
		{
			EAsnStackElementOperation* el = new EAsnStackElementOperation(m_pParser);
			el->SetOperationProperties(strType.c_str(), &m_ModuleComment, m_CollectComments);
			m_pParser->m_stack.push_back(el);
			m_CollectComments.clear();
			return 0;
		}
		else
		{
			ESequenceComment comment;
			convertCommentList(m_CollectComments, &comment);
			if (!isFiltered(comment))
			{
				m_strFilteredFileContent += m_strRawSourceFileIncrement;
				m_strRawSourceFileIncrement.clear();
			}
			m_CollectComments.clear();
		}

		return 0;
	}

	return 0;
}

bool EAsnStackElementModule::isModuleFiltered() const
{
	return isFiltered(m_ModuleComment);
}

void EAsnStackElementSequence::SetSequenceProperties(bool bOpenBracket, const char* szTypeName, EModuleComment* pmodcomment, std::list<std::string>& listComments)
{
	bOpenBracketFound = bOpenBracket;
	m_comment.strTypeName_UTF8 = szTypeName;
	m_comment.strCategory_UTF8 = pmodcomment->strCategory_UTF8;
	m_comment.iPrivate = pmodcomment->iPrivate;
	m_comment.i64Deprecated = pmodcomment->i64Deprecated;
	m_comment.i64Added = pmodcomment->i64Added;
	m_pmodcomment = pmodcomment;
	convertCommentList(listComments, &m_comment);
}

int EAsnStackElementSequence::ProcessLine(const char* szModuleName, const char* szRawSourceLine, std::string& szLine, std::string& szComment, EElementState& state)
{
	m_strRawSourceFileIncrement += szRawSourceLine;

	if (bOpenBracketFound == false)
	{
		if (szLine.empty())
			return 0;

		size_t iPos = szLine.find("{");
		if (iPos != std::string::npos)
		{
			bOpenBracketFound = true;
			szLine = trim(szLine.substr(iPos + 1, szLine.size() - iPos - 1));
		}
		return 0;
	}

	if (szLine.empty())
	{
		if (!szComment.empty())
			m_CollectComments.push_back(szComment);
		return 0;
	}

	auto tokens = explode(szLine, ' ');
	auto iter = tokens.begin();
	if (iter != tokens.end())
	{
		std::string strMember = *iter;
		iter++;
		if (strMember[0] >= 'a' && strMember[0] <= 'z')
		{
			// Member found
			// in an ENUMERATED, the Member ends with (
			size_t ii = strMember.find('(');
			if (ii != std::string::npos)
				strMember = rtrim(strMember.substr(0, ii));
			replaceAll(strMember, "-", "_");

			if (szComment.size())
				m_CollectComments.push_back(szComment);

			EStructMemberComment member;

			convertMemberCommentList(m_CollectComments, &member);
			m_comment.mapMembers[strMember] = member;

			std::string strBasicType1 = "";
			if (iter != tokens.end())
			{
				strBasicType1 = *iter;
				iter++;
			}
			std::string strBasicType2 = "";
			if (iter != tokens.end())
			{
				strBasicType2 = *iter;
				iter++;
			}
			if (strBasicType1 == "SEQUENCE" && strBasicType2 == "OF")
			{
				std::string strType = m_comment.strTypeName_UTF8 + "List";
				// SEQUENCE OF is usually only one line of code
				EAsnStackElementSequenceOf* el = new EAsnStackElementSequenceOf(m_pParser);
				el->m_comment.strTypeName_UTF8 = strType;
				el->m_comment.strCategory_UTF8 = m_comment.strCategory_UTF8;
				el->commentsBefore = m_CollectComments;

				m_pParser->m_stack.push_back(el);

				m_CollectComments.clear();
				return 0;
			}
			else if (strBasicType1 == "SEQUENCE" && (strBasicType2 == "" || strBasicType2 == "{"))
			{
				std::string strType = m_comment.strTypeName_UTF8 + "Seq";
				EAsnStackElementSequence* el = new EAsnStackElementSequence(m_pParser);
				el->SetSequenceProperties(strBasicType2 == "{", strType.c_str(), m_pmodcomment, m_CollectComments);

				m_pParser->m_stack.push_back(el);

				m_CollectComments.clear();
				return 0;
			}
			else if (strBasicType1 == "ENUMERATED" && (strBasicType2 == "" || strBasicType2 == "{"))
			{
				std::string strType = m_comment.strTypeName_UTF8 + "Enum";
				EAsnStackElementSequence* el = new EAsnStackElementSequence(m_pParser);
				el->SetSequenceProperties(strBasicType2 == "{", strType.c_str(), m_pmodcomment, m_CollectComments);

				m_pParser->m_stack.push_back(el);

				m_CollectComments.clear();
				return 0;
			}
			else if (strBasicType1 == "CHOICE" && (strBasicType2 == "" || strBasicType2 == "{"))
			{
				std::string strType = m_comment.strTypeName_UTF8 + "Choice";
				EAsnStackElementSequence* el = new EAsnStackElementSequence(m_pParser);
				el->SetSequenceProperties(strBasicType2 == "{", strType.c_str(), m_pmodcomment, m_CollectComments);

				m_pParser->m_stack.push_back(el);

				m_CollectComments.clear();
				return 0;
			}
			else
			{
				m_CollectComments.clear();
			}
		}
	}

	// finally check for close...
	if (szLine.find("}") != std::string::npos)
	{
		// sequence complete
		std::string strKey = szModuleName;
		const auto pos = strKey.find_first_of(".");
		if (pos != std::string::npos)
			strKey = strKey.substr(0, pos);
		strKey += "::";
		strKey += m_comment.strTypeName_UTF8;
		gComments.mapSequences[strKey] = m_comment;

		if (isFiltered(m_comment))
			state = EElementState::end_and_filtered;
		else
		{
			m_strFilteredFileContent += m_strRawSourceFileIncrement;
			state = EElementState::end;
		}
		m_strRawSourceFileIncrement.clear();
	}

	return 0;
}

int EAsnStackElementSequenceOf::ProcessLine(const char* szModuleName, const char* szRawSourceLine, std::string& szLine, std::string& szComment, EElementState& state)
{
	m_strRawSourceFileIncrement += szRawSourceLine;

	// sequence of is complete on the next line anyway
	// add comments before the sequence...
	convertCommentList(commentsBefore, &m_comment);
	commentsBefore.clear();

	std::string strKey = szModuleName;
	const auto pos = strKey.find_first_of(".");
	if (pos != std::string::npos)
		strKey = strKey.substr(0, pos);
	strKey += "::";
	strKey += m_comment.strTypeName_UTF8;

	gComments.mapSequences[strKey] = m_comment;

	if (isFiltered(m_comment))
		state = EElementState::end_and_filtered;
	else
	{
		m_strFilteredFileContent += m_strRawSourceFileIncrement;
		state = EElementState::end;
	}
	m_strRawSourceFileIncrement.clear();

	return 0;
}

void EAsnStackElementOperation::SetOperationProperties(const char* szTypeName, EModuleComment* pmodcomment, std::list<std::string>& listComments)
{
	m_comment.strTypeName_UTF8 = szTypeName;
	m_comment.strCategory_UTF8 = pmodcomment->strCategory_UTF8;
	m_comment.iPrivate = pmodcomment->iPrivate;
	m_comment.i64Deprecated = pmodcomment->i64Deprecated;
	m_comment.i64Added = pmodcomment->i64Added;
	convertCommentList(listComments, &m_comment);
}

int EAsnStackElementOperation::ProcessLine(const char* szModuleName, const char* szRawSourceLine, std::string& szLine, std::string& szComment, EElementState& state)
{
	m_strRawSourceFileIncrement += szRawSourceLine;

	if (szLine.find("::=") != std::string::npos)
	{
		std::string strKey = szModuleName;
		const auto pos = strKey.find_first_of(".");
		if (pos != std::string::npos)
			strKey = strKey.substr(0, pos);
		strKey += "::";
		strKey += m_comment.strTypeName_UTF8;
		gComments.mapOperations[strKey.c_str()] = m_comment;

		if (isFiltered(m_comment))
			state = EElementState::end_and_filtered;
		else
		{
			m_strFilteredFileContent += m_strRawSourceFileIncrement;
			state = EElementState::end;
		}
		m_strRawSourceFileIncrement.clear();
	}
	return 0;
}

EAsnComments::EAsnComments()
{
	// Add a sample here
	EOperationComment a;
	a.strTypeName_UTF8 = "asnKeepAlive";
	a.strShort_UTF8 = escapeJsonString("Send a Keepalive on the Connection in a regular interval");
	a.strLong_UTF8 = escapeJsonString("Send a Keepalive on the Connection in a regular interval\nA client is required to do this every xxx seconds.");
	mapOperations[a.strTypeName_UTF8] = a;
}

int EAsnCommentParser::ParseFileForComments(FILE* fp, const char* szModuleName, const enum EFILETYPE type)
{
	// set to beginning of file
	fseek(fp, type == UTF8WITHBOM ? 3 : 0, SEEK_SET);

	m_stack.clear();

	m_stack.push_back(new EAsnStackElementFile(this, szModuleName));

	char szLine[5000] = {0};

	while (fgets(szLine, 5000, fp))
		if (type == ASCII)
			ProcessLine(szModuleName, AsnStringConvert::AsciiToUTF8(szLine).c_str());
		else
			ProcessLine(szModuleName, szLine);

	// clear stack
	if (m_stack.size() > 1)
		fprintf(errFileG, "WARNING - EAsnCommentParser inconsistent file syntax\n");
	else if (m_stack.size() == 1)
	{
		std::string strFileName;
		if (gszOutputPath)
			strFileName = gszOutputPath;
		strFileName += szModuleName;

		auto pFile = m_stack.back();
		if (gFilterASN1Files)
		{
			if (pFile->m_strFilteredFileContent.empty())
			{
#ifdef _WIN32
				_unlink(strFileName.c_str());
#else
				unlink(strFileName.c_str());
#endif
			}
			else
			{
				// Add lines at the end of the file which haven´t had a element association
				pFile->m_strFilteredFileContent += pFile->m_strRawSourceFileIncrement;
				FILE* filteredFile = nullptr;
#ifdef _WIN32
				errno_t err = fopen_s(&filteredFile, strFileName.c_str(), "w");
				if (err != 0)
					filteredFile = NULL;
#else
				filteredFile = fopen(strFileName.c_str(), "w");
#endif
				if (filteredFile)
				{
					if (type == UTF8WITHBOM)
					{
						unsigned char bom[] = {0xEF, 0xBB, 0xBF};
						fwrite(bom, sizeof(unsigned char), 3, filteredFile);
					}
					const auto& strData = pFile->m_strFilteredFileContent;
					auto strElements = explode(strData, '\n', false);
					for (auto& strElement : strElements)
					{
						if (strElement.length() > 4 && strElement.substr(0, 5) == "-- ~ " || strElement.length() == 4 && strElement.substr(0, 4) == "-- ~")
							continue;
						strElement += "\n";
						if (type == ASCII)
						{
							auto strASCII = AsnStringConvert::UTF8ToAscii(strElement.c_str());
							fwrite(strASCII.c_str(), sizeof(char), strASCII.length(), filteredFile);
						}
						else
							fwrite(strElement.c_str(), sizeof(char), strElement.length(), filteredFile);
					}
					fclose(filteredFile);
				}
			}
		}
	}

	while (!m_stack.empty())
	{
		delete m_stack.back();
		m_stack.pop_back();
	}

	// reset to beginning of file
	fseek(fp, type == UTF8WITHBOM ? 3 : 0, SEEK_SET);

	return 0; // NO Error
}

int EAsnCommentParser::ProcessLine(const char* szModuleName, const char* szLine)
{
	int iResult = 1; // error

	std::string strLine(szLine);
	strLine = trim(strLine);

	std::string strComment;
	// Seperate command and comment
	size_t iPos = strLine.find("--");
	if (iPos != std::string::npos)
	{
		strComment = strLine.substr(iPos + 2, strLine.size() - iPos - 2);
		strLine = strLine.substr(0, iPos);

		strLine = trim(strLine);

		// Kommentare bekommen nur das erste Leerzeichen entfernt
		if (strComment.substr(0, 1) == " ")
			strComment = strComment.substr(1, strComment.size() - 1);

		// strComment.TrimRight();
		// Ein existierender Kommentar ist nie ganz leer
		if (strComment.empty())
			strComment = " ";

		// Ein Kommentar der mit ~ beginnt wird ignoriert
		if (strComment.substr(0, 1) == "~")
			strComment.clear();
	}
	replaceAll(strLine, "\t", " ");
	replaceAll(strLine, "  ", " ");

	EAsnStackElement* el = m_stack.back();
	EElementState elementState = EElementState::not_yet_ended;
	iResult = el->ProcessLine(szModuleName, szLine, strLine, strComment, elementState);
	if (elementState != EElementState::not_yet_ended)
	{
		m_stack.pop_back();
		EAsnStackElement* elBefor = m_stack.back();
		if (elementState == EElementState::end)
		{
			elBefor->m_strFilteredFileContent += elBefor->m_strRawSourceFileIncrement;
			el->m_strRawSourceFileIncrement.clear();
			elBefor->m_strFilteredFileContent += el->m_strFilteredFileContent;
		}

		if (m_stack.size() == 1)
		{
			auto pModule = dynamic_cast<EAsnStackElementModule*>(el);
			if (pModule && pModule->isModuleFiltered())
				elBefor->m_strFilteredFileContent.clear();
			else
				elBefor->m_strRawSourceFileIncrement += elBefor->m_strFilteredFileContent;
		}

		elBefor->m_strRawSourceFileIncrement.clear();

		delete el;
	}

	return iResult;
}

void EModuleComment::setModuleName(const char* szModuleName)
{
	m_strModuleName = szModuleName;
}

long long EModuleComment::getModuleMinorVersion()
{
	if (m_i64ModuleVersion == -1)
	{
		const auto iNameLength = m_strModuleName.length();
		long long i64HighestVersion = 0;
		for (const auto& operationComment : gComments.mapOperations)
		{
			if (operationComment.first.substr(0, iNameLength) == m_strModuleName)
			{
				if (operationComment.second.i64Added > i64HighestVersion)
					i64HighestVersion = operationComment.second.i64Added;
			}
		}
		for (const auto& sequenceComment : gComments.mapSequences)
		{
			if (sequenceComment.first.substr(0, iNameLength) == m_strModuleName)
			{
				if (sequenceComment.second.i64Added > i64HighestVersion)
					i64HighestVersion = sequenceComment.second.i64Added;
				for (const auto& memberComment : sequenceComment.second.mapMembers)
					if (memberComment.second.i64Added > i64HighestVersion)
						i64HighestVersion = memberComment.second.i64Added;
			}
		}
		m_i64ModuleVersion = i64HighestVersion;
	}

	return m_i64ModuleVersion;
}