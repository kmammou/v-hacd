#include "FastXml.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <new>

#define DEBUG_LOG 0

namespace FAST_XML
{

#define MIN_CLOSE_COUNT 2
#define DEFAULT_READ_BUFFER_SIZE (16*1024)

#define DEBUG_ASSERT(x) //assert(x)
#define DEBUG_ALWAYS_ASSERT() DEBUG_ASSERT(0)

#define U(c) ((unsigned char)(c))

static inline bool isSingleOrDoubleQuote(char c)
{
	return c == 34 || c == 39;
}

// Abstract interface for reading the XML file in; from either a file or memory
class FileBuffer
{
public:
	virtual uint32_t read(void *dest, uint32_t readLen) = 0;
	virtual uint32_t seekRead(uint32_t loc) = 0;
	virtual uint32_t tellRead(void) const = 0;
	virtual uint32_t getFileLength(void) const = 0;
	virtual void release(void) = 0;
};

// Simple class to read from a file
class FileReader : public FileBuffer
{
public:
	FileReader(const char *fname)
	{
		mFph = fopen(fname, "rb");
		if (mFph)
		{
			fseek(mFph, 0L, SEEK_END);
			mFileSize = uint32_t(ftell(mFph));
			fseek(mFph, 0L, SEEK_SET);
		}
	}

	~FileReader(void)
	{
		if (mFph)
		{
			fclose(mFph);
		}
	}

	virtual uint32_t read(void *dest, uint32_t readLen) final
	{
		uint32_t readBytes = 0;
		if (mFph)
		{
			readBytes = uint32_t(fread(dest, 1, readLen, mFph));
		}
		mReadLocation += readBytes;
		return readBytes;
	}

	virtual uint32_t seekRead(uint32_t loc) final
	{
		if (mFph)
		{
			fseek(mFph, loc, SEEK_SET);
			mReadLocation = uint32_t(ftell(mFph));
		}
		return mReadLocation;
	}

	virtual uint32_t tellRead(void) const final
	{
		return mReadLocation;
	}

	virtual uint32_t getFileLength(void) const final
	{
		return mFileSize;
	}

	virtual void release(void)
	{
		delete this;
	}

private:
	uint32_t	mFileSize{ 0 };
	uint32_t	mReadLocation{ 0 };
	FILE		*mFph{ nullptr };
};

// Abstract interface for reading the XML file in; from either a file or memory
class FileMemory : public FileBuffer
{
public:
	FileMemory(const void *data, uint32_t dlen)
	{
		mBuffer = (const uint8_t *)data;
		mFileLength = dlen;
		mReadLocation = 0;
	}

	virtual uint32_t read(void *dest, uint32_t readLen)
	{
		assert(mReadLocation <= mFileLength);
		uint32_t bytesAvailable = mFileLength - mReadLocation;
		if (readLen > bytesAvailable)
		{
			readLen = bytesAvailable;
		}
		if (readLen)
		{
			memcpy(dest, &mBuffer[mReadLocation], readLen);
			mReadLocation += readLen;
		}
		return readLen;
	}

	virtual uint32_t seekRead(uint32_t loc)
	{
		if (loc > mFileLength)
		{
			loc = mFileLength;
		}
		mReadLocation = loc;
		return loc;
	}

	virtual uint32_t tellRead(void) const
	{
		return mReadLocation;
	}

	virtual uint32_t getFileLength(void) const
	{
		return mFileLength;
	}

	virtual void release(void)
	{
		delete this;
	}
private:
	const uint8_t		*mBuffer{ nullptr };
	uint32_t			mFileLength{ 0 };
	uint32_t			mReadLocation{ 0 };
};


class MyFastXml : public FastXml
{
public:
	enum CharType
	{
		CT_DATA,
		CT_EOF,
		CT_SOFT,
		CT_END_OF_ELEMENT, // either a forward slash or a greater than symbol
		CT_END_OF_LINE,
	};

	MyFastXml(void)
	{
		mStreamFromMemory = true;
		mCallback = nullptr;
		memset(mTypes, CT_DATA, sizeof(mTypes));
		mTypes[0] = CT_EOF;
		mTypes[U(' ')] = mTypes[U('\t')] = CT_SOFT;
		mTypes[U('/')] = mTypes[U('>')] = mTypes[U('?')] = CT_END_OF_ELEMENT;
		mTypes[U('\n')] = mTypes[U('\r')] = CT_END_OF_LINE;
		mError = 0;
		mStackIndex = 0;
		mFileBuf = nullptr;
		mReadBufferEnd = nullptr;
		mReadBuffer = nullptr;
		mReadBufferSize = DEFAULT_READ_BUFFER_SIZE;
		mOpenCount = 0;
		mLastReadLoc = 0;
		for (uint32_t i=0; i<(MAX_STACK+1); i++)
		{
			mStack[i] = nullptr;
			mStackAllocated[i] = false;
		}
#if DEBUG_LOG
		mFph = fopen("xml_log.txt","wb");
#endif
	}

	virtual ~MyFastXml(void)
	{
		releaseMemory();
#if DEBUG_LOG
		if ( mFph )
		{
			fclose(mFph);
		}
#endif
	}

#if DEBUG_LOG
	void indent(void)
	{
		if ( mFph )
		{
			for (uint32_t i=0; i<mStackIndex; i++)
			{
				fprintf(mFph,"\t");
				fflush(mFph);
			}
		}
	}
#endif

	char *processClose(char c, const char *element, char *scan, uint32_t argc, const char **argv, FastXml::Callback *iface,bool &isError)
	{
		isError = true; // by default, if we return null it's due to an error.
		if ( c == '/' || c == '?' )
		{
			char *slash = (char *)strchr(element, c);
			if( slash )
				*slash = 0;

			if( c == '?' && strcmp(element, "xml") == 0 )
			{
				if( !iface->processXmlDeclaration(argc/2, argv, 0, mLineNo) )
					return nullptr;
			}
			else
			{
#if DEBUG_LOG
				if ( mFph )
				{
					indent();
					fprintf(mFph,"<%s ",element);
					for (uint32_t i=0; i<argc/2; i++)
					{
						fprintf(mFph," %s=\"%s\"", argv[i*2], argv[i*2+1] );
					}
					fprintf(mFph,">\r\n");
					fflush(mFph);
				}
#endif

				if ( !iface->processElement(element, argc, argv, 0, mLineNo) )
				{
					mError = "User aborted the parsing process";
					return nullptr;
				}

				pushElement(element);

				const char *close = popElement();
#if DEBUG_LOG
				indent();
				fprintf(mFph,"</%s>\r\n", close );
				fflush(mFph);
#endif	
				if( !iface->processClose(close,mStackIndex,isError,mLineNo) )
				{
					return nullptr;
				}
			}

			if ( !slash )
				++scan;
		}
		else
		{
			scan = skipNextData(scan);
			char *data = scan; // this is the data portion of the element, only copies memory if we encounter line feeds
			char *dest_data = 0;
			while ( *scan && *scan != '<' )
			{
				if ( mTypes[U(*scan)] == CT_END_OF_LINE )
				{
					if ( *scan == '\r' ) mLineNo++;
					dest_data = scan;
					*dest_data++ = ' '; // replace the linefeed with a space...
					scan = skipNextData(scan);
					while ( *scan && *scan != '<' )
					{
						if ( mTypes[U(*scan)] == CT_END_OF_LINE )
						{
							if ( *scan == '\r' ) mLineNo++;
							*dest_data++ = ' '; // replace the linefeed with a space...
							scan = skipNextData(scan);
						}
						else
						{
							*dest_data++ = *scan++;
						}
					}
					break;
				}
				else
					++scan;
			}

			if ( *scan == '<' )
			{
				if ( scan[1] != '/' )
				{
					assert(mOpenCount>0);
					mOpenCount--;
				}
				if ( dest_data )
				{
					*dest_data = 0;
				}
				else
				{
					*scan = 0;
				}

				scan++; // skip it..

				if ( *data == 0 ) data = 0;

#if DEBUG_LOG
				if ( mFph )
				{
					indent();
					fprintf(mFph,"<%s ",element);
					for (uint32_t i=0; i<argc/2; i++)
					{
						fprintf(mFph," %s=\"%s\"", argv[i*2], argv[i*2+1] );
					}
					fprintf(mFph,">\r\n");
					if ( data )
					{
						indent();
						fprintf(mFph,"%s\r\n", data );
					}
					fflush(mFph);
				}
#endif
				if ( !iface->processElement(element, argc, argv, data, mLineNo) )
				{
					mError = "User aborted the parsing process";
					return 0;
				}

				pushElement(element);

				// check for the comment use case...
				if ( scan[0] == '!' && scan[1] == '-' && scan[2] == '-' )
				{
					scan+=3;
					while ( *scan && *scan == ' ' )
						++scan;

					char *comment = scan;
					char *comment_end = strstr(scan, "-->");
					if ( comment_end )
					{
						*comment_end = 0;
						scan = comment_end+3;
						if( !iface->processComment(comment) )
						{
							mError = "User aborted the parsing process";
							return 0;
						}
					}
				}
				else if ( *scan == '/' )
				{
					scan = processClose(scan, iface, isError);
					if( scan == nullptr ) 
					{
						return nullptr;
					}
				}
			}
			else
			{
				mError = "Data portion of an element wasn't terminated properly";
				return nullptr;
			}
		}

		if ( mOpenCount < MIN_CLOSE_COUNT )
		{
			scan = readData(scan);
		}

		return scan;
	}

	char *processClose(char *scan, FastXml::Callback *iface,bool &isError)
	{
		const char *start = popElement(), *close = start;
		if( scan[1] != '>')
		{
			scan++;
			close = scan;
			while ( *scan && *scan != '>' ) scan++;
			*scan = 0;
		}

		if( 0 != strcmp(start, close) )
		{
			mError = "Open and closing tags do not match";
			return 0;
		}
#if DEBUG_LOG
		indent();
		fprintf(mFph,"</%s>\r\n", close );
		fflush(mFph);
#endif	
		if( !iface->processClose(close,mStackIndex,isError,mLineNo) )
		{
			// we need to set the read pointer!
			uint32_t offset = (uint32_t)(mReadBufferEnd-scan)-1;
			uint32_t readLoc = mLastReadLoc-offset;
			mFileBuf->seekRead(readLoc); 
			return nullptr;
		}
		++scan;

		return scan;
	}

	virtual bool processXml(const char *fname,Callback *callback)
	{
		releaseMemory();
		mStreamFromMemory = false;
		mCallback = callback;
		FileReader *fr = new FileReader(fname);
		mFileBuf = static_cast<FileBuffer *>(fr);
		return processXml(mCallback);
	}

	// Process an XML that has been pre-loaded into memory
	virtual bool processXml(const void *data, uint32_t dataLen, Callback *iface) final
	{
		releaseMemory();
		mStreamFromMemory = true;
		mCallback = iface;
		FileMemory *fm = new FileMemory(data, dataLen);
		mFileBuf = static_cast<FileBuffer *>(fm);
		return processXml(mCallback);
	}

	// if we have finished processing the data we had pending..
	char * readData(char *scan)
	{
		for (uint32_t i=0; i<(mStackIndex+1); i++)
		{
			if ( !mStackAllocated[i] )
			{
				const char *text = mStack[i];
				if ( text )
				{
					uint32_t tlen = (uint32_t)strlen(text);
					mStack[i] = (const char *)malloc(tlen+1);
					memcpy((void *)mStack[i],text,tlen+1);
					mStackAllocated[i] = true;

				}
			}
		}

		if ( !mStreamFromMemory )
		{
			if ( scan == nullptr )
			{
				uint32_t seekLoc = mFileBuf->tellRead();
				mReadBufferSize = (mFileBuf->getFileLength()-seekLoc);
			}
			else
			{
				return scan;
			}
		}

		if ( mReadBuffer == nullptr )
		{
			mReadBuffer = (char *)malloc(mReadBufferSize+1);
		}
		uint32_t offset = 0;
		uint32_t readLen = mReadBufferSize;

		if ( scan )
		{
			offset = (uint32_t)(scan - mReadBuffer );
			uint32_t copyLen = mReadBufferSize-offset;
			if ( copyLen )
			{
				assert(scan >= mReadBuffer);
				memmove(mReadBuffer,scan,copyLen);
				mReadBuffer[copyLen] = 0;
				readLen = mReadBufferSize - copyLen;
			}
			offset = copyLen;
		}

		uint32_t readCount = mFileBuf->read(&mReadBuffer[offset],readLen);

		while ( readCount > 0 )
		{

			mReadBuffer[readCount+offset] = 0; // end of string terminator...
			mReadBufferEnd = &mReadBuffer[readCount+offset];

			const char *scan2 = &mReadBuffer[offset];
			while ( *scan2 )
			{
				if ( *scan2 == '<' && scan2[1] != '/' )
				{
					mOpenCount++;
				}
				scan2++;
			}

			if ( mOpenCount < MIN_CLOSE_COUNT )
			{
				uint32_t oldSize = (uint32_t)(mReadBufferEnd-mReadBuffer);
				mReadBufferSize = mReadBufferSize*2;
				char *oldReadBuffer = mReadBuffer;
				mReadBuffer = (char *)malloc(mReadBufferSize+1);
				memcpy(mReadBuffer,oldReadBuffer,oldSize);
				free(oldReadBuffer);
				offset = oldSize;
				uint32_t readSize = mReadBufferSize - oldSize;
				readCount = mFileBuf->read(&mReadBuffer[offset],readSize);
				if ( readCount == 0 )
					break;
			}
			else
			{
				break;
			}
		}
		mLastReadLoc = mFileBuf->tellRead();

		return mReadBuffer;
	}

	bool processXml(FastXml::Callback *iface)
	{
		bool ret = true;

		const int MAX_ATTRIBUTE = 2048; // can't imagine having more than 2,048 attributes in a single element right?

		mLineNo = 1;

		char *element, *scan = readData(0);

		while( *scan )
		{

			scan = skipNextData(scan);

			if( *scan == 0 ) break;

			if( *scan == '<' )
			{

				if ( scan[1] != '/' )
				{
					assert(mOpenCount>0);
					mOpenCount--;
				}
				scan++;

				if( *scan == '?' ) //Allow xml declarations
				{
					scan++;
				}
				else if ( scan[0] == '!' && scan[1] == '-' && scan[2] == '-' )
				{
					scan+=3;
					while ( *scan && *scan == ' ' )
						scan++;
					char *comment = scan, *comment_end = strstr(scan, "-->");
					if ( comment_end )
					{
						*comment_end = 0;
						scan = comment_end+3;
						if( !iface->processComment(comment) )
						{
							mError = "User aborted the parsing process";
							DEBUG_ALWAYS_ASSERT();
							return false;
						}
					}
					continue;
				}
				else if ( scan[0] == '!' ) //Allow doctype
				{
					scan++;

					//DOCTYPE syntax differs from usual XML so we parse it here

					//Read DOCTYPE
					const char *tag = "DOCTYPE";
					if( !strstr(scan, tag) )
					{
						mError = "Invalid DOCTYPE";
						DEBUG_ALWAYS_ASSERT();
						return false;
					}

					scan += strlen(tag);

					//Skip whites
					while(  CT_SOFT == mTypes[U(*scan)] )
						++scan;

					//Read rootElement
					const char *rootElement = scan;
					while( CT_DATA == mTypes[U(*scan)] )
						++scan;

					char *endRootElement = scan;

					//TODO: read remaining fields (fpi, uri, etc.)
					while( CT_END_OF_ELEMENT != mTypes[U(*scan++)] );

					*endRootElement = 0;

					if( !iface->processDoctype(rootElement, 0, 0, 0) )
					{
						mError = "User aborted the parsing process";
						DEBUG_ALWAYS_ASSERT();
						return false;
					}

					continue; //Restart loop
				}
			}


			if( *scan == '/' )
			{
				bool isError;
				scan = processClose(scan, iface, isError);
				if( !scan )
				{
					if ( isError )
					{
						DEBUG_ALWAYS_ASSERT();
						mError = "User aborted the parsing process";
					}
					return !isError;
				}
			}
			else
			{
				if( *scan == '?' )
					scan++;
				element = scan;
				uint32_t argc = 0;
				const char *argv[MAX_ATTRIBUTE];
				bool close;
				scan = nextSoftOrClose(scan, close);
				if( close )
				{
					char c = *(scan-1);
					if ( c != '?' && c != '/' )
					{
						c = '>';
					}
					*scan++ = 0;
					bool isError;
					scan = processClose(c, element, scan, argc, argv, iface, isError);
					if ( !scan )
					{
						if ( isError )
						{
							DEBUG_ALWAYS_ASSERT();
							mError = "User aborted the parsing process";
						}
						return !isError;
					}
				}
				else
				{
					if ( *scan == 0 )
					{
						return ret;
					}

					*scan = 0; // place a zero byte to indicate the end of the element name...
					scan++;

					while ( *scan )
					{
						scan = skipNextData(scan); // advance past any soft seperators (tab or space)

						if ( mTypes[U(*scan)] == CT_END_OF_ELEMENT )
						{
							char c = *scan++;
							if( '?' == c )
							{
								if( '>' != *scan ) //?>
								{
									DEBUG_ALWAYS_ASSERT();
									return false;
								}

								scan++;
							}
							bool isError;
							scan = processClose(c, element, scan, argc, argv, iface, isError);
							if ( !scan )
							{
								if ( isError )
								{
									DEBUG_ALWAYS_ASSERT();
									mError = "User aborted the parsing process";
								}
								return !isError;
							}
							break;
						}
						else
						{
							if( argc >= MAX_ATTRIBUTE )
							{
								DEBUG_ALWAYS_ASSERT();
								mError = "encountered too many attributes";
								return false;
							}
							argv[argc] = scan;
							scan = nextSep(scan);  // scan up to a space, or an equal
							if( *scan )
							{
								if( *scan != '=' )
								{
									*scan = 0;
									scan++;
									while ( *scan && *scan != '=' ) scan++;
									if ( *scan == '=' ) scan++;
								}
								else
								{
									*scan=0;
									scan++;
								}

								if( *scan ) // if not eof...
								{
									scan = skipNextData(scan);
									if( isSingleOrDoubleQuote(*scan) )
									{
										scan++;
										argc++;
										argv[argc] = scan;
										argc++;
										while ( *scan && *scan != 34 ) scan++;
										if( isSingleOrDoubleQuote(*scan) )
										{
											*scan = 0;
											scan++;
										}
										else
										{
											DEBUG_ALWAYS_ASSERT();
											mError = "Failed to find closing quote for attribute";
											return false;
										}
									}
									else
									{
										//mError = "Expected quote to begin attribute";
										//return false;
										// PH: let's try to have a more graceful fallback
										argc--;
										while(*scan != '/' && *scan != '>' && *scan != 0)
											scan++;
									}
								}
							} //if( *scan )
						} //if ( mTypes[*scan]
					} //if( close )
				} //if( *scan == '/'
			} //while( *scan )
		}

		if( mStackIndex )
		{
			DEBUG_ALWAYS_ASSERT();
			mError = "Invalid file format";
			return false;
		}

		return ret;
	}

	const char *getError(uint32_t &lineno)
	{
		const char *ret = mError;
		lineno = mLineNo;
		mError = 0;
		return ret;
	}

	virtual void release(void)
	{
		delete this;
	}

private:

	inline void releaseMemory(void)
	{
		if (mFileBuf)
		{
			mFileBuf->release();
			mFileBuf = nullptr;
		}
		free(mReadBuffer);
		mReadBuffer = nullptr;
		mLastReadLoc = 0;
		mStackIndex = 0;
		mReadBufferEnd = nullptr;
		mOpenCount = 0;
		mError = nullptr;
		for (uint32_t i=0; i<(mStackIndex+1); i++)
		{
			if ( mStackAllocated[i] )
			{
				free((void *)mStack[i]);
				mStackAllocated[i] = false;
			}
			mStack[i] = nullptr;
		}
	}

	inline char *nextSoft(char *scan)
	{
		while ( *scan && mTypes[U(*scan)] != CT_SOFT ) scan++;
		return scan;
	}

	inline char *nextSoftOrClose(char *scan, bool &close)
	{
		while ( *scan && mTypes[U(*scan)] != CT_SOFT && *scan != '>' ) scan++;
		close = *scan == '>';
		return scan;
	}

	inline char *nextSep(char *scan)
	{
		while ( *scan && mTypes[U(*scan)] != CT_SOFT && *scan != '=' ) scan++;
		return scan;
	}

	inline char *skipNextData(char *scan)
	{
		// while we have data, and we encounter soft seperators or line feeds...
		while ( *scan && mTypes[U(*scan)] == CT_SOFT || mTypes[U(*scan)] == CT_END_OF_LINE )
		{
			if ( *scan == '\n' ) mLineNo++;
			scan++;
		}
		return scan;
	}

	void pushElement(const char *element)
	{
		assert( mStackIndex < MAX_STACK );
		if( mStackIndex < MAX_STACK )
		{
			if ( mStackAllocated[mStackIndex] )
			{
				free((void *)mStack[mStackIndex]);
				mStackAllocated[mStackIndex] = false;
			}
			mStack[mStackIndex++] = element;
		}
	}

	const char *popElement(void)
	{
		assert(mStackIndex>0);
		if ( mStackAllocated[mStackIndex] )
		{
			free((void*)mStack[mStackIndex]);
			mStackAllocated[mStackIndex] = false;
		}
		mStack[mStackIndex] = nullptr;
		return mStackIndex ? mStack[--mStackIndex] : nullptr;
	}

	static const unsigned MAX_STACK = 2048;

	char mTypes[256];

	FileBuffer *mFileBuf;

	char			*mReadBuffer;
	char			*mReadBufferEnd;

	uint32_t	mOpenCount;
	uint32_t	mReadBufferSize;
	uint32_t	mLastReadLoc;

	uint32_t mLineNo;
	const char *mError;
	uint32_t mStackIndex;
	const char *mStack[MAX_STACK+1];
	bool		mStreamFromMemory;
	bool		mStackAllocated[MAX_STACK+1];
	Callback	*mCallback;
#if DEBUG_LOG
	FILE	*mFph;
#endif
};

const char *getAttribute(const char *attr, uint32_t argc, const char **argv)
{
	uint32_t count = argc/2;
	for(uint32_t i = 0; i < count; ++i)
	{
		const char *key = argv[i*2], *value = argv[i*2+1];
		if( strcmp(key, attr) == 0 )
			return value;
	}

	return 0;
}

FastXml * FastXml::create(void)
{
	MyFastXml *m = new MyFastXml;
	return static_cast<FastXml *>(m);
}

}	// end of namespace
