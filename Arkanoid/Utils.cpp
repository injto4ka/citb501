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

const char *FileDialog::GetFilterStr()
{
	if(!m_lFilters.size())
		return NULL;
	if( bFilterModified )
	{
		m_strFilter = "";
		for(auto it = m_lFilters.begin(); it != m_lFilters.end(); it++)
		{
			FileFilter &filter = *it;
			m_strFilter += filter.m_strInfo;
			m_strFilter += " (*.";
			m_strFilter += filter.m_strExt;
			m_strFilter += ")";
			m_strFilter += '\0';
			m_strFilter += "*.";
			m_strFilter += filter.m_strExt;
			m_strFilter += '\0' ;
		}
		m_strFilter += '\0';
	}
	return m_strFilter.c_str();
}

void FileDialog::Fill(OPENFILENAME &ofn, DWORD flags)
{
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = m_hWnd;
	ofn.lpstrFilter = GetFilterStr();
	ofn.nFilterIndex = m_uFilterIndex;
    ofn.lpstrFile = m_pchPath;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = flags | OFN_EXPLORER | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	if(m_strFileDir.size() > 0)
		ofn.lpstrInitialDir = m_strFileDir.c_str();
    ofn.lpstrDefExt = m_strDefExt.c_str();
}

void FileDialog::Parse(const OPENFILENAME &ofn)
{
	m_uFilterIndex = ofn.nFilterIndex;
	GetShortPathName(m_pchPath, m_pchShortPath, MAX_PATH);
	m_cDrive = m_pchPath[0];
	m_strFileName = m_pchPath + ofn.nFileOffset;
	m_strFileDir.assign(m_pchPath, ofn.nFileOffset);
	if(ofn.nFileExtension > 0)
	{
		m_strNameOnly.assign(m_pchPath, ofn.nFileExtension - ofn.nFileOffset - 1, ofn.nFileOffset);
		m_strFileExt = m_pchPath + ofn.nFileExtension;
	}
	else
	{
		m_strNameOnly = m_strFileName;
		m_strFileExt = "";
	}
}

const char *FileDialog::Open(const char *pchDirPath)
{
	if(pchDirPath)
		m_strFileDir = pchDirPath;
	OPENFILENAME ofn;
	Fill(ofn, OFN_FILEMUSTEXIST);
    if(!GetOpenFileName(&ofn))
		return NULL;
	Parse(ofn);
	return m_pchPath;
}

const char *FileDialog::Save(const char *pchFilePath)
{
	if(pchFilePath)
		FORMAT(m_pchPath, "%s", pchFilePath);
	OPENFILENAME ofn;
	Fill(ofn);
    if(!GetSaveFileName(&ofn))
		return NULL;
	Parse(ofn);
	return m_pchPath;
}

void Directory::Set(const char *path, const char *ext)
{
	Reset();
	if( !path )
		path = "";
	if( !ext )
		ext = "*";
	int length = strlen(path);
	if(length > 0 && path[length-1] != '\\')
		FORMAT(m_pchName, "%s\\*.%s", path, ext);
	else
		FORMAT(m_pchName, "%s*.%s", path, ext);
}

BOOL Directory::Next()
{
	if(!h)
	{
		h = FindFirstFile(m_pchName, &m_data);
		if(h == INVALID_HANDLE_VALUE)
		{
			h = NULL;
			return FALSE;
		}
	}
	else if(!FindNextFile(h, &m_data))
	{
		Reset();
		return FALSE;
	}
	return TRUE;
}
const char *Directory::GetCurrent()
{
	DWORD size = GetCurrentDirectory(0, NULL);
	if(!size || size > MAX_PATH || !GetCurrentDirectory(size, m_pchBuff))
		return NULL;
	return m_pchBuff;
}
BOOL Directory::SetCurrent(const char *current)
{
	return SetCurrentDirectory(current);
}
void Directory::Reset()
{
	if(h)
	{
		FindClose(h);
		h=NULL;
	}
}
