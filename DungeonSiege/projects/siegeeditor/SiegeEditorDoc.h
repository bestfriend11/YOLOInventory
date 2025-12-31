// SiegeEditorDoc.h : interface of the CSiegeEditorDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_SIEGEEDITORDOC_H__04160B36_E3C9_4ADB_867A_458C10A0E811__INCLUDED_)
#define AFX_SIEGEEDITORDOC_H__04160B36_E3C9_4ADB_867A_458C10A0E811__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "gpcore.h"


class CSiegeEditorDoc : public CDocument, public Singleton <CSiegeEditorDoc>
{
protected: // create from serialization only
	CSiegeEditorDoc();
	DECLARE_DYNCREATE(CSiegeEditorDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSiegeEditorDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CSiegeEditorDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Set the current document string
	void SetRegionTitle( char * sTitle );

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CSiegeEditorDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#define gLeftView CLeftView::GetSingleton()

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SIEGEEDITORDOC_H__04160B36_E3C9_4ADB_867A_458C10A0E811__INCLUDED_)
