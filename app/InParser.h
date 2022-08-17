#ifndef IN_PARSER_H
#define IN_PARSER_H

#include <stdint.h>

namespace IN_PARSER
{

/*******************************************************************/
/******************** InParser.h  ********************************/
/*******************************************************************/
class InPlaceParserInterface
{
public:
	virtual uint32_t ParseLine(uint32_t lineno, uint32_t argc, const char **argv) = 0;  // return TRUE to continue parsing, return FALSE to abort parsing process
};

enum SeparatorType
{
	ST_DATA,        // is data
	ST_HARD,        // is a hard separator
	ST_SOFT,        // is a soft separator
	ST_EOS          // is a comment symbol, and everything past this character should be ignored
};

class InPlaceParser
{
public:
	InPlaceParser(void)
	{
		Init();
	}

	InPlaceParser(char *data, uint32_t len)
	{
		Init();
		SetSourceData(data, len);
	}

	InPlaceParser(const char *fname)
	{
		Init();
		SetFile(fname);
	}

	~InPlaceParser(void);

	void Init(void)
	{
		mQuoteChar = 34;
		mData = 0;
		mLen = 0;
		mMyAlloc = false;
		for (uint32_t i = 0; i < 256; i++)
		{
			mHard[i] = ST_DATA;
			mHardString[i * 2] = (char)i;
			mHardString[i * 2 + 1] = 0;
		}
		mHard[0] = ST_EOS;
		mHard[32] = ST_SOFT;
		mHard[9] = ST_SOFT;
		mHard[13] = ST_SOFT;
		mHard[10] = ST_SOFT;
	}

	void SetFile(const char *fname); // use this file as source data to parse.

	void SetSourceData(char *data, uint32_t len)
	{
		mData = data;
		mLen = len;
		mMyAlloc = false;
	};

	uint32_t  Parse(InPlaceParserInterface *callback); // returns true if entire file was parsed, false if it aborted for some reason

	uint32_t ProcessLine(uint32_t lineno, char *line, InPlaceParserInterface *callback);

	const char ** GetArglist(char *source, uint32_t &count); // convert source string into an arg list, this is a destructive parse.

	void SetHardSeparator(char c) // add a hard separator
	{
		mHard[c] = ST_HARD;
	}

	void SetHard(char c) // add a hard separator
	{
		mHard[c] = ST_HARD;
	}


	void SetCommentSymbol(char c) // comment character, treated as 'end of string'
	{
		mHard[c] = ST_EOS;
	}

	void ClearHardSeparator(char c)
	{
		mHard[c] = ST_DATA;
	}


	void DefaultSymbols(void); // set up default symbols for hard separator and comment symbol of the '#' character.

	bool EOS(char c)
	{
		if (mHard[c] == ST_EOS)
		{
			return true;
		}
		return false;
	}

	void SetQuoteChar(char c)
	{
		mQuoteChar = c;
	}

private:


	inline char * AddHard(uint32_t &argc, const char **argv, char *foo);
	inline bool   IsHard(char c);
	inline char * SkipSpaces(char *foo);
	inline bool   IsWhiteSpace(char c);
	inline bool   IsNonSeparator(char c); // non separator, neither hard nor soft

	bool			mMyAlloc; // whether or not *I* allocated the buffer and am responsible for deleting it.
	char			*mData;  // ascii data to parse.
	uint32_t		mLen;   // length of data
	SeparatorType	mHard[256];
	char			mHardString[256 * 2];
	char			mQuoteChar;
};



} // end of IN_PARSER namespace

#endif
