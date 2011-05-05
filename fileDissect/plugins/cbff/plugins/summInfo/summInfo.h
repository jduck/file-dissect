/*
 * Windows Compound Binary File Format implementation
 * Joshua J. Drake <jdrake idefense.com>
 * 
 * summinfo.h:
 * class declaration for summinfo cbffStreamPlugin class
 */
#ifndef __summInfo_h_
#define __summInfo_h_

#include "cbffStreamPlugin.h"
#include "cbff_defs.h"


class summInfo : public cbffStreamPlugin
{
public:
	summInfo(wxLog *plog, fileDissectTreeCtrl *tree);

	// plugin interface methods
	void MarkDesiredStreams(void);
	void Dissect(void);
	// we don't store anything extra, this isn't needed (use default)
	// void CloseFile(void);

private:
	void DissectStream(cbffStream *pStream);

	wxChar *HumanReadablePropId(ULONG id);
	wxChar *HumanReadablePropType(ULONG type);
};


struct SummaryInformationHeader
{
	USHORT uByteOrder;
	USHORT uReserved;
	USHORT uOSVersion;
	USHORT uPlatform;
	CLSID clsid;
	ULONG ulSectionCount;
};

struct SummaryInformationSectionDeclaration
{
	CLSID clsid;
	ULONG ulOffset;
};

struct SummaryInformationSectionHeader
{
	ULONG ulLength;
	ULONG ulPropertyCount;
};

struct SummaryInformationPropertyDeclaration
{
	ULONG ulPropertyId;
	ULONG ulOffset;
};

struct SummaryInformationProperty
{
	ULONG ulType;
	union
	{
		ULONG ulDword1;
		USHORT uWord1;
		SHORT sWord1;
		LONG lDword1;
	} u;
};


#define SVT_SHORT		0x02
#define SVT_LONG		0x03
#define SVT_ULONG		0x13
#define SVT_STRING		0x1e
#define SVT_FILETIME	0x40
#define SVT_CLIPBOARD	0x47


#define SPT_CODEPAGE		0x00000001
#define SPT_TITLE			0x00000002
#define SPT_SUBJECT			0x00000003
#define SPT_AUTHOR			0x00000004
#define SPT_KEYWORDS		0x00000005
#define SPT_COMMENTS		0x00000006
#define SPT_TEMPLATE		0x00000007
#define SPT_LASTAUTHOR		0x00000008
#define SPT_REVNUMBER		0x00000009
#define SPT_EDITTIME		0x0000000A
#define SPT_LASTPRINTED		0x0000000B
#define SPT_CREATE_DTM		0x0000000C
#define SPT_LASTSAVE_DTM	0x0000000D
#define SPT_PAGECOUNT		0x0000000E
#define SPT_WORDCOUNT		0x0000000F
#define SPT_CHARCOUNT		0x00000010
#define SPT_THUMBNAIL		0x00000011
#define SPT_APPNAME			0x00000012
#define SPT_SECURITY		0x00000013
#define SPT_LOCALEID		0x80000000

#endif
