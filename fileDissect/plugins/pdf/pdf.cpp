/*
 * Adobe Portable Document Format implementation
 * Joshua J. Drake <jdrake accuvant.com>
 * 
 * pdf.cpp:
 * implementation for pdf class
 */
#include "pdf.h"

// for handling stream data
#include "pdfPred.h"
#include <wx/zstream.h>


static wxByte *read_line_fwd(wxByte *str, size_t len, wxByte **ptr, size_t *plen);
static inline wxByte *find_number_end(wxByte *str, wxByte *end, bool *pdecimal);
static bool parse_two_integers(wxByte *str, size_t len, 
	unsigned long *pone, wxByte **p1s, wxByte **p1e,
	unsigned long *ptwo, wxByte **p2s, wxByte **p2e);

pdf::pdf(wxLog *plog, fileDissectTreeCtrl *tree)
{
	m_description = wxT("Portable Document Format");
	m_extensions = wxT("*.pdf;*.fdf");

	m_trailer = 0;
	InitFileData();

	m_log = plog;
	wxLog::SetActiveTarget(m_log);
	m_tree = tree;
}


void pdf::InitFileData(void)
{
	if (m_trailer)
	{
		delete m_trailer;
		m_trailer = 0;
	}

	m_xref_off = wxInvalidOffset;

	m_root_id.Unset();
	m_hdr_id.Unset();
	m_xref_id.Unset();
	m_indobj_id.Unset();

	pdfObjectsHashMap::iterator it = m_objects.begin();
	pdfObjectsHashMap::iterator en = m_objects.end();
    for (; it != en; ++it)
	{
		pdfObjectList *pOL = (pdfObjectList *)it->second;
		// For some reason, wxHashMap creates keys with NULL values whenever
		// you access an element that didn't exist. We can't delete NULL
		// so we just skip them..
		if (!pOL)
			continue;

		pdfObjectList::iterator itl = pOL->begin();
		pdfObjectList::iterator enl = pOL->end();
		for (; itl != enl; ++itl)
			pdfObjectBase::Delete((pdfObjectBase *)*itl);
		pOL->clear();
		delete pOL;
	}
	m_objects.clear();
}


pdf::~pdf(void)
{
	DestroyFileData();
}


void pdf::DestroyFileData(void)
{
	InitFileData();
}

void pdf::CloseFile(void)
{
	DestroyFileData();
}


bool pdf::SupportsExtension(const wxChar *extension)
{
	if (::wxStrcmp(extension, wxT("pdf")) == 0
		|| ::wxStrcmp(extension, wxT("fdf")) == 0)
		return true;
	return false;
}


void pdf::Dissect(void)
{
	if (!m_log || !m_tree || !m_file)
		return;

	// add a base node
	m_root_id = m_tree->AddRoot(wxT("Portable Document"));

	// dissect the rest
	while (1)
	{
		if (!DissectHeader())
			break;
		if (!DissectTrailer())
			break;
		if (!DissectXref())
			break;
		if (!DissectObjects())
			break;

		// we never really intended to loop, just a programming syntax hack!
		break;
	}

	// set the initial tree state
	m_tree->Expand(m_root_id);
	// m_tree->Expand(m_dir_root_id);
	m_tree->SelectItem(m_root_id);
}


bool pdf::DissectHeader(void)
{
	ssize_t nr;

	// add the header to the tree
	wxByte hdr[10] = { 0 };
	nr = m_file->Read(hdr, sizeof(hdr));
	if (nr != sizeof(hdr))
	{
		wxLogError(wxT("%s: Read returned %d"), wxT("DissectHeader"), nr);
		return false;
	}
	m_hdr_id = m_tree->AppendItem(m_root_id, wxT("Header"), -1, -1,
		new fdTIData(0, sizeof(hdr)-1));

	if (memcmp(hdr, "%PDF-", 5) != 0)
		wxLogWarning(wxT("%s: Not a PDF? Got: %s"), wxT("DissectHeader"), hdr);
	if (hdr[8] != 0x0a && hdr[8] != 0x0d)
		wxLogWarning(wxT("%s: Header does not end with a newline!"), wxT("DissectHeader"));
	hdr[8] = 0x0;

	if (hdr[6] != 0x2e)
		wxLogWarning(wxT("%s: Header does not have a M.m version!"), wxT("DissectHeader"));

	wxByte major = hdr[5];
	if (major != '1')
		wxLogWarning(wxT("%s: Header does not have a major version of 1!"), wxT("DissectHeader"));

	wxByte minor = hdr[7];
	if (minor < '0' || minor > '7')
		wxLogWarning(wxT("%s: Invalid minor version 0x%x!"), wxT("DissectHeader"), minor);

	// convert to decimal
	major -= '0';
	minor -= '0';

	wxTreeItemId id = m_tree->AppendItem(m_hdr_id, wxString::Format(wxT("Version: %u.%u"), major, minor), -1, -1, new fdTIData(5, 3));

	return true;
}


bool pdf::DissectTrailer(void)
{
	if (m_file->Length() < PDF_TRAILER_MIN_SIZE)
	{
		wxLogError(wxT("%s: Not enough data to read trailer!"), wxT("DissectTrailer"));
		return false;
	}

	// seek to the end and look for %%EOF
	wxFileOffset oldoff = m_file->Seek(0, wxFromEnd);
	if (oldoff == wxInvalidOffset)
	{
		wxLogError(wxT("%s: Failed to seek for trailer!"), wxT("DissectTrailer"));
		return false;
	}

	// We will add the "Trailer" node to the tree regardless of whether or not we end up
	// getting anything when parsing...
	wxTreeItemId trail_id = m_tree->AppendItem(m_root_id, wxT("Trailer"));

	// hold these for optimization
	wxByte *p_fend = m_file->GetAddress();
	wxByte *p_base = m_file->GetBaseAddress();

	wxByte *p_tend = p_fend;
	wxByte *p_sxref = NULL;

	// look for %%EOF
	wxByte *p_eof = m_file->FindStringReverse("%%EOF");
	if (p_eof)
	{
		// 18. Acrobat viewers require only that the %%EOF marker appear somewhere within the last 1024 bytes of the file.
		// We will try to get the last 1024 bytes or inspect everthing...
		if (p_fend - p_eof > 1024)
			// just warn
			wxLogWarning(wxT("%s: %%%%EOF was not found in the last 1024 bytes!"), wxT("DissectTrailer"));

		// adjust end ptr
		p_tend = p_eof + 5;

		// look for startxref
		p_sxref = m_file->FindStringReverse("startxref");
		if (!p_sxref)
		{
			// failed
			wxLogError(wxT("%s: Ran out of bytes looking for startxref line!"), wxT("DissectTrailer"));
			m_file->Seek(oldoff);
			return false;
		}

		// parse out the offset to xref table
		wxByte *p_off = p_sxref + 9;
		while (*p_off == 0x0a || *p_off == 0x0d)
			p_off++;
		wxByte *p_off_end;
		m_xref_off = strtoul((char *)p_off, (char **)&p_off_end, 10); // TODO: check for error

		// add the xref offset to the tree
		wxTreeItemId sx_id = m_tree->AppendItem(trail_id, wxString::Format(wxT("startxref"), m_xref_off), -1, -1, 
			new fdTIData(p_sxref - p_base, p_tend - p_sxref));
		m_tree->AppendItem(sx_id, wxString::Format(wxT("Offset: 0x%x"), m_xref_off), -1, -1, 
			new fdTIData(p_off - p_base, p_off_end - p_off));
	}
	else
		wxLogWarning(wxT("%s: Unable to locate %%%%EOF"), wxT("DissectTrailer"));

	// ok, try to find "trailer" (might not exist)
	wxByte *p_trailer = m_file->FindStringReverse("trailer");

	// default to "startxref" being the beginning of the trailer data selection
	// if we find a "trailer" then use that
	wxFileOffset trail_off = p_sxref - p_base;
	size_t trail_len = p_tend - p_sxref;
	if (p_trailer)
	{
		// found it!
		trail_off = p_trailer - p_base;
		trail_len = p_tend - p_trailer;
	}

	// XXX: TODO: get this from processing dictionary
	wxByte *p_trailer_end = p_sxref; 
	if (!p_trailer_end)
		p_trailer_end = p_fend;

	// create the trailer object and set the tree item data
	m_trailer = new pdfIndirect(0xffffffff, trail_off, 0xffffffff);
	m_trailer->m_id = trail_id;
	m_tree->SetItemData(trail_id, new fdTIData(trail_off, trail_len));

	// add trailer info, if we found some
	wxByte *p;
	if (p_trailer)
	{
		// point to start of dictionary
		p = p_trailer + 7;

		wxTreeItemId tmp = m_tree->AppendItem(m_trailer->m_id, wxT("Data"), -1, -1, 
			new fdTIData(p - p_base, p_trailer_end - p_trailer));

		// fill in the global trailer encapsulating object and use it in here..
		m_trailer->m_data = p;
		m_trailer->m_datalen = p_trailer_end - p;
		m_trailer->m_length = p_trailer_end - p_trailer;

		// parse read the dictionary after "trailer"
		(void) DissectData(tmp, m_trailer);

		// okay, now we have the trailer dictionary, lets check for some important keys
		pdfInteger *pInt = (pdfInteger *)m_trailer->m_dict->m_entries[wxT("XRefStm")];
		if (pInt && pInt->m_type == PDF_OBJ_INTEGER)
		{
			if (pInt->m_value >= m_file->Length())
				wxLogError(wxT("%s: Trailer dictionary contains \"XRefStm\" value that is out of range!"), wxT("DissectTrailer"));
			else
				(void) DissectXrefStm(p_base + pInt->m_value, p_base);
		}

		// if the Prev key exists, it represents the offset where we'll find a traditional xref table
		pInt = (pdfInteger *)m_trailer->m_dict->m_entries[wxT("Prev")];
		if (pInt && pInt->m_type == PDF_OBJ_INTEGER)
		{
			if (pInt->m_value >= m_file->Length())
				wxLogError(wxT("%s: Trailer dictionary contains \"Prev\" value that is out of range!"), wxT("DissectTrailer"));
			else
				(void) DissectXref(pInt->m_value);
		}
	}
	else
		wxLogWarning(wxT("%s: No trailer dictionary was found!"), wxT("DissectTrailer"));

	m_file->Seek(oldoff);
	return true;
}


bool pdf::DissectXref(wxFileOffset offset)
{
	// we'll add an Xref node, regardless of if we end up finding stuff inside...
	if (!m_xref_id.IsOk())
		m_xref_id = m_tree->AppendItem(m_root_id, wxT("Xref"));

	wxTreeItemId id; // for tmp usage

	// if we don't have one, we can't dissect it..
	if (offset == wxInvalidOffset)
		offset = m_xref_off;
	if (offset == wxInvalidOffset)
	{
		wxLogError(wxT("%s: Called without an offset to dissect!"), wxT("DissectXref"));
		return false;
	}

	wxFileOffset oldoff = m_file->Seek(offset);
	if (oldoff == wxInvalidOffset)
	{
		wxLogError(wxT("%s: Failed to seek for xref table!"), wxT("DissectXref"));
		return false;
	}

	wxByte *base = m_file->GetBaseAddress();
	wxByte *p = m_file->GetAddress();
	size_t slen, len = m_file->Length();
	wxByte *end = base + len;

	// try to read the xref line
	wxByte *p_xref = read_line_fwd(base, len, &p, &slen);
	if (slen < 4 || memcmp(p_xref, "xref", 4) != 0)
	{
		// there's no traditional xref table here, let's try it as an object...
		bool ret = false;
		if (DissectXrefStm(p_xref, base))
			ret = true;
		else
			wxLogError(wxT("%s: Did not find an xref table or stream @ 0x%x!"), wxT("DissectXref"), offset);

		m_file->Seek(oldoff);
		return ret;
	}

	// if we get here, then its definitely an xref table, process the subsections
	wxByte *p_xref_end = p_xref; // default to 0 bytes
	while (p < end)
	{
		// read the subsection line
		wxByte *p_first = read_line_fwd(base, len, &p, &slen);

		// if we find "trailer" then we just break out of the loop
		if (memcmp(p_first, "trailer", 7) == 0)
		{
			p_xref_end = p_first;
			break;
		}

		// parse the two integers, keeping track of their start/end pointers
		unsigned long first_obj, num_ents;
		wxByte *p_1end, *p_2nd, *p_2end;
		if (!parse_two_integers(p_first, slen, &first_obj, NULL, &p_1end, &num_ents, &p_2nd, &p_2end))
		{
			wxLogError(wxT("%s: Unable to parse subsection in the xref table @ 0x%x!"), wxT("DissectXref"), p_first - base);
			m_file->Seek(oldoff);
			return false;
		}

		// add stuff to the tree
		wxTreeItemId entry_id, ss_id, entries_id;
		wxFileOffset off;

		// add first subsection
		ss_id = m_tree->AppendItem(m_xref_id, wxT("Subsection"), -1, -1, new fdTIData(p_first - base, slen));
		off = p_first - base;
		id = m_tree->AppendItem(ss_id, wxString::Format(wxT("First Object: %u"), first_obj), -1, -1, new fdTIData(off, p_1end - p_first));
		off = p_2nd - base;
		id = m_tree->AppendItem(ss_id, wxString::Format(wxT("Object Count: %u"), num_ents), -1, -1, new fdTIData(off, p_2end - p_2nd));
		entries_id = m_tree->AppendItem(ss_id, wxT("Entries")); // TODO: offset/len for selection

		// now try to read the specified number of regular entries
#if 0
	// quickly check the trailer dictionary "Size" element
	pdfInteger *pTD = (pdfInteger *)m_trailer->m_dict->m_entries[wxT("Size")];
	if (pTD && pTD->m_type == PDF_OBJ_INTEGER && pTD->m_value > 0)
		num_ents = pTD->m_value;
#endif

		wxByte *ln;
		char *tmp;
		unsigned long n = 0;
		unsigned long next_obj = first_obj;
		unsigned long nnn, generation;
		while (n < num_ents 
			&& (ln = read_line_fwd(base, len, &p, &slen)))
		{
			tmp = (char *)ln;

			// NOTE: some PDF writers write \x20\x0a instead of \x0d\x0a
			// This is permitted by the spec.
			if (slen < 18 || slen > 19)
			{
				// maybe its the beginning of another sub-section, lets try
				// NOTE: we should never reach this with well-formed files (proper object counts for each subsection)
				p = ln;
				break;
			}
#if 0
				unsigned long n_first, n_num;
				if (parse_two_integers(ln, slen, &n_first, NULL, &p_1end, &n_num, &p_2nd, &p_2end))
				{
					// yep!
					ss_id = m_tree->AppendItem(m_xref_id, wxT("Subsection"), -1, -1, new fdTIData(ln - base, slen));
					off = ln - base;
					id = m_tree->AppendItem(ss_id, wxString::Format(wxT("First Object: %u"), n_first), -1, -1, new fdTIData(off, p_1end - ln));
					off = p_2nd - base;
					id = m_tree->AppendItem(ss_id, wxString::Format(wxT("Object Count: %u"), n_num), -1, -1, new fdTIData(off, p_2end - p_2nd));
					entries_id = m_tree->AppendItem(ss_id, wxT("Entries")); // TODO: offset/len for selection

					// make sure we aren't trying to go overboard (not that we would anyway)
					if (n_num > num_ents - n)
					{
						n_num = num_ents - n;
						wxLogWarning(wxT("%s: Sub-section contains more entires than specified, reading first %u"), wxT("DissectXref"), n_num);
					}
					next_obj = n_first;
					continue;
				}

				wxLogError(wxT("%s: Xref entry %u has an invalid length (%u)!"), wxT("DissectXref"), n, slen);
				n++;
				continue;
			}
#endif

			if (ln[10] != ' ')
			{
				wxLogError(wxT("%s: Xref entry %u is malformed (1)!"), wxT("DissectXref"), n);
				n++;
				next_obj++;
				continue;
			}

			char *ep;
			nnn = strtoul(tmp, &ep, 10);

			if ((wxByte *)ep != (ln + 10) || ln[16] != ' ')
			{
				wxLogError(wxT("%s: Xref entry %u is malformed (2)!"), wxT("DissectXref"), n);
				n++;
				next_obj++;
				continue;
			}

			generation = strtoul(tmp + 11, &ep, 10);
			if ((wxByte *)ep != (ln + 16))
			{
				wxLogError(wxT("%s: Xref entry %u is malformed (2)!"), wxT("DissectXref"), n);
				n++;
				next_obj++;
				continue;
			}

			wxByte type = ln[17]; // free or in use

			// got all the data, add a node for this entry
			off = ln - base;
			entry_id = m_tree->AppendItem(entries_id, wxString::Format(wxT("Entry %u"), n), -1, -1, new fdTIData(off, slen));

			wxString strType;
			switch (type)
			{
			case 'n':
				strType = wxT("In-Use");
				m_tree->SetItemText(entry_id, wxString::Format(wxT("Entry %u - Object %u %u"), n, next_obj, generation));
				id = m_tree->AppendItem(entry_id, wxString::Format(wxT("Offset: 0x%x"), nnn), -1, -1, new fdTIData(off, 10));
				{
					wxString key = wxString::Format(wxT("%u %u"), next_obj, generation);
					pdfObjectList *pOL = (pdfObjectList *)m_objects[key];
					if (!pOL)
					{
						m_objects[key] = pOL = new pdfObjectList();
						pOL->Append(new pdfIndirect(next_obj, nnn, generation));
					}
					else
					{
						bool found = false;
						pdfObjectList::iterator pli = pOL->begin();
						pdfObjectList::iterator ple = pOL->end();
						for (; pli != ple; ++pli)
						{
							pdfIndirect *pInd = (pdfIndirect *)*pli;
							if (pInd->m_offset == nnn)
							{
								found = true;
								break;
							}
						}
						if (!found)
						{
							wxLogWarning(wxT("%s(%u %u): Object number already defined!"), wxT("DissectXref"), next_obj, generation);
							pOL->Append(new pdfIndirect(next_obj, nnn, generation));
						}
					}
				}
				break;

			case 'f':
				strType = wxT("Free");
				id = m_tree->AppendItem(entry_id, wxString::Format(wxT("Next Free Object: %u"), nnn), -1, -1, new fdTIData(off, 10));
				// we don't currently save the free objects
				break;

			default:
				strType = wxT("Unknown");
				wxLogWarning(wxT("%s: Encountered unknown object type (#%u - 0x%x)!"), wxT("DissectXref"), n, type);
				break;
			}

			off += 11;
			id = m_tree->AppendItem(entry_id, wxString::Format(wxT("Generation: %u"), generation), -1, -1, new fdTIData(off, 5));

			off += 6;
			id = m_tree->AppendItem(entry_id, wxString::Format(wxT("Entry type: %s"), strType), -1, -1, new fdTIData(off, 1));

			n++;
			next_obj++;
		}
	}

	// set the item data for the xref table
	m_tree->SetItemData(m_xref_id, new fdTIData(offset, p_xref_end - p_xref));

	m_file->Seek(oldoff);
	return true;
}


bool pdf::DissectXrefStm(wxByte *ptr, wxByte *base)
{
	// we'll add an Xref node, regardless of if we end up finding stuff inside...
	if (!m_xref_id.IsOk())
		m_xref_id = m_tree->AppendItem(m_root_id, wxT("Xref"));

	// maybe its an xrefstm ?
	pdfIndirect *xref_obj = new pdfIndirect(0xffffffff, ptr - base, 0xffffffff);
	if (!ReadIndirect(xref_obj))
	{
		wxLogError(wxT("%s: Did not find an xref table or stream @ 0x%x!"), wxT("DissectXref"), xref_obj->m_offset);
		delete xref_obj;
		return false;
	}

	// successfully read an indirect object ("N G obj ... endobj")
	// we assume its an xref stream for now...
	wxTreeItemId xrefstm_id = m_tree->AppendItem(m_xref_id, wxString::Format(wxT("Xref Stream - Object %u %u"), xref_obj->m_number, xref_obj->m_generation));
	if (!DissectData(xrefstm_id, xref_obj))
	{
		wxLogError(wxT("%s: Unable to dissect xref stream data @ 0x%x!"), wxT("DissectXref"), xref_obj->m_offset);
		delete xref_obj;
		return false;
	}
	
	// set the tree item data now that we processed the guts
	m_tree->SetItemData(xrefstm_id, new fdTIData(xref_obj->m_offset, xref_obj->m_length));

	// we successfuly parsed and read in the associated data with this indirect object
	// now let's make sure its an xref stream and contains what we need
	if (!xref_obj->m_dict)
	{
		wxLogError(wxT("%s: Xref stream did not contain a dictionary @ 0x%x!"), wxT("DissectXref"), xref_obj->m_offset);
		delete xref_obj;
		return false;
	}
	if (!xref_obj->m_stream)
	{
		wxLogError(wxT("%s: Xref stream did not contain a stream @ 0x%x!"), wxT("DissectXref"), xref_obj->m_offset);
		delete xref_obj;
		return false;
	}

	pdfName *pType = (pdfName *)xref_obj->m_dict->m_entries[wxT("Type")];
	if (!pType)
	{
		wxLogError(wxT("%s: Xref stream dictionary did not contain a \"/Type\" key @ 0x%x!"), wxT("DissectXref"), xref_obj->m_offset);
		delete xref_obj;
		return false;
	}
	if (pType->m_type != PDF_OBJ_NAME)
	{
		wxLogError(wxT("%s: Xref stream dictionary \"Type\" key was not a name value @ 0x%x!"), wxT("DissectXref"), xref_obj->m_offset);
		delete xref_obj;
		return false;
	}
	if (pType->m_value != wxT("XRef"))
	{
		wxLogError(wxT("%s: Xref stream dictionary \"Type\" key was not \"XRef\" @ 0x%x!"), wxT("DissectXref"), xref_obj->m_offset);
		delete xref_obj;
		return false;
	}

	// ok, we know for sure that is an xref stream now, lets try to get the other required entries
	pdfInteger *sz = (pdfInteger *)xref_obj->m_dict->m_entries[wxT("Size")];
	if (!sz || sz->m_type != PDF_OBJ_INTEGER)
	{
		wxLogError(wxT("%s: Xref stream dictionary \"Size\" key was not an integer @ 0x%x!"), wxT("DissectXref"), xref_obj->m_offset);
		delete xref_obj;
		return false;
	}

	pdfArray *fmt = (pdfArray *)xref_obj->m_dict->m_entries[wxT("W")];
	if (!fmt || fmt->m_type != PDF_OBJ_ARRAY)
	{
		wxLogError(wxT("%s: Xref stream dictionary \"W\" key was not an array @ 0x%x!"), wxT("DissectXref"), xref_obj->m_offset);
		delete xref_obj;
		return false;
	}
	if (fmt->m_list.size() != 3)
	{
		wxLogError(wxT("%s: Xref stream dictionary \"W\" key did not contain three elements @ 0x%x!"), wxT("DissectXref"), xref_obj->m_offset);
		delete xref_obj;
		return false;
	}

	// optional members
	pdfArray *idx = (pdfArray *)xref_obj->m_dict->m_entries[wxT("Index")];
	if (idx && idx->m_type != PDF_OBJ_ARRAY)
	{
		wxLogError(wxT("%s: Xref stream dictionary \"Index\" key was not an array @ 0x%x!"), wxT("DissectXref"), xref_obj->m_offset);
		delete xref_obj;
		return false;
	}
	pdfInteger *prev = (pdfInteger *)xref_obj->m_dict->m_entries[wxT("Prev")];
	if (prev && prev->m_type != PDF_OBJ_INTEGER)
	{
		wxLogError(wxT("%s: Xref stream dictionary \"Prev\" key was not an integer @ 0x%x!"), wxT("DissectXref"), xref_obj->m_offset);
		delete xref_obj;
		return false;
	}

	// ok, whatever table 3.15 (xref stream dict) entries we have check out..

	// figure out how to decode
	pdfDictionary *pDP = (pdfDictionary *)xref_obj->m_dict->m_entries[wxT("DecodeParms")];
	if (!pDP)
		pDP = (pdfDictionary *)xref_obj->m_dict->m_entries[wxT("DP")];
	if (pDP && pDP->m_type != PDF_OBJ_DICTIONARY)
	{
		wxLogError(wxT("%s: Xref stream dictionary \"DecodeParms\" key was not a dictionary @ 0x%x!"), wxT("DissectXref"), xref_obj->m_offset);
		delete xref_obj;
		return false;
	}

	pdfName *pFilter = (pdfName *)xref_obj->m_dict->m_entries[wxT("Filter")];
	if (pFilter && pFilter->m_type != PDF_OBJ_NAME)
	{
		wxLogError(wxT("%s: Xref stream dictionary \"Filter\" key was not a name @ 0x%x!"), wxT("DissectXref"), xref_obj->m_offset);
		delete xref_obj;
		return false;
	}

	// decode stream data
	wxInputStream *p_in = new wxMemoryInputStream((const char *)xref_obj->m_stream->m_ptr, xref_obj->m_stream->m_len);

	// flate decoded?
	if (pFilter && pFilter->m_value == wxT("FlateDecode"))
		p_in = new wxZlibInputStream(p_in, wxZLIB_ZLIB);

	if (pDP)
	{
		pdfInteger *pCols = (pdfInteger *)pDP->m_entries[wxT("Columns")];
		if (!pCols || pCols->m_type != PDF_OBJ_INTEGER)
		{
			wxLogError(wxT("%s: Xref stream dictionary \"DecodeParms\" key has illegal \"Columns\" key @ 0x%x!"), wxT("DissectXref"), xref_obj->m_offset);
			delete xref_obj;
			return false;
		}

		pdfInteger *pPredictor = (pdfInteger *)pDP->m_entries[wxT("Predictor")];
		if (!pPredictor || pPredictor->m_type != PDF_OBJ_INTEGER)
		{
			wxLogError(wxT("%s: Xref stream dictionary \"DecodeParms\" key has illegal \"Predictor\" key @ 0x%x!"), wxT("DissectXref"), xref_obj->m_offset);
			delete xref_obj;
			return false;
		}

		p_in = new pdfPredInputStream(p_in, pPredictor->m_value, pCols->m_value);
	}

	unsigned long cnt = 0;
	unsigned long first_obj = 0;

	if (idx)
	{
		// XXX: need to check if these are indeed integers!
		pdfInteger *pStart = (pdfInteger *)idx->m_list[0];
		first_obj = pStart->m_value;
		pdfInteger *pEnd = (pdfInteger *)idx->m_list[1];
		cnt = pEnd->m_value;
	}
	else
		cnt = sz->m_value;

	// what follows is a hard-coded 1,2,1 xref table decode.
	unsigned long j = 0;
	unsigned long value[3] = { 0 }; // the fmt->m_list was already validated to be exactly 3 entries
	for (unsigned long i = 0; i < cnt; i++)
	{
		wxString strEntry;
		strEntry = wxString::Format(wxT("Entry %d -"), first_obj + i);

		j = 0;
		memset(value, 0, sizeof(value));
		for (pdfObjectList::iterator it = fmt->m_list.begin(), en = fmt->m_list.end(); it != en; ++it, ++j)
		{
			pdfInteger *pFldSz = (pdfInteger *)*it;
			wxByte buf[4];

			// XXX: TODO: check if its an integer!
			switch (pFldSz->m_value)
			{
			case 1:
				memset(buf, 0, sizeof(buf));
				p_in->Read(buf, 1);
				value[j] = buf[0];
				strEntry += wxString::Format(wxT(" 0x%02x"), value[j]);
				break;

			case 2:
				memset(buf, 0, sizeof(buf));
				p_in->Read(buf + 1, 1);
				p_in->Read(buf, 1);
				value[j] = *(wxUint16 *)buf;
				strEntry += wxString::Format(wxT(" 0x%04x"), value[j]);
				break;

			case 3:
				memset(buf, 0, sizeof(buf));
				p_in->Read(buf + 2, 1);
				p_in->Read(buf + 1, 1);
				p_in->Read(buf, 1);
				value[j] = *(wxUint32 *)buf;
				strEntry += wxString::Format(wxT(" 0x%06x"), value[j]);
				break;

			case 4:
				memset(buf, 0, sizeof(buf));
				p_in->Read(buf + 3, 1);
				p_in->Read(buf + 2, 1);
				p_in->Read(buf + 1, 1);
				p_in->Read(buf, 1);
				value[j] = *(wxUint32 *)buf;
				strEntry += wxString::Format(wxT(" 0x%08x"), value[j]);
				break;

			default:
				wxLogError(wxT("%s: Xref stream dictionary \"W\" key has an illegal value (%u) @ 0x%x!"), wxT("DissectXref"), pFldSz->m_value, xref_obj->m_offset);
				break;
			}
		}

		// indirect object
		if (value[0] == 0x01)
		{
			wxString key = wxString::Format(wxT("%u %u"), first_obj + i, value[2]);
			pdfObjectList *pOL = (pdfObjectList *)m_objects[key];
			if (!pOL)
			{
				m_objects[key] = pOL = new pdfObjectList();
				pOL->Append(new pdfIndirect(first_obj + i, value[1], value[2]));
			}
			else
			{
				bool found = false;
				pdfObjectList::iterator pli = pOL->begin();
				pdfObjectList::iterator ple = pOL->end();
				for (; pli != ple; ++pli)
				{
					pdfIndirect *pInd = (pdfIndirect *)*pli;
					if (pInd->m_offset == value[1])
					{
						found = true;
						break;
					}
				}
				if (!found)
				{
					wxLogWarning(wxT("%s(%u %u): Object number already defined!"), wxT("DissectTrailer"), first_obj + i, value[2]);
					pOL->Append(new pdfIndirect(first_obj + i, value[1], value[2]));
				}
			}
		}

		// add to the tree
		m_tree->AppendItem(xref_obj->m_stream->m_id, strEntry);
	}

	if (prev)
	{
		if (prev->m_value >= m_file->Length())
			wxLogError(wxT("%s: Xref stream dictionary contains \"Prev\" value that is out of range!"), wxT("DissectTrailer"));
		else
			DissectXrefStm(base + prev->m_value, base);
	}

	// everything must have been good, clean up and return true
	delete p_in;
	delete xref_obj; // we don't need this object because we'll end up reading it again...
	return true;
}


bool pdf::DissectObjects(void)
{
	m_indobj_id = m_tree->AppendItem(m_root_id, wxT("Indirect Objects")); // no offset assoicated

	for (pdfObjectsHashMap::iterator oi = m_objects.begin(); 
		oi != m_objects.end();
		oi++)
	{
		pdfObjectList *pOL = (pdfObjectList *)oi->second;
		if (!pOL) // skip empty elements
			continue;

		for (pdfObjectList::iterator oli = pOL->begin();
			oli != pOL->end();
			++oli)
		{
			pdfIndirect *pObj = (pdfIndirect *)*oli;
			if (pObj->m_type != PDF_OBJ_INDIRECT)
				continue;

			pObj->m_id = m_tree->AppendItem(m_indobj_id, wxString::Format(wxT("Object %u %u"), pObj->m_number, pObj->m_generation));

			if (!ReadIndirect(pObj))
				continue;

			m_tree->SetItemData(pObj->m_id, new fdTIData(pObj->m_offset, pObj->m_length));

			if (!DissectData(pObj->m_id, pObj))
				continue;

			if (pObj->m_stream)
				(void) DissectStream(pObj);
		}
	}

	// XXX: TODO: can we sort numerically instead of lexigraphically?
	m_tree->SortChildren(m_indobj_id);
	return true;
}


bool pdf::ReadIndirect(pdfIndirect *pObj)
{
	// beware, calling this on a 'free' object is naughty.

	// no object could possibly be at offset 0, reject immediately
	if (pObj->m_offset == 0)
	{
		wxLogError(wxT("%s: Object #%u cannot possibly be at 0x%x!"), wxT("ReadObject"), pObj->m_number, pObj->m_offset);
		return false;
	}

	wxFileOffset oldoff = m_file->Seek(pObj->m_offset);
	if (oldoff == wxInvalidOffset)
	{
		wxLogError(wxT("%s: Failed to seek for object #%u at 0x%x!"), wxT("ReadObject"), pObj->m_number, pObj->m_offset);
		return false;
	}

	// look for the beginning of the indirect object
	wxByte *p = m_file->GetAddress();
	wxByte *p_start = m_file->FindString("obj");
	if (!p_start)
	{
		wxLogError(wxT("%s: Failed to find \"obj\" (object #%u at 0x%x)!"), wxT("ReadObject"), pObj->m_number, pObj->m_offset);
		m_file->Seek(oldoff);
		return false;
	}

	// try to parse the numbers
	size_t len = p_start - p;
	if (len < 4)
	{
		wxLogError(wxT("%s: Malformed object line (object #%u at 0x%x)!"), wxT("ReadObject"), pObj->m_number, pObj->m_offset);
		m_file->Seek(oldoff);
		return false;
	}

	// XXX: Perhaps this could utilize parse_two_integers()
	unsigned long num, gen;
	wxByte *p_1end, *p_2nd, *p_2end;
	if (!parse_two_integers(p, p_start - p, &num, NULL, &p_1end, &gen, &p_2nd, &p_2end))
	{
		wxLogError(wxT("%s: Unable to parse object number (object #%u at 0x%x)!"), wxT("ReadObject"), pObj->m_number, pObj->m_offset);
		m_file->Seek(oldoff);
		return false;
	}

	// check for " obj" here
	if (memcmp(p_2end, " obj", 4) != 0)
	{
		wxLogError(wxT("%s: \"obj\" does not follow generation (object #%u at 0x%x)!"), wxT("ReadObject"), pObj->m_number, pObj->m_offset);
		m_file->Seek(oldoff);
		return false;
	}

	// look for endobj
	// XXX: this is very error prone
	wxByte *pEnd = m_file->FindString("endobj");
	if (!pEnd)
	{
		wxLogError(wxT("%s: Unable to find \"endobj\" (object #%u at 0x%x)!"), wxT("ReadObject"), pObj->m_number, pObj->m_offset);
		m_file->Seek(oldoff);
		return false;
	}

	// double-check the numbers if they were known coming in!
	if (pObj->m_number != 0xffffffff)
	{
		if (pObj->m_number != num)
			wxLogWarning(wxT("%s: Object number does not match xref, overriding (object #%u at 0x%x)!"), wxT("ReadObject"), pObj->m_number, pObj->m_offset);
	}
	else
		pObj->m_number = num;
	if (pObj->m_generation != 0xffffffff)
	{
		if (pObj->m_generation != gen)
			wxLogWarning(wxT("%s: Generation does not match xref, overriding (object #%u at 0x%x)!"), wxT("ReadObject"), pObj->m_number, pObj->m_offset);
	}
	else
		pObj->m_generation = gen;

	// including obj/endobj
	pObj->m_length = (pEnd + 6) - p;

	// create object data
	wxByte *pDataStart = (wxByte *)p_2end + 4;
	len = pEnd - pDataStart;
	if (len > 0)
	{
		pObj->m_datalen = len;
		pObj->m_data_offset = pObj->m_offset + (pDataStart - p);
		pObj->m_data = pDataStart;
	}

	m_file->Seek(oldoff);
	return true;
}


static inline wxByte decode_hex_nibble(wxByte *p)
{
	wxByte b = 0;

	if (*p >= 'a' && *p <= 'f')
		b = *p - 'a' + 0xa;
	else if (*p >= 'A' && *p <= 'F')
		b = *p - 'A' + 0xa;
	else if (*p >= '0' && *p <= '9')
		b = *p - '0';
	// else error!
	return b;
}


static inline wxByte *find_number_end(wxByte *str, wxByte *end, bool *pdecimal)
{
	wxByte *p = str;
	bool num = false;
	while (p < end)
	{
		if (*p == '-' || *p == '+')
		{
			// can only exist at the beginning
			if (p != str)
				return NULL;
		}
		else if (*p == '.')
			*pdecimal = true;
		else if (*p >= '0' && *p <= '9')
		{
			num = true;
		}
		else
			break;
		p++;
	}
	if (num)
		return p;
	return NULL;
}


bool pdf::DissectData(wxTreeItemId &parent, pdfIndirect *pObj)
{
	wxString str;
	wxByte *beg = pObj->m_data;
	wxByte *end = pObj->m_data + pObj->m_datalen;
	wxByte *p_token, *p = beg;
	wxByte *base = m_file->GetBaseAddress();
	
	// XXX: TODO: FIX/REMOVE THIS BIG NASTY FKN HACK.
	if (pObj->m_obj)
	{
		// can only really be an Integer for now
		pdfInteger *pInt = (pdfInteger *)pObj->m_obj;
#if 0
		// move the item data to the new node =)
		fdTIData *pTID = (fdTIData *)m_tree->GetItemData(pInt->m_id);
		m_tree->SetItemData(pInt->m_id, 0);
		wxTreeItemId id = m_tree->AppendItem(parent, wxString::Format(wxT("%u"), pInt->m_value), -1, -1, pTID);
#else
		// just create a new item data (probably faster than moving it...)
		wxTreeItemId id = m_tree->AppendItem(parent, wxString::Format(wxT("%u"), pInt->m_value), -1, -1, 
			new fdTIData(pInt->m_ptr - base, pInt->m_len));
#endif
		m_tree->Delete(pInt->m_id);
		pInt->m_id = id;
		return true;
	}

	// for tracking which container we're in
	wxTreeItemId new_node;
	pdfObjectList conts;
	pdfObjectBase *cur_cont = new pdfObjectBase(parent, pObj->m_ptr);
	pdfObjectBase *cur_obj;
	bool got_value;

	while (p < end)
	{
		cur_obj = NULL;
		got_value = false;

		// skip any whitespace
		while (p < end && memchr(PDF_WHITESPACE_CHARS, *p, PDF_WHITESPACE_CHARSLEN))
			p++;

		// don't go past the end!
		if (p > end)
			throw "OMG PAST END!";
		if (p == end)
			break;

		// point the current token start pointer at p
		p_token = p;

		// how we process this token depends on the first character...
		switch (*p)
		{
		case '<':
			// see if its a dictionary start token...
			if (p < end - 1 && *(p + 1) == '<')
			{
				// dictionary!
				p += 2;

				new_node = m_tree->AppendItem(cur_cont->m_id, wxT("Dictionary")); // we'll add item data later..

				// push the current container since we're becoming a new one
				conts.push_back(cur_cont);
				cur_cont = new pdfDictionary(new_node, p - 2);
				continue; // don't need to post-process this token
			}
			else
			{
				// hexadecimal string object
				p++;
				p_token = p;
				str = wxT("");
				int b = -1;
				// need to put the content somewhere...
				while (p < end && !got_value)
				{
					if (*p == '>')
					{
						// contents have been obtained!
						p++;
						got_value = true;
						break;
					}

					if ((*p >= 'A' && *p <= 'F') ||
						(*p >= 'a' && *p <= 'f') ||
						(*p >= '0' && *p <= '9'))
					{
						if (b == -1)
							b = decode_hex_nibble(p) << 4;
						else
						{
							b |= decode_hex_nibble(p);
							str += b;
							b = -1;
						}
					}

					// what should we do when we get something invalid?
					// -- we'll ignore for now...
#if 0
					if (!memchr(PDF_WHITESPACE_CHARS, *p, PDF_WHITESPACE_CHARSLEN))
						// ??
#endif
					p++;
				}

				// woot. we got it.
				if (!got_value)
				{
					wxLogError(wxT("%s(%u %u): Unterminated hexadecimal string encountered!"), wxT("DissectData"), pObj->m_number, pObj->m_generation);
					continue; // don't need to post-process this token
				}
				else
				{
					// we might have half a byte left over..
					if (b != -1)
						str += b;

					size_t len = 0;
					if (p > p_token)
						len = p - 1 - p_token;

					// we'll put the raw representation in the tree
					// XXX: it would be nice to put a sanitized version of the decoded string instead
					new_node = m_tree->AppendItem(cur_cont->m_id, wxString::Format(wxT("Hex String: %s"), 
						wxString::From8BitData((const char *)p_token, len)), -1, -1,
						new fdTIData(p_token - base, len));

					// here we store the original value in ->m_value and the decoded stuff in ->m_decoded
					pdfHexString *pHex = new pdfHexString(new_node, p_token, len);
					cur_obj = pHex;
					pHex->m_decoded = str;
				}
			}
			break;

		case '>':
			if (p < end - 1 && *(p + 1) == '>')
			{
				// end of dictionary!
				// XXX: should we check that the end came at a good time?
				p += 2;
				// set the array node length and item data
				cur_cont->m_len = p - cur_cont->m_ptr;
				cur_obj = cur_cont;
				m_tree->SetItemData(cur_cont->m_id, new fdTIData(cur_cont->m_ptr - base, cur_cont->m_len));
				// pop out of thise container
				cur_cont = conts.back();
				conts.pop_back();
				got_value = true;
			}
			else
			{
				// the > should be consumed by the handling for '<'
				wxLogError(wxT("%s(%u %u): Encountered spurious '>'"), wxT("DissectData"), pObj->m_number, pObj->m_generation);
				p++;
				continue; // don't need to post-process this token
			}
			break;

		case '(':
			// literal string object
			p++;
			if (p < end)
			{
				p_token = p;
				int paren_count = 1;
				while (p < end && paren_count > 0)
				{
					if (*p == '(')
						paren_count++;
					else if (*p == ')')
						paren_count--;
					// XXX: TODO: Implement '\\' processing further
					else if (*p == '\\')
					{
						p++;
						if (p == end)
							break;
					}
					p++;
				}

				if (paren_count)
					wxLogError(wxT("%s(%u %u): Unterminated literal string encountered!"), wxT("DissectData"), pObj->m_number, pObj->m_generation);
				else
				{
					size_t len = 0;
					if (p > p_token)
						len = p - 1 - p_token;

					pdfLiteral *pLit = new pdfLiteral(new_node, p_token, len); // wrong node for now
					cur_obj = pLit;
					new_node = m_tree->AppendItem(cur_cont->m_id, wxString::Format(wxT("Literal String: %s"), pLit->m_value.c_str()), -1, -1, 
						new fdTIData(p_token - base, len));
					cur_obj->m_id = new_node;
					got_value = true;
				}
			}
			break;

		case ')':
			// the ) should be consumed by the handling for '('
			wxLogError(wxT("%s(%u %u): Encountered spurious ')'"), wxT("DissectData"), pObj->m_number, pObj->m_generation);
			p++;
			continue; // don't need to post-process this token
			break;

		case '%':
			// comment, just skip the stuff (don't even dissect)
			while (p < end && *p != 0x0a && *p != 0x0d)
				p++;
			continue; // don't need to post-process this token
			break;

		case '/':
			// name object
			p++;
			p_token = p;
			while (p < end && !memchr(PDF_NAME_DELIM_CHARS, *p, PDF_NAME_DELIM_CHARSLEN))
				p++;
			
			if (p >= p_token)
			{
				size_t len = p - p_token;
				pdfName *pName = new pdfName(new_node, p_token, len);
				cur_obj = pName;
				new_node = m_tree->AppendItem(cur_cont->m_id, wxString::Format(wxT("Name: %s"), pName->m_value.c_str()), -1, -1, 
					new fdTIData(p_token - base, len));
				pName->m_id = new_node;
				
				// push a container for this item if we're inside a dictionary
				if (cur_cont->m_type == PDF_OBJ_DICTIONARY)
				{
					// push the current container since we're becoming a new one
					conts.push_back(cur_cont);
					cur_cont = pName;
					continue; // don't need to post-process this token
				}
				got_value = true;
			}
			break;

		case '[':
			// create a new array
			p++;
			new_node = m_tree->AppendItem(cur_cont->m_id, wxT("Array")); // don't know where the end is at this point...

			// push the current container since we're becoming a new one
			conts.push_back(cur_cont);
			cur_cont = new pdfArray(new_node, p - 1);
			continue; // don't need to post-process this token
			break;

		case ']':
			// close array
			p++;
			if (cur_cont->m_type != PDF_OBJ_ARRAY)
			{
				wxLogError(wxT("%s(%u %u): Spurious ']' encountered!"), wxT("DissectData"), pObj->m_number, pObj->m_generation);
				continue; // don't need to post-process this token
			}
			// set the array node length and item data
			cur_cont->m_len = p - cur_cont->m_ptr;
			cur_obj = cur_cont;
			m_tree->SetItemData(cur_cont->m_id, new fdTIData(cur_cont->m_ptr - base, cur_cont->m_len));
			// pop out from this container
			cur_cont = conts.back();
			conts.pop_back();
			got_value = true;
			break;

		case '{': // ??
		case '}': // ??
			p++;
			continue; // don't need to post-process this token
			break;

		default:
			// find the next delimeter
			//while (p < end && !memchr(PDF_NAME_DELIM_CHARS, *p, PDF_NAME_DELIM_CHARSLEN))
			while (p < end && !memchr(PDF_DELIMITERS_CHARS, *p, PDF_DELIMITERS_CHARSLEN))
				p++;
			break;
		}

		// if we still didn't get anything, try process the token we found
		// p_token points at the start
		// p points at the end
		size_t len = p - p_token;
		if (len < 1) // empty token?
			continue; // don't need to post-process this token

		// try to get a value from this..
		while (!got_value)
		{
			// handle stream specially
			if (len > 6 && memcmp(p_token, "stream", 6) == 0)
			{
				// skip "stream" start..
				p_token += 6;
				// XXX: might need to do file-wide line end detection and behave accordingly
				if (p_token < end && *p_token == 0x0d)
					p_token++;
				if (p_token < end && *p_token == 0x0a)
					p_token++;

				p = p_token;

				// check for a /Length
				pdfInteger *pLen = NULL;
				bool found_end = false;
				if (pObj->m_dict && cur_cont->m_type == PDF_OBJ_BASE)
				{
					// This is REQUIRED! However, if we dont get it, we'll try to detect the length..
					pLen = (pdfInteger *)pObj->m_dict->m_entries[wxT("Length")];
					if (!pLen || (pLen->m_type != PDF_OBJ_INTEGER && pLen->m_type != PDF_OBJ_REFERENCE))
					{
						wxLogWarning(wxT("%s(%u %u): Object does not contain \"Length\" key for stream!"), wxT("DissectData"), pObj->m_number, pObj->m_generation);
						pLen = NULL;
					}
					else
					{
						// resolve /Length key reference if needed
						if (pLen->m_type == PDF_OBJ_REFERENCE)
						{
							pdfReference *pRef = (pdfReference *)pLen;
							wxString key = wxString::Format(wxT("%u %u"), pRef->m_refnum, pRef->m_refgen);
							pdfObjectList *pOL = m_objects[key];
							if (pOL)
							{
								pdfIndirect *pInd = (pdfIndirect *)pOL->front();
								// maybe its there but hasn't been read in yet, lets go get it
								if (!pInd->m_obj)
								{
									if (!ReadIndirect(pInd))
										continue;
									if (!DissectData(m_root_id, pInd))
										continue;
								}
								pdfInteger *pInt = (pdfInteger *)pInd->m_obj;
								if (!pInt || pInt->m_type != PDF_OBJ_INTEGER)
								{
									wxLogError(wxT("%s(%u %u): Object \"Length\" key reference does not point to an integer!"), wxT("DissectData"), pObj->m_number, pObj->m_generation);
									// probably the wrong error handling here
									continue;
								}
								len = pInt->m_value;
							}
							else
							{
								wxLogError(wxT("%s(%u %u): Object \"Length\" key reference points to non-existant object!"), wxT("DissectData"), pObj->m_number, pObj->m_generation);
								// probably the wrong error handling here
								continue;
							}
						}
						else
							len = pLen->m_value;

						if (p >= (end - 9)
							|| len >= (size_t)((end - 9) - p))
						{
							wxLogError(wxT("%s(%u %u): Object \"Length\" key is greater than available data!"), wxT("DissectData"), pObj->m_number, pObj->m_generation);
							// XXX: probably wrong handling of error here.
							continue; // don't need to post-process this token
						}

						p = p_token + len;
						if (p < end && *p == 0x0d)
							p++;
						if (p < end && *p == 0x0a)
							p++;
						found_end = true;
					}
				}

				// detect endstream if we dont already have it
				// NOTE: this is rather error prone due possible "endstream" inside stream data
				while (!found_end)
				{
					while (p < end - 9 && *p != 'e')
						p++;
					if (p >= end - 9)
						break;
					else if (memcmp(p, "endstream", 9) == 0)
					{
						// XXX: this isn't a perfect fix, but it should work ok most of the time.
						if (*(p - 1) == 0x0a || *(p - 1) == 0x0d)
							p--;
						len = p - p_token;
						found_end = true;
					}
					else
						p++;
				}

				// if we still didn't find it, we dont have it..
				if (!found_end)
				{
					wxLogError(wxT("%s(%u %u): Unterminated stream encountered!"), wxT("DissectData"), pObj->m_number, pObj->m_generation);
					continue; // don't need to post-process this token
				}
				else if (!pLen)
					wxLogWarning(wxT("%s(%u %u): Detected stream length to be %u bytes!"), wxT("DissectData"), pObj->m_number, pObj->m_generation, len);

				pdfStream *pStm = new pdfStream(new_node, p_token, len);
				cur_obj = pStm;
				new_node = m_tree->AppendItem(cur_cont->m_id, wxT("Stream Data"), -1, -1, new fdTIData(p_token - base, len));
				pStm->m_id = new_node;

				p += 9; // skip "endstream"

				// got it, no need to loop
				got_value = true;
				break;
			}

			// "null" and "true" boolen object
			if (len >= 4)
			{
				got_value = true;
				if (memcmp(p_token, "null", 4) == 0)
				{
					new_node = m_tree->AppendItem(cur_cont->m_id, wxT("null"), -1, -1, new fdTIData(p_token - base, 4));
					cur_obj = new pdfNull(new_node, p_token, 4);
				}
				else if (memcmp(p_token, "true", 4) == 0)
				{
					new_node = m_tree->AppendItem(cur_cont->m_id, wxT("Boolean: True"), -1, -1, new fdTIData(p_token - base, 4));
					pdfBoolean *pBool = new pdfBoolean(new_node, p_token, 4);
					pBool->m_value = true;
					cur_obj = pBool;
				}
				else
					got_value = false;

				if (got_value)
					break;
			}

			// how about false?
			if (len >= 5 && memcmp(p_token, "false", 5) == 0)
			{
				m_tree->AppendItem(cur_cont->m_id, wxT("Boolean: False"), -1, -1, new fdTIData(p_token - base, 5));
				pdfBoolean *pBool = new pdfBoolean(new_node, p_token, 4);
				pBool->m_value = false;
				cur_obj = pBool;
				got_value = true;
				break;
			}

			// see if its a real number, because...
			bool is_real = false;
			wxByte *p_numend = find_number_end(p_token, end, &is_real);
			wxByte *p_num2end = NULL;
			if (p_numend)
			{
				p = p_numend; // we'll at least end up pointing after this number

				// reals cant be part of object references.
				if (is_real)
				{
					double d = strtod((char *)p_token, NULL);
					size_t len2 = p - p_token;
					new_node = m_tree->AppendItem(cur_cont->m_id, wxString::Format(wxT("Real: %g"), d), -1, -1, new fdTIData(p_token - base, len2));
					pdfReal *pReal = new pdfReal(new_node, p_token, len2);
					pReal->m_value = d;
					cur_obj = pReal;
					got_value = true;
					break;
				}

				// we know we have a number in bounds now, decode this one
				long num1 = strtol((char *)p_token, NULL, 10);

				// we if have a space following, we might have an object reference
				if (p_numend < end && *p_numend == ' ')
				{
					p_num2end = find_number_end(p_numend + 1, end, &is_real);
					if (p_num2end 
						&& !is_real 
						&& p_num2end < end - 1 
						&& *p_num2end == ' ' 
						&& *(p_num2end + 1) == 'R')
					{
						// we most certainly found an indirect object reference!
						p = p_num2end + 2;
						len = p - p_token;
						str = wxString::From8BitData((const char *)p_token, len);
						new_node = m_tree->AppendItem(cur_cont->m_id, wxString::Format(wxT("Reference: %s"), str.c_str()), -1, -1, 
							new fdTIData(p_token - base, len));
						pdfReference *pRef = new pdfReference(new_node, p_token, len);
						pRef->m_refnum = num1;
						pRef->m_refgen = strtol((char *)p_numend + 1, NULL, 10);
						cur_obj = pRef;
						got_value = true;
						break;
					}
				}

				// otherwise, we just process the first number
				len = p - p_token;
				new_node = m_tree->AppendItem(cur_cont->m_id, wxString::Format(wxT("Integer: %ld"), num1), -1, -1, new fdTIData(p_token - base, len));
				pdfInteger *pInt = new pdfInteger(new_node, p_token, len);
				pInt->m_value = num1;
				cur_obj = pInt;
				got_value = true;
				break;
			}

			// regardless if we got a value, we break anyway, noop looping in this loop.
			break;
		}

		// still didn't get anything??
		if (!got_value)
		{
			// if we got all the way down here, who knows what we have... just error about it.
			str = wxString::From8BitData((const char *)p_token, len);
			wxLogWarning(wxT("%s(%u %u): Unknown token found: %s"), wxT("DissectData"), pObj->m_number, pObj->m_generation, str.c_str());
			continue; // don't need to post-process this token
		}

		// so, we got a value...

		if (!cur_obj)
		{
			got_value = false;
			break;
		}

		// add the cur_obj to the container, if we have one
		switch (cur_cont->m_type)
		{
		case PDF_OBJ_DICTIONARY:
			wxLogError(wxT("%s(%u %u): Invalid attempt to use a non-name as a dictionary key!"), wxT("DissectData"), pObj->m_number, pObj->m_generation);
			break;

		case PDF_OBJ_ARRAY:
			{
				pdfArray *pCont = (pdfArray *)cur_cont;
				pCont->m_list.Append(cur_obj);
			}
			break;

		case PDF_OBJ_NAME:
			// if we are in a name container, and got the value already, we need to pop out
			{
				pdfName *pName = (pdfName *)cur_cont;
				cur_cont = conts.back();
				conts.pop_back();

				if (cur_cont->m_type == PDF_OBJ_DICTIONARY)
				{
#if 0
					// this is #ifdef'd out since it can never happen..
					if (pName->m_type != PDF_OBJ_NAME)
						wxLogError(wxT("%s(%u %u): Dictionary erroneously contains a non-name entry!"), wxT("DissectData"), pObj->m_number, pObj->m_generation);
#endif
					pdfDictionary *pDict = (pdfDictionary *)cur_cont;
					if (pDict->m_entries[pName->m_value])
						pDict->m_entries.erase(pName->m_value);
					pDict->m_entries[pName->m_value] = cur_obj;
					delete pName;
				}
				else
					wxLogError(wxT("%s(%u %u): A name was treated as a container when not inside a dictionary!"), wxT("DissectData"), pObj->m_number, pObj->m_generation);
			}
			// We do nothing extra for names that were added as a container, since
			// they will be handled when the value is obtained.
			// IN FACT, we don't even get here for those (they use "continue" waaay above)
			break;

		case PDF_OBJ_BASE:
			// we add the final element to passed-in object's dictionary
			if (cur_obj->m_type == PDF_OBJ_DICTIONARY)
			{
				if (pObj->m_dict)
					wxLogError(wxT("%s(%u %u): Indirect object has multiple dictionaries!?"), wxT("DissectData"), pObj->m_number, pObj->m_generation);
				else
					pObj->m_dict = (pdfDictionary *)cur_obj;
			}
			else if (cur_obj->m_type == PDF_OBJ_STREAM)
			{
				if (pObj->m_stream)
					wxLogError(wxT("%s(%u %u): Indirect object has multiple streams!?"), wxT("DissectData"), pObj->m_number, pObj->m_generation);
				else
					pObj->m_stream = (pdfStream *)cur_obj;
			}
			else
			{
#if 0
				wxLogWarning(wxT("%s(%u %u): Final item was not a dictionary or stream (type: 0x%02x, object: %u %u, offset: 0x%x)!"), wxT("DissectData"), pObj->m_number, pObj->m_generation, 
					cur_obj->m_type, pObj->m_number, pObj->m_generation, pObj->m_offset);
#endif
				if (pObj->m_obj)
					wxLogError(wxT("%s(%u %u): Indirect object has multiple objects with no container!?"), wxT("DissectData"), pObj->m_number, pObj->m_generation);
				else
					pObj->m_obj = cur_obj;
			}
			break;

		default:
			wxLogError(wxT("%s(%u %u): Unable to add object to an unknown container type!"), wxT("DissectData"), pObj->m_number, pObj->m_generation);
			break;
		}
	}

	if (cur_cont && cur_cont->m_type == PDF_OBJ_BASE)
		delete cur_cont;

	if (conts.size() > 0)
	{
		wxLogWarning(wxT("%s(%u %u): Containers remain on the container stack!"), wxT("DissectData"), pObj->m_number, pObj->m_generation);
		while (conts.size() > 0)
		{
			cur_cont = conts.back();
			conts.pop_back();
			delete cur_cont;
		}
		return false;
	}

	return true;
}


bool pdf::DissectStream(pdfIndirect *pObj)
{
	pdfStream *pStm = pObj->m_stream;

	// decode stream data
	wxInputStream *p_in = new wxMemoryInputStream((const char *)pStm->m_ptr, pStm->m_len);

	// NOTE: XRefStm and XRef objects have already been processed before,
	// but we process them again for completeness. They only real down side 
	// here is that they end up occurring multiple times in the tree.
	// figure out how to decode

	// This object must have a dictionary in order to have any filters or predictors, etc
	if (pObj->m_dict)
	{
		// XXX: TODO: support an array of filters / decode parms dictionarys

		// See if we have Decode Parameters
		pdfDictionary *pDP = (pdfDictionary *)pObj->m_dict->m_entries[wxT("DecodeParms")];
		if (!pDP)
			pDP = (pdfDictionary *)pObj->m_dict->m_entries[wxT("DP")];
		if (pDP && pDP->m_type != PDF_OBJ_DICTIONARY)
		{
			wxLogError(wxT("%s: Stream dictionary \"DecodeParms\" key was not a dictionary @ 0x%x!"), wxT("DissectStream"), pObj->m_offset);
			delete pObj;
			return false;
		}

		// See if we have any filters set
		pdfName *pFilter = (pdfName *)pObj->m_dict->m_entries[wxT("Filter")];
		if (pFilter)
		{
			if (pFilter->m_type != PDF_OBJ_NAME)
			{
				wxLogError(wxT("%s: Stream dictionary \"Filter\" key was not a name @ 0x%x!"), wxT("DissectStream"), pObj->m_offset);
				delete pObj;
				return false;
			}

			// flate decoded?
			if (pFilter->m_value == wxT("FlateDecode"))
				p_in = new wxZlibInputStream(p_in, wxZLIB_ZLIB);
		}

		// If we have a predictor set, we need to pipe the stream data through the proper predictor
		if (pDP)
		{
			pdfInteger *pCols = (pdfInteger *)pDP->m_entries[wxT("Columns")];
			if (!pCols || pCols->m_type != PDF_OBJ_INTEGER)
			{
				wxLogError(wxT("%s: Xref stream dictionary \"DecodeParms\" key has illegal \"Columns\" key @ 0x%x!"), wxT("DissectStream"), pObj->m_offset);
				delete pObj;
				return false;
			}

			pdfInteger *pPredictor = (pdfInteger *)pDP->m_entries[wxT("Predictor")];
			if (!pPredictor || pPredictor->m_type != PDF_OBJ_INTEGER)
			{
				wxLogError(wxT("%s: Xref stream dictionary \"DecodeParms\" key has illegal \"Predictor\" key @ 0x%x!"), wxT("DissectStream"), pObj->m_offset);
				delete pObj;
				return false;
			}

			if (pPredictor->m_value != 0x0c)
				wxLogWarning(wxT("%s(%u %u): Unsupported predictor 0x%x!"), wxT("DissectStream"), pObj->m_number, pObj->m_generation, pPredictor->m_value);

			p_in = new pdfPredInputStream(p_in, pPredictor->m_value, pCols->m_value);
		}
	}

	// read all the data into the memory output stream for this stream
	p_in->Read(pStm->m_decoded);
	m_tree->AppendItem(pStm->m_id, wxString::Format(wxT("Length: %u"), pStm->m_decoded.GetLength()));

	// we wont do it this way, since its uber slow.
#if 0
	// now we have an auto-magic stack of stream filters that will give us raw data when we read.
	pStm->m_decoded.Empty();
	while (!p_in->Eof())
	{
		wxByte byte;
		p_in->Read(&byte, 1);
		pStm->m_decoded += byte;
	}

	// we don't have an offset for this because its the result of uber decode
	m_tree->AppendItem(pStm->m_id, pStm->m_decoded);
#endif

	delete p_in;
	return true;
}


static wxByte *read_line_fwd(wxByte *str, size_t len, wxByte **ptr, size_t *plen)
{
	wxByte *p = *ptr;
	wxByte *e = str + len;
	wxByte *ret = NULL;

	while (*p != 0x0a && *p != 0x0d)
	{
		// if p gets to the end, return NULL
		if (p >= e)
			return NULL;
		p++;
	}

	// return pointer to beginning
	ret = *ptr;

	// nul terminate at first found newline / cr
	*plen = p - ret;

	// eat following whitespace
	while (p < (e - 1) && ((p[1] == 0x0a) || (p[1] == 0x0d)))
		p++;

	// adjust older pointer to point past nul (or at end)
	if (p < (e - 1))
		p++;
	*ptr = p;

	return ret;
}

static bool parse_two_integers(wxByte *str, size_t len,
	unsigned long *pone, wxByte **p1s, wxByte **p1e,
	unsigned long *ptwo, wxByte **p2s, wxByte **p2e)
{
	wxByte *p_first = str;
	wxByte *p_1end, *p_2nd, *p_2end;
	wxByte *end = str + len;
	unsigned long one, two;
	char *ep, *ep2;

	// try to get the first number
	bool decimal = false;
	p_1end = find_number_end(p_first, end, &decimal);
	if (!p_1end || decimal) // we don't want numbers with decimal places
		return false;

	one = strtoul((char *)p_first, &ep, 10);
	if ((wxByte *)ep != p_1end // number ending difference?
		|| *ep != ' ') // wasn't "X Y" ??
		return false;

	// try to find the end of the second number
	decimal = false;
	p_2nd = p_1end + 1;
	p_2end = find_number_end(p_2nd, end, &decimal);
	if (!p_2end || decimal) // we don't want numbers with decimal places
		return false;

	two = strtoul(ep + 1, &ep2, 10);
	if ((wxByte *)ep2 != p_2end // number ending difference?
		|| (*ep2 != ' ' && *ep2 != 0x0a && *ep2 != 0x0d))
		return false; // no whitespace after 2nd number?

	if (pone)
		*pone = one;
	if (p1s)
		*p1s = p_first;
	if (p1e)
		*p1e = p_1end;

	if (ptwo)
		*ptwo = two;
	if (p2s)
		*p2s = p_2nd;
	if (p2e)
		*p2e = p_2end;
	return true;
}


// declare the exported function
DECLARE_FD_PLUGIN(pdf)
