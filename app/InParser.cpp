#include "InParser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

namespace IN_PARSER
{
	/*******************************************************************/
	/******************** InParser.cpp  ********************************/
	/*******************************************************************/
	void InPlaceParser::SetFile(const char *fname)
	{
		if (mMyAlloc)
		{
			free(mData);
		}
		mData = 0;
		mLen = 0;
		mMyAlloc = false;

		FILE *fph = fopen(fname, "rb");
		if (fph)
		{
			fseek(fph, 0L, SEEK_END);
			mLen = ftell(fph);
			fseek(fph, 0L, SEEK_SET);
			if (mLen)
			{
				mData = (char *)malloc(sizeof(char)*(mLen + 1));
				size_t ok = fread(mData, mLen, 1, fph);
				if (!ok)
				{
					free(mData);
					mData = 0;
				}
				else
				{
					mData[mLen] = 0; // zero byte terminate end of file marker.
					mMyAlloc = true;
				}
			}
			fclose(fph);
		}

	}

	InPlaceParser::~InPlaceParser(void)
	{
		if (mMyAlloc)
		{
			free(mData);
		}
	}

#define MAXARGS 512

	bool InPlaceParser::IsHard(char c)
	{
		return mHard[c] == ST_HARD;
	}

	char * InPlaceParser::AddHard(uint32_t &argc, const char **argv, char *foo)
	{
		while (IsHard(*foo))
		{
			const char *hard = &mHardString[*foo * 2];
			if (argc < MAXARGS)
			{
				argv[argc++] = hard;
			}
			foo++;
		}
		return foo;
	}

	bool   InPlaceParser::IsWhiteSpace(char c)
	{
		return mHard[c] == ST_SOFT;
	}

	char * InPlaceParser::SkipSpaces(char *foo)
	{
		while (!EOS(*foo) && IsWhiteSpace(*foo)) foo++;
		return foo;
	}

	bool InPlaceParser::IsNonSeparator(char c)
	{
		if (!IsHard(c) && !IsWhiteSpace(c) && c != 0) return true;
		return false;
	}


	uint32_t InPlaceParser::ProcessLine(uint32_t lineno, char *line, InPlaceParserInterface *callback)
	{
		uint32_t ret = 0;

		const char *argv[MAXARGS];
		uint32_t argc = 0;

		char *foo = line;

		while (!EOS(*foo) && argc < MAXARGS)
		{

			foo = SkipSpaces(foo); // skip any leading spaces

			if (EOS(*foo)) break;

			if (*foo == mQuoteChar) // if it is an open quote
			{
				foo++;
				if (argc < MAXARGS)
				{
					argv[argc++] = foo;
				}
				while (!EOS(*foo) && *foo != mQuoteChar) foo++;
				if (!EOS(*foo))
				{
					*foo = 0; // replace close quote with zero byte EOS
					foo++;
				}
			}
			else
			{

				foo = AddHard(argc, argv, foo); // add any hard separators, skip any spaces

				if (IsNonSeparator(*foo))  // add non-hard argument.
				{
					bool quote = false;
					if (*foo == mQuoteChar)
					{
						foo++;
						quote = true;
					}

					if (argc < MAXARGS)
					{
						argv[argc++] = foo;
					}

					if (quote)
					{
						while (*foo && *foo != mQuoteChar) foo++;
						if (*foo) *foo = 32;
					}

					// continue..until we hit an eos ..
					while (!EOS(*foo)) // until we hit EOS
					{
						if (IsWhiteSpace(*foo)) // if we hit a space, stomp a zero byte, and exit
						{
							*foo = 0;
							foo++;
							break;
						}
						else if (IsHard(*foo)) // if we hit a hard separator, stomp a zero byte and store the hard separator argument
						{
							const char *hard = &mHardString[*foo * 2];
							*foo = 0;
							if (argc < MAXARGS)
							{
								argv[argc++] = hard;
							}
							foo++;
							break;
						}
						foo++;
					} // end of while loop...
				}
			}
		}

		if (argc)
		{
			ret = callback->ParseLine(lineno, argc, argv);
		}

		return ret;
	}

	uint32_t  InPlaceParser::Parse(InPlaceParserInterface *callback) // returns true if entire file was parsed, false if it aborted for some reason
	{
		if (!mData) return 0;

		uint32_t ret = 0;

		uint32_t lineno = 0;

		char *foo = mData;
		char *begin = foo;


		while (*foo)
		{
			if (*foo == 10 || *foo == 13)
			{
				lineno++;
				*foo = 0;

				if (*begin) // if there is any data to parse at all...
				{
					uint32_t v = ProcessLine(lineno, begin, callback);
					if (v) ret = v;
				}

				foo++;
				if (*foo == 10) foo++; // skip line feed, if it is in the carriage-return line-feed format...
				begin = foo;
			}
			else
			{
				foo++;
			}
		}

		lineno++; // last line.

		uint32_t v = ProcessLine(lineno, begin, callback);
		if (v) ret = v;
		return ret;
	}


	void InPlaceParser::DefaultSymbols(void)
	{
		SetHardSeparator(',');
		SetHardSeparator('(');
		SetHardSeparator(')');
		SetHardSeparator('=');
		SetHardSeparator('[');
		SetHardSeparator(']');
		SetHardSeparator('{');
		SetHardSeparator('}');
		SetCommentSymbol('#');
	}


	const char ** InPlaceParser::GetArglist(char *line, uint32_t &count) // convert source string into an arg list, this is a destructive parse.
	{
		const char **ret = 0;

		static const char *argv[MAXARGS];
		uint32_t argc = 0;

		char *foo = line;

		while (!EOS(*foo) && argc < MAXARGS)
		{

			foo = SkipSpaces(foo); // skip any leading spaces

			if (EOS(*foo)) break;

			if (*foo == mQuoteChar) // if it is an open quote
			{
				foo++;
				if (argc < MAXARGS)
				{
					argv[argc++] = foo;
				}
				while (!EOS(*foo) && *foo != mQuoteChar) foo++;
				if (!EOS(*foo))
				{
					*foo = 0; // replace close quote with zero byte EOS
					foo++;
				}
			}
			else
			{

				foo = AddHard(argc, argv, foo); // add any hard separators, skip any spaces

				if (IsNonSeparator(*foo))  // add non-hard argument.
				{
					bool quote = false;
					if (*foo == mQuoteChar)
					{
						foo++;
						quote = true;
					}

					if (argc < MAXARGS)
					{
						argv[argc++] = foo;
					}

					if (quote)
					{
						while (*foo && *foo != mQuoteChar) foo++;
						if (*foo) *foo = 32;
					}

					// continue..until we hit an eos ..
					while (!EOS(*foo)) // until we hit EOS
					{
						if (IsWhiteSpace(*foo)) // if we hit a space, stomp a zero byte, and exit
						{
							*foo = 0;
							foo++;
							break;
						}
						else if (IsHard(*foo)) // if we hit a hard separator, stomp a zero byte and store the hard separator argument
						{
							const char *hard = &mHardString[*foo * 2];
							*foo = 0;
							if (argc < MAXARGS)
							{
								argv[argc++] = hard;
							}
							foo++;
							break;
						}
						foo++;
					} // end of while loop...
				}
			}
		}

		count = argc;
		if (argc)
		{
			ret = argv;
		}

		return ret;
	}


} // end of IN_PARSER namespace
