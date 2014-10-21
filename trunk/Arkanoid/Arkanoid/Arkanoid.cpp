#include <windows.h>			// Windows API Definitions
#include <stdio.h>
#include <deque>
#include <vector>
#include <math.h>
#include <gl/gl.h>									// Header File For The OpenGL32 Library
#include <gl/glu.h>									// Header File For The GLu32 Library

#pragma comment( lib, "opengl32.lib" )				// Search For OpenGL32.lib While Linking
#pragma comment( lib, "glu32.lib" )					// Search For GLu32.lib While Linking

#pragma warning(disable:4996)

#ifndef WM_TOGGLEFULLSCREEN							// Application Define Message For Toggling
#	define WM_TOGGLEFULLSCREEN (WM_USER+1)									
#endif

#ifndef PI
#	define PI 3.1415926535897932384626433832795f
#endif

#define MASK(start, end) ((~((~0L)<<((end)-(start)+1)))<<(start))
#define GET_BIT(number, index) ((((number)>>(index))&1) == 1)
#define SET_BIT(number, index, value) ((value) ? (number)|(1<<(index)) : (number)&(~(1<<(index))))
#define SET_BITS(number, start, end, bits) (((number)&~MASK((start), (end)))|((bits)<<(start)))
#define GET_BITS(number, start, end) (((number)&MASK((start), (end)))>>(start))

#define INT_TO_BYTE(color) ((BYTE*)(&(color)))
#define BOOL_TO_STR(boolean) ((boolean) ? "true" : "false")


enum RGB_COMPS
{
	RED, 
	GREEN, 
	BLUE, 
	ALPHA
};

/// the tga header (18 bytes!)
#pragma pack(push,1) // turn off data boundary alignment
struct TGAHeader
{
	// sometimes the tga file has a field with some custom info in. This 
	// just identifies the size of that field. If it is anything other
	// than zero, forget it.
	unsigned char m_iIdentificationFieldSize;
	// This field specifies if a colour map is present, 0-no, 1 yes...
	unsigned char m_iColourMapType;
	// only going to support RGB/RGBA/8bit - 2, colour mapped - 1
	unsigned char m_iImageTypeCode;
	// ignore this field....0
	unsigned short m_iColorMapOrigin;
	// size of the colour map
	unsigned short m_iColorMapLength;
	// bits per pixel of the colour map entries...
	unsigned char m_iColourMapEntrySize;
	// ignore this field..... 0
	unsigned short m_iX_Origin;
	// ignore this field..... 0
	unsigned short m_iY_Origin;
	// the image width....
	unsigned short m_iWidth;
	// the image height.... 
	unsigned short m_iHeight;
	// the bits per pixel of the image, 8,16,24 or 32
	unsigned char m_iBPP;
	// ignore this field.... 0
	unsigned char m_ImageDescriptorByte;
};
#pragma pack(pop)

#define PIXEL_COMP_OLD	0xFF
#define PIXEL_COMP_GRAY	0x01
#define PIXEL_COMP_RGB	0x03
#define PIXEL_COMP_RGBA	0x04

void SetMemAlign(int nMemWidth, BOOL bPack)
{
	if(nMemWidth <= 0)
		return;
	for(GLint nAlign = 8;;nAlign >>= 1)
	{
		if(nAlign == 1 || nMemWidth % nAlign == 0)
		{
			glPixelStorei(bPack ? GL_PACK_ALIGNMENT : GL_UNPACK_ALIGNMENT, nAlign);
			return;
		}
	}
}

class File
{
	FILE *pFile;
	int nId;
public:
	File():pFile(NULL), nId(-1) {}
	~File() { Close(); }
	BOOL Open(const char *pchFileName, const char *pchMode = "rb")
	{
		Close();
		if(pchFileName && pchMode)
		{
			pFile = fopen(pchFileName, pchMode);
			if(pFile)
			{
				nId = _fileno(pFile);
				return TRUE;
			}
		}
		return FALSE;
	}
	void Close()
	{
		if(!pFile)
			return;
		fclose(pFile);
		pFile = NULL;
		nId = -1;
	}
	int Descript() const
	{
		return nId;
	}
	operator FILE*(){ return pFile; }
	operator int(){ return nId; }
};

#define REPLACE(a,b) (a)^=(b)^=(a)^=(b)

class Image
{
	std::vector<char> m_vBuffer;
	WORD m_uWidth, m_uHeight;
	BYTE m_uComps;
public:
	Image():m_uWidth(0), m_uHeight(0), m_uComps(PIXEL_COMP_RGB){}

	char *GetDataPtr() { return m_vBuffer.size() > 0 ? &m_vBuffer[0] : NULL; }
	const char *GetDataPtr() const { return m_vBuffer.size() > 0 ? &m_vBuffer[0] : NULL; }
	size_t GetDataSize() const { return m_vBuffer.size(); }
	WORD GetWidth() const { return m_uWidth; }
	WORD GetHeight() const { return m_uHeight; }
	BYTE GetComp() const { return m_uComps; }
	
	BOOL SetSize(int nNewWidth, int nNewHeight, int nNewComp = PIXEL_COMP_RGB)
	{
		if(nNewWidth <= 0 || nNewHeight <= 0 ||
			nNewComp != PIXEL_COMP_GRAY && nNewComp != PIXEL_COMP_RGB && nNewComp != PIXEL_COMP_RGBA)
			return FALSE;
		m_vBuffer.resize(nNewWidth * nNewHeight * nNewComp);
		m_uComps = nNewComp;
		m_uWidth = nNewWidth;
		m_uHeight = nNewHeight;
		return TRUE;
	}
	void Clear()
	{
		m_vBuffer.clear();
	}
	void FlipV()
	{
		char *pData = GetDataPtr();
		const int my = m_uHeight/2, B = m_uComps * m_uWidth, D = (m_uHeight - 1) * B;
		for(int i = 0 ; i < m_uWidth; i++)
		{
			const int A = m_uComps * i, C = A + D;
			for(int j = 0; j < my; j++)
			{
				const int p1 = A + j * B, p2 = C - j * B;
				switch(m_uComps)
				{
				case PIXEL_COMP_RGBA:
					REPLACE(pData[p1+3], pData[p2+3]);
				case PIXEL_COMP_RGB:
					REPLACE(pData[p1+2], pData[p2+2]);
					REPLACE(pData[p1+1], pData[p2+1]);
				case PIXEL_COMP_GRAY:
					REPLACE(pData[p1], pData[p2]);
				}
			}
		}
	}
	void Image::FlipC()
	{
		if(m_uComps<PIXEL_COMP_RGB)
			return;
		const int m=m_uComps;
		const int s=m_uWidth*m_uHeight;
		const void *d=GetDataPtr();
		__asm							// Assembler Code To Follow
		{
			mov ecx, s					// Counter Set To Dimensions Of Our Memory Block
			mov ebx, d					// Points ebx To Our Data (b)
			label:						// Label Used For Looping
			mov al,[ebx+0]				// Loads Value At ebx Into al
			mov ah,[ebx+2]				// Loads Value At ebx+2 Into ah
			mov [ebx+2],al				// Stores Value In al At ebx+2
			mov [ebx+0],ah				// Stores Value In ah At ebx
		
			add ebx,m					// Moves Through The Data
			dec ecx						// Decreases Our Loop Counter
			jnz label					// If Not Zero Jump Back To Label
		}
	}
	// Read TGA format rgb or rgba image
	BOOL ReadTGA(FILE* fp)
	{
		TGAHeader header;
		if(	!fp ||
			!fread(&header, sizeof(TGAHeader), 1, fp) ||
			header.m_iImageTypeCode != 2 ||
			!SetSize(header.m_iWidth, header.m_iHeight, header.m_iBPP / 8))
			return FALSE;
		if( !fread(GetDataPtr(), 1, GetDataSize(), fp) )
				return FALSE;
		if(GET_BIT(header.m_ImageDescriptorByte, 5))
			FlipV();
		// flip red and blue components
		FlipC();
		return TRUE; 
	}
	BOOL ReadTGA(const char *pchFileName)
	{
		File file;
		return (file.Open(pchFileName)) ? ReadTGA(file) : FALSE;
	}
	GLuint GetPixelFormat() const
	{
		switch(m_uComps)
		{
		case PIXEL_COMP_RGBA:
			return GL_RGBA;
		case PIXEL_COMP_RGB:
			return GL_RGB;
		default:
			return GL_LUMINANCE;
		}
	}
	void Draw(float x, float y)
	{
		if(!m_uWidth)
			return;
		SetMemAlign(m_uWidth, FALSE);
		glRasterPos2f(x, y);
		glDrawPixels(m_uWidth, m_uHeight, GetPixelFormat(), GL_UNSIGNED_BYTE, GetDataPtr());
	}
};

class Texture
{
protected:
	GLuint				id; // GL Texture Identification
public:
	BOOL				mipmapped;
	GLint				width; // For Not Mipmapped Must be 2^n + 2(border) for some integer n. 
	GLint				height;	// For Not Mipmapped Must be 2^m + 2(border) for some integer m. 
	GLint				levelDetail; // The level-of-detail number. Level 0 is the base image level. Level n is the nth mipmap reduction image. 
    GLint				childs; // The number of color childs in the texture. Must be 1, 2, 3, or 4. 	
	GLint				border;	// The width of the border. Must be either 0 or 1. 
    GLuint				format; // GL_COLOR_INDEX, GL_STENCIL_INDEX, GL_DEPTH_COMPONENT,GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA, GL_RGB, GL_RGBA, GL_LUMINANCE,GL_LUMINANCE_ALPHA, GL_BGR_EXT, GL_BGRA_EXT 
    GLuint				dataFormat; // GL_UNSIGNED_BYTE, GL_BYTE, GL_BITMAP, GL_UNSIGNED_SHORT, GL_SHORT, GL_UNSIGNED_INT, GL_INT, and GL_FLOAT	
    BYTE				*data; // image data
	GLint				magFilter; // Mag Filter To Use (GL_NEAREST, GL_LINEAR)
	GLint				minFilter; // Min Filter To Use (GL_NEAREST, GL_LINEAR)
	GLint				wrap; // Clamped or Repeated (GL_CLAMP, GL_REPEAT)
	int					error;

	Texture():
		width(0), height(0), id(0),
		minFilter(GL_LINEAR), magFilter(GL_LINEAR), wrap(GL_REPEAT),
		childs(3), data(NULL), dataFormat(GL_UNSIGNED_BYTE), format(GL_RGB)
	{}
	~Texture()
	{
		Destroy();
	}
	BOOL Create(const Image &image)
	{
		if(!image.GetDataSize())
			return FALSE;
		GLuint format = image.GetPixelFormat();
		GLint width = image.GetWidth();
		GLint height = image.GetHeight();
		const char *data = image.GetDataPtr();

		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_2D, id);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);

		if(mipmapped)
		{
			gluBuild2DMipmaps(
				GL_TEXTURE_2D,	// 2D image
				childs,			// number of color childs
				width,			// width
				height,			// height
				format,			// color model
				GL_UNSIGNED_BYTE, // data word type
				data);			// data
		}
		else
		{
			glTexImage2D(
				GL_TEXTURE_2D,	// 2D image
				levelDetail,	// detail level
				childs,			// number of color childs
				width,			// width
				height,			// height
				border,			// border
				format,			// color model
				GL_UNSIGNED_BYTE, // data word type
				data);			// data
		}
		//GL_INVALID_VALUE

		GLenum error = glGetError();
		return error ? FALSE : TRUE;
	}
	void Destroy()
	{
		id=0;
		glDeleteTextures(1, &id);
	}
	BOOL Bind() const
	{
		glBindTexture(GL_TEXTURE_2D, id);
		GLenum error = glGetError();
		return error ? FALSE : TRUE;
	}
	int GetID() const { return id; }
};

void Print(char *fmt, ...)
{
	if (fmt  ==  NULL)
		return;
	char text[1024];
	va_list ap;
	va_start(ap, fmt);
		_vsnprintf(text, 1024, fmt, ap);
	va_end(ap);
	static BOOL s_bIsDebuggerPresent = IsDebuggerPresent();
	if(s_bIsDebuggerPresent)
		OutputDebugString(text);
	printf(text);
}

BYTE nBpp = 32;				// Bits Per Pixel
BYTE nDepth = 32;				// Number Of Bits For The Depth Buffer
BYTE nStencil = 8;			// Number Of Bits For The Stencil Buffer
LPTSTR pchCmdLine = "";
int nShow = 0;
HINSTANCE hInst = NULL;
HWND hWnd = NULL; // Window Handle
HDC hDC = NULL;   // Device Context
HGLRC hRC = NULL; // Rendering Context
BOOL bFullscreen = FALSE;
BOOL bVisible = TRUE;
BOOL bUpdated = TRUE; // Will force redraw
LPTSTR pchName = "Arkanoid";
LONG nLeft = 400, nLastLeft = 0;		// Left Position
LONG nTop = 300, nLastTop = 0; 		// Top Position
LONG nWinWidth = 800;
LONG nWinHeight = 600;
float fPerspAngle = 45;
float fPerspNearZ = 0.01f;
float fPerspFarZ = 100.0f;
RECT rect; 		      // oustide borders
DWORD nStyle = 0;
DWORD nExtStyle = 0;
BOOL bAllowSleep = FALSE;
BOOL bAllowYield = TRUE;
BOOL bIsProgramLooping = TRUE;
BOOL bCreateFullScreen = FALSE;
LONG nDisplayWidth = 800;
LONG nDisplayHeight = 600; 
bool bLoaded = false;
Image imgBall, imgTexture;
Texture tTexture;
GLfloat
	pLightAmbient[]= { 0.5f, 0.5f, 0.5f, 1.0f }, // Ambient Light Values
	pLightDiffuse[]= { 1.0f, 1.0f, 1.0f, 1.0f }, // Diffuse Light Values
	pLightPosition[]= { 0.0f, 1.0f, 1.0f, 0.0f }, // Light Position
	pFogColor[]= {0.0f, 0.0f, 0.0f, 1.0f}; // Fog Color
float fFogStart = 0.0f;
float fFogEnd = 1.0f;
float fFogDensity = 0.08f;
GLenum uFogMode = GL_EXP;
GLenum uFogQuality = GL_DONT_CARE;

void Message(char *fmt, ...)
{
	// ATTN: blocks the execution and may eat input messages!
	if (fmt  ==  NULL)
		return;
	char text[1024];
	va_list ap;
	va_start(ap, fmt);
		_vsnprintf(text, 1024, fmt, ap);
	va_end(ap);
	MessageBox(hWnd, text, "Message", MB_OK | MB_ICONEXCLAMATION);
}

void Terminate()
{
	bIsProgramLooping = FALSE;
	PostMessage (hWnd, WM_QUIT, 0, 0);
}

void ToggleFullscreen()
{
	bCreateFullScreen = !bCreateFullScreen;
	PostMessage(hWnd, WM_QUIT, 0, 0);
}

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
	SHORT				repeatCount;				// repeat count
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

BOOL bKeys[256] = {0};
std::deque<Input> dInput;

class CriticalSection // Thread synchronization class
{
protected:
	CRITICAL_SECTION critical_section;
public:
	CriticalSection()
	{
		InitializeCriticalSection(&critical_section);
	}
	~CriticalSection()
	{
		DeleteCriticalSection(&critical_section);
	}
	void Enter()
	{
		EnterCriticalSection(&critical_section);
	}
	void Leave()
	{
		LeaveCriticalSection(&critical_section);
	}
	void Wait()
	{
		EnterCriticalSection(&critical_section);
		LeaveCriticalSection(&critical_section);
	}
} csInput;

class Lock
{
private:
	CriticalSection &__cs;
public:
	Lock(CriticalSection &cs):__cs(cs)
	{
		__cs.Enter();
	}
	~Lock()
	{
		__cs.Leave();
	}
};

class Timer
{
protected:
	__int64 nCountsPerSecond, nStartCounter;
	static __int64 Count()
	{
		LARGE_INTEGER tmp;
		if(!QueryPerformanceCounter(&tmp))
			return 0;
		return (__int64)tmp.QuadPart;
	}
public:
	Timer():nCountsPerSecond(0), nStartCounter(0)
	{
		LARGE_INTEGER tmp;
		if(!QueryPerformanceFrequency(&tmp))
			return;
		nCountsPerSecond = (__int64)tmp.QuadPart;
	}
	float Time()
	{
		return nCountsPerSecond ? (float)(Count() - nStartCounter) / nCountsPerSecond : 0;
	}
	int Time(int nPrecision)
	{
		return nCountsPerSecond ? (int)(nPrecision * (Count() - nStartCounter) / nCountsPerSecond) : 0;
	}
	void Restart()
	{
		nStartCounter = Count();
	}
} timer;


BOOL ReadImage(Image &image, const char *pchFilename)
{
	if( image.ReadTGA(pchFilename) )
		return TRUE;
	Print("Error reading image %s!", pchFilename);
	return FALSE;
}

void Update()
{
	if( dInput.size() > 0 )
	{
		Input input;
		{
			Lock lock(csInput);
			input = dInput.front();
			dInput.pop_front();
		}
		switch(input.eType)
		{
		case InputKey:
			const Keyboard &keyboard = input.keyboard;
			bKeys[keyboard.code] = keyboard.pressed;
			if( !keyboard.repeated && keyboard.pressed )
			{
				switch( keyboard.code )
				{
				case VK_F11:
					ToggleFullscreen();
					break;
				case VK_F4:
					Terminate();
					break;
				case VK_LEFT:
					Print("LEFT\n");
					break;
				case VK_RIGHT:
					Print("RIGHT\n");
					break;
				case VK_UP:
					Print("UP\n");
					break;
				case VK_DOWN:
					Print("DOWN\n");
					break;
				}
			}
		}
	}
	bUpdated = TRUE;
}

void Draw2D()
{
	imgBall.Draw(0, 0);
}

GLvoid DrawSphere(float R, int nDivs)
{
	if(R < 0 || nDivs <= 0)
		return;
	int nLat = nDivs;
	int nLon = 2 * nDivs;
	float
		u = 0, v = 0, du = 1.0f/nLon, dv = 1.0f/nLat, 
		N[3], 
		lat = -0.5*PI, lon = 0, 
		dlat = PI/nLat, dlon = 2*PI/nLon, 
		clat, slat, 
		clat0 = 0, slat0 = -1, 
		clon, slon, 
		clon0 = 1, slon0 = 0;

	glBegin(GL_QUADS); // Start Drawing Quads
	for(int i = 0;i < nLat;i++)
	{
		// lat update
		lat += dlat;
		clat = cosf(lat);
		slat = sinf(lat);
		
		u = 0;
		for(int j = 0;j < nLon;j++)
		{
			// lon update
			lon += dlon;
			clon = cosf(lon);
			slon = sinf(lon);
			
			///////////////////////////////////////////
			// 
			//		(u, v+dv)	(u+du, v+dv)
			//		P3----------P2	lon
			//		|			|
			//		|			|	/\
			//		|			|
			//		P0----------P1	lon0
			//		(u, v)		(u+du, v)	
			//		lat0	>	lat
			//
			///////////////////////////////////////////

			// P0: lat = lat0, lon = lon0
			N[0] = clon0*clat0;
			N[1] = slon0*clat0;
			N[2] = slat0;
			glNormal3d(N[0], N[1], N[2]);
			glTexCoord2d(u, v);
			glVertex3d(N[0]*R, N[1]*R, N[2]*R);
			
			// P1: lat = lat0, lon = lon
			N[0] = clon*clat0;
			N[1] = slon*clat0;
			N[2] = slat0;
			glNormal3d(N[0], N[1], N[2]);
			glTexCoord2d(u+du, v);
			glVertex3d(N[0]*R, N[1]*R, N[2]*R);

			// P2: lat = lat, lon = lon
			N[0] = clon*clat;
			N[1] = slon*clat;
			N[2] = slat;
			glNormal3d(N[0], N[1], N[2]);
			glTexCoord2d(u+du, v+dv);
			glVertex3d(N[0]*R, N[1]*R, N[2]*R);

			// P3: lat = lat, lon = lon0
			N[0] = clon0*clat;
			N[1] = slon0*clat;
			N[2] = slat;
			glNormal3d(N[0], N[1], N[2]);
			glTexCoord2d(u, v+dv);
			glVertex3d(N[0]*R, N[1]*R, N[2]*R);
			
			clon0 = clon;
			slon0 = slon;
			u += du;
		}
		clat0 = clat;
		slat0 = slat;
		v += dv;
	}
	glEnd();
}

void Draw3D()
{
	tTexture.Bind();
	DrawSphere(0.5f, 16);
}

void Draw()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if( !bLoaded )
		return;

	// 3D
	glPushAttrib(GL_ENABLE_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D); // Enable Texture Mapping
	glDisable(GL_COLOR_MATERIAL);
	glEnable(GL_FOG); // Enables fog
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity ();
	gluPerspective (fPerspAngle,
					(float)nWinWidth / nWinHeight,
					fPerspNearZ,
					fPerspFarZ);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0, 0, -5.0f);
	Draw3D();
	glPopAttrib();

	// 2D
	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_FOG);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, nWinWidth, 
			0, nWinHeight, 
			1, -1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	Draw2D();
	glPopAttrib();
}

void Load()
{
	if( bLoaded )
		return;
	ReadImage(imgBall, "art/ball.tga");
	if( ReadImage(imgTexture, "art/texture.tga") )
	{
		tTexture.minFilter = GL_LINEAR_MIPMAP_NEAREST;
		tTexture.magFilter = GL_LINEAR;
		tTexture.mipmapped = TRUE;
		if( !tTexture.Create(imgTexture) )
			Print("Error creating texture!");
	}
	bLoaded = true;
}

void Redraw()
{
	bUpdated = FALSE;
	Draw();
	SwapBuffers(hDC);
}

void CreateLight(GLenum id, GLfloat *ambient, GLfloat *diffuse, GLfloat *position)
{
	glLightfv(id, GL_AMBIENT, ambient);				// Setup The Ambient Light
	glLightfv(id, GL_DIFFUSE, diffuse);				// Setup The Diffuse Light
	glLightfv(id, GL_POSITION, position);			// Position The Light
	glEnable(id);                                   // Enable Light One
}

void CreateFog(GLfloat start, GLfloat end, GLfloat *color, GLfloat density, GLuint func, GLenum quality)
{
	glFogi(GL_FOG_MODE, func);
	glFogf(GL_FOG_DENSITY, density);
	glFogf(GL_FOG_START, start);
	glFogf(GL_FOG_END, end); 
	glFogfv(GL_FOG_COLOR, color);
	glHint(GL_FOG_HINT, quality);
}

BOOL glCreate()
{
	// Enable transparent colors
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// Enables Smooth Shading
	glShadeModel(GL_SMOOTH);
	// Specify depth params
	glClearDepth(1);
	glDepthFunc(GL_LEQUAL);
	// Really Nice Perspective Calculations
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	// Create light 1
	CreateLight(GL_LIGHT1, pLightAmbient, pLightDiffuse, pLightPosition);

	CreateFog(fFogStart, fFogEnd, pFogColor, fFogDensity, uFogMode, uFogQuality);

	// Set the clear color
	glClearColor(	pFogColor[RED], 
					pFogColor[GREEN], 
					pFogColor[BLUE], 
					pFogColor[ALPHA]);

	return TRUE;
}

void glDestroy()
{}

void Reshape()
{
	glViewport (0, 0, nWinWidth, nWinHeight);
	Redraw();
}

void InitWindow()
{
	nStyle = WS_OVERLAPPEDWINDOW;
	nExtStyle = WS_EX_APPWINDOW;
	if(bCreateFullScreen)
	{
		DEVMODE dmScreenSettings;								// Device Mode
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));						// Make Sure Memory Is Cleared
		dmScreenSettings.dmSize				= sizeof (DEVMODE);	// Size Of The Devmode Structure
		dmScreenSettings.dmPelsWidth		= nDisplayWidth;			// Select Screen Width
		dmScreenSettings.dmPelsHeight		= nDisplayHeight;			// Select Screen Height
		dmScreenSettings.dmBitsPerPel		= nBpp;				// Select Bits Per Pixel
		dmScreenSettings.dmFields			= DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
		if( ChangeDisplaySettings (&dmScreenSettings, CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL )
		{
			// Seting Top Window To Be Covering Everything Else
			nStyle = WS_POPUP;										
			nExtStyle |= WS_EX_TOPMOST;
			nLastLeft = nLeft;
			nLastTop = nTop;
			nLeft = 0;
			nTop = 0;
			nWinWidth = nDisplayWidth;
			nWinHeight = nDisplayHeight;
		}
		else
		{
			Message("Mode Switch Failed!\nCannot Use That Resolution: %d:%d:%d\n", 
					nDisplayWidth, nDisplayHeight, nBpp);
			bCreateFullScreen = FALSE;
		}
	}
	else if( nLastLeft || nLastTop )
	{
		nLeft = nLastLeft;
		nTop = nLastTop;
	}
	RECT winRect = { nLeft, nTop, nLeft + nWinWidth, nTop + nWinHeight };
	if(!bCreateFullScreen)// Adjust Window, Account For Window Borders
		AdjustWindowRectEx (&winRect, nStyle, 0, nExtStyle);
	rect = winRect;
	bFullscreen = bCreateFullScreen;
}

BOOL CreateNewWindow()
{
	InitWindow();
	hWnd = CreateWindowEx ( nExtStyle, 						// Extended Style
							pchName, 								// Class Name
							pchName, 						// Window Title
							nStyle, 						// Window Style
							max(0, rect.left), // Window X Position
							max(0, rect.top), 	// Window Y Position
							rect.right - rect.left, 	// Window Width
							rect.bottom - rect.top, 	// Window Height
							HWND_DESKTOP, 						
							0, 									// No Menu
							hInst, 								
							NULL);
	if (hWnd)												
	{
		hDC = GetDC (hWnd);
		if (hDC)	
		{
			PIXELFORMATDESCRIPTOR pfd =	
			{
				sizeof (PIXELFORMATDESCRIPTOR), 
				1, 							// Version Number
				PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER, 
				PFD_TYPE_RGBA, 
				nBpp, 
				0, 0, 0, 0, 0, 0, 			// Color Bits Ignored
				0, 							// No Alpha Buffer
				0, 							// Shift Bit Ignored
				0, 							// No Accumulation Buffer
				0, 0, 0, 0, 					// Accumulation Bits Ignored
				nDepth, 				// Z-Buffer (Depth Buffer)
				nStencil, 			// Stencil Buffer
				0, 							// No Auxiliary Buffer
				PFD_MAIN_PLANE, 				// Main Drawing Layer
				0, 							// Reserved
				0, 0, 0						// Layer Masks Ignored
			};
			int pf = ChoosePixelFormat (hDC, &pfd);
			if (pf && SetPixelFormat(hDC, pf , &pfd))
			{
				hRC = wglCreateContext(hDC);
				if(hRC)
				{
					if (wglMakeCurrent(hDC, hRC))
					{
						ShowWindow (hWnd, nShow);
						Reshape();	
						return TRUE;
					}
					wglDeleteContext (hRC);
					hRC = 0;	
				}
			}
			ReleaseDC (hWnd, hDC);
			hDC = NULL;
		}
		DestroyWindow (hWnd);
		hWnd = NULL;
	}
	return FALSE;
}

BOOL DestroyWindow()
{
	if (hWnd)													// Does The Window Have A Handle?
	{	
		if (hDC)													// Does The Window Have A Device Context?
		{
			wglMakeCurrent (hDC, NULL);							// Set The Current Active Rendering Context To Zero
			if (hRC)												// Does The Window Have A Rendering Context?
			{
				wglDeleteContext (hRC);							// Release The Rendering Context
				hRC = NULL;										// Zero The Rendering Context
			}
			ReleaseDC (hWnd, hDC);						// Release The Device Context
			hDC = NULL;											// Zero The Device Context
		}
		DestroyWindow (hWnd);									// Destroy The Window
		hWnd = NULL;												// Zero The Window Handle
	}
	if (bFullscreen)												// Is Window In Fullscreen Mode
		ChangeDisplaySettings (NULL, 0);									// Switch Back To Desktop Resolution
	ShowCursor(TRUE);													// Show The Cursor
	return TRUE;														// Return True
}

LRESULT CALLBACK WinProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_SYSCOMMAND:
		{
			switch (wParam)												// Check System Calls
			{
				case SC_SCREENSAVE:	
				case SC_MONITORPOWER:
					return 0;
			}
			break;
		}
		case WM_CLOSE:													// Closing The Window
		{
			Terminate();
			return 0;
		}
		case WM_PAINT:
		{
			Draw();
			SwapBuffers(hDC);
			break;
		}
		case WM_MOVE:
		{
			nLeft = LOWORD(lParam);   // horizontal position 
			nTop = HIWORD(lParam);   // vertical position
			return 0;
		}
		case WM_SIZE:
		{
			switch (wParam)
			{
				case SIZE_MINIMIZED:
				{
					bVisible = FALSE;
				}
				return 0;
				case SIZE_MAXIMIZED:
				case SIZE_RESTORED:
				{
					bVisible = TRUE;
					nWinWidth = LOWORD (lParam);
					nWinHeight = HIWORD (lParam);
					Reshape();
				}
				return 0;
			}
			break;
		}
		case WM_TOGGLEFULLSCREEN:
		{
			ToggleFullscreen();
			break;
		}
		case WM_CHAR:
		case WM_DEADCHAR:
		case WM_SYSDEADCHAR:
		case WM_SYSCHAR:
		{
			Input input = {uMsg, InputChar};
			input.nSymbol = (SHORT)wParam;
			{
				Lock lock(csInput);
				dInput.push_back(input);
			}
			return 0;
		}
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			Input input = {uMsg, InputKey};
			input.keyboard.code = (BYTE)wParam;
			input.keyboard.repeatCount = (SHORT)GET_BITS((DWORD)lParam, 0, 15);
			input.keyboard.scanCode = (BYTE)GET_BITS((DWORD)lParam, 16, 23);
			input.keyboard.extendKey = GET_BIT((DWORD)lParam, 24);
			input.keyboard.alt = GET_BIT((DWORD)lParam, 29);
			input.keyboard.repeated = GET_BIT((DWORD)lParam, 30);
			input.keyboard.pressed = !GET_BIT((DWORD)lParam, 31);
			{
				Lock lock(csInput);
				dInput.push_back(input);
			}
			return 0;
		}
		case WM_MOUSEMOVE:
		case WM_RBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_LBUTTONDBLCLK:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MBUTTONDBLCLK:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_MOUSEACTIVATE:
		case WM_MOUSEWHEEL:
		{
			Input input = {uMsg, InputMouse};
			input.mouse.x = LOWORD(lParam);
			input.mouse.y = HIWORD(lParam);
			input.mouse.wheel = ((short)HIWORD(wParam))/WHEEL_DELTA;			// wheel rotation
			input.mouse.lbutton = (wParam&MK_LBUTTON) != 0;
			input.mouse.mbutton = (wParam&MK_MBUTTON) != 0;
			input.mouse.rbutton = (wParam&MK_RBUTTON) != 0;
			input.mouse.shift = (wParam&MK_SHIFT) != 0;
			input.mouse.ctrl = (wParam&MK_CONTROL) != 0;
			{
				Lock lock(csInput);
				dInput.push_back(input);
			}
			return 0;
		}
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void SetThreadName(LPCSTR name, DWORD threadID = -1)
{
	if(!name||!name[0])
		return;

	struct
	{
		DWORD dwType; // must be 0x1000
		LPCSTR szName; // pointer to name (in user addr space)
		DWORD dwThreadID; // thread ID (-1=caller thread)
		DWORD dwFlags; // reserved for future use, must be zero
	} info = {0x1000, name, threadID, 0};

	__try
	{
		RaiseException( 0x406D1388, 0, sizeof(info)/sizeof(DWORD), (DWORD*)&info );
	}
	__except(EXCEPTION_CONTINUE_EXECUTION)
	{
	}
}

static DWORD WINAPI UpdateProc(void * param)
{
	HANDLE hThread = GetCurrentThread();
	SetThreadPriority(hThread, THREAD_PRIORITY_NORMAL);
	SetThreadName("Update");

	while (bIsProgramLooping)
	{
		Update();
		Sleep(10);
	}

	CloseHandle(hThread);
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	hInst = hInstance;
	pchCmdLine = lpCmdLine;
	nShow = nCmdShow;
	
	LPTSTR pchName = "Arkanoid";
	HCURSOR hCursor = LoadCursor(NULL, IDC_ARROW);

	if (!hPrevInstance)
	{
		WNDCLASSEX windowClass;
		ZeroMemory (&windowClass, sizeof (WNDCLASSEX));
		windowClass.cbSize			 =  sizeof (WNDCLASSEX);
		windowClass.style			 =  CS_HREDRAW|CS_VREDRAW|CS_OWNDC|CS_DBLCLKS;
		windowClass.lpfnWndProc		 =  (WNDPROC) WinProc;
		windowClass.hInstance		 =  hInstance;
		windowClass.hbrBackground	 =  (HBRUSH)(COLOR_APPWORKSPACE);
		windowClass.hCursor			 =  hCursor;
		windowClass.lpszClassName	 =  pchName;
		if( !RegisterClassEx (&windowClass) )
		{
			Message("Register Class '%s' Failed!", pchName);
			return -1;
		}
	}

	if( !CreateThread(NULL, 0, UpdateProc, NULL, 0, NULL) )
	{
		Message("Failed to start the update thread!");
		return -1;
	}

	while (bIsProgramLooping)
	{
		if (CreateNewWindow())
		{
			if (!glCreate())
			{
				Terminate();
			}
			else
			{
				Load();
				for(;;)
				{
					MSG msg; // Message Info
					if (PeekMessage (&msg, hWnd, 0, 0, PM_REMOVE) != 0)
					{
						if (msg.message == WM_QUIT)
							break;
						TranslateMessage(&msg);
						DispatchMessage(&msg);
						continue;
					}
					if( bUpdated )
					{
						bUpdated = FALSE;
						Draw();
						SwapBuffers(hDC);
					}
					else if(bAllowSleep)
					{
						WaitMessage();
						continue;
					}
					if(bAllowYield)
					{
						Sleep(0);
					}
				}
			}	
			glDestroy();
			DestroyWindow();
		}
		else
		{
			Message("Error Creating Window!");
			bIsProgramLooping = FALSE;
		}
	}

	UnregisterClass (pchName, hInstance);

	return 0;
}