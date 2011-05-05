/*
 * fileDissect - a cross platform file dissection tool
 * Joshua J. Drake <jdrake idefense.com>
 *
 * fileDissectTree.cpp:
 * Tree Control code
 */
#include "fileDissectTree.h"
#include "fileDissectGUI.h"
#include "fileDissectApp.h"

BEGIN_EVENT_TABLE(fileDissectTreeCtrl, wxTreeCtrl)
  EVT_TREE_ITEM_COLLAPSING(IDC_TREE, fileDissectTreeCtrl::OnItemCollapsing)
  EVT_TREE_ITEM_MENU(IDC_TREE, fileDissectTreeCtrl::OnItemMenu)
  EVT_TREE_ITEM_RIGHT_CLICK(IDC_TREE, fileDissectTreeCtrl::OnRightClick)
  EVT_TREE_SEL_CHANGED(IDC_TREE, fileDissectTreeCtrl::OnSelChanged)
END_EVENT_TABLE();

/* implement the class */
IMPLEMENT_DYNAMIC_CLASS(fileDissectTreeCtrl, wxTreeCtrl);

/* === member functions ==== */

// constructor
fileDissectTreeCtrl::fileDissectTreeCtrl(wxWindow *parent, const wxWindowID id,
			     const wxPoint& pos, const wxSize& size,
			     long style)
  : wxTreeCtrl(parent, id, pos, size, style)
{
	// CreateImageList(24);
}


#if 0
// create the image list
void fileDissectTreeCtrl::CreateImageList(int WXUNUSED(size))
{
}
#endif


// item collapse event handler
void fileDissectTreeCtrl::OnItemCollapsing(wxTreeEvent& event)
{
	wxTreeItemId id = event.GetItem();

	// prevent the root from collapsing
	if (id == GetRootItem())
	{
		event.Veto();
		return;
	}
}

// context menu event
void fileDissectTreeCtrl::OnItemMenu(wxTreeEvent& event)
{
	wxMenu mnuPopup;
	wxTreeItemId id = event.GetItem();
	wxMenuItem *pm;

	pm = mnuPopup.Append(IDM_NODE_EXPANDBELOW, wxT("E&xpand children"));
	if (!HasChildren(id))
		pm->Enable(false);

	pm = mnuPopup.Append(IDM_NODE_HIGHLIGHT, wxT("&Highlight"));
	if (!GetItemData(id))
		pm->Enable(false);

	wxPoint pt = event.GetPoint();
    PopupMenu(&mnuPopup, pt);
}


void fileDissectTreeCtrl::OnRightClick(wxTreeEvent& event)
{
	wxTreeItemId id = event.GetItem();
	
	SelectItem(id);
}

void fileDissectTreeCtrl::OnSelChanged(wxTreeEvent& event)
{
	wxTreeItemId id = event.GetItem();
	wxGetApp().m_frame->HighlightItem(id);
}
