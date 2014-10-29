#include "UI.h"

Font::Font(const char *_face, int _height):
		m_nHeight(_height),m_nWidth(0), m_nFirst(0),m_nCount(0),m_nExpand(0),
		m_uCharset(ANSI_CHARSET),m_uQuality(ANTIALIASED_QUALITY),
		m_bBold(FALSE),m_bItalic(FALSE),m_bUnderline(FALSE),m_bStrike(FALSE),
		m_nEscapement(0),m_nOrientation(0),
		m_pnCharWidths(NULL)
{
	FORMAT(m_pchFace, _face);
	ZeroMemory(m_pchFace,FONT_FACE_SIZE);
}

Font::~Font()
{
	Destroy();
}

ErrorCode Font::Create(HDC hDC) 
{
	Destroy();
	if(!hDC)
		return "No device context";
	if(m_nFirst<0)
		m_nFirst=0;
	if(m_nCount<=0)
		m_nCount=256;
	GLuint uList = glGenLists(m_nCount); // Storage For m_nCount Characters
	if( !uList || glGetError() )
		return "Failed to create list";
	m_uList = uList;
	int nWeight = m_bBold ? FW_BOLD : 0;
	HFONT fontID = CreateFont(m_nHeight, // Height Of Font, Based On Character (-) Or Cell (+)
						m_nWidth, // Width Of Font (0 = use default value)
						m_nEscapement, // Angle Of Escapement
						m_nOrientation, // Orientation Angle
						nWeight, // Font Weight
						m_bItalic, // Italic
						m_bUnderline, // Underline
						m_bStrike, // Strikeout
						m_uCharset, // Character Set Identifier
						OUT_TT_PRECIS, // TrueType Output Precision
						CLIP_DEFAULT_PRECIS, // Clipping Precision
						m_uQuality, // Output Quality
						FF_DONTCARE|DEFAULT_PITCH, // Pitch And Family
						m_pchFace); // Font Face Name
	if(!fontID)
	{
		Destroy();
		return "Failed to create font";
	}
	HFONT oldFontID = (HFONT)SelectObject(hDC, fontID); // Selects The Font We Want
	if( !GetTextMetrics(hDC, &m_textMetrix) )
	{
		SelectObject(hDC, oldFontID); // Restore The Old Font
		DeleteObject(fontID); // Delete The Font
		Destroy();
		return "Failed to get text metrics";
	}
	m_pnCharWidths = new INT[m_nCount];
	if(!GetCharWidth32(hDC, m_nFirst, m_nFirst + m_nCount - 1, m_pnCharWidths) && !GetCharWidth(hDC, m_nFirst, m_nFirst + m_nCount - 1, m_pnCharWidths))
	{
		SelectObject(hDC, oldFontID); // Restore The Old Font
		DeleteObject(fontID); // Delete The Font
		Destroy();
		return "Failed to get char widths";
	}
	if(!wglUseFontBitmaps(hDC, m_nFirst, m_nCount, m_uList))
	{
		SelectObject(hDC, oldFontID); // Restore The Old Font
		DeleteObject(fontID); // Delete The Font
		Destroy();
		return "Failed to select bitmap font";
	}
	SelectObject(hDC, oldFontID); // Restore The Old Font
	DeleteObject(fontID); // Delete The Font
	return NULL;
}

void Font::Destroy() // Delete The Font List
{
	if(!m_uList)
		return;
	glDeleteLists(m_uList, m_nCount); // Delete All Characters
	m_uList = 0;
	if(m_pnCharWidths)
	{
		delete [] m_pnCharWidths;
		m_pnCharWidths = NULL;
	}
}
int Font::GetTextIndex(const char *pchText, float nWidth, int nIndex, int nLength) const
{
	if(!m_uList || nWidth < 0 || !m_pnCharWidths)
		return -1;
	int maxLength = StrLen(pchText);
	if(nIndex < 0|| nIndex >= maxLength)
		nIndex = 0;
	if(nLength < 0 || nLength > maxLength - nIndex)
		nLength = maxLength - nIndex;
	if(nLength <= 0)
		return -1;
	int code;
	pchText += nIndex;
	for(int i=0;i<nLength;i++)
	{
		code = pchText[i] - m_nFirst;
		if(code < 0 || code >= m_nCount)
			return 0;
		nWidth -= m_pnCharWidths[code];
		if(nWidth < 0)
			return nIndex + i - 1;
	}
	return nIndex + nLength;
}
float Font::GetTextWidth(const char *pchText, int nIndex, int nLength) const
{
	if(!pchText || !m_uList || !m_pnCharWidths)
		return 0;
	if(nIndex < 0)
		nIndex = 0;
	if(nLength < 0)
		nLength = StrLen(pchText) - nIndex;
	if(nLength <= 0)
		return FALSE;

	int code;
	float m_nWidth=0;
	pchText += nIndex;
	for(int i=0 ;i< nLength; i++) // Loop To Find Text Length
	{
		code = pchText[i] - m_nFirst;
		if(code < 0 || code >= m_nCount)
			return 0;
		m_nWidth += m_pnCharWidths[code];
	}
	return m_nWidth;
}
void Font::Print(const char *pchText, float x, float y, int nColor, int nAlign)
{
	if(!m_uList)
		return;
	int nLength = StrLen(pchText);
	if(!nLength)
		return;

	glPushAttrib(GL_ENABLE_BIT); // Push The Enable Bits
	glPushAttrib(GL_CURRENT_BIT); // Push The Current Bits
	glMatrixMode(GL_MODELVIEW); // Select The Modelview Matrix
	glPushMatrix(); // Store The Modelview Matrix

	if(nColor)
		SetColor(nColor);

	float alignedX = x;
	switch(nAlign)
	{
	case ALIGN_CENTRE:
		alignedX -= 0.5f * GetTextWidth(pchText, 0, nLength);
		break;
	case ALIGN_RIGHT:
		alignedX -= GetTextWidth(pchText, 0, nLength);
		break;
	}
	glDisable(GL_TEXTURE_2D); // Enable Texture Mapping
	glDisable(GL_LIGHTING);
	glRasterPos2f(alignedX, y);

	glPushAttrib(GL_LIST_BIT); // Pushes The Display List Bits To Keep Other Lists Inaffected
	glListBase(m_uList - m_nFirst + m_nExpand*128); // Sets The Base Character to m_nFirst
	// Draws The Display List Text
	glCallLists(nLength, GL_UNSIGNED_BYTE, pchText);
	glPopAttrib(); // Pops The Display List Bits
	
	glMatrixMode(GL_MODELVIEW); // Select The Modelview Matrix
	glPopMatrix(); // Restore The Old Modelview Matrix
	
	glPopAttrib(); // Pops The Current Bits
	glPopAttrib(); // Pops The Enable Bits
}

void Control::Add(Control &child)
{
	m_lChilds.push_back(&child);
	m_lChilds.sort();
}
void Control::_Draw(float x, float y)
{
	Draw(x, y);
	x += m_fLeft;
	y += m_fTop;
	for(auto it = m_lChilds.begin(); it != m_lChilds.end(); it++)
	{
		(*it)->_Draw(x, y);
	}
}