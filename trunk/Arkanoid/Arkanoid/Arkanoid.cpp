#include <windows.h>			// Windows API Definitions
#include <deque>
#include <map>

#include <math.h>
#include <gl/gl.h>									// Header File For The OpenGL32 Library
#include <gl/glu.h>									// Header File For The GLu32 Library

#include "utils.h"
#include "graphics.h"
#include "ui.h"
#include "math.h"

#pragma comment( lib, "opengl32.lib" )				// Search For OpenGL32.lib While Linking
#pragma comment( lib, "glu32.lib" )					// Search For GLu32.lib While Linking

#ifndef WM_TOGGLEFULLSCREEN							// Application Define Message For Toggling
#	define WM_TOGGLEFULLSCREEN (WM_USER+1)									
#endif

#define LEVEL_WIDTH 20
#define LEVEL_HEIGHT 10
#define MAX_TYPE 4

BYTE nBpp = 32;				// Bits Per Pixel
BYTE nDepth = 32;			// Number Of Bits For The Depth Buffer
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
float fPerspAngle = 60;
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
std::map<std::string, Image> mImages;
const Image *imgBall2D = NULL;
Texture texBall, texParticle, texWood, texStation, texElectronics, texSmile, texLava, texClock, texPlatform;
GLfloat
	pLightAmbient[]= { 0.5f, 0.5f, 0.5f, 1.0f }, // Ambient Light Values
	pLightDiffuse[]= { 1.0f, 1.0f, 1.0f, 1.0f }, // Diffuse Light Values
	pLightGlowDiffuse[]= { 1.0f, 1.0f, 1.0f, 1.0f }, // Effect Light Values
	pLightGlowAttenuate[] = {0.25f, 1.0f, 2.0f},
	pLightPosition[]= { 0.0f, 0.0f, 0.0f, 1.0f }, // Light Position
	pFogColor[]= {0.0f, 0.0f, 0.0f, 1.0f}; // Fog Color
float fFogDensity = 0.1f;
float fLastSimTime = 0;
GLenum uFogQuality = GL_DONT_CARE;
BOOL bKeys[256] = {0};
std::deque<Input> dInput;
CriticalSection csInput;
Timer timer;
DisplayList dlBall, dlBrickBall, dlBrickCube, dlBack, dlSides, dlBottom, dlPlatform;
Transform transform;
const float
	fPlaneZDef = -4.8f,
	fParPlaneZ = -20.0f,
	fBallSpeed = 1.5f,
	fBallRotation = 60.0f,
	fBallR = 0.2,
	fLevelWidth = 5.0f,
	fBrickMargin = 0.01f,
	fBrickSize = fLevelWidth / (LEVEL_WIDTH - 1) - fBrickMargin,
	fBrickRadiusBall = 0.85f * fBrickSize / 2,
	fBrickRadiusCube = fBrickSize / 2,
	fMinDistBase = fBallR + 0.71f * fBrickSize,
	fMinDistBall = fBallR + fBrickRadiusBall,
	fLevelHeight = (LEVEL_HEIGHT - 1) * (fBrickSize + fBrickMargin),
	fLevelOffsetX = 0,
	fLevelOffsetY = 0.5f,
	fLevelMinX = fLevelOffsetX - fLevelWidth / 2,
	fLevelMaxX = fLevelOffsetX + fLevelWidth / 2,
	fLevelMinY = fLevelOffsetY - fLevelHeight / 2,
	fLevelMaxY = fLevelOffsetY + fLevelHeight / 2,
	fMaxSelDist = fBrickSize,
	fMaxSelDist2 = fMaxSelDist * fMaxSelDist,
	fLevelDepth = 1.0f,
	fLevelSpanX = 3.0f,
	fLevelSpanY = 3 * fLevelSpanX / 4,
	fPlatW = 1.0f, fPlatV = 2.0f, fPlatY = -fLevelSpanY + 0.3f,
	fBallXStart = 0, fBallYStart = fPlatY + fBallR, fBallZStart = 0;
float
	fPlaneZ = fPlaneZDef,
	fBallX = fBallXStart, fBallY = fBallYStart, fBallZ = fBallZStart,
	fBallA = 0,
	fBallDirX = 0, fBallDirY = 0,
	fBallRotX = 0, fBallRotY = 1, fBallRotZ = 0,
	fSelX = -1.0f, fSelY = -1.0f, fSelZ = 0.0f,
	fPlatX = 0;
int nMouseX = 0, nMouseY = 0, nNewWinX = -1, nNewWinY = -1, nBallN = 6;
bool bNewBall = false, bNewMouse = false, bNewSelection = false, bValidSpeed = false;
Font font("Times New Roman", -16), smallFont("Courier New", -12);
Event evTask;
Panel c_pEditor, c_pGame, c_pControls, c_pParticles;
Label c_lPath;
Button c_bExit, c_bLoad, c_bSave;
CheckBox c_cbFullscreen, c_cbGeometry, c_cbParticles;
SliderBar c_sFriction, c_sSlowdown, c_sBrick;
Control c_container;
std::deque<float> lfFrameIntervals;
const float fTimeSumMax = 3;
float fLastFrameTime = 0, fFrameInterval = 0, fSimTimeCoef = 1.0f;
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
	fParRotateMax = 50.0f, fParRotateMin = -50.0f,
	fParX0 = 0, fParY0 = 0, fParZ0 = fParPlaneZ;
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
bool bEditor = false, bInterface = false;
const int nBrickCount = LEVEL_WIDTH * LEVEL_HEIGHT;
struct Brick
{
	int type;
	float x, y;
} bricks[ nBrickCount ] = {};
float fJumpEffectZ = 0;
int nSelectedBrick = -1;
std::string strCurrentLevel;
bool bSortDraw = true;

#define rc fBrickRadiusCube
#define rb fBallR
const float fBoxSeg[4][5][2] = {
	{ { 0,-1}, { -rc,      -rc - rb }, {  rc,      -rc - rb }, { -rc, -rc }, {  0,   rb } }, // bottom side
	{ { 1, 0}, {  rc + rb, -rc      }, {  rc + rb,  rc      }, {  rc, -rc }, { -rb,   0 } }, // right side
	{ { 0, 1}, { -rc,       rc + rb }, {  rc,       rc + rb }, {  rc,  rc }, {   0, -rb } }, // top side
	{ {-1, 0}, { -rc - rb, -rc      }, { -rc - rb,  rc      }, { -rc,  rc }, {  rb,   0 } }, // left side
};
#undef rc
#undef rb

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

void UpdateVisibility()
{
	c_pGame.m_bVisible = !bEditor && bInterface;
	c_pEditor.m_bVisible = bEditor;
}

void ToggleEditor()
{
	bEditor = !bEditor;
	UpdateVisibility();
}

void SetBrickType()
{
	if ( nSelectedBrick != -1 )
		bricks[nSelectedBrick].type = Round(c_sBrick.m_slider.m_fValue);
}

bool LoadLevel(const char *pchPath)
{
	if (!pchPath)
		return false;
	strCurrentLevel = pchPath;
	c_lPath.m_strText = pchPath;
	File f;
	if( !f.Open(pchPath, "rt") )
	{
		Print("Cannot open %s for reading!", pchPath);
		return false;
	}
	int index = 0;
	int types[nBrickCount] = {};
	for (int i = 0; i < nBrickCount; i++)
	{
		int type = -1;
		if (fscanf(f, "%d,", &type) <= 0 || type < 0 || type >= MAX_TYPE)
		{
			Print("Corrupted level file: %s", pchPath);
			return false;
		}
		types[i] = type;
	}
	for (int i = 0; i < nBrickCount; i++)
	{
		bricks[i].type = types[i];
	}
	nSelectedBrick = -1;
	fBallX = fBallXStart;
	fBallY = fBallYStart;
	fBallZ = fBallZStart;
	fBallDirX = 0;
	fBallDirY = 0;
	return true;
}

void LoadLevel()
{
	LoadLevel(fd.Open());
}

bool SaveLevel(const char *pchPath)
{
	if (!pchPath)
		return false;
	c_lPath.m_strText = pchPath;
	File f;
	if( !f.Open(pchPath, "wt") )
	{
		Print("Cannot open %s for writing!", pchPath);
		return false;
	}
	for (int i = 0; i < nBrickCount; i++)
		fprintf(f, "%d,", bricks[i].type);
	return true;
}

void SaveLevel()
{
	SaveLevel(fd.Save());
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
	Print("Error reading image '%s': %s\n", pchFilename, err);
	return FALSE;
}

const Image *GetImage(const char *pchImageKey)
{
	auto pair = mImages.find(pchImageKey);
	if( pair == mImages.end() )
		return NULL;
	return &pair->second;
}

BOOL CreateTexture(Texture &tex, const char *pchImageKey)
{
	const Image *img = GetImage(pchImageKey);
	if( !img )
	{
		Print("No such image: %s", pchImageKey);
		return FALSE;
	}
	ErrorCode err = tex.Create(*img);
	if( err )
	{
		Print("Error creating texture: %s", err);
		return FALSE;
	}
	return TRUE;
}

void ScreenToScene(const int &x0, const int &y0, float &x, float &y, float &z)
{
	double dX0, dY0, dDepthZ, dX, dY, dZ;
	transform.Update();
	transform.GetWindowCoor(0, 0, 0, dX0, dY0, dDepthZ);
	transform.GetObjectCoor((double)x0, (double)y0, dDepthZ, dX, dY, dZ);
	x = (float)dX;
	y = (float)dY;
	z = (float)dZ;
}

float SetNewDir(float dx, float dy)
{
	float fDist2 = dx*dx + dy*dy;
	if( fDist2 > 0 )
	{
		float fDistRec = FastInvSqrt(fDist2);
		fBallDirX = dx * fDistRec;
		fBallDirY = dy * fDistRec;
	}
	return fDist2;
}

void ProcessInput()
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
					if( mouse.wheel )
					{
						fPlaneZ += 0.1f * mouse.wheel;
						Print("fPlaneZ = %f\n", fPlaneZ);
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
					case VK_ESCAPE:
						bInterface = !bInterface;
						UpdateVisibility();
						break;
					case VK_F11:
						ToggleFullscreen();
						break;
					case VK_F3:
						ToggleEditor();
						break;
					case VK_F4:
						if( keyboard.alt )
							Terminate();
						break;
					case VK_F6:
						bSortDraw = !bSortDraw;
						break;
					case VK_F7:
						fPlaneZ = fPlaneZDef;
						break;
					case VK_UP:
						if(keyboard.alt && nBallN < 10000)
						{
							nBallN++;
							bNewBall = true;
						}
						break;
					case VK_DOWN:
						if(keyboard.alt && nBallN > 2)
						{
							nBallN--;
							bNewBall = true;
						}
						break;
					case VK_OEM_MINUS:
					case VK_SUBTRACT:
						fSimTimeCoef /= 1.5f;
						Print("fSimTimeCoef = %f\n", fSimTimeCoef);
						break;
					case VK_OEM_PLUS:
					case VK_ADD:
						fSimTimeCoef *= 1.5f;
						Print("fSimTimeCoef = %f\n", fSimTimeCoef);
						break;
					case VK_MULTIPLY:
						fSimTimeCoef = 1;
						Print("fSimTimeCoef = %f\n", fSimTimeCoef);
						break;
					}
				}
			}
		}
	}
}

void Update()
{
	ProcessInput();

	bool bUpdateSelection = bNewSelection;
	bNewSelection = false;

	float time = timer.Time();
	float dt = (time - fLastSimTime) * fSimTimeCoef;
	fLastSimTime = time;

	if( bEditor )
	{
		if( bUpdateSelection )
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
				if (nSelectedBrick != -1)
					c_sBrick.SetValue((float)bricks[nSelectedBrick].type);
			}
		}
		fJumpEffectZ += PI * dt;
	}
	else
	{
		float fPlatX0 = fPlatX;
		if (bKeys[VK_RIGHT])
			fPlatX = min(fLevelSpanX - fPlatW / 2, fPlatX + fPlatV * dt);
		else if (bKeys[VK_LEFT])
			fPlatX = max(-fLevelSpanX + fPlatW / 2, fPlatX - fPlatV * dt);
		fBallA += fBallRotation * dt;
		int nLastCollision = -1;
		float d = fBallSpeed * dt, fNewBallX, fNewBallY;
		for (;;)
		{
			float dx = fBallDirX * d, dy = fBallDirY * d;
			float fBallXc = fBallX + 0.5f * dx, fBallYc = fBallY + 0.5f * dy;
			fNewBallX = fBallX + dx;
			fNewBallY = fBallY + dy;
			if (!bValidSpeed)
				break;
			float fMinDist = fMinDistBase + 0.5f * d, fMinDist2 = fMinDist * fMinDist, colk, coll, colx, coly;
			bool bNewCollision = false;
			int i = 0;
			for(; i < nBrickCount && !bNewCollision; i++)
			{
				Brick &brick = bricks[i];
				if( !brick.type || nLastCollision == i )
					continue;
				float dxc = fBallXc - brick.x, dyc = fBallYc - brick.y;
				if( dxc * dxc + dyc * dyc > fMinDist2 )
					continue;
				switch( brick.type )
				{
				case 1:
				case 3:
					if( dx * (fBallX - brick.x) + dy * (fBallY - brick.y) < 0 && IntersectSegmentCircle2D(fBallX, fBallY, fNewBallX, fNewBallY, brick.x, brick.y, fMinDistBall, &colk) )
					{
						colx = brick.x;
						coly = brick.y;
						bNewCollision = true;
						brick.type = 0;
					}
					break;
				case 2:
					{
						const float xc = brick.x, yc = brick.y;
						// first test collision with each box side
						for(int j = 0; j < 4; j++)
						{
							auto fSeg = fBoxSeg[j];
							if( dx * fSeg[0][0] + dy * fSeg[0][1] > 0 )
								continue;
							float
								fSegX1 = xc + fSeg[1][0],
								fSegY1 = yc + fSeg[1][1],
								fSegX2 = xc + fSeg[2][0],
								fSegY2 = yc + fSeg[2][1];
							if( IntersectSegmentSegment2D(
								fBallX, fBallY, fNewBallX, fNewBallY,
								fSegX1, fSegY1, fSegX2, fSegY2,
								&colk, &coll) )
							{
								colx = fSegX1 + fSeg[4][0] + (fSegX2 - fSegX1) * coll;
								coly = fSegY1 + fSeg[4][1] + (fSegY2 - fSegY1) * coll;
								bNewCollision = true;
								brick.type = 3;
								break;
							}
						}
						// if no side is hit, test collision with each box corner
						if( !bNewCollision )
						{
							for(int j = 0; j < 4; j++)
							{
								auto fCenter = fBoxSeg[j][3];
								float xco = xc + fCenter[0], yco = yc + fCenter[1];
								if( dx * (fBallX - xco) + dy * (fBallY - yco) >= 0 )
									continue;
								if( IntersectSegmentCircle2D(
									fBallX, fBallY, fNewBallX, fNewBallY,
									xco, yco,
									fBallR, &colk) )
								{
									colx = xco;
									coly = yco;
									bNewCollision = true;
									brick.type = 3;
									break;
								}
							}
						}
					}
					break;
				}
			}
			if (!bNewCollision && nLastCollision != i && dy < 0)
			{
				float fPlatSpan = (fPlatW + abs(fPlatX - fPlatX0)) / 2;
				float fMinPlatDist = fBallR + 0.5f * d + fPlatSpan;
				float fPlatXc = (fPlatX + fPlatX0) / 2;
				float dxc = fBallXc - fPlatXc, dyc = fBallYc - fPlatY;
				if (dxc * dxc + dyc * dyc <= fMinPlatDist * fMinPlatDist)
				{
					if (IntersectSegmentSegment2D(
						fBallX, fBallY, fNewBallX, fNewBallY,
						fPlatXc - fPlatSpan, fPlatY + fBallR, fPlatXc + fPlatSpan, fPlatY + fBallR,
						&colk, &coll))
					{
						colx = fPlatXc - fPlatSpan + 2 * fPlatSpan * coll;
						coly = fPlatY;
						bNewCollision = true;
					}
					else if (IntersectSegmentCircle2D(
						fBallX, fBallY, fNewBallX, fNewBallY,
						fPlatXc - fPlatSpan, fPlatY,
						fBallR, &colk))
					{
						colx = fPlatXc - fPlatSpan;
						coly = fPlatY;
						bNewCollision = true;
					}
					else if (IntersectSegmentCircle2D(
						fBallX, fBallY, fNewBallX, fNewBallY,
						fPlatXc + fPlatSpan, fPlatY,
						fBallR, &colk))
					{
						colx = fPlatXc + fPlatSpan;
						coly = fPlatY;
						bNewCollision = true;
					}
				}
			}
			if( !bNewCollision )
				break;
			nLastCollision = i;
			fBallX += dx * colk;
			fBallY += dy * colk;
			DbgClear();
			DbgAddVector(fBallX, fBallY, fBallZ, colx, coly, fBallZ, 0xffffffff, 0xff0000ff);
			DbgAddCircle(fBallX, fBallY, fBallZ, fBallR, 0xff00ffff);

			DbgAddVector(fBallX, fBallY, fBallZ, fBallX + fBallSpeed * fBallDirX, fBallY + fBallSpeed * fBallDirY, fBallZ, 0xffffffff);
			DbgAddVector(fSelX, fSelY, fSelZ, fSelX + (fBallX - fSelX) / 2, fSelY + (fBallY - fSelY) / 2, fSelZ, 0xffffffff);
			DbgAddSpline(
				fBallX, fBallY, fBallZ,
				fBallX + fBallSpeed * fBallDirX, fBallY + fBallSpeed * fBallDirY, fBallZ,
				fSelX + (fBallX - fSelX) / 2, fSelY + (fBallY - fSelY) / 2, fSelZ,
				fSelX, fSelY, fSelZ,
				0xffffff00, 1, 0.001f);

			float xn = fBallX - colx, yn = fBallY - coly;
			float dot = fBallDirX * xn + fBallDirY * yn;
			float len = -2 * dot / (xn * xn + yn * yn);
			SetNewDir(fBallDirX + len * xn, fBallDirY + len * yn);
			d *= 1 - colk;
		}
		fBallX = fNewBallX;
		fBallY = fNewBallY;
		if( fBallDirX < 0 && fBallX - fBallR <= -fLevelSpanX || fBallDirX > 0 && fBallX + fBallR >= fLevelSpanX  )
			fBallDirX = -fBallDirX;
		if( fBallDirY < 0 && fBallY - fBallR <= -fLevelSpanY || fBallDirY > 0 && fBallY + fBallR >= fLevelSpanY  )
			fBallDirY = -fBallDirY;

		// Particles
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
				par.x = fParX0;
				par.y = fParY0;
				par.z = fParZ0;
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
			float dt = fFrameInterval / fSlowdown;
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
	
	FORMAT(buff, "Divs: %d, Sort: %s", nBallN, BOOL_TO_STR(bSortDraw));
	font.Print(buff, (float)nWinWidth/2, (float)nWinHeight, 0xffffffff, ALIGN_CENTER, ALIGN_TOP);

	lfFrameIntervals.push_back(fFrameInterval);
	float fTimeSum = 0;
	int nFrames = 0, nToRemove = 0;
	auto it = lfFrameIntervals.end(), first = it;
	for(;;)
	{
		it--;
		fTimeSum += *it;
		nFrames++;
		if( fTimeSum > fTimeSumMax )
			nToRemove++;
		if( it == lfFrameIntervals.begin() )
			break;
	}
	for(int i = 0; i < nToRemove; i++)
		lfFrameIntervals.pop_front();
	if (nFrames)
	{
		FORMAT(buff, "%.0f", nFrames / fTimeSum);
		font.Print(buff, 5, (float)nWinHeight, 0xffffffff, ALIGN_LEFT, ALIGN_TOP);
	}

	if( !bEditor && bInterface && imgBall2D )
	{
		imgBall2D->Draw(0, 0);
	}
}

void Draw3D()
{
	glTranslatef(0, 0, fPlaneZ);

	if( bNewMouse )
	{
		bNewMouse = false;
		bNewSelection = true;
		ScreenToScene(nNewWinX, nNewWinY, fSelX, fSelY, fSelZ);
		bValidSpeed = SetNewDir(fSelX - fBallX, fSelY - fBallY) > 0;
	}
	
	if( !dlBrickBall )
	{
		CompileDisplayList cds(dlBrickBall);
		DrawSphere(fBrickRadiusBall, 4);
	}
	if( !dlBrickCube )
	{
		CompileDisplayList cds(dlBrickCube);
		DrawCube(2 * fBrickRadiusCube);
	}
	if( !dlBack )
	{
		CompileDisplayList cds(dlBack);
		const float depth = fLevelDepth, spanx = fLevelSpanX, spany = fLevelSpanY;
		glBegin(GL_QUADS);
		// background
		glNormal3f(0.0f, 0.0f, 1.0f);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(-spanx, -spany, -depth);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(-spanx, spany, -depth);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(spanx, spany, -depth);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(spanx, -spany, -depth);
		glEnd();
	}

	if( !dlSides )
	{
		CompileDisplayList cds(dlSides);
		const float depth = fLevelDepth, spanx = fLevelSpanX, spany = fLevelSpanY;
		glBegin(GL_QUADS);
		// left side
		glNormal3f(1.0f, 0.0f, 0.0f);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(-spanx, -spany, +depth);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(-spanx, spany, +depth);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(-spanx, spany, -depth);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(-spanx, -spany, -depth);
		// right side
		glNormal3f(-1.0f, 0.0f, 0.0f);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(spanx, -spany, +depth);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(spanx, spany, +depth);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(spanx, spany, -depth);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(spanx, -spany, -depth);
		// top side
		glNormal3f(0.0f, -1.0f, 0.0f);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(-spanx, spany, +depth);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(spanx, spany, +depth);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(spanx, spany, -depth);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(-spanx, spany, -depth);
		glEnd();
	}

	if( !dlBottom )
	{
		CompileDisplayList cds(dlBottom);
		const float depth = fLevelDepth, spanx = fLevelSpanX, spany = fLevelSpanY;
		glBegin(GL_QUADS);
		// bottom side
		glNormal3f(0.0f, 1.0f, 0.0f);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(-spanx, -spany, +depth);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(spanx, -spany, +depth);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(spanx, -spany, -depth);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(-spanx, -spany, -depth);
		glEnd();
	}

	if( !dlPlatform )
	{
		CompileDisplayList cds(dlPlatform);
		const float hw = fPlatW / 2, hh = 0.25f;
		glBegin(GL_QUADS);
		glNormal3f(0.0f, 1.0f, 0.0f);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(-hw, 0, hh);
		glTexCoord2f(1.0f, 0.0f); glVertex3f( hw, 0, hh);
		glTexCoord2f(1.0f, 1.0f); glVertex3f( hw, 0,-hh);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(-hw, 0,-hh);
		glEnd();
	}
	
	if( !dlBall || bNewBall )
	{
		CompileDisplayList cds(dlBall);
		DrawSphere(fBallR, nBallN);
		bNewBall = false;
	}

	glPushAttrib(GL_ENABLE_BIT);
	glEnable(GL_FOG);
	glDisable(GL_LIGHTING);
	texStation.Bind();
	dlBack.Execute();
	texElectronics.Bind();
	dlSides.Execute();
	texLava.Bind();
	dlBottom.Execute();
	glPopAttrib();

	glPushAttrib(GL_ENABLE_BIT);
	if( bEditor )
	{
		GLfloat pGlowPos[] = {fSelX, fSelY, fSelZ, 1.0f};
		glLightfv(GL_LIGHT2, GL_POSITION, pGlowPos);
		if( nSelectedBrick == -1 )
			glDisable(GL_FOG);
		else
			glEnable(GL_FOG);
	}
	else
	{
		GLfloat pGlowPos[] = {fBallX, fBallY, fBallZ, 1.0f};
		glLightfv(GL_LIGHT2, GL_POSITION, pGlowPos);

		if( bInterface )
		{
			DrawLine3D(fSelX, fSelY, fSelZ, fBallX, fBallY, fBallZ, 0xff00ffff);
			DrawFrame(fSelX, fSelY, fSelZ);
		}

		glPushMatrix();
		glTranslatef(fBallX, fBallY, fBallZ);
		glRotatef(fBallA, fBallRotX, fBallRotY, fBallRotZ);
		texBall.Bind();
		dlBall.Execute();
		glPopMatrix();

		glPushMatrix();
		glTranslatef(fPlatX, fPlatY, 0);
		texPlatform.Bind();
		dlPlatform.Execute();
		glPopMatrix();

	}
	for(int k = 0; k < MAX_TYPE; k++)
	{
		for(int i = 0; i < nBrickCount; i++)
		{
			const Brick &brick = bricks[i];
			if( bSortDraw && brick.type != k )
				continue;
			bool bSelected = i == nSelectedBrick;
			float z = 0;
			glPushAttrib(GL_ENABLE_BIT);
			if( bEditor && bSelected )
			{
				z = 0.1f + 0.1f*sinf(fJumpEffectZ);
				glDisable(GL_FOG);
			}
			glPushMatrix();
			glTranslatef(brick.x, brick.y, z);
			switch( brick.type )
			{
			case 1:
				texSmile.Bind();
				dlBrickBall.Execute();
				break;
			case 2:
				texWood.Bind();
				dlBrickCube.Execute();
				break;
			case 3:
				texClock.Bind();
				dlBrickBall.Execute();
				break;
			default:
				if( bEditor )
				{
					texParticle.Bind();
					dlBrickBall.Execute();
				}
			}
			glPopMatrix();
			glPopAttrib();
		}
		if( !bSortDraw )
			break;
	}
	glPopAttrib();

	DbgDraw();
}

void DrawPar()
{
	bool bShow = c_cbParticles.m_bChecked;
	if( !bShow )
		return;
	glTranslatef(0, 0, fParPlaneZ);
	ScreenToScene(200, 300, fParX0, fParY0, fParZ0);
	texParticle.Bind();
	for (int loop = 0; loop < MAX_PARTICLES; loop++)                   // Loop Through All The Particles
	{
		auto &par = particles[loop];
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

void Draw()
{
	float time = timer.Time();
	fFrameInterval = time - fLastFrameTime;
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
	glPushAttrib(GL_COLOR_BUFFER_BIT | GL_HINT_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
	glDisable(GL_LIGHTING);
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

void Randomize()
{
	for (int i = 0; i < nBrickCount; i++)
		bricks[i].type = Random(MAX_TYPE);
}

bool LoadNextLevel()
{
	bool bReached = strCurrentLevel == "";
	const char *pchCurrentDir = dir.GetCurrent();
	for(;;)
	{
		dir.Reset();
		while(dir.Next())
		{
			auto attrib = dir.GetAttributes();
			if( !attrib.directory && attrib.size )
			{
				char pchPath[MAX_PATH];
				FORMAT(pchPath, "%s\\Data\\%s", pchCurrentDir, dir.GetData().cFileName);
				if( bReached && LoadLevel(pchPath) )
					return true;
				bReached = strCurrentLevel == pchPath;
			}
		}
		if( bReached )
			return false;
		bReached = true;
	}
}

void Init()
{
	const char *pchCurrentDir = dir.GetCurrent();
	dir.Set("Art", "tga");
	while(dir.Next())
	{
		auto attrib = dir.GetAttributes();
		if( !attrib.directory && attrib.size )
		{
			const char *pchFilename = dir.GetData().cFileName;
			int nIdx = strrchr(pchFilename, '.') - pchFilename;
			if( nIdx <= 0 )
				Print("Invalid file name '%s'\n", pchFilename);
			char pchKey[MAX_PATH] = {0}, pchPath[MAX_PATH];
			FORMAT(pchPath, "%s\\Art\\%s", pchCurrentDir, pchFilename);
			strlwr(strncpy(pchKey, pchFilename, nIdx));
			ErrorCode err = mImages[pchKey].ReadTGA(pchPath);
			if( err )
				Print("Error reading image '%s': %s\n", pchFilename, err);
		}
	}

	imgBall2D = GetImage("ball2d");

	texParticle.minFilter = GL_NEAREST;
	texParticle.magFilter = GL_NEAREST;
	texParticle.mipmapped = FALSE;

	texStation.minFilter = GL_LINEAR;
	texStation.magFilter = GL_LINEAR;
	texStation.mipmapped = FALSE;

	texElectronics.minFilter = GL_LINEAR;
	texElectronics.magFilter = GL_LINEAR;
	texElectronics.mipmapped = FALSE;
	
	texLava.minFilter = GL_LINEAR;
	texLava.magFilter = GL_LINEAR;
	texLava.mipmapped = FALSE;

	fd.m_strFileDir = "Data";
	fd.AddFilter("txt", "Text files");
	fd.AddFilter("tga", "Image files");

	font.m_bBold = true;

	c_lPath.SetBounds(130, 10, 390, 20);
	c_lPath.m_nAnchorRight = 10;
	c_lPath.m_pFont = &smallFont;
	c_lPath.m_nMarginX = 5;
	c_lPath.m_nOffsetY = 4;
	c_lPath.m_eAlignH = ALIGN_LEFT;
	c_lPath.m_eAlignV = ALIGN_CENTER;
	c_lPath.m_nBorderColor = 0xff000000;
	c_lPath.m_nForeColor = 0xffcc0000;
	c_lPath.CopyTo(c_bExit);

	c_bExit.SetBounds(50, 10, 120, 60);
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
	c_bExit.CopyTo(c_bLoad);

	c_bLoad.m_nLeft = 10;
	c_bLoad.m_nBottom = 10;
	c_bLoad.m_strText = "Load";
	c_bLoad.m_pOnClick = LoadLevel;
	c_bLoad.CopyTo(c_bSave);

	c_bSave.m_nLeft = 70;
	c_bSave.m_strText = "Save";
	c_bSave.m_pOnClick = SaveLevel;

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

	c_sFriction.CopyTo(c_sBrick);
	c_sBrick.m_name.m_strText = "Type";
	c_sBrick.m_nHeight = 40;
	c_sBrick.m_nBottom = 40;
	c_sBrick.m_nAnchorRight = 10;
	c_sBrick.m_slider.m_nAnchorTop = 5;
	c_sBrick.m_slider.m_nAnchorRight = 5;
	c_sBrick.m_slider.m_fValue = 0;
	c_sBrick.m_slider.m_fMin = 0.0f;
	c_sBrick.m_slider.m_fMax = 2.0f;
	c_sBrick.m_pchFormat = "%.0f";
	c_sBrick.m_slider.OnValueChanged = SetBrickType;

	c_pParticles.Add(&c_sFriction);
	c_pParticles.Add(&c_sSlowdown);

	c_pControls.SetBounds(320, 10, 400, 150);
	c_pControls.m_nBorderColor = 0xff0000ff;
	c_pControls.m_nBackColor = 0xffffffff;
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
	c_pGame.m_bVisible = bInterface;

	c_pEditor.SetBounds(10, 10, 600, 120);
	c_pEditor.SetAnchor(10, -1);
	c_pEditor.m_bVisible = false;
	c_pEditor.Add(&c_lPath);
	c_pEditor.Add(&c_bLoad);
	c_pEditor.Add(&c_bSave);
	c_pEditor.Add(&c_sBrick);

	c_container.Add(&c_pGame);
	c_container.Add(&c_pEditor);
	c_container._Invalidate();

	for(int y = 0, o = 0; y < LEVEL_HEIGHT; y++)
	{
		float fPosY = fLevelMinY + (fLevelMaxY - fLevelMinY) * y / (LEVEL_HEIGHT - 1);
		for(int x = 0; x < LEVEL_WIDTH; x++, o++)
		{
			float fPosX = fLevelMinX + (fLevelMaxX - fLevelMinX) * x / (LEVEL_WIDTH - 1);
			Brick &brick = bricks[o];
			brick.x = fPosX;
			brick.y = fPosY;
		}
	}
	pchCurrentDir = dir.GetCurrent();
	Print("Main directory: %s\n", pchCurrentDir);
	dir.Set("Data", "txt");
	Print("Directory contents for: %s\n", dir.Get());
	while(dir.Next())
	{
		auto attrib = dir.GetAttributes();
		if( !attrib.directory )
			Print("\t%s (%d B)\n", dir.GetData().cFileName, attrib.size);
	}

	LoadNextLevel();
	// Randomize();
}

void Reshape()
{
	glViewport(0, 0, nWinWidth, nWinHeight);
	glScissor(0, 0, nWinWidth, nWinHeight);
	c_container._AdjustSize(nWinWidth, nWinHeight);
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
	CreateLight(GL_LIGHT1, pLightPosition, pLightAmbient, pLightDiffuse);
	CreateLight(GL_LIGHT2, NULL, NULL, pLightGlowDiffuse, NULL, pLightGlowAttenuate );
	glEnable(GL_LIGHTING);
	glDisable(GL_COLOR_MATERIAL);

	glFogi(GL_FOG_MODE, GL_EXP);
	glFogf(GL_FOG_DENSITY, fFogDensity);
	glFogfv(GL_FOG_COLOR, pFogColor);
	glHint(GL_FOG_HINT, uFogQuality);

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

	CreateTexture(texBall,        "ball3d");
	CreateTexture(texParticle,    "star");
	CreateTexture(texWood,        "crate");
	CreateTexture(texStation,     "station");
	CreateTexture(texElectronics, "electronics");
	CreateTexture(texSmile,       "smile");
	CreateTexture(texLava,        "lava");
	CreateTexture(texClock,       "clock");
	CreateTexture(texPlatform,    "platform");

	fd.m_hWnd = hWnd;

	return TRUE;
}

void glDestroy()
{
	dlBall.Destroy();
	dlBrickBall.Destroy();
	dlBrickCube.Destroy();
	dlBack.Destroy();
	dlSides.Destroy();
	dlBottom.Destroy();
	dlPlatform.Destroy();
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

void Task()
{
}

static DWORD WINAPI TaskProc(void * param)
{
	HANDLE hThread = GetCurrentThread();
	SetThreadPriority(hThread, THREAD_PRIORITY_NORMAL);
	SetThreadName("Task");

	while (bIsProgramLooping)
	{
		Task();
		evTask.Wait();
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

	if( !CreateThread(NULL, 0, TaskProc, NULL, 0, NULL) )
	{
		Message("Failed to start the task thread!");
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
							evTask.Signal();
							break;
						}
						TranslateMessage(&msg);
						DispatchMessage(&msg);
						continue;
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
			DestroyWindow();
		}
		else
		{
			Message("Error Creating Window!");
			bIsProgramLooping = FALSE;
		}
	}

	UnregisterClass(pchName, hInstance);

	return 0;
}