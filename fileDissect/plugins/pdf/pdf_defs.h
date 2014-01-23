/*
 * Adobe Portable Document Format implementation
 * Joshua J. Drake <jdrake accuvant.com>
 * 
 * pdf_defs.h:
 * type declarations for portal documents
 */
#ifndef __pdf_defs_h_
#define __pdf_defs_h_

// types used in struct declarations
#ifdef __WXWINDOWS__
// yay for wxWidgets
typedef wxByte BYTE;
typedef wxUint16 USHORT;
typedef wxInt16 SHORT;
//typedef wxUint32 ULONG;
//typedef wxInt32 LONG;
#else
// NOTE: these might not be correct (particularly on 64-bit platforms)
typedef unsigned char BYTE;
typedef unsigned short USHORT;
typedef unsigned long ULONG;
typedef short SHORT;
typedef int LONG;
#endif


typedef USHORT WORD;
typedef ULONG DWORD;


#define PDF_TRAILER_MIN_SIZE 18 // startxref\nN\n%%EOF\n

#define PDF_WHITESPACE_CHARS	"\x00\x09\x0a\x0c\x0d\x20"
#define PDF_WHITESPACE_CHARSLEN 6

#define PDF_DELIMITERS_CHARS	"()<>[]{}/%"
#define PDF_DELIMITERS_CHARSLEN 10

#define PDF_NAME_DELIM_CHARS    (PDF_WHITESPACE_CHARS PDF_DELIMITERS_CHARS)
#define PDF_NAME_DELIM_CHARSLEN (PDF_WHITESPACE_CHARSLEN + PDF_DELIMITERS_CHARSLEN)


// types of pdf objects
enum pdfObjType
{
	PDF_OBJ_BASE = 0,
	PDF_OBJ_NULL,
	PDF_OBJ_BOOLEAN,
	PDF_OBJ_INTEGER,		// 3
	PDF_OBJ_REAL,
	PDF_OBJ_NAME,
	PDF_OBJ_REFERENCE,
	PDF_OBJ_HEXSTRING,		// 7
	PDF_OBJ_LITERAL,
	PDF_OBJ_ARRAY,
	PDF_OBJ_DICTIONARY,
	PDF_OBJ_INDIRECT,
	PDF_OBJ_STREAM
};


#pragma pack(push, 1)

// ...

#pragma pack(pop)

#endif
