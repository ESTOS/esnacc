// FILE: snaccexcept.cpp
//
//
#include "../include/snaccexcept.h"
#include <memory.h>
#include <cstring>

namespace SNACC
{
	const char* ConstraintErrorStringList[SEQ_OF_SIZE_VALUE_RANGE + 1] = {"INTEGER VALUE CONSTRAINT ERROR :: Integer value is not within Upper and Lower constrained Bounds!!\n", "INTEGER VALUE CONSTRAINT ERROR :: Integer value is not equal to single value constraint!!\n",		   "STRING SIZE CONSTRAINT ERROR :: String size is not equal to single value size constraint!!\n",		  "STRING SIZE CONSTRAINT ERROR :: String size is not within Upper and Lower constrained Bounds!!\n",
																		  "STRING ALPHA CONSTRAINT ERROR :: String contents not within designated permitted alphabet!!\n",		  "STRING ALPHA CONSTRAINT ERROR :: String contents not within designated types normal alphabet!!\n",	   "WIDE STRING SIZE CONSTRAINT ERROR :: String size is not equal to single value size constraint!!\n",	  "WIDE STRING SIZE CONSTRAINT ERROR :: String size is not within Upper and Lower constrained Bounds!!\n",
																		  "WIDE STRING ALPHA CONSTRAINT ERROR :: String contents not within designated permitted alphabet!!\n",	  "WIDE STRING ALPHA CONSTRAINT ERROR :: String contents not within designated types normal alphabet!!\n", "OCTET STRING CONSTRAINT ERROR :: Octet String size is not equal to single value size constraint!!\n", "OCTET STRING CONSTRAINT ERROR :: Octet String size is not within Upper and Lower constrained Bounds!!\n",
																		  "BIT STRING CONSTRAINT ERROR :: Bit String size is not equal to single value size constraint!!\n",	  "BIT STRING CONSTRAINT ERROR :: Bit String size is not within Upper and Lower constrained Bounds!!\n",   "SET OF CONSTRAINT ERROR :: Set Of element count is not equal to single value size constraint!!\n",	  "SET OF CONSTRAINT ERROR :: Set Of element count is not within Upper and Lower constrained Bounds!!\n",
																		  "SEQ OF CONSTRAINT ERROR :: Seq Of element count is not equal to single value size constraint!!\n",	  "SEQ OF CONSTRAINT ERROR :: Seq Of element count is not within Upper and Lower constrained Bounds!!\n"};
}

using namespace SNACC;

SnaccException::SnaccException(long errorCode) noexcept
{
	m_errorCode = errorCode;
	stackPos = -1;
	memset(&stack[0], 0, sizeof(CallStack) * STACK_DEPTH);
}

SnaccException::SnaccException(const char* file, long line_number, const char* function, const char* whatStrIn, long errorCode) noexcept
{
	memset(&stack[0], 0, sizeof(CallStack) * STACK_DEPTH);

	try
	{
		if (whatStrIn != NULL)
			this->m_whatStr = whatStrIn;
	}
	catch (...)
	{
		// do nothing
	}

	m_errorCode = errorCode;
	stackPos = 0;
	stack[0].file = file;
	stack[0].line_number = line_number;
	stack[0].function = function;
}

SnaccException::~SnaccException() noexcept
{
	m_errorCode = DEFAULT_ERROR_CODE;
	stackPos = -1;
}

SnaccException& SnaccException::operator=(const SnaccException& o)
{
	try
	{
		stackPos = o.stackPos;
		memcpy(stack, o.stack, sizeof(stack));
		m_errorCode = o.m_errorCode;

		m_whatStr = o.m_whatStr;
	}
	catch (...)
	{
		// do nothing
	}

	return *this;
}

void SnaccException::push(const char* file, long line_number, const char* function) noexcept
{
	if (stackPos < STACK_DEPTH)
	{
		stack[stackPos].file = file;
		stack[stackPos].line_number = line_number;
		stack[stackPos].function = function;
		stackPos++;
	}
}

const char* SnaccException::what() const noexcept
{
	return m_whatStr.c_str();
}

void SnaccException::getCallStack(std::ostream& os) const
{
	int i = 0;
	const char* ptr = NULL;

	for (; i <= stackPos; i++)
	{
#ifdef _WIN32
		if ((ptr = strrchr(stack[i].file, '\\')) == NULL)
#else
		if ((ptr = strrchr(stack[i].file, '/')) == NULL)
#endif
			ptr = (char*)stack[i].file;
		else
			ptr++;
		os << ptr;
		os << "\t" << stack[i].line_number;
		if (stack[i].function)
			os << "\t" << stack[i].function << "\n";
		else
			os << "\t"
			   << "\n";
	}
}

FileException::FileException(const char* filename, enum FileErrType errType, const char* file, long line_number, const char* function) noexcept
	: SnaccException(file, line_number, function, NULL, FILE_IO_ERROR)
{
	switch (errType)
	{
		case OPEN:
			strcpy_s(whatStr, 512, "Error opening file: ");
			break;
		case CREATE:
			strcpy_s(whatStr, 512, "Error creating file: ");
			break;
		case READ:
			strcpy_s(whatStr, 512, "Error reading file: ");
			break;
		case WRITE:
			strcpy_s(whatStr, 512, "Error writing file: ");
			break;
	}
	strcat_s(whatStr, 511, filename);
}

const char* FileException::what() const noexcept
{
	return &whatStr[0];
}

MemoryException::MemoryException(long memorySize, const char* variable, const char* file, long line_number, const char* function) noexcept
	: SnaccException(file, line_number, function, "MemoryException", MEMORY_ERROR)
{
	sprintf_s(memoryInfo, 128, "Error allocating %ld bytes for ", memorySize);
	int memUsed = (int)strlen(memoryInfo);
	int len2copy = (int)strlen(variable);
	if (len2copy > (128 - memUsed - 1))
		len2copy = 128 - memUsed - 1;
	memcpy(&memoryInfo[memUsed], variable, len2copy);
	memoryInfo[memUsed + len2copy] = '\0';
}

const char* MemoryException::what() const noexcept
{
	return &memoryInfo[0];
}

InvalidTagException::InvalidTagException(const char* type, long tagId, const char* file, long line_number, const char* function) noexcept
	: SnaccException(file, line_number, function, "InvalidTagException", INVALID_TAG)
{
	sprintf_s(wrongTagInfo, 128, "Tag [%ld] is invalid for type ", tagId);
	int memUsed = (int)strlen(wrongTagInfo);
	int len2copy = (int)strlen(type);
	if (len2copy > (128 - memUsed - 1))
		len2copy = 128 - memUsed - 1;
	memcpy(&wrongTagInfo[memUsed], type, len2copy);
	wrongTagInfo[memUsed + len2copy] = '\0';
}

InvalidTagException::InvalidTagException(const char* type, const char* elementName, const char* file, long line_number, const char* function) noexcept
	: SnaccException(file, line_number, function, "InvalidTagException", INVALID_TAG)
{
	sprintf_s(wrongTagInfo, 128, "Json Error [%s] in ", elementName);
	int memUsed = (int)strlen(wrongTagInfo);
	int len2copy = (int)strlen(type);
	if (len2copy > (128 - memUsed - 1))
		len2copy = 128 - memUsed - 1;
	memcpy(&wrongTagInfo[memUsed], type, len2copy);
	wrongTagInfo[memUsed + len2copy] = '\0';
}

const char* InvalidTagException::what() const noexcept
{
	return &wrongTagInfo[0];
}
