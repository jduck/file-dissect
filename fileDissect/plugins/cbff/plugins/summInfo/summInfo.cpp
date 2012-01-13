/*
 * Windows Compound Binary File Format implementation
 * Joshua J. Drake <jdrake idefense.com>
 * 
 * summinfo.h:
 * class implementation for summinfo cbffStreamPlugin class
 */
#include "summInfo.h"

// Format IDs : windows appears to have these definted in objidl.h
#ifndef __objidl_h__
CLSID FMTID_SummaryInformation = { 0xf29f85e0, 0x4ff9, 0x1068, { 0xab, 0x91, 0x08, 0x00, 0x2b, 0x27, 0xb3, 0xd9 } };
CLSID FMTID_DocSummaryInformation = { 0xd5cdd502, 0x2e9c, 0x101b, { 0x93, 0x97, 0x08, 0x00, 0x2b, 0x2c, 0xf9, 0xae } };
CLSID FMTID_UserDefinedProperties = { 0xd5cdd505, 0x2e9c, 0x101b, { 0x93, 0x97, 0x08, 0x00, 0x2b, 0x2c, 0xf9, 0xae } };
#endif

// map of property types -> property value types
typedef struct __property_type_stru
{
	wxChar *name;
	ULONG id;
	ULONG type;
} propType_t;


/*
 * http://sedna-soft.de/summary-information-stream/
 *
Property IDs for the section {f29f85e0-4ff9-1068-ab91-08002b27b3d9}
01 	Code page (02) 	06 	Comments (1e) 	0b 	Last Printed (40) 	10 	Number of Characters (03)
02 	Title (1e) 	07 	Template (1e) 	0c 	Create Time/Date (40) 	11 	Thumbnail (47)
03 	Subject (1e) 	08 	Last Saved By (1e) 	0d 	Last Saved Time/Date (40) 	12 	Name of Creating Application (1e)
04 	Author (1e) 	09 	Revision Number (1e) 	0e 	Number of Pages (03) 	13 	Security (03)
05 	Keywords (1e) 	0a 	Total Editing Time (40) 	0f 	Number of Words (03) 	8000 0000 	Locale ID (13)
 */
propType_t summInfoPropIds[] =
{
	{ wxT("Code Page"),				SPT_CODEPAGE,		SVT_SHORT },
	{ wxT("Title"),					SPT_TITLE,			SVT_STRING },
	{ wxT("Subject"),				SPT_SUBJECT,		SVT_STRING },
	{ wxT("Author"),				SPT_AUTHOR,			SVT_STRING },
	{ wxT("Keywords"),				SPT_KEYWORDS,		SVT_STRING },
	{ wxT("Comments"),				SPT_COMMENTS,		SVT_STRING },
	{ wxT("Template"),				SPT_TEMPLATE,		SVT_STRING },
	{ wxT("Last Saved By"),			SPT_LASTAUTHOR,		SVT_STRING },
	{ wxT("Revision Number"),		SPT_REVNUMBER,		SVT_STRING },
	{ wxT("Total Editing Time"),	SPT_EDITTIME,		SVT_FILETIME },
	{ wxT("Last Printed"),			SPT_LASTPRINTED,	SVT_FILETIME },
	{ wxT("Create Time/Date"),		SPT_CREATE_DTM,		SVT_FILETIME },
	{ wxT("Last Saved Time/Date"),	SPT_LASTSAVE_DTM,	SVT_FILETIME },
	{ wxT("Number of Pages"),		SPT_PAGECOUNT,		SVT_LONG },
	{ wxT("Number of Words"),		SPT_WORDCOUNT,		SVT_LONG },
	{ wxT("Number of Characters"),	SPT_CHARCOUNT,		SVT_LONG },
	{ wxT("Thumbnail"),				SPT_THUMBNAIL,		SVT_CLIPBOARD },
	{ wxT("Name of Creating Application"),
									SPT_APPNAME,		SVT_STRING },
	{ wxT("Security"),				SPT_SECURITY,		SVT_LONG },
	{ wxT("Locale Id"),				SPT_LOCALEID,		SVT_ULONG },
	{ NULL,							0,					0 }
};


summInfo::summInfo(wxLog *plog, fileDissectTreeCtrl *tree)
{
	m_log = plog;
	wxLog::SetActiveTarget(m_log);
	m_tree = tree;

	m_description = wxT("SummaryInformation Stream Dissector");
	m_streams = 0;
}


void summInfo::MarkDesiredStreams(void)
{
	wxLog::SetActiveTarget(m_log);

	for (cbffStreamList::iterator i = m_streams->begin();
		i != m_streams->end();
		i++)
	{
		cbffStream *p = (cbffStream *)(*i);
		if (p->m_name.Matches(wxT("*SummaryInformation")))
		{
			// wxLogMessage(wxT("%s: flagging %s"), wxT("summInfo::MarkDesiredStream()"), p->m_name);
			p->m_wanted = true;
		}
	}
}


void summInfo::Dissect(void)
{
	// still need to visit all streams to know if we should actually dissect
	for (cbffStreamList::iterator i = m_streams->begin();
		i != m_streams->end();
		i++)
	{
		cbffStream *p = (cbffStream *)(*i);

		if (p->m_name.Matches(wxT("*SummaryInformation")))
		{
			// wxLogMessage(wxT("%s: dissecting %s"), wxT("summInfo::MarkDesiredStream()"), p->m_name);
			DissectStream(p);
		}
	}
}


void summInfo::DissectStream(cbffStream *pStream)
{
	// erm, wtf?
	if (!pStream->m_id)
		return;

	// Dissect Header
	if (pStream->m_length < sizeof(struct SummaryInformationHeader))
	{
		wxLogError(wxT("%s: Stream too short (no header)"), wxT("summInfo::DissectStream()"));
		return;
	}

	// add the header to the tree
	struct SummaryInformationHeader *phdr = (struct SummaryInformationHeader *)pStream->m_data;
	// XXX: what about tiny tiny mini sector sizes??
	wxFileOffset off = pStream->GetFileOffset(0);
	wxTreeItemId hdr_id = m_tree->AppendItem(pStream->m_id, wxT("Header"), -1, -1, 
		new fdTIData(off, sizeof(struct SummaryInformationHeader)));
	m_tree->AppendItem(hdr_id, wxString::Format(wxT("Byte Order: 0x%04x"), phdr->uByteOrder),
		-1, -1, new fdTIData(off + FDT_OFFSET_OF(uByteOrder, (*phdr)), FDT_SIZE_OF(uByteOrder, (*phdr))));
	m_tree->AppendItem(hdr_id, wxString::Format(wxT("Reserved: 0x%04x"), phdr->uReserved),
		-1, -1, new fdTIData(off + FDT_OFFSET_OF(uReserved, (*phdr)), FDT_SIZE_OF(uReserved, (*phdr))));
	m_tree->AppendItem(hdr_id, wxString::Format(wxT("OS Version: 0x%04x"), phdr->uOSVersion),
		-1, -1, new fdTIData(off + FDT_OFFSET_OF(uOSVersion, (*phdr)), FDT_SIZE_OF(uOSVersion, (*phdr))));
	m_tree->AppendItem(hdr_id, wxString::Format(wxT("Platform: 0x%04x"), phdr->uPlatform),
		-1, -1, new fdTIData(off + FDT_OFFSET_OF(uPlatform, (*phdr)), FDT_SIZE_OF(uPlatform, (*phdr))));
	m_tree->AppendItem(hdr_id, wxString::Format(
			wxT("CLSID: %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x"), 
			phdr->clsid.Data1,
			phdr->clsid.Data2,
			phdr->clsid.Data3,
			phdr->clsid.Data4[0],
			phdr->clsid.Data4[1],
			phdr->clsid.Data4[2],
			phdr->clsid.Data4[3],
			phdr->clsid.Data4[4],
			phdr->clsid.Data4[5],
			phdr->clsid.Data4[6],
			phdr->clsid.Data4[7]),
		-1, -1, new fdTIData(off + FDT_OFFSET_OF(clsid, (*phdr)), FDT_SIZE_OF(clsid, (*phdr))));
	m_tree->AppendItem(hdr_id, wxString::Format(wxT("Section Count: 0x%08x"), phdr->ulSectionCount),
		-1, -1, new fdTIData(off + FDT_OFFSET_OF(ulSectionCount, (*phdr)), FDT_SIZE_OF(ulSectionCount, (*phdr))));

	// validate the data
	if (phdr->uByteOrder != 0xfffe)
	{
		wxLogError(wxT("%s: Unsupported byte order (0x%04x) in section header!"), wxT("summInfo::DissectStream()"), phdr->uByteOrder);
		return;
	}
	if (phdr->uReserved != 0)
	{
		wxLogError(wxT("%s: The reserved field is non-zero"), wxT("summInfo::DissectStream()"));
		return;
	}

	// sections
	wxTreeItemId sroot_id = m_tree->AppendItem(pStream->m_id, wxT("Sections"));
	if (phdr->ulSectionCount == 0)
	{
		wxLogWarning(wxT("%s: Section count was zero!"), wxT("summInfo::DissectStream()"));
		return;
	}

	// ensure the section count wont overflow for the check below
	if (phdr->ulSectionCount > (0xffffffff / sizeof(struct SummaryInformationSectionDeclaration)))
	{
		wxLogError(wxT("%s: Section count would cause integer overflow!"), wxT("summInfo::DissectStream()"));
		return;
	}
	ULONG ulSHdrSz = phdr->ulSectionCount * sizeof(struct SummaryInformationSectionDeclaration);
	// adding the static size might cause overflow too
	if (0xffffffff - ulSHdrSz < sizeof(SummaryInformationHeader))
	{
		wxLogError(wxT("%s: Adding header size would cause integer overflow!"), wxT("summInfo::DissectStream()"));
		return;
	}
	ulSHdrSz += sizeof(SummaryInformationHeader);
	// make sure there are enough bytes in the stream for the section headers
	// NOTE: this doesn't factor in section data
	if (pStream->m_length < ulSHdrSz)
	{
		wxLogError(wxT("%s: Not enough data for section declarations"), wxT("summInfo::DissectStream()"));
		return;
	}

	// treat this as an array since we know we have enough for the section declarations
	struct SummaryInformationSectionDeclaration *psd = (struct SummaryInformationSectionDeclaration *)(pStream->m_data + sizeof(SummaryInformationHeader));

	// for each section...
	ULONG i;
	for (i = 0; i < phdr->ulSectionCount; i++)
	{
		// XXX: what about tiny tiny mini sector sizes??
		off = pStream->GetFileOffset(sizeof(SummaryInformationHeader) + (sizeof(SummaryInformationSectionDeclaration) * i));

		// ...add a pStream->m_id and...
		wxTreeItemId sec_id = m_tree->AppendItem(sroot_id, wxString::Format(wxT("Section %lu"), i));
		wxTreeItemId id = m_tree->AppendItem(sec_id, wxT("Header"),
			-1, -1, new fdTIData(off, sizeof(SummaryInformationSectionDeclaration)));

		if (pStream->m_name == wxT("\x05SummaryInformation"))
		{
			if (memcmp(&FMTID_SummaryInformation, &(psd[i].clsid), sizeof(CLSID)) != 0)
				wxLogWarning(wxT("%s: Section %lu CLSID is not FMTID_SummaryInformation!"), wxT("summInfo::DissectStream()"), i);
		}
		else if (pStream->m_name == (wxT("\x05") wxT("DocumentSummaryInformation")))
		{
			if (i == 0 && memcmp(&FMTID_DocSummaryInformation, &(psd[i].clsid), sizeof(CLSID)) != 0)
				wxLogWarning(wxT("%s: Section %lu CLSID is not FMTID_DocSummaryInformation!"), wxT("summInfo::DissectStream()"), i);
			if (i == 1 && memcmp(&FMTID_UserDefinedProperties, &(psd[i].clsid), sizeof(CLSID)) != 0)
				wxLogWarning(wxT("%s: Section %lu CLSID is not FMTID_UserDefinedProperties!"), wxT("summInfo::DissectStream()"), i);
		}

		// ...add declaration data...
		m_tree->AppendItem(id, wxString::Format(
			wxT("CLSID: %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x"), 
			psd[i].clsid.Data1,
			psd[i].clsid.Data2,
			psd[i].clsid.Data3,
			psd[i].clsid.Data4[0],
			psd[i].clsid.Data4[1],
			psd[i].clsid.Data4[2],
			psd[i].clsid.Data4[3],
			psd[i].clsid.Data4[4],
			psd[i].clsid.Data4[5],
			psd[i].clsid.Data4[6],
			psd[i].clsid.Data4[7]),
			-1, -1, new fdTIData(off + FDT_OFFSET_OF(clsid, psd[i]), FDT_SIZE_OF(clsid, psd[i])));
		m_tree->AppendItem(id, wxString::Format(wxT("Offset: 0x%08x"), psd[i].ulOffset),
			-1, -1, new fdTIData(off + FDT_OFFSET_OF(ulOffset, psd[i]), FDT_SIZE_OF(ulOffset, psd[i])));

		// make sure the offset is inside the stream
		if (psd[i].ulOffset > pStream->m_length)
		{
			wxLogError(wxT("%s: Section %lu offset outside of stream!"), wxT("summInfo::DissectStream()"), i);
			// go to the next one
			continue;
		}
		if (pStream->m_length - psd[i].ulOffset < sizeof(struct SummaryInformationSectionHeader))
		{
			wxLogError(wxT("%s: Not enough space for a section header at section %lu offset"), wxT("summInfo::DissectStream()"), i);
			// go to the next one
			continue;
		}

		// now add the section header data
		struct SummaryInformationSectionHeader *pshdr;
		pshdr = (struct SummaryInformationSectionHeader *)(pStream->m_data + psd[i].ulOffset);
		// XXX: what about tiny tiny mini sector sizes??
		off = pStream->GetFileOffset(psd[i].ulOffset);
		id = m_tree->AppendItem(sec_id, wxT("Section Header"),
			-1, -1, new fdTIData(off, sizeof(SummaryInformationSectionHeader)));
		m_tree->AppendItem(id, wxString::Format(wxT("Length: 0x%08x"), pshdr->ulLength),
			-1, -1, new fdTIData(off + FDT_OFFSET_OF(ulLength, (*pshdr)), FDT_SIZE_OF(ulLength, (*pshdr))));
		m_tree->AppendItem(id, wxString::Format(wxT("Property Count: 0x%08x"), pshdr->ulPropertyCount),
			-1, -1, new fdTIData(off + FDT_OFFSET_OF(ulPropertyCount, (*pshdr)), FDT_SIZE_OF(ulPropertyCount, (*pshdr))));

		wxTreeItemId proot_id = m_tree->AppendItem(sec_id, wxT("Properties"));

		// no properties?  go to next section
		if (pshdr->ulPropertyCount < 1)
		{
			wxLogWarning(wxT("%s: Section %lu property count was zero!"), wxT("summInfo::DissectStream()"), i);
			continue;
		}

		// prevent integer overflow with property count
		if (pshdr->ulPropertyCount > (ULONG_MAX / sizeof(struct SummaryInformationPropertyDeclaration)))
		{
			wxLogError(wxT("%s: Section %lu property count would cause integer overflow!"), wxT("summInfo::DissectStream()"), i);
			continue;
		}
		ULONG ulPHdrSz = pshdr->ulPropertyCount * sizeof(struct SummaryInformationPropertyDeclaration);
		if (ulPHdrSz > pStream->m_length)
		{
			wxLogError(wxT("%s: Not enough data for property declarations in section %lu"), wxT("summInfo::DissectStream()"), i);
			continue;
		}

		// point to section data!
		BYTE *sec_data = pStream->m_data + sizeof(SummaryInformationSectionHeader) + psd[i].ulOffset;
		struct SummaryInformationPropertyDeclaration *ppd = (struct SummaryInformationPropertyDeclaration *)sec_data;

		// treat this one as an array since we know we have enough data for the declarations
		ULONG j;
		for (j = 0; j < pshdr->ulPropertyCount; j++)
		{
			wxTreeItemId prop_id = m_tree->AppendItem(proot_id, wxString::Format(wxT("Property %lu"), j));

			// XXX: what about tiny tiny mini sector sizes??
			off = pStream->GetFileOffset(psd[i].ulOffset + sizeof(SummaryInformationSectionHeader) + (sizeof(struct SummaryInformationPropertyDeclaration) * j));

			id = m_tree->AppendItem(prop_id, wxT("Declaration"), -1, -1,
				new fdTIData(off, sizeof(struct SummaryInformationPropertyDeclaration)));

			// XXX: link PropIds arrays to CLSIDs (but have a generic handler for unknown ones)
			m_tree->AppendItem(id, wxString::Format(wxT("Id: 0x%08x (%s)"), ppd[j].ulPropertyId, 
					HumanReadablePropId(ppd[j].ulPropertyId)),
				-1, -1, new fdTIData(off + FDT_OFFSET_OF(ulPropertyId, ppd[j]), 
					FDT_SIZE_OF(ulPropertyId, ppd[j])));
			m_tree->AppendItem(id, wxString::Format(wxT("Offset: 0x%08x"), ppd[j].ulOffset),
				-1, -1, new fdTIData(off + FDT_OFFSET_OF(ulOffset, ppd[j]), 
					FDT_SIZE_OF(ulOffset, ppd[j])));

			// make sure the offset fits within this section's data
			if (ppd[j].ulOffset > pshdr->ulLength)
			{
				wxLogWarning(wxT("%s: Section %lu property %lu offset (0x%x) out of bounds!"), wxT("summInfo::DissectStream()"), i, j, ppd[j].ulOffset);
				continue;
			}
			if (pshdr->ulLength - ppd[j].ulOffset < sizeof(struct SummaryInformationProperty))
			{
				wxLogWarning(wxT("%s: Not enough data for information property in section %lu property %lu"), wxT("summInfo::DissectStream()"), i, j);
				continue;
			}

			// okay, we have enough for the property
			struct SummaryInformationProperty *pprop;
			pprop = (struct SummaryInformationProperty *)(pStream->m_data + psd[i].ulOffset + ppd[j].ulOffset);
			off = pStream->GetFileOffset(psd[i].ulOffset + ppd[j].ulOffset);
			id = m_tree->AppendItem(prop_id, wxT("Property"), -1, -1,
				new fdTIData(off, sizeof(struct SummaryInformationProperty)));

			// XXX: link PropIds arrays to CLSIDs (but have a generic handler for unknown ones)
			m_tree->AppendItem(id, wxString::Format(wxT("Type: 0x%08x (%s)"), pprop->ulType, 
					HumanReadablePropType(pprop->ulType)),
				-1, -1, new fdTIData(off + FDT_OFFSET_OF(ulType, (*pprop)), 
					FDT_SIZE_OF(ulType, (*pprop))));

			// property value
			switch (pprop->ulType)
			{
				case SVT_SHORT:
					m_tree->AppendItem(id, wxString::Format(wxT("Value: 0x%04x"), pprop->u.sWord1),
						-1, -1, new fdTIData(off + FDT_OFFSET_OF(u.sWord1, (*pprop)), 
							FDT_SIZE_OF(u.sWord1, (*pprop))));
					break;

				case SVT_LONG:
					m_tree->AppendItem(id, wxString::Format(wxT("Value: 0x%08x"), pprop->u.lDword1),
						-1, -1, new fdTIData(off + FDT_OFFSET_OF(u.lDword1, (*pprop)), 
							FDT_SIZE_OF(u.lDword1, (*pprop))));
					break;

				case SVT_ULONG:
					m_tree->AppendItem(id, wxString::Format(wxT("Value: 0x%08x"), pprop->u.ulDword1),
						-1, -1, new fdTIData(off + FDT_OFFSET_OF(u.ulDword1, (*pprop)), 
							FDT_SIZE_OF(u.ulDword1, (*pprop))));
					break;

				case SVT_STRING:
					m_tree->AppendItem(id, wxString::Format(wxT("Length: 0x%08x"), pprop->u.ulDword1),
						-1, -1, new fdTIData(off + FDT_OFFSET_OF(u.ulDword1, (*pprop)), 
							FDT_SIZE_OF(u.ulDword1, (*pprop))));
					if (pprop->u.ulDword1 > 0 && pprop->u.ulDword1 < pStream->m_length)
					{
						BYTE *str;

						str = pStream->m_data + psd[i].ulOffset + ppd[j].ulOffset + sizeof(struct SummaryInformationProperty);
						off = pStream->GetFileOffset(psd[i].ulOffset + ppd[j].ulOffset + sizeof(struct SummaryInformationProperty));
						wxString strValue = wxT("Value: ");
						strValue += wxString::From8BitData((const char *)str, pprop->u.ulDword1);
						m_tree->AppendItem(id, strValue, -1, -1, new fdTIData(off, pprop->u.ulDword1));
					}
					break;

				case SVT_BLOB:
					m_tree->AppendItem(id, wxString::Format(wxT("Length: 0x%08x"), pprop->u.ulDword1),
						-1, -1, new fdTIData(off + FDT_OFFSET_OF(u.ulDword1, (*pprop)), 
							FDT_SIZE_OF(u.ulDword1, (*pprop))));
					if (pprop->u.ulDword1 > 0 && pprop->u.ulDword1 <= pStream->m_length)
					{
						BYTE *str;

						str = pStream->m_data + psd[i].ulOffset + ppd[j].ulOffset + sizeof(struct SummaryInformationProperty);
						off = pStream->GetFileOffset(psd[i].ulOffset + ppd[j].ulOffset + sizeof(struct SummaryInformationProperty));

						wxString strValue = wxT("Value decoding not supported.");
						m_tree->AppendItem(id, strValue, -1, -1, new fdTIData(off, pprop->u.ulDword1));
					}
					break;

				case SVT_CLIPBOARD:
					m_tree->AppendItem(id, wxString::Format(wxT("Length: 0x%08x"), pprop->u.ulDword1),
						-1, -1, new fdTIData(off + FDT_OFFSET_OF(u.ulDword1, (*pprop)), 
							FDT_SIZE_OF(u.ulDword1, (*pprop))));
					if (pprop->u.ulDword1 > 0 && pprop->u.ulDword1 <= pStream->m_length)
					{
						ULONG left = pprop->u.ulDword1;
						DWORD format;
						BYTE *str;

						if (left < sizeof(DWORD))
						{
							wxLogWarning(wxT("%s: Not enough data for Clipboard Data in section %lu property %lu"), wxT("summInfo::DissectStream()"), i, j);
							break;
						}
						str = pStream->m_data + psd[i].ulOffset + ppd[j].ulOffset + sizeof(struct SummaryInformationProperty);
						off = pStream->GetFileOffset(psd[i].ulOffset + ppd[j].ulOffset + sizeof(struct SummaryInformationProperty));

						format = *(DWORD *)str;
						m_tree->AppendItem(id, wxString::Format(wxT("Format: 0x%08x"), format),
							-1, -1, new fdTIData(off, sizeof(DWORD)));
						off += sizeof(DWORD);
						left -= sizeof(DWORD);

						if (left > 0)
						{
							wxString strValue = wxT("Value decoding not supported.");
							//strValue += wxString::From8BitData((const char *)str, pprop->u.ulDword1);
							m_tree->AppendItem(id, strValue, -1, -1, new fdTIData(off, left));
						}
					}
					break;

				// XXX: add support for more types
				case SVT_FILETIME:
				default:
					m_tree->AppendItem(id, wxT("Unsupported type!"));
					break;

			} // end switch
		} // end property loop
	} // end section loop
}


wxChar *summInfo::HumanReadablePropId(ULONG id)
{
	ULONG i;

	for (i = 0; summInfoPropIds[i].name; i++)
	{
		if (summInfoPropIds[i].id == id)
			return summInfoPropIds[i].name;
	}
	return wxT("Unknown");
}


wxChar *summInfo::HumanReadablePropType(ULONG type)
{
	switch (type)
	{
		case SVT_SHORT:
			return wxT("Short");

		case SVT_LONG:
			return wxT("Long");

		case SVT_ULONG:
			return wxT("Unsigned Long");

		case SVT_STRING:
			return wxT("String");

		case SVT_FILETIME:
			return wxT("File Time");

		case SVT_BLOB:
			return wxT("BLOB");

		case SVT_CLIPBOARD:
			return wxT("Clipboard");
	}
	return wxT("Unknown");
}



DECLARE_CBF_PLUGIN(summInfo);
