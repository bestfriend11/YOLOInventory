/*=====================================================================

	Name	:	UIItemOverlay
	Date	:	Jan, 2001
	Author	:	Adam Swensen

---------------------------------------------------------------------*/

#include "Precomp_Game.h"
#include "UIItemOverlay.h"
#include "FuBiSchema.h"
#include "Go.h"
#include "GoCore.h"
#include "GoAspect.h"
#include "GoData.h"
#include "GoDb.h"
#include "GoDefs.h"
#include "GoMagic.h"
#include "UI_Text.h"
#include "siege_pos.h"
#include "nema_aspect.h"
#include "UIGame.h"
#include "UICommands.h"
#include "siege_mouse_shadow.h"
#include "Server.h"
#include "Player.h"


UIItemOverlay::UIItemOverlay( int maxVisibleItems )
	: m_Visible          ( false )
	, m_MaxVisibleLabels ( maxVisibleItems )		
	, m_TextPadding      ( 3 )
	, m_DraggedGoid		 ( GOID_INVALID )
	, m_dwLabelColor	 ( 0xFFFFFFFF )
	, m_dwLabelSelectedColor( 0xFFFFADAD )
	, m_dwLabelHoverColor( 0xFFFF0000 )
	, m_bSortAlternate	 ( false )
	, m_HoverGoid		 ( GOID_INVALID )
{	
	// create the item overlay interface
	gUIShell.AddBlankInterface( "ui_item_overlay" );
	gUIShell.HideInterface( "ui_item_overlay" );

	// add textbox windows to interface
	m_pLabels = new ScreenItem[ m_MaxVisibleLabels ];
	memset( (void *)m_pLabels, 0, sizeof ( ScreenItem *) * m_MaxVisibleLabels );	

	for ( int i = 0; i < m_MaxVisibleLabels; ++i )
	{
		m_pLabels[i].m_pText = new UIText;
		
		m_pLabels[i].m_pText->SetName( gpstringf( "item_overlay_text%d", i ) );
		m_pLabels[i].m_pText->LoadTexture( "b_gui_cmn_box_alpha_154" );
		m_pLabels[i].m_pText->SetHasTexture( true );
		m_pLabels[i].m_pText->SetAlpha( 0.75f );
		m_pLabels[i].m_pText->SetFont( "b_gui_fnt_12p_copperplate-light" );
		m_pLabels[i].m_pText->SetColor( m_dwLabelColor );
		m_pLabels[i].m_pText->SetBorder( true );
		m_pLabels[i].m_pText->SetBorderColor( 0 );
		m_pLabels[i].m_pText->SetBorderPadding( -1 );
		m_pLabels[i].m_pText->SetDrawOrder( i );
		m_pLabels[i].m_pText->SetAutoSize( true );
		m_pLabels[i].m_pText->SetJustification( JUSTIFY_CENTER );
		m_pLabels[i].m_pText->SetPassThrough( true );

		{
			UI_EVENT ue;
			EVENT_ACTION ea;
			ue.msg_code = MSG_LBUTTONDOWN;
			ea.action = ACTION_NOTIFY;
			ea.parameter = "item_overlay_ldown";
			ue.event_actions.push_back( ea );
			m_pLabels[i].m_pText->GetEvents()->push_back( ue );
		}
		{
			UI_EVENT ue;
			EVENT_ACTION ea;
			ue.msg_code = MSG_RBUTTONDOWN;
			ea.action = ACTION_NOTIFY;
			ea.parameter = "item_overlay_rdown";
			ue.event_actions.push_back( ea );
			m_pLabels[i].m_pText->GetEvents()->push_back( ue );
		}
		{
			UI_EVENT ue;
			EVENT_ACTION ea;
			ue.msg_code = MSG_BUTTONPRESS;
			ea.action = ACTION_NOTIFY;
			ea.parameter = "item_overlay_lselect";
			ue.event_actions.push_back( ea );
			m_pLabels[i].m_pText->GetEvents()->push_back( ue );
		}
		{
			UI_EVENT ue;
			EVENT_ACTION ea;
			ue.msg_code = MSG_RBUTTONPRESS;
			ea.action = ACTION_NOTIFY;
			ea.parameter = "item_overlay_rselect";
			ue.event_actions.push_back( ea );
			m_pLabels[i].m_pText->GetEvents()->push_back( ue );
		}
		{
			UI_EVENT ue;
			EVENT_ACTION ea;
			ue.msg_code = MSG_GLOBALLBUTTONUP;
			ea.action = ACTION_NOTIFY;
			ea.parameter = "item_overlay_release";
			ue.event_actions.push_back( ea );
			m_pLabels[i].m_pText->GetEvents()->push_back( ue );
		}
		{
			UI_EVENT ue;
			EVENT_ACTION ea;
			ue.msg_code = MSG_GLOBALRBUTTONUP;
			ea.action = ACTION_NOTIFY;
			ea.parameter = "item_overlay_release";
			ue.event_actions.push_back( ea );
			m_pLabels[i].m_pText->GetEvents()->push_back( ue );
		}
		{
			UI_EVENT ue;
			EVENT_ACTION ea;
			ue.msg_code = MSG_LDOUBLECLICK;
			ea.action = ACTION_NOTIFY;
			ea.parameter = "item_overlay_lselectall";
			ue.event_actions.push_back( ea );
			m_pLabels[i].m_pText->GetEvents()->push_back( ue );
		}
		{
			UI_EVENT ue;
			EVENT_ACTION ea;
			ue.msg_code = MSG_RDOUBLECLICK;
			ea.action = ACTION_NOTIFY;
			ea.parameter = "item_overlay_rselectall";
			ue.event_actions.push_back( ea );
			m_pLabels[i].m_pText->GetEvents()->push_back( ue );
		}
		{
			UI_EVENT ue;
			EVENT_ACTION ea;
			ue.msg_code = MSG_ROLLOVER;
			ea.action = ACTION_NOTIFY;
			ea.parameter = "item_overlay_rollover";
			ue.event_actions.push_back( ea );
			m_pLabels[i].m_pText->GetEvents()->push_back( ue );
		}
		{
			UI_EVENT ue;
			EVENT_ACTION ea;
			ue.msg_code = MSG_ROLLOFF;
			ea.action = ACTION_NOTIFY;
			ea.parameter = "item_overlay_rolloff";
			ue.event_actions.push_back( ea );
			m_pLabels[i].m_pText->GetEvents()->push_back( ue );
		}

		gUIShell.AddWindowToInterface( m_pLabels[i].m_pText, "ui_item_overlay" );
	}

	FastFuelHandle hSettings( "ui:config:item_overlay:item_overlay_settings" );
	if ( hSettings )
	{
		hSettings.Get( "label_color", m_dwLabelColor );
		hSettings.Get( "label_selected_color", m_dwLabelSelectedColor );
		hSettings.Get( "label_hover_color", m_dwLabelHoverColor );	
	}
}

UIItemOverlay::~UIItemOverlay()
{
	gUIShell.DeactivateInterface( "ui_item_overlay" );
	delete [] m_pLabels;
	m_pLabels = NULL;
}

void UIItemOverlay::SetVisible( bool visible )
{
	UICheckbox * pWindow = (UICheckbox *)gUIShell.FindUIWindow( "window_labels", "data_bar" );
	if ( visible && !m_Visible )
	{
		if ( pWindow && pWindow->GetState() != visible )
		{
			pWindow->SetState( visible );			
		}
		gUIShell.ShowInterface( "ui_item_overlay" );
	}
	else if ( !visible && m_Visible )
	{
		for ( int i = 0; i < m_MaxVisibleLabels; ++i )
		{
			m_pLabels[i].m_pText->SetVisible( false );
			m_pLabels[i].m_pText->SetColor( m_dwLabelColor );
		}

		if ( pWindow && pWindow->GetState() != visible )
		{
			pWindow->SetState( visible );			
		}
		gUIShell.HideInterface( "ui_item_overlay" );
	}
	m_Visible = visible;
}

void UIItemOverlay::Draw()
{
	GPPROFILERSAMPLE( "UIItemOverlay :: Draw", SP_UI | SP_DRAW );

	m_HoverGoid = GOID_INVALID;
	if ( !m_Visible )
	{
		return;
	}

	// find all items to be drawn in the GoDb
	int itemCount = 0;
	{
	for ( GopSet::iterator i = gGoDb.GetVisibleItems().begin(), end = gGoDb.GetVisibleItems().end(); i != end; ++i )
	{
		m_pLabels[itemCount].m_pText->SetText( L"" );

		// Early out 1 - Position check
		if( !gSiegeEngine.IsNodeInAnyFrustum( (*i)->GetPlacement()->GetPosition().node ) )
		{
			continue;
		}

		// Early out 2 - Label Loot Filter
		if ( gServer.GetScreenPlayer() && !gServer.GetScreenPlayer()->IsAllowedLootLabel( (*i)->GetGoid() ) )
		{
			continue;
		}

		if ( (*i)->IsGold() )
		{
			m_pLabels[itemCount].m_pText->SetText( gpwstringf( gpwtranslate( $MSG$ "%d Gold" ), (*i)->GetAspect()->GetGoldValue() ) );
		}
		else
		{
			bool bSet = false;
			if ( (*i)->HasGui() && (*i)->HasMagic() && (*i)->GetMagic()->HasNonInnateEnchantments() && IsEquippableSlot( (*i)->GetGui()->GetEquipSlot() ) )
			{		
				gpwstring sTranslated = (*i)->GetCommon()->GetTemplateScreenName();
				m_pLabels[itemCount].m_pText->SetText( sTranslated );
					
				if ( !sTranslated.same_no_case( (*i)->GetCommon()->GetScreenName() ) )
				{
					if ( !(*i)->GetGui()->GetIsIdentified() )
					{
						m_pLabels[itemCount].m_pText->SetText( sTranslated );						
						bSet = true;
					}																
				}							
			}

			if ( !bSet )
			{
				m_pLabels[itemCount].m_pText->SetText( (*i)->GetCommon()->GetScreenName() );
			}			
		}

		if ( m_pLabels[itemCount].m_pText->GetText().empty() )
		{
			continue;
		}
				
		bool bSelectedItem = false;
		{for ( GoidColl::iterator j = m_SelectedItems.begin(); j != m_SelectedItems.end(); ++j )
		{
			if ( *j == (*i)->GetGoid() )
			{
				bSelectedItem = true;
				break;
			}
		}}

		if ( m_DraggedGoid == (*i)->GetGoid() )
		{
			m_pLabels[ itemCount ].m_pText->SetColor( m_dwLabelHoverColor );
		}
		else if ( bSelectedItem || (*i)->IsSelected() )
		{
			m_pLabels[ itemCount ].m_pText->SetColor( m_dwLabelSelectedColor );
		}
		else if ( m_pLabels[ itemCount ].m_pText->GetColor() != m_dwLabelHoverColor )
		{
			m_pLabels[ itemCount ].m_pText->SetColor( GetColorFromItem( *i ) );
		}
		else if ( m_pLabels[ itemCount ].m_pText->GetColor() != m_dwLabelHoverColor )
		{
			m_pLabels[ itemCount ].m_pText->SetColor( m_dwLabelColor );
		}	

		{
			SiegePos spos = (*i)->GetPlacement()->GetPosition();

			matrix_3x3 orient;
			vector_3 center, halfDiag;
			(*i)->GetAspect()->GetNodeSpaceOrientedBoundingVolume( orient, spos.pos, halfDiag );
			halfDiag = orient * halfDiag;
			float radius = (spos.pos.y - (*i)->GetPlacement()->GetPosition().pos.y) + halfDiag.y + 0.1f;
						 	
			spos.pos.y += radius;
			{				
				vector_3 pos = spos.ScreenPos();
				if ( !GetValidRect( itemCount, (int)pos.x, (int)pos.y ) )
				{
					continue;
				}
			}
			m_pLabels[ itemCount ].m_pText->SetTag( (long)(*i) );
		}

		POINT mousePt;
		mousePt.x = gUIShell.GetMouseX();
		mousePt.y = gUIShell.GetMouseY();
		if ( m_pLabels[itemCount].m_pText->GetRect().Contains( mousePt ) )
		{
			m_HoverGoid = ((Go *)m_pLabels[itemCount].m_pText->GetTag())->GetGoid();
		}
		
		++itemCount;
		if ( itemCount == m_MaxVisibleLabels )
		{
			break;
		}
	}}

	{for ( int i = itemCount; i < m_MaxVisibleLabels; ++i )
	{
		m_pLabels[i].m_pText->SetVisible( false );
		m_pLabels[i].m_pText->SetColor( m_dwLabelColor );
	}}

	if ( gGoDb.GetVisibleItems().empty() )
	{
		return;
	}	
}



bool UIItemOverlay::GetValidRect( int index, int x, int y, eLabelSort sort )
{
	int height	= ITEM_LABEL_HEIGHT;
	int width	= m_pLabels[index].m_pText->GetWidth() + m_TextPadding+4;
	GRect rect;
	rect.left	= x - width/2;
	rect.right	= rect.left + width;
	rect.top	= y - height/2;
	rect.bottom = rect.top + height;

	int i = 0;	
	while ( i != index ) 
	{		
		if ( rect.Intersects( m_pLabels[i].rect ) )
		{	
			if ( sort == SORT_LEFT )
			{
				rect.right = m_pLabels[i].rect.left-1;
				rect.left = rect.right - width;	
				i = 0;
			}
			else if ( sort == SORT_RIGHT )
			{
				rect.left = m_pLabels[i].rect.right+1;
				rect.right = rect.left + width;
				i = 0;
			}
			else if ( sort == SORT_TOP )
			{
				rect.bottom = m_pLabels[i].rect.top-1;
				rect.top = rect.bottom - height;				
				i = 0;
			}
			else if ( sort == SORT_BOTTOM )
			{
				rect.top = m_pLabels[i].rect.bottom+1;
				rect.bottom = rect.top + height;								
				i = 0;
			}			
			else if ( sort == SORT_ANY )
			{
				if (( rect.top <= m_pLabels[i].rect.top && rect.bottom >= m_pLabels[i].rect.top ) ||			
					( rect.top <= m_pLabels[i].rect.bottom && rect.bottom >= m_pLabels[i].rect.bottom ))
				{

					if ( (rect.top + height/2) <= gUIShell.GetScreenHeight()/2 )
					{
						rect.bottom = m_pLabels[i].rect.top-1;
						rect.top = rect.bottom - height;				
					}
					else
					{
						rect.top = m_pLabels[i].rect.bottom+1;
						rect.bottom = rect.top + height;					
					}
					i = 0;					
				}
				else if (( rect.left <= m_pLabels[i].rect.left && rect.right >= m_pLabels[i].rect.left ) ||
						 ( rect.left <= m_pLabels[i].rect.right && rect.right >= m_pLabels[i].rect.left ))
				{				
					if ( (rect.left + width/2) <= gUIShell.GetScreenWidth()/2 )
					{
						rect.right = m_pLabels[i].rect.left-1;
						rect.left = rect.right - width;				
					}
					else
					{
						rect.left = m_pLabels[i].rect.right+1;
						rect.right = rect.left + width;
					}
					i = 0;			
				}	
			}			
		}
		else
		{
			++i;
		}

		if ( i > m_MaxVisibleLabels )
		{
			m_pLabels[i].m_pText->SetVisible( false );
			break;
		}
	}

	if ( rect.left >= 0 && rect.right <= gUIShell.GetScreenWidth() &&
		 rect.top >= 0 && rect.bottom <= gUIShell.GetScreenHeight() )
	{
		m_pLabels[ index ].m_pText->SetRect( rect.left, rect.right, rect.top, rect.bottom );
		m_pLabels[ index ].m_pText->SetVisible( true );
		m_pLabels[ index ].rect = rect;
		return true;
	}	
	else if ( sort == SORT_ANY )
	{
		if ( rect.left < 0 )
		{
			return GetValidRect( index, x, y, SORT_RIGHT );			
		}
		else if ( rect.right > gUIShell.GetScreenWidth() )
		{
			return GetValidRect( index, x, y, SORT_LEFT );
		}
		else if ( rect.top < 0 )
		{
			return GetValidRect( index, x, y, SORT_BOTTOM );
		}
		else if ( rect.bottom > gUIShell.GetScreenHeight() )
		{
			return GetValidRect( index, x, y, SORT_TOP );
		}
	}

	return false;
}


void UIItemOverlay::SelectItem( Goid goid, bool bSelected )
{
	bool bFound = false;
	for ( GoidColl::iterator i = m_SelectedItems.begin(); i != m_SelectedItems.end(); ++i )
	{
		if ( *i == goid )
		{
			if ( !bSelected )
			{
				m_SelectedItems.erase( i );
			}
			bFound = true;
			break;
		}
	}
	if ( !bFound && bSelected )
	{
		m_SelectedItems.push_back( goid );
	}
}


void UIItemOverlay::PickupSelectedItems()
{
	for ( GoidColl::iterator i = m_SelectedItems.begin(); i != m_SelectedItems.end(); ++i )
	{
		gUIGame.GetUICommands()->ContextActionOnGameObject( *i, true );
	}
	m_SelectedItems.clear();
}


void UIItemOverlay::SelectAllItemsLike( Go *pItemGo )
{
	bool bSelect = IsItemSelected( pItemGo->GetGoid() );

	for ( GopSet::iterator i = gGoDb.GetVisibleItems().begin(); i != gGoDb.GetVisibleItems().end(); ++i )
	{
		if (   (pItemGo->IsGold() && (*i)->IsGold())
			|| (pItemGo->IsWeapon() && (*i)->IsWeapon())
			|| (pItemGo->IsSpell() && (*i)->IsSpell())
			|| (pItemGo->IsArmor() && (*i)->IsArmor())   )
		{
			SelectItem( (*i)->GetGoid(), bSelect );
		}
	}
}


bool UIItemOverlay::IsItemSelected( Goid goid )
{
	for ( GoidColl::iterator i = m_SelectedItems.begin(); i != m_SelectedItems.end(); ++i )
	{
		if ( *i == goid )
		{
			return true;
		}
	}
	return false;
}


Goid UIItemOverlay::GetHoverItem()
{
	return m_HoverGoid;	
}

DWORD UIItemOverlay::GetColorFromItem( Go * pItem )
{
	if ( !pItem || !pItem->HasGui() )
	{
		return 0xffffffff;
	}

	DWORD color = pItem->GetGui()->GetToolTipColor();
	if ( color == 0 )
	{		
		if ( pItem->HasMagic() && (pItem->GetMagic()->HasNonInnateEnchantments() && !pItem->IsSpell() && !pItem->IsPotion() ) )
		{
			color = gUIShell.GetTipColor( "magic" );
		}
		else if ( pItem->IsSpell() )
		{
			if ( pItem->GetMagic()->GetMagicClass() == MC_NATURE_MAGIC )
			{
				color = gUIShell.GetTipColor( "nature_magic" );
			}
			else if ( pItem->GetMagic()->GetMagicClass() == MC_COMBAT_MAGIC )
			{
				color = gUIShell.GetTipColor( "combat_magic" );
			}			
		}
		else
		{
			return 0xffffffff;
		}
	}
	
	return color;
}