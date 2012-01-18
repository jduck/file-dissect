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


wxByte *wxFileMap::GetBaseAddress()
{
	return m_ptr;
}

wxByte *wxFileMap::GetAddress()
{
	return m_ptr + m_offset;
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

	if (mode == wxFromStart)
	{
		if (ofs < 0 || ofs > m_len)
			return wxInvalidOffset;
		m_offset = ofs;
	}
	else if (mode == wxFromEnd)
	{
		if (ofs > 0 || (ofs < (-1 * m_len)))
			return wxInvalidOffset;
		m_offset = m_len + ofs;
	}
	else
		// not implementing other seek modes
		return wxInvalidOffset;

	return ofs;
}


wxByte *wxFileMap::FindString(const char *str)
{
	wxByte *p = m_ptr + m_offset;
	wxByte *end = m_ptr + m_len;
	size_t len = strlen(str);

	while (1)
	{
		// find next occurrence of first char
		while (p < end && *p != *str)
			p++;
		// eek!
		if (p >= end)
			break;
		// check if the entire string is there
		if (memcmp(p, str, len) == 0)
			return p;
		// skip current occurrence of byte
		p++;
	}

	// not found :(
	return NULL;
}


wxByte *wxFileMap::FindStringReverse(const char *str)
{
	size_t len = strlen(str);
	wxByte *p = m_ptr + m_offset;
	wxByte *end = m_ptr + m_len;
	do
	{
		while (p > m_ptr && *p != *str)
			p--;

		if (p <= m_ptr)
			return NULL;

		if ((size_t)(end - p) < len) // should be safe, so we cast to get rid of warning
		{
			// not enough
			p--;
			continue;
		}

		if (memcmp(p, str, len) != 0)
		{
			// mismatch
			p--;
			continue;
		}

		// got it
		break;
	} while(1);

	return p;
}
