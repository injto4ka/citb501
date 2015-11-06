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
Image imgBall2D, imgBall3D, imgParticle;
Texture texBall, texParticle;
GLfloat
	pLightAmbient[]= { 0.5f, 0.5f, 0.5f, 1.0f }, // Ambient Light Values
	pLightDiffuse[]= { 1.0f, 1.0f, 1.0f, 1.0f }, // Diffuse Light Values
	pLightPosition[]= { 0.0f, 1.0f, 1.0f, 0.0f }, // Light Position
	pFogColor[]= {0.0f, 0.0f, 0.0f, 1.0f}; // Fog Color
float fFogStart = 0.0f;
float fFogEnd = 10.0f;
float fFogDensity = 0.17f;
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
	fPlaneZ = -4.0f,
	fParPlaneZ = -20.0f,
	fBallX = 0, fBallY = 0, fBallZ = 0,
	fBallX0 = 0, fBallY0 = 0,
	fFrameX = -1.0f, fFrameY = -1.0f, fFrameZ = 0.0f,
	fBallSpeed = 1.0f;
int nMouseX = 0, nMouseY = 0;
volatile int nNewWinX = -1, nNewWinY = -1, nBallN = 16;
volatile float fBallR = 0.5f;
volatile bool bNewBall = false;
Font font("Courier New", -16);
Event evInput;
Panel c_panel;
Label c_label;
Button c_bExit;
CheckBox c_cbFullscreen, c_cbGeometry;
Container c_container;
std::list<float> lfFrameIntervals;
const int nMaxFrames = 300;
float
	fLastFrameTime = 0, fFrameInterval = 0,
	fLastUpdateTime = 0, fUpdateInterval = 0;
bool bGeometry = false;
#define MAX_PARTICLES 1200
#define MAX_PAR_AGE 6
float
	fParSlowdown = 1.0f,
	fParSpeedX = 5.0f,
	fParSpeedY = 3.0f,
	fParSpeedInitMax = 2.0f,
	fParSpeedInitMin = 1.0f,
	fParAccelX = 0.0f,
	fParAccelY =-1.5f,
	fParAccelZ = 0.0f,
	fParFriction = 0.05f,
	fParFadeMin = 0.05f,
	fParFadeMax = 0.5f,
	fParSize = 0.5f,
	fParResizeSpeedMin = 0.05f,
	fParResizeSpeedMax = 0.2f,
	fParRotateSpeedMin = -60.0f,
	fParRotateSpeedMax = 60.0f;
static GLfloat pfParColors[12][3] =
{
    {1.0f,0.5f,0.5f}, {1.0f,0.75f,0.5f}, {1.0f,1.0f,0.5f}, {0.75f,1.0f,0.5f},
    {0.5f,1.0f,0.5f}, {0.5f,1.0f,0.75f}, {0.5f,1.0f,1.0f}, {0.5f,0.75f,1.0f},
    {0.5f,0.5f,1.0f}, {0.75f,0.5f,1.0f}, {1.0f,0.5f,1.0f}, {1.0f,0.5f,0.75f}
};
struct Particle
{
	float age;
    float life;
    float fade;
	float angle;
	float r, g, b;
	float x, y, z, w;
	float va, vx, vy, vz, vw;
}
particles[MAX_PARTICLES] = {0}; 

//=========================================================================================================

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

void ToggleGeometry()
{
	bGeometry = !bGeometry;
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
	float time = timer.Time();
	fUpdateInterval = time - fLastUpdateTime;
	fLastUpdateTime = time;

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
					int x = mouse.x;
					int y = nWinHeight - mouse.y;
					if( !c_container._OnMousePos(x, y, mouse.lbutton) )
					{
						if( mouse.lbutton )
						{
							nNewWinX = x;
							nNewWinY = y;
						}
					}
				}
				break;
			}
			case InputKey:
			{
				const Keyboard &keyboard = input.keyboard;
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
						if(keyboard.alt && fBallR > 0.1f)
						{
							Lock lock(csShared);
							fBallR -= 0.1f;
							bNewBall = true;
						}
						break;
					case VK_RIGHT:
						if(keyboard.alt && fBallR < 10.0f)
						{
							Lock lock(csShared);
							fBallR += 0.1f;
							bNewBall = true;
						}
						break;
					case VK_UP:
						if(keyboard.alt && nBallN < 10000)
						{
							Lock lock(csShared);
							nBallN++;
							bNewBall = true;
						}
						break;
					case VK_DOWN:
						if(keyboard.alt && nBallN > 2)
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

	if( !bKeys[VK_MENU] )
	{
		if (bKeys[VK_NUMPAD8] && (fParAccelY<10)) fParAccelY += 0.5f * fUpdateInterval;
		if (bKeys[VK_NUMPAD2] && (fParAccelY>-10)) fParAccelY -= 0.5f * fUpdateInterval;

		if (bKeys[VK_NUMPAD6] && (fParAccelX<10)) fParAccelX += 0.5f * fUpdateInterval;
		if (bKeys[VK_NUMPAD4] && (fParAccelX>-10)) fParAccelX -= 0.5f * fUpdateInterval;

		if (bKeys[VK_ADD] && (fParSlowdown>0.1f)) fParSlowdown -= 0.5f * fUpdateInterval;
		if (bKeys[VK_SUBTRACT] && (fParSlowdown<20.0f)) fParSlowdown += 0.5f * fUpdateInterval;

		if (bKeys[VK_UP] && (fParSpeedY < 20)) fParSpeedY += 5 * fUpdateInterval;
		if (bKeys[VK_DOWN] && (fParSpeedY >- 20)) fParSpeedY -= 5 * fUpdateInterval;

		if (bKeys[VK_RIGHT] && (fParSpeedX < 20)) fParSpeedX += 5 * fUpdateInterval;
		if (bKeys[VK_LEFT] && (fParSpeedX >- 20)) fParSpeedX -= 5 * fUpdateInterval;
	}
}

void Draw2D()
{
	imgBall2D.Draw(0, 0);

	char buff[128];
	FORMAT(buff, "(%d, %d)", nMouseX, nMouseY);
	font.Print(buff, (float)nWinWidth - 5, (float)nWinHeight, 0xffffffff, ALIGN_RIGHT, ALIGN_TOP);
	
	FORMAT(buff, "Ball N %d", nBallN);
	font.Print(buff, (float)nWinWidth/2, (float)nWinHeight, 0xffffffff, ALIGN_CENTER, ALIGN_TOP);

	lfFrameIntervals.push_back(fFrameInterval);
	while (lfFrameIntervals.size() > nMaxFrames)
		lfFrameIntervals.pop_front();
	float fTimeSum = 0;
	int nFrames = 0;
	for (auto it = lfFrameIntervals.begin(); it != lfFrameIntervals.end(); it++)
	{
		fTimeSum += *it;
		nFrames++;
	}
	if (nFrames)
	{
		FORMAT(buff, "%.1f", nFrames / fTimeSum);
		font.Print(buff, 5, (float)nWinHeight, 0xffffffff, ALIGN_LEFT, ALIGN_TOP);
	}
}

void ScreenToScene(int x0, int y0, float &x, float &y, float &z)
{
	double dX0, dY0, dDepthZ, dX, dY, dZ;
	transform.Update();
	transform.GetWindowCoor(0, 0, 0, dX0, dY0, dDepthZ);
	transform.GetObjectCoor((double)x0, (double)y0, dDepthZ, dX, dY, dZ);
	x = (float)dX;
	y = (float)dY;
	z = (float)dZ;
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
		ScreenToScene(nWinX, nWinY, fFrameX, fFrameY, fFrameZ);
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
	texBall.Bind();
	dlBall.Execute();
	glPopMatrix();
}

void DrawPar()
{
	glTranslatef(0, 0, fParPlaneZ);
	texParticle.Bind();

	float x0, y0, z0;
	ScreenToScene(nMouseX, nMouseY, x0, y0, z0);

	for (int loop = 0; loop < MAX_PARTICLES; loop++)                   // Loop Through All The Particles
	{
		auto &par = particles[loop];
		if (par.life <= 0 || par.age > MAX_PAR_AGE || par.w <= 0)
		{
			par.age = 0;
			par.life = Random(0.0f, 1.0f);
			par.fade = Random(fParFadeMin, fParFadeMax);
			par.x = x0;
			par.y = y0;
			par.z = z0;
			par.w = fParSize;
			float fAngle = Random(0.0f, 2*PI);
			float fParSpeedInit = Random(fParSpeedInitMin, fParSpeedInitMax);
			par.vx = fParSpeedX + fParSpeedInit * cosf(fAngle);
			par.vy = fParSpeedY + fParSpeedInit * sinf(fAngle);
			par.vz = Random(-fParSpeedInit, fParSpeedInit);
			par.vw = Random(fParResizeSpeedMin, fParResizeSpeedMax);
			par.va = Random(fParRotateSpeedMin, fParRotateSpeedMax);
			par.angle = Random(0.0f, 360.0f);
			float *fColor = pfParColors[(loop / 100) % 12];
			par.r = fColor[0];
			par.g = fColor[1];
			par.b = fColor[2];
		}

		float x = par.x;
		float y = par.y;
		float z = par.z;
		float w = par.w;

		glPushMatrix();
		glTranslatef(x, y, 0.0f);
		glRotatef(par.angle, 0.0f, 0.0f, 1.0f);
		glColor4f(par.r, par.g, par.b, par.life); // Material color
		glBegin(GL_TRIANGLE_STRIP);
			glTexCoord2d(1, 1); glVertex3f(+ w, + w, z); // Top Right
			glTexCoord2d(0, 1); glVertex3f(- w, + w, z); // Top Left
			glTexCoord2d(1, 0); glVertex3f(+ w, - w, z); // Bottom Right
			glTexCoord2d(0, 0); glVertex3f(- w, - w, z); // Bottom Left
		glEnd();
		glPopMatrix();

		float dt = fFrameInterval / fParSlowdown;
		par.x += par.vx * dt;
		par.y += par.vy * dt;
		par.z += par.vz * dt;
		par.w += par.vw * dt;
		par.angle += par.va * dt;

		par.vx += (fParAccelX - fParFriction * par.vx) * dt;
		par.vy += (fParAccelY - fParFriction * par.vy) * dt;
		par.vz += (fParAccelZ - fParFriction * par.vz) * dt;

		par.life -= par.fade * dt;
		par.age += dt;
	}
}

void Draw()
{
	float time = timer.Time();
	fFrameInterval = time - fLastFrameTime;
	fLastFrameTime = time;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// 3D ---------------------------------------
	glPushAttrib(GL_ENABLE_BIT);
	glPushAttrib(GL_POLYGON_BIT);
	if( bGeometry )
	{
		glPolygonMode(GL_BACK, GL_LINE); // Back Face Is Filled In
		glPolygonMode(GL_FRONT, GL_LINE); // Front Face Is Drawn With Lines
		glDisable(GL_TEXTURE_2D); 
	}
	else
	{
		glPolygonMode(GL_BACK, GL_FILL);
		glPolygonMode(GL_FRONT, GL_FILL);
		glEnable(GL_TEXTURE_2D); 
	}
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glDisable(GL_COLOR_MATERIAL);
	glEnable(GL_FOG);
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
	glPopAttrib();

	// particles ---------------------------------------
	glPushAttrib(GL_COLOR_BUFFER_BIT);
	glPushAttrib(GL_HINT_BIT);
	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST); 
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glEnable(GL_TEXTURE_2D); 
	glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	DrawPar();
	glPopAttrib();
	glPopAttrib();
	glPopAttrib();

	// 2D ---------------------------------------
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
	c_container._Draw();
	glPopAttrib();
}

void Init()
{
	ReadImage(imgBall2D, "art/ball2d.tga");
	ReadImage(imgBall3D, "art/ball3d.tga");
	ReadImage(imgParticle, "art/star.tga");

	texBall.minFilter = GL_LINEAR_MIPMAP_NEAREST;
	texBall.magFilter = GL_LINEAR;
	texBall.mipmapped = TRUE;

	font.m_bBold = true;

	c_label.SetBounds(-10, 20, 120, 60);
	c_label.m_strText = "Label text";
	c_label.m_pFont = &font;
	c_label.m_nMarginX = 5;
	c_label.m_nOffsetY = 4;
	c_label.m_eAlignH = ALIGN_RIGHT;
	c_label.m_eAlignV = ALIGN_CENTER;
	c_label.m_nForeColor = 0xff0000ff;
	c_label.CopyTo(c_bExit);

	c_bExit.SetBounds(100, 10, 120, 60);
	c_bExit.m_strText = "Exit";
	c_bExit.m_eAlignV = ALIGN_CENTER;
	c_bExit.m_nForeColor = 0xff0000cc;
	c_bExit.m_nBorderColor = 0xff0000cc;
	c_bExit.m_nBackColor = 0xff00cccc;
	c_bExit.m_nOverColor = 0xff00ffff;
	c_bExit.m_nClickColor = 0xffccffff;
	c_bExit.m_pOnClick = Terminate;
	c_bExit.CopyTo(c_cbFullscreen);

	c_cbFullscreen.m_nBottom = 40;
	c_cbFullscreen.m_strText = "Fullscreen";
	c_cbFullscreen.m_nCheckColor = 0xffccffcc;
	c_cbFullscreen.m_pOnClick = ToggleFullscreen;
	c_cbFullscreen.CopyTo(c_cbGeometry);

	c_cbGeometry.m_nBottom = 70;
	c_cbGeometry.m_strText = "Geometry";
	c_cbGeometry.m_pOnClick = ToggleGeometry;

	c_panel.SetBounds(320, 20, 200, 100);
	c_panel.m_nBorderColor = 0xff0000ff;
	c_panel.m_nBackColor = 0xffffffff;
	c_panel.Add(&c_label);
	c_panel.Add(&c_bExit);
	c_panel.Add(&c_cbFullscreen);
	c_panel.Add(&c_cbGeometry);

	c_container.Add(&c_panel);
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

	err = texBall.Create(imgBall3D);
	if( err )
		Print("Error creating ball texBall: %s", err);

	err = texParticle.Create(imgParticle);
	if( err )
		Print("Error creating particle texBall: %s", err);

	c_container._AdjustSize();

	return TRUE;
}

void glDestroy()
{
	font.Destroy();
	texBall.Destroy();
	texParticle.Destroy();
	dlBall.Destroy();
}

//================================================================================================================

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