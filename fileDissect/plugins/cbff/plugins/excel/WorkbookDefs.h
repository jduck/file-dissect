/*
 * Microsoft Office Excel Binary File Format implementation
 * Joshua J. Drake <jdrake idefense.com>
 * 
 * WorkbookDefs.h:
 * data structure definitions requierd by Workbook cbffStreamPlugin class
 */

#ifndef __WorkbookDefs_h_
#define __WorkbookDefs_h_

#pragma pack(push, 1)
struct WorkbookRecord
{
	USHORT uNumber;
	USHORT uLength;
};

// manully added record types
#define WBOOK_RT_UFONT					0x31

// BOF record types (including version)
#define WBOOK_RT_BOF2					0x09
#define WBOOK_RT_BOF3					0x209
#define WBOOK_RT_BOF4					0x409
#define WBOOK_RT_BOF578					0x809

// misc missing record numbers
#define WBOOK_RT_RK3					0x27E

// auto-generated record types
#define WBOOK_RT_EOF 	0x0A
#define WBOOK_RT_CALCCOUNT 	0x0C
#define WBOOK_RT_CALCMODE 	0x0D
#define WBOOK_RT_PRECISION 	0x0E
#define WBOOK_RT_REFMODE 	0x0F
#define WBOOK_RT_DELTA 	0x10
#define WBOOK_RT_ITERATION 	0x11
#define WBOOK_RT_PROTECT 	0x12
#define WBOOK_RT_PASSWORD 	0x13
#define WBOOK_RT_HEADER 	0x14
#define WBOOK_RT_FOOTER 	0x15
#define WBOOK_RT_EXTERNCOUNT 	0x16
#define WBOOK_RT_EXTERNSHEET 	0x17
#define WBOOK_RT_WINDOWPROTECT 	0x19
#define WBOOK_RT_VERTICALPAGEBREAKS 	0x1A
#define WBOOK_RT_HORIZONTALPAGEBREAKS 	0x1B
#define WBOOK_RT_NOTE 	0x1C
#define WBOOK_RT_SELECTION 	0x1D
#define WBOOK_RT_1904 	0x22
#define WBOOK_RT_LEFTMARGIN 	0x26
#define WBOOK_RT_RIGHTMARGIN 	0x27
#define WBOOK_RT_TOPMARGIN 	0x28
#define WBOOK_RT_BOTTOMMARGIN 	0x29
#define WBOOK_RT_PRINTHEADERS 	0x2A
#define WBOOK_RT_PRINTGRIDLINES 	0x2B
#define WBOOK_RT_FILEPASS 	0x2F
#define WBOOK_RT_CONTINUE 	0x3C
#define WBOOK_RT_WINDOW1 	0x3D
#define WBOOK_RT_BACKUP 	0x40
#define WBOOK_RT_PANE 	0x41
// dupes!
#define WBOOK_RT_CODENAME 	0x42
#define WBOOK_RT_CODEPAGE 	0x42
#define WBOOK_RT_PLS 	0x4D
#define WBOOK_RT_DCON 	0x50
#define WBOOK_RT_DCONREF 	0x51
#define WBOOK_RT_DCONNAME 	0x52
#define WBOOK_RT_DEFCOLWIDTH 	0x55
#define WBOOK_RT_XCT 	0x59
#define WBOOK_RT_CRN 	0x5A
#define WBOOK_RT_FILESHARING 	0x5B
#define WBOOK_RT_WRITEACCESS 	0x5C
#define WBOOK_RT_OBJ 	0x5D
#define WBOOK_RT_UNCALCED 	0x5E
#define WBOOK_RT_SAVERECALC 	0x5F
#define WBOOK_RT_TEMPLATE 	0x60
#define WBOOK_RT_OBJPROTECT 	0x63
#define WBOOK_RT_COLINFO 	0x7D
#define WBOOK_RT_RK 	0x7E
#define WBOOK_RT_IMDATA 	0x7F
#define WBOOK_RT_GUTS 	0x80
#define WBOOK_RT_WSBOOL 	0x81
#define WBOOK_RT_GRIDSET 	0x82
#define WBOOK_RT_HCENTER 	0x83
#define WBOOK_RT_VCENTER 	0x84
#define WBOOK_RT_BOUNDSHEET 	0x85
#define WBOOK_RT_WRITEPROT 	0x86
#define WBOOK_RT_ADDIN 	0x87
#define WBOOK_RT_EDG 	0x88
#define WBOOK_RT_PUB 	0x89
#define WBOOK_RT_COUNTRY 	0x8C
#define WBOOK_RT_HIDEOBJ 	0x8D
#define WBOOK_RT_SORT 	0x90
#define WBOOK_RT_SUB 	0x91
#define WBOOK_RT_PALETTE 	0x92
#define WBOOK_RT_LHRECORD 	0x94
#define WBOOK_RT_LHNGRAPH 	0x95
#define WBOOK_RT_SOUND 	0x96
#define WBOOK_RT_LPR 	0x98
#define WBOOK_RT_STANDARDWIDTH 	0x99
#define WBOOK_RT_FILTERMODE 	0x9B
#define WBOOK_RT_FNGROUPCOUNT 	0x9C
#define WBOOK_RT_AUTOFILTERINFO 	0x9D
#define WBOOK_RT_AUTOFILTER 	0x9E
#define WBOOK_RT_SCL 	0xA0
#define WBOOK_RT_SETUP 	0xA1
#define WBOOK_RT_COORDLIST 	0xA9
#define WBOOK_RT_GCW 	0xAB
#define WBOOK_RT_SCENMAN 	0xAE
#define WBOOK_RT_SCENARIO 	0xAF
#define WBOOK_RT_SXVIEW 	0xB0
#define WBOOK_RT_SXVD 	0xB1
#define WBOOK_RT_SXVI 	0xB2
#define WBOOK_RT_SXIVD 	0xB4
#define WBOOK_RT_SXLI 	0xB5
#define WBOOK_RT_SXPI 	0xB6
#define WBOOK_RT_DOCROUTE 	0xB8
#define WBOOK_RT_RECIPNAME 	0xB9
#define WBOOK_RT_SHRFMLA 	0xBC
#define WBOOK_RT_MULRK 	0xBD
#define WBOOK_RT_MULBLANK 	0xBE
#define WBOOK_RT_MMS 	0xC1
#define WBOOK_RT_ADDMENU 	0xC2
#define WBOOK_RT_DELMENU 	0xC3
#define WBOOK_RT_SXDI 	0xC5
#define WBOOK_RT_SXDB 	0xC6
#define WBOOK_RT_SXSTRING 	0xCD
#define WBOOK_RT_SXTBL 	0xD0
#define WBOOK_RT_SXTBRGIITM 	0xD1
#define WBOOK_RT_SXTBPG 	0xD2
#define WBOOK_RT_OBPROJ 	0xD3
#define WBOOK_RT_SXIDSTM 	0xD5
#define WBOOK_RT_RSTRING 	0xD6
#define WBOOK_RT_DBCELL 	0xD7
#define WBOOK_RT_BOOKBOOL 	0xDA
// dupes!
#define WBOOK_RT_PARAMQRY 	0xDC
#define WBOOK_RT_SXEXT 	0xDC
#define WBOOK_RT_SCENPROTECT 	0xDD
#define WBOOK_RT_OLESIZE 	0xDE
#define WBOOK_RT_UDDESC 	0xDF
#define WBOOK_RT_XF 	0xE0
#define WBOOK_RT_INTERFACEHDR 	0xE1
#define WBOOK_RT_INTERFACEEND 	0xE2
#define WBOOK_RT_SXVS 	0xE3
#define WBOOK_RT_MERGECELLS 	0xE5
#define WBOOK_RT_TABIDCONF 	0xEA
#define WBOOK_RT_MSODRAWINGGROUP 	0xEB
#define WBOOK_RT_MSODRAWING 	0xEC
#define WBOOK_RT_MSODRAWINGSELECTION 	0xED
#define WBOOK_RT_SXRULE 	0xF0
#define WBOOK_RT_SXEX 	0xF1
#define WBOOK_RT_SXFILT 	0xF2
#define WBOOK_RT_SXDXF 	0xF4
#define WBOOK_RT_SXITM 	0xF5
#define WBOOK_RT_SXNAME 	0xF6
#define WBOOK_RT_SXSELECT 	0xF7
#define WBOOK_RT_SXPAIR 	0xF8
#define WBOOK_RT_SXFMLA 	0xF9
#define WBOOK_RT_SXFORMAT 	0xFB
#define WBOOK_RT_SST 	0xFC
#define WBOOK_RT_LABELSST 	0xFD
#define WBOOK_RT_EXTSST 	0xFF
#define WBOOK_RT_SXVDEX 	0x100
#define WBOOK_RT_SXFORMULA 	0x103
#define WBOOK_RT_SXDBEX 	0x122
#define WBOOK_RT_TABID 	0x13D
#define WBOOK_RT_USESELFS 	0x160
#define WBOOK_RT_DSF 	0x161
#define WBOOK_RT_XL5MODIFY 	0x162
#define WBOOK_RT_FILESHARING2 	0x1A5
#define WBOOK_RT_USERBVIEW 	0x1A9
#define WBOOK_RT_USERSVIEWBEGIN 	0x1AA
#define WBOOK_RT_USERSVIEWEND 	0x1AB
#define WBOOK_RT_QSI 	0x1AD
#define WBOOK_RT_SUPBOOK 	0x1AE
#define WBOOK_RT_PROT4REV 	0x1AF
#define WBOOK_RT_CONDFMT 	0x1B0
#define WBOOK_RT_CF 	0x1B1
#define WBOOK_RT_DVAL 	0x1B2
#define WBOOK_RT_DCONBIN 	0x1B5
#define WBOOK_RT_TXO 	0x1B6
#define WBOOK_RT_REFRESHALL 	0x1B7
#define WBOOK_RT_HLINK 	0x1B8
#define WBOOK_RT_SXFDBTYPE 	0x1BB
#define WBOOK_RT_PROT4REVPASS 	0x1BC
#define WBOOK_RT_DV 	0x1BE
#define WBOOK_RT_EXCEL9FILE 	0x1C0
#define WBOOK_RT_RECALCID 	0x1C1
#define WBOOK_RT_DIMENSIONS 	0x200
#define WBOOK_RT_BLANK 	0x201
#define WBOOK_RT_NUMBER 	0x203
#define WBOOK_RT_LABEL 	0x204
#define WBOOK_RT_BOOLERR 	0x205
#define WBOOK_RT_STRING 	0x207
#define WBOOK_RT_ROW 	0x208
#define WBOOK_RT_INDEX 	0x20B
#define WBOOK_RT_NAME 	0x218
#define WBOOK_RT_ARRAY 	0x221
#define WBOOK_RT_EXTERNNAME 	0x223
#define WBOOK_RT_DEFAULTROWHEIGHT 	0x225
#define WBOOK_RT_FONT 	0x231
#define WBOOK_RT_TABLE 	0x236
#define WBOOK_RT_WINDOW2 	0x23E
#define WBOOK_RT_STYLE 	0x293
#define WBOOK_RT_FORMULA 	0x406
#define WBOOK_RT_FORMAT 	0x41E
#define WBOOK_RT_HLINKTOOLTIP 	0x800
#define WBOOK_RT_WEBPUB 	0x801
#define WBOOK_RT_QSISXTAG 	0x802
#define WBOOK_RT_DBQUERYEXT 	0x803
#define WBOOK_RT_EXTSTRING 	0x804
#define WBOOK_RT_TXTQUERY 	0x805
#define WBOOK_RT_QSIR 	0x806
#define WBOOK_RT_QSIF 	0x807
// #define WBOOK_RT_BOF 	0x809
#define WBOOK_RT_OLEDBCONN 	0x80A
#define WBOOK_RT_WOPT 	0x80B
#define WBOOK_RT_SXVIEWEX 	0x80C
#define WBOOK_RT_SXTH 	0x80D
#define WBOOK_RT_SXPIEX 	0x80E
#define WBOOK_RT_SXVDTEX 	0x80F
#define WBOOK_RT_SXVIEWEX9 	0x810
#define WBOOK_RT_CONTINUEFRT 	0x812
#define WBOOK_RT_REALTIMEDATA 	0x813
#define WBOOK_RT_SHEETEXT 	0x862
#define WBOOK_RT_BOOKEXT 	0x863
#define WBOOK_RT_SXADDL 	0x864
#define WBOOK_RT_CRASHRECERR 	0x865
#define WBOOK_RT_HFPicture 	0x866
#define WBOOK_RT_FEATHEADR 	0x867
#define WBOOK_RT_FEAT 	0x868
#define WBOOK_RT_DATALABEXT 	0x86A
#define WBOOK_RT_DATALABEXTCONTENTS 	0x86B
#define WBOOK_RT_CELLWATCH 	0x86C
#define WBOOK_RT_FEATHEADR11 	0x871
#define WBOOK_RT_FEAT11 	0x872
#define WBOOK_RT_FEATINFO11 	0x873
#define WBOOK_RT_DROPDOWNOBJIDS 	0x874
#define WBOOK_RT_CONTINUEFRT11 	0x875
#define WBOOK_RT_DCONN 	0x876
#define WBOOK_RT_LIST12 	0x877
#define WBOOK_RT_FEAT12 	0x878
#define WBOOK_RT_CONDFMT12 	0x879
#define WBOOK_RT_CF12 	0x87A
#define WBOOK_RT_CFEX 	0x87B
#define WBOOK_RT_XFCRC 	0x87C
#define WBOOK_RT_XFEXT 	0x87D
#define WBOOK_RT_EZFILTER12 	0x87E
#define WBOOK_RT_CONTINUEFRT12 	0x87F
#define WBOOK_RT_SXADDL12 	0x881
#define WBOOK_RT_MDTINFO 	0x884
#define WBOOK_RT_MDXSTR 	0x885
#define WBOOK_RT_MDXTUPLE 	0x886
#define WBOOK_RT_MDXSET 	0x887
#define WBOOK_RT_MDXPROP 	0x888
#define WBOOK_RT_MDXKPI 	0x889
#define WBOOK_RT_MDTB 	0x88A
#define WBOOK_RT_PLV_XLS2007 	0x88B
#define WBOOK_RT_COMPAT12 	0x88C
#define WBOOK_RT_DXF 	0x88D
#define WBOOK_RT_TABLESTYLES 	0x88E
#define WBOOK_RT_TABLESTYLE 	0x88F
#define WBOOK_RT_TABLESTYLEELEMENT 	0x890
#define WBOOK_RT_STYLEEXT 	0x892
#define WBOOK_RT_NAMEPUBLISH 	0x893
#define WBOOK_RT_NAMECMT 	0x894
#define WBOOK_RT_SORTDATA12 	0x895
#define WBOOK_RT_THEME 	0x896
#define WBOOK_RT_GUIDTYPELIB 	0x897
#define WBOOK_RT_FNGRP12 	0x898
#define WBOOK_RT_NAMEFNGRP12 	0x899
#define WBOOK_RT_MTRSETTINGS 	0x89A
#define WBOOK_RT_COMPRESSPICTURES 	0x89B
#define WBOOK_RT_HEADERFOOTER 	0x89C
#define WBOOK_RT_FORCEFULLCALCULATION 	0x8A3
#define WBOOK_RT_LISTOBJ 	0x8c1
#define WBOOK_RT_LISTFIELD 	0x8c2
#define WBOOK_RT_LISTDV 	0x8c3
#define WBOOK_RT_LISTCONDFMT 	0x8c4
#define WBOOK_RT_LISTCF 	0x8c5
#define WBOOK_RT_FMQRY 	0x8c6
#define WBOOK_RT_FMSQRY 	0x8c7
#define WBOOK_RT_PLV_MAC11 	0x8c8
#define WBOOK_RT_LNEXT 	0x8c9
#define WBOOK_RT_MKREXT 	0x8ca
#define WBOOK_RT_CRTCOOPT 	0x8cb

// auto-generated chart record types
#define WBOOK_RT_CHUNITS 	0x1001
#define WBOOK_RT_CHCHART 	0x1002
#define WBOOK_RT_CHSERIES 	0x1003
#define WBOOK_RT_CHDATAFORMAT 	0x1006
#define WBOOK_RT_CHLINEFORMAT 	0x1007
#define WBOOK_RT_CHMARKERFORMAT 	0x1009
#define WBOOK_RT_CHAREAFORMAT 	0x100A
#define WBOOK_RT_CHPIEFORMAT 	0x100B
#define WBOOK_RT_CHATTACHEDLABEL 	0x100C
#define WBOOK_RT_CHSERIESTEXT 	0x100D
#define WBOOK_RT_CHCHARTFORMAT 	0x1014
#define WBOOK_RT_CHLEGEND 	0x1015
#define WBOOK_RT_CHSERIESLIST 	0x1016
#define WBOOK_RT_CHBAR 	0x1017
#define WBOOK_RT_CHLINE 	0x1018
#define WBOOK_RT_CHPIE 	0x1019
#define WBOOK_RT_CHAREA 	0x101A
#define WBOOK_RT_CHSCATTER 	0x101B
#define WBOOK_RT_CHCHARTLINE 	0x101C
#define WBOOK_RT_CHAXIS 	0x101D
#define WBOOK_RT_CHTICK 	0x101E
#define WBOOK_RT_CHVALUERANGE 	0x101F
#define WBOOK_RT_CHCATSERRANGE 	0x1020
#define WBOOK_RT_CHAXISLINEFORMAT 	0x1021
#define WBOOK_RT_CHCHARTFORMATLINK 	0x1022
#define WBOOK_RT_CHDEFAULTTEXT 	0x1024
#define WBOOK_RT_CHTEXT 	0x1025
#define WBOOK_RT_CHFONTX 	0x1026
#define WBOOK_RT_CHOBJECTLINK 	0x1027
#define WBOOK_RT_CHFRAME 	0x1032
#define WBOOK_RT_CHBEGIN 	0x1033
#define WBOOK_RT_CHEND 	0x1034
#define WBOOK_RT_CHPLOTAREA 	0x1035
#define WBOOK_RT_CH3D 	0x103A
#define WBOOK_RT_CHPICF 	0x103C
#define WBOOK_RT_CHDROPBAR 	0x103D
#define WBOOK_RT_CHRADAR 	0x103E
#define WBOOK_RT_CHSURFACE 	0x103F
#define WBOOK_RT_CHRADARAREA 	0x1040
#define WBOOK_RT_CHAXISPARENT 	0x1041
#define WBOOK_RT_CHLEGENDXN 	0x1043
#define WBOOK_RT_CHSHTPROPS 	0x1044
#define WBOOK_RT_CHSERTOCRT 	0x1045
#define WBOOK_RT_CHAXESUSED 	0x1046
#define WBOOK_RT_CHSBASEREF 	0x1048
#define WBOOK_RT_CHSERPARENT 	0x104A
#define WBOOK_RT_CHSERAUXTREND 	0x104B
#define WBOOK_RT_CHIFMT 	0x104E
#define WBOOK_RT_CHPOS 	0x104F
#define WBOOK_RT_CHALRUNS 	0x1050
#define WBOOK_RT_CHAI 	0x1051
#define WBOOK_RT_CHSERAUXERRBAR 	0x105B
#define WBOOK_RT_CHSERFMT 	0x105D
#define WBOOK_RT_CHFBI 	0x1060
#define WBOOK_RT_CHBOPPOP 	0x1061
#define WBOOK_RT_CHAXCEXT 	0x1062
#define WBOOK_RT_CHDAT 	0x1063
#define WBOOK_RT_CHPLOTGROWTH 	0x1064
#define WBOOK_RT_CHSIINDEX 	0x1065
#define WBOOK_RT_CHGELFRAME 	0x1066
#define WBOOK_RT_CHBOPPOPCUSTOM 	0x1067

// manually defined BOF types
#define WBOOK_BT_WBGLOBALS		0x05
#define WBOOK_BT_VBMODULE		0x06
#define WBOOK_BT_SHEET			0x10
#define WBOOK_BT_CHART			0x20
#define WBOOK_BT_XCL4MACRO		0x40
#define WBOOK_BT_WORKSPACE		0x100

// manually defined RK value types
#define WBOOK_RKT_IEEE			0
#define WBOOK_RKT_IEEEx100		1
#define WBOOK_RKT_INT			2
#define WBOOK_RKT_INTx100		3


struct WorkbookWSZ
{
	USHORT cch;			// string length
	BYTE grbit;			// string options
	// char *rgb;		// string data (variable fields not included)
};

struct WorkbookBOFRecord
{
	USHORT _uvers;		// Version number (0x0006 for BIFF8)
	USHORT _udt;		// Substream type	
	USHORT _urupBuild;	// Build identifier (=0DBBh for Excel 97)
	USHORT _urupYear;	// Build year (=07CCh for Excel 97)
	ULONG _ulbfh;		// File history flags
	ULONG _ulsfo;		// Lowest BIFF version
};

struct WorkbookFORMATRecord
{
	USHORT ifmt;		// internal format index
	USHORT cch;			// length of format string
	BYTE grbit;			// string options
	// char *rgb;		// string data (var fields not included)
};

struct WorkbookWRITEACCESSRecord7
{
	BYTE cch;			// Length of user name
	BYTE stName[31];	// User name (space padded)
};

struct WorkbookWRITEACCESSRecord8
{
	USHORT cch;			// length of string
	BYTE grbit;			// string options
	BYTE stName[109];	// user name (space padded)
};

struct WorkbookBOUNDSHEETRecord
{
	ULONG lbPlyPos;		// Stream position for BOF
	USHORT grbit;		// sheet options
	BYTE cch;			// length of string
	BYTE rg_grbit;		// (custom adjustment) string options byte
	// char *rgch; (var fields not included)
};

struct WorkbookFONTRecord
{
	USHORT dyHeight;	// Height of the font
	USHORT grbit;		// Font attributes
	USHORT icv;			// Index to the color palette
	USHORT bls;			// Bold style
	USHORT sss;			// Subscript/Superscript
	BYTE uls;			// Underline style
	BYTE bFamily;		// Font family
	BYTE bCharSet;		// Character set
	BYTE _reserved;		// Reserved
	BYTE cch;			// Length of the font name
	BYTE rg_grbit;		// (custom adjustment) string options byte
	// char *rgch;		// string data (var fields not included)
};

struct WorkbookLABELSSTRecord
{
	USHORT rw;			// Row (0-based)
	USHORT col;			// Column (0-based)
	USHORT ixfe;		// Index to XF record
	ULONG isst;			// Index into the SST record where the string is stored
};

struct WorkbookSSTRecord
{
	ULONG cstTotal;		// Total strings in SST and EXTSST
	ULONG cstUnique;	// Unique strings in SST
	// char *rgb;		// array of strings (var fields not included)
};

struct WorkbookEXTSSTRecord
{
	USHORT Dsst;		// Number of strings in each bucket
	// struct WorkbookEXTSST_ISSTINF Rgisstinf[];
						// array of ISSTINF structures (var fields not included)
};

struct WorkbookEXTSST_ISSTINF
{
	ULONG ib;			// Stream position of string
	USHORT cb;			// Offset into SST where bucket begins
	USHORT _reserved;	// Reserved "must be zero"
};

struct WorkbookMULBLANKRecord
{
	USHORT rw;			// Row number (0-based)
	USHORT colFirst;	// Column number (0-based) of the first column of the multiple RK record
	// variable length rgixfe data - Array of indexes to XF records
	USHORT colLast;		// Last column containing the BLANKREC structure
};

struct WorkbookMULRKRecord
{
	USHORT rw;			// Row number (0-based)
	USHORT colFirst;	// Column number (0-based) of the first column of the multiple RK record
	// struct WorkbookRKREC rgrkrec[];
						// Array of 6-byte RKREC structures
	USHORT colLast;		// Last column containing the RKREC structure
};

struct WorkbookRKREC
{
	USHORT ixfe;		// index to XF record
	ULONG RK;			// RK number
};

struct WorkbookRKRecord
{
	USHORT rw;			// Row number
	USHORT col;			// Column number
	struct WorkbookRKREC rk; // RK data
};


#pragma pack(pop)

#endif
