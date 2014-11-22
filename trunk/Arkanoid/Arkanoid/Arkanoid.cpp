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
DisplayList dlBall, dlBrick;
Transform transform;
float
	fPlaneZ = -4.0f,
	fParPlaneZ = -20.0f,
	fBallX = 0, fBallY = 0, fBallZ = 0, fBallA = 0,
	fBallX0 = 0, fBallY0 = 0,
	fSelX = -1.0f, fSelY = -1.0f, fSelZ = 0.0f,
	fBallSpeed = 1.0f, fBallRotation = 60.0f;
int nMouseX = 0, nMouseY = 0;
volatile int nNewWinX = -1, nNewWinY = -1, nBallN = 6;
volatile float fBallR = 0.5f;
volatile bool bNewBall = false, bNewMouse = false;
Font font("Times New Roman", -16), smallFont("Courier New", -12);
Event evInput;
Panel c_pEditor, c_pGame, c_pControls, c_pParticles;
Label c_lPath;
Button c_bExit, c_bOpen;
CheckBox c_cbFullscreen, c_cbGeometry, c_cbParticles;
SliderBar c_sFriction, c_sSlowdown;
Control c_container;
std::list<float> lfSelIntervals;
const int nMaxFrames = 300;
float
	fLastFrameTime = 0, fSelInterval = 0,
	fLastUpdateTime = 0, fUpdateInterval = 0;
bool bGeometry = false;
#define MAX_PARTICLES 1200
float
	fParSlowdown = 0.0f,
	fParSpeedX = 5.0f, fParSpeedY = 3.0f, 
	fParSpeedInitMin = 1.0f, fParSpeedInitMax = 2.0f,
	fParAccelX = 0.0f, fParAccelY = -1.5f, fParAccelZ = 0.0f,
	fParFriction = 0.05f,
	fParFadeMin = 0.05f, fParFadeMax = 0.5f,
	fParMaxAge = 6.0f,
	fParResizeMax = -0.15f, fParResizeMin = -0.05f,
	fParSizeMin = 0.45f, fParSizeMax = 0.55f,
	fParRotateMax = 50.0f, fParRotateMin = -50.0f;
static GLfloat pfParColors[12][3] =
{
    {1.0f,0.5f,0.5f}, {1.0f,0.75f,0.5f}, {1.0f,1.0f,0.5f}, {0.75f,1.0f,0.5f},
    {0.5f,1.0f,0.5f}, {0.5f,1.0f,0.75f}, {0.5f,1.0f,1.0f}, {0.5f,0.75f,1.0f},
    {0.5f,0.5f,1.0f}, {0.75f,0.5f,1.0f}, {1.0f,0.5f,1.0f}, {1.0f,0.5f,0.75f}
};
struct Particle
{
	float age, alpha, size;
    float fade, resize, rotate;
	float r, g, b;
	float x, y, z;
	float vx, vy,vz;
	float angle;
}
particles[MAX_PARTICLES] = {0};
FileDialog fd;
Directory dir;
bool bEditor = false;

#define LEVEL_WIDTH 20
#define LEVEL_HEIGHT 10
const int nBrickCount = LEVEL_WIDTH * LEVEL_HEIGHT;
struct Brick
{
	int type;
	float x, y;
	void Draw() const
	{
		switch( type )
		{
		case 1:
			texParticle.Bind();
			dlBrick.Execute();
			break;
		default:
			texBall.Bind();
			dlBrick.Execute();
		}
	}
} bricks[ nBrickCount ] = {};
const float
	fLevelMinX = -1.7f,
	fLevelMaxX = 1.7f,
	fLevelOffsetX = 0,
	fLevelOffsetY = -0.5f,
	fLevelMinY = 0.0f,
	fLevelMaxY = 1.7f,
	fBallEditorR = 0.07f,
	fMaxSelDist2 = 0.1f;
float fJumpEffectZ = 0;
int nSelectedBrick = -1;

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

void ToggleEditor()
{
	bEditor = !bEditor;
	c_pGame.m_bVisible = !bEditor;
	c_pEditor.m_bVisible = bEditor;
}

void Browse()
{
	const char *pchPath = fd.Open();
	if( pchPath )
		c_lPath.m_strText = pchPath;
}

void ToggleGeometry()
{
	bGeometry = !bGeometry;
}

void ToggleParticlesDlg()
{
	c_pParticles.m_bVisible = !c_pParticles.m_bVisible;
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
							bNewMouse = true;
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
					case VK_F3:
						ToggleEditor();
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
}

void DrawUI()
{
	c_container._Draw();
}

void Draw2D()
{
	char buff[128];
	FORMAT(buff, "(%d, %d)", nMouseX, nMouseY);
	font.Print(buff, (float)nWinWidth - 5, (float)nWinHeight, 0xffffffff, ALIGN_RIGHT, ALIGN_TOP);
	
	FORMAT(buff, "Ball N %d", nBallN);
	font.Print(buff, (float)nWinWidth/2, (float)nWinHeight, 0xffffffff, ALIGN_CENTER, ALIGN_TOP);

	lfSelIntervals.push_back(fSelInterval);
	while (lfSelIntervals.size() > nMaxFrames)
		lfSelIntervals.pop_front();
	float fTimeSum = 0;
	int nFrames = 0;
	for (auto it = lfSelIntervals.begin(); it != lfSelIntervals.end(); it++)
	{
		fTimeSum += *it;
		nFrames++;
	}
	if (nFrames)
	{
		FORMAT(buff, "%.1f", nFrames / fTimeSum);
		font.Print(buff, 5, (float)nWinHeight, 0xffffffff, ALIGN_LEFT, ALIGN_TOP);
	}

	if( !bEditor )
	{
		imgBall2D.Draw(0, 0);
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
	glTranslatef(0, 0, fPlaneZ);

	bool bUpdateMouse = bNewMouse;
	bNewMouse = false;

	if( nNewWinY >= 0 )
	{
		int nWinX, nWinY;
		{
			Lock lock(csShared);
			nWinX = nNewWinX;
			nWinY = nNewWinY;
		}
		ScreenToScene(nWinX, nWinY, fSelX, fSelY, fSelZ);
	}

	float time = timer.Time();
	float dt = time - fLastDrawTime;
	fLastDrawTime = time;

	if( bEditor )
	{
		if( !dlBrick )
		{
			CompileDisplayList cds(dlBrick);
			DrawSphere(fBallEditorR);
		}
		if( bUpdateMouse )
		{
			int nNewSelectedBrick = -1;
			float fMinDist2 = 0;
			for(int i = 0; i < nBrickCount; i++)
			{
				const Brick &brick = bricks[i];
				float dx = fSelX - brick.x, dy = fSelY - brick.y;
				float fDist2 = dx * dx + dy * dy;
				if( fDist2 < fMaxSelDist2 && (nNewSelectedBrick < 0 || fDist2 < fMinDist2 ) )
				{
					fMinDist2 = fDist2;
					nNewSelectedBrick = i;
				}
			}
			if( nNewSelectedBrick != nSelectedBrick )
			{
				fJumpEffectZ = 0;
				nSelectedBrick = nNewSelectedBrick;
			}
		}
		for(int i = 0; i < nBrickCount; i++)
		{
			const Brick &brick = bricks[i];
			bool bSelected = i == nSelectedBrick;
			float z = 0;
			glPushAttrib(GL_ENABLE_BIT);
			if( bSelected )
			{
				z = 0.1f + 0.1f*sinf(fJumpEffectZ);
				glDisable(GL_FOG);
			}
			glPushMatrix();
			glTranslatef(brick.x, brick.y, z);

			brick.Draw();
			
			glPopAttrib();
			glPopMatrix();
		}
		fJumpEffectZ += PI * dt;
	}
	else
	{
		if( !dlBall || bNewBall )
		{
			Lock lock(csShared);
			CompileDisplayList cds(dlBall);
			DrawSphere(fBallR, nBallN);
			bNewBall = false;
		}

		fBallA += fBallRotation * dt;
		float dx = fSelX - fBallX, dy = fSelY - fBallY, fDist2 = dx*dx + dy*dy;
		if( fDist2 > 1e-6f )
		{
			// Interpolate the ball position towards the target position
			float fDist = sqrtf(fDist2);
			float fTravelTime = fDist / fBallSpeed;
			if( dt > fTravelTime )
			{
				fBallX = fSelX;
				fBallY = fSelY;
			}
			else
			{
				float progress = dt / fTravelTime;
				fBallX += (fSelX - fBallX) * progress;
				fBallY += (fSelY - fBallY) * progress;
			}
		}
		else
		{
			fBallX = fSelX;
			fBallY = fSelY;
		}

		DrawLine(fSelX, fSelY, fSelZ, fBallX, fBallY, fBallZ, 0xff00ffff);
		DrawFrame(fSelX, fSelY, fSelZ);

		glPushMatrix();
		glTranslatef(fBallX, fBallY, fBallZ);
		glRotatef(fBallA, 0, 1, 0);
		texBall.Bind();
		dlBall.Execute();
		glPopMatrix();
	}
}

void DrawPar()
{
	glTranslatef(0, 0, fParPlaneZ);

	if( bEditor )
	{
	}
	else
	{
		texParticle.Bind();

		float x0, y0, z0;
		ScreenToScene(200, 300, x0, y0, z0);

		for (int loop = 0; loop < MAX_PARTICLES; loop++)                   // Loop Through All The Particles
		{
			auto &par = particles[loop];
			if (par.alpha <= 0 || par.age > fParMaxAge)
			{
				par.age = 0;
				par.alpha = Random(0.0f, 1.0f);
				par.fade = Random(fParFadeMin, fParFadeMax);
				par.resize = Random(fParResizeMin, fParResizeMax);
				par.rotate = Random(fParRotateMin, fParRotateMax);
				par.size = Random(fParSizeMin, fParSizeMax);
				par.x = x0;
				par.y = y0;
				par.z = z0;
				float fAngle = Random(0.0f, 2*PI);
				float fParSpeedInit = Random(fParSpeedInitMin, fParSpeedInitMax);
				par.vx = fParSpeedX + fParSpeedInit * cosf(fAngle);
				par.vy = fParSpeedY + fParSpeedInit * sinf(fAngle);
				par.vz = Random(-fParSpeedInit, fParSpeedInit);
				float *fColor = pfParColors[(loop / 100) % 12];
				par.r = fColor[0];
				par.g = fColor[1];
				par.b = fColor[2];
			}

			float fSlowdown = expf(0.69314718056f * c_sSlowdown.m_slider.m_fValue);
			float dt = fSelInterval / fSlowdown;
			par.x += par.vx * dt;
			par.y += par.vy * dt;
			par.z += par.vz * dt;

			float fFriction = c_sFriction.m_slider.m_fValue;
			par.vx += (fParAccelX - fFriction * par.vx) * dt;
			par.vy += (fParAccelY - fFriction * par.vy) * dt;
			par.vz += (fParAccelZ - fFriction * par.vz) * dt;

			par.alpha -= par.fade * dt;
			par.size += par.resize * dt;
			par.angle += par.rotate * dt;
			par.age += dt;

			glColor4f(par.r, par.g, par.b, par.alpha); // Material color
			glPushMatrix();
			glTranslatef(par.x, par.y, par.z);
			glRotatef(par.angle, 0, 0, 1);
			glBegin(GL_TRIANGLE_STRIP);
				float s = par.size;
				glTexCoord2d(1, 1); glVertex3f(s, s, 0); // Top Right
				glTexCoord2d(0, 1); glVertex3f(-s, s, 0); // Top Left
				glTexCoord2d(1, 0); glVertex3f(s, -s, 0); // Bottom Right
				glTexCoord2d(0, 0); glVertex3f(-s, -s, 0); // Bottom Left
			glEnd();
			glPopMatrix();
		}
	}
}

void Draw()
{
	float time = timer.Time();
	fSelInterval = time - fLastFrameTime;
	fLastFrameTime = time;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// 3D ---------------------------------------
	glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT);
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

	// particles ---------------------------------------
	glPushAttrib(GL_COLOR_BUFFER_BIT | GL_HINT_BIT | GL_ENABLE_BIT);
	glDisable(GL_LIGHTING);
	glDisable(GL_COLOR_MATERIAL);
	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glEnable(GL_TEXTURE_2D); 
	glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	DrawPar();
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
	DrawUI();
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

	fd.AddFilter("txt", "Text files");
	fd.AddFilter("cpp", "Source files");
	fd.AddFilter("tga", "Image files");

	font.m_bBold = true;

	c_lPath.SetBounds(5, 125, 390, 20);
	c_lPath.m_strText = "";
	c_lPath.m_pFont = &smallFont;
	c_lPath.m_nMarginX = 5;
	c_lPath.m_nOffsetY = 4;
	c_lPath.m_eAlignH = ALIGN_LEFT;
	c_lPath.m_eAlignV = ALIGN_CENTER;
	c_lPath.m_nBorderColor = 0xff000000;
	c_lPath.m_nForeColor = 0xff0000ff;
	c_lPath.CopyTo(c_bExit);

	c_bExit.SetBounds(100, 10, 120, 60);
	c_bExit.m_strText = "Exit";
	c_bExit.m_pFont = &font;
	c_bExit.m_eAlignV = ALIGN_CENTER;
	c_bExit.m_nForeColor = 0xff0000cc;
	c_bExit.m_nBorderColor = 0xff0000cc;
	c_bExit.m_nBackColor = 0xff00cccc;
	c_bExit.m_nOverColor = 0xff00ffff;
	c_bExit.m_nClickColor = 0xffccffff;
	c_bExit.m_pOnClick = Terminate;
	c_bExit.m_bAutoSize = true;
	c_bExit.CopyTo(c_cbFullscreen);
	c_bExit.CopyTo(c_bOpen);

	c_bOpen.m_nLeft = 10;
	c_bOpen.m_nBottom = 80;
	c_bOpen.m_strText = "Browse";
	c_bOpen.m_pOnClick = Browse;

	c_cbFullscreen.m_nBottom = 40;
	c_cbFullscreen.m_strText = "Fullscreen";
	c_cbFullscreen.m_nCheckColor = 0xffccffcc;
	c_cbFullscreen.m_pOnClick = ToggleFullscreen;
	c_cbFullscreen.CopyTo(c_cbGeometry);

	c_cbGeometry.m_nBottom = 70;
	c_cbGeometry.m_strText = "Geometry";
	c_cbGeometry.m_pOnClick = ToggleGeometry;
	c_cbGeometry.CopyTo(c_cbParticles);

	c_cbParticles.m_nBottom = 100;
	c_cbParticles.m_strText = "Particles";
	c_cbParticles.m_pOnClick = ToggleParticlesDlg;

	c_sFriction.SetBounds(10, 10, 310, 30);
	c_sFriction.m_nBackColor = 0xffffff00;
	c_sFriction.m_nBorderColor = 0xff000000;
	c_sFriction.m_pchFormat = "%.2f";

	c_lPath.CopyTo(c_sFriction.m_name);
	c_sFriction.m_name.m_pFont = &font;
	c_sFriction.m_name.m_nBorderColor = 0;
	c_sFriction.m_name.SetBounds(5, 5, 90, 20);
	c_sFriction.m_name.m_eAlignH = ALIGN_RIGHT;
	c_sFriction.m_name.m_strText = "Friction";
	c_sFriction.m_name.m_nForeColor = 0xff000000;

	c_sFriction.m_name.CopyTo(c_sFriction.m_value);
	c_sFriction.m_value.SetBounds(105, 5, 90, 20);
	c_sFriction.m_value.m_eAlignH = ALIGN_CENTER;
	c_sFriction.m_value.m_nForeColor = 0xff000000;

	c_sFriction.m_slider.SetBounds(200, 5, 100, 20);
	c_sFriction.m_slider.m_nBorderColor = 0xff000000;
	c_sFriction.m_slider.m_nBackColor = 0xffffffff;
	c_sFriction.m_slider.m_fValue = fParFriction;
	c_sFriction.m_slider.m_fMax = 1;
	c_sFriction.m_slider.m_fMin = 0;

	c_sFriction.CopyTo(c_sSlowdown);
	c_sSlowdown.m_name.m_strText = "Slowdown";
	c_sSlowdown.m_nBottom = 40;
	c_sSlowdown.m_slider.m_fValue = 0;
	c_sSlowdown.m_slider.m_fMax = 3.0f;
	c_sSlowdown.m_slider.m_fMin = -3.0f;

	c_pParticles.Add(&c_sFriction);
	c_pParticles.Add(&c_sSlowdown);

	c_pControls.SetBounds(320, 10, 400, 150);
	c_pControls.m_nBorderColor = 0xff0000ff;
	c_pControls.m_nBackColor = 0xffffffff;
	c_pControls.Add(&c_lPath);
	c_pControls.Add(&c_bOpen);
	c_pControls.Add(&c_bExit);
	c_pControls.Add(&c_cbFullscreen);
	c_pControls.Add(&c_cbGeometry);
	c_pControls.Add(&c_cbParticles);
	c_pControls.CopyTo(c_pParticles);
	c_pControls.CopyTo(c_pEditor);

	c_pParticles.SetBounds(320, 170, 400, 100);
	c_pParticles.m_bVisible = false;

	c_pGame.Add(&c_pControls);
	c_pGame.Add(&c_pParticles);

	c_pEditor.SetBounds(10, 10, 600, 100);
	c_pEditor.SetAnchor(10, -1);
	c_pEditor.m_bVisible = false;

	c_container.Add(&c_pGame);
	c_container.Add(&c_pEditor);

	c_container._Invalidate();

	for(int y = 0, o = 0; y < LEVEL_HEIGHT; y++)
	{
		float fPosY = fLevelOffsetY + fLevelMinY + (fLevelMaxY - fLevelMinY) * y / (LEVEL_HEIGHT - 1);
		for(int x = 0; x < LEVEL_WIDTH; x++, o++)
		{
			float fPosX = fLevelOffsetX + fLevelMinX + (fLevelMaxX - fLevelMinX) * x / (LEVEL_WIDTH - 1);
			Brick &brick = bricks[o];
			brick.x = fPosX;
			brick.y = fPosY;
		}
	}
	for(int i = 0; i < 30; i++)
		bricks[Random(nBrickCount)].type = 1;
}

void Redraw()
{
	Draw();
	SwapBuffers(hDC);
}

void Reshape()
{
	glViewport(0, 0, nWinWidth, nWinHeight);
	glScissor(0, 0, nWinWidth, nWinHeight);
	c_container._AdjustSize(nWinWidth, nWinHeight);
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

	err = smallFont.Create(hDC);
	if( err )
		Print("Font creation error: %s\n", err);

	err = texBall.Create(imgBall3D);
	if( err )
		Print("Error creating ball texBall: %s", err);

	err = texParticle.Create(imgParticle);
	if( err )
		Print("Error creating particle texBall: %s", err);

	fd.m_hWnd = hWnd;

	return TRUE;
}

void glDestroy()
{
	font.Destroy();
	texBall.Destroy();
	texParticle.Destroy();
	dlBall.Destroy();
	dlBrick.Destroy();
}

//================================================================================================================

void InitWindow()
{
	nStyle = WS_OVERLAPPEDWINDOW;
	nExtStyle = WS_EX_APPWINDOW;
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

	Print("Main directory: %s\n", dir.GetCurrent());
	dir.Set("", "cpp");
	Print("Directory contents for: %s\n", dir.Get());
	while(dir.Next())
	{
		auto attrib = dir.GetAttributes();
		if( !attrib.directory )
			Print("\t%s (%d KB)\n", dir.GetData().cFileName, attrib.size / 1024);
	}

	hInst = hInstance;
	pchCmdLine = lpCmdLine;
	nShow = nCmdShow;
	
	LPTSTR pchName = "Arkanoid";
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
				ShowWindow(hWnd, nShow);
				for(;;)
				{
					MSG msg; // Message Info
					if(PeekMessage(&msg, hWnd, 0, 0, PM_REMOVE) != 0)
					{
						if (msg.message == WM_QUIT)
						{
							evInput.Signal();
							break;
						}
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