#ifndef __UTILS_H_
#define __UTILS_H_

#include <windows.h>			// Windows API Definitions
#include <stdio.h>
#include <list>
#include <string>

#pragma warning(disable:4996)

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

// Returns formatted text
#define FORMAT(buff, fmt, ...) ( _snprintf(buff, sizeof(buff) - 1, fmt, __VA_ARGS__), buff[sizeof(buff) - 1] = 0, buff )

inline int StrLen(const char *text) { return (text && *text) ? (int)strlen(text) : 0; }

typedef const char* ErrorCode;

void Print(const char *fmt, ...);
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

template<class A>
A Clamp(A value, A min, A max)
{
	if (value <= min)
		return min;
	if (value >= max)
		return max;
	return value;
}

class FileDialog
{
protected:
	void Fill(OPENFILENAME &ofn, DWORD flags = 0);
	void Parse(const OPENFILENAME &ofn);
	std::string m_strFilter;
	bool bFilterModified;
	struct FileFilter
	{
		std::string m_strExt, m_strInfo;
		FileFilter(const char *ext = NULL, const char *info = NULL)
		{
			if(!ext)
			{
				m_strExt = "*";
				m_strInfo = "All Files";
			}
			else
			{
				m_strExt = ext;
				m_strInfo = info ? info : "";
			}
		}
	};
	std::list<FileFilter> m_lFilters;
	const char *GetFilterStr();
public:
	char m_pchPath[MAX_PATH + 1], m_pchShortPath[MAX_PATH + 1];
	std::string m_strDefExt, m_strFileDir, m_strFileName, m_strFileExt, m_strNameOnly;
	char m_cDrive;
	
	DWORD m_uFilterIndex;
	HWND m_hWnd;

	FileDialog():m_hWnd(NULL), m_cDrive(0), m_uFilterIndex(0), bFilterModified(false)
	{
		m_strFilter.reserve(MAX_PATH);
		m_pchPath[0] = 0;
		m_pchShortPath[0] = 0;
	}

	void AddFilter(const char *ext = NULL, const char *info = NULL)
	{
		bFilterModified = true;
		m_lFilters.push_back(FileFilter(ext, info));
	}
	void ClearFilters()
	{
		bFilterModified = true;
		m_lFilters.clear();
	}
	const char *Open(const char *pchDirPath = NULL);
	const char *Save(const char *pchFilePath = NULL);
};

struct FileAttributes
{
	__int64 size;
	bool
		archive, compressed, directory,
		encrypted, hidden, normal,
		indexed, remote, readonly,
		link, sparse, system, temp;
	FileAttributes(const WIN32_FIND_DATA &data)
	{
		int flags = data.dwFileAttributes;

		archive = !!(flags & FILE_ATTRIBUTE_ARCHIVE);
		compressed = !!(flags & FILE_ATTRIBUTE_COMPRESSED);
		directory = !!(flags & FILE_ATTRIBUTE_DIRECTORY);
		encrypted = !!(flags & FILE_ATTRIBUTE_ENCRYPTED);
		hidden = !!(flags & FILE_ATTRIBUTE_HIDDEN);
		normal = !!(flags & FILE_ATTRIBUTE_NORMAL);
		indexed = !(flags & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED);
		remote = !!(flags & FILE_ATTRIBUTE_OFFLINE);
		readonly = !!(flags & FILE_ATTRIBUTE_READONLY);
		link = !!(flags & FILE_ATTRIBUTE_REPARSE_POINT);
		sparse = !!(flags & FILE_ATTRIBUTE_SPARSE_FILE);
		system = !!(flags & FILE_ATTRIBUTE_SYSTEM);
		temp = !!(flags & FILE_ATTRIBUTE_TEMPORARY);

		size = data.nFileSizeHigh * ((__int64)MAXDWORD+1) + data.nFileSizeLow;
	}
};

class Directory
{
	HANDLE h;
	char m_pchName[MAX_PATH + 1], m_pchBuff[MAX_PATH + 1];
	void Reset();
	WIN32_FIND_DATA m_data;
public:
	FileAttributes GetAttributes() const { return m_data; }
	const WIN32_FIND_DATA& GetData() const { return m_data; }
	Directory():h(NULL)
	{
		ZeroMemory(m_pchName, sizeof(m_pchName));
		ZeroMemory(m_pchBuff, sizeof(m_pchBuff));
	}
	~Directory(){ Reset(); }
	void Set(const char *path = NULL, const char *ext = NULL);
	const char *Get() const { return m_pchName; }
	BOOL Next();
	const char *GetCurrent();
	static BOOL SetCurrent(const char *current);
};

#endif __UTILS_H_