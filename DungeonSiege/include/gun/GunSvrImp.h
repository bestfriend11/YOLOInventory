#pragma once

namespace Gun2
{

template<class T> class CGunServerUnknownImp: public T
{
public:
    CGunServerUnknownImp()
    {
        m_refCount = 1;
    }

	//IUnknown
	STDMETHOD(QueryInterface)( REFIID iid, void** ppvObject )
	{
	    *ppvObject = NULL;

		if ( IsEqualGUID( iid, IID_IUnknown ) )
		{
			AddRef();
			*ppvObject = (IUnknown*) this;
		}
		else if ( IsEqualGUID( iid, __uuidof(IServer )) )
		{
			AddRef();
			*ppvObject = (IServer*) this;
		}

		if ( *ppvObject )
		{
			return S_OK;
		}
		else
		{
			return E_NOINTERFACE;
		}

	};
    
    STDMETHOD_(ULONG,AddRef)(void)
	{
	    InterlockedIncrement(&m_refCount);
	    return m_refCount;

	}

    STDMETHOD_(ULONG,Release)(void)
	{
	    ULONG ref = InterlockedDecrement(&m_refCount);
		if ( ref > 0 )
		{
			return ref;
		}
		else
		{
			delete this;
			return 0;
		}
	}

protected:

	LONG		m_refCount;
};

} // namespace

