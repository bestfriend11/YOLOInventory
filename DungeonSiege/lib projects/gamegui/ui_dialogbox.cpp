//////////////////////////////////////////////////////////////////////////////
//
// File     :  ui_dialogbox.cpp
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


// Include Files
#include "precomp_gamegui.h"
#include "ui_shell.h"
#include "ui_window.h"
#include "ui_dialogbox.h"
#include "fuel.h"
#include "rapiowner.h"
#include "stringtool.h"
#include "ui_messenger.h"
#include "ui_animation.h"


UIDialogBox::UIDialogBox( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent )
	: UIWindow( fhWindow, parent_interface, parent )
	, box_bottom_left_corner( 0 )
	, box_bottom_right_corner( 0 )
	, box_bottom_side( 0 )
	, box_left_side( 0 )
	, box_right_side( 0 )
	, box_top_left_corner( 0 )	
	, box_top_right_corner( 0 )	
	, box_top_side( 0 )
	, box_fill( 0 )
{	
	SetType( UI_TYPE_DIALOGBOX );
	SetInputType( TYPE_INPUT_ALL );

	bool bCommonCtrl = false;
	if ( fhWindow.Get( "common_control", bCommonCtrl ) ) {
		if ( bCommonCtrl ) 
		{
			gpstring sTemplate;
			fhWindow.Get( "common_template", sTemplate );
			CreateCommonCtrl( sTemplate );		
		}
		return;
	}
}
UIDialogBox::UIDialogBox()
	: UIWindow()
	, box_bottom_left_corner( 0 )
	, box_bottom_right_corner( 0 )
	, box_bottom_side( 0 )
	, box_left_side( 0 )
	, box_right_side( 0 )
	, box_top_left_corner( 0 )	
	, box_top_right_corner( 0 )	
	, box_top_side( 0 )
	, box_fill( 0 )
{
	SetType( UI_TYPE_DIALOGBOX );
	SetInputType( TYPE_INPUT_ALL );
}



UIDialogBox::~UIDialogBox()
{
	if ( box_bottom_left_corner	 != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( box_bottom_left_corner );
	}

	if ( box_bottom_right_corner != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( box_bottom_right_corner );
	}

	if ( box_bottom_side != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( box_bottom_side );
	}

	if ( box_left_side != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( box_left_side );
	}

	if ( box_right_side != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( box_right_side );
	}

	if ( box_top_left_corner != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( box_top_left_corner );
	}

	if ( box_top_right_corner != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( box_top_right_corner );
	}

	if ( box_top_side != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( box_top_side );
	}

	if ( box_fill != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( box_fill );
	}
}



bool UIDialogBox::ProcessAction( UI_ACTION action, gpstring parameter )
{
	if ( UIWindow::ProcessAction( action, parameter ) == true ) {
		return true;
	}	
	return false;
}


bool UIDialogBox::ProcessMessage( UIMessage & msg )
{
	UIWindow::ProcessMessage( msg );

	return true;
}


void UIDialogBox::CreateCommonCtrl( gpstring sTemplate )
{
	SetIsCommonControl( true );	
	if ( sTemplate.same_no_case( "cpbox" ) )
	{
		SetCommonTemplate( sTemplate );
		box_bottom_left_corner		= gUIShell.GetCommonArt( "cpbox_bottom_left_corner" );
		box_bottom_right_corner		= gUIShell.GetCommonArt( "cpbox_bottom_right_corner" );
		box_bottom_side				= gUIShell.GetCommonArt( "cpbox_bottom_side" );
		box_left_side				= gUIShell.GetCommonArt( "cpbox_left_side" );
		box_right_side				= gUIShell.GetCommonArt( "cpbox_right_side" );
		box_top_left_corner			= gUIShell.GetCommonArt( "cpbox_top_left_corner" );		
		box_top_right_corner		= gUIShell.GetCommonArt( "cpbox_top_right_corner" );		
		box_top_side				= gUIShell.GetCommonArt( "cpbox_top_side" );
		box_fill					= gUIShell.GetCommonArt( "cpbox_fill" );	
	}
	else if ( sTemplate.same_no_case( "cpbox_wide" ) )
	{
		SetCommonTemplate( sTemplate );
		box_bottom_left_corner		= gUIShell.GetCommonArt( "cpbox_wide_bottom_left_corner" );
		box_bottom_right_corner		= gUIShell.GetCommonArt( "cpbox_wide_bottom_right_corner" );
		box_bottom_side				= gUIShell.GetCommonArt( "cpbox_wide_bottom_side" );
		box_left_side				= gUIShell.GetCommonArt( "cpbox_wide_left_side" );
		box_right_side				= gUIShell.GetCommonArt( "cpbox_wide_right_side" );
		box_top_left_corner			= gUIShell.GetCommonArt( "cpbox_wide_top_left_corner" );		
		box_top_right_corner		= gUIShell.GetCommonArt( "cpbox_wide_top_right_corner" );		
		box_top_side				= gUIShell.GetCommonArt( "cpbox_wide_top_side" );
		box_fill					= gUIShell.GetCommonArt( "cpbox_fill" );	
	}
	else if ( sTemplate.same_no_case( "cpbox_thin" ) )
	{
		SetCommonTemplate( sTemplate );
		box_bottom_left_corner		= gUIShell.GetCommonArt( "cpbox_thin_bottom_left_corner" );
		box_bottom_right_corner		= gUIShell.GetCommonArt( "cpbox_thin_bottom_right_corner" );
		box_bottom_side				= gUIShell.GetCommonArt( "cpbox_thin_bottom_side" );
		box_left_side				= gUIShell.GetCommonArt( "cpbox_thin_left_side" );
		box_right_side				= gUIShell.GetCommonArt( "cpbox_thin_right_side" );
		box_top_left_corner			= gUIShell.GetCommonArt( "cpbox_thin_top_left_corner" );		
		box_top_right_corner		= gUIShell.GetCommonArt( "cpbox_thin_top_right_corner" );		
		box_top_side				= gUIShell.GetCommonArt( "cpbox_thin_top_side" );
		box_fill					= gUIShell.GetCommonArt( "cpbox_fill" );	
	}
	else if ( sTemplate.same_no_case( "cpbox_thin_dark" ) )
	{
		SetCommonTemplate( sTemplate );
		box_bottom_left_corner		= gUIShell.GetCommonArt( "cpbox_thin_dark_bottom_left_corner" );
		box_bottom_right_corner		= gUIShell.GetCommonArt( "cpbox_thin_dark_bottom_right_corner" );
		box_bottom_side				= gUIShell.GetCommonArt( "cpbox_thin_dark_bottom_side" );
		box_left_side				= gUIShell.GetCommonArt( "cpbox_thin_dark_left_side" );
		box_right_side				= gUIShell.GetCommonArt( "cpbox_thin_dark_right_side" );
		box_top_left_corner			= gUIShell.GetCommonArt( "cpbox_thin_dark_top_left_corner" );		
		box_top_right_corner		= gUIShell.GetCommonArt( "cpbox_thin_dark_top_right_corner" );		
		box_top_side				= gUIShell.GetCommonArt( "cpbox_thin_dark_top_side" );
		box_fill					= gUIShell.GetCommonArt( "cpbox_thin_dark_fill" );	
	}
	else if ( sTemplate.same_no_case( "woodbox" ) )
	{
		SetCommonTemplate( sTemplate );
		box_bottom_left_corner		= gUIShell.GetCommonArt( "woodbox_bottom_left_corner" );
		box_bottom_right_corner		= gUIShell.GetCommonArt( "woodbox_bottom_right_corner" );
		box_bottom_side				= gUIShell.GetCommonArt( "woodbox_bottom_side" );
		box_left_side				= gUIShell.GetCommonArt( "woodbox_left_side" );
		box_right_side				= gUIShell.GetCommonArt( "woodbox_right_side" );
		box_top_left_corner			= gUIShell.GetCommonArt( "woodbox_top_left_corner" );
		box_top_right_corner		= gUIShell.GetCommonArt( "woodbox_top_right_corner" );
		box_top_side				= gUIShell.GetCommonArt( "woodbox_top_side" );
		box_fill					= gUIShell.GetCommonArt( "woodbox_fill" );	
	}
	else if ( sTemplate.same_no_case( "jbox" ) )
	{
		SetCommonTemplate( sTemplate );
		box_bottom_left_corner		= gUIShell.GetCommonArt( "jbox_bottom_left_corner" );
		box_bottom_right_corner		= gUIShell.GetCommonArt( "jbox_bottom_right_corner" );
		box_bottom_side				= gUIShell.GetCommonArt( "jbox_bottom_side" );
		box_left_side				= gUIShell.GetCommonArt( "jbox_left_side" );
		box_right_side				= gUIShell.GetCommonArt( "jbox_right_side" );
		box_top_left_corner			= gUIShell.GetCommonArt( "jbox_top_left_corner" );
		box_top_right_corner		= gUIShell.GetCommonArt( "jbox_top_right_corner" );
		box_top_side				= gUIShell.GetCommonArt( "jbox_top_side" );
		box_fill					= gUIShell.GetCommonArt( "jbox_fill" );	
	}	
	else if ( sTemplate.same_no_case( "graytrim" ) )
	{
		SetCommonTemplate( sTemplate );
		box_bottom_left_corner		= gUIShell.GetCommonArt( "graytrim_bottom_left_corner" );
		box_bottom_right_corner		= gUIShell.GetCommonArt( "graytrim_bottom_right_corner" );
		box_bottom_side				= gUIShell.GetCommonArt( "graytrim_bottom_side" );
		box_left_side				= gUIShell.GetCommonArt( "graytrim_left_side" );
		box_right_side				= gUIShell.GetCommonArt( "graytrim_right_side" );
		box_top_left_corner			= gUIShell.GetCommonArt( "graytrim_top_left_corner" );
		box_top_right_corner		= gUIShell.GetCommonArt( "graytrim_top_right_corner" );
		box_top_side				= gUIShell.GetCommonArt( "graytrim_top_side" );
		box_fill					= gUIShell.GetCommonArt( "cpbox_fill" );	
	}
	else if ( sTemplate.same_no_case( "browntrim" ) )
	{
		SetCommonTemplate( sTemplate );
		box_bottom_left_corner		= gUIShell.GetCommonArt( "browntrim_bottom_left_corner" );
		box_bottom_right_corner		= gUIShell.GetCommonArt( "browntrim_bottom_right_corner" );
		box_bottom_side				= gUIShell.GetCommonArt( "browntrim_bottom_side" );
		box_left_side				= gUIShell.GetCommonArt( "browntrim_left_side" );
		box_right_side				= gUIShell.GetCommonArt( "browntrim_right_side" );
		box_top_left_corner			= gUIShell.GetCommonArt( "browntrim_top_left_corner" );
		box_top_right_corner		= gUIShell.GetCommonArt( "browntrim_top_right_corner" );
		box_top_side				= gUIShell.GetCommonArt( "browntrim_top_side" );
		box_fill					= gUIShell.GetCommonArt( "browntrim_fill" );	
	}
}


void UIDialogBox::Draw()
{
	if ( GetVisible() ) 
	{
		UIWindow::Draw();
		UIRect component_rect;
		UINormalizedRect uv_component_rect;
		
		if ( box_top_side )
		{		
			int sideWidth	= gUIShell.GetRenderer().GetTextureWidth( box_top_side );	

			int top_width	= gUIShell.GetRenderer().GetTextureWidth( box_top_left_corner );
			int top_height	= gUIShell.GetRenderer().GetTextureHeight( box_top_left_corner );

			component_rect.top		= GetRect().top;
			component_rect.left		= GetRect().left;
			component_rect.bottom	= GetRect().top + top_height;
			component_rect.right	= GetRect().left + top_width;
			DrawComponent( component_rect, box_top_left_corner, GetAlpha() );

			component_rect.top		= GetRect().top;
			component_rect.left		= GetRect().right - top_width;
			component_rect.bottom	= GetRect().top + top_height;
			component_rect.right	= GetRect().right;
			DrawComponent( component_rect, box_top_right_corner, GetAlpha() );
				
			component_rect.top		= GetRect().top;
			component_rect.left		= GetRect().left + top_width;
			component_rect.bottom	= GetRect().top + top_height;
			component_rect.right	= GetRect().right - top_width;
			uv_component_rect.top	= 0.0f;
			uv_component_rect.left	= 0.0f;
			uv_component_rect.right	= (float)( component_rect.right-component_rect.left ) / (float)sideWidth;
			uv_component_rect.bottom= 1.0f;
			DrawComponent( component_rect, box_top_side, GetAlpha(), uv_component_rect, true );

			int width	= gUIShell.GetRenderer().GetTextureWidth( box_bottom_left_corner );
			int height	= gUIShell.GetRenderer().GetTextureHeight( box_bottom_left_corner );

			component_rect.top		= GetRect().bottom - height;
			component_rect.left		= GetRect().left + width;
			component_rect.bottom	= GetRect().bottom;
			component_rect.right	= GetRect().right - width;
			uv_component_rect.top	= 0.0f;
			uv_component_rect.left	= 0.0f;
			uv_component_rect.right	= (float)( component_rect.right-component_rect.left ) / (float)sideWidth;
			uv_component_rect.bottom= 1.0f;
			DrawComponent( component_rect, box_bottom_side, GetAlpha(), uv_component_rect, true );

			component_rect.top		= GetRect().bottom - height;
			component_rect.left		= GetRect().left;
			component_rect.bottom	= GetRect().bottom;
			component_rect.right	= GetRect().left + width;
			DrawComponent( component_rect, box_bottom_left_corner, GetAlpha() );

			component_rect.top		= GetRect().bottom - height;
			component_rect.left		= GetRect().right - width;
			component_rect.bottom	= GetRect().bottom;
			component_rect.right	= GetRect().right;
			DrawComponent( component_rect, box_bottom_right_corner, GetAlpha() );

			component_rect.top		= GetRect().top + top_height;
			component_rect.left		= GetRect().left;
			component_rect.bottom	= GetRect().bottom - height;
			component_rect.right	= GetRect().left + width;
			uv_component_rect.top	= 0.0f;
			uv_component_rect.left	= 0.0f;
			uv_component_rect.right	= 1.0f;
			uv_component_rect.bottom= (float)( component_rect.bottom - component_rect.top ) / (float)sideWidth;
			DrawComponent( component_rect, box_left_side, GetAlpha(), uv_component_rect, true );

			component_rect.top		= GetRect().top + top_height;
			component_rect.left		= GetRect().right - width;
			component_rect.bottom	= GetRect().bottom - height;
			component_rect.right	= GetRect().right;
			uv_component_rect.top	= 0.0f;
			uv_component_rect.left	= 0.0f;
			uv_component_rect.right	= 1.0f;
			uv_component_rect.bottom= (float)( component_rect.bottom - component_rect.top ) / (float)sideWidth;
			DrawComponent( component_rect, box_right_side, GetAlpha(), uv_component_rect, true );

			if ( box_fill )
			{
				int fillWidth	= gUIShell.GetRenderer().GetTextureWidth( box_fill );	
				int fillHeight	= gUIShell.GetRenderer().GetTextureHeight( box_fill );	

				component_rect.top		= GetRect().top + top_height;
				component_rect.left		= GetRect().left + width;
				component_rect.bottom	= GetRect().bottom - height;
				component_rect.right	= GetRect().right - width;
				uv_component_rect.top	= 0.0f;
				uv_component_rect.left	= 0.0f;
				uv_component_rect.right	= (float)( component_rect.right - component_rect.left ) / fillHeight;
				uv_component_rect.bottom= (float)( component_rect.bottom - component_rect.top ) / fillWidth;
				DrawComponent( component_rect, box_fill, GetAlpha(), uv_component_rect, true );
			}
		}
	}	
}


#if !GP_RETAIL
void UIDialogBox::Save( FuelHandle hSave, bool bChild )
{
	UIWindow::Save( hSave, bChild );
	if ( GetParentWindow() && ( bChild == false ))
	{
		return;
	}	

	if ( hSave.IsValid() )
	{
		FuelHandle hWindow = hSave->GetChildBlock( GetName(), true );
		if ( hWindow.IsValid() )
		{
			hWindow->SetType( "dialog_box" );
		}
	}
}
#endif