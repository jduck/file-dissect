/*
 * fileDissect - a cross platform file dissection tool
 * Joshua J. Drake <jdrake idefense.com>
 *
 * fileDissectTree.h:
 * tree view declarations
 */
#ifndef __fileDissectTree_h_
#define __fileDissectTree_h_

#include "fileDissect.h"		// wxWidgets base
#include <wx/treectrl.h> 		// tree control
#include "fileDissectSel.h"		// selection list


// macro to calculate the offset into a structure
#define FDT_OFFSET_OF(member,stru) \
	((wxByte *)&(stru.member) - (wxByte *)&stru)
#define FDT_SIZE_OF(member,stru) \
	(sizeof(stru.member))
// macro to make a new item for this structure member
#define FDT_NEW(member,stru) \
	new fdTIData( \
		FDT_OFFSET_OF(member,stru), \
		FDT_SIZE_OF(member,stru))

/* 
 * the item data class for our tree 
 */
class fdTIData : public wxTreeItemData
{
public:
	fdTIData(const wxFileOffset offset_start, const wxFileOffset length)
	{
		m_selected.AddToSelection(offset_start, offset_start + length);
	};

	// list of selected bytes
	fileDissectSelList m_selected;
};


/* 
 * the tree holding our data 
 */
class fileDissectTreeCtrl : public wxTreeCtrl
{
 public:
   fileDissectTreeCtrl()
     {
     }
   
   fileDissectTreeCtrl(wxWindow *parent, const wxWindowID id,
	      const wxPoint& pos, const wxSize& size,
	      long style);

#if 0
   // images..
   void CreateImageList(int);
#endif

   // event handlers
   void OnItemCollapsing(wxTreeEvent& event);
   void OnItemMenu(wxTreeEvent& event);
   void OnRightClick(wxTreeEvent& event);
   void OnSelChanged(wxTreeEvent& event);

   DECLARE_DYNAMIC_CLASS(fileDissectTreeCtrl)
   DECLARE_EVENT_TABLE()
};

#endif
