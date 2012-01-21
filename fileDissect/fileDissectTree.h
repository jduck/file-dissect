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
#include "fileDissectItemData.h"


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
