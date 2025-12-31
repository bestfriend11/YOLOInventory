// Dialog_Portraits.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "RapiImage.h"
#include "Dialog_Portraits.h"
#include "siege_engine.h"
#include "FuelDb.h"
#include "Server.h"
#include "Config.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Portraits dialog


Dialog_Portraits::Dialog_Portraits(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Portraits::IDD, pParent)
{
	temp = 0;
	//{{AFX_DATA_INIT(Dialog_Portraits)
	//}}AFX_DATA_INIT
}


void Dialog_Portraits::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Portraits)
	DDX_Control(pDX, IDC_EDIT_ICON_NAME, m_iconName);
	DDX_Control(pDX, IDC_EDIT_TEMPLATE, m_template);
	DDX_Control(pDX, IDC_EDIT_SCID, m_scid);
	DDX_Control(pDX, IDC_CHECK_USE_SKINS, m_useSkins);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Portraits, CDialog)
	//{{AFX_MSG_MAP(Dialog_Portraits)
	ON_BN_CLICKED(ID_BUTTON_GENERATE_PORTRAIT, OnButtonGeneratePortrait)
	ON_BN_CLICKED(IDC_BUTTON_SET_PORTRAIT_WINDOW, OnButtonSetPortraitWindow)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Portraits message handlers

void Dialog_Portraits::OnButtonGeneratePortrait() 
{
	CString rPrefix;
	m_iconName.GetWindowText( rPrefix );

	CString rTemplate;
	m_template.GetWindowText( rTemplate );

	if ( m_useSkins.GetCheck() )
	{	
		SkinVec pants;
		SkinVec shirts;
		SkinVec faces;
		SkinVec hair;

		FuelHandle fhSkins( "world:global:skins" );
		if ( fhSkins.IsValid() )
		{
			FuelHandleList fhlSkins = fhSkins->ListChildBlocks();
			FuelHandleList::iterator i;		

			for ( i = fhlSkins.begin(); i != fhlSkins.end(); ++i ) 
			{
				gpstring sTemplate;
				gpstring sTexture;

				if ( (!(*i)->Get( "group", sTemplate )) || (sTemplate.size() == 0) )
				{
					continue;
				}

				if ( !sTemplate.same_no_case( rTemplate.GetBuffer( 0 ) ) )
				{
					continue;
				}
				
				if ( (!(*i)->Get( "texture", sTexture )) || (sTexture.size() == 0) )
				{
					continue;
				}

				if ( gpstring( "pants*" ).same_no_case( (*i)->GetName() ) )
				{
					pants.push_back( sTexture );
				}
				else if ( gpstring( "shirts*" ).same_no_case( (*i)->GetName() ) )
				{
					shirts.push_back( sTexture );
				}
				else if ( gpstring( "faces*" ).same_no_case( (*i)->GetName() ) )
				{
					faces.push_back( sTexture );
				}
				else if ( gpstring( "hair*" ).same_no_case( (*i)->GetName() ) )
				{
					hair.push_back( sTexture );
				}
				else
				{
					continue;
				}								
			}
		}

		SkinVec::iterator iFace;
		SkinVec::iterator iHair;
		
		GoHandle hObject( m_object );
		if ( hObject.IsValid() )
		{
			//for ( iPants = pants.begin(); iPants != pants.end(); ++iPants )
			//{
				//for ( iShirt = shirts.begin(); iShirt != shirts.end(); ++iShirt )
				//{
					for ( iFace = faces.begin(); iFace != faces.end(); ++iFace )
					{
						for ( iHair = hair.begin(); iHair != hair.end(); ++iHair )
						{
							/*
							gpstring sCloth;
							sCloth += *iPants;
							sCloth += ",";
							sCloth += *iShirt;
							*/						

							gpstring sSkin;
							sSkin += *iFace;
							sSkin += ",";
							sSkin += *iHair;

							// Set pants / shirt
							//hObject->GetAspect()->SetDynamicTexture( PS_CLOTH, sCloth );
																					
							// Set Face	/ Hair
							hObject->GetAspect()->SetDynamicTexture( PS_FLESH, sSkin );
							
							gpstring sName;
							sName += rPrefix.GetBuffer( 0 );
							
							/*
							unsigned int pos = (*iPants).find_last_of( "_" );
							if ( pos != gpstring::npos )
							{
								sName += (*iPants).substr( pos, (*iPants).size() );
							}

							pos = (*iShirt).find_last_of( "_" );
							if ( pos != gpstring::npos )
							{
								sName += (*iShirt).substr( pos, (*iShirt).size() );
							}
							*/
							
							unsigned int pos = (*iFace).find_last_of( "_" );
							if ( pos != gpstring::npos )
							{
								sName += (*iFace).substr( pos, (*iFace).size() );
							}

							pos = (*iHair).find_last_of( "_" );
							if ( pos != gpstring::npos )
							{
								sName += (*iHair).substr( pos, (*iHair).size() );
							}

							gEditor.OnFrame( 0.0f, 0.0f );
							CreatePortrait( sName );
						}
					}
				//}
			//}
		}
	}
	else
	{	
		gpstring sName;
		sName += rPrefix.GetBuffer( 0 );
		sName += rTemplate.GetBuffer( 0 );
		CreatePortrait( sName );
	}
}


void Dialog_Portraits::CreatePortrait( gpstring sName )
{
	std::auto_ptr <RapiMemImage> pImage( gSiegeEngine.Renderer().CreateSubsectionImage( false, &m_portraitRect ) );
	// $$$ Temp
/*	
	temp = gSiegeEngine.Renderer().CreateSubsectionTexture(	m_portraitRect.left, m_portraitRect.top,
															m_portraitRect.right, m_portraitRect.bottom, false, 0, TEX_STATIC );
*/

	// Write it to a file
	gpstring sDir = gConfig.ResolvePathVars( "%out%/art/bitmaps/gui/in_game/inventory/icons/" );
	sDir += sName;
//	::WriteImage( sDir, IMAGE_BMP, *pImage, true, false );
}


void Dialog_Portraits::OnButtonSetPortraitWindow() 
{
	SetAllowDrag( true );	
}

BOOL Dialog_Portraits::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_portraitRect.left		= 0;
	m_portraitRect.right	= 32;
	m_portraitRect.top		= 0;
	m_portraitRect.bottom	= 32;	
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void Dialog_Portraits::Select( Goid object )
{
	GoHandle hObject( object );
	if ( hObject.IsValid() )
	{
		m_object = object;
		m_template.SetWindowText( hObject->GetTemplateName() );
		
		gpstring sScid;
		sScid.assignf( "0x%08X", MakeInt( hObject->GetScid() ) );
		m_scid.SetWindowText( sScid.c_str() );
	}
}


void Dialog_Portraits::Draw()
{	
	gSiegeEngine.Renderer().SetTextureStageState(	0,
													D3DTOP_DISABLE,
													D3DTOP_SELECTARG1,
													D3DTADDRESS_WRAP,
													D3DTADDRESS_WRAP,
													D3DTFG_LINEAR,
													D3DTFN_LINEAR,
													D3DTFP_LINEAR,
													D3DBLEND_SRCALPHA,
													D3DBLEND_INVSRCALPHA,
													false );

	tVertex LineVerts[5];
	memset( LineVerts, 0, sizeof( tVertex ) * 5 );
	DWORD borderColor = 0xff00ff00;			

	LineVerts[0].x			= m_portraitRect.left-0.5f;
	LineVerts[0].y			= m_portraitRect.top-0.5f;
	LineVerts[0].z			= 0.0f;
	LineVerts[0].rhw		= 1.0f;
	LineVerts[0].color		= borderColor;

	LineVerts[1].x			= m_portraitRect.left-0.5f;
	LineVerts[1].y			= m_portraitRect.bottom-0.5f+1.0f;
	LineVerts[1].z			= 0.0f;
	LineVerts[1].rhw		= 1.0f;
	LineVerts[1].color		= borderColor;

	LineVerts[2].x			= m_portraitRect.right-0.5f+1.0f;
	LineVerts[2].y			= m_portraitRect.bottom-0.5f+1.0f;
	LineVerts[2].z			= 0.0f;
	LineVerts[2].rhw		= 1.0f;
	LineVerts[2].color		= borderColor;

	LineVerts[3].x			= m_portraitRect.right-0.5f+1.0f;
	LineVerts[3].y			= m_portraitRect.top-0.5f;
	LineVerts[3].z			= 0.0f;
	LineVerts[3].rhw		= 1.0f;
	LineVerts[3].color		= borderColor;

	LineVerts[4].x			= m_portraitRect.left-0.5f;
	LineVerts[4].y			= m_portraitRect.top-0.5f;
	LineVerts[4].z			= 0.0f;
	LineVerts[4].rhw		= 1.0f;
	LineVerts[4].color		= borderColor;

	gSiegeEngine.Renderer().DrawPrimitive( D3DPT_LINESTRIP, LineVerts, 5, TVERTEX, 0, 0 );

	/*
	if ( temp )
	{
		RECT rect = { 0, 0, 32, 32 };
		gSiegeEngine.Renderer().SetTextureStageState(	0,
													temp != 0 ? D3DTOP_MODULATE : D3DTOP_DISABLE,
													D3DTOP_MODULATE,
													D3DTADDRESS_CLAMP,
													D3DTADDRESS_CLAMP,
													D3DTFG_POINT,		// LINEAR
													D3DTFN_POINT,
													D3DTFP_POINT,
													D3DBLEND_SRCALPHA,
													D3DBLEND_INVSRCALPHA,
													true );

		tVertex Verts[4];
		memset( Verts, 0, sizeof( tVertex ) * 4 );

		DWORD color = 0xffffffff;			

		Verts[0].x			= rect.left-0.5f;
		Verts[0].y			= rect.top-0.5f;
		Verts[0].z			= 0.0f;
		Verts[0].rhw		= 1.0f;
		Verts[0].uv.u		= 0.0f;
		Verts[0].uv.v		= 1.0f;
		Verts[0].color		= color;

		Verts[1].x			= rect.left-0.5f;
		Verts[1].y			= rect.bottom-0.5f;
		Verts[1].z			= 0.0f;
		Verts[1].rhw		= 1.0f;
		Verts[1].uv.u		= 0.0f;
		Verts[1].uv.v		= 0.0f;
		Verts[1].color		= color;

		Verts[2].x			= rect.right-0.5f;
		Verts[2].y			= rect.top-0.5f;
		Verts[2].z			= 0.0f;
		Verts[2].rhw		= 1.0f;
		Verts[2].uv.u		= 1.0f;
		Verts[2].uv.v		= 1.0f;
		Verts[2].color		= color;

		Verts[3].x			= rect.right-0.5f;
		Verts[3].y			= rect.bottom-0.5f;
		Verts[3].z			= 0.0f;
		Verts[3].rhw		= 1.0f;
		Verts[3].uv.u		= 1.0f;
		Verts[3].uv.v		= 0.0f;
		Verts[3].color		= color;

		gSiegeEngine.Renderer().DrawPrimitive( D3DPT_TRIANGLESTRIP, Verts, 4, TVERTEX, &temp, 1 );
	}
	*/
}


void Dialog_Portraits::AdjustRectToMouse( int x, int y )
{
	m_portraitRect.left		= x - 16;
	m_portraitRect.right	= x + 16;
	m_portraitRect.top		= y - 16;
	m_portraitRect.bottom	= y + 16;
}
