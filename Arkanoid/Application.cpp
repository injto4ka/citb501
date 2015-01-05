#include <deque>

#include "application.h"
#include "utils.h"

#ifndef WM_TOGGLEFULLSCREEN							// Application Define Message For Toggling
#	define WM_TOGGLEFULLSCREEN (WM_USER+1)									
#endif

static DWORD nStyle =  0, nExtStyle = 0;
static Application *pApp = NULL;
static std::deque<Input> dInput;
static CriticalSection csInput;
static void OnNewInput(const Input &input)
{
	Lock lock(csInput);
	dInput.push_back(input);
}

Application::Application(LPTSTR pchName):pchName(pchName)
{
	pchCmdLine = "";
	nShow = 0;
	hInst = NULL;
	hWnd = NULL;
	hDC = NULL;
	hRC = NULL;
	bIsProgramLooping = TRUE;
	bCreateFullScreen = FALSE;
	bAllowSleep = FALSE;
	bAllowYield = TRUE;
	bFullscreen = FALSE;
	bVisible = TRUE;
	nLeft = 0;
	nLastLeft = 0;
	nTop = 0;
	nLastTop = 0;
	nWinWidth = 800;
	nWinHeight = 600;
	nDisplayWidth = 800;
	nDisplayHeight = 600;
	nMouseX = 0;
	nMouseY = 0;
	nBpp = 32;
	nDepth = 32;
	nStencil = 8;
	nWinStyle = WS_OVERLAPPEDWINDOW;
	nWinExtStyle = WS_EX_APPWINDOW;
	ZeroMemory(bKeys, sizeof(bKeys));
}

Application::~Application()
{
	if(pApp == this)
		pApp = NULL;
}

LRESULT CALLBACK WinProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return pApp ? pApp->Proc(hWnd, uMsg, wParam, lParam) : DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT Application::Proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
			// Redraw();
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
			OnNewInput(input);
			return 0;
		}
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			BYTE code = (BYTE)wParam;
			BOOL pressed = !GET_BIT((DWORD)lParam, 31);
			bKeys[code] = pressed;

			Input input = {uMsg, InputKey};
			input.keyboard.code = code;
			input.keyboard.repeatCount = (SHORT)GET_BITS((DWORD)lParam, 0, 15);
			input.keyboard.scanCode = (BYTE)GET_BITS((DWORD)lParam, 16, 23);
			input.keyboard.extendKey = GET_BIT((DWORD)lParam, 24);
			input.keyboard.alt = GET_BIT((DWORD)lParam, 29);
			input.keyboard.repeated = GET_BIT((DWORD)lParam, 30);
			input.keyboard.pressed = pressed;
			OnNewInput(input);

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
			WORD x = LOWORD(lParam);
			WORD y = HIWORD(lParam);
			nMouseX = x;
			nMouseY = nWinHeight - y;

			Input input = {uMsg, InputMouse};
			input.mouse.x = x;
			input.mouse.y = y;
			input.mouse.wheel = ((short)HIWORD(wParam))/WHEEL_DELTA;			// wheel rotation
			input.mouse.lbutton = (wParam&MK_LBUTTON) != 0;
			input.mouse.mbutton = (wParam&MK_MBUTTON) != 0;
			input.mouse.rbutton = (wParam&MK_RBUTTON) != 0;
			input.mouse.shift = (wParam&MK_SHIFT) != 0;
			input.mouse.ctrl = (wParam&MK_CONTROL) != 0;
			OnNewInput(input);

			return 0;
		}
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void Application::InitWindow()
{
	nStyle = nWinStyle;
	nExtStyle = nWinExtStyle;
	if(bCreateFullScreen)
	{
		DEVMODE dmScreenSettings;								// Device Mode
		ZeroMemory(&dmScreenSettings, sizeof(dmScreenSettings));						// Make Sure Memory Is Cleared
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

BOOL Application::CreateNewWindow()
{
	InitWindow();
	hWnd = CreateWindowEx(  nExtStyle, 						// Extended Style
							pchName, 						// Class Name
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
						return TRUE;
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

void Application::DestroyCurWindow()
{
	if (hWnd)													// Does The Window Have A Handle?
	{	
		if (hDC)													// Does The Window Have A Device Context?
		{
			wglMakeCurrent(hDC, NULL);							// Set The Current Active Rendering Context To Zero
			if (hRC)												// Does The Window Have A Rendering Context?
			{
				wglDeleteContext(hRC);							// Release The Rendering Context
				hRC = NULL;										// Zero The Rendering Context
			}
			ReleaseDC(hWnd, hDC);						// Release The Device Context
			hDC = NULL;											// Zero The Device Context
		}
		DestroyWindow(hWnd);									// Destroy The Window
		hWnd = NULL;												// Zero The Window Handle
	}
	if (bFullscreen)												// Is Window In Fullscreen Mode
		ChangeDisplaySettings(NULL, 0);									// Switch Back To Desktop Resolution
	ShowCursor(TRUE);													// Show The Cursor													// Return True
}

void Application::Redraw()
{
	Draw();
	SwapBuffers(hDC);
}

void Application::Reshape()
{
	glViewport(0, 0, nWinWidth, nWinHeight);
	glScissor(0, 0, nWinWidth, nWinHeight);
	c_container._AdjustSize(nWinWidth, nWinHeight);
}

void Application::Terminate()
{
	bIsProgramLooping = FALSE;
	PostMessage (hWnd, WM_QUIT, 0, 0);
}

void Application::ToggleFullscreen()
{
	bCreateFullScreen = !bCreateFullScreen;
	PostMessage(hWnd, WM_QUIT, 0, 0);
}

void Application::Main(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	InitRandGen();

	hInst = hInstance;
	pchCmdLine = lpCmdLine;
	nShow = nCmdShow;

	HCURSOR hCursor = LoadCursor(NULL, IDC_ARROW);

	SetThreadName("Render");

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
			return;
		}
	}

	pApp = this;

	if( !Create() )
	{
		Message("Application init failed!");
		return;
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
				ShowWindow(hWnd, nShow);
				for(;;)
				{
					MSG msg; // Message Info
					if(PeekMessage(&msg, hWnd, 0, 0, PM_REMOVE) != 0)
					{
						if (msg.message == WM_QUIT)
							break;
						TranslateMessage(&msg);
						DispatchMessage(&msg);
						continue;
					}

					while( dInput.size() > 0 )
					{
						Input input;
						{
							Lock lock(csInput);
							input = dInput.front();
							dInput.pop_front();
						}
						OnInput(input);
					}

					Update();

					if( bVisible )
					{
						Redraw();
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
			DestroyCurWindow();
		}
		else
		{
			Message("Error Creating Window!");
			bIsProgramLooping = FALSE;
		}
	}

	Destroy();
	UnregisterClass(pchName, hInstance);
}