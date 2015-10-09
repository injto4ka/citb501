#include <windows.h>			// Windows API Definitions
#include <deque>
#include <gl/gl.h>									// Header File For The OpenGL32 Library
#include <gl/glu.h>									// Header File For The GLu32 Library

#pragma comment( lib, "opengl32.lib" )				// Search For OpenGL32.lib While Linking
#pragma comment( lib, "glu32.lib" )					// Search For GLu32.lib While Linking

#pragma warning(disable:4996)

#ifndef WM_TOGGLEFULLSCREEN							// Application Define Message For Toggling
#	define WM_TOGGLEFULLSCREEN (WM_USER+1)									
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
BOOL bUpdated = 0;
LPTSTR pchName = "Arkanoid";
LONG nLeft = 400, nLastLeft = 0;		// Left Position
LONG nTop = 300, nLastTop = 0; 		// Top Position
LONG nWinWidth = 800;
LONG nWinHeight = 600;
RECT rect; 		      // oustide borders
DWORD nStyle = 0;
DWORD nExtStyle = 0;
BOOL bAllowSleep = FALSE;
BOOL bAllowYield = TRUE;
BOOL bIsProgramLooping = TRUE;
BOOL bCreateFullScreen = FALSE;
LONG nDisplayWidth = 800;
LONG nDisplayHeight = 600; 
int nClearColor = 0xff00ffff;

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

class PrecisionTimer
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
	PrecisionTimer():nCountsPerSecond(0), nStartCounter(0)
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

int nClearColorEnd = 0;
float fFreq = 5.0f;
bool bTimeStart = false;

void StartFreq(float fNewFreq)
{
	bTimeStart = true;
	fFreq = fNewFreq;
	timer.Restart();
	nClearColorEnd = 0xffff0000;
	bUpdated = TRUE;
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
				case VK_F5:
					StartFreq(5.0f);
					break;
				case VK_F6:
					StartFreq(1.0f);
					break;
				case VK_F7:
					bTimeStart = false;
					break;
				}
			}
		}
	}
	bUpdated = TRUE;
}
void Draw()
{
	int nClearColorNow = nClearColor;
	BYTE *rgb = INT_TO_BYTE(nClearColorNow);

	if (bTimeStart)
	{
		BYTE *rgb1 = INT_TO_BYTE(nClearColorEnd);
		float fCoef = (1 + sin(-3.14f/2 + 2 * 3.14f * timer.Time() / fFreq)) / 2;

		rgb[RED] = rgb[RED] + (int)((rgb1[RED] - rgb[RED]) * fCoef);
		rgb[GREEN] = rgb[GREEN] + (int)((rgb1[GREEN] - rgb[GREEN]) * fCoef);
		rgb[BLUE] = rgb[BLUE] + (int)((rgb1[BLUE] - rgb[BLUE]) * fCoef);

		bUpdated = TRUE;
	}

	glClearColor(rgb[RED] / 255.0f,
		rgb[GREEN] / 255.0f,
		rgb[BLUE] / 255.0f,
		rgb[ALPHA] / 255.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Redraw()
{
	bUpdated = FALSE;
	Draw();
	SwapBuffers(hDC);
}

BOOL glCreate()
{
	// Disable 3D specific features
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);

	// Enable transparent colors
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	return TRUE;
}
void glDestroy()
{}
void Reshape()
{
	glViewport (0, 0, nWinWidth, nWinHeight);
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, nWinWidth, 
			0, nWinHeight, 
			1, -1);
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