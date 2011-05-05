/*
 * wxFileMap
 * cross-platform class for creating a file mapping
 * Joshua J. Drake <jdrake idefense.com>
 *
 * wxFileMap.cpp:
 * implementation details
 */
#include "wxFileMap.h"


wxFileMap::wxFileMap (void)
{
	m_ptr = NULL;
	m_offset = 0;
	m_filename = NULL;
}

wxFileMap::~wxFileMap (void)
{
	if (m_ptr)
		Close();
	m_ptr = NULL;
	m_offset = 0;
}

bool wxFileMap::Open(const wxChar *filename, OpenMode mode, int WXUNUSED(perms))
{
	// open the file
	if (!wxFile::Open(filename, mode))
		return false;

	// get the length
	m_len = wxFile::Length();
	int fd = wxFile::fd();

// windows version
#if defined(__WXMSW__)
	HANDLE hFile = (HANDLE)_get_osfhandle(fd);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		wxFile::Close();
		return false;
	}
	DWORD flProtect = PAGE_READONLY;
	if (mode == wxFile::read_write)
		flProtect = PAGE_READWRITE;
	m_hMap = CreateFileMapping(hFile, NULL, flProtect, m_len >> 32, m_len & 0xffffffff, NULL);
	if (m_hMap == NULL)
	{
		wxFile::Close();
		return false;
	}

	DWORD desiredAccess = FILE_MAP_READ;
	if (mode == wxFile::read_write)
		desiredAccess = FILE_MAP_ALL_ACCESS;
	m_ptr = (wxByte *)MapViewOfFile(m_hMap, desiredAccess, 0, 0, 0);
	if (!m_ptr)
	{
		CloseHandle(m_hMap);
		wxFile::Close();
		return false;
	}

// *nix version
#elif (defined(__UNIX__) || defined(__GNUWIN32__))
	int prot = PROT_READ;
	if (mode == wxFile::read_write)
		prot |= PROT_WRITE;
	m_ptr = (wxByte *)mmap(NULL, m_len, prot, MAP_PRIVATE, fd, 0);
	if (m_ptr == MAP_FAILED)
	{
		m_ptr = NULL;
		wxFile::Close();
		return false;
	}
#endif

	m_filename = filename;
	return true;
}

void wxFileMap::Close(void)
{
#ifdef __WXMSW__
	if (m_ptr)
	{
		UnmapViewOfFile(m_ptr);
		CloseHandle(m_hMap);
	}
#elif (defined(__UNIX__) || defined(__GNUWIN32__))
	if (m_ptr && munmap(m_ptr, m_len) == -1)
		/* need error report */;
#endif

	wxFile::Close();
	m_ptr = NULL;
	m_len = m_offset = 0;
}


wxByte *wxFileMap::GetAddress()
{
	return m_ptr;
}

wxFileOffset wxFileMap::Length(void)
{
	return m_len;
}

ssize_t wxFileMap::Read(void *pBuf, size_t nCount)
{
	// must have map
	if (!m_ptr)
		return wxInvalidOffset;

	// offset must be in bounds
	if (m_offset < 0 || m_offset > m_len)
		return wxInvalidOffset;

	// convert to space after the offset only
	wxFileOffset left = m_len - m_offset;

	// must have enough data to cover the entire read
	if ((wxFileOffset)nCount > left)
		return wxInvalidOffset;

	memcpy(pBuf, m_ptr + m_offset, nCount);
	return nCount;
}

wxFileOffset wxFileMap::Seek(wxFileOffset ofs, wxSeekMode mode)
{
	// must have map
	if (!m_ptr)
		return wxInvalidOffset;

	// not implementing other seek modes
	if (mode != wxFromStart)
		return wxInvalidOffset;

	if (ofs < 0 || ofs > m_len)
		return wxInvalidOffset;

	m_offset = ofs;

	return ofs;
}
