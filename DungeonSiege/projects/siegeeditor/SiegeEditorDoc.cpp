// SiegeEditorDoc.cpp : implementation of the CSiegeEditorDoc class
//

#include "PrecompEditor.h"
#include "SiegeEditor.h"
#include "SiegeEditorDoc.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSiegeEditorDoc

IMPLEMENT_DYNCREATE(CSiegeEditorDoc, CDocument)

BEGIN_MESSAGE_MAP(CSiegeEditorDoc, CDocument)
	//{{AFX_MSG_MAP(CSiegeEditorDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSiegeEditorDoc construction/destruction

CSiegeEditorDoc::CSiegeEditorDoc()
{
	// TODO: add one-time construction code here

}

CSiegeEditorDoc::~CSiegeEditorDoc()
{
}

BOOL CSiegeEditorDoc::OnNewDocument()
{
	CDocument::SetTitle( "No Region Loaded" );
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)
	gEditor.SetDocument( this );

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CSiegeEditorDoc serialization

void CSiegeEditorDoc::Serialize(CArchive& ar)
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
// CSiegeEditorDoc diagnostics

#ifdef _DEBUG
void CSiegeEditorDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CSiegeEditorDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CSiegeEditorDoc commands


void CSiegeEditorDoc::SetRegionTitle( char * sTitle )
{
	CDocument::SetTitle( sTitle );
}