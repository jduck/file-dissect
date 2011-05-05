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
	wxFileMap (void);
	~wxFileMap (void);

	bool Open(const wxChar *filename, OpenMode mode = read,
            int perms = wxS_DEFAULT);
	void Close(void);

	ssize_t Read(void *pBuf, size_t nCount);
	wxFileOffset Seek(wxFileOffset ofs, wxSeekMode mode = wxFromStart);

	wxByte *GetAddress(void);
	wxFileOffset Length(void);

	const wxChar *m_filename;

protected:
#ifdef __WXMSW__
	HANDLE m_hMap;
#endif
	wxByte *m_ptr;
	wxFileOffset m_len;
	wxFileOffset m_offset;
};

#endif
