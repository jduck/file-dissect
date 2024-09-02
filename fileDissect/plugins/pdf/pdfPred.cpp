/*
 * Adobe Portable Document Format implementation
 * Joshua J. Drake <jdrake accuvant.com>
 * 
 * pdfPred.h:
 * implementation for pdfPredInputStream class
 */

#include "pdfPred.h"

pdfPredInputStream::pdfPredInputStream(wxInputStream& stream, unsigned long type, unsigned long cols)
	: wxFilterInputStream(stream), m_prevln(0),
	m_cols(cols), m_predictor(type)
{
	Init();
}

pdfPredInputStream::pdfPredInputStream(wxInputStream *stream, unsigned long type, unsigned long cols)
  : wxFilterInputStream(stream), m_prevln(0),
	m_cols(cols), m_predictor(type)
{
	Init();
}


pdfPredInputStream::~pdfPredInputStream(void)
{
	if (m_ln1)
		delete[] m_ln1;
	if (m_ln2)
		delete[] m_ln2;
}


void pdfPredInputStream::Init(void)
{
	m_ln1 = new wxByte[1 + m_cols];
	m_ln2 = new wxByte[1 + m_cols];
	m_line = m_ln1;
	m_ptr = NULL;
}


size_t pdfPredInputStream::OnSysRead(void *buffer, size_t size)
{
	if (!IsOk() || !size)
		return 0;

	wxByte *p_buf = (wxByte *)buffer;
	wxByte *p_out = p_buf;

	// what follows is a PNG predictor decoder that won't work well
	// XXX: TODO: This only implements PNG Up predictor..
	while ((size_t)(p_out - p_buf) < size && m_parent_i_stream->IsOk())
	{
		// do we have some data already ready?
		if (m_ptr)
		{
			while (m_ptr < m_prevln + m_cols + 1
				&& (size_t)(p_out - p_buf) < size)
				*p_out++ = *m_ptr++;
			if (m_ptr >= m_prevln + m_cols + 1)
				m_ptr = NULL;
		}
		else
		{
			// read another scanline of data
			m_parent_i_stream->Read(m_line, m_cols + 1);
			if (m_parent_i_stream->LastRead() < 1)
				break;

			// We don't support changing predictors, nor any others than this one.
			if (m_line[0] != 0x02)
				break;

			// decode it
			if (m_prevln)
			{
				for (unsigned long j = 1; j < m_cols + 1; j++)
					m_line[j] = m_line[j] + m_prevln[j];
			}

			// save the ptr to the data we have ready
			m_ptr = m_line + 1;

			// switch line to the other buffer
			m_prevln = m_line;
			if (m_line == m_ln1)
				m_line = m_ln2;
			else
				m_line = m_ln1;
		}
	}
	if (!m_parent_i_stream->IsOk())
		m_lasterror = wxSTREAM_EOF;
	return (p_out - p_buf);
}
