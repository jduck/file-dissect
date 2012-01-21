/*
 * wxFileMap
 * cross-platform class for creating a file mapping
 * Joshua J. Drake <jdrake idefense.com>
 *
 * wxFileMap.h:
 * class declaration
 */
#ifndef __wxFileMap_h_
#define __wxFileMap_h_

#ifdef __linux__
# define __declspec(x)
#endif


// we derive from wxFile

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>
#ifdef __BORLANDC__
# pragma hdrstop
#endif
#ifndef WX_PRECOMP
# include <wx/wx.h>
#endif
#include <wx/file.h>

// for mapping code
#ifdef __WXMSW__
# include <windows.h>
#elif (defined(__UNIX__) || defined(__GNUWIN32__))
# include <sys/mman.h>
#endif

class wxFileMap : private wxFile
{
public:
	__declspec(dllexport) wxFileMap (void);
	__declspec(dllexport) ~wxFileMap (void);

	__declspec(dllexport) bool Open(const wxChar *filename, OpenMode mode = read, int perms = wxS_DEFAULT);
	__declspec(dllexport) void Close(void);

	__declspec(dllexport) ssize_t Read(void *pBuf, size_t nCount);
	__declspec(dllexport) wxFileOffset Seek(wxFileOffset ofs, wxSeekMode mode = wxFromStart);
	__declspec(dllexport) wxByte *FindString(const char *str);
	__declspec(dllexport) wxByte *FindStringReverse(const char *str);

	__declspec(dllexport) wxByte *GetBaseAddress(void);
	__declspec(dllexport) wxByte *GetAddress(void);
	__declspec(dllexport) wxFileOffset Length(void);

	wxString m_filename;

protected:
#ifdef __WXMSW__
	HANDLE m_hMap;
#endif
	wxByte *m_ptr;
	wxFileOffset m_len;
	wxFileOffset m_offset;
};

#endif
