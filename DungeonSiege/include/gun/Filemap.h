#pragma once

#ifndef __FILEMAP_H

namespace Gun2
{

//Class designed for 
//one writer many readers
//shared memory
//static, fixed size growing buffers
class CFileMapping 
{
protected:
	TCHAR  m_szFile[MAX_PATH+1];
	HANDLE m_hFile;
	HANDLE m_hMap;
	DWORD  m_MapSize;
	DWORD  m_FileSize;
	DWORD  m_flags;
	void * m_pMapAddress;

public:
	DWORD OpenFileMap( const char *szFile);
	DWORD CreateFileMap( const char *szFile,DWORD maxSize);
	DWORD UnMapFile();
	
	CFileMapping();

};

}
#endif 