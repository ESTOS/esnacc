#include "../include/asn-incl.h"

using namespace SNACC;

// AsnBuf.cpp
AsnBuf::AsnBuf()
{
	m_card = m_deck.begin();
	// m_card = m_deck.insert(m_deck.begin(), new Card());
}

AsnBuf::AsnBuf(const char* seg, size_t segLen)
{
	m_card = m_deck.insert(m_deck.begin(), new Card(new AsnRvsBuf(seg, segLen)));
}

AsnBuf::AsnBuf(const std::stringstream& ss)
{
	m_card = m_deck.insert(m_deck.begin(), new Card(ss));
}

AsnBuf::AsnBuf(std::streambuf* sb)
{
	m_card = m_deck.insert(m_deck.begin(), new Card(sb));
}

// Copy constructor
AsnBuf::AsnBuf(const AsnBuf& o)
{
	operator=(o);
}

SNACC::AsnBuf::~AsnBuf()
{
	clear();

	/*
	[2019/05/19 AL]

	operation auf ungueltigen prev/next-pointer
	bei ~deque in _Myproxy fuehrt zu absturz
	bei suchen vom typ searchResEntryCid.
	~deque() noexcept { // destroy the deque
		_Tidy();

		// added in devstudio 2019:
		_Alproxy_ty _Proxy_allocator(_Getal());
		_Delete_plain(_Proxy_allocator, _STD exchange(_Get_data()._Myproxy, nullptr));
	}
	*/

	/* [2019/10/04 AL] removed
	#if(_MSC_VER >= 1920) // devstudio 2019
		memset(this, 0x00, sizeof(AsnBuf));
	#endif
	*/
}

void AsnBuf::insert(const AsnBuf& that)
{
	if (!m_deck.empty() && m_card != m_deck.end() && (*m_card)->length() == 0)
		m_card = m_deck.erase(m_card);

	AsnRvsBuf* rvsBuf = new AsnRvsBuf(that);
	m_card = m_deck.insert(m_deck.begin(), new Card(rvsBuf));
}

AsnBuf& AsnBuf::operator=(const AsnBuf& o)
{
	if (this != &o)
	{
		clear();
		insert(o);
	}
	return *this;
}

AsnBuf::AsnBuf(const char* pFilename)
{
	m_card = m_deck.insert(m_deck.begin(), new Card(new AsnFileSeg(pFilename)));
}

void AsnBuf::clear()
{
	for (m_card = m_deck.begin(); m_card != m_deck.end(); m_card++)
		delete *m_card;
	m_deck.clear();
}

void AsnBuf::PutByteRvs(char byte)
{

	// if empty add an AsnRvsBuf card
	//
	if (m_deck.empty())
		m_card = m_deck.insert(m_deck.begin(), new Card(new AsnRvsBuf));

	if ((*m_card)->rdbuf()->sputc(byte) == EOF)
	{
		m_card = m_deck.insert(m_deck.begin(), new Card(new AsnRvsBuf));
		(*m_card)->rdbuf()->sputc(byte);
	}
}

void AsnBuf::PutSegRvs(const char* seg, size_t segLen)
{
	// if empty add an AsnRvsBuf card
	//
	if (m_deck.empty())
		m_card = m_deck.insert(m_deck.begin(), new Card(new AsnRvsBuf));

	while (segLen > 0)
	{
		segLen -= (size_t)(*m_card)->rdbuf()->sputn(seg, segLen);
		if (segLen > 0)
		{
			// reuse any existing card(s)
			if (m_card == m_deck.begin())
				m_card = m_deck.insert(m_deck.begin(), new Card(new AsnRvsBuf));
			else
				--m_card;
		}
	}
}

// #ifdef _WIN32
// void AsnBuf::ResetMode(std::ios_base::open_mode mode) const
// #else // _WIN32
void AsnBuf::ResetMode(std::ios_base::openmode mode) const
// #endif // _WIN32
{
	AsnRvsBuf tmp;
	Deck::iterator i;
	i = m_deck.begin();

	while (i != m_deck.end())
	{
		if (mode & std::ios_base::out)
		{
			// dump all cards that are not AsnRvsBuf's because
			// those are the only cards that can be encoded into.
			//
			if ((*i)->bufType() != RVS_BUF_TYPE)
			{
				i = m_deck.erase(i);
				continue;
			}
		}
		(*i)->rdbuf()->pubseekpos(0, mode);
		i++;
	}

	if (mode == std::ios_base::in)
		m_card = m_deck.begin();
	else
	{
		m_card = m_deck.end();
		if (!m_deck.empty())
			--m_card;
	}
}

// GetFileSeg()
//
// Create a AsnFileSeg object from the current card in the deck
// for the length "segLen".
//
AsnFileSeg* AsnBuf::GetFileSeg(long segLen) const
{
	FUNC("AsnBuf::GetFileSeg()");

	// If the bufType == FILE_TYPE then the card is a AsnFileSeg
	// cast it and pass it to the AsnFileSeg constructor
	//
	if ((*m_card)->bufType() == FILE_TYPE)
		return (new AsnFileSeg((AsnFileSeg*)(*m_card)->rdbuf(), segLen));
	else
		throw BufferException("GetFileSeg called with non file card", STACK_ENTRY);
}

// PutFileSeg()
//
// Copy AsnFileSeg pointer into this AsnBuf's deck.  AsnBuf assumes
// responsibility of cleanup for it.
//
void AsnBuf::PutFileSeg(AsnFileSeg* pFs)
{
	m_card = m_deck.insert(m_deck.begin(), new Card(pFs));
}

char* AsnBuf::GetSeg(long segLen) const
{
	FUNC("AsnBuf::GetSeg()");

	char* seg = new char[segLen];
	if (seg == NULL)
		throw SNACC_MEMORY_EXCEPT(segLen, "seg");

	/* try to get the seg */
	try
	{
		GetSeg(seg, segLen);
		return seg;
	}
	catch (...) /* if failed free mem and throw */
	{
		delete[] seg;
		throw;
	}
}

// unsigned long AsnBuf::GetSeg(char *seg, long segLen) const
void AsnBuf::GetSeg(char* seg, long segLen) const
{
	FUNC("AsnBuf::GetSeg()");
	long bytesRead = 0;
	long lTmp;

	while (segLen > 0 && m_card != m_deck.end())
	{
		lTmp = (long)(*m_card)->rdbuf()->sgetn(&seg[bytesRead], segLen);

		bytesRead += lTmp;
		if (lTmp != segLen)
			m_card++;

		segLen -= lTmp;
	}
	if (segLen > 0)
		throw BufferException("Read past end of data", STACK_ENTRY);

	// return bytesRead;
}
// FUNCTION: GetSeg()
// PURPOSE: Retrieve the contents of the AsnBuf into a std::string.
//          segLen is defaulted 0.  If defaulted to 0 then the entire
//          buffer is returned.
//
void AsnBuf::GetSeg(std::string& seg, long segLen) const
{
	long i;

	FUNC("AsnBuf::GetSeg(std::string &seg, long segLen)");

	if (segLen == 0)
		segLen = length();

	if ((unsigned long)segLen > this->length()) // RWC; TOO MUCH DATA requested...

		throw BufferException("GetSeg attempt to read past end of data", STACK_ENTRY);
	seg.resize(segLen);
	for (i = 0; i < segLen; i++)
		seg[i] = GetByte();
}

bool AsnBuf::operator<(const AsnBuf& rhs) const
{
	bool lessThan = true;
	ResetMode();
	rhs.ResetMode();
	std::streambuf::int_type ch1;
	std::streambuf::int_type ch2;
	while (lessThan)
	{
		try
		{
			ch1 = GetUByte();
		}
		catch (BufferException&)
		{
			ch1 = EOF;
		}

		try
		{
			ch2 = rhs.GetUByte();
		}
		catch (BufferException&)
		{
			ch2 = EOF;
		}

		if ((ch1 == EOF) && (ch2 == EOF))
		{
			lessThan = false;
			break;
		}
		else if (ch2 == EOF)
		{
			lessThan = false;
			break;
		}
		else if (ch1 == EOF)
		{
			break;
		}

		if (ch1 > ch2)
		{
			lessThan = false;
			break;
		}
		else if (ch1 < ch2)
			break;
	}
	ResetMode();
	rhs.ResetMode();
	return lessThan;
}

// FUNCTION: length()
// PURPOSE;  Traverse all REMAINING cards in deck and calculate the overall length
//           of the AsnBuf.
//
unsigned long AsnBuf::length() const
{
	unsigned long bytesRemaining = 0;
	Deck::iterator tmpCard = m_card;
	int currPos, endPos;

	if (!m_deck.empty())
	{
		// AsnBufLoc readLoc = GetReadLoc();

		// ResetMode();    //RWC;

		while (tmpCard != m_deck.end())
		{
			//(*tmpCard)->rdbuf()->sgetc();
			currPos = (int)(*tmpCard)->rdbuf()->pubseekoff(0, std::ios_base::cur, std::ios_base::in);
			if (currPos == -1)
				currPos = 0;
			endPos = (int)(*tmpCard)->rdbuf()->pubseekoff(0, std::ios_base::end, std::ios_base::in);
			(*tmpCard)->rdbuf()->pubseekpos(currPos, std::ios_base::in);
			bytesRemaining += endPos - currPos;

			tmpCard++;
		}

		// SetReadLoc(readLoc);
	}

	return bytesRemaining;
}

/* JKG -- added 03/03/04 for support of unkown any's in extension additions                   */
/* For unkown any's we need to unget the tag and length bytes to be able to properly decode   */
/* unkown any's in extension additions, but outside the extension addition root list          */
void AsnBuf::UnGetBytes(long lBytesToPutBack) const
{
	FUNC("AsnBuf::UnGetByte()");

	while (lBytesToPutBack)
	{
		if (((*m_card)->rdbuf()->sungetc()) == EOF)
			if (m_card != m_deck.begin())
				m_card--;
			else
				throw BufferException("Failed putting bytes back", STACK_ENTRY);
		else
			lBytesToPutBack--;
	}
}

char AsnBuf::GetByte() const
{
	FUNC("AsnBuf::GetByte()");

	std::streambuf::int_type ch;

	if (m_card != m_deck.end())
	{
		/*
	 #ifdef _DEBUG
		 Card *tmpCard = *m_card;
	 #endif
		*/
		while ((ch = (*m_card)->rdbuf()->sbumpc()) == EOF)
		{
			m_card++;

			if ((m_card == m_deck.end()))
				throw BufferException("Read past end of data", STACK_ENTRY);
		}
	}
	else
		throw BufferException("Read past end of data", STACK_ENTRY);

	return (char)ch;
}

// FUNCTION: GrabAnyEx()
// PURPOSE : copy the current sequence of bytes (i.e. Tag Length and associated data)
//           into AsnBuf that was passed in. This is useful for copying the raw encoding
//           of any ANY out of an AsnBuf.
//			 This method superseeds GrabAny and separates getting the tag and the len from the method
// - anyBuf - the buffer to copy the data to (the data is the full object including tag, len and payload)
// - headerLen - the len of the header elements (tag and len)
// - payloadLen - the len of the payload after tag and len
// - bytesDecoded - the current position of the reader (behind the header elements)
//
void AsnBuf::GrabAnyEx(const AsnBuf& anyBuf, AsnLen headerLen, AsnLen payloadLen, AsnLen& bytesDecoded) const
{
	FUNC("AsnBuf::GrabAnyEx");

	// Our length
	auto ourLength = this->length();
	if (payloadLen == INDEFINITE_LEN)
	{
		ConsStringDeck deck(0);
		AsnLen lTmpbytesDecoded = 0;
		try
		{
			deck.Fill(*this, payloadLen, lTmpbytesDecoded);
		}
		catch (... /*std::exception &e*/)
		{
			throw BufferException("deck.Fill(...) failed, unexpected exception (STACK?)", STACK_ENTRY);
		}
		payloadLen = lTmpbytesDecoded;
	}
	else if (payloadLen > ourLength)
	{
		throw BufferException("len error from BDecLen call", STACK_ENTRY);
	}

	// Calculate the full length of the object including tag and len information
	auto fullLength = headerLen + payloadLen;

	// Reset our position which is currently past tag and header to the position of the tag element
	UnGetBytes(headerLen);

	AsnRvsBuf* pRvsBuf = new AsnRvsBuf(fullLength);
	GetSeg(pRvsBuf->m_buf, fullLength);

	pRvsBuf->m_pStart = pRvsBuf->m_buf;
	anyBuf.m_card = anyBuf.m_deck.insert(anyBuf.m_deck.begin(), new Card(pRvsBuf));

	bytesDecoded += payloadLen;
}

// FUNCTION: GrabAny()
// PURPOSE : copy the current sequence of bytes (i.e. Tag Length and associated data)
//           into AsnBuf that was passed in.  This is useful for copying the raw encoding
//           of any ANY out of an AsnBuf.
//
void AsnBuf::GrabAny(const AsnBuf& anyBuf, AsnLen& bytesDecoded) const
{
	FUNC("AsnBuf::GrabAny");

	// Store the len where we are (this is the len of the whole object, including the type tag)
	AsnLen tmpLen = bytesDecoded;
	// Store the current reader location to restore it later
	AsnBufLoc readLoc = GetReadLoc();
	// Decode tag of the ANY.  This will be encoded into AnyBuf after the length
	BDecTag(*this, bytesDecoded);
	// Decode length of the ANY. This will be encoded into anyBuf after the data.
	AsnLen payloadLen = BDecLen(*this, bytesDecoded);
	// Length of the header information (tag and len part)
	AsnLen headerLen = bytesDecoded - tmpLen;
	// Our length
	auto ourLength = this->length();

	if (payloadLen == INDEFINITE_LEN)
	{
		ConsStringDeck deck(0);
		AsnLen lTmpbytesDecoded = 0;
		try
		{
			deck.Fill(*this, payloadLen, lTmpbytesDecoded);
		}
		catch (... /*std::exception &e*/)
		{
			throw BufferException("deck.Fill(...) failed, unexpected exception (STACK?)", STACK_ENTRY);
		}
		payloadLen = lTmpbytesDecoded;
	}
	else if (payloadLen > ourLength)
	{
		throw BufferException("len error from BDecLen call", STACK_ENTRY);
	}

	// Restore the reader location
	SetReadLoc(readLoc);

	// Calculate the full length of the object including tag and len information
	auto fullLength = headerLen + payloadLen;

	AsnRvsBuf* pRvsBuf = new AsnRvsBuf(fullLength);
	GetSeg(pRvsBuf->m_buf, fullLength);

	pRvsBuf->m_pStart = pRvsBuf->m_buf;
	anyBuf.m_card = anyBuf.m_deck.insert(anyBuf.m_deck.begin(), new Card(pRvsBuf));
	bytesDecoded += payloadLen;
}

char AsnBuf::PeekByte() const
{
	FUNC("AsnBuf::PeekByte()");

	std::streambuf::int_type ch;
	if (m_card != m_deck.end())
	{
		if ((ch = (*m_card)->rdbuf()->sgetc()) == EOF)
		{
			m_card++;
			if ((m_card == m_deck.end()) || (ch = (*m_card)->rdbuf()->sgetc()) == EOF)
				throw BufferException("Read past end of data", STACK_ENTRY);
		}
	}
	else
		throw BufferException("Read past end of data", STACK_ENTRY);

	return (unsigned char)ch;
}

AsnBufLoc AsnBuf::GetReadLoc() const
{
	AsnBufLoc bl;
	bl.m_card = m_card;
	bl.m_offset = (long)(*m_card)->rdbuf()->pubseekoff(0, std::ios_base::cur, std::ios_base::in);
	if (bl.m_offset == -1)
		bl.m_offset = 0;
	return bl;
}

void AsnBuf::SetReadLoc(const AsnBufLoc& bl) const
{
	FUNC("AsnBuf::setReadLoc()");

	Deck::iterator i = m_deck.begin();

	// first make sure interator bl.m_card is between current card
	// start of the deck.
	//

	while (i != bl.m_card && i != m_deck.end())
		i++;

	if (i == bl.m_card)
	{
		ResetMode();
		m_card = bl.m_card;
		(*m_card)->rdbuf()->pubseekpos(bl.m_offset, std::ios_base::in);
	}
	else
	{
		throw BufferException("Invalid AsnBufLoc", STACK_ENTRY);
	}
}

// skip()
//
// skips ahead by the number of "skipBytes"
//
void AsnBuf::skip(size_t skipBytes)
{
	FUNC("AsnBuf::skip()");

	size_t nCardLen;
	while (skipBytes > 0 && m_card != m_deck.end())
	{
		nCardLen = (*m_card)->length();

		if (skipBytes > nCardLen)
		{
			skipBytes -= nCardLen;
			(*m_card)->rdbuf()->pubseekoff(0, std::ios_base::end, std::ios_base::in);
			m_card++;
		}
		else
		{
			(*m_card)->rdbuf()->pubseekoff(skipBytes, std::ios_base::cur, std::ios_base::in);
			skipBytes = 0;
		}
	}
	if (skipBytes > 0)
		throw BufferException("Skipped past end of buffer", STACK_ENTRY);
}

// insert()
//
// insert cards from b into this AsnBuf
//
// TBD: is it necessary to return the length?
//
long AsnBuf::splice(AsnBuf& b)
{
	// If the current card is empty, delete it
	if ((m_card != m_deck.end()) && ((*m_card == NULL) || ((*m_card)->length() == 0)))
	{
		delete *m_card;
		m_card = m_deck.erase(m_card);
	}

	long length = b.length();

	Deck::reverse_iterator ib;
	for (ib = b.m_deck.rbegin(); ib != b.m_deck.rend(); ++ib)
		m_card = m_deck.insert(m_deck.begin(), *ib);
	b.m_deck.clear();

	b.m_card = m_deck.end();

	return length;
}

void BDEC_2ND_EOC_OCTET(const SNACC::AsnBuf& b, SNACC::AsnLen& bytesDecoded)
{
	FUNC("BDEC_2ND_EOC_OCTET");

	if ((b.GetByte() != 0))
		throw EXCEPT("second octet of EOC not zero", DECODE_ERROR);

	bytesDecoded++;
}

void sortSet(std::list<SNACC::AsnBuf>& bufList)
{
	// std::greater<SNACC::AsnBuf> sortByByte;
	std::list<SNACC::AsnBuf> i;

	std::list<SNACC::AsnBuf>::iterator j;

	for (j = bufList.begin(); j != bufList.end(); j++)
		j->ResetMode();

	// bufList.sort(sortByByte);
	bufList.sort();
}

#define ASN_UNIVERSAL 0x00
#define ASN_APPLICATION 0x40
#define ASN_CONTEXT 0x80
#define ASN_PRIVATE 0xC0

/*
// Sort by encoding included tag, length, and data
//
bool std::greater<SNACC::AsnBuf>::operator()(const SNACC::AsnBuf &x,
											 const SNACC::AsnBuf &y) const
{
   AsnLen len;
   AsnTag xTag = (BDecTag(x, len) & 0xDFFFFFFF);
   AsnTag yTag = (BDecTag(y, len) & 0xDFFFFFFF);

   x.ResetMode();
   y.ResetMode();

   return (xTag > yTag);
}
*/

bool AsnBuf::operator==(const AsnBuf& b) const
{
	bool equal = true;
	ResetMode();
	b.ResetMode();
	std::streambuf::int_type ch1;
	std::streambuf::int_type ch2;
	while (equal)
	{
		try
		{
			ch1 = GetUByte();
		}
		catch (BufferException&)
		{
			ch1 = EOF;
		}

		try
		{
			ch2 = b.GetUByte();
		}
		catch (BufferException&)
		{
			ch2 = EOF;
		}

		if ((ch1 == EOF) && (ch2 == EOF))
			break;

		if (ch1 != ch2)
			equal = false;
	}
	ResetMode();
	b.ResetMode();
	return equal;
}

void AsnBuf::hexDump(std::ostream& os) const
{
	bool done = false;
	int ch;

	ResetMode();

	std::hex(os);

	while (!done)
	{

		try
		{
			ch = GetUByte();
			os << "0x";
			os << ch;
			os << "   ";
		}
		catch (...)
		{

			os.unsetf(std::ios_base::hex);
			os.unsetf(std::ios_base::hex);
			done = true;
		}
	}
}

std::ostream& operator<<(std::ostream& os, const SNACC::AsnBuf& b)
{
	SNACC::Deck::const_iterator card = b.deck().begin();
	while (card != b.deck().end())
	{
		if ((*card)->length() > 0)
			os << (*card)->rdbuf();
		card++;
	}
	os.flush();
	return os;
}

long Card::size()
{
	long currPos, endPos;

	currPos = (long)first->pubseekoff(0, std::ios_base::cur, std::ios_base::in);

	endPos = (long)first->pubseekoff(0, std::ios_base::end, std::ios_base::in);

	if (currPos != -1)
		first->pubseekpos(currPos, std::ios_base::in);

	if (endPos == -1)
		endPos = 0;

	return endPos;
}

long Card::length()
{
	long currPos, endPos;

	currPos = (long)first->pubseekoff(0, std::ios_base::cur, std::ios_base::in);

	if (currPos == -1)
		currPos = 0;

	endPos = (long)first->pubseekoff(0, std::ios_base::end, std::ios_base::in);

	first->pubseekpos(currPos, std::ios_base::in);

	if (endPos == -1)
		endPos = 0;

	return endPos - currPos;
}

#ifdef _DEBUG
void AsnBuf::status(std::ostream& os)
{
	std::cout << "**** AsnBuf Status ****\n";
	std::cout << "Card  Written   Bytes TBR    Type    Max Size\n";
	std::cout << "----------------------------------------------------------------------\n";

	Deck::iterator i;
	int j;
	for (j = 0, i = m_deck.begin(); i != m_deck.end(); i++, j++)
	{
		std::cout << j << "\t" << (*i)->size() << "\t" << (*i)->length() << "\t" << (*i)->bufTypeStr() << "\t";
		if ((*i)->bufType() == RVS_BUF_TYPE)
		{
			const AsnRvsBuf* pRvsBuf = (const AsnRvsBuf*)(*i)->rdbuf();
			std::cout << pRvsBuf->max_size() << "\n";
		}
		else
			std::cout << "?\n";
	}
	std::cout << "**** AsnBuf Status ****\n";
}

// AsnBufType { FILE_TYPE=0, RVS_BUF_TYPE, IN_MEM_TYPE, EXT_MEM_TYPE} ;

const char* Card::bufTypeStr()
{
	switch (this->bufType())
	{
		case RVS_BUF_TYPE:
			return "ASN_RVS_BUF";
		case FILE_TYPE:
			return "FILE_TYPE";
		case IN_MEM_TYPE:
			return "IN_MEM_TYPE";
		case EXT_MEM_TYPE:
			return "EXT_MEM_TYPE";
	}
	return NULL;
}

#endif
