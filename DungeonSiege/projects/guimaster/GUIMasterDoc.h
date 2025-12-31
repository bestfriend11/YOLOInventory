// GUIMasterDoc.h : interface of the CGUIMasterDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_GUIMASTERDOC_H__FAFD0BEA_7B11_4E79_B332_022F36C4C8BE__INCLUDED_)
#define AFX_GUIMASTERDOC_H__FAFD0BEA_7B11_4E79_B332_022F36C4C8BE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CGUIMasterDoc : public CDocument
{
protected: // create from serialization only
	CGUIMasterDoc();
	DECLARE_DYNCREATE(CGUIMasterDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGUIMasterDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CGUIMasterDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CGUIMasterDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GUIMASTERDOC_H__FAFD0BEA_7B11_4E79_B332_022F36C4C8BE__INCLUDED_)
