#ifndef __UTILS_H_
#define __UTILS_H_

#include <windows.h>			// Windows API Definitions
#include <stdio.h>

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

typedef const char* ErrorCode;

void Print(char *fmt, ...);
void Message(HWND hWnd, char *fmt, ...);

void SetThreadName(LPCSTR name, DWORD threadID = -1);

inline int Random(int nRange)
{
	return nRange ? rand() % nRange : 0;
}
inline int Random(int nMin, int nMax)
{
	return nMin + Random(nMax - nMin + 1);
}
inline float Random(float fMin, float fMax)
{
	return fMin + (fMax - fMin) * rand() / RAND_MAX;
}

void InitRandGen();

// FLOAT TO INT OPERATIONS:
inline int Trunc(float x)
{
	int retval;
	_asm cvttss2si eax, x
	_asm mov retval, eax
	return retval;
}
inline int Round(float x)
{
	return Trunc(x < 0.0f ? (x - 0.5f) : (x + 0.5f));
}

class File
{
	FILE *pFile;
	int nId;
public:
	File():pFile(NULL), nId(-1) {}
	~File() { Close(); }
	BOOL Open(const char *pchFileName, const char *pchMode = "rb");
	void Close();
	int Descript() const { return nId; }
	operator FILE*(){ return pFile; }
	operator int(){ return nId; }
};

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
};

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
		Restart();
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
};

class Event
{
protected:
	HANDLE m_handle;
public:
	Event(bool bManualReset = false, char *pEventName = 0)
	{
		m_handle = CreateEvent(0, bManualReset, false, pEventName);
	}
	~Event()
	{
		CloseHandle(m_handle);
	}
	bool IsSignaled() const
	{
		return WAIT_OBJECT_0 == ::WaitForSingleObject(m_handle, 0);
	}
	bool Wait(int timeout = -1) const
	{
		return WAIT_OBJECT_0 == ::WaitForSingleObject(m_handle, timeout == -1 ? INFINITE : timeout);
	}
	void Signal() const
	{
		SetEvent(m_handle);
	}
	void Reset() const
	{
		ResetEvent(m_handle);
	}
};


#endif __UTILS_H_