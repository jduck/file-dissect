/*
 * Windows Compound Binary File Format implementation
 * Joshua J. Drake <jdrake idefense.com>
 * 
 * cbff.h:
 * class declaration for cbff class
 */
#ifndef __cbff_h_
#define __cbff_h_

// TODO: support more than 109 FAT sectors
// #define READ_FAT_OFFSETS

#include "fileDissectPlugin.h"
#include "cbff_defs.h" // compound binary file format (OLE 2)

#include "cbffStream.h"
#include "cbffStreamPlugin.h"
#include "cbffStreamPlugins.h"

// list of sectors we visited (used to prevent loops)
#include <wx/list.h>
WX_DECLARE_LIST(SECT, visitedSectors);

class cbff : public fileDissectPlugin
{
public:
	cbff(wxLog *plog, fileDissectTreeCtrl *tree);
	~cbff (void);

	// plugin member functions
	bool SupportsExtension(const wxChar *extension);
	void Dissect(void);
	void CloseFile(void);

private:
	void DestroyFileData(void);
	void InitFileData(void);

	// tree nodes for dissection output
	wxTreeItemId m_root_id;
	wxTreeItemId m_hdr_id;
	wxTreeItemId m_dir_root_id;

	// additional dissection routines
	bool DissectHeader(void);
	bool DissectFAT(void);
	bool DissectMiniFAT(void);
	bool DissectDirectory(void);
	void AddDirectoryNode(wxTreeItemId&, ULONG);
	void AddStreamData(wxTreeItemId&, wxString&, DIRENT_T *);

	// private file format functionality
	bool ReadStructuredStorageHeader(void);
	bool ReadFAT(void);
	bool ReadMiniFAT(void);
	bool ReadDirectory(void);
	void ReadStreamData(cbffStream *pStream);
	void ReadStreamDataFAT(cbffStream *pStream);
	void ReadStreamDataMiniFAT(cbffStream *pStream);

	//===============================
	// low-level file format methods
	//===============================
	bool GetSectorData(SECT sector, wxByte *dest, size_t len, wxFileOffset *poff);
	SECT GetNextSectorId(SECT sector, visitedSectors &visited);
	// mini-sector stuff
	bool GetMiniSectorData(SECT sector, wxByte *dest, size_t len, wxFileOffset *poff);
	SECT GetNextMiniSectorId(SECT sector, visitedSectors &visited);
	// directory stuff
	void ConvertDirEntName(DIRENT_T *pdir, wxString &dest);

	// this program is for human use after all...
	wxChar *HumanReadableByteOrder(void);
	wxChar *HumanReadableSectorType(SECT sector);
	wxChar *HumanReadableDirObjType(wxByte type);
	wxChar *HumanReadableColor(wxByte flags);

	//===============================
	// low-level file format details
	//===============================
	ULONG m_sectorSize;
	ULONG m_miniSectorSize;
	// header
	struct StructuredStorageHeader m_sshdr;
	// cache of FAT/miniFAT sectors
	SECT *m_FAT;
#ifdef READ_FAT_OFFSETS
	wxFileOffset *m_FATOffsets;
#endif
	SECT *m_MiniFAT;
	wxFileOffset *m_MiniFATOffsets;

	// directory details
	ULONG m_nDirSects;
	ULONG m_nDirEntries;
	struct StructuredStorageDirectoryEntry *m_DIR;
	struct StructuredStorageDirectoryEntry *m_DirRoot;
	wxFileOffset *m_DIROffsets;
	
	// plugin handling
	void QueryStreamPlugins(void);
	void ReadDesiredStreamData(void);
	void InvokeStreamPlugins(void);
	cbffStreamPlugins *m_plugins;

	// for passing to plugins
	cbffStreamList m_streams;
};

#endif
