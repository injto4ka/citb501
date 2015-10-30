#ifndef __INPUT_H_
#define __INPUT_H_

#include <windows.h>			// Windows API Definitions
#include <list>

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
	virtual void Draw(){}
	virtual void AdjustSize(){}
	virtual void OnMousePos(int x, int y, BOOL click){}
	virtual void OnMouseExit(){}
	virtual void OnMouseEnter(){}
public:
	int m_nBottom, m_nLeft, m_nWidth, m_nHeight;
	bool m_bVisible, m_bDisabled, m_bOver;
	int m_nZ;
	Control *m_pOwner;
	std::list<Control *> m_lChilds;
	Control():
		m_nZ(0), m_nBottom(0), m_nLeft(0), m_nWidth(0), m_nHeight(0),
		m_bVisible(true), m_bOver(false), m_bDisabled(false),
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
	void Add(Control *child);
	void _Draw();
	bool _OnMousePos(int x, int y, BOOL click);
	void _AdjustSize();
	bool operator < (const Control& other) const { return m_nZ < other.m_nZ; }
};

class Panel: public Control
{
protected:
	virtual void Draw()
	{
		DrawBounds(m_nBackColor, m_nBorderColor, m_nBorderWidth);
	}
	void DrawBounds(int nBackColor, int nBorderColor, int nBorderWidth);
public:
	int m_nBorderColor, m_nBackColor, m_nBorderWidth;
	Panel():m_nBorderColor(0), m_nBackColor(0), m_nBorderWidth(1){}
};

class Label: public Panel
{
protected:
	virtual void Draw()
	{
		DrawBounds(m_nBackColor, m_nBorderColor, m_nBorderWidth);
		DrawText(m_nForeColor);
	}
	void DrawText(int nTextColor);
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
};

class Button: public Label
{
protected:
	virtual void OnMousePos(int x, int y, BOOL click);
	virtual void OnMouseExit();
	virtual void OnMouseEnter();
	virtual void OnClick()
	{
		if (m_bChecked)
			m_bChecked = false;
		else
			m_bChecked = m_bCheckable;
		if (m_pOnClick)
			m_pOnClick();
	}
	virtual void Draw()
	{
		DrawBounds(((m_bClick || m_bChecked) && m_nClickColor) ? m_nClickColor : ((m_bOver && m_nOverColor) ? m_nOverColor : m_nBackColor), m_nBorderColor, m_nBorderWidth);
		DrawText(m_nForeColor);
	}
public:
	bool m_bClick, m_bWaitClick, m_bChecked, m_bCheckable;
	int m_nClickColor, m_nOverColor;
	void (*m_pOnClick)();
	Button():
		m_nOverColor(0), m_nClickColor(0), m_bClick(false),
		m_bWaitClick(false), m_pOnClick(NULL),
		m_bChecked(false), m_bCheckable(false)
	{}
};


#endif __INPUT_H_