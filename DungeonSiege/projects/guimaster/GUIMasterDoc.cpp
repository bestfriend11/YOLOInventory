// GUIMasterDoc.cpp : implementation of the CGUIMasterDoc class
//

#include "stdafx.h"
#include "GUIMaster.h"

#include "GUIMasterDoc.h"


#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGUIMasterDoc

IMPLEMENT_DYNCREATE(CGUIMasterDoc, CDocument)

BEGIN_MESSAGE_MAP(CGUIMasterDoc, CDocument)
	//{{AFX_MSG_MAP(CGUIMasterDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGUIMasterDoc construction/destruction

CGUIMasterDoc::CGUIMasterDoc()
{
	// TODO: add one-time construction code here

}

CGUIMasterDoc::~CGUIMasterDoc()
{
}

BOOL CGUIMasterDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CGUIMasterDoc serialization

void CGUIMasterDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CGUIMasterDoc diagnostics

#ifdef _DEBUG
void CGUIMasterDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CGUIMasterDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CGUIMasterDoc commands
