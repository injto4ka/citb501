#ifndef __INPUT_H_
#define __INPUT_H_

#include <windows.h>			// Windows API Definitions
#include <list>
#include <string>

#include "graphics.h"

#define FONT_FACE_SIZE 32

struct Mouse{
	BOOL				lbutton;					// Mouse Left Button
	BOOL				mbutton;					// Mouse Middle Button
	BOOL				rbutton;					// Mouse Right Button
	WORD				x;							// Mouse X Position	
	WORD				y;							// Mouse Y Position
	short				wheel;						// Mouse Wheel
	BOOL				shift;						// Shift Button Pressed While Mouse Event
	BOOL				ctrl;						// Control Button Pressed While Mouse Event
};

struct Keyboard{	
	BYTE				code;						// Virtual Key Code Of The Last Button Pressed (VK)
	SHORT				repeatCount;				// repeat m_nCount
	BYTE				scanCode;					// scan code
	BOOL				extendKey;					// extended-key flag
	BOOL				alt;						// context code
	BOOL				repeated;					// previous key-state flag
	BOOL				pressed;					// transition-state flag
	
};

enum InputType { InputKey, InputChar, InputMouse };

struct Input{
	UINT uMsg;
	InputType eType;
	SHORT nSymbol;
	Mouse mouse;
	Keyboard keyboard;
};

enum TextAlign {
	ALIGN_CENTER,
	ALIGN_LEFT,
	ALIGN_RIGHT,
	ALIGN_TOP,
	ALIGN_BOTTOM,
};

class Font
{
	GLuint m_uList; // Display List
public:
	int					m_nHeight; // Font Height (Based On Character (-) Or Cell (+) For Windows Fonts)
	int					m_nWidth; // Font Width (0 For Automatic Match For Windows Fonts)
	int					m_nFirst; // First character
	int					m_nCount; // Number Of Characters
	int					m_nExpand;
	char				m_pchFace[FONT_FACE_SIZE]; // Font Name
	DWORD				m_uCharset; // ANSI_CHARSET, DEFAULT_CHARSET, SYMBOL_CHARSET, SHIFTJIS_CHARSET, HANGEUL_CHARSET, HANGUL_CHARSET, GB2312_CHARSET, CHINESEBIG5_CHARSET, OEM_CHARSET, JOHAB_CHARSET, HEBREW_CHARSET, ARABIC_CHARSET, GREEK_CHARSET, TURKISH_CHARSET, VIETNAMESE_CHARSET, THAI_CHARSET, EASTEUROPE_CHARSET, RUSSIAN_CHARSET, MAC_CHARSET, BALTIC_CHARSET
	BYTE				m_uQuality; // ANTIALIASED_QUALITY, CLEARTYPE_QUALITY, DEFAULT_QUALITY, DRAFT_QUALITY, NONANTIALIASED_QUALITY, PROOF_QUALITY
	BOOL				m_bBold;
	BOOL				m_bItalic;
	BOOL				m_bUnderline;
	BOOL				m_bStrike;
	int					m_nEscapement; // Angle Of Escapement
	int					m_nOrientation; // Base-line m_nOrientation angle
	TEXTMETRIC			m_textMetrix; // font size data
	INT					*m_pnCharWidths; // Individual widths of the chars
	
	Font(const char *_face, int m_nHeight);
	~Font();

	ErrorCode Create(HDC	hDC);
	void Destroy();

	int GetRowHeight() const { return m_textMetrix.tmHeight + m_textMetrix.tmInternalLeading; }
	int GetTextWidth(const char *pchText, int nIndex = 0, int nLength = -1) const;
	int GetTextIndex(const char *pchText, float m_nWidth, int nIndex = 0, int nLength = -1) const;
	void Print(const char *pchText, float x = 0, float y = 0, int nColor = 0, int eAlignH = ALIGN_LEFT, int eAlignV = ALIGN_BOTTOM);

	bool IsLoaded() const { return !!m_uList; }
	operator bool() const { return IsLoaded(); }
};



class Control
{
protected:
	bool m_bOver;
	virtual void Draw() const {}
	virtual void AdjustSize(){}
	virtual void OnMousePos(int x, int y, BOOL click){}
	virtual void OnMouseExit(){}
	virtual void OnMouseEnter(){}
	Control *m_pOwner;
	std::vector<Control *> m_vChilds;
public:
	int m_nBottom, m_nLeft, m_nWidth, m_nHeight, m_nAnchorRight, m_nAnchorTop;
	bool m_bVisible, m_bDisabled, m_bAutoSize;
	int m_nZ;
	Control():
		m_nZ(0), m_nBottom(0), m_nLeft(0), m_nWidth(-1), m_nHeight(-1),
		m_nAnchorRight(-1), m_nAnchorTop(-1),
		m_bVisible(true), m_bOver(false), m_bDisabled(false), m_bAutoSize(false),
		m_pOwner(NULL)
	{}
	void ClientToScreen(int &x, int &y) const
	{
		x += m_nLeft;
		y += m_nBottom;
		if( m_pOwner )
			m_pOwner->ClientToScreen(x, y);
	}
	void ScreenToClient(int &x, int &y) const
	{
		if( m_pOwner )
			m_pOwner->ScreenToClient(x, y);
		x -= m_nLeft;
		y -= m_nBottom;
	}
	void SetBounds(int nLeft, int nBottom, int nWidth, int nHeight);
	void SetAnchor(int nRight, int nTop);
	void Add(Control *child);
	void _Draw();
	bool _OnMousePos(int x, int y, BOOL click);
	void _AdjustSize(int nWinWidth, int nWinHeight);
	void _Invalidate();
	bool operator < (const Control& other) const { return m_nZ < other.m_nZ; }
	void CopyTo(Control& other) const;
	virtual void Invalidate(){}
};

class Panel: public Control
{
protected:
	virtual void Draw() const 
	{
		DrawBounds(m_nBackColor, m_nBorderColor, m_nBorderWidth);
	}
	void DrawBounds(int nBackColor, int nBorderColor, int nBorderWidth) const;
public:
	int m_nBorderColor, m_nBackColor, m_nBorderWidth;
	Panel():m_nBorderColor(0), m_nBackColor(0), m_nBorderWidth(1){}
	void CopyTo(Panel& other) const;
};

class Slider : public Panel
{
protected:
	virtual void Draw() const;
	virtual void OnMousePos(int x, int y, BOOL click);
	virtual void OnChanged(){ if (m_pOnChanged) m_pOnChanged(); }
public:
	int m_nForeColor, m_nMarginX, m_nMarginY;
	float m_fMin, m_fMax, m_fValue;
	Slider() :m_nForeColor(0xff00ff00), m_fMin(0), m_fMax(1), m_fValue(0.5), m_nMarginX(5), m_nMarginY(5), m_pOnChanged(NULL)
	{}
	float GetRatio() const
	{
		return Clamp((m_fValue - m_fMin) / (m_fMax - m_fMin), 0.0f, 1.0f);
	}
	void CopyTo(Slider& other) const;
	void(*m_pOnChanged)();
};

class Label: public Panel
{
protected:
	virtual void Draw() const 
	{
		DrawBounds(m_nBackColor, m_nBorderColor, m_nBorderWidth);
		DrawText(m_nForeColor);
	}
	void DrawText(int nTextColor) const;
	virtual void AdjustSize();
public:
	Font *m_pFont;
	std::string m_strText;
	int m_nForeColor, m_eAlignH, m_eAlignV, m_nMarginX, m_nMarginY, m_nOffsetX, m_nOffsetY;
	Label():
		m_pFont(NULL), m_nForeColor(0xff000000),
		m_eAlignH(ALIGN_CENTER), m_eAlignV(ALIGN_CENTER),
		m_nMarginX(0), m_nMarginY(0),
		m_nOffsetX(0), m_nOffsetY(0)
	{}
	void CopyTo(Label& other) const;
};

class SliderBar : public Panel
{
public:
	const char *m_pchFormat;
	Slider m_slider;
	Label m_name, m_value;
	SliderBar():m_pchFormat("%d")
	{
		Add(&m_slider);
		Add(&m_name);
		Add(&m_value);
	}
	void CopyTo(SliderBar &other) const;
	virtual void Invalidate();
};

class Button: public Label
{
protected:
	bool m_bClick, m_bWaitClick;
	virtual void OnMousePos(int x, int y, BOOL click);
	virtual void OnMouseExit();
	virtual void OnMouseEnter();
	virtual void OnClick(){ if(m_pOnClick) m_pOnClick();}
	virtual void Draw() const 
	{
		DrawBounds((m_bClick && m_nClickColor) ? m_nClickColor : ((m_bOver && m_nOverColor) ? m_nOverColor : m_nBackColor), m_nBorderColor, m_nBorderWidth);
		DrawText(m_nForeColor);
	}
public:
	int m_nClickColor, m_nOverColor;
	void (*m_pOnClick)();
	Button():m_nOverColor(0), m_nClickColor(0), m_bClick(false), m_bWaitClick(false), m_pOnClick(NULL){}
	void CopyTo(Button& other) const;
};

class CheckBox : public Button
{
protected:
	virtual void OnMousePos(int x, int y, BOOL click);
	virtual void Draw() const 
	{
		DrawBounds(
			(m_bClick && m_nClickColor) ?
				m_nClickColor :
				((m_bOver && m_nOverColor) ?
					m_nOverColor :
					(m_bChecked && m_nCheckColor ?
						m_nCheckColor :
						m_nBackColor)),
			m_nBorderColor, m_nBorderWidth);
		DrawText(m_nForeColor);
	}
public:
	bool m_bChecked;
	int m_nCheckColor;
	CheckBox() :m_nCheckColor(0), m_bChecked(false) {}
	void CopyTo(CheckBox& other) const;
};


#endif __INPUT_H_