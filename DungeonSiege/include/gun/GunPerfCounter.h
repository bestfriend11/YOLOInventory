//
// Gun Performance counters use a shared, memory-mapped file as their base communication
// mechanism with WMI.
//

#pragma once


namespace Gun2  // namespace for all guntech reusable library code.
{

class CSharedCounters;

#define MAX_COUNTER_NAME_SIZE 0x40
#define MAX_COUNTER_INSTANCE_SIZE 0x7F

class CSharedCounterHelper
{
	DWORD m_index;
	
public:
	CSharedCounterHelper();
	
    static DWORD Create(CSharedCounters *pCounters, const char *szType=NULL, const char * szInstance=NULL,DWORD count=128);
	
	DWORD BeginEnum();
	
	DWORD GetNext(char *szType, CSharedCounters **ppCounters);
	
	static DWORD OpenInstance(char *szInstance, CSharedCounters **ppCounters);
};

interface IVariable
{	
	STDMETHOD(	SetInt32)(long value)=0;
	STDMETHOD(	GetInt32)(long *pValue)=0;

	STDMETHOD(	Delete)()=0;
	
};

class CVariable : public IVariable
{
public:
	STDMETHOD(	SetInt32)(long value)
	{
		m_value=value;
		return S_OK;
	};

	STDMETHOD(	GetInt32)(long *pValue)
	{
		*pValue = m_value;
		return S_OK;
	};
private:
	long m_value;
};


class  CSmartVariable
{	
public:
	CSmartVariable()
	{
		m_pVar=NULL;
	}

	~CSmartVariable()
	{
		if (m_pVar)
		{
			m_pVar->Delete();
		}
	}

	IVariable *m_pVar;

	IVariable** operator&()
	{
		return &m_pVar;
	}

	IVariable* operator->() const
	{
		return m_pVar;
	}

	long operator++()   // prefix
	{
		long local;
		m_pVar->GetInt32(&local);
		++local;
		m_pVar->SetInt32(local);
		return local;
	}

	long operator++(int)    // postfix
	{
		long local;
		m_pVar->GetInt32(&local);
		local++;
		m_pVar->SetInt32(local);
		return local;
	}

	long operator--()   // prefix
	{
		long local;
		m_pVar->GetInt32(&local);
		--local;
		m_pVar->SetInt32(local);
		return local;
	}

	long operator--(int)    // postfix
	{
		long local;
		m_pVar->GetInt32(&local);
		local--;
		m_pVar->SetInt32(local);
		return local;
	}

	long operator+=(long value) 
	{
		long local;
		m_pVar->GetInt32(&local);
		m_pVar->SetInt32(local+value);
		return local;
	}
	
	long operator-=(long value) 
	{
		long local;
		m_pVar->GetInt32(&local);
		m_pVar->SetInt32(local-value);
		return local;
	}


	long operator=(long value) 
	{
		m_pVar->SetInt32(value);
		return value;
	}

	long operator=(int value) 
	{
		m_pVar->SetInt32(value);
		return value;
	}

};


//Class designed for
//one writer many readers
//shared memory
//static, fixed size growing buffers
class CFileMapping
{
public:
        DWORD OpenFileMap( const char *szFile);
        DWORD CreateFileMap( const char *szFile,DWORD maxSize);
        DWORD UnMapFile();

        CFileMapping();
protected:
        TCHAR  m_szFile[MAX_PATH+1];
        HANDLE m_hFile;
        HANDLE m_hMap;
        DWORD  m_MapSize;
        DWORD  m_FileSize;
        DWORD  m_flags;
        void * m_pMapAddress;
};

class CSharedCounters : public CFileMapping
{
public:

	CSharedCounters();
	
	DWORD Create(const char *szFile,DWORD instance, const char *szType=NULL,const char *szInstance=NULL, DWORD count=32);
	
	DWORD Open(const char *szFile);
	
	DWORD Close();
	
	DWORD GetCount();
	
	DWORD GetInstanceName(char * szInstance,DWORD size);

	DWORD GetInstance();
	
	DWORD IsActive();
	
	DWORD IsTypeEqual(const char * szType);
	
	DWORD CreateVariable(const char *szName,DWORD type, IVariable **ppVar);
	
	DWORD OpenVariable(const char *szName,DWORD type, IVariable **ppVar);
	
	void Delete();

	DWORD ZeroValues();
private:
//This structure is tightly defined because it will exist
//in shared memory across components that will be compiled
//potentially with different flags
#pragma pack(4)
	struct Counter
	{
		char szName[MAX_COUNTER_NAME_SIZE + 1];
		DWORD value;
	};

	struct 
	__declspec(uuid("{532782FB-2141-4710-9ABB-058A0B239B33}"))	
	CounterArray
	{
		GUID version;
		DWORD number;
		DWORD creatorClosed;
		DWORD instance;
		char  szInstanceName[MAX_COUNTER_INSTANCE_SIZE];
		char  szType[MAX_COUNTER_NAME_SIZE];
		Counter counters[1];
	};

#pragma pack()

	CounterArray * m_pCounters;
	DWORD		m_maxCounters;
	bool		m_bOpen;
	bool		m_bCreateVariableCalled;

};



} // namespace

