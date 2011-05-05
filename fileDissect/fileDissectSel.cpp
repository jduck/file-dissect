/*
 * fileDissect - a cross platform file dissection tool
 * Joshua J. Drake <jdrake idefense.com>
 *
 * fileDissectSel.cpp:
 * a list class of selected bytes in a file
 */
#include "fileDissectSel.h"

// actually define the internal list class
#include <wx/listimpl.cpp>
WX_DEFINE_LIST(fileDissectSelListInt);
