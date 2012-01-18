/*
 * Microsoft Office Excel Binary File Format implementation
 * Joshua J. Drake <jdrake idefense.com>
 * 
 * Workbook.h:
 * class definition for Workbook cbffStreamPlugin class
 */

#include "Workbook.h"


Workbook::Workbook(wxLog *plog, fileDissectTreeCtrl *tree)
{
	m_log = plog;
	wxLog::SetActiveTarget(m_log);
	m_tree = tree;
	
	m_description = wxT("Workbook Stream Dissector");
	m_cur = m_end = 0;
}


void Workbook::MarkDesiredStreams(void)
{
	wxLog::SetActiveTarget(m_log);

	for (cbffStreamList::iterator i = m_streams->begin();
		i != m_streams->end();
		i++)
	{
		cbffStream *p = (cbffStream *)(*i);
		if (p->m_name.Matches(wxT("Workbook")) || p->m_name.Matches(wxT("Book")))
		{
			// wxLogMessage(wxT("%s: flagging %s"), wxT("summInfo::MarkDesiredStream()"), p->m_name);
			p->m_wanted = true;
		}
	}
}


void Workbook::Dissect(void)
{
	// still need to visit all streams to know if we should actually dissect
	for (cbffStreamList::iterator i = m_streams->begin();
		i != m_streams->end();
		i++)
	{
		cbffStream *p = (cbffStream *)(*i);

		if (p->m_name.Matches(wxT("Workbook")) || p->m_name.Matches(wxT("Book")))
		{
			// wxLogMessage(wxT("%s: dissecting %s"), wxT("summInfo::MarkDesiredStream()"), p->m_name);
			DissectStream(p);
		}
	}
}


void Workbook::DissectStream(cbffStream *pStream)
{
	// erm, wtf?
	if (!pStream->m_id)
		return;

	wxFileOffset off;

	m_cur = pStream->m_data;
	m_end = pStream->m_data + pStream->m_length;

	wxNodeStack bof_nodes;
	wxTreeItemId &cur_node = pStream->m_id;
	struct WorkbookRecord *prec;
	while ((prec = GetNextRecord()))
	{
		// convert stream offset of this record to a file offset
		off = pStream->GetFileOffset((BYTE *)prec - pStream->m_data);

		// add this line no matter what
		wxTreeItemId new_node = m_tree->AppendItem(pStream->m_id, 
			wxString::Format(wxT("Record 0x%04x (length 0x%04x): %s: %s"), 
			prec->uNumber, prec->uLength, HumanReadableRecordTypeShort(prec->uNumber), 
			HumanReadableRecordTypeLong(prec->uNumber)), -1, -1,
			new fdTIData(off, sizeof(struct WorkbookRecord) + prec->uLength));

		// handle further processing
		switch (prec->uNumber)
		{
			// special case, do special stuff
			case WBOOK_RT_BOF2:
			case WBOOK_RT_BOF3:
			case WBOOK_RT_BOF4:
			case WBOOK_RT_BOF578:
				bof_nodes.push(cur_node);
				cur_node = new_node;

				AddBOFContents(pStream, new_node, prec);
				// add a children node and put them children inside it
				cur_node = m_tree->AppendItem(cur_node, wxT("Children"));
				break;

			// another special one!
			case WBOOK_RT_EOF:
				// don't do anything special with a stray EOF
				if (bof_nodes.size() < 1)
					wxLogWarning(wxT("%s: Stray EOF record encountered"), wxT("Workbook::AddToTree()"));
				else
				{
					cur_node = bof_nodes.top();
					bof_nodes.pop();
				}
				// EOF should have no data
				if (prec->uLength != 0)
					wxLogWarning(wxT("%s: EOF erroneously has data (0x%04x bytes)!"), wxT("Workbook::AddToTree()"), prec->uLength);
				break;

			case WBOOK_RT_FORMAT:
				AddFORMATContents(pStream, new_node, prec);
				break;

			case WBOOK_RT_INTERFACEHDR:
			case WBOOK_RT_CODEPAGE:
				AddINTERFACEHDRContents(pStream, new_node, prec);
				break;

			case WBOOK_RT_MMS:
				AddMMSContents(pStream, new_node, prec);
				break;

			case WBOOK_RT_WRITEACCESS:
				AddWRITEACCESSContents(pStream, new_node, prec);
				break;

			case WBOOK_RT_BOUNDSHEET:
				AddBOUNDSHEETContents(pStream, new_node, prec);
				break;

			case WBOOK_RT_UFONT:
			case WBOOK_RT_FONT:
				AddFONTContents(pStream, new_node, prec);
				break;

			case WBOOK_RT_LABELSST:
				AddLABELSSTContents(pStream, new_node, prec);
				break;

			case WBOOK_RT_SST:
				AddSSTContents(pStream, new_node, prec);
				break;

			case WBOOK_RT_EXTSST:
				AddEXTSSTContents(pStream, new_node, prec);
				break;

			case WBOOK_RT_HEADER:
				AddHEADERContents(pStream, new_node, prec);
				break;

			case WBOOK_RT_MULBLANK:
				AddMULBLANKContents(pStream, new_node, prec);
				break;

			case WBOOK_RT_MULRK:
				AddMULRKContents(pStream, new_node, prec);
				break;

			case WBOOK_RT_RK:
			case WBOOK_RT_RK3:
				AddRKContents(pStream, new_node, prec);
				break;

			// might as well not have a default
			default:
				break;
		}
	}
}


struct WorkbookRecord *Workbook::GetNextRecord(void)
{
	struct WorkbookRecord *prec;

	// if we're pointing at the end, just return NULL without logging
	if (m_cur == m_end)
		return (struct WorkbookRecord *)NULL;

	// make sure we have enough for a record
	if ((size_t)(m_end - m_cur) < sizeof(struct WorkbookRecord))
	{
		wxLogError(wxT("%s: Not enough data left for record header!"), wxT("Workbook::GetNextRecord()"));
		return (struct WorkbookRecord *)NULL;
	}
	prec = (struct WorkbookRecord *)m_cur;

	// make sure the data is in bounds as well
	if (m_end - (m_cur + sizeof(struct WorkbookRecord)) < prec->uLength)
	{
		wxLogError(wxT("%s: Not enough data left for record number 0x%04x, length 0x%04x!"), wxT("Workbook::GetNextRecord()"), prec->uNumber, prec->uLength);
		return (struct WorkbookRecord *)NULL;
	}

	// all good, advance the cur ptr and return it
	m_cur += sizeof(struct WorkbookRecord) + prec->uLength;
	return prec;
}


void Workbook::AddBOFContents(cbffStream *pStream, wxTreeItemId &parent, struct WorkbookRecord *prec)
{
	// validate we have enough data
	if (prec->uLength < sizeof(struct WorkbookBOFRecord))
	{
		wxLogError(wxT("%s: Not enough data for a BOF record (0x%04x)!"), wxT("Workbook::AddBOFContents()"), prec->uLength);
		return;
	}

	// get the file offset for the beginning of this record
	wxFileOffset off = pStream->GetFileOffset((BYTE *)(prec + 1) - pStream->m_data);

	// add the BOF details
	struct WorkbookBOFRecord *pBOF = (struct WorkbookBOFRecord *)(prec + 1);
	m_tree->AppendItem(parent, wxString::Format(wxT("Version: 0x%04x"), pBOF->_uvers), -1, -1, 
		new fdTIData(off + FDT_OFFSET_OF(_uvers, (*pBOF)), FDT_SIZE_OF(_uvers, (*pBOF))));
	m_tree->AppendItem(parent, wxString::Format(wxT("Type: 0x%04x (%s)"), pBOF->_udt, HumanReadableBOFType(pBOF->_udt)), -1, -1,
		new fdTIData(off + FDT_OFFSET_OF(_udt, (*pBOF)), FDT_SIZE_OF(_udt, (*pBOF))));
	m_tree->AppendItem(parent, wxString::Format(wxT("Build: 0x%04x"), pBOF->_urupBuild), -1, -1,
		new fdTIData(off + FDT_OFFSET_OF(_urupBuild, (*pBOF)), FDT_SIZE_OF(_urupBuild, (*pBOF))));
	m_tree->AppendItem(parent, wxString::Format(wxT("Build year: 0x%04x"), pBOF->_urupYear), -1, -1,
		new fdTIData(off + FDT_OFFSET_OF(_urupYear, (*pBOF)), FDT_SIZE_OF(_urupYear, (*pBOF))));

	/* these two are not available pre-BIFF8 */
	// XXX: add string representation
	m_tree->AppendItem(parent, wxString::Format(wxT("File history flags: 0x%08x"), pBOF->_ulbfh), -1, -1,
		new fdTIData(off + FDT_OFFSET_OF(_ulbfh, (*pBOF)), FDT_SIZE_OF(_ulbfh, (*pBOF))));
	m_tree->AppendItem(parent, wxString::Format(wxT("Lowest version: 0x%08x"), pBOF->_ulsfo), -1, -1,
		new fdTIData(off + FDT_OFFSET_OF(_ulsfo, (*pBOF)), FDT_SIZE_OF(_ulsfo, (*pBOF))));

	// XXX: support BIFF5/BIFF7 (and perhaps earlier)
}


void Workbook::AddFORMATContents(cbffStream *pStream, wxTreeItemId &parent, struct WorkbookRecord *prec)
{
	size_t len;

	// validate we have enough for the header at least
	if (prec->uLength < sizeof(struct WorkbookFORMATRecord))
	{
		wxLogError(wxT("%s: Not enough data for a format record (0x%04x)!"), wxT("Workbook::AddFORMATContents()"), prec->uLength);
		return;
	}

	// get the file offset for the beginning of this record
	wxFileOffset off = pStream->GetFileOffset((BYTE *)(prec + 1) - pStream->m_data);

	// add the FORMAT details
	struct WorkbookFORMATRecord *pFMT = (struct WorkbookFORMATRecord *)(prec + 1);
	m_tree->AppendItem(parent, wxString::Format(wxT("Format index code: 0x%04x"), pFMT->ifmt), -1, -1, 
		new fdTIData(off + FDT_OFFSET_OF(ifmt, (*pFMT)), FDT_SIZE_OF(ifmt, (*pFMT))));
	m_tree->AppendItem(parent, wxString::Format(wxT("Length of string: 0x%04x"), pFMT->cch), -1, -1, 
		new fdTIData(off + FDT_OFFSET_OF(cch, (*pFMT)), FDT_SIZE_OF(cch, (*pFMT))));
	m_tree->AppendItem(parent, wxString::Format(wxT("Option Flags: 0x%02x"), pFMT->grbit), -1, -1, 
		new fdTIData(off + FDT_OFFSET_OF(grbit, (*pFMT)), FDT_SIZE_OF(grbit, (*pFMT))));

	wxString strValue = wxT("");
	if (pFMT->cch > prec->uLength - sizeof(struct WorkbookFORMATRecord))
	{
		wxLogError(wxT("%s: String length (0x%04x) is longer than the record (0x%04x)!"), wxT("Workbook::AddFORMATContents()"), pFMT->cch, prec->uLength);
		len = prec->uLength - sizeof(struct WorkbookFORMATRecord);
	}
	else
		len = pFMT->cch;
	if (len)
	{
		BYTE *pByte = (BYTE *)(prec + 1) + sizeof(struct WorkbookFORMATRecord);
		DecodeString(pByte, len, pFMT->grbit, strValue);
	}
	m_tree->AppendItem(parent, wxString::Format(wxT("Value: %s"), strValue.c_str()), -1, -1,
		new fdTIData(off + FDT_OFFSET_OF(grbit, (*pFMT)) + FDT_SIZE_OF(grbit, (*pFMT)), len));
}


void Workbook::AddHEADERContents(cbffStream *pStream, wxTreeItemId &parent, struct WorkbookRecord *prec)
{
	// if we dont have any bytes, we have no child data (valid possibility)
	if (prec->uLength < 3)
		return;

	// get the file offset for the beginning of this record
	BYTE *cch = (BYTE *)(prec + 1);
	wxFileOffset off = pStream->GetFileOffset(cch - pStream->m_data);

	// add the HEADER details
	m_tree->AppendItem(parent, wxString::Format(wxT("String length: 0x%02x"), *cch), -1, -1, new fdTIData(off, 1));

	// XXX: what happened to bytes 2 and 3 ?

	ssize_t len;
	if (*cch > prec->uLength - 3)
	{
		wxLogError(wxT("%s: String length (0x%04x) is longer than the record (0x%04x)!"), wxT("Workbook::AddHEADERContents()"), *cch, prec->uLength);
		len = prec->uLength - 3;
	}
	else
		len = *cch;

	wxString strValue = wxT("");
	if (len)
	{
		BYTE *pByte = cch + 3;
		DecodeString(pByte, len, 0, strValue);
	}
	m_tree->AppendItem(parent, wxString::Format(wxT("Value: %s"), strValue.c_str()), -1, -1, new fdTIData(off + 3, len));
}


void Workbook::AddINTERFACEHDRContents(cbffStream *pStream, wxTreeItemId &parent, struct WorkbookRecord *prec)
{
	// older versions might not have any data
	if (prec->uLength == 0)
		return;

	// if its not zero, it should be at least 2
	if (prec->uLength < 2)
	{
		wxLogError(wxT("%s: Not enough data for an interfacehdr record (0x%04x)!"), wxT("Workbook::AddINTERFACEHDRContents()"), prec->uLength);
		return;
	}

	// if we have a different amount of data, print a warning
	if (prec->uLength > 2)
		wxLogWarning(wxT("%s: Extra data bytes (0x%04x) for INTERFACEHDR record"), wxT("Workbook::AddINTERFACEHDRContents()"), prec->uLength);

	// get the file offset for the beginning of this record
	wxFileOffset off = pStream->GetFileOffset((BYTE *)(prec + 1) - pStream->m_data);

	WORD *pCodePage = (WORD *)(prec + 1);
	m_tree->AppendItem(parent, wxString::Format(wxT("Codepage: 0x%04x"), *pCodePage), -1, -1, new fdTIData(off, 2));
}


void Workbook::AddMMSContents(cbffStream *pStream, wxTreeItemId &parent, struct WorkbookRecord *prec)
{
	// XXX: create structure for this one and use it

	// if we have a different amount of data, print a warning
	if (prec->uLength != 2)
	{
		wxLogError(wxT("%s: Incorrect amount of data for MMS: ADDMENU/DELMENU record (0x%04x)!"), wxT("Workbook::AddMMSContents()"), prec->uLength);
		return;
	}

	// get the file offset for the beginning of this record
	BYTE *pByte = (BYTE *)(prec + 1);
	wxFileOffset off = pStream->GetFileOffset(pByte - pStream->m_data);

	m_tree->AppendItem(parent, wxString::Format(wxT("ADDMENU Count: 0x%02x"), *pByte), -1, -1, new fdTIData(off, 1));
	pByte++;
	m_tree->AppendItem(parent, wxString::Format(wxT("DELMENU Count: 0x%02x"), *pByte), -1, -1, new fdTIData(off + 1, 1));
}


void Workbook::AddWRITEACCESSContents(cbffStream *pStream, wxTreeItemId &parent, struct WorkbookRecord *prec)
{
	/* XXX: which version is the file? */

	/* version 8 */

	// check that we have enough data
	if (prec->uLength != sizeof(struct WorkbookWRITEACCESSRecord8))
	{
		wxLogError(wxT("%s: Incorrect amount of data for WRITEACCESS record (0x%04x)!"), wxT("Workbook::AddWRITEACCESSContents()"), prec->uLength);
		return;
	}

	// get the file offset for the beginning of this record
	wxFileOffset off = pStream->GetFileOffset((BYTE *)(prec + 1) - pStream->m_data);

	struct WorkbookWRITEACCESSRecord8 *pWA = (struct WorkbookWRITEACCESSRecord8 *)(prec + 1);
	m_tree->AppendItem(parent, wxString::Format(wxT("String length: 0x%04x"), pWA->cch), -1, -1, 
		new fdTIData(off + FDT_OFFSET_OF(cch, (*pWA)), FDT_SIZE_OF(cch, (*pWA))));
	m_tree->AppendItem(parent, wxString::Format(wxT("String options: 0x%02x"), pWA->grbit), -1, -1, 
		new fdTIData(off + FDT_OFFSET_OF(grbit, (*pWA)), FDT_SIZE_OF(grbit, (*pWA))));

	wxString str_stName = wxT("");
	BYTE *pByte = (BYTE *)pWA->stName;
	// note the cch value is ignored (fixed length string)
	DecodeString(pByte, sizeof(pWA->stName), pWA->grbit, str_stName);
	str_stName.Trim();
	m_tree->AppendItem(parent, wxString::Format(wxT("User name: %s"), str_stName.c_str()), -1, -1,
		new fdTIData(off + FDT_OFFSET_OF(stName, (*pWA)), FDT_SIZE_OF(stName, (*pWA))));

	/*
	 * TODO: support BIFF7 and earlier as follows
	 *
	{
		if (prec->uLength != sizeof(struct WorkbookWRITEACCESSRecord7))
		{
			wxLogError(wxT("%s: Incorrect amount of data for WRITEACCESS record (0x%04x)!"), wxT("Workbook::AddWRITEACCESSContents()"), prec->uLength);
			return;
		}
		struct WorkbookWRITEACCESSRecord7 *pWA;
		pWA = (struct WorkbookWRITEACCESSRecord7 *)(prec + 1);
		m_tree->AppendItem(parent, wxString::Format(wxT("String length: 0x%02x"), pWA->cch), -1, -1, ...
		BYTE *pByte = (BYTE *)pWA->stName;
		// note the cch value is ignored (fixed length string)
		// also, no flags are present -- use ANSI string
		DecodeString(pByte, sizeof(pWA->stName), 0, str_stName);
		str_stName.Trim();
		m_tree->AppendItem(child_id, wxString::Format(wxT("User name: %s"), str_stName), -1, -1, ...
	}
	 */
}


void Workbook::AddBOUNDSHEETContents(cbffStream *pStream, wxTreeItemId &parent, struct WorkbookRecord *prec)
{
	size_t len;

	// validate we have enough for the header at least
	if (prec->uLength < sizeof(struct WorkbookBOUNDSHEETRecord))
	{
		wxLogError(wxT("%s: Not enough data for BOUNDSHEET record (0x%04x)!"), wxT("Workbook::AddBOUNDSHEETContents()"), prec->uLength);
		return;
	}

	// get the file offset for the beginning of this record
	wxFileOffset off = pStream->GetFileOffset((BYTE *)(prec + 1) - pStream->m_data);

	// add the BOUNDSHEET details
	struct WorkbookBOUNDSHEETRecord *pBS = (struct WorkbookBOUNDSHEETRecord *)(prec + 1);
	m_tree->AppendItem(parent, wxString::Format(wxT("BOF Stream Position: 0x%08x"), pBS->lbPlyPos), -1, -1, 
		new fdTIData(off + FDT_OFFSET_OF(lbPlyPos, (*pBS)), FDT_SIZE_OF(lbPlyPos, (*pBS))));
	m_tree->AppendItem(parent, wxString::Format(wxT("Options flags: 0x%04x"), pBS->grbit), -1, -1, 
		new fdTIData(off + FDT_OFFSET_OF(grbit, (*pBS)), FDT_SIZE_OF(grbit, (*pBS))));
	m_tree->AppendItem(parent, wxString::Format(wxT("Length of string: 0x%02x"), pBS->cch), -1, -1, 
		new fdTIData(off + FDT_OFFSET_OF(cch, (*pBS)), FDT_SIZE_OF(cch, (*pBS))));
	m_tree->AppendItem(parent, wxString::Format(wxT("String options: 0x%02x"), pBS->rg_grbit), -1, -1, 
		new fdTIData(off + FDT_OFFSET_OF(rg_grbit, (*pBS)), FDT_SIZE_OF(rg_grbit, (*pBS))));

	// check that the length is ok
	wxString strValue = wxT("");
	if (pBS->cch > prec->uLength - sizeof(struct WorkbookBOUNDSHEETRecord))
	{
		wxLogError(wxT("%s: String length (0x%04x) is longer than the record (0x%04x)!"), wxT("Workbook::AddBOUNDSHEETContents()"), pBS->cch, prec->uLength);
		len = prec->uLength - sizeof(struct WorkbookBOUNDSHEETRecord);
	}
	else
		len = pBS->cch;

	if (len)
	{
		BYTE *pByte = (BYTE *)(prec + 1) + sizeof(struct WorkbookBOUNDSHEETRecord);
		DecodeString(pByte, len, pBS->rg_grbit, strValue);
	}
	m_tree->AppendItem(parent, wxString::Format(wxT("Value: %s"), strValue.c_str()), -1, -1, 
		new fdTIData(off + FDT_OFFSET_OF(rg_grbit, (*pBS)) + FDT_SIZE_OF(rg_grbit, (*pBS)), len));

	// extra validation -- see if the boundsheet is pointing to a BOF record
	// XXX: could also check that the pointed to BOF record fits inside
	if (pBS->lbPlyPos > pStream->m_length - sizeof(struct WorkbookRecord))
		wxLogWarning(wxT("%s: lbPlyPos (0x%08x) points outside of stream"), wxT("Workbook::AddBOUNDSHEETContents()"), pBS->lbPlyPos);
	else
	{
		prec = (struct WorkbookRecord *)(pStream->m_data + pBS->lbPlyPos);
		// note: this record didn't exist prior to BIFF5
		if (prec->uNumber != WBOOK_RT_BOF578)
			wxLogWarning(wxT("%s: lbPlyPos (0x%08x) does not point at a BOF record"), wxT("Workbook::AddBOUNDSHEETContents()"), pBS->lbPlyPos);
	}
}


void Workbook::AddFONTContents(cbffStream *pStream, wxTreeItemId &parent, struct WorkbookRecord *prec)
{
	size_t len;

	// validate we have enough for the header at least
	if (prec->uLength < sizeof(struct WorkbookFONTRecord))
	{
		wxLogError(wxT("%s: Not enough data for FONT record (0x%04x)!"), wxT("Workbook::AddFONTContents()"), prec->uLength);
		return;
	}

	// get the file offset for the beginning of this record
	wxFileOffset off = pStream->GetFileOffset((BYTE *)(prec + 1) - pStream->m_data);

	// add the BOUNDSHEET details
	struct WorkbookFONTRecord *pFONT = (struct WorkbookFONTRecord *)(prec + 1);
	m_tree->AppendItem(parent, wxString::Format(wxT("Font height: 0x%04x"), pFONT->dyHeight), -1, -1, 
		new fdTIData(off + FDT_OFFSET_OF(dyHeight, (*pFONT)), FDT_SIZE_OF(dyHeight, (*pFONT))));
	m_tree->AppendItem(parent, wxString::Format(wxT("Font attributes: 0x%04x"), pFONT->grbit), -1, -1, 
		new fdTIData(off + FDT_OFFSET_OF(grbit, (*pFONT)), FDT_SIZE_OF(grbit, (*pFONT))));
	m_tree->AppendItem(parent, wxString::Format(wxT("Color palette index: 0x%04x"), pFONT->icv), -1, -1, 
		new fdTIData(off + FDT_OFFSET_OF(icv, (*pFONT)), FDT_SIZE_OF(icv, (*pFONT))));
	m_tree->AppendItem(parent, wxString::Format(wxT("Bold style: 0x%04x"), pFONT->bls), -1, -1, 
		new fdTIData(off + FDT_OFFSET_OF(bls, (*pFONT)), FDT_SIZE_OF(bls, (*pFONT))));
	m_tree->AppendItem(parent, wxString::Format(wxT("Sub/Superscript: 0x%04x"), pFONT->sss), -1, -1, 
		new fdTIData(off + FDT_OFFSET_OF(sss, (*pFONT)), FDT_SIZE_OF(sss, (*pFONT))));
	m_tree->AppendItem(parent, wxString::Format(wxT("Underline style: 0x%02x"), pFONT->uls), -1, -1, 
		new fdTIData(off + FDT_OFFSET_OF(uls, (*pFONT)), FDT_SIZE_OF(uls, (*pFONT))));
	m_tree->AppendItem(parent, wxString::Format(wxT("Font family: 0x%02x"), pFONT->bFamily), -1, -1, 
		new fdTIData(off + FDT_OFFSET_OF(bFamily, (*pFONT)), FDT_SIZE_OF(bFamily, (*pFONT))));
	m_tree->AppendItem(parent, wxString::Format(wxT("Character set: 0x%02x"), pFONT->bCharSet), -1, -1, 
		new fdTIData(off + FDT_OFFSET_OF(bCharSet, (*pFONT)), FDT_SIZE_OF(bCharSet, (*pFONT))));
	m_tree->AppendItem(parent, wxString::Format(wxT("Reserved: 0x%02x"), pFONT->_reserved), -1, -1, 
		new fdTIData(off + FDT_OFFSET_OF(_reserved, (*pFONT)), FDT_SIZE_OF(_reserved, (*pFONT))));
	m_tree->AppendItem(parent, wxString::Format(wxT("Name length: 0x%02x"), pFONT->cch), -1, -1, 
		new fdTIData(off + FDT_OFFSET_OF(cch, (*pFONT)), FDT_SIZE_OF(cch, (*pFONT))));
	m_tree->AppendItem(parent, wxString::Format(wxT("String options: 0x%02x"), pFONT->rg_grbit), -1, -1, 
		new fdTIData(off + FDT_OFFSET_OF(rg_grbit, (*pFONT)), FDT_SIZE_OF(rg_grbit, (*pFONT))));

	// check that the length is ok
	wxString strValue = wxT("");
	if (pFONT->cch > prec->uLength - sizeof(struct WorkbookFONTRecord))
	{
		wxLogError(wxT("%s: String length (0x%04x) is longer than the record (0x%04x)!"), wxT("Workbook::AddFONTContents()"), pFONT->cch, prec->uLength);
		len = prec->uLength - sizeof(struct WorkbookFONTRecord);
	}
	else
		len = pFONT->cch;
	// if we have string data, process it
	if (len)
	{
		BYTE *pByte = (BYTE *)(prec + 1) + sizeof(struct WorkbookFONTRecord);
		DecodeString(pByte, len, pFONT->rg_grbit, strValue);
	}
	// XXX: maybe don't add tree item data if len < 1
	m_tree->AppendItem(parent, wxString::Format(wxT("Value: %s"), strValue.c_str()), -1, -1,
		new fdTIData(off + sizeof(struct WorkbookFONTRecord), len));
}


void Workbook::AddLABELSSTContents(cbffStream *pStream, wxTreeItemId &parent, struct WorkbookRecord *prec)
{
	// validate we have enough data
	if (prec->uLength < sizeof(struct WorkbookLABELSSTRecord))
	{
		wxLogError(wxT("%s: Not enough data for LABELSST record (0x%04x)!"), wxT("Workbook::AddLABELSSTContents()"), prec->uLength);
		return;
	}

	// get the file offset for the beginning of this record
	wxFileOffset off = pStream->GetFileOffset((BYTE *)(prec + 1) - pStream->m_data);

	// add the details
	struct WorkbookLABELSSTRecord *pLABELSST = (struct WorkbookLABELSSTRecord *)(prec + 1);
	m_tree->AppendItem(parent, wxString::Format(wxT("Row: 0x%04x"), pLABELSST->rw), -1, -1, 
		new fdTIData(off + FDT_OFFSET_OF(rw, (*pLABELSST)), FDT_SIZE_OF(rw, (*pLABELSST))));
	m_tree->AppendItem(parent, wxString::Format(wxT("Column: 0x%04x"), pLABELSST->col), -1, -1, 
		new fdTIData(off + FDT_OFFSET_OF(col, (*pLABELSST)), FDT_SIZE_OF(col, (*pLABELSST))));
	m_tree->AppendItem(parent, wxString::Format(wxT("XF Index: 0x%04x"), pLABELSST->ixfe), -1, -1, 
		new fdTIData(off + FDT_OFFSET_OF(ixfe, (*pLABELSST)), FDT_SIZE_OF(ixfe, (*pLABELSST))));
	m_tree->AppendItem(parent, wxString::Format(wxT("SST Index: 0x%08x"), pLABELSST->isst), -1, -1, 
		new fdTIData(off + FDT_OFFSET_OF(isst, (*pLABELSST)), FDT_SIZE_OF(isst, (*pLABELSST))));
}


void Workbook::AddSSTContents(cbffStream *pStream, wxTreeItemId &parent, struct WorkbookRecord *prec)
{
	// validate we have enough for the header at least
	if (prec->uLength < sizeof(struct WorkbookSSTRecord))
	{
		wxLogError(wxT("%s: Not enough data for SST record (0x%04x)!"), wxT("Workbook::AddSSTContents()"), prec->uLength);
		return;
	}

	// get the file offset for the beginning of this record
	wxFileOffset off = pStream->GetFileOffset((BYTE *)(prec + 1) - pStream->m_data);

	// add the structure details
	struct WorkbookSSTRecord *pSST = (struct WorkbookSSTRecord *)(prec + 1);
	m_tree->AppendItem(parent, wxString::Format(wxT("Total strings: 0x%08x"), pSST->cstTotal), -1, -1, 
		new fdTIData(off + FDT_OFFSET_OF(cstTotal, (*pSST)), FDT_SIZE_OF(cstTotal, (*pSST))));
	m_tree->AppendItem(parent, wxString::Format(wxT("Unique count: 0x%08x"), pSST->cstUnique), -1, -1, 
		new fdTIData(off + FDT_OFFSET_OF(cstTotal, (*pSST)), FDT_SIZE_OF(cstTotal, (*pSST))));

	// add a node (with details below) for each string
	DWORD i = pSST->cstUnique;
	WORD uBytesLeft = prec->uLength - sizeof(struct WorkbookSSTRecord);
	BYTE *pByte = (BYTE *)(pSST + 1);
	unsigned long cur_num = 0;
	while (i > 0)
	{
		if (uBytesLeft < sizeof(struct WorkbookWSZ))
		{
			wxLogWarning(wxT("%s: End of data looking for string header."), wxT("Workbook::AddSSTContents()"));
			return;
		}

		struct WorkbookWSZ *pWSZ = (struct WorkbookWSZ *)pByte;
		off = pByte - pStream->m_data;

		// add the parent node and the WSZ fields
		wxTreeItemId str_id = m_tree->AppendItem(parent, wxString::Format(wxT("String %lu"), cur_num), -1, -1, 
			new fdTIData(off, sizeof(*pWSZ)));
		m_tree->AppendItem(str_id, wxString::Format(wxT("String length: 0x%04x"), pWSZ->cch), -1, -1, 
			new fdTIData(off + FDT_OFFSET_OF(cch, (*pWSZ)), FDT_SIZE_OF(cch, (*pWSZ))));
		m_tree->AppendItem(str_id, wxString::Format(wxT("String options: 0x%02x"), pWSZ->grbit), -1, -1, 
			new fdTIData(off + FDT_OFFSET_OF(grbit, (*pWSZ)), FDT_SIZE_OF(grbit, (*pWSZ))));

		// skip the WSZ struct
		pByte += sizeof(struct WorkbookWSZ);
		uBytesLeft -= sizeof(struct WorkbookWSZ);

		// check that the string length is ok
		size_t len;
		len = GetStringLength(pWSZ->cch, pWSZ->grbit);
		if (len > uBytesLeft)
		{
			wxLogError(wxT("%s: String length (0x%04x) is longer than the remaining data (0x%04x)!"), wxT("Workbook::AddSSTContents()"), len, uBytesLeft);
			len = uBytesLeft;
		}

		// if we have string data, process it
		if (len)
		{
			wxString strValue = wxT("");

			DecodeString(pByte, pWSZ->cch, pWSZ->grbit, strValue);

			m_tree->AppendItem(str_id, wxString::Format(wxT("Value: %s"), strValue.c_str()), -1, -1, 
				new fdTIData(off + sizeof(struct WorkbookWSZ), len));

			pByte += len;
			uBytesLeft -= len;
		}

		i--;
		cur_num++;
	}

	if (uBytesLeft)
		wxLogWarning(wxT("%s: Extra data remains (0x%04x bytes)"), wxT("Workbook::AddSSTContents()"), uBytesLeft);
}


void Workbook::AddEXTSSTContents(cbffStream *pStream, wxTreeItemId &parent, struct WorkbookRecord *prec)
{
	// validate we have enough for the header at least
	if (prec->uLength < sizeof(struct WorkbookEXTSSTRecord))
	{
		wxLogError(wxT("%s: Insufficient data (0x%04x)!"), wxT("Workbook::AddEXTSSTContents()"), prec->uLength);
		return;
	}

	// get the file offset for the beginning of this record
	wxFileOffset off = pStream->GetFileOffset((BYTE *)(prec + 1) - pStream->m_data);

	// add the structure details
	struct WorkbookEXTSSTRecord *pXSST = (struct WorkbookEXTSSTRecord *)(prec + 1);
	m_tree->AppendItem(parent, wxString::Format(wxT("Strings per bucket: 0x%04x"), pXSST->Dsst), -1, -1, 
		new fdTIData(off + FDT_OFFSET_OF(Dsst, (*pXSST)), FDT_SIZE_OF(Dsst, (*pXSST))));
	off += sizeof(*pXSST);

	// check out the remaining data
	USHORT uBytesLeft = prec->uLength - sizeof(struct WorkbookEXTSSTRecord);
	if (uBytesLeft % sizeof(struct WorkbookEXTSSTRecord))
		wxLogWarning(wxT("%s: Remaining length (0x%04x) should be a multiple of %d."), wxT("Workbook::AddEXTSSTContents()"), uBytesLeft, sizeof(struct WorkbookEXTSSTRecord));

	// process only this many
	DWORD entries = uBytesLeft / sizeof(struct WorkbookEXTSST_ISSTINF);

	// add an entry for each item 
	// (note the number of items is stored in an SST record)
	struct WorkbookEXTSST_ISSTINF *pINF = (struct WorkbookEXTSST_ISSTINF *)(pXSST + 1);
	DWORD i = 0;
	while (i < entries)
	{
		// make sure we have enuff for the structure
		if (uBytesLeft < sizeof(struct WorkbookEXTSST_ISSTINF))
		{
			wxLogError(wxT("%s: End of data looking for structure content."), wxT("Workbook::AddEXTSSTContents()"));
			return;
		}

		// add the parent node and the structure fields fields
		wxTreeItemId isst_id = m_tree->AppendItem(parent, wxString::Format(wxT("ISSTINF %d"), i + 1), -1, -1,
			new fdTIData(off, sizeof(*pINF)));
		m_tree->AppendItem(isst_id, wxString::Format(wxT("Stream position: 0x%08x"), pINF->ib), -1, -1, 
		new fdTIData(off + FDT_OFFSET_OF(ib, (*pINF)), FDT_SIZE_OF(ib, (*pINF))));
		m_tree->AppendItem(isst_id, wxString::Format(wxT("Bucket SST offset: 0x%04x"), pINF->cb), -1, -1, 
		new fdTIData(off + FDT_OFFSET_OF(cb, (*pINF)), FDT_SIZE_OF(cb, (*pINF))));
		m_tree->AppendItem(isst_id, wxString::Format(wxT("Reserved (must be zero): 0x%04x"), pINF->_reserved), -1, -1, 
		new fdTIData(off + FDT_OFFSET_OF(_reserved, (*pINF)), FDT_SIZE_OF(_reserved, (*pINF))));

		uBytesLeft -= sizeof(struct WorkbookEXTSST_ISSTINF);
		pXSST++;
		off += sizeof(*pXSST);
		i++;
	}

	// checking for remaining bytes isn't necessary, handled above with % 8 check
}


void Workbook::AddMULBLANKContents(cbffStream *pStream, wxTreeItemId &parent, struct WorkbookRecord *prec)
{
	// validate we have enough for the header at least
	if (prec->uLength < sizeof(struct WorkbookMULBLANKRecord))
	{
		wxLogError(wxT("%s: Not enough data for a multiple blank cells record (0x%04x)!"), wxT("Workbook::AddMULBLANKContents()"), prec->uLength);
		return;
	}

	// get the file offset for the beginning of this record
	wxFileOffset off = pStream->GetFileOffset((BYTE *)(prec + 1) - pStream->m_data);

	// add the MULBLANK details
	struct WorkbookMULBLANKRecord *pMB = (struct WorkbookMULBLANKRecord *)(prec + 1);
	m_tree->AppendItem(parent, wxString::Format(wxT("Row number: 0x%04x"), pMB->rw), -1, -1, 
		new fdTIData(off + FDT_OFFSET_OF(rw, (*pMB)), FDT_SIZE_OF(rw, (*pMB))));
	m_tree->AppendItem(parent, wxString::Format(wxT("First column: 0x%04x"), pMB->colFirst), -1, -1, 
		new fdTIData(off + FDT_OFFSET_OF(colFirst, (*pMB)), FDT_SIZE_OF(colFirst, (*pMB))));

	// get the last word of the record (XXX: validate?)
	pMB->colLast = *(USHORT *)((BYTE *)pMB + prec->uLength - sizeof(pMB->colLast));
	m_tree->AppendItem(parent, wxString::Format(wxT("Last column: 0x%04x"), pMB->colLast), -1, -1, 
		new fdTIData(off + prec->uLength - sizeof(pMB->colLast), sizeof(pMB->colLast)));

	// extract the array
	USHORT bytes_left = prec->uLength - sizeof(struct WorkbookMULBLANKRecord);
	if (pMB->colFirst > pMB->colLast)
	{
		wxLogError(wxT("%s: BLANK first column (0x%04x) is greater than last column (0x%04x)!"), wxT("Workbook::AddMULBLANKContents()"), pMB->colFirst, pMB->colLast);
		return;
	}
	size_t num_ixfe = (pMB->colLast - pMB->colFirst) + 1;
	USHORT num_rec = bytes_left / sizeof(USHORT);
	if (bytes_left % sizeof(USHORT) != 0
		|| num_rec != num_ixfe)
	{
		wxLogWarning(wxT("%s: Number of BLANK records (%lu) does not match remaining byte count (0x%04x)!"), wxT("Workbook::AddMULBLANKContents()"), num_ixfe, bytes_left);
	}

	USHORT *pXFI = (USHORT *)((BYTE *)pMB + sizeof(pMB->rw) + sizeof(pMB->colFirst));
	off += sizeof(*pMB) - sizeof(pMB->colLast);
	int i;
	for (i = 0; i < num_rec; i++)
		m_tree->AppendItem(parent, wxString::Format(wxT("Column %d XF Index: 0x%04x"), i + pMB->colFirst, pXFI[i]), -1, -1,
			new fdTIData(off + (i * sizeof(*pXFI)), sizeof(*pXFI)));
}


void Workbook::AddMULRKContents(cbffStream *pStream, wxTreeItemId &parent, struct WorkbookRecord *prec)
{
	// validate we have enough for the header at least
	if (prec->uLength < sizeof(struct WorkbookMULRKRecord))
	{
		wxLogError(wxT("%s: Not enough data for a multiple RK cells record (0x%04x)!"), wxT("Workbook::AddMULRKContents()"), prec->uLength);
		return;
	}

	// get the file offset for the beginning of this record
	wxFileOffset off = pStream->GetFileOffset((BYTE *)(prec + 1) - pStream->m_data);

	// add the MULRK details
	struct WorkbookMULRKRecord *pMB = (struct WorkbookMULRKRecord *)(prec + 1);
	m_tree->AppendItem(parent, wxString::Format(wxT("Row number: 0x%04x"), pMB->rw), -1, -1, 
		new fdTIData(off + FDT_OFFSET_OF(rw, (*pMB)), FDT_SIZE_OF(rw, (*pMB))));
	m_tree->AppendItem(parent, wxString::Format(wxT("First column: 0x%04x"), pMB->colFirst), -1, -1, 
		new fdTIData(off + FDT_OFFSET_OF(colFirst, (*pMB)), FDT_SIZE_OF(colFirst, (*pMB))));

	// extract the last col from the end
	pMB->colLast = *(USHORT *)((BYTE *)pMB + prec->uLength - sizeof(pMB->colLast));
	m_tree->AppendItem(parent, wxString::Format(wxT("Last column: 0x%04x"), pMB->colLast), -1, -1, 
		new fdTIData(off + prec->uLength - sizeof(pMB->colLast), sizeof(pMB->colLast)));

	// extract the array
	USHORT bytes_left = prec->uLength - sizeof(struct WorkbookMULRKRecord);
	if (pMB->colFirst > pMB->colLast)
	{
		wxLogError(wxT("%s: RK first column (0x%04x) is greater than RK last column (0x%04x)!"), wxT("Workbook::AddMULRKContents()"), pMB->colFirst, pMB->colLast);
		return;
	}
	size_t num_rk = (pMB->colLast - pMB->colFirst) + 1;
	USHORT num_rec = bytes_left / sizeof(struct WorkbookRKREC);
	if (bytes_left % sizeof(struct WorkbookRKREC) != 0
		|| num_rec != num_rk)
	{
		wxLogWarning(wxT("%s: Number of RK records (%lu) does not match remaining byte count (0x%04x)!"), wxT("Workbook::AddMULRKContents()"), num_rk, bytes_left);
	}
	struct WorkbookRKREC *pRK = (struct WorkbookRKREC *)((BYTE *)pMB + sizeof(pMB->rw) + sizeof(pMB->colFirst));
	off += sizeof(*pMB) - sizeof(pMB->colLast);
	int i;
	for (i = 0; i < num_rec; i++)
	{
		wxTreeItemId col_node = m_tree->AppendItem(parent, wxString::Format(wxT("Column %d"), i + pMB->colFirst), -1, -1,
			new fdTIData(off, sizeof(*pRK)));
		m_tree->AppendItem(col_node, wxString::Format(wxT("XF Index: 0x%04x"), pRK[i].ixfe), -1, -1, 
			new fdTIData(off + FDT_OFFSET_OF(ixfe, (*pRK)), FDT_SIZE_OF(ixfe, (*pRK))));
		// m_tree->AppendItem(parent, wxString::Format(wxT("RK: 0x%08x"), pRK[i].RK));
		m_tree->AppendItem(col_node, wxString::Format(wxT("RK type: %s"), HumanReadableRKType(pRK[i].RK)), -1, -1, 
			new fdTIData(off + FDT_OFFSET_OF(RK, (*pRK)), FDT_SIZE_OF(RK, (*pRK))));
		m_tree->AppendItem(col_node, wxString::Format(wxT("RK value: %g"), RKDecode(pRK[i].RK)), -1, -1, 
			new fdTIData(off + FDT_OFFSET_OF(RK, (*pRK)), FDT_SIZE_OF(RK, (*pRK))));
		off += sizeof(*pRK);
	}
}


void Workbook::AddRKContents(cbffStream *pStream, wxTreeItemId &parent, struct WorkbookRecord *prec)
{
	// validate we have enough for the header at least
	if (prec->uLength < sizeof(struct WorkbookRKRecord))
	{
		wxLogError(wxT("%s: Not enough data for an RK cell record (0x%04x)!"), wxT("Workbook::AddRKContents()"), prec->uLength);
		return;
	}

	// get the file offset for the beginning of this record
	wxFileOffset off = pStream->GetFileOffset((BYTE *)(prec + 1) - pStream->m_data);

	// add the RK details
	struct WorkbookRKRecord *pRK = (struct WorkbookRKRecord *)(prec + 1);
	m_tree->AppendItem(parent, wxString::Format(wxT("Row number: 0x%04x"), pRK->rw), -1, -1, 
			new fdTIData(off + FDT_OFFSET_OF(rw, (*pRK)), FDT_SIZE_OF(rw, (*pRK))));
	m_tree->AppendItem(parent, wxString::Format(wxT("Column number: 0x%04x"), pRK->col), -1, -1, 
			new fdTIData(off + FDT_OFFSET_OF(col, (*pRK)), FDT_SIZE_OF(col, (*pRK))));
	// RK value stuff
	m_tree->AppendItem(parent, wxString::Format(wxT("XF Index: 0x%04x"), pRK->rk.ixfe), -1, -1, 
			new fdTIData(off + FDT_OFFSET_OF(rk.ixfe, (*pRK)), FDT_SIZE_OF(rk.ixfe, (*pRK))));
	// m_tree->AppendItem(parent, wxString::Format(wxT("RK: 0x%08x"), pRK->rk.RK));
	m_tree->AppendItem(parent, wxString::Format(wxT("RK type: %s"), HumanReadableRKType(pRK->rk.RK)), -1, -1, 
			new fdTIData(off + FDT_OFFSET_OF(rk.RK, (*pRK)), FDT_SIZE_OF(rk.RK, (*pRK))));
	m_tree->AppendItem(parent, wxString::Format(wxT("RK value: %g"), RKDecode(pRK->rk.RK)), -1, -1, 
			new fdTIData(off + FDT_OFFSET_OF(rk.RK, (*pRK)), FDT_SIZE_OF(rk.RK, (*pRK))));
}


wxChar *Workbook::HumanReadableBOFType(USHORT type)
{
	switch (type)
	{
		case WBOOK_BT_WBGLOBALS:
			return wxT("Workbook globals");
		case WBOOK_BT_VBMODULE:
			return wxT("Visual Basic module");
		case WBOOK_BT_SHEET:
			return wxT("Worksheet or dialog sheet");
		case WBOOK_BT_CHART:
			return wxT("Chart");
		case WBOOK_BT_XCL4MACRO:
			return wxT("Excel 4.0 macro sheet");
		case WBOOK_BT_WORKSPACE:
			return wxT("Workspace file");
		default:
			return wxT("Unknown");
	}
}


wxChar *Workbook::HumanReadableRecordTypeLong(USHORT type)
{
	switch (type)
	{
		// undocumented, added manually
		case WBOOK_RT_UFONT:
			return wxT("Unicode Font");

		// manually processed
		case WBOOK_RT_BOF2:
		case WBOOK_RT_BOF3:
		case WBOOK_RT_BOF4:
		case WBOOK_RT_BOF578:
			return wxT("Beginning of File");

		// auto-generated long normal record type strings
		case WBOOK_RT_EOF:
			return wxT("End of File");
		case WBOOK_RT_CALCCOUNT:
			return wxT("Iteration Count");
		case WBOOK_RT_CALCMODE:
			return wxT("Calculation Mode");
		case WBOOK_RT_PRECISION:
			return wxT("Precision");
		case WBOOK_RT_REFMODE:
			return wxT("Reference Mode");
		case WBOOK_RT_DELTA:
			return wxT("Iteration Increment");
		case WBOOK_RT_ITERATION:
			return wxT("Iteration Mode");
		case WBOOK_RT_PROTECT:
			return wxT("Protection Flag");
		case WBOOK_RT_PASSWORD:
			return wxT("Protection Password");
		case WBOOK_RT_HEADER:
			return wxT("Print Header on Each Page");
		case WBOOK_RT_FOOTER:
			return wxT("Print Footer on Each Page");
		case WBOOK_RT_EXTERNCOUNT:
			return wxT("Number of External References");
		case WBOOK_RT_EXTERNSHEET:
			return wxT("External Reference");
		case WBOOK_RT_WINDOWPROTECT:
			return wxT("Windows Are Protected");
		case WBOOK_RT_VERTICALPAGEBREAKS:
			return wxT("Explicit Column Page Breaks");
		case WBOOK_RT_HORIZONTALPAGEBREAKS:
			return wxT("Explicit Row Page Breaks");
		case WBOOK_RT_NOTE:
			return wxT("Comment Associated with a Cell");
		case WBOOK_RT_SELECTION:
			return wxT("Current Selection");
		case WBOOK_RT_1904:
			return wxT("1904 Date System");
		case WBOOK_RT_LEFTMARGIN:
			return wxT("Left Margin Measurement");
		case WBOOK_RT_RIGHTMARGIN:
			return wxT("Right Margin Measurement");
		case WBOOK_RT_TOPMARGIN:
			return wxT("Top Margin Measurement");
		case WBOOK_RT_BOTTOMMARGIN:
			return wxT("Bottom Margin Measurement");
		case WBOOK_RT_PRINTHEADERS:
			return wxT("Print Row/Column Labels");
		case WBOOK_RT_PRINTGRIDLINES:
			return wxT("Print Gridlines Flag");
		case WBOOK_RT_FILEPASS:
			return wxT("File Is Password-Protected");
		case WBOOK_RT_CONTINUE:
			return wxT("Continues Long Records");
		case WBOOK_RT_WINDOW1:
			return wxT("Window Information");
		case WBOOK_RT_BACKUP:
			return wxT("Save Backup Version of the File");
		case WBOOK_RT_PANE:
			return wxT("Number of Panes and Their Position");
		// dupes! (combined)
		case WBOOK_RT_CODENAME:
			return wxT("VBE Object Name / Default Code Page");
		// case WBOOK_RT_CODEPAGE:
		//	return wxT("Default Code Page");
		case WBOOK_RT_PLS:
			return wxT("Environment-Specific Print Record");
		case WBOOK_RT_DCON:
			return wxT("Data Consolidation Information");
		case WBOOK_RT_DCONREF:
			return wxT("Data Consolidation References");
		case WBOOK_RT_DCONNAME:
			return wxT("Data Consolidation Named References");
		case WBOOK_RT_DEFCOLWIDTH:
			return wxT("Default Width for Columns");
		case WBOOK_RT_XCT:
			return wxT("CRN Record Count");
		case WBOOK_RT_CRN:
			return wxT("Nonresident Operands");
		case WBOOK_RT_FILESHARING:
			return wxT("File-Sharing Information");
		case WBOOK_RT_WRITEACCESS:
			return wxT("Write Access User Name");
		case WBOOK_RT_OBJ:
			return wxT("Describes a Graphic Object");
		case WBOOK_RT_UNCALCED:
			return wxT("Recalculation Status");
		case WBOOK_RT_SAVERECALC:
			return wxT("Recalculate Before Save");
		case WBOOK_RT_TEMPLATE:
			return wxT("Workbook Is a Template");
		case WBOOK_RT_OBJPROTECT:
			return wxT("Objects Are Protected");
		case WBOOK_RT_COLINFO:
			return wxT("Column Formatting Information");
		case WBOOK_RT_RK:
		case WBOOK_RT_RK3:
			return wxT("Cell Value, RK Number");
		case WBOOK_RT_IMDATA:
			return wxT("Image Data");
		case WBOOK_RT_GUTS:
			return wxT("Size of Row and Column Gutters");
		case WBOOK_RT_WSBOOL:
			return wxT("Additional Workspace Information");
		case WBOOK_RT_GRIDSET:
			return wxT("State Change of Gridlines Option");
		case WBOOK_RT_HCENTER:
			return wxT("Center Between Horizontal Margins");
		case WBOOK_RT_VCENTER:
			return wxT("Center Between Vertical Margins");
		case WBOOK_RT_BOUNDSHEET:
			return wxT("Sheet Information");
		case WBOOK_RT_WRITEPROT:
			return wxT("Workbook Is Write-Protected");
		case WBOOK_RT_ADDIN:
			return wxT("Workbook Is an Add-in Macro");
		case WBOOK_RT_EDG:
			return wxT("Edition Globals");
		case WBOOK_RT_PUB:
			return wxT("Publisher");
		case WBOOK_RT_COUNTRY:
			return wxT("Default Country and WIN.INI Country");
		case WBOOK_RT_HIDEOBJ:
			return wxT("Object Display Options");
		case WBOOK_RT_SORT:
			return wxT("Sorting Options");
		case WBOOK_RT_SUB:
			return wxT("Subscriber");
		case WBOOK_RT_PALETTE:
			return wxT("Color Palette Definition");
		case WBOOK_RT_LHRECORD:
			return wxT(".WK? File Conversion Information");
		case WBOOK_RT_LHNGRAPH:
			return wxT("Named Graph Information");
		case WBOOK_RT_SOUND:
			return wxT("Sound Note");
		case WBOOK_RT_LPR:
			return wxT("Sheet Was Printed Using LINE.PRINT(");
		case WBOOK_RT_STANDARDWIDTH:
			return wxT("Standard Column Widt");
		case WBOOK_RT_FILTERMODE:
			return wxT("Sheet Contains Filtered List");
		case WBOOK_RT_FNGROUPCOUNT:
			return wxT("Built-in Function Group Count");
		case WBOOK_RT_AUTOFILTERINFO:
			return wxT("Drop-Down Arrow Count");
		case WBOOK_RT_AUTOFILTER:
			return wxT("AutoFilter Data");
		case WBOOK_RT_SCL:
			return wxT("Window Zoom Magnification");
		case WBOOK_RT_SETUP:
			return wxT("Page Setup");
		case WBOOK_RT_COORDLIST:
			return wxT("Polygon Object Vertex Coordinates");
		case WBOOK_RT_GCW:
			return wxT("Global Column-Width Flags");
		case WBOOK_RT_SCENMAN:
			return wxT("Scenario Output Data");
		case WBOOK_RT_SCENARIO:
			return wxT("Scenario Data");
		case WBOOK_RT_SXVIEW:
			return wxT("View Definition");
		case WBOOK_RT_SXVD:
			return wxT("View Fields");
		case WBOOK_RT_SXVI:
			return wxT("View Item");
		case WBOOK_RT_SXIVD:
			return wxT("Row/Column Field IDs");
		case WBOOK_RT_SXLI:
			return wxT("Line Item Array");
		case WBOOK_RT_SXPI:
			return wxT("Page Item");
		case WBOOK_RT_DOCROUTE:
			return wxT("Routing Slip Information");
		case WBOOK_RT_RECIPNAME:
			return wxT("Recipient Name");
		case WBOOK_RT_SHRFMLA:
			return wxT("Shared Formula");
		case WBOOK_RT_MULRK:
			return wxT("Multiple RK Cells");
		case WBOOK_RT_MULBLANK:
			return wxT("Multiple Blank Cells");
		case WBOOK_RT_MMS:
			return wxT("ADDMENU/DELMENU Record Group Count");
		case WBOOK_RT_ADDMENU:
			return wxT("Menu Addition");
		case WBOOK_RT_DELMENU:
			return wxT("Menu Deletion");
		case WBOOK_RT_SXDI:
			return wxT("Data Item");
		case WBOOK_RT_SXDB:
			return wxT("PivotTable Cache Data");
		case WBOOK_RT_SXSTRING:
			return wxT("String");
		case WBOOK_RT_SXTBL:
			return wxT("Multiple Consolidation Source Info");
		case WBOOK_RT_SXTBRGIITM:
			return wxT("Page Item Name Count");
		case WBOOK_RT_SXTBPG:
			return wxT("Page Item Indexes");
		case WBOOK_RT_OBPROJ:
			return wxT("Visual Basic Project");
		case WBOOK_RT_SXIDSTM:
			return wxT("Stream ID");
		case WBOOK_RT_RSTRING:
			return wxT("Cell with Character Formatting");
		case WBOOK_RT_DBCELL:
			return wxT("Stream Offsets");
		case WBOOK_RT_BOOKBOOL:
			return wxT("Workbook Option Flag");
		// dupes! (combined)
		case WBOOK_RT_PARAMQRY:
			return wxT("Query Parameters / External Source Information");
		// case WBOOK_RT_SXEXT:
		//	return wxT("External Source Information");
		case WBOOK_RT_SCENPROTECT:
			return wxT("Scenario Protection");
		case WBOOK_RT_OLESIZE:
			return wxT("Size of OLE Object");
		case WBOOK_RT_UDDESC:
			return wxT("Description String for Chart Autoformat");
		case WBOOK_RT_XF:
			return wxT("Extended Format");
		case WBOOK_RT_INTERFACEHDR:
			return wxT("Beginning of User Interface Records");
		case WBOOK_RT_INTERFACEEND:
			return wxT("End of User Interface Records");
		case WBOOK_RT_SXVS:
			return wxT("View Source");
		case WBOOK_RT_MERGECELLS:
			return wxT("Merged Cells");
		case WBOOK_RT_TABIDCONF:
			return wxT("Sheet Tab ID of Conflict History");
		case WBOOK_RT_MSODRAWINGGROUP:
			return wxT("Microsoft Office Drawing Group");
		case WBOOK_RT_MSODRAWING:
			return wxT("Microsoft Office Drawing");
		case WBOOK_RT_MSODRAWINGSELECTION:
			return wxT("Microsoft Office Drawing Selection");
		case WBOOK_RT_SXRULE:
			return wxT("PivotTable Rule Data");
		case WBOOK_RT_SXEX:
			return wxT("PivotTable View Extended Information");
		case WBOOK_RT_SXFILT:
			return wxT("PivotTable Rule Filter");
		case WBOOK_RT_SXDXF:
			return wxT("Pivot Table Formatting");
		case WBOOK_RT_SXITM:
			return wxT("Pivot Table Item Indexes");
		case WBOOK_RT_SXNAME:
			return wxT("PivotTable Name");
		case WBOOK_RT_SXSELECT:
			return wxT("PivotTable Selection Information");
		case WBOOK_RT_SXPAIR:
			return wxT("PivotTable Name Pair");
		case WBOOK_RT_SXFMLA:
			return wxT("Pivot Table Parsed Expression");
		case WBOOK_RT_SXFORMAT:
			return wxT("PivotTable Format Record");
		case WBOOK_RT_SST:
			return wxT("Shared String Table");
		case WBOOK_RT_LABELSST:
			return wxT("Cell Value, String Constant/SST");
		case WBOOK_RT_EXTSST:
			return wxT("Extended Shared String Table");
		case WBOOK_RT_SXVDEX:
			return wxT("Extended PivotTable View Fields");
		case WBOOK_RT_SXFORMULA:
			return wxT("PivotTable Formula Record");
		case WBOOK_RT_SXDBEX:
			return wxT("PivotTable Cache Data");
		case WBOOK_RT_TABID:
			return wxT("Sheet Tab Index Array");
		case WBOOK_RT_USESELFS:
			return wxT("Natural Language Formulas Flag");
		case WBOOK_RT_DSF:
			return wxT("Double Stream File");
		case WBOOK_RT_XL5MODIFY:
			return wxT("Flag for DSF");
		case WBOOK_RT_FILESHARING2:
			return wxT("File-Sharing Information for Shared Lists");
		case WBOOK_RT_USERBVIEW:
			return wxT("Workbook Custom View Settings");
		case WBOOK_RT_USERSVIEWBEGIN:
			return wxT("Custom View Settings");
		case WBOOK_RT_USERSVIEWEND:
			return wxT("End of Custom View Records");
		case WBOOK_RT_QSI:
			return wxT("External Data Range");
		case WBOOK_RT_SUPBOOK:
			return wxT("Supporting Workbook");
		case WBOOK_RT_PROT4REV:
			return wxT("Shared Workbook Protection Flag");
		case WBOOK_RT_CONDFMT:
			return wxT("Conditional Formatting Range Information");
		case WBOOK_RT_CF:
			return wxT("Conditional Formatting Conditions");
		case WBOOK_RT_DVAL:
			return wxT("Data Validation Information");
		case WBOOK_RT_DCONBIN:
			return wxT("Data Consolidation Information");
		case WBOOK_RT_TXO:
			return wxT("Text Object");
		case WBOOK_RT_REFRESHALL:
			return wxT("Refresh Flag");
		case WBOOK_RT_HLINK:
			return wxT("Hyperlink");
		case WBOOK_RT_SXFDBTYPE:
			return wxT("SQL Datatype Identifier");
		case WBOOK_RT_PROT4REVPASS:
			return wxT("Shared Workbook Protection Password");
		case WBOOK_RT_DV:
			return wxT("Data Validation Criteria");
		case WBOOK_RT_EXCEL9FILE:
			return wxT("Excel 9 File");
		case WBOOK_RT_RECALCID:
			return wxT("Recalc Information");
		case WBOOK_RT_DIMENSIONS:
			return wxT("Cell Table Size");
		case WBOOK_RT_BLANK:
			return wxT("Cell Value, Blank Cell");
		case WBOOK_RT_NUMBER:
			return wxT("Cell Value, Floating-Point Number");
		case WBOOK_RT_LABEL:
			return wxT("Cell Value, String Constant");
		case WBOOK_RT_BOOLERR:
			return wxT("Cell Value, Boolean or Error");
		case WBOOK_RT_STRING:
			return wxT("String Value of a Formula");
		case WBOOK_RT_ROW:
			return wxT("Describes a Row");
		case WBOOK_RT_INDEX:
			return wxT("Index Record");
		case WBOOK_RT_NAME:
			return wxT("Defined Name");
		case WBOOK_RT_ARRAY:
			return wxT("Array-Entered Formula");
		case WBOOK_RT_EXTERNNAME:
			return wxT("Externally Referenced Name");
		case WBOOK_RT_DEFAULTROWHEIGHT:
			return wxT("Default Row Height");
		case WBOOK_RT_FONT:
			return wxT("Font Description");
		case WBOOK_RT_TABLE:
			return wxT("Data Table");
		case WBOOK_RT_WINDOW2:
			return wxT("Sheet Window Information");
		case WBOOK_RT_STYLE:
			return wxT("Style Information");
		case WBOOK_RT_FORMULA:
			return wxT("Cell Formula");
		case WBOOK_RT_FORMAT:
			return wxT("Number Format");
		case WBOOK_RT_HLINKTOOLTIP:
			return wxT("Hyperlink Tooltip");
		case WBOOK_RT_WEBPUB:
			return wxT("Web Publish Item");
		case WBOOK_RT_QSISXTAG:
			return wxT("PivotTable and Query Table Extensions");
		case WBOOK_RT_DBQUERYEXT:
			return wxT("Database Query Extensions");
		case WBOOK_RT_EXTSTRING:
			return wxT("FRT String");
		case WBOOK_RT_TXTQUERY:
			return wxT("Text Query Information");
		case WBOOK_RT_QSIR:
			return wxT("Query Table Formatting");
		case WBOOK_RT_QSIF:
			return wxT("Query Table Field Formatting");
		case WBOOK_RT_OLEDBCONN:
			return wxT("OLE Database Connection");
		case WBOOK_RT_WOPT:
			return wxT("Web Options");
		case WBOOK_RT_SXVIEWEX:
			return wxT("Pivot Table OLAP Extensions");
		case WBOOK_RT_SXTH:
			return wxT("PivotTable OLAP Hierarchy");
		case WBOOK_RT_SXPIEX:
			return wxT("OLAP Page Item Extensions");
		case WBOOK_RT_SXVDTEX:
			return wxT("View Dimension OLAP Extensions");
		case WBOOK_RT_SXVIEWEX9:
			return wxT("Pivot Table Extensions");
		case WBOOK_RT_CONTINUEFRT:
			return wxT("Continued FRT");
		case WBOOK_RT_REALTIMEDATA:
			return wxT("Real-Time Data (RTD)");
		case WBOOK_RT_SHEETEXT:
			return wxT("Extra Sheet Info");
		case WBOOK_RT_BOOKEXT:
			return wxT("Extra Book Info");
		case WBOOK_RT_SXADDL:
			return wxT("Pivot Table Additional Info");
		case WBOOK_RT_CRASHRECERR:
			return wxT("Crash Recovery Error");
		case WBOOK_RT_HFPicture:
			return wxT("Header / Footer Picture");
		case WBOOK_RT_FEATHEADR:
			return wxT("Shared Feature Header");
		case WBOOK_RT_FEAT:
			return wxT("Shared Feature Record");
		case WBOOK_RT_DATALABEXT:
			return wxT("Chart Data Label Extension");
		case WBOOK_RT_DATALABEXTCONTENTS:
			return wxT("Chart Data Label Extension Contents");
		case WBOOK_RT_CELLWATCH:
			return wxT("Cell Watc");
		case WBOOK_RT_FEATHEADR11:
			return wxT("Shared Feature Header 11");
		case WBOOK_RT_FEAT11:
			return wxT("Shared Feature 11 Record");
		case WBOOK_RT_FEATINFO11:
			return wxT("Shared Feature Info 11 Record");
		case WBOOK_RT_DROPDOWNOBJIDS:
			return wxT("Drop Down Object");
		case WBOOK_RT_CONTINUEFRT11:
			return wxT("Continue FRT 11");
		case WBOOK_RT_DCONN:
			return wxT("Data Connection");
		case WBOOK_RT_LIST12:
			return wxT("Extra Table Data Introduced in Excel 2007");
		case WBOOK_RT_FEAT12:
			return wxT("Shared Feature 12 Record");
		case WBOOK_RT_CONDFMT12:
			return wxT("Conditional Formatting Range Information 12");
		case WBOOK_RT_CF12:
			return wxT("Conditional Formatting Condition 12");
		case WBOOK_RT_CFEX:
			return wxT("Conditional Formatting Extension");
		case WBOOK_RT_XFCRC:
			return wxT("XF Extensions Checksum");
		case WBOOK_RT_XFEXT:
			return wxT("XF Extension");
		case WBOOK_RT_EZFILTER12:
			return wxT("AutoFilter Data Introduced in Excel 2007");
		case WBOOK_RT_CONTINUEFRT12:
			return wxT("Continue FRT 12");
		case WBOOK_RT_SXADDL12:
			return wxT("Additional Workbook Connections Information");
		case WBOOK_RT_MDTINFO:
			return wxT("Information about a Metadata Type");
		case WBOOK_RT_MDXSTR:
			return wxT("MDX Metadata String");
		case WBOOK_RT_MDXTUPLE:
			return wxT("Tuple MDX Metadata");
		case WBOOK_RT_MDXSET:
			return wxT("Set MDX Metadata");
		case WBOOK_RT_MDXPROP:
			return wxT("Member Property MDX Metadata");
		case WBOOK_RT_MDXKPI:
			return wxT("Key Performance Indicator MDX Metadata");
		case WBOOK_RT_MDTB:
			return wxT("Block of Metadata Records");
		case WBOOK_RT_PLV_XLS2007:
			return wxT("Page Layout View Settings in Excel 2007");
		case WBOOK_RT_COMPAT12:
			return wxT("Compatibility Checker 12");
		case WBOOK_RT_DXF:
			return wxT("Differential XF");
		case WBOOK_RT_TABLESTYLES:
			return wxT("Table Styles");
		case WBOOK_RT_TABLESTYLE:
			return wxT("Table Style");
		case WBOOK_RT_TABLESTYLEELEMENT:
			return wxT("Table Style Element");
		case WBOOK_RT_STYLEEXT:
			return wxT("Named Cell Style Extension");
		case WBOOK_RT_NAMEPUBLISH:
			return wxT("Publish To Excel Server Data for Name");
		case WBOOK_RT_NAMECMT:
			return wxT("Name Comment");
		case WBOOK_RT_SORTDATA12:
			return wxT("Sort Data 12");
		case WBOOK_RT_THEME:
			return wxT("Theme");
		case WBOOK_RT_GUIDTYPELIB:
			return wxT("VB Project Typelib GUID");
		case WBOOK_RT_FNGRP12:
			return wxT("Function Group");
		case WBOOK_RT_NAMEFNGRP12:
			return wxT("Extra Function Group");
		case WBOOK_RT_MTRSETTINGS:
			return wxT("Multi-Threaded Calculation Settings");
		case WBOOK_RT_COMPRESSPICTURES:
			return wxT("Automatic Picture Compression Mode");
		case WBOOK_RT_HEADERFOOTER:
			return wxT("Header Footer");
		case WBOOK_RT_FORCEFULLCALCULATION:
			return wxT("Force Full Calculation Settings");
		case WBOOK_RT_LISTOBJ:
			return wxT("List Object");
		case WBOOK_RT_LISTFIELD:
			return wxT("List Field");
		case WBOOK_RT_LISTDV:
			return wxT("List Data Validation");
		case WBOOK_RT_LISTCONDFMT:
			return wxT("List Conditional Formatting");
		case WBOOK_RT_LISTCF:
			return wxT("List Cell Formatting");
		case WBOOK_RT_FMQRY:
			return wxT("Filemaker queries");
		case WBOOK_RT_FMSQRY:
			return wxT("File maker queries");
		case WBOOK_RT_PLV_MAC11:
			return wxT("Page Layout View in Mac Excel 11");
		case WBOOK_RT_LNEXT:
			return wxT("Extension information for borders in Mac Office 11");
		case WBOOK_RT_MKREXT:
			return wxT("Extension information for markers in Mac Office 11");
		case WBOOK_RT_CRTCOOPT:
			return wxT("Color options for Chart series in Mac Office 11");

		// auto-generated long chart record type strings
		case WBOOK_RT_CHUNITS:
			return wxT("Chart Units");
		case WBOOK_RT_CHCHART:
			return wxT("Location and Overall Chart Dimensions");
		case WBOOK_RT_CHSERIES:
			return wxT("Series Definition");
		case WBOOK_RT_CHDATAFORMAT:
			return wxT("Series and Data Point Numbers");
		case WBOOK_RT_CHLINEFORMAT:
			return wxT("Style of a Line or Border");
		case WBOOK_RT_CHMARKERFORMAT:
			return wxT("Style of a Line Marker");
		case WBOOK_RT_CHAREAFORMAT:
			return wxT("Colors and Patterns for an Area");
		case WBOOK_RT_CHPIEFORMAT:
			return wxT("Position of the Pie Slice");
		case WBOOK_RT_CHATTACHEDLABEL:
			return wxT("Series Data/Value Labels");
		case WBOOK_RT_CHSERIESTEXT:
			return wxT("Legend/Category/Value Text");
		case WBOOK_RT_CHCHARTFORMAT:
			return wxT("Parent Record for Chart Group");
		case WBOOK_RT_CHLEGEND:
			return wxT("Legend Type and Position");
		case WBOOK_RT_CHSERIESLIST:
			return wxT("Specifies the Series in an Overlay Chart");
		case WBOOK_RT_CHBAR:
			return wxT("Chart Group is a Bar or Column Chart Group");
		case WBOOK_RT_CHLINE:
			return wxT("Chart Group Is a Line Chart Group");
		case WBOOK_RT_CHPIE:
			return wxT("Chart Group Is a Pie Chart Group");
		case WBOOK_RT_CHAREA:
			return wxT("Chart Group Is an Area Chart Group");
		case WBOOK_RT_CHSCATTER:
			return wxT("Chart Group Is a Scatter Chart Group");
		case WBOOK_RT_CHCHARTLINE:
			return wxT("Drop/Hi-Lo/Series Lines on a Line Chart");
		case WBOOK_RT_CHAXIS:
			return wxT("Axis Type");
		case WBOOK_RT_CHTICK:
			return wxT("Tick Marks and Labels Format");
		case WBOOK_RT_CHVALUERANGE:
			return wxT("Defines Value Axis Scale");
		case WBOOK_RT_CHCATSERRANGE:
			return wxT("Defines a Category or Series Axis");
		case WBOOK_RT_CHAXISLINEFORMAT:
			return wxT("Defines a Line That Spans an Axis");
		case WBOOK_RT_CHCHARTFORMATLINK:
			return wxT("Not Used");
		case WBOOK_RT_CHDEFAULTTEXT:
			return wxT("Default Data Label Text Properties");
		case WBOOK_RT_CHTEXT:
			return wxT("Defines Display of Text Fields");
		case WBOOK_RT_CHFONTX:
			return wxT("Font Index");
		case WBOOK_RT_CHOBJECTLINK:
			return wxT("Attaches Text to Chart or to Chart Item");
		case WBOOK_RT_CHFRAME:
			return wxT("Defines Border Shape Around Displayed Text");
		case WBOOK_RT_CHBEGIN:
			return wxT("Defines the Beginning of an Object");
		case WBOOK_RT_CHEND:
			return wxT("Defines the End of an Object");
		case WBOOK_RT_CHPLOTAREA:
			return wxT("Frame Belongs to Plot Area ");
		case WBOOK_RT_CH3D:
			return wxT("Chart Group Is a 3-D Chart Group");
		case WBOOK_RT_CHPICF:
			return wxT("Picture Format");
		case WBOOK_RT_CHDROPBAR:
			return wxT("Defines Drop Bars");
		case WBOOK_RT_CHRADAR:
			return wxT("Chart Group Is a Radar Chart Group");
		case WBOOK_RT_CHSURFACE:
			return wxT("Chart Group Is a Surface Chart Group");
		case WBOOK_RT_CHRADARAREA:
			return wxT("Chart Group Is a Radar Area Chart Group");
		case WBOOK_RT_CHAXISPARENT:
			return wxT("Axis Size and Location");
		case WBOOK_RT_CHLEGENDXN:
			return wxT("Legend Exception");
		case WBOOK_RT_CHSHTPROPS:
			return wxT("Sheet Properties");
		case WBOOK_RT_CHSERTOCRT:
			return wxT("Series Chart-Group Index");
		case WBOOK_RT_CHAXESUSED:
			return wxT("Number of Axes Sets");
		case WBOOK_RT_CHSBASEREF:
			return wxT("PivotTable Reference");
		case WBOOK_RT_CHSERPARENT:
			return wxT("Trendline or ErrorBar Series Index");
		case WBOOK_RT_CHSERAUXTREND:
			return wxT("Series Trendline");
		case WBOOK_RT_CHIFMT:
			return wxT("Number-Format Index");
		case WBOOK_RT_CHPOS:
			return wxT("Position Information");
		case WBOOK_RT_CHALRUNS:
			return wxT("Text Formatting");
		case WBOOK_RT_CHAI:
			return wxT("Linked Data");
		case WBOOK_RT_CHSERAUXERRBAR:
			return wxT("Series ErrorBar");
		case WBOOK_RT_CHSERFMT:
			return wxT("Series Format");
		case WBOOK_RT_CHFBI:
			return wxT("Font Basis");
		case WBOOK_RT_CHBOPPOP:
			return wxT("Bar of Pie/Pie of Pie Chart Options");
		case WBOOK_RT_CHAXCEXT:
			return wxT("Axis Options");
		case WBOOK_RT_CHDAT:
			return wxT("Data Table Options");
		case WBOOK_RT_CHPLOTGROWTH:
			return wxT("Font Scale Factors");
		case WBOOK_RT_CHSIINDEX:
			return wxT("Series Index");
		case WBOOK_RT_CHGELFRAME:
			return wxT("Fill Data");
		case WBOOK_RT_CHBOPPOPCUSTOM:
			return wxT("Custom Bar of Pie/Pie of Pie Chart Options");

		// manually defined
		default:
			return wxT("Unknown Record Type");
	}
}


wxChar *Workbook::HumanReadableRecordTypeShort(USHORT type)
{
	switch (type)
	{
		// done manually -- not in documentation
		case WBOOK_RT_UFONT:
			return wxT("UFONT");

		// done manually
		case WBOOK_RT_BOF2:
		case WBOOK_RT_BOF3:
		case WBOOK_RT_BOF4:
		case WBOOK_RT_BOF578:
			return wxT("BOF");

		// auto-generated short normal record type strings
		case WBOOK_RT_EOF:
			return wxT("EOF");
		case WBOOK_RT_CALCCOUNT:
			return wxT("CALCCOUNT");
		case WBOOK_RT_CALCMODE:
			return wxT("CALCMODE");
		case WBOOK_RT_PRECISION:
			return wxT("PRECISION");
		case WBOOK_RT_REFMODE:
			return wxT("REFMODE");
		case WBOOK_RT_DELTA:
			return wxT("DELTA");
		case WBOOK_RT_ITERATION:
			return wxT("ITERATION");
		case WBOOK_RT_PROTECT:
			return wxT("PROTECT");
		case WBOOK_RT_PASSWORD:
			return wxT("PASSWORD");
		case WBOOK_RT_HEADER:
			return wxT("HEADER");
		case WBOOK_RT_FOOTER:
			return wxT("FOOTER");
		case WBOOK_RT_EXTERNCOUNT:
			return wxT("EXTERNCOUNT");
		case WBOOK_RT_EXTERNSHEET:
			return wxT("EXTERNSHEET");
		case WBOOK_RT_WINDOWPROTECT:
			return wxT("WINDOWPROTECT");
		case WBOOK_RT_VERTICALPAGEBREAKS:
			return wxT("VERTICALPAGEBREAKS");
		case WBOOK_RT_HORIZONTALPAGEBREAKS:
			return wxT("HORIZONTALPAGEBREAKS");
		case WBOOK_RT_NOTE:
			return wxT("NOTE");
		case WBOOK_RT_SELECTION:
			return wxT("SELECTION");
		case WBOOK_RT_1904:
			return wxT("1904");
		case WBOOK_RT_LEFTMARGIN:
			return wxT("LEFTMARGIN");
		case WBOOK_RT_RIGHTMARGIN:
			return wxT("RIGHTMARGIN");
		case WBOOK_RT_TOPMARGIN:
			return wxT("TOPMARGIN");
		case WBOOK_RT_BOTTOMMARGIN:
			return wxT("BOTTOMMARGIN");
		case WBOOK_RT_PRINTHEADERS:
			return wxT("PRINTHEADERS");
		case WBOOK_RT_PRINTGRIDLINES:
			return wxT("PRINTGRIDLINES");
		case WBOOK_RT_FILEPASS:
			return wxT("FILEPASS");
		case WBOOK_RT_CONTINUE:
			return wxT("CONTINUE");
		case WBOOK_RT_WINDOW1:
			return wxT("WINDOW1");
		case WBOOK_RT_BACKUP:
			return wxT("BACKUP");
		case WBOOK_RT_PANE:
			return wxT("PANE");
		// dupes! (combined)
		case WBOOK_RT_CODENAME:
			return wxT("CODENAME / CODEPAGE");
		// case WBOOK_RT_CODEPAGE:
		//	return wxT("CODEPAGE");
		case WBOOK_RT_PLS:
			return wxT("PLS");
		case WBOOK_RT_DCON:
			return wxT("DCON");
		case WBOOK_RT_DCONREF:
			return wxT("DCONREF");
		case WBOOK_RT_DCONNAME:
			return wxT("DCONNAME");
		case WBOOK_RT_DEFCOLWIDTH:
			return wxT("DEFCOLWIDTH");
		case WBOOK_RT_XCT:
			return wxT("XCT");
		case WBOOK_RT_CRN:
			return wxT("CRN");
		case WBOOK_RT_FILESHARING:
			return wxT("FILESHARING");
		case WBOOK_RT_WRITEACCESS:
			return wxT("WRITEACCESS");
		case WBOOK_RT_OBJ:
			return wxT("OBJ");
		case WBOOK_RT_UNCALCED:
			return wxT("UNCALCED");
		case WBOOK_RT_SAVERECALC:
			return wxT("SAVERECALC");
		case WBOOK_RT_TEMPLATE:
			return wxT("TEMPLATE");
		case WBOOK_RT_OBJPROTECT:
			return wxT("OBJPROTECT");
		case WBOOK_RT_COLINFO:
			return wxT("COLINFO");
		case WBOOK_RT_RK:
		case WBOOK_RT_RK3:
			return wxT("RK");
		case WBOOK_RT_IMDATA:
			return wxT("IMDATA");
		case WBOOK_RT_GUTS:
			return wxT("GUTS");
		case WBOOK_RT_WSBOOL:
			return wxT("WSBOOL");
		case WBOOK_RT_GRIDSET:
			return wxT("GRIDSET");
		case WBOOK_RT_HCENTER:
			return wxT("HCENTER");
		case WBOOK_RT_VCENTER:
			return wxT("VCENTER");
		case WBOOK_RT_BOUNDSHEET:
			return wxT("BOUNDSHEET");
		case WBOOK_RT_WRITEPROT:
			return wxT("WRITEPROT");
		case WBOOK_RT_ADDIN:
			return wxT("ADDIN");
		case WBOOK_RT_EDG:
			return wxT("EDG");
		case WBOOK_RT_PUB:
			return wxT("PUB");
		case WBOOK_RT_COUNTRY:
			return wxT("COUNTRY");
		case WBOOK_RT_HIDEOBJ:
			return wxT("HIDEOBJ");
		case WBOOK_RT_SORT:
			return wxT("SORT");
		case WBOOK_RT_SUB:
			return wxT("SUB");
		case WBOOK_RT_PALETTE:
			return wxT("PALETTE");
		case WBOOK_RT_LHRECORD:
			return wxT("LHRECORD");
		case WBOOK_RT_LHNGRAPH:
			return wxT("LHNGRAPH");
		case WBOOK_RT_SOUND:
			return wxT("SOUND");
		case WBOOK_RT_LPR:
			return wxT("LPR");
		case WBOOK_RT_STANDARDWIDTH:
			return wxT("STANDARDWIDTH");
		case WBOOK_RT_FILTERMODE:
			return wxT("FILTERMODE");
		case WBOOK_RT_FNGROUPCOUNT:
			return wxT("FNGROUPCOUNT");
		case WBOOK_RT_AUTOFILTERINFO:
			return wxT("AUTOFILTERINFO");
		case WBOOK_RT_AUTOFILTER:
			return wxT("AUTOFILTER");
		case WBOOK_RT_SCL:
			return wxT("SCL");
		case WBOOK_RT_SETUP:
			return wxT("SETUP");
		case WBOOK_RT_COORDLIST:
			return wxT("COORDLIST");
		case WBOOK_RT_GCW:
			return wxT("GCW");
		case WBOOK_RT_SCENMAN:
			return wxT("SCENMAN");
		case WBOOK_RT_SCENARIO:
			return wxT("SCENARIO");
		case WBOOK_RT_SXVIEW:
			return wxT("SXVIEW");
		case WBOOK_RT_SXVD:
			return wxT("SXVD");
		case WBOOK_RT_SXVI:
			return wxT("SXVI");
		case WBOOK_RT_SXIVD:
			return wxT("SXIVD");
		case WBOOK_RT_SXLI:
			return wxT("SXLI");
		case WBOOK_RT_SXPI:
			return wxT("SXPI");
		case WBOOK_RT_DOCROUTE:
			return wxT("DOCROUTE");
		case WBOOK_RT_RECIPNAME:
			return wxT("RECIPNAME");
		case WBOOK_RT_SHRFMLA:
			return wxT("SHRFMLA");
		case WBOOK_RT_MULRK:
			return wxT("MULRK");
		case WBOOK_RT_MULBLANK:
			return wxT("MULBLANK");
		case WBOOK_RT_MMS:
			return wxT("MMS");
		case WBOOK_RT_ADDMENU:
			return wxT("ADDMENU");
		case WBOOK_RT_DELMENU:
			return wxT("DELMENU");
		case WBOOK_RT_SXDI:
			return wxT("SXDI");
		case WBOOK_RT_SXDB:
			return wxT("SXDB");
		case WBOOK_RT_SXSTRING:
			return wxT("SXSTRING");
		case WBOOK_RT_SXTBL:
			return wxT("SXTBL");
		case WBOOK_RT_SXTBRGIITM:
			return wxT("SXTBRGIITM");
		case WBOOK_RT_SXTBPG:
			return wxT("SXTBPG");
		case WBOOK_RT_OBPROJ:
			return wxT("OBPROJ");
		case WBOOK_RT_SXIDSTM:
			return wxT("SXIDSTM");
		case WBOOK_RT_RSTRING:
			return wxT("RSTRING");
		case WBOOK_RT_DBCELL:
			return wxT("DBCELL");
		case WBOOK_RT_BOOKBOOL:
			return wxT("BOOKBOOL");
		// dupes! (combined)
		case WBOOK_RT_PARAMQRY:
			return wxT("PARAMQRY / SXEXT");
		// case WBOOK_RT_SXEXT:
		//	return wxT("SXEXT");
		case WBOOK_RT_SCENPROTECT:
			return wxT("SCENPROTECT");
		case WBOOK_RT_OLESIZE:
			return wxT("OLESIZE");
		case WBOOK_RT_UDDESC:
			return wxT("UDDESC");
		case WBOOK_RT_XF:
			return wxT("XF");
		case WBOOK_RT_INTERFACEHDR:
			return wxT("INTERFACEHDR");
		case WBOOK_RT_INTERFACEEND:
			return wxT("INTERFACEEND");
		case WBOOK_RT_SXVS:
			return wxT("SXVS");
		case WBOOK_RT_MERGECELLS:
			return wxT("MERGECELLS");
		case WBOOK_RT_TABIDCONF:
			return wxT("TABIDCONF");
		case WBOOK_RT_MSODRAWINGGROUP:
			return wxT("MSODRAWINGGROUP");
		case WBOOK_RT_MSODRAWING:
			return wxT("MSODRAWING");
		case WBOOK_RT_MSODRAWINGSELECTION:
			return wxT("MSODRAWINGSELECTION");
		case WBOOK_RT_SXRULE:
			return wxT("SXRULE");
		case WBOOK_RT_SXEX:
			return wxT("SXEX");
		case WBOOK_RT_SXFILT:
			return wxT("SXFILT");
		case WBOOK_RT_SXDXF:
			return wxT("SXDXF");
		case WBOOK_RT_SXITM:
			return wxT("SXITM");
		case WBOOK_RT_SXNAME:
			return wxT("SXNAME");
		case WBOOK_RT_SXSELECT:
			return wxT("SXSELECT");
		case WBOOK_RT_SXPAIR:
			return wxT("SXPAIR");
		case WBOOK_RT_SXFMLA:
			return wxT("SXFMLA");
		case WBOOK_RT_SXFORMAT:
			return wxT("SXFORMAT");
		case WBOOK_RT_SST:
			return wxT("SST");
		case WBOOK_RT_LABELSST:
			return wxT("LABELSST");
		case WBOOK_RT_EXTSST:
			return wxT("EXTSST");
		case WBOOK_RT_SXVDEX:
			return wxT("SXVDEX");
		case WBOOK_RT_SXFORMULA:
			return wxT("SXFORMULA");
		case WBOOK_RT_SXDBEX:
			return wxT("SXDBEX");
		case WBOOK_RT_TABID:
			return wxT("TABID");
		case WBOOK_RT_USESELFS:
			return wxT("USESELFS");
		case WBOOK_RT_DSF:
			return wxT("DSF");
		case WBOOK_RT_XL5MODIFY:
			return wxT("XL5MODIFY");
		case WBOOK_RT_FILESHARING2:
			return wxT("FILESHARING2");
		case WBOOK_RT_USERBVIEW:
			return wxT("USERBVIEW");
		case WBOOK_RT_USERSVIEWBEGIN:
			return wxT("USERSVIEWBEGIN");
		case WBOOK_RT_USERSVIEWEND:
			return wxT("USERSVIEWEND");
		case WBOOK_RT_QSI:
			return wxT("QSI");
		case WBOOK_RT_SUPBOOK:
			return wxT("SUPBOOK");
		case WBOOK_RT_PROT4REV:
			return wxT("PROT4REV");
		case WBOOK_RT_CONDFMT:
			return wxT("CONDFMT");
		case WBOOK_RT_CF:
			return wxT("CF");
		case WBOOK_RT_DVAL:
			return wxT("DVAL");
		case WBOOK_RT_DCONBIN:
			return wxT("DCONBIN");
		case WBOOK_RT_TXO:
			return wxT("TXO");
		case WBOOK_RT_REFRESHALL:
			return wxT("REFRESHALL");
		case WBOOK_RT_HLINK:
			return wxT("HLINK");
		case WBOOK_RT_SXFDBTYPE:
			return wxT("SXFDBTYPE");
		case WBOOK_RT_PROT4REVPASS:
			return wxT("PROT4REVPASS");
		case WBOOK_RT_DV:
			return wxT("DV");
		case WBOOK_RT_EXCEL9FILE:
			return wxT("EXCEL9FILE");
		case WBOOK_RT_RECALCID:
			return wxT("RECALCID");
		case WBOOK_RT_DIMENSIONS:
			return wxT("DIMENSIONS");
		case WBOOK_RT_BLANK:
			return wxT("BLANK");
		case WBOOK_RT_NUMBER:
			return wxT("NUMBER");
		case WBOOK_RT_LABEL:
			return wxT("LABEL");
		case WBOOK_RT_BOOLERR:
			return wxT("BOOLERR");
		case WBOOK_RT_STRING:
			return wxT("STRING");
		case WBOOK_RT_ROW:
			return wxT("ROW");
		case WBOOK_RT_INDEX:
			return wxT("INDEX");
		case WBOOK_RT_NAME:
			return wxT("NAME");
		case WBOOK_RT_ARRAY:
			return wxT("ARRAY");
		case WBOOK_RT_EXTERNNAME:
			return wxT("EXTERNNAME");
		case WBOOK_RT_DEFAULTROWHEIGHT:
			return wxT("DEFAULTROWHEIGHT");
		case WBOOK_RT_FONT:
			return wxT("FONT");
		case WBOOK_RT_TABLE:
			return wxT("TABLE");
		case WBOOK_RT_WINDOW2:
			return wxT("WINDOW2");
		case WBOOK_RT_STYLE:
			return wxT("STYLE");
		case WBOOK_RT_FORMULA:
			return wxT("FORMULA");
		case WBOOK_RT_FORMAT:
			return wxT("FORMAT");
		case WBOOK_RT_HLINKTOOLTIP:
			return wxT("HLINKTOOLTIP");
		case WBOOK_RT_WEBPUB:
			return wxT("WEBPUB");
		case WBOOK_RT_QSISXTAG:
			return wxT("QSISXTAG");
		case WBOOK_RT_DBQUERYEXT:
			return wxT("DBQUERYEXT");
		case WBOOK_RT_EXTSTRING:
			return wxT("EXTSTRING");
		case WBOOK_RT_TXTQUERY:
			return wxT("TXTQUERY");
		case WBOOK_RT_QSIR:
			return wxT("QSIR");
		case WBOOK_RT_QSIF:
			return wxT("QSIF");
		case WBOOK_RT_OLEDBCONN:
			return wxT("OLEDBCONN");
		case WBOOK_RT_WOPT:
			return wxT("WOPT");
		case WBOOK_RT_SXVIEWEX:
			return wxT("SXVIEWEX");
		case WBOOK_RT_SXTH:
			return wxT("SXTH");
		case WBOOK_RT_SXPIEX:
			return wxT("SXPIEX");
		case WBOOK_RT_SXVDTEX:
			return wxT("SXVDTEX");
		case WBOOK_RT_SXVIEWEX9:
			return wxT("SXVIEWEX9");
		case WBOOK_RT_CONTINUEFRT:
			return wxT("CONTINUEFRT");
		case WBOOK_RT_REALTIMEDATA:
			return wxT("REALTIMEDATA");
		case WBOOK_RT_SHEETEXT:
			return wxT("SHEETEXT");
		case WBOOK_RT_BOOKEXT:
			return wxT("BOOKEXT");
		case WBOOK_RT_SXADDL:
			return wxT("SXADDL");
		case WBOOK_RT_CRASHRECERR:
			return wxT("CRASHRECERR");
		case WBOOK_RT_HFPicture:
			return wxT("HFPicture");
		case WBOOK_RT_FEATHEADR:
			return wxT("FEATHEADR");
		case WBOOK_RT_FEAT:
			return wxT("FEAT");
		case WBOOK_RT_DATALABEXT:
			return wxT("DATALABEXT");
		case WBOOK_RT_DATALABEXTCONTENTS:
			return wxT("DATALABEXTCONTENTS");
		case WBOOK_RT_CELLWATCH:
			return wxT("CELLWATCH");
		case WBOOK_RT_FEATHEADR11:
			return wxT("FEATHEADR11");
		case WBOOK_RT_FEAT11:
			return wxT("FEAT11");
		case WBOOK_RT_FEATINFO11:
			return wxT("FEATINFO11");
		case WBOOK_RT_DROPDOWNOBJIDS:
			return wxT("DROPDOWNOBJIDS");
		case WBOOK_RT_CONTINUEFRT11:
			return wxT("CONTINUEFRT11");
		case WBOOK_RT_DCONN:
			return wxT("DCONN");
		case WBOOK_RT_LIST12:
			return wxT("LIST12");
		case WBOOK_RT_FEAT12:
			return wxT("FEAT12");
		case WBOOK_RT_CONDFMT12:
			return wxT("CONDFMT12");
		case WBOOK_RT_CF12:
			return wxT("CF12");
		case WBOOK_RT_CFEX:
			return wxT("CFEX");
		case WBOOK_RT_XFCRC:
			return wxT("XFCRC");
		case WBOOK_RT_XFEXT:
			return wxT("XFEXT");
		case WBOOK_RT_EZFILTER12:
			return wxT("EZFILTER12");
		case WBOOK_RT_CONTINUEFRT12:
			return wxT("CONTINUEFRT12");
		case WBOOK_RT_SXADDL12:
			return wxT("SXADDL12");
		case WBOOK_RT_MDTINFO:
			return wxT("MDTINFO");
		case WBOOK_RT_MDXSTR:
			return wxT("MDXSTR");
		case WBOOK_RT_MDXTUPLE:
			return wxT("MDXTUPLE");
		case WBOOK_RT_MDXSET:
			return wxT("MDXSET");
		case WBOOK_RT_MDXPROP:
			return wxT("MDXPROP");
		case WBOOK_RT_MDXKPI:
			return wxT("MDXKPI");
		case WBOOK_RT_MDTB:
			return wxT("MDTB");
		case WBOOK_RT_PLV_XLS2007:
			return wxT("PLV_XLS2007");
		case WBOOK_RT_COMPAT12:
			return wxT("COMPAT12");
		case WBOOK_RT_DXF:
			return wxT("DXF");
		case WBOOK_RT_TABLESTYLES:
			return wxT("TABLESTYLES");
		case WBOOK_RT_TABLESTYLE:
			return wxT("TABLESTYLE");
		case WBOOK_RT_TABLESTYLEELEMENT:
			return wxT("TABLESTYLEELEMENT");
		case WBOOK_RT_STYLEEXT:
			return wxT("STYLEEXT");
		case WBOOK_RT_NAMEPUBLISH:
			return wxT("NAMEPUBLISH");
		case WBOOK_RT_NAMECMT:
			return wxT("NAMECMT");
		case WBOOK_RT_SORTDATA12:
			return wxT("SORTDATA12");
		case WBOOK_RT_THEME:
			return wxT("THEME");
		case WBOOK_RT_GUIDTYPELIB:
			return wxT("GUIDTYPELIB");
		case WBOOK_RT_FNGRP12:
			return wxT("FNGRP12");
		case WBOOK_RT_NAMEFNGRP12:
			return wxT("NAMEFNGRP12");
		case WBOOK_RT_MTRSETTINGS:
			return wxT("MTRSETTINGS");
		case WBOOK_RT_COMPRESSPICTURES:
			return wxT("COMPRESSPICTURES");
		case WBOOK_RT_HEADERFOOTER:
			return wxT("HEADERFOOTER");
		case WBOOK_RT_FORCEFULLCALCULATION:
			return wxT("FORCEFULLCALCULATION");
		case WBOOK_RT_LISTOBJ:
			return wxT("LISTOBJ");
		case WBOOK_RT_LISTFIELD:
			return wxT("LISTFIELD");
		case WBOOK_RT_LISTDV:
			return wxT("LISTDV");
		case WBOOK_RT_LISTCONDFMT:
			return wxT("LISTCONDFMT");
		case WBOOK_RT_LISTCF:
			return wxT("LISTCF");
		case WBOOK_RT_FMQRY:
			return wxT("FMQRY");
		case WBOOK_RT_FMSQRY:
			return wxT("FMSQRY");
		case WBOOK_RT_PLV_MAC11:
			return wxT("PLV_MAC11");
		case WBOOK_RT_LNEXT:
			return wxT("LNEXT");
		case WBOOK_RT_MKREXT:
			return wxT("MKREXT");
		case WBOOK_RT_CRTCOOPT:
			return wxT("CRTCOOPT");

		// auto-generated short chart record type strings
		case WBOOK_RT_CHUNITS:
			return wxT("CHUNITS");
		case WBOOK_RT_CHCHART:
			return wxT("CHCHART");
		case WBOOK_RT_CHSERIES:
			return wxT("CHSERIES");
		case WBOOK_RT_CHDATAFORMAT:
			return wxT("CHDATAFORMAT");
		case WBOOK_RT_CHLINEFORMAT:
			return wxT("CHLINEFORMAT");
		case WBOOK_RT_CHMARKERFORMAT:
			return wxT("CHMARKERFORMAT");
		case WBOOK_RT_CHAREAFORMAT:
			return wxT("CHAREAFORMAT");
		case WBOOK_RT_CHPIEFORMAT:
			return wxT("CHPIEFORMAT");
		case WBOOK_RT_CHATTACHEDLABEL:
			return wxT("CHATTACHEDLABEL");
		case WBOOK_RT_CHSERIESTEXT:
			return wxT("CHSERIESTEXT");
		case WBOOK_RT_CHCHARTFORMAT:
			return wxT("CHCHARTFORMAT");
		case WBOOK_RT_CHLEGEND:
			return wxT("CHLEGEND");
		case WBOOK_RT_CHSERIESLIST:
			return wxT("CHSERIESLIST");
		case WBOOK_RT_CHBAR:
			return wxT("CHBAR");
		case WBOOK_RT_CHLINE:
			return wxT("CHLINE");
		case WBOOK_RT_CHPIE:
			return wxT("CHPIE");
		case WBOOK_RT_CHAREA:
			return wxT("CHAREA");
		case WBOOK_RT_CHSCATTER:
			return wxT("CHSCATTER");
		case WBOOK_RT_CHCHARTLINE:
			return wxT("CHCHARTLINE");
		case WBOOK_RT_CHAXIS:
			return wxT("CHAXIS");
		case WBOOK_RT_CHTICK:
			return wxT("CHTICK");
		case WBOOK_RT_CHVALUERANGE:
			return wxT("CHVALUERANGE");
		case WBOOK_RT_CHCATSERRANGE:
			return wxT("CHCATSERRANGE");
		case WBOOK_RT_CHAXISLINEFORMAT:
			return wxT("CHAXISLINEFORMAT");
		case WBOOK_RT_CHCHARTFORMATLINK:
			return wxT("CHCHARTFORMATLINK");
		case WBOOK_RT_CHDEFAULTTEXT:
			return wxT("CHDEFAULTTEXT");
		case WBOOK_RT_CHTEXT:
			return wxT("CHTEXT");
		case WBOOK_RT_CHFONTX:
			return wxT("CHFONTX");
		case WBOOK_RT_CHOBJECTLINK:
			return wxT("CHOBJECTLINK");
		case WBOOK_RT_CHFRAME:
			return wxT("CHFRAME");
		case WBOOK_RT_CHBEGIN:
			return wxT("CHBEGIN");
		case WBOOK_RT_CHEND:
			return wxT("CHEND");
		case WBOOK_RT_CHPLOTAREA:
			return wxT("CHPLOTAREA");
		case WBOOK_RT_CH3D:
			return wxT("CH3D");
		case WBOOK_RT_CHPICF:
			return wxT("CHPICF");
		case WBOOK_RT_CHDROPBAR:
			return wxT("CHDROPBAR");
		case WBOOK_RT_CHRADAR:
			return wxT("CHRADAR");
		case WBOOK_RT_CHSURFACE:
			return wxT("CHSURFACE");
		case WBOOK_RT_CHRADARAREA:
			return wxT("CHRADARAREA");
		case WBOOK_RT_CHAXISPARENT:
			return wxT("CHAXISPARENT");
		case WBOOK_RT_CHLEGENDXN:
			return wxT("CHLEGENDXN");
		case WBOOK_RT_CHSHTPROPS:
			return wxT("CHSHTPROPS");
		case WBOOK_RT_CHSERTOCRT:
			return wxT("CHSERTOCRT");
		case WBOOK_RT_CHAXESUSED:
			return wxT("CHAXESUSED");
		case WBOOK_RT_CHSBASEREF:
			return wxT("CHSBASEREF");
		case WBOOK_RT_CHSERPARENT:
			return wxT("CHSERPARENT");
		case WBOOK_RT_CHSERAUXTREND:
			return wxT("CHSERAUXTREND");
		case WBOOK_RT_CHIFMT:
			return wxT("CHIFMT");
		case WBOOK_RT_CHPOS:
			return wxT("CHPOS");
		case WBOOK_RT_CHALRUNS:
			return wxT("CHALRUNS");
		case WBOOK_RT_CHAI:
			return wxT("CHAI");
		case WBOOK_RT_CHSERAUXERRBAR:
			return wxT("CHSERAUXERRBAR");
		case WBOOK_RT_CHSERFMT:
			return wxT("CHSERFMT");
		case WBOOK_RT_CHFBI:
			return wxT("CHFBI");
		case WBOOK_RT_CHBOPPOP:
			return wxT("CHBOPPOP");
		case WBOOK_RT_CHAXCEXT:
			return wxT("CHAXCEXT");
		case WBOOK_RT_CHDAT:
			return wxT("CHDAT");
		case WBOOK_RT_CHPLOTGROWTH:
			return wxT("CHPLOTGROWTH");
		case WBOOK_RT_CHSIINDEX:
			return wxT("CHSIINDEX");
		case WBOOK_RT_CHGELFRAME:
			return wxT("CHGELFRAME");
		case WBOOK_RT_CHBOPPOPCUSTOM:
			return wxT("CHBOPPOPCUSTOM");

		// manually defined
		default:
			return wxT("UNKNOWN");
	}
}


wxChar *Workbook::HumanReadableRKType(ULONG encValue)
{
	switch (encValue & 0x3)
	{
		case WBOOK_RKT_IEEE:
			return wxT("IEEE Number");
			break;

		case WBOOK_RKT_IEEEx100:
			return wxT("IEEE Number x 100");
			break;

		case WBOOK_RKT_INT:
			return wxT("Integer");
			break;

		case WBOOK_RKT_INTx100:
			return wxT("Integer x 100");
			break;

		default:
			return wxT("UKNOWN");
	}
}


void Workbook::DecodeString(BYTE *pByte, USHORT cch, USHORT grbit, wxString &str)
{
	// XXX: support other options, encodings, etc...

	// is the string "compressed unicode" or not?
	if (grbit & 0x1)
	{
		// not compressed - unicode string
		wchar_t *rgch = (wchar_t *)pByte;

		wxMBConvUTF16 u16;
		str = u16.cWC2WX(rgch);
		str.Truncate(cch);
	}
	else
	{
		// compressed -- ansi string
		char *rgch = (char *)pByte;
		str = wxString::From8BitData(rgch, cch);
	}
}


size_t Workbook::GetStringLength(USHORT cch, USHORT grbit)
{
	// XXX: support other options, encodings, etc...
	if (grbit & 0x1)
	{
		/* non-compressed unicode */
		// XXX: integer overflow (DO NOT USE THIS FUNCTION TO ALLOCATE MEMORY)
		return cch * sizeof(WORD);
	}
	return cch;
}


double Workbook::RKDecode(ULONG encValue)
{
	bool bit_a = (encValue & 0x1);
	bool bit_b = (encValue & 0x2) != 0;
	double ret = 0;

	// shift off these bits
	encValue >>= 2;
	if (bit_b)
		ret = encValue;
	else
		// set the 30 most-significant bits only
		*((ULONG *)&ret + 1) = encValue;

	if (bit_a)
	  ret /= 100.0;

	return ret;
}


DECLARE_CBF_PLUGIN(Workbook);
