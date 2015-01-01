#ifndef __APP_H_
#define __APP_H_

#include <windows.h>

#include "ui.h"

#define Message(fmt, ...) Message(hWnd, fmt, __VA_ARGS__)

struct Application
{
	LPTSTR pchName, pchCmdLine;
	int nShow, nMouseX, nMouseY;
	HINSTANCE hInst;
	HWND hWnd;
	HDC hDC;
	HGLRC hRC;
	BOOL bIsProgramLooping, bCreateFullScreen, bKeys[256], bFullscreen, bVisible, bAllowSleep, bAllowYield;
	Control c_container;
	LONG nLeft, nLastLeft, nTop, nLastTop, nWinWidth, nWinHeight;
	DWORD nWinStyle, nWinExtStyle;
	LONG nDisplayWidth, nDisplayHeight;
	BYTE nBpp, nDepth, nStencil;
	RECT rect;

	Application(LPTSTR pchName);
	~Application();

	void Main(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow);
	LRESULT Proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void InitWindow();
	BOOL CreateNewWindow();
	void DestroyCurWindow();
	void Reshape();
	void Redraw();
	void Terminate();
	void ToggleFullscreen();

	BOOL Create();
	BOOL glCreate();
	void glDestroy();
	void Destroy();
	void Draw();
	void OnInput(const Input &input);
	void Update();
};

#endif __APP_H_