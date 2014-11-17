#include "utils.h"
#include <time.h>

BOOL File::Open(const char *pchFileName, const char *pchMode)
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
void File::Close()
{
	if(!pFile)
		return;
	fclose(pFile);
	pFile = NULL;
	nId = -1;
}

void Print(const char *fmt, ...)
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

void Message(HWND hWnd, char *fmt, ...)
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

void SetThreadName(LPCSTR name, DWORD threadID)
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

void InitRandGen()
{
	srand((UINT)time(NULL));
}
