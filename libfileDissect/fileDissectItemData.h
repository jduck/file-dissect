/*
 * fileDissect - a cross platform file dissection tool
 * Joshua J. Drake <jdrake idefense.com>
 *
 * fileDissectItemData.h:
 * tree item data declarations
 */
#ifndef __fileDissectItemData_h_
#define __fileDissectItemData_h_

#include "fileDissectSel.h"
#include <wx/treebase.h>

// macro to calculate the offset into a structure
#define FDT_OFFSET_OF(member,stru)	((wxByte *)&(stru.member) - (wxByte *)&stru)
#define FDT_SIZE_OF(member,stru)	(sizeof(stru.member))

// macro to make a new item for this structure member
#define FDT_NEW(member,stru)			new fdTIData( FDT_OFFSET_OF(member,stru), FDT_SIZE_OF(member,stru))
#define FDT_NEW_OFF(off,member,stru)	new fdTIData( off + FDT_OFFSET_OF(member,stru), FDT_SIZE_OF(member,stru))


/*
 * the item data class for our tree 
 */
class fdTIData : public wxTreeItemData
{
public:
	__declspec(dllexport) fdTIData(const wxFileOffset offset_start, const wxFileOffset length);
	__declspec(dllexport) ~fdTIData();

	typedef fileDissectSelList::iterator iterator;
	__declspec(dllexport) iterator begin(void);
	__declspec(dllexport) iterator end(void);

private:
	// list of selected bytes
	fileDissectSelList m_selected;
};

#endif
