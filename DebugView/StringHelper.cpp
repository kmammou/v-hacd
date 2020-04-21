#include "StringHelper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <float.h>
#include <math.h>

namespace STRING_HELPER
{

int32_t stringFormatV(char* dst, size_t dstSize, const char* src, va_list arg)
{
	return ::vsnprintf(dst, dstSize, src, arg);
}

int32_t stringFormat(char* dst, size_t dstSize, const char* format, ...)
{
	va_list arg;
	va_start(arg, format);
	int32_t r = stringFormatV(dst, dstSize, format, arg);
	va_end(arg);
	return r;
}

static inline bool isWhitespace(char c)
{
	if (c == ' ' || c == 9 || c == 13 || c == 10 || c == ',') return true;
	return false;
}


const char * skipWhitespace(const char *str)
{
	while (*str && isWhitespace(*str)) str++;
	return str;
}


#define MAXNUM 32

static inline char toLower(char c)
{
	if (c >= 'A' && c <= 'Z') c += 32;
	return c;
}

uint32_t getHex(uint8_t c)
{
	uint32_t v = 0;
	c = toLower(c);
	if (c >= '0' && c <= '9')
		v = c - '0';
	else
	{
		if (c >= 'a' && c <= 'f')
		{
			v = 10 + c - 'a';
		}
	}
	return v;
}

uint8_t getHEX1(const char *foo, const char **endptr)
{
	uint32_t ret = 0;

	ret = (getHex(foo[0]) << 4) | getHex(foo[1]);

	if (endptr)
	{
		*endptr = foo + 2;
	}

	return (uint8_t)ret;
}


uint16_t getHEX2(const char *foo, const char **endptr)
{
	uint32_t ret = 0;

	ret = (getHex(foo[0]) << 12) | (getHex(foo[1]) << 8) | (getHex(foo[2]) << 4) | getHex(foo[3]);

	if (endptr)
	{
		*endptr = foo + 4;
	}

	return (uint16_t)ret;
}

uint32_t getHEX4(const char *foo, const char **endptr)
{
	uint32_t ret = 0;

	for (uint32_t i = 0; i < 8; i++)
	{
		ret = (ret << 4) | getHex(foo[i]);
	}

	if (endptr)
	{
		*endptr = foo + 8;
	}

	return ret;
}

uint32_t getHEX(const char *foo, const char **endptr)
{
	uint32_t ret = 0;

	while (*foo)
	{
		uint8_t c = toLower(*foo);
		uint32_t v = 0;
		if (c >= '0' && c <= '9')
			v = c - '0';
		else
		{
			if (c >= 'a' && c <= 'f')
			{
				v = 10 + c - 'a';
			}
			else
				break;
		}
		ret = (ret << 4) | v;
		foo++;
	}

	if (endptr) *endptr = foo;

	return ret;
}


float	getFloatValue(const char *str, const char **next)
{
	float ret = 0;

	if (str == nullptr || *str == 0)
	{
		return 0;
	}

	if (next) *next = nullptr;

	str = skipWhitespace(str);

	char dest[MAXNUM];
	char *dst = dest;
	const char *hex = 0;

	for (uint32_t i = 0; i < (MAXNUM - 1); i++)
	{
		char c = *str;
		if (c == 0 || isWhitespace(c))
		{
			if (next) *next = str;
			break;
		}
		else if (c == '$')
		{
			hex = str + 1;
		}
		*dst++ = toLower(c);
		str++;
	}

	*dst = 0;

	if (hex)
	{
		uint32_t iv = getHEX(hex, 0);
		float *v = (float *)&iv;
		ret = *v;
	}
	else if (dest[0] == 'f')
	{
		if (strcmp(dest, "fltmax") == 0 || strcmp(dest, "fmax") == 0)
		{
			ret = FLT_MAX;
		}
		else if (strcmp(dest, "fltmin") == 0 || strcmp(dest, "fmin") == 0)
		{
			ret = FLT_MIN;
		}
	}
	else if (dest[0] == 't') // t or 'true' is treated as the value '1'.
	{
		ret = 1;
	}
	else
	{
		ret = (float)atof(dest);
	}
	return ret;
}

void	getVec3(const char *str, const char **next, float &x, float &y, float &z)
{
	x = 0;
	y = 0;
	z = 0;
	const char *mynext = nullptr;
	x = getFloatValue(str, &mynext);
	if (*mynext)
	{
		y = getFloatValue(mynext, &mynext);
		if (mynext)
		{
			z = getFloatValue(mynext, &mynext);
		}
	}
	if (next)
	{
		*next = mynext;
	}
}

void	getVec4(const char *str, const char **next, float &x, float &y, float &z,float &w)
{
	x = 0;
	y = 0;
	z = 0;
	w = 0;
	const char *mynext = nullptr;
	x = getFloatValue(str, &mynext);
	if (*mynext)
	{
		y = getFloatValue(mynext, &mynext);
		if (mynext)
		{
			z = getFloatValue(mynext, &mynext);
			if (mynext)
			{
				w = getFloatValue(mynext, &mynext);
			}
		}
	}
	if (next)
	{
		*next = mynext;
	}
}

static bool isDigit(char c)
{
	return c >= '0' && c <= '9';
}

static const char *skipDigits(const char *str)
{
	while (*str)
	{
		if (isDigit(*str))
		{
			str++;
		}
		else
		{
			break;
		}
	}
	if (*str == 0)
	{
		str = nullptr;
	}
	return str;
}

static const char *skipNonDigits(const char *str)
{
	while (*str)
	{
		if (isDigit(*str))
		{
			break;
		}
		else
		{
			str++;
		}
	}
	if (*str == 0 )
	{
		str = nullptr;
	}
	return str;
}

uint32_t	getUint32Value(const char *str, const char **next)
{
	uint32_t ret = 0;
	*next = nullptr;

	str = skipNonDigits(str);
	if (str)
	{
		ret = atoi(str);
		str = skipDigits(str);
		if (str)
		{
			str = skipNonDigits(str);
		}
	}
	*next = str;

	return ret;
}

bool getBool(const char *v)
{
	bool ret = false;
	if (v && *v)
	{
		if (isDigit(*v))
		{
			int32_t iv = atoi(v);
			ret = iv == 0 ? false : true; // any non-zero integer value is treated as 'true'
		}
		else
		{
			// upper or lower case 't' means 'true' upper or lower case 'y' means 'yes'
			switch (*v)
			{
			case 't':
			case 'T':
			case 'y':
			case 'Y':
				ret = true;
				break;
			}
		}
	}
	return ret;
}

void		normalizePathSlashes(char *fname)
{
	while (*fname)
	{
		if (*fname == '/')
		{
			*fname = '\\';
		}
		fname++;
	}
}

} // End of STRING_HELPER namespace

