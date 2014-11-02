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
int Font::GetTextWidth(const char *pchText, int nIndex, int nLength) const
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
	int m_nWidth=0;
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
void Font::Print(const char *pchText, float x, float y, int nColor, int eAlignH, int eAlignV)
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

	float alignedX;
	switch(eAlignH)
	{
	case ALIGN_CENTER:
		alignedX = x - 0.5f * GetTextWidth(pchText, 0, nLength);
		break;
	case ALIGN_RIGHT:
		alignedX = x - GetTextWidth(pchText, 0, nLength);
		break;
	default:
		alignedX = x;
	}
	float alignedY;
	switch(eAlignV)
	{
	case ALIGN_CENTER:
		alignedY = y - m_textMetrix.tmHeight / 2;
		break;
	case ALIGN_TOP:
		alignedY = y - m_textMetrix.tmAscent;
		break;
	default:
		alignedY = y + m_textMetrix.tmDescent;
	}
	glDisable(GL_TEXTURE_2D); // Enable Texture Mapping
	glDisable(GL_LIGHTING);
	glRasterPos2f(alignedX, alignedY);

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

void Control::Add(Control *child)
{
	child->m_pOwner = this;
	m_lChilds.push_back(child);
	m_lChilds.sort();
}
void Control::_AdjustSize()
{
	AdjustSize();
	for(auto it = m_lChilds.begin(); it != m_lChilds.end(); it++)
		(*it)->_AdjustSize();
}
void Control::_Draw()
{
	if( !m_bVisible )
		return;
	glPushAttrib(GL_ENABLE_BIT);
	glPushAttrib(GL_SCISSOR_BIT);
	if( m_nWidth && m_nHeight )
	{
		glEnable(GL_SCISSOR_TEST);
		GLint box[4] = {};
		glGetIntegerv(GL_SCISSOR_BOX, box);
		int x = 0, y = 0;
		ClientToScreen(x, y);
		int nLeft = max(box[0], x - 1),
			nBottom = max(box[1], y - 1),
			nRight = min(box[0] + box[2], x + m_nWidth + 1),
			nTop = min(box[1] + box[3], y + m_nHeight + 1);
		glScissor(nLeft, nBottom, nRight - nLeft, nTop - nBottom);
	}
	Draw();
	for(auto it = m_lChilds.begin(); it != m_lChilds.end(); it++)
		(*it)->_Draw();
	glPopAttrib();
	glPopAttrib();
}
bool Control::_OnMousePos(int x, int y, BOOL click)
{
	x -= m_nLeft;
	y -= m_nBottom;
	if( m_bDisabled || x < 0 || m_nWidth && x >= m_nWidth || y < 0 || m_nHeight && y >= m_nHeight )
	{
		if( m_bOver )
		{
			m_bOver = false;
			OnMouseExit();
			for(auto it = m_lChilds.begin(); it != m_lChilds.end(); it++)
			{
				Control *pChild = *it;
				if( pChild->m_bOver )
				{
					pChild->m_bOver = false;
					pChild->OnMouseExit();
				}
			}
		}
		return false;
	}
	else
	{
		if( !m_bOver )
		{
			m_bOver = true;
			OnMouseEnter();
		}
		bool bHandled = false;
		for(auto it = m_lChilds.begin(); it != m_lChilds.end(); it++)
			bHandled = (*it)->_OnMousePos(x, y, click) || bHandled;
		if( bHandled )
			return true;
		OnMousePos(x, y, click);
		return m_nWidth && m_nHeight;
	}
}
void Control::SetBounds(int nLeft, int nBottom, int nWidth, int nHeight)
{
	m_nWidth = nWidth;
	m_nHeight = nHeight;
	m_nLeft = nLeft;
	m_nBottom = nBottom;
}
void Panel::DrawBounds(int nBackColor, int nBorderColor, int nBorderWidth)
{
	int x = 0, y = 0;
	ClientToScreen(x, y);
	if( m_nBackColor )
		FillBox((float)x, (float)y, (float)m_nWidth, (float)m_nHeight, nBackColor);
	if( m_nBorderColor && nBorderWidth )
		DrawBox((float)x, (float)y, (float)m_nWidth, (float)m_nHeight, nBorderColor, (float)nBorderWidth);
}
void Label::DrawText(int nTextColor)
{
	if( !m_pFont || !nTextColor || m_strText.length() == 0 )
		return;
	int x = 0, y = 0;
	ClientToScreen(x, y);
	switch(m_eAlignH)
	{
		case ALIGN_LEFT:
			x += m_nMarginX;
			break;
		case ALIGN_RIGHT:
			x += m_nWidth - m_nMarginX;
			break;
		default:
			x += m_nWidth / 2;
	}
	switch(m_eAlignV)
	{
		case ALIGN_BOTTOM:
			y += m_nMarginY;
			break;
		case ALIGN_TOP:
			y += m_nHeight - m_nMarginY;
			break;
		default:
			y += m_nHeight / 2;
	}
	m_pFont->Print(m_strText.c_str(), (float)(x + m_nOffsetX), (float)(y + m_nOffsetY), nTextColor, m_eAlignH, m_eAlignV);
}
void Label::AdjustSize()
{
	if( !m_pFont || !m_pFont->IsLoaded() )
		return;
	m_nWidth = m_pFont->GetTextWidth(m_strText.c_str()) + 2*m_nMarginX;
	m_nHeight = m_pFont->GetRowHeight() + 2*m_nMarginY;
}
void Button::OnMousePos(int x, int y, BOOL click)
{
	if( m_bWaitClick )
	{
		if( click )
			return;
		m_bWaitClick = false;
	}
	if( click )
	{
		m_bClick = true;
	}
	else if( m_bClick )
	{
		m_bClick = false;
		OnClick();
	}
}
void Button::OnMouseExit()
{
	m_bClick = false;
}
void Button::OnMouseEnter()
{
	m_bWaitClick = true;
}