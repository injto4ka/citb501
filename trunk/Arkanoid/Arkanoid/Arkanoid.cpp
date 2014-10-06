#include <windows.h>			// Windows API Definitions
#include <string>
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
	MessageBox (NULL, text, "Message", MB_OK | MB_ICONEXCLAMATION);
}

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
LPTSTR pchName = "Arkanoid";
LONG nLeft = 400, nLastLeft = 0;		// Left Position
LONG nTop = 300, nLastTop = 0; 		// Top Position
LONG nWinWidth = 800;
LONG nWinHeight = 600;
RECT rect; 		      // oustide borders
DWORD nStyle = 0;
DWORD nExtStyle = 0;
BOOL bAllowSleep = TRUE;
BOOL bAllowYield = TRUE;
BOOL bIsProgramLooping = TRUE;
BOOL bCreateFullScreen = FALSE;
LONG nDisplayWidth = 800;
LONG nDisplayHeight = 600; 
int nClearColor = 0xff00ffff;

void Update()
{}
void Draw()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

BOOL glCreate()
{
	// Disable 3D specific features
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);

	// Enable transparent colors
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Set the clear color
	BYTE *rgb = INT_TO_BYTE(nClearColor);
	glClearColor(	rgb[RED] / 255.0f, 
					rgb[GREEN] / 255.0f, 
					rgb[BLUE] / 255.0f, 
					rgb[ALPHA] / 255.0f);

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

void Terminate()
{
	bIsProgramLooping = FALSE;
	PostMessage (hWnd, WM_QUIT, 0, 0);
}

void ToggleFullscreen()
{
	bCreateFullScreen =! bCreateFullScreen;
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
	BOOL				move;						// Is the mouse moving
} mouse = {0};

struct Keyboard{	
	BOOL				keys[256];					// Keyboard Buttons
	SHORT				symbol;						// Ascii Character Of The Last Button
	BYTE				code;						// Virtual Key Code Of The Last Button Pressed (VK)
	SHORT				repeatCount;				// repeat count
	BYTE				scanCode;					// scan code
	BOOL				extendKey;					// extended-key flag
	BOOL				alt;						// context code
	BOOL				repeated;					// previous key-state flag
	BOOL				pressed;					// transition-state flag
	BOOL				waiting;					// Indicates symbol waiting
} keyboard = {0};

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
			keyboard.waiting = FALSE;
			keyboard.symbol = (SHORT)wParam;
			return 0;
		}
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			BYTE code = (BYTE)wParam;
			keyboard.waiting = TRUE;
			keyboard.code = code;
			keyboard.repeatCount = (SHORT)GET_BITS((DWORD)lParam, 0, 15);
			keyboard.scanCode = (BYTE)GET_BITS((DWORD)lParam, 16, 23);
			keyboard.extendKey = GET_BIT((DWORD)lParam, 24);
			keyboard.alt = GET_BIT((DWORD)lParam, 29);
			keyboard.repeated = GET_BIT((DWORD)lParam, 30);
			keyboard.keys[code] = keyboard.pressed = !GET_BIT((DWORD)lParam, 31);

			if( !keyboard.repeated )
			{
				if( keyboard.keys[VK_F11] )
				{
					ToggleFullscreen();
				}
				else if( keyboard.keys[VK_F4] && keyboard.alt )
				{
					Terminate();
				}
				else if( keyboard.keys[VK_LEFT] )
				{
					Print("LEFT\n");
				}
				else if( keyboard.keys[VK_RIGHT] )
				{
					Print("RIGHT\n");
				}
				else if( keyboard.keys[VK_UP] )
				{
					Print("UP\n");
				}
				else if( keyboard.keys[VK_DOWN] )
				{
					Print("DOWN\n");
				}
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
			mouse.x = LOWORD(lParam);
			mouse.y = HIWORD(lParam);
			mouse.wheel = ((short)HIWORD(wParam))/WHEEL_DELTA;			// wheel rotation
			mouse.lbutton = (wParam&MK_LBUTTON) != 0;
			mouse.mbutton = (wParam&MK_MBUTTON) != 0;
			mouse.rbutton = (wParam&MK_RBUTTON) != 0;
			mouse.shift = (wParam&MK_SHIFT) != 0;
			mouse.ctrl = (wParam&MK_CONTROL) != 0;
			mouse.move = (uMsg == WM_MOUSEMOVE);
			return 0;
		}
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
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
					Update();
					if(bVisible)
					{
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