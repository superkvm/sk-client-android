

#include "ultrajson.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <float.h>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

static const double g_pow10[] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};
static const char g_hexChars[] = "0123456789abcdef";
static const char g_escapeChars[] = "0123456789\\b\\t\\n\\f\\r\\\"\\\\\\/";





static const JSUINT8 g_asciiOutputTable[256] = 
{
 0, 30, 30, 30, 30, 30, 30, 30, 10, 12, 14, 30, 16, 18, 30, 30, 
 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
 1, 1, 20, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 22, 1, 1, 1,
 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 
 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 1, 1
};


static void SetError (JSOBJ obj, JSONObjectEncoder *enc, const char *message)
{
	enc->errorMsg = message;
	enc->errorObj = obj;
}


void Buffer_Realloc (JSONObjectEncoder *enc, size_t cbNeeded)
{
	size_t curSize = enc->end - enc->start;
	size_t newSize = curSize * 2;
	size_t offset = enc->offset - enc->start;

	while (newSize < curSize + cbNeeded)
	{
		newSize *= 2;
	}

	if (enc->heap)
	{
		enc->start = (char *) enc->realloc (enc->start, newSize);
	}
	else
	{
		char *oldStart = enc->start;
		enc->heap = 1;
		enc->start = (char *) enc->malloc (newSize);
		memcpy (enc->start, oldStart, offset);
	}
	enc->offset = enc->start + offset;
	enc->end = enc->start + newSize;
}

FASTCALL_ATTR INLINE_PREFIX void FASTCALL_MSVC Buffer_AppendShortHexUnchecked (char *outputOffset, unsigned short value)
{
	*(outputOffset++) = g_hexChars[(value & 0xf000) >> 12];
	*(outputOffset++) = g_hexChars[(value & 0x0f00) >> 8];
	*(outputOffset++) = g_hexChars[(value & 0x00f0) >> 4];
	*(outputOffset++) = g_hexChars[(value & 0x000f) >> 0];
}

int Buffer_EscapeStringUnvalidated (JSOBJ obj, JSONObjectEncoder *enc, const char *io, const char *end)
{
	char *of = (char *) enc->offset;

	while (1)
	{
		switch (*io)
		{
		case 0x00:
			enc->offset += (of - enc->offset); 
			return TRUE;

		case '\"': (*of++) = '\\'; (*of++) = '\"'; break;
		case '\\': (*of++) = '\\'; (*of++) = '\\'; break;
		
		case '\b': (*of++) = '\\'; (*of++) = 'b'; break;
		case '\f': (*of++) = '\\'; (*of++) = 'f'; break;
		case '\n': (*of++) = '\\'; (*of++) = 'n'; break;
		case '\r': (*of++) = '\\'; (*of++) = 'r'; break;
		case '\t': (*of++) = '\\'; (*of++) = 't'; break;

		case 0x01:
		case 0x02:
		case 0x03:
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
		case 0x0b:
		case 0x0e:
		case 0x0f:
		case 0x10:
		case 0x11:
		case 0x12:
		case 0x13:
		case 0x14:
		case 0x15:
		case 0x16:
		case 0x17:
		case 0x18:
		case 0x19:
		case 0x1a:
		case 0x1b:
		case 0x1c:
		case 0x1d:
		case 0x1e:
		case 0x1f:
			*(of++) = '\\';
			*(of++) = 'u';
			*(of++) = '0';
			*(of++) = '0';
			*(of++) = g_hexChars[ (unsigned char) (((*io) & 0xf0) >> 4)];
			*(of++) = g_hexChars[ (unsigned char) ((*io) & 0x0f)];
			break;

		default: (*of++) = (*io); break;
		}

		*io++;
	}

	return FALSE;
}



int Buffer_EscapeStringValidated (JSOBJ obj, JSONObjectEncoder *enc, const char *io, const char *end)
{
	JSUTF32 ucs;
	char *of = (char *) enc->offset;

	while (1)
	{

		
		JSUINT8 utflen = g_asciiOutputTable[(unsigned char) *io];

		switch (utflen)
		{
			case 0: 
			{
				enc->offset += (of - enc->offset); 
				return TRUE;
			}

			case 1:
			{
				*(of++)= (*io++); 
				continue;
			}

			case 2:
			{
				JSUTF32 in;

				if (io + 1 > end)
				{
					enc->offset += (of - enc->offset);
					SetError (obj, enc, "Unterminated UTF-8 sequence when encoding string");
					return FALSE;
				}

				in = *((JSUTF16 *) io);

#ifdef __LITTLE_ENDIAN__
				ucs = ((in & 0x1f) << 6) | ((in >> 8) & 0x3f);
#else
				ucs = ((in & 0x1f00) >> 2) | (in & 0x3f);
#endif

				if (ucs < 0x80)
				{
					enc->offset += (of - enc->offset);
					SetError (obj, enc, "Overlong 2 byte UTF-8 sequence detected when encoding string");
					return FALSE;
				}

				io += 2;
				break;
			}

			case 3:
			{
				JSUTF32 in;

				if (io + 2 > end)
				{
					enc->offset += (of - enc->offset);
					SetError (obj, enc, "Unterminated UTF-8 sequence when encoding string");
					return FALSE;
				}

#ifdef __LITTLE_ENDIAN__
				in = *((JSUTF16 *) io);
				in |= *((JSUINT8 *) io + 2) << 16;
				ucs = ((in & 0x0f) << 12) | ((in & 0x3f00) >> 2) | ((in & 0x3f0000) >> 16);
#else
				in = *((JSUTF16 *) io) << 8;
				in |= *((JSUINT8 *) io + 2);
				ucs = ((in & 0x0f0000) >> 4) | ((in & 0x3f00) >> 2) | (in & 0x3f);
#endif


				if (ucs < 0x800)
				{
					enc->offset += (of - enc->offset);
					SetError (obj, enc, "Overlong 3 byte UTF-8 sequence detected when encoding string");
					return FALSE;
				}

				io += 3;
				break;
			}
			case 4:
			{
				JSUTF32 in;
				
				if (io + 3 > end)
				{
					enc->offset += (of - enc->offset);
					SetError (obj, enc, "Unterminated UTF-8 sequence when encoding string");
					return FALSE;
				}

#ifdef __LITTLE_ENDIAN__
				in = *((JSUTF32 *) io);
				ucs = ((in & 0x07) << 18) | ((in & 0x3f00) << 4) | ((in & 0x3f0000) >> 10) | ((in & 0x3f000000) >> 24);
#else
				in = *((JSUTF32 *) io);
				ucs = ((in & 0x07000000) >> 6) | ((in & 0x3f0000) >> 4) | ((in & 0x3f00) >> 2) | (in & 0x3f);
#endif
				if (ucs < 0x10000)
				{
					enc->offset += (of - enc->offset);
					SetError (obj, enc, "Overlong 4 byte UTF-8 sequence detected when encoding string");
					return FALSE;
				}

				io += 4;
				break;
			}


			case 5:
			case 6:
				enc->offset += (of - enc->offset);
				SetError (obj, enc, "Unsupported UTF-8 sequence length when encoding string");
				return FALSE;

			case 30:
				
				*(of++) = '\\';
				*(of++) = 'u';
				*(of++) = '0';
				*(of++) = '0';
				*(of++) = g_hexChars[ (unsigned char) (((*io) & 0xf0) >> 4)];
				*(of++) = g_hexChars[ (unsigned char) ((*io) & 0x0f)];
				io ++;
				continue;

			case 10:
			case 12:
			case 14:
			case 16:
			case 18:
			case 20:
			case 22:
			
				*(of++) = *( (char *) (g_escapeChars + utflen + 0));
				*(of++) = *( (char *) (g_escapeChars + utflen + 1));
				io ++;
				continue;
		}

		
		if (ucs >= 0x10000)
		{
			ucs -= 0x10000;
			*(of++) = '\\';
			*(of++) = 'u';
			Buffer_AppendShortHexUnchecked(of, (ucs >> 10) + 0xd800);
			of += 4;

			*(of++) = '\\';
			*(of++) = 'u';
			Buffer_AppendShortHexUnchecked(of, (ucs & 0x3ff) + 0xdc00);
			of += 4;
		}
		else
		{
			*(of++) = '\\';
			*(of++) = 'u';
			Buffer_AppendShortHexUnchecked(of, ucs);
			of += 4;
		}
	}

	return FALSE;
}

#define Buffer_Reserve(__enc, __len) \
	if ((__enc)->offset + (__len) > (__enc)->end)	\
	{	\
		Buffer_Realloc((__enc), (__len));\
	}	\


#define Buffer_AppendCharUnchecked(__enc, __chr) \
				*((__enc)->offset++) = __chr; \

FASTCALL_ATTR INLINE_PREFIX void FASTCALL_MSVC strreverse(char* begin, char* end)
{
	char aux;
	while (end > begin)
	aux = *end, *end-- = *begin, *begin++ = aux;
}

void Buffer_AppendIntUnchecked(JSONObjectEncoder *enc, JSINT32 value)
{
	char* wstr;
	JSUINT32 uvalue = (value < 0) ? -value : value;

	wstr = enc->offset;
	
	
	do *wstr++ = (char)(48 + (uvalue % 10)); while(uvalue /= 10);
	if (value < 0) *wstr++ = '-';

	
	strreverse(enc->offset,wstr - 1);
	enc->offset += (wstr - (enc->offset));
}

void Buffer_AppendLongUnchecked(JSONObjectEncoder *enc, JSINT64 value)
{
	char* wstr;
	JSUINT64 uvalue = (value < 0) ? -value : value;

	wstr = enc->offset;
	
	
	do *wstr++ = (char)(48 + (uvalue % 10ULL)); while(uvalue /= 10ULL);
	if (value < 0) *wstr++ = '-';

	
	strreverse(enc->offset,wstr - 1);
	enc->offset += (wstr - (enc->offset));
}

int Buffer_AppendDoubleUnchecked(JSOBJ obj, JSONObjectEncoder *enc, double value)
{
	
	const double thres_max = (double)(0x7FFFFFFF);
	int count;
	double diff = 0.0;
	char* str = enc->offset;
	char* wstr = str;
	int whole;
	double tmp;
	uint32_t frac;
	int neg;
	double pow10;

	if (value == HUGE_VAL || value == -HUGE_VAL)
	{
		SetError (obj, enc, "Invalid Inf value when encoding double");
		return FALSE;
	}
	if (! (value == value)) 
	{
		SetError (obj, enc, "Invalid Nan value when encoding double");
		return FALSE;
	}


	
	neg = 0;
	if (value < 0) 
	{
		neg = 1;
		value = -value;
	}

	pow10 = g_pow10[enc->doublePrecision];

	whole = (int) value;
	tmp = (value - whole) * pow10;
	frac = (uint32_t)(tmp);
	diff = tmp - frac;

	if (diff > 0.5) 
	{
		++frac;
		
		if (frac >= pow10) 
		{
			frac = 0;
			++whole;
		}
	} 
	else 
	if (diff == 0.5 && ((frac == 0) || (frac & 1))) 
	{
		
		++frac;
	}

	
	
	if (value > thres_max) 
	{
		enc->offset += sprintf(str, "%e", neg ? -value : value);
		return TRUE;
	}

	if (enc->doublePrecision == 0) 
	{
		diff = value - whole;

		if (diff > 0.5) 
		{
		
		++whole;
		}
		else 
		if (diff == 0.5 && (whole & 1)) 
		{
			
			
			++whole;
		}

			
	} 
	else 
	if (frac) 
	{ 
		count = enc->doublePrecision;
		
		
		
		while (!(frac % 10))
		{
		--count;
		frac /= 10;
		}
		

		
		do 
		{
			--count;
			*wstr++ = (char)(48 + (frac % 10));
		} while (frac /= 10);
		
		while (count-- > 0)
		{
			*wstr++ = '0';
		}
		
		*wstr++ = '.';
	}
	else
	{
		*wstr++ = '0';
		*wstr++ = '.';
	}

	
	
	
	do *wstr++ = (char)(48 + (whole % 10)); while (whole /= 10);
	
	if (neg) 
	{
		*wstr++ = '-';
	}
	strreverse(str, wstr-1);
	enc->offset += (wstr - (enc->offset));

	return TRUE;
}










void encode(JSOBJ obj, JSONObjectEncoder *enc, const char *name, size_t cbName)
{
	JSONTypeContext tc;
	size_t szlen;

	if (enc->level > enc->recursionMax)
	{
		SetError (obj, enc, "Maximum recursion level reached");
		return;
	}

	

	Buffer_Reserve(enc, 256 + (((cbName / 4) + 1) * 12));

	if (name)
	{
		Buffer_AppendCharUnchecked(enc, '\"');

		if (enc->forceASCII)
		{
			if (!Buffer_EscapeStringValidated(obj, enc, name, name + cbName))
			{
				return;
			}
		}
		else
		{
			if (!Buffer_EscapeStringUnvalidated(obj, enc, name, name + cbName))
			{
				return;
			}
		}


		Buffer_AppendCharUnchecked(enc, '\"');

		Buffer_AppendCharUnchecked (enc, ':');
#ifndef JSON_NO_EXTRA_WHITESPACE
		Buffer_AppendCharUnchecked (enc, ' ');
#endif
	}

	enc->beginTypeContext(obj, &tc);

	switch (tc.type)
	{
		case JT_INVALID:
			return;

		case JT_ARRAY:
		{
			int count = 0;
			JSOBJ iterObj;
			enc->iterBegin(obj, &tc);

			Buffer_AppendCharUnchecked (enc, '[');

			while (enc->iterNext(obj, &tc))
			{
				if (count > 0)
				{
					Buffer_AppendCharUnchecked (enc, ',');
#ifndef JSON_NO_EXTRA_WHITESPACE
					Buffer_AppendCharUnchecked (buffer, ' ');
#endif
				}

				iterObj = enc->iterGetValue(obj, &tc);

				enc->level ++;
				encode (iterObj, enc, NULL, 0);			
				count ++;
			}

			enc->iterEnd(obj, &tc);
			Buffer_AppendCharUnchecked (enc, ']');
			break;
		}

		case JT_OBJECT:
		{
			int count = 0;
			JSOBJ iterObj;
			char *objName;

			enc->iterBegin(obj, &tc);

			Buffer_AppendCharUnchecked (enc, '{');

			while (enc->iterNext(obj, &tc))
			{
				if (count > 0)
				{
					Buffer_AppendCharUnchecked (enc, ',');
#ifndef JSON_NO_EXTRA_WHITESPACE
					Buffer_AppendCharUnchecked (enc, ' ');
#endif
				}

				iterObj = enc->iterGetValue(obj, &tc);
				objName = enc->iterGetName(obj, &tc, &szlen);

				enc->level ++;
				encode (iterObj, enc, objName, szlen);			
				count ++;
			}

			enc->iterEnd(obj, &tc);
			Buffer_AppendCharUnchecked (enc, '}');
			break;
		}

		case JT_LONG:
		{
			Buffer_AppendLongUnchecked (enc, enc->getLongValue(obj, &tc));
			break;
		}

		case JT_INT:
		{
			Buffer_AppendIntUnchecked (enc, enc->getIntValue(obj, &tc));
			break;
		}

		case JT_TRUE:
		{
			Buffer_AppendCharUnchecked (enc, 't');
			Buffer_AppendCharUnchecked (enc, 'r');
			Buffer_AppendCharUnchecked (enc, 'u');
			Buffer_AppendCharUnchecked (enc, 'e');
			break;
		}

		case JT_FALSE:
		{
			Buffer_AppendCharUnchecked (enc, 'f');
			Buffer_AppendCharUnchecked (enc, 'a');
			Buffer_AppendCharUnchecked (enc, 'l');
			Buffer_AppendCharUnchecked (enc, 's');
			Buffer_AppendCharUnchecked (enc, 'e');
			break;
		}


		case JT_NULL: 
		{
			Buffer_AppendCharUnchecked (enc, 'n');
			Buffer_AppendCharUnchecked (enc, 'u');
			Buffer_AppendCharUnchecked (enc, 'l');
			Buffer_AppendCharUnchecked (enc, 'l');
			break;
		}

		case JT_DOUBLE:
		{
			if (!Buffer_AppendDoubleUnchecked (obj, enc, enc->getDoubleValue(obj, &tc)))
			{
				enc->endTypeContext(obj, &tc);
				enc->level --;
				return;
			}
			break;
		}

		case JT_UTF8:
		{
			const char *value = enc->getStringValue(obj, &tc, &szlen);
			Buffer_Reserve(enc, ((szlen / 4) + 1) * 12);
			Buffer_AppendCharUnchecked (enc, '\"');


			if (enc->forceASCII)
			{
				if (!Buffer_EscapeStringValidated(obj, enc, value, value + szlen))
				{
					enc->endTypeContext(obj, &tc);
					enc->level --;
					return;
				}
			}
			else
			{
				if (!Buffer_EscapeStringUnvalidated(obj, enc, value, value + szlen))
				{
					enc->endTypeContext(obj, &tc);
					enc->level --;
					return;
				}
			}

			Buffer_AppendCharUnchecked (enc, '\"');
			break;
		}
	}

	enc->endTypeContext(obj, &tc);
	enc->level --;

}

char *JSON_EncodeObject(JSOBJ obj, JSONObjectEncoder *enc, char *_buffer, size_t _cbBuffer)
{
	enc->malloc = enc->malloc ? enc->malloc : malloc;
	enc->free =  enc->free ? enc->free : free;
	enc->realloc = enc->realloc ? enc->realloc : realloc;
	enc->errorMsg = NULL;
	enc->errorObj = NULL;
	enc->level = 0;

	if (enc->recursionMax < 1)
	{
		enc->recursionMax = JSON_MAX_RECURSION_DEPTH;
	}

	if (enc->doublePrecision < 0 ||
			enc->doublePrecision > JSON_DOUBLE_MAX_DECIMALS)
	{
		enc->doublePrecision = JSON_DOUBLE_MAX_DECIMALS;
	}

	if (_buffer == NULL)
	{
		_cbBuffer = 32768;
		enc->start = (char *) enc->malloc (_cbBuffer);
		enc->heap = 1;
	}
	else
	{
		enc->start = _buffer;
		enc->heap = 0;
	}

	enc->end = enc->start + _cbBuffer;
	enc->offset = enc->start;


	encode (obj, enc, NULL, 0);
	
	Buffer_Reserve(enc, 1);
	Buffer_AppendCharUnchecked(enc, '\0');

	return enc->start;
}