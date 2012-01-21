/*
 * fileDissect - a cross platform file dissection tool
 * Joshua J. Drake <jdrake idefense.com>
 *
 * fileDissectItemData.cpp:
 * Tree Item Data code
 */

#include "fileDissectItemData.h"


fdTIData::fdTIData(const wxFileOffset offset_start, const wxFileOffset length)
{
	m_selected.AddToSelection(offset_start, offset_start + length);
};


fdTIData::~fdTIData()
{
	m_selected.clear();
}


fdTIData::iterator fdTIData::begin(void)
{
	return m_selected.begin();
}


fdTIData::iterator fdTIData::end(void)
{
	return m_selected.end();
}
