/*
 * Windows Compound Binary File Format implementation
 * Joshua J. Drake <jdrake idefense.com>
 * 
 * cbff.cpp:
 * implementation for cbff class
 */
#include "cbff.h"

#include <wx/listimpl.cpp>
WX_DEFINE_LIST(visitedSectors);


// windows raises exceptions on integer overflow in calloc!
#ifdef __WIN32__
void *my_calloc(size_t nmemb, size_t size);
#else
#define my_calloc calloc
#endif


// TODO: support Double-Indirect Fat

cbff::cbff(wxLog *plog, fileDissectTreeCtrl *tree)
{
	m_description = wxT("Compound Binary File");
	// TODO: build from stream plugins
	m_extensions = wxT("*.xls;*.doc;*.ppt");

	InitFileData();

	m_log = plog;
	wxLog::SetActiveTarget(m_log);
	m_tree = tree;

	// this is a ptr just to allow rescanning
	m_plugins = new cbffStreamPlugins();
	m_plugins->LoadPlugins(plog, tree);
}


void cbff::InitFileData(void)
{
	m_sectorSize = m_miniSectorSize = 0;

	m_FAT = 0;

	m_MiniFAT = 0;
	m_MiniFATOffsets = 0;

	m_DIR = 0;
	m_DIROffsets = 0;
	m_DirRoot = 0;
	m_nDirSects = m_nDirEntries = 0;

	m_hdr_id.Unset();
	m_root_id.Unset();
	m_dir_root_id.Unset();

	m_streams.DeleteContents(true);
}


cbff::~cbff (void)
{
	DestroyFileData();
	if (m_plugins)
		delete m_plugins;
}


void cbff::DestroyFileData(void)
{
	if (m_FAT)
		free(m_FAT);
	if (m_MiniFAT)
		free(m_MiniFAT);
	if (m_MiniFATOffsets)
		free(m_MiniFATOffsets);
	if (m_DIR)
		free(m_DIR);
	if (m_DIROffsets)
		free(m_DIROffsets);

	// deinit all the plugins
	for (cbffStreamPlugins::iterator i = m_plugins->begin();
		i != m_plugins->end();
		i++)
	{
		cbffStreamPlugin *p = (*i)->m_instance;
		p->CloseFile();
	}

	// reset the stream list
	m_streams.clear();
}

void cbff::CloseFile(void)
{
	DestroyFileData();
	InitFileData();
}


bool cbff::SupportsExtension(const wxChar *extension)
{
	// TODO: use information from sub-modules
	if (::wxStrcmp(extension, wxT("xls")) == 0
		|| ::wxStrcmp(extension, wxT("doc")) == 0
		|| ::wxStrcmp(extension, wxT("ppt")) == 0)
		return true;
	return false;
}


//
// see if any of the loaded plugins want the streams we found
//
void cbff::QueryStreamPlugins(void)
{
	for (cbffStreamPlugins::iterator i = m_plugins->begin();
		i != m_plugins->end();
		i++)
	{
		cbffStreamPlugin *pp = (cbffStreamPlugin *)(*i)->m_instance;
		pp->m_streams = &m_streams;
		pp->MarkDesiredStreams();
	}
}


//
// read all streams that plugins tagged for loading
//
void cbff::ReadDesiredStreamData(void)
{
	// read stream data for those that are wanted
	for (cbffStreamList::iterator i = m_streams.begin();
		i != m_streams.end();
		i++)
	{
		cbffStream *p = (cbffStream *)(*i);
		if (p->m_wanted 
			// don't read it twice
			&& !p->m_data)
		{
			ReadStreamData(p);
			// XXX: p->m_data could still be null!
		}
	}
}


//
// invoke all the plugins
//
void cbff::InvokeStreamPlugins(void)
{
	for (cbffStreamPlugins::iterator i = m_plugins->begin();
		i != m_plugins->end();
		i++)
	{
		cbffStreamPlugin *pp = (cbffStreamPlugin *)(*i)->m_instance;

		// pass the file and some UI elements off to the plugin
		wxLogMessage(wxT("Dissecting streams using the \"%s\" plug-in."), pp->m_description);
		pp->Dissect();
	}
}


void cbff::Dissect(void)
{
	if (!m_log || !m_tree || !m_file)
		return;

	// add a base node
	m_root_id = m_tree->AddRoot(wxT("Compound Binary File"));

	// dissect the rest
	while (1)
	{
		if (!DissectHeader())
			break;
		if (!DissectFAT())
			break;
		(void) DissectMiniFAT();
		if (!DissectDirectory())
			break;

		// see if any plugins want stream data
		QueryStreamPlugins();

		// read the data (if any is wanted)
		ReadDesiredStreamData();

		// dissect streams
		InvokeStreamPlugins();

		// we never really intended to loop, just a programming syntax hack!
		break;
	}

	// set the initial tree state
	m_tree->Expand(m_root_id);
	m_tree->Expand(m_dir_root_id);
	m_tree->SelectItem(m_root_id);
}


bool cbff::DissectHeader(void)
{
	// add the header to the tree
	if (!ReadStructuredStorageHeader())
		return false;

	m_hdr_id = m_tree->AppendItem(m_root_id, wxT("Header"), -1, -1,
		new fdTIData(0, sizeof(m_sshdr)));
	wxTreeItemId id;
	wxString str;
	int i;

	// the header - signature
	str = wxT("");
	for (i = 0; i < (int)sizeof(m_sshdr._abSig); i++)
	{
		if (!str.IsEmpty())
			str += wxT(" ");
		str += wxString::Format(wxT("%02x"), m_sshdr._abSig[i]);
	}
	str = wxT("Signature: ") + str;
	id = m_tree->AppendItem(m_hdr_id, str, -1, -1, FDT_NEW(_abSig, m_sshdr));

	// the header - clsid
	id = m_tree->AppendItem(m_hdr_id, wxString::Format(
		wxT("CLSID: %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x"), 
		m_sshdr._clid.Data1,
		m_sshdr._clid.Data2,
		m_sshdr._clid.Data3,
		m_sshdr._clid.Data4[0],
		m_sshdr._clid.Data4[1],
		m_sshdr._clid.Data4[2],
		m_sshdr._clid.Data4[3],
		m_sshdr._clid.Data4[4],
		m_sshdr._clid.Data4[5],
		m_sshdr._clid.Data4[6],
		m_sshdr._clid.Data4[7]),
		-1, -1, FDT_NEW(_clid, m_sshdr));

	// the header - versions
	id = m_tree->AppendItem(m_hdr_id, wxString::Format(wxT("Minor Version: 0x%04x"), m_sshdr._uMinorVersion),
		-1, -1, FDT_NEW(_uMinorVersion, m_sshdr));
	id = m_tree->AppendItem(m_hdr_id, wxString::Format(wxT("Major Version: 0x%04x"), m_sshdr._uDllVersion),
		-1, -1, FDT_NEW(_uDllVersion, m_sshdr));

	// the header - file config options
	id = m_tree->AppendItem(m_hdr_id, wxString::Format(wxT("Byte Order: 0x%04x (%s)"), m_sshdr._uByteOrder, 
			HumanReadableByteOrder()),
		-1, -1, FDT_NEW(_uByteOrder, m_sshdr));
	id = m_tree->AppendItem(m_hdr_id, wxString::Format(wxT("Sector Shift: 0x%04x (%d bytes)"), 
		m_sshdr._uSectorShift, m_sectorSize),
		-1, -1, FDT_NEW(_uSectorShift, m_sshdr));
	id = m_tree->AppendItem(m_hdr_id, wxString::Format(wxT("MiniSector Shift: 0x%04x (%d bytes)"), 
		m_sshdr._uMiniSectorShift, m_miniSectorSize),
		-1, -1, FDT_NEW(_uMiniSectorShift, m_sshdr));

	// the header - reserved stuff
	id = m_tree->AppendItem(m_hdr_id, wxString::Format(wxT("Reserved: 0x%04x"), m_sshdr._usReserved),
		-1, -1, FDT_NEW(_usReserved, m_sshdr));
	id = m_tree->AppendItem(m_hdr_id, wxString::Format(wxT("Reserved: 0x%08x"), m_sshdr._ulReserved1),
		-1, -1, FDT_NEW(_ulReserved1, m_sshdr));
	id = m_tree->AppendItem(m_hdr_id, wxString::Format(wxT("Reserved: 0x%08x"), m_sshdr._ulReserved2),
		-1, -1, FDT_NEW(_ulReserved2, m_sshdr));

	// the header - fat count
	id = m_tree->AppendItem(m_hdr_id, wxString::Format(wxT("FAT Sector Count: 0x%08x"), m_sshdr._csectFat),
		-1, -1, FDT_NEW(_csectFat, m_sshdr));

	// the header - directory sector start
	id = m_tree->AppendItem(m_hdr_id, wxString::Format(wxT("Directory Start Sector: 0x%08x"), m_sshdr._sectDirStart),
		-1, -1, FDT_NEW(_sectDirStart, m_sshdr));

	// the header - more config
	id = m_tree->AppendItem(m_hdr_id, wxString::Format(wxT("Transaction Signature: 0x%08x"), m_sshdr._signature),
		-1, -1, FDT_NEW(_signature, m_sshdr));
	id = m_tree->AppendItem(m_hdr_id, wxString::Format(wxT("Mini-sector Cut-Off: 0x%08x"), m_sshdr._ulMiniSectorCutoff),
		-1, -1, FDT_NEW(_ulMiniSectorCutoff, m_sshdr));

	// the header - minifat stuff
	id = m_tree->AppendItem(m_hdr_id, wxString::Format(wxT("Mini-FAT Sector: 0x%08x"), m_sshdr._sectMiniFatStart),
		-1, -1, FDT_NEW(_sectMiniFatStart, m_sshdr));
	id = m_tree->AppendItem(m_hdr_id, wxString::Format(wxT("Mini-FAT Count: 0x%08x"), m_sshdr._csectMiniFat),
		-1, -1, FDT_NEW(_csectMiniFat, m_sshdr));

	// the header - DIF stuff
	id = m_tree->AppendItem(m_hdr_id, wxString::Format(wxT("DIF Sector: 0x%08x"), m_sshdr._sectDifStart),
		-1, -1, FDT_NEW(_sectDifStart, m_sshdr));
	id = m_tree->AppendItem(m_hdr_id, wxString::Format(wxT("DIF Count: 0x%08x"), m_sshdr._csectDif),
		-1, -1, FDT_NEW(_csectDif, m_sshdr));
	return true;
}


bool cbff::DissectFAT(void)
{
	if (!ReadFAT())
		return false;

	wxTreeItemId id;
	int i;

	wxTreeItemId fat_id = m_tree->AppendItem(m_root_id, wxT("FAT")); // node data?

	// a parent for the FAT sectors
	wxTreeItemId fatsect_id = m_tree->AppendItem(m_hdr_id, wxT("FAT Sectors"), -1, -1,
		FDT_NEW(_sectFat, m_sshdr));
	for (i = 0; i < 109; i++)
		id = m_tree->AppendItem(fatsect_id, wxString::Format(wxT("[%03d]: 0x%08x"), i, m_sshdr._sectFat[i]),
			-1, -1, FDT_NEW(_sectFat[i], m_sshdr));

	// add the FAT !
	FSINDEX fidx;
	for (fidx = 0; fidx < m_sshdr._csectFat; fidx++)
	{
		wxFileOffset off = sizeof(m_sshdr) + (m_sshdr._sectFat[fidx] * m_sectorSize);
		ULONG idx = fidx * (m_sectorSize / sizeof(SECT));

		id = m_tree->AppendItem(fat_id, wxString::Format(wxT("FAT sector # %d"), fidx), -1, -1,
			new fdTIData(off, m_sectorSize));

		ULONG j;
		for (j = 0; j < (m_sectorSize / sizeof(SECT)); j++)
		{
			wxFileOffset sectOff = off + (j * sizeof(SECT));

			wxString hr = HumanReadableSectorType(m_FAT[idx + j]);
			if (hr.IsEmpty())
				m_tree->AppendItem(id, wxString::Format(wxT("[0x%08x]: 0x%08x"), 
						idx + j, m_FAT[idx + j]),
					-1, -1, new fdTIData(sectOff, sizeof(SECT)));
			else
				m_tree->AppendItem(id, wxString::Format(wxT("[0x%08x]: 0x%08x (%s)"), 
						idx + j, m_FAT[idx + j], hr.GetData()),
					-1, -1, new fdTIData(sectOff, sizeof(SECT)));
		}
	}
	return true;
}


bool cbff::DissectMiniFAT(void)
{
	// if there's no minifat, return success
	if (m_sshdr._csectMiniFat < 1)
		return true;
	if (!ReadMiniFAT())
		return false;

	wxTreeItemId id;
	wxTreeItemId mfat_id = m_tree->AppendItem(m_root_id, wxT("MiniFAT")); // node data?

	// add the MiniFAT !
	FSINDEX fidx;
	for (fidx = 0; fidx < m_sshdr._csectMiniFat; fidx++)
	{
		wxFileOffset off = m_MiniFATOffsets[fidx];
		ULONG idx = fidx * (m_sectorSize / sizeof(SECT));

		id = m_tree->AppendItem(mfat_id, wxString::Format(wxT("MiniFAT sector # %d"), fidx),
			-1, -1, new fdTIData(off, m_sectorSize));

		ULONG j;
		for (j = 0; j < (m_sectorSize / sizeof(SECT)); j++)
		{
			wxFileOffset sectOff = off + (j * sizeof(SECT));
			wxString hr = HumanReadableSectorType(m_MiniFAT[idx + j]);

			if (hr.IsEmpty())
				m_tree->AppendItem(id, wxString::Format(wxT("[0x%08x]: 0x%08x"), 
						idx + j, m_MiniFAT[idx + j]),
					-1, -1, new fdTIData(sectOff, sizeof(SECT)));
			else
				m_tree->AppendItem(id, wxString::Format(wxT("[0x%08x]: 0x%08x (%s)"), 
						idx + j, m_MiniFAT[idx + j], hr.GetData()),
					-1, -1, new fdTIData(sectOff, sizeof(SECT)));
		}
	}
	return true;
}


bool cbff::DissectDirectory(void)
{
	if (!ReadDirectory())
		return false;

#if 0
	// XXX: support extended directories
	if (m_DirRoot
		&& m_DirRoot->_sectStart != CBFF_SECT_ENDOFCHAIN)
	{
	}
#endif

	wxTreeItemId dir_id = m_tree->AppendItem(m_root_id, wxT("Directory"));

	// these are used through the rest of the function
	wxTreeItemId id;
	wxString str;
	int i;

	// the directory sectors
	FSINDEX fidx;
	for (fidx = 0; fidx < m_nDirSects; fidx++)
	{
		// point to the current sector
		DIRENT_T *pdir = (DIRENT_T *)((BYTE *)m_DIR + (fidx * m_sectorSize));
		wxFileOffset off = 	m_DIROffsets[fidx];

		// add the parent node
		wxTreeItemId ds_id = m_tree->AppendItem(dir_id, wxString::Format(wxT("Directory Sector # %d"), fidx),
			-1, -1, new fdTIData(off, m_sectorSize));

		// add the 4 entries under it
		for (i = 0; i < 4; i++)
		{
			wxFileOffset deOff = off + (i * sizeof(DIRENT_T));

			// it must have a type to add it
			if (pdir[i]._mse != CBFF_STGTY_INVALID)
			{
				id = m_tree->AppendItem(ds_id, wxString::Format(wxT("Entry %d"), (fidx * 4) + i),
					-1, -1, new fdTIData(deOff, sizeof(DIRENT_T)));

				// add the detail for this directory entry under its name
				ConvertDirEntName(pdir + i, str);
				m_tree->AppendItem(id, wxString::Format(wxT("Name: %s"), str.c_str()),
					-1, -1, new fdTIData(deOff + FDT_OFFSET_OF(_ab, pdir[i]), FDT_SIZE_OF(_ab, pdir[i])));

				m_tree->AppendItem(id, wxString::Format(wxT("Name Length: 0x%04x"), pdir[i]._cb),
					-1, -1, new fdTIData(deOff + FDT_OFFSET_OF(_cb, pdir[i]), FDT_SIZE_OF(_cb, pdir[i])));
				m_tree->AppendItem(id, wxString::Format(wxT("Object Type: 0x%02x (%s)"), pdir[i]._mse, 
						HumanReadableDirObjType(pdir[i]._mse)),
					-1, -1, new fdTIData(deOff + FDT_OFFSET_OF(_mse, pdir[i]), FDT_SIZE_OF(_mse, pdir[i])));
				m_tree->AppendItem(id, wxString::Format(wxT("Flags: 0x%02x (%s)"), pdir[i]._bflags,
						HumanReadableColor(pdir[i]._bflags)),
					-1, -1, new fdTIData(deOff + FDT_OFFSET_OF(_bflags, pdir[i]), FDT_SIZE_OF(_bflags, pdir[i])));
				m_tree->AppendItem(id, wxString::Format(wxT("Left Sibling: 0x%08x"), pdir[i]._sidLeftSib),
					-1, -1, new fdTIData(deOff + FDT_OFFSET_OF(_sidLeftSib, pdir[i]), FDT_SIZE_OF(_sidLeftSib, pdir[i])));
				m_tree->AppendItem(id, wxString::Format(wxT("Right Sibling: 0x%08x"), pdir[i]._sidRightSib),
					-1, -1, new fdTIData(deOff + FDT_OFFSET_OF(_sidRightSib, pdir[i]), FDT_SIZE_OF(_sidRightSib, pdir[i])));
				m_tree->AppendItem(id, wxString::Format(wxT("Child: 0x%08x"), pdir[i]._sidChild),
					-1, -1, new fdTIData(deOff + FDT_OFFSET_OF(_sidChild, pdir[i]), FDT_SIZE_OF(_sidChild, pdir[i])));

				// entry type dependent stuff
				if (fidx == 0 && i == 0) // Root
				{
					m_tree->AppendItem(id, wxString::Format(wxT("MiniStream Start: 0x%08x"), pdir[i]._sectStart),
						-1, -1, new fdTIData(deOff + FDT_OFFSET_OF(_sectStart, pdir[i]), FDT_SIZE_OF(_sectStart, pdir[i])));
					m_tree->AppendItem(id, wxString::Format(wxT("MiniStream Length: 0x%08x"), pdir[i]._ulSize),
						-1, -1, new fdTIData(deOff + FDT_OFFSET_OF(_ulSize, pdir[i]), FDT_SIZE_OF(_ulSize, pdir[i])));

					// only the first root node counts...
					if (!m_dir_root_id.m_pItem)
					{
						m_dir_root_id = m_tree->AppendItem(m_root_id, wxT("Root"));

						// recursively add children (the root should never have siblings)
						if (pdir[i]._sidChild != CBFF_SECT_FREE)
							AddDirectoryNode(m_dir_root_id, pdir[i]._sidChild);
					}
				}
				else if (pdir[i]._mse == CBFF_STGTY_STORAGE)
				{
					// storage has clsid, flags, times, and child
					m_tree->AppendItem(id, wxString::Format(
						wxT("CLSID: %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x"), 
							pdir[i]._clsId.Data1,
							pdir[i]._clsId.Data2,
							pdir[i]._clsId.Data3,
							pdir[i]._clsId.Data4[0],
							pdir[i]._clsId.Data4[1],
							pdir[i]._clsId.Data4[2],
							pdir[i]._clsId.Data4[3],
							pdir[i]._clsId.Data4[4],
							pdir[i]._clsId.Data4[5],
							pdir[i]._clsId.Data4[6],
							pdir[i]._clsId.Data4[7]),
						-1, -1, new fdTIData(deOff + FDT_OFFSET_OF(_clsId, pdir[i]), FDT_SIZE_OF(_clsId, pdir[i])));
					m_tree->AppendItem(id, wxString::Format(wxT("User Flags: 0x%08x"), pdir[i]._dwUserFlags),
						-1, -1, new fdTIData(deOff + FDT_OFFSET_OF(_dwUserFlags, pdir[i]), FDT_SIZE_OF(_dwUserFlags, pdir[i])));
					m_tree->AppendItem(id, wxString::Format(wxT("Creation Time: H:0x%08x L:0x%08x"),
							pdir[i]._time[0].dwHighDateTime,
							pdir[i]._time[0].dwLowDateTime),
						-1, -1, new fdTIData(deOff + FDT_OFFSET_OF(_time[0], pdir[i]), FDT_SIZE_OF(_time[0], pdir[i])));
					m_tree->AppendItem(id, wxString::Format(wxT("Modificaiton Time: H:0x%08x L:0x%08x"),
							pdir[i]._time[1].dwHighDateTime,
							pdir[i]._time[1].dwLowDateTime),
						-1, -1, new fdTIData(deOff + FDT_OFFSET_OF(_time[1], pdir[i]), FDT_SIZE_OF(_time[1], pdir[i])));
				}
				else if (pdir[i]._mse == CBFF_STGTY_STREAM)
				{
					// streams have sector start and length
					m_tree->AppendItem(id, wxString::Format(wxT("Start Sector: 0x%08x"), pdir[i]._sectStart),
						-1, -1, new fdTIData(deOff + FDT_OFFSET_OF(_sectStart, pdir[i]), FDT_SIZE_OF(_sectStart, pdir[i])));
					m_tree->AppendItem(id, wxString::Format(wxT("Stream Length: 0x%08x"), pdir[i]._ulSize),
						-1, -1, new fdTIData(deOff + FDT_OFFSET_OF(_ulSize, pdir[i]), FDT_SIZE_OF(_ulSize, pdir[i])));
				}
				else
					wxLogWarning(wxT("%s: Unknown directory entry type (0x%x)"), wxT("cbff::DissectDirectory()"), pdir[i]._mse);
			}
		}
	}
	return true;
}


void cbff::AddDirectoryNode(wxTreeItemId &parent, ULONG didx)
{
	static int max_recursion = 0;
	DIRENT_T *pdir = NULL;
	if (didx < m_nDirEntries)
		pdir = m_DIR + didx;
	if (!pdir)
	{
		wxLogWarning(wxT("%s: Invalid directory reference (0x%x)"), wxT("cbff::AddDirectoryNode()"), didx);
		return;
	}

	// XXX: TODO: better recursion avoidance fix :)
	if (max_recursion > 0x20)
	{
		wxLogWarning(wxT("%s: Maximum recursion depth reached (0x%x)"), wxT("cbff::AddDirectoryNode()"), max_recursion);
		return;
	}
	max_recursion++;

	// add the detail for this directory entry under its name
	wxString str;
	ConvertDirEntName(pdir, str);
	wxTreeItemId id = m_tree->AppendItem(parent, str);
	switch (pdir->_mse)
	{
		case CBFF_STGTY_STORAGE:
			if (pdir->_sidChild != CBFF_SECT_FREE)
				AddDirectoryNode(id, pdir->_sidChild);

			m_tree->Expand(id);

			if (pdir->_sidLeftSib != CBFF_SECT_FREE)
				AddDirectoryNode(parent, pdir->_sidLeftSib);
			if (pdir->_sidRightSib != CBFF_SECT_FREE)
				AddDirectoryNode(parent, pdir->_sidRightSib);
			break;

		case CBFF_STGTY_STREAM:
			if (pdir->_sidLeftSib != CBFF_SECT_FREE)
				AddDirectoryNode(parent, pdir->_sidLeftSib);
			if (pdir->_sidRightSib != CBFF_SECT_FREE)
				AddDirectoryNode(parent, pdir->_sidRightSib);

			// save stream names for plugin to select from
			{
				cbffStream *pnew = new cbffStream(str);
				pnew->m_id = id;
				pnew->m_start = pdir->_sectStart;
				pnew->m_length = pdir->_ulSize;
				pnew->m_phdr = &m_sshdr;
				pnew->m_sectorSize = m_sectorSize;
				pnew->m_miniSectorSize = m_miniSectorSize;
				m_streams.Append(pnew);
			}
			break;

		default:
			// ugh..
			break;
	}
	max_recursion--;
}


void cbff::AddStreamData(wxTreeItemId &WXUNUSED(id), wxString &WXUNUSED(str), DIRENT_T *pdir)
{
	// dont process streams with no data
	if (pdir->_ulSize <= 0)
		return;

#if 0
	// only continue if there is a handler for this type
	cbf_stream *handler = GetStreamObject(str);
	if (!handler)
		return;

	// try to get the data
	ULONG len;
	BYTE *data = NULL;
	len = pdir->_ulSize;
	data = m_file->GetStreamData(pdir->_sectStart, len);
	if (!data)
	{
		delete handler;
		return;
	}

	// got the data, pass it to the handler
	handler->SetStream(len, data);
	handler->AddToTree(m_tree, id);
	delete handler;
#endif
}


bool cbff::ReadStructuredStorageHeader(void)
{
	ssize_t nr;

	nr = m_file->Read(&m_sshdr, sizeof(m_sshdr));
	if (nr != sizeof(m_sshdr))
	{
		wxLogError(wxT("%s: Read returned %d"), wxT("ReadStructuredStorageHeader"), nr);
		return false;
	}

	/* show header */
	if (memcmp(m_sshdr._abSig, cbff_signature, 8) != 0
		&& memcmp(m_sshdr._abSig, cbff_betaSig, 8) != 0)
	{
		// format the signature string
		wxString strSig;
	        unsigned long i;
		strSig += wxString::Format(wxT("%02x"), m_sshdr._abSig[0]);
		for (i = 1; i < sizeof(m_sshdr._abSig); i++)
			strSig += wxString::Format(wxT(" %02x"), m_sshdr._abSig[i]);

		wxLogError(wxT("%s: Invalid CBFF signature: %s"), wxT("cbff::ReadStructuredStorageHeader()"), strSig.GetData());
		return false;
	}

	/* calculate various sizes */
	m_sectorSize = 1 << m_sshdr._uSectorShift;
	if (m_sectorSize < 1)
	{
		wxLogError(wxT("%s: Invalid sector size: 0x%x"), wxT("cbff::ReadStructuredStorageHeader()"), m_sectorSize);
		return false;
	}

	m_miniSectorSize = 1 << m_sshdr._uMiniSectorShift;
	if (m_miniSectorSize < 1)
	{
		wxLogError(wxT("%s: Invalid mini-sector size: 0x%x"), wxT("cbff::ReadStructuredStorageHeader()"), m_miniSectorSize);
		return false;
	}

	return true;
}


bool cbff::ReadFAT(void)
{
	// TODO: support > 109 FAT sectors (large files)

	/* make sure we don't have more than the max in-header FAT sectors */
	if (m_sshdr._csectFat > 109)
	{
		wxLogWarning(wxT("%s: Unsupported FAT sector count: %d -- Only reading the first 109."), wxT("cbff::ReadFAT()"), m_sshdr._csectFat);
		m_sshdr._csectFat = 109;
	}

	/* allocate memory for the FAT */
	if (!(m_FAT = (SECT *)my_calloc(m_sshdr._csectFat, m_sectorSize)))
	{
		wxLogError(wxT("%s: Unable to allocate memory for %d FAT sectors"), wxT("cbff::ReadFAT()"), m_sshdr._csectFat);
		return false;
	}

#ifdef READ_FAT_OFFSETS
	// allocate memory for the fat sector offets
	if (!(m_FATOffsets = (wxFileOffset *)my_calloc(m_sshdr._csectFat, sizeof(wxFileOffset))))
	{
		free(m_FAT);
		m_FAT = NULL;
		wxLogError(wxT("%s: Unable to allocate memory for %d FAT sector offsets"), wxT("cbff::ReadFAT()"), m_sshdr._csectFat);
		return false;
	}
#endif

	/* read all the blocks into the buffer */
	FSINDEX nFS = m_sshdr._csectFat;
	FSINDEX fIdx = 0;
	int i;

	/* check all the fat entries in the header */
	for (i = 0; (i < 109) && (nFS > 0); i++)
	{
		if (m_sshdr._sectFat[i] != CBFF_SECT_FREE)
		{
			if (!GetSectorData(m_sshdr._sectFat[i], (BYTE *)m_FAT + (fIdx * m_sectorSize),
					m_sectorSize, 
#ifdef READ_FAT_OFFSETS
					m_FATOffsets + fIdx
#else
					NULL
#endif
				))
				break;
			fIdx++;
			nFS--;
		}
	}

	/* see if we got them all */
	if (nFS > 0)
	{
		wxLogWarning(wxT("%s: Only found %d of %d FAT sectors, truncating!"), wxT("cbff::ReadFAT()"), fIdx, m_sshdr._csectFat);

		// decrease the number of fat sectors
		m_sshdr._csectFat -= nFS;
	}

	return true;
}


bool cbff::ReadMiniFAT(void)
{
	// attempt to read it when we don't have one
	if (m_sshdr._csectMiniFat < 1)
		return false;

	/* allocate memory for the MiniFAT */
	m_MiniFAT = (SECT *)my_calloc(m_sshdr._csectMiniFat, m_sectorSize);
	if (!m_MiniFAT)
	{
		wxLogError(wxT("%s: Unable to allocate memory for %d MiniFAT sectors"), wxT("cbff::ReadMiniFAT()"), m_sshdr._csectMiniFat);
		m_sshdr._csectMiniFat = 0;
		return false;
	}

	// allocate memory for the file offsets
	m_MiniFATOffsets = (wxFileOffset *)my_calloc(m_sshdr._csectMiniFat, sizeof(wxFileOffset));
	if (!m_MiniFATOffsets)
	{
		wxLogError(wxT("%s: Unable to allocate memory for %d MiniFAT offsets"), wxT("cbff::ReadMiniFAT()"), m_sshdr._csectMiniFat);
		m_sshdr._csectMiniFat = 0;
		free(m_MiniFAT);
		m_MiniFAT = NULL;
		return false;
	}

	/* read all the blocks into the buffer */
	FSINDEX nFS = m_sshdr._csectMiniFat;
	FSINDEX fIdx = 0;
	SECT diroff = m_sshdr._sectMiniFatStart;
	visitedSectors visited;
	visited.Append(&diroff);

	/* check all the fat entries in the header */
	while (nFS > 0 && diroff != CBFF_SECT_ENDOFCHAIN)
	{
		if (!GetSectorData(diroff, (BYTE *)m_MiniFAT + (fIdx * m_sectorSize),
				m_sectorSize, m_MiniFATOffsets + fIdx))
			break;
		// m_MiniFATOffsets[fIdx] = sizeof(m_sshdr) + (diroff * m_sectorSize);
		nFS--;
		fIdx++;
		diroff = GetNextSectorId(diroff, visited);
	}

	/* see if we got them all */
	if (nFS > 0)
	{
		wxLogError(wxT("%s: Only found %d of %d MiniFAT sectors!"), wxT("cbff::ReadMiniFAT()"), fIdx, m_sshdr._csectMiniFat);
		return false;
	}
	return true;
}


bool cbff::ReadDirectory(void)
{
	// first figure out how many directory sectors we have
	m_nDirSects = 0;
	SECT diroff = m_sshdr._sectDirStart;
	visitedSectors visited;
	visited.Append(&m_sshdr._sectDirStart);
	while (diroff != CBFF_SECT_ENDOFCHAIN)
	{
		m_nDirSects++;
		diroff = GetNextSectorId(diroff, visited);
	}

	// allocate memory for them
	m_DIR = (DIRENT_T *)my_calloc(m_nDirSects, m_sectorSize);
	if (!m_DIR)
	{
		wxLogError(wxT("%s: Unable to allocate memory for %d directory entries"), wxT("cbff::ReadDirectory()"), m_nDirSects);
		return false;
	}

	m_DIROffsets = (wxFileOffset *)my_calloc(m_nDirSects, sizeof(wxFileOffset));
	if (!m_DIROffsets)
	{
		wxLogError(wxT("%s: Unable to allocate memory for %d directory offsets"), wxT("cbff::ReadDirectory()"), m_nDirSects);
		return false;
	}

	// read the sectors
	ULONG i;
	diroff = m_sshdr._sectDirStart;
	visited.Clear();
	visited.Append(&m_sshdr._sectDirStart);
	for (i = 0; i < m_nDirSects; i++)
	{
		if (!GetSectorData(diroff, (BYTE *)m_DIR + (i * m_sectorSize),
				m_sectorSize, m_DIROffsets + i))
			break;
		// m_DIROffsets[i] = sizeof(m_sshdr) + (diroff * m_sectorSize);
		diroff = GetNextSectorId(diroff, visited);
	}

	if (i < m_nDirSects)
	{
		wxLogError(wxT("%s: Only read %d of %d Directory sectors!"), wxT("cbff::ReadDirectory()"), i + 1, m_nDirSects);
		return false;
	}

	// calculate the number of directory entries
	for (i = 0; i < m_nDirSects * 4; i++)
	{
		// note the root dir entry for later
		if (!m_DirRoot
			&& m_DIR[i]._mse == CBFF_STGTY_ROOT)
			m_DirRoot = m_DIR + i;

		// if we find an invalid one, its likely there won't be more good ones
		if (m_DIR[i]._mse == CBFF_STGTY_INVALID)
			// wxLogWarning(wxT("%s: Encountered invalid directory entry (%d)"), wxT("cbff::ReadDirectory()"), i);
			break;
	}
	m_nDirEntries = i;

	return true;
}


//
// read the specified length of bytes from the specified sector
// the data is stoerd into the dest buffer
//
bool cbff::GetSectorData(SECT sector, BYTE *dest, size_t len, wxFileOffset *poff)
{
	wxFileOffset off;

	off = (sector << m_sshdr._uSectorShift) + sizeof(m_sshdr);
	if (m_file->Seek(off) != off)
	{
		wxLogError(wxT("%s: Unable to seek to offset 0x%lx"), wxT("cbff::GetSectorData()"), off);
		return false;
	}
	if (poff)
		*poff = off;

	ssize_t nr;
	ssize_t target_len = m_sectorSize;
	if (len < m_sectorSize)
		target_len = len;
	nr = m_file->Read(dest, target_len);
	if (nr != target_len)
	{
		wxLogError(wxT("%s: Unable to read sector 0x%x"), wxT("cbff::GetSectorData()"), sector);
		return false;
	}
	return true;
}


//
// read the specified length of bytes from the specified mini sector
// the data is stoerd into the dest buffer
//
bool cbff::GetMiniSectorData(SECT sector, BYTE *dest, size_t len, wxFileOffset *poff)
{
	// we must have a root directory entry
	if (!m_DirRoot)
		return false;

	// this is the byte offset into the ministream (which is a standard change of sectors)
	ULONG ms_off = (sector << m_sshdr._uMiniSectorShift);

	// skip the required number of normal sectors
	SECT diroff = m_DirRoot->_sectStart;
	visitedSectors visited;
	visited.Append(&diroff);
	while (ms_off >= m_sectorSize && diroff != CBFF_SECT_ENDOFCHAIN)
	{
		ms_off -= m_sectorSize;
		// just skip to the next one
		diroff = GetNextSectorId(diroff, visited);
	}

	if (diroff == CBFF_SECT_ENDOFCHAIN)
	{
		wxLogError(wxT("%s: End of chain reached skipping sectors"), wxT("cbff::GetMiniSectorData()"));
		return false;
	}

	// seek to the location in the file that we want
	wxFileOffset off;
	off = (diroff << m_sshdr._uSectorShift) + sizeof(m_sshdr);
	off += ms_off;
	if (m_file->Seek(off) != off)
	{
		wxLogError(wxT("%s: Unable to seek to offset 0x%lx"), wxT("cbff::GetMiniSectorData()"), off);
		return false;
	}
	if (poff)
		*poff = off;

	ssize_t nr;
	ssize_t target_len = m_miniSectorSize;
	if (len < m_miniSectorSize)
		target_len = len;
	nr = m_file->Read(dest, target_len);
	if (nr != target_len)
	{
		wxLogError(wxT("%s: Unable to read mini-sector 0x%x"), wxT("cbff::GetMiniSectorData()"), sector);
		return false;
	}

	return true;
}


//
// read the entire stream data
//
// m_start specifies the first sector in the stream data
// m_length specifies the total length of the stream (in bytes)
//
// the stream data is stored into m_data
//
void cbff::ReadStreamData(cbffStream *pStream)
{
	ULONG len = pStream->m_length;

	// do nothing for no data
	if (len < 1)
		return;

	// decide if we should use the mini-fat or the fat
	if (len < m_sshdr._ulMiniSectorCutoff)
	{
		// stream is stored in the MiniFAT
		ReadStreamDataMiniFAT(pStream);
		return;
	}
	// stream is stored in the regular FAT
	ReadStreamDataFAT(pStream);
}


void cbff::ReadStreamDataFAT(cbffStream *pStream)
{
	ULONG len = pStream->m_length;

	// allocate memory for the stream
	if (!(pStream->m_data = (BYTE *)my_calloc(len, sizeof(BYTE))))
	{
		wxLogError(wxT("%s: Unable to allocate %d bytes for stream at sector 0x%x"), 
			wxT("cbff::ReadStreamDataFAT()"), len, pStream->m_start);
		return;
	}

	// estimate sector count
	ULONG estSectCnt = len / m_sectorSize;
	if (len % m_sectorSize)
		estSectCnt++;

	// allocate memory for file offsets
	if (!(pStream->m_offsets = (wxFileOffset *)my_calloc(estSectCnt, sizeof(wxFileOffset))))
	{
		free(pStream->m_data);
		pStream->m_data = NULL;
		wxLogError(wxT("%s: Unable to allocate memory for stream file offsets"), wxT("cbff::ReadStreamDataFAT()"));
		return;
	}

	BYTE *p = pStream->m_data;
	SECT diroff = pStream->m_start;
	visitedSectors visited;

	visited.Append(&diroff);
	while (len > 0 && diroff != CBFF_SECT_ENDOFCHAIN)
	{
		ULONG rl = m_sectorSize;
		// if the remaining bytes are less than a sector, only get what we need...
		if (len < m_sectorSize)
			rl = len;

		// where to store the offset
		wxFileOffset *pOff = NULL;
		if (pStream->m_sectCnt < estSectCnt)
			pOff = pStream->m_offsets + pStream->m_sectCnt;
		else
			wxLogError(wxT("%s: Detected file offset array overflow!"), wxT("cbff::ReadStreamDataFAT()"));

		// get data / store offset
		if (!GetSectorData(diroff, p, rl, pOff))
			break;

		// successful get, increase loop values
		pStream->m_sectCnt++;
		len -= rl;
		p += rl;
		diroff = GetNextSectorId(diroff, visited);
	}

	if (len > 0)
		wxLogWarning(wxT("%s: Returning with %lu bytes remaining"), wxT("cbff::ReadStreamDataFAT()"), len);
}


void cbff::ReadStreamDataMiniFAT(cbffStream *pStream)
{
	ULONG len = pStream->m_length;

	// allocate memory for the stream
	if (!(pStream->m_data = (BYTE *)my_calloc(len, sizeof(BYTE))))
	{
		wxLogError(wxT("%s: Unable to allocate %d bytes for stream at sector 0x%x"), 
			wxT("cbff::ReadStreamDataMiniFAT()"), len, pStream->m_start);
		return;
	}

	// estimate sector count
	ULONG estSectCnt = len / m_miniSectorSize;
	if (len % m_miniSectorSize)
		estSectCnt++;

	// allocate memory for file offsets
	if (!(pStream->m_offsets = (wxFileOffset *)my_calloc(estSectCnt, sizeof(wxFileOffset))))
	{
		free(pStream->m_data);
		pStream->m_data = NULL;
		wxLogError(wxT("%s: Unable to allocate memory for stream file offsets"), wxT("cbff::ReadStreamDataMiniFAT()"));
		return;
	}

	BYTE *p = pStream->m_data;
	SECT diroff = pStream->m_start;
	visitedSectors visited;

	visited.Append(&diroff);
	while (len > 0 && diroff != CBFF_SECT_ENDOFCHAIN)
	{
		ULONG rl = m_miniSectorSize;
		// if the remaining bytes are less than a sector, only get what we need...
		if (len < m_miniSectorSize)
			rl = len;

		// where to store the offset
		wxFileOffset *pOff = NULL;
		if (pStream->m_sectCnt < estSectCnt)
			pOff = pStream->m_offsets + pStream->m_sectCnt;
		else
			wxLogError(wxT("%s: Detected file offset array overflow!"), wxT("cbff::ReadStreamDataMiniFAT()"));

		// get data / store offset
		if (!GetMiniSectorData(diroff, p, rl, pOff))
			break;

		// successful get, increase loop values
		pStream->m_sectCnt++;
		len -= rl;
		p += rl;
		diroff = GetNextMiniSectorId(diroff, visited);
	}

	if (len > 0)
		wxLogWarning(wxT("%s: Returning with %lu bytes remaining"), wxT("cbff::ReadStreamDataMiniFAT()"), len);
}


SECT cbff::GetNextSectorId(SECT sector, visitedSectors& visited)
{
	if (sector > ((m_sectorSize / sizeof(SECT)) * m_sshdr._csectFat))
	{
		wxLogWarning(wxT("%s: Index outside of FAT (%lu) requested.  Forcing end of chain!"), wxT("cbff::GetNextSectorId()"), sector);
		return CBFF_SECT_ENDOFCHAIN;
	}

	SECT *pElement = m_FAT + sector;
	if (visited.Find(pElement))
	{
		wxLogWarning(wxT("%s: Sector chain loop detected! (0x%x) Forcing end of chain!"), wxT("cbff::GetNextSectorId()"), sector);
		return CBFF_SECT_ENDOFCHAIN;
	}
	visited.Append(pElement);
	return *pElement;
}


SECT cbff::GetNextMiniSectorId(SECT sector, visitedSectors& visited)
{
	if (!m_MiniFAT)
	{
		wxLogWarning(wxT("%s: MiniFAT sector 0x%x requested, but no MiniFAT loaded!"), wxT("cbff::GetNextMiniSectorId()"), sector);
		return CBFF_SECT_ENDOFCHAIN;
	}

	if (sector > ((m_sectorSize / sizeof(SECT)) * m_sshdr._csectMiniFat))
	{
		wxLogWarning(wxT("%s: Index outside of MiniFAT (%lu) requested.  Forcing end of chain!"), wxT("cbff::GetNextMiniSectorId()"), sector);
		return CBFF_SECT_ENDOFCHAIN;
	}

	SECT *pElement = m_MiniFAT + sector;
	if (visited.Find(pElement))
	{
		wxLogWarning(wxT("%s: Mini-sector chain loop detected! (0x%x) Forcing end of chain!"), wxT("cbff::GetNextMiniSectorId()"), sector);
		return CBFF_SECT_ENDOFCHAIN;
	}
	visited.Append(pElement);
	return *pElement;
}


wxChar *cbff::HumanReadableByteOrder(void)
{
	switch (m_sshdr._uByteOrder)
	{
		case CBFF_LITTLE_ENDIAN:
			return wxT("Little Endian");
		default:
			return wxT("Unknown");
	}
}


wxChar *cbff::HumanReadableSectorType(SECT sector)
{
	switch (sector)
	{
		case CBFF_SECT_FREE:
			return wxT("Free");
		case CBFF_SECT_DIF:
			return wxT("DIF");
		case CBFF_SECT_ENDOFCHAIN:
			return wxT("EndOfChain");
		case CBFF_SECT_FAT:
			return wxT("FAT");
		default:
			return wxT("");
	}
}

wxChar *cbff::HumanReadableDirObjType(BYTE type)
{
	switch (type)
	{
		case CBFF_STGTY_INVALID:
			return wxT("STGTY_INVALID");
		case CBFF_STGTY_STORAGE:
			return wxT("STGTY_STORAGE");
		case CBFF_STGTY_STREAM:
			return wxT("STGTY_STREAM");
		case CBFF_STGTY_LOCKBYTES:
			return wxT("STGTY_LOCKBYTES");
		case CBFF_STGTY_PROPERTY:
			return wxT("STGTY_PROPERTY");
		case CBFF_STGTY_ROOT:
			return wxT("STGTY_ROOT");
		default:
			return wxT("Unknown");
	}
}

wxChar *cbff::HumanReadableColor(BYTE flags)
{
	switch (flags)
	{
		case CBFF_COLOR_RED:
			return wxT("Red");
		case CBFF_COLOR_BLACK:
			return wxT("Black");
		default:
			return wxT("Unknown");
	}
}


void cbff::ConvertDirEntName(DIRENT_T *pdir, wxString &dest)
{
	// XXX: portability?
	// is there a better way to read from something that is always unicode?
	ULONG len = pdir->_cb;
	if (len > (32 * sizeof(WCHAR)))
		len = (32 * sizeof(WCHAR));

	WCHAR tmp[32+1];
	memset(tmp, 0, sizeof(tmp));
	memcpy(tmp, pdir->_ab, len);
#if defined(__WXMSW__) && defined(UNICODE)
	dest = tmp;
#else
	ULONG i;
	dest.clear();
	for (i = 0; i < len; i++)
		dest += (wxChar)(tmp[i] & 0xff);
#endif
}


#ifdef __WIN32__
static void *my_calloc(size_t nmemb, size_t size)
{
	size_t max = -1;

	if (max / nmemb <= size)
		return 0;
	return calloc(nmemb, size);
}
#endif


// declare the exported function
DECLARE_FD_PLUGIN(cbff)
