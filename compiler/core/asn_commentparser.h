#ifndef ASN_COMMENTPARSER_H
#define ASN_COMMENTPARSER_H

#include <map>
#include <string>
#include <list>
#include <vector>
#include "filetype.h"

std::string escapeJsonString(const std::string& input);

class EDeprecated
{
public:
	void handleDeprecated(const std::string& strParsedLine);
	// Type is deprecated, the value shows the time in unix time when the property has been flagged deprecated

	// @deprecated [optional] 1.1.2023 [optional] comment
	// e.g.
	// @deprecated 1.1.2023 Superseeded by method xyz

	// If no time has been specified the value is set to 1
	// The value is compared with the global gi64NoDeprecatedSymbols to validate whether the symbol shall get excluded or not for the output
	long long i64Deprecated = 0;
	// Deprecated comment - text that was written behing the @deprecated flag
	std::string strDeprecated_UTF8;
	std::string strDeprecated_ASCII;
};

class EAdded
{
public:
	void handleAdded(const std::string& strParsedLine);
	// Type has been added, the value shows the time in unix time when the property was added to the interface

	// @added 1.1.2023
	long long i64Added = 0;
};

class EStructMemberComment : public EDeprecated, public EAdded
{
public:
	// Short Description - must be stored json encoded
	std::string strShort_UTF8;
	std::string strShort_ASCII;
	// Linked Type (Enum used in Integer)
	std::string strLinkedType_UTF8;
	std::string strLinkedType_ASCII;
	// Type is private
	int iPrivate = 0;
	// Interal flag that stores whether the UTF8 value has already been converted to ascii (is done on access)
	bool m_bConvertedToAscii = false;
};

class ETypeComment : public EDeprecated, public EAdded
{
public:
	// Name of the strCategory
	std::string strCategory_UTF8;
	std::string strCategory_ASCII;
	// Name of the Type (name of SEQUENCE or name of OPERATION)
	std::string strTypeName_UTF8;
	std::string strTypeName_ASCII;
	// Short Description - must be stored json encoded
	std::string strShort_UTF8;
	std::string strShort_ASCII;
	// Long Description - must be stored json encoded
	std::string strLong_UTF8;
	std::string strLong_ASCII;
	// Type is private
	int iPrivate = 0;
	// Interal flag that stores whether the UTF8 value has already been converted to ascii (is done on access)
	bool m_bConvertedToAscii = false;
};

class EOperationComment : public ETypeComment
{
public:
};

class ESequenceComment : public ETypeComment
{
public:
	// Map member Name to Struct Member Comment
	std::map<std::string, EStructMemberComment> mapMembers;
};

class EModuleComment : public ETypeComment
{
public:
	// These elements shall be filtered when logging (currently only in the typescript rose stubs)
	// The elements are added through @logfilter in the beginning of the file ; delimited
	std::vector<std::string> strLogFilter;
	// Helper to get the LogFilters from the set in c
	int iCounter = 0;
};

void convertCommentList(std::list<std::string>& commentList, ETypeComment* pType);

class EAsnStackElement;

class EAsnCommentParser
{
public:
	int ParseFileForComments(FILE* fp, const char* szModuleName, const enum EFILETYPE type);

	std::list<EAsnStackElement*> m_stack;

private:
	int ProcessLine(const char* szModuleName, const char* szLine);
};

// Element on the Parser Stack
class EAsnStackElement
{
public:
	enum state
	{
		undef = -1,
		infile = 0,
		inmodule,
		insequence
	} m_state = undef;

	EAsnStackElement(EAsnCommentParser* pParser)
		: m_pParser(pParser)
	{
	}
	virtual ~EAsnStackElement()
	{
	}

	EAsnCommentParser* m_pParser;
	virtual int ProcessLine(const char* szModuleName, std::string& szLine, std::string& szComment, bool& bElementEnd) = 0;
};

class EAsnStackElementFile : public EAsnStackElement
{
public:
	EAsnStackElementFile(EAsnCommentParser* pParser, const char* szModuleName)
		: EAsnStackElement(pParser)
	{
		m_state = infile;
		m_strModuleName = szModuleName;
	}

	virtual int ProcessLine(const char* szModuleName, std::string& szLine, std::string& szComment, bool& bElementEnd) override;

	std::string m_strModuleName;

private:
	std::list<std::string> m_CollectComments;
};

class EAsnStackElementModule : public EAsnStackElement
{
public:
	EAsnStackElementModule(EAsnCommentParser* pParser)
		: EAsnStackElement(pParser)
	{
		m_state = inmodule;
	}

	virtual int ProcessLine(const char* szModuleName, std::string& szLine, std::string& szComment, bool& bElementEnd) override;

	void SetProperties(const char* szTypeName, const char* szCategory, std::list<std::string>& listComments);

private:
	bool m_bWaitForSemiColon = false;
	std::list<std::string> m_CollectComments;

	EModuleComment m_ModuleComment;
};

class EAsnStackElementSequence : public EAsnStackElement
{
public:
	EAsnStackElementSequence(EAsnCommentParser* pParser)
		: EAsnStackElement(pParser)
	{
		m_state = insequence;
		m_pmodcomment = 0;
	}

	virtual int ProcessLine(const char* szModuleName, std::string& szLine, std::string& szComment, bool& bElementEnd) override;

	//{ has been found
	bool bOpenBracketFound = false;

	void SetProperties(bool bOpenBracket, const char* szTypeName, EModuleComment* pmodcomment, std::list<std::string>& listComments);

	// collected comments during parsing
	std::list<std::string> m_CollectComments;

	ESequenceComment m_comment;
	EModuleComment* m_pmodcomment;
};

class EAsnStackElementSequenceOf : public EAsnStackElement
{
public:
	EAsnStackElementSequenceOf(EAsnCommentParser* pParser)
		: EAsnStackElement(pParser)
	{
		m_state = insequence;
	}

	virtual int ProcessLine(const char* szModuleName, std::string& szLine, std::string& szComment, bool& bElementEnd) override;

	// List of comment lines before the element
	std::list<std::string> commentsBefore;

	ESequenceComment m_comment;
};

class EAsnStackElementOperation : public EAsnStackElement
{
public:
	EAsnStackElementOperation(EAsnCommentParser* pParser)
		: EAsnStackElement(pParser)
	{
		m_state = insequence;
	}

	virtual int ProcessLine(const char* szModuleName, std::string& szLine, std::string& szComment, bool& bElementEnd) override;

	void SetProperties(const char* szTypeName, EModuleComment* pmodcomment, std::list<std::string>& listComments);

private:
	EOperationComment m_comment;
};

class EAsnComments
{
public:
	EAsnComments();

	// map Module Name to Details
	std::map<std::string, EModuleComment> mapModules;

	// map Operation Name to Details
	//  Key is the combination of the module and the operation as operations may exist multiple times
	//  modulename::operationname
	std::map<std::string, EOperationComment> mapOperations;

	// map Type Name to Details
	//  Key is the combination of the module and the type name as type names may exist multiple times
	//  modulename::typename
	std::map<std::string, ESequenceComment> mapSequences;
};

extern EAsnComments gComments;

#endif // ASN_COMMENTPARSER_H