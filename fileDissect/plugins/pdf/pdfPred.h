/*
 * Adobe Portable Document Format implementation
 * Joshua J. Drake <jdrake accuvant.com>
 * 
 * pdfPred.h:
 * class declaration for pdfPredInputStream class
 */
#ifndef __pdfPred_h_
#define __pdfPred_h_

#include <wx/stream.h>

class pdfPredInputStream : public wxFilterInputStream
{
public:
	pdfPredInputStream(wxInputStream &, unsigned long, unsigned long);
	pdfPredInputStream(wxInputStream *, unsigned long, unsigned long);
	~pdfPredInputStream();

private:
	void Init(void );

	wxByte *m_line;
	wxByte *m_prevln;
	wxByte *m_ln1;
	wxByte *m_ln2;
	wxByte *m_ptr;

protected:
	size_t OnSysRead(void *buffer, size_t size);

	unsigned long m_cols;
	unsigned long m_predictor;

	DECLARE_NO_COPY_CLASS(pdfPredInputStream)
};

#endif
