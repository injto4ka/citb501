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
	virtual void Draw(float x, float y){}
public:
	float m_fBottom, m_fLeft, m_fWidth, m_fHeight;
	bool m_bVisible;
	int m_nZ;
	Control *m_pOwner;
	std::list<Control *> m_lChilds;
	Control():m_nZ(0), m_fBottom(0), m_fLeft(0), m_fWidth(0), m_fHeight(0), m_bVisible(true), m_pOwner(NULL){}
	void SetBounds(float fLeft, float fBottom, float fWidth,float fHeight);
	void Add(Control *child);
	void _Draw(float x = 0, float y = 0);
	bool operator < (const Control& other) const { return m_nZ < other.m_nZ; }
};

class Panel: public Control
{
protected:
	virtual void Draw(float x, float y)
	{
		if( m_nBackColor )
			FillBox(x, y, m_fWidth, m_fHeight, m_nBackColor);
		if( m_nBorderColor )
			DrawBox(x, y, m_fWidth, m_fHeight, m_nBorderColor, m_fBorderWidth);
	}
public:
	std::string m_strText;
	Font *m_pFont;
	int m_nBorderColor, m_nBackColor;
	float m_fBorderWidth;
	Panel():m_nBorderColor(0), m_nBackColor(0), m_fBorderWidth(1.0f){}
};

class Label: public Panel
{
protected:
	virtual void Draw(float x, float y);
public:
	Font *m_pFont;
	std::string m_strText;
	int m_nForeColor, m_eAlignH, m_eAlignV;
	float m_fMarginX, m_fMarginY, m_fOffsetX, m_fOffsetY;
	Label():
		m_pFont(NULL), m_nForeColor(0xff000000),
		m_eAlignH(ALIGN_CENTER), m_eAlignV(ALIGN_CENTER),
		m_fMarginX(0), m_fMarginY(0),
		m_fOffsetX(0), m_fOffsetY(0)
	{}
	bool AdjustSize();
};

#endif __INPUT_H_