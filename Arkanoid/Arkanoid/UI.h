#ifndef __INPUT_H_
#define __INPUT_H_

#include <windows.h>			// Windows API Definitions

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

class Font
{
	GLuint m_uList; // Display List
public:
	enum TextAlign {
		ALIGN_LEFT,
		ALIGN_CENTRE,
		ALIGN_RIGHT
	};
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

	float GetTextWidth(const char *pchText, int nIndex = 0, int nLength = -1) const;
	int GetTextIndex(const char *pchText, float m_nWidth, int nIndex = 0, int nLength = -1) const;
	void Print(const char *pchText, float x = 0, float y = 0, int nColor = 0, int nAlign = ALIGN_LEFT);

	operator bool() const {return !!m_uList;}
};

#endif __INPUT_H_