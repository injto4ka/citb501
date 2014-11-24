#include "UI.h"
#include <algorithm>

Font::Font(const char *_face, int _height):
		m_nHeight(_height),m_nWidth(0), m_nFirst(0),m_nCount(0),m_nExpand(0),
		m_uCharset(ANSI_CHARSET),m_uQuality(ANTIALIASED_QUALITY),
		m_bBold(FALSE),m_bItalic(FALSE),m_bUnderline(FALSE),m_bStrike(FALSE),
		m_nEscapement(0),m_nOrientation(0),
		m_pnCharWidths(NULL)
{
	FORMAT(m_pchFace, _face);
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
	int m_nWidth = m_textMetrix.tmOverhang;
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
	child->Invalidate();
	child->m_pOwner = this;
	m_vChilds.push_back(child);
	std::sort(m_vChilds.begin(), m_vChilds.end());
}
void Control::_AdjustSize(int nWinWidth, int nWinHeight)
{
	if( m_nAnchorRight >= 0 )
	{
		int nWidth = m_pOwner ? m_pOwner->m_nWidth : -1;
		if( nWidth < 0 )
			nWidth = nWinWidth;
		m_nWidth = nWidth - (m_nLeft + m_nAnchorRight);
	}
	if( m_nAnchorTop >= 0 )
	{
		int nHeight = m_pOwner ? m_pOwner->m_nHeight : -1;
		if( nHeight < 0 )
			nHeight = nWinHeight;
		m_nHeight = nHeight - (m_nBottom + m_nAnchorTop);
	}
	for(size_t i = 0; i < m_vChilds.size(); i++)
		m_vChilds[i]->_AdjustSize(nWinWidth, nWinHeight);
	if( m_bAutoSize )
		AdjustSize();
}
void Control::_Invalidate()
{
	Invalidate();
	for(size_t i = 0; i < m_vChilds.size(); i++)
		m_vChilds[i]->_Invalidate();
}
void Control::_Draw()
{
	if( !m_bVisible )
		return;
	if( m_nWidth < 0 || m_nHeight < 0 )
	{
		for(size_t i = 0; i < m_vChilds.size(); i++)
			m_vChilds[i]->_Draw();
	}
	else
	{
		glPushAttrib(GL_ENABLE_BIT);
		glPushAttrib(GL_SCISSOR_BIT);
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
		Draw();
		for(size_t i = 0; i < m_vChilds.size(); i++)
			m_vChilds[i]->_Draw();
		glPopAttrib();
		glPopAttrib();
	}
}

bool Control::_OnMousePos(int x, int y, BOOL click)
{
	if( !m_bVisible )
		return false;
	x -= m_nLeft;
	y -= m_nBottom;
	bool m_bValid = m_nWidth >= 0 && m_nHeight >= 0;
	if( m_bDisabled || m_bValid && (x < 0 || x >= m_nWidth || y < 0 || y >= m_nHeight) )
	{
		if( m_bOver )
		{
			m_bOver = false;
			OnMouseExit();
			for(size_t i = 0; i < m_vChilds.size(); i++)
			{
				Control *pChild = m_vChilds[i];
				if( pChild->m_bOver )
				{
					pChild->m_bOver = false;
					pChild->OnMouseExit();
				}
			}
		}
	}
	else
	{
		if( !m_bOver )
		{
			m_bOver = true;
			OnMouseEnter();
		}
		for(int i = (int)m_vChilds.size() - 1; i >= 0 ; i--)
			if (m_vChilds[i]->_OnMousePos(x, y, click))
				return true;
		if( m_bValid )
		{
			OnMousePos(x, y, click);
			return true;
		}
	}
	return false;
}
void Control::SetBounds(int nLeft, int nBottom, int nWidth, int nHeight)
{
	m_nWidth = nWidth;
	m_nHeight = nHeight;
	m_nLeft = nLeft;
	m_nBottom = nBottom;
}
void Control::CopyTo(Control& other) const
{
	other.m_nBottom = m_nBottom;
	other.m_nLeft = m_nLeft;
	other.m_nWidth = m_nWidth;
	other.m_nHeight = m_nHeight;
	other.m_bVisible = m_bVisible;
	other.m_bDisabled = m_bDisabled;
	other.m_bAutoSize = m_bAutoSize;
	other.m_nZ = m_nZ;
}
void Control::SetAnchor(int nRight, int nTop)
{
	m_nAnchorRight = nRight;
	m_nAnchorTop = nTop;
}
void Panel::DrawBounds(int nBackColor, int nBorderColor, int nBorderWidth) const
{
	int x = 0, y = 0;
	ClientToScreen(x, y);
	if (nBackColor)
		FillBox(x, y, m_nWidth, m_nHeight, nBackColor);
	if (nBorderColor && nBorderWidth)
		DrawBox(x, y, m_nWidth, m_nHeight, nBorderColor, (float)nBorderWidth);
}
void Panel::CopyTo(Panel& other) const
{
	Control::CopyTo(other);
	other.m_nBorderColor = m_nBorderColor;
	other.m_nBackColor = m_nBackColor;
	other.m_nBorderWidth = m_nBorderWidth;
}
void Label::DrawText(int nTextColor) const
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
void Label::CopyTo(Label& other) const
{
	Panel::CopyTo(other);
	other.m_strText = m_strText;
	other.m_pFont = m_pFont;
	other.m_nForeColor = m_nForeColor;
	other.m_eAlignH = m_eAlignH;
	other.m_eAlignV = m_eAlignV;
	other.m_nMarginX = m_nMarginX;
	other.m_nMarginY = m_nMarginY;
	other.m_nOffsetX = m_nOffsetX;
	other.m_nOffsetY = m_nOffsetY;
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
void CheckBox::OnMousePos(int x, int y, BOOL click)
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
		m_bChecked = !m_bChecked;
		OnClick();
	}
}
void CheckBox::CopyTo(CheckBox& other) const
{
	Button::CopyTo(other);
	other.m_nCheckColor = m_nCheckColor;
	other.m_bChecked = m_bChecked;
}
void Button::OnMouseExit()
{
	m_bClick = false;
}
void Button::OnMouseEnter()
{
	m_bWaitClick = true;
}
void Button::CopyTo(Button& other) const
{
	Label::CopyTo(other);
	other.m_nClickColor = m_nClickColor;
	other.m_nOverColor = m_nOverColor;
	other.m_pOnClick = m_pOnClick;
}
void Slider::Draw() const
{
	int x = 0, y = 0;
	ClientToScreen(x, y);
	if (m_nBackColor)
		FillBox(x, y, m_nWidth, m_nHeight, m_nBackColor);
	if (m_nForeColor)
		FillBox((float)x + m_nMarginX, (float)y + m_nMarginY, GetRatio()*(m_nWidth - 2*m_nMarginX), (float)m_nHeight - 2 * m_nMarginY, m_nForeColor);
	if (m_nBorderColor && m_nBorderWidth)
	{
		DrawBox(x, y, m_nWidth, m_nHeight, m_nBorderColor, (float)m_nBorderWidth);
		DrawBox(x + m_nMarginX, y + m_nMarginY, m_nWidth - 2 * m_nMarginX, m_nHeight - 2 * m_nMarginY, m_nBorderColor, (float)m_nBorderWidth);
	}
}
void Slider::OnMousePos(int x, int y, BOOL click)
{
	if (!click)
		return;
	float fNewValue;
	if (x > m_nWidth - m_nMarginX)
		fNewValue = m_fMax;
	else if (x < m_nMarginX)
		fNewValue = m_fMin;
	else
		fNewValue = m_fMin + (m_fMax - m_fMin) * (x - m_nMarginX) / (m_nWidth - 2*m_nMarginX);
	if (fNewValue != m_fValue)
	{
		m_fValue = fNewValue;
		if( OnValueChanged )
			OnValueChanged();
		if (m_pOwner)
			m_pOwner->Invalidate();
	}
}
void Slider::CopyTo(Slider& other) const
{
	Panel::CopyTo(other);
	other.m_nForeColor = m_nForeColor;
	other.m_nMarginX = m_nMarginX;
	other.m_nMarginY = m_nMarginY;
	other.m_fMin = m_fMin;
	other.m_fMax = m_fMax;
	other.m_fValue = m_fValue;
}
void SliderBar::Invalidate()
{
	if (m_pchFormat && *m_pchFormat)
	{
		char buff[128];
		m_value.m_strText = FORMAT(buff, m_pchFormat, m_slider.m_fValue);
	}
}
void SliderBar::CopyTo(SliderBar &other) const
{
	Panel::CopyTo(other);
	m_slider.CopyTo(other.m_slider);
	m_name.CopyTo(other.m_name);
	m_value.CopyTo(other.m_value);
	other.m_pchFormat = m_pchFormat;
}