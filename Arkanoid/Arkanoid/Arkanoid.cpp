#include <windows.h>			// Windows API Definitions
#include <deque>

#include <math.h>
#include <gl/gl.h>									// Header File For The OpenGL32 Library
#include <gl/glu.h>									// Header File For The GLu32 Library

#include "utils.h"
#include "graphics.h"
#include "ui.h"

#pragma comment( lib, "opengl32.lib" )				// Search For OpenGL32.lib While Linking
#pragma comment( lib, "glu32.lib" )					// Search For GLu32.lib While Linking

#ifndef WM_TOGGLEFULLSCREEN							// Application Define Message For Toggling
#	define WM_TOGGLEFULLSCREEN (WM_USER+1)									
#endif

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
Image imgBall, imgTexture;
Texture texture;
GLfloat
	pLightAmbient[]= { 0.5f, 0.5f, 0.5f, 1.0f }, // Ambient Light Values
	pLightDiffuse[]= { 1.0f, 1.0f, 1.0f, 1.0f }, // Diffuse Light Values
	pLightPosition[]= { 0.0f, 1.0f, 1.0f, 0.0f }, // Light Position
	pFogColor[]= {0.0f, 0.0f, 0.0f, 1.0f}; // Fog Color
float fFogStart = 0.0f;
float fFogEnd = 1.0f;
float fFogDensity = 0.08f;
float fLastDrawTime = 0;
GLenum uFogMode = GL_EXP;
GLenum uFogQuality = GL_DONT_CARE;
BOOL bKeys[256] = {0};
std::deque<Input> dInput;
CriticalSection csInput, csShared;
Timer timer;
DisplayList dlBall;
Transform transform;
float
	fPlaneZ = -5.0f,
	fBallX = 0, fBallY = 0, fBallZ = 0,
	fBallX0 = 0, fBallY0 = 0,
	fFrameX = -1.0f, fFrameY = -1.0f, fFrameZ = 0.0f,
	fBallSpeed = 0.5f;
volatile int nNewWinX = -1, nNewWinY = -1, nBallN = 16, nCoordX = 0, nCoordY = 0;
volatile float fBallR = 0.5f;
volatile bool bNewBall = false;
Font font("Courier New", -16);
Event evInput;
Panel cPanel;
Label cLabel;
Control cContainer;

#define Message(fmt, ...) Message(hWnd, fmt, __VA_ARGS__)

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

BOOL ReadImage(Image &image, const char *pchFilename)
{
	ErrorCode err = image.ReadTGA(pchFilename);
	if( !err )
		return TRUE;
	Print("Error reading image '%s': %s", pchFilename, err);
	return FALSE;
}

void Update()
{
	while( dInput.size() > 0 )
	{
		Input input;
		{
			Lock lock(csInput);
			input = dInput.front();
			dInput.pop_front();
		}
		switch(input.eType)
		{
			case InputMouse:
			{
				const Mouse &mouse = input.mouse;
				{
					Lock lock(csShared);
					nCoordX = mouse.x;
					nCoordY = nWinHeight - mouse.y;
					if( mouse.lbutton )
					{
						nNewWinX = nCoordX;
						nNewWinY = nCoordY;
					}
				}
				break;
			}
			case InputKey:
			{
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
						if(fBallR > 0.1f)
						{
							Lock lock(csShared);
							fBallR -= 0.1f;
							bNewBall = true;
						}
						break;
					case VK_RIGHT:
						if(fBallR < 10.0f)
						{
							Lock lock(csShared);
							fBallR += 0.1f;
							bNewBall = true;
						}
						break;
					case VK_UP:
						if(nBallN < 100)
						{
							Lock lock(csShared);
							nBallN++;
							bNewBall = true;
						}
						break;
					case VK_DOWN:
						if(nBallN > 2)
						{
							Lock lock(csShared);
							nBallN--;
							bNewBall = true;
						}
						break;
					}
				}
			}
		}
	}
}

void Draw2D()
{
	imgBall.Draw(0, 0);

	int fontH = font.GetRowHeight();
	font.Print("Align Right", 200, 200, 0xff00ffff, ALIGN_RIGHT);
	font.Print("Align Centre", 200, 200 + (float)fontH, 0xffffffff, ALIGN_CENTER);
	font.Print("Align Left", 200, 200 + 2*(float)fontH, 0xffffff00, ALIGN_LEFT);

	char buff[128];
	FORMAT(buff, "(%d, %d)", nCoordX, nCoordY);
	font.Print(buff, (float)nWinWidth - 5, (float)nWinHeight, 0xffffffff, ALIGN_RIGHT, ALIGN_BOTTOM);

}

void Draw3D()
{
	if( !dlBall || bNewBall )
	{
		Lock lock(csShared);
		CompileDisplayList cds(dlBall);
		DrawSphere(fBallR, nBallN);
		bNewBall = false;
	}

	glTranslatef(0, 0, fPlaneZ);

	if( nNewWinY >= 0 )
	{
		int nWinX, nWinY;
		{
			Lock lock(csShared);
			nWinX = nNewWinX;
			nWinY = nNewWinY;
		}

		double dWinX0, dWinY0, dDepthZ, dFrameX, dFrameY, dFrameZ;
		transform.Update();
		transform.GetWindowCoor(0, 0, 0, dWinX0, dWinY0, dDepthZ);
		transform.GetObjectCoor((double)nWinX, (double)nWinY, dDepthZ, dFrameX, dFrameY, dFrameZ);
		fFrameX = (float)dFrameX;
		fFrameY = (float)dFrameY;
		fFrameZ = (float)dFrameZ;
	}

	float time = timer.Time();
	float dx = fFrameX - fBallX, dy = fFrameY - fBallY, fDist2 = dx*dx + dy*dy;
	if( fDist2 > 1e-6f )
	{
		// Interpolate the ball position towards the target position
		float fDist = sqrtf(fDist2);
		float fTravelTime = fDist / fBallSpeed;
		float dt = time - fLastDrawTime;
		if( dt > fTravelTime )
		{
			fBallX = fFrameX;
			fBallY = fFrameY;
		}
		else
		{
			float progress = dt / fTravelTime;
			fBallX += (fFrameX - fBallX) * progress;
			fBallY += (fFrameY - fBallY) * progress;
		}
	}
	else
	{
		fBallX = fFrameX;
		fBallY = fFrameY;
	}
	fLastDrawTime = time;

	DrawLine(fFrameX, fFrameY, fFrameZ, fBallX, fBallY, fBallZ, 0xff00ffff);
	DrawFrame(fFrameX, fFrameY, fFrameZ);

	glPushMatrix();
	glTranslatef(fBallX, fBallY, fBallZ);
	texture.Bind();
	dlBall.Execute();
	glPopMatrix();
}

void Draw()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
	glLoadIdentity();
	cContainer._Draw();
	glPopAttrib();
}

void Init()
{
	ReadImage(imgBall, "art/ball.tga");
	ReadImage(imgTexture, "art/texture.tga");
	texture.minFilter = GL_LINEAR_MIPMAP_NEAREST;
	texture.magFilter = GL_LINEAR;
	texture.mipmapped = TRUE;

	cLabel.SetBounds(-10, 20, 120, 60);
	cLabel.m_strText = "Label text";
	cLabel.m_pFont = &font;
	cLabel.AdjustSize();
	cLabel.m_nMarginX = 5;
	cLabel.m_nOffsetY = 4;
	cLabel.m_eAlignH = ALIGN_CENTER;
	cLabel.m_eAlignV = ALIGN_CENTER;
	cLabel.m_nForeColor = 0xff0000ff;
	cLabel.m_nBorderColor = 0xff0000ff;
	cLabel.m_nBackColor = 0xff00ffff;

	cPanel.SetBounds(320, 20, 200, 100);
	cPanel.m_nBorderColor = 0xff0000ff;
	cPanel.m_nBackColor = 0xffffffff;
	cPanel.Add(&cLabel);

	cContainer.Add(&cPanel);
}

void Redraw()
{
	Draw();
	SwapBuffers(hDC);
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

	glPolygonMode( GL_BACK, GL_FILL ); // Back Face Is Filled In
	glPolygonMode( GL_FRONT, GL_FILL ); // Front Face Is Drawn With Lines

	// Create light 1
	CreateLight(GL_LIGHT1, pLightAmbient, pLightDiffuse, pLightPosition);

	CreateFog(fFogStart, fFogEnd, pFogColor, fFogDensity, uFogMode, uFogQuality);

	// Set the clear color
	glClearColor(	pFogColor[RED], 
					pFogColor[GREEN], 
					pFogColor[BLUE], 
					pFogColor[ALPHA]);

	ErrorCode err;

	err = font.Create(hDC);
	if( err )
		Print("Font creation error: %s\n", err);

	err = texture.Create(imgTexture);
	if( err )
		Print("Error creating texture: %s", err);

	cLabel.AdjustSize();

	return TRUE;
}

void glDestroy()
{
	font.Destroy();
	texture.Destroy();
	dlBall.Destroy();
}

void Reshape()
{
	glViewport (0, 0, nWinWidth, nWinHeight);
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

void OnNewInput(const Input &input)
{
	Lock lock(csInput);
	dInput.push_back(input);
	evInput.Signal();
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
			OnNewInput(input);
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
			Input input = {uMsg, InputMouse};
			input.mouse.x = LOWORD(lParam);
			input.mouse.y = HIWORD(lParam);
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

static DWORD WINAPI UpdateProc(void * param)
{
	HANDLE hThread = GetCurrentThread();
	SetThreadPriority(hThread, THREAD_PRIORITY_NORMAL);
	SetThreadName("Update");

	while (bIsProgramLooping)
	{
		Update();
		evInput.Wait(30);
	}

	CloseHandle(hThread);
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	InitRandGen();

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

	Init();

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
					if(PeekMessage(&msg, hWnd, 0, 0, PM_REMOVE) != 0)
					{
						if (msg.message == WM_QUIT)
							break;
						TranslateMessage(&msg);
						DispatchMessage(&msg);
						continue;
					}
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