/**************************************************************************

Filename		: ui_animation.cpp

Description		: This class controls the updating and maintenance of all
				  UI animations

Creation Date	: 11/3/99

**************************************************************************/


// Include Files
#include "precomp_gamegui.h"
#include "ui_window.h"
#include "ui_animation.h"
#include "ui_cursor.h"
#include "ui_types.h"
#include "ui_shell.h"


UIAnimation::UIAnimation()
	: m_last_update( 0 )
	, m_bDisableUpdate( false )
{
}

UIAnimation::~UIAnimation()
{
}


void UIAnimation::AddRectAnimation( double duration, UIRect dest_rect, UIWindow *pWindow )
{
	UI_RECT_ANIMATION ua;
	ua.dest_rect	= dest_rect;
	ua.duration		= duration;
	ua.pWindow		= pWindow;
	ua.source_rect	= pWindow->GetRect();	

	m_rect_animations.push_back( ua );
}


void UIAnimation::AddAlphaAnimation( double duration, double start_alpha, double end_alpha, UIWindow *pWindow, bool bLoop )
{
	UI_ALPHA_ANIMATION ua;
	ua.duration		= duration;
	ua.start_alpha	= start_alpha;
	ua.end_alpha	= end_alpha;
	ua.pWindow		= pWindow;
	ua.bLoop		= bLoop;
	ua.total_duration = duration;
	pWindow->SetAlpha( (float)start_alpha );

	m_alpha_animations.push_back( ua );
}


void UIAnimation::AddTextureAnimation( double framepersec, unsigned int numFrames, UIWindow *pWindow )
{
	UI_TEXTURE_ANIMATION ua;
	ua.framepersec	= framepersec;
	ua.numFrames	= numFrames;
	ua.pWindow		= pWindow;
	ua.elapsedTime	= 0.0f;

	m_texture_animations.push_back( ua );
}


void UIAnimation::AddFlashAnimation( double duration, UIWindow *pWindow, bool bLoop, float maxAlpha, int numLoops ) 
{
	std::vector< UI_FLASH_ANIMATION >::iterator i;
	for ( i = m_flash_animations.begin(); i != m_flash_animations.end(); i++ ) 
	{
		if ( (*i).pWindow == pWindow )
		{
			return;
		}
	}

	UI_FLASH_ANIMATION ua;
	ua.duration		= duration;
	ua.pWindow		= pWindow;
	ua.bLoop		= bLoop;
	ua.maxAlpha		= maxAlpha;
	ua.total_duration = duration;
	ua.numLoops		= numLoops;

	if ( ua.numLoops != -1 )
	{
		ua.numLoops *= 2;
	}
	
	m_flash_animations.push_back( ua );
}


void UIAnimation::AddClockAnimation( double duration, UIWindow * pWindow, DWORD color )
{
	std::vector< UI_CLOCK_ANIMATION >::iterator i;
	for ( i = m_clock_animations.begin(); i != m_clock_animations.end(); i++ ) 
	{
		if ( (*i).pWindow == pWindow )
		{
			return;
		}
	}

	UI_CLOCK_ANIMATION ua;
	ua.total_duration	= duration;
	ua.duration			= duration;
	ua.pWindow			= pWindow;
	ua.color			= color;

	m_clock_animations.push_back( ua );
}


// Master update
void UIAnimation::Update( double seconds )
{
	if ( !m_bDisableUpdate )
	{		
		UpdateRectAnimation		( seconds );
		UpdateAlphaAnimation	( seconds );
		UpdateTextureAnimation	( seconds );
		UpdateFlashAnimation	( seconds );
		UpdateClockAnimation	( seconds );
	}
}

void UIAnimation::Draw( void )
{
	std::vector< UI_CLOCK_ANIMATION >::iterator i = m_clock_animations.begin();
	while ( i != m_clock_animations.end() ) 
	{		
		if ( !gUIShell.IsInterfaceVisible( (*i).pWindow->GetInterfaceParent() ) || !(*i).pWindow->GetVisible() ) 
		{			
			++i;
			continue;
		}
		
		// $$$ todo: add drawing code here.		
		// Retrieve the percentage of time left.
		float durationPercent	= (float)(*i).duration / (float)(*i).total_duration;

		// There are a total of eight segments so we divide by 1/8 or 0.125.
		float numSegments		= durationPercent / 0.125f;
		float numFullSegments	= floorf( numSegments );
		float partialSegment	= numSegments - numFullSegments;

		UIRect rect = (*i).pWindow->GetRect();
		int middleX = rect.left	+ (rect.right-rect.left)/2;
		int middleY = rect.top	+ (rect.bottom-rect.top)/2;
		int height	= rect.bottom - rect.top;
		int width	= rect.right - rect.left;
				
		GPoint pt1( middleX, middleY		);
		GPoint pt2( middleX, rect.top		);
		GPoint pt3( rect.left, rect.top		);		
		GPoint pt4( rect.left, middleY		);
		GPoint pt5( rect.left, rect.bottom	);
		GPoint pt6( middleX, rect.bottom	);
		GPoint pt7( rect.right, rect.bottom	);
		GPoint pt8( rect.right, middleY		);
		GPoint pt9( rect.right, rect.top	);

		if ( numFullSegments >= 1 ) 
		{ 
			DrawFilledTriangle( pt3, pt1, pt2, (*i).color ); 
		}
		else if ( partialSegment != 0.0f )
		{
			GPoint partial = pt3;
			GetPartialPoint( partial, middleX, middleY, height, width, partialSegment );
			DrawFilledTriangle( partial, pt1, pt2, (*i).color ); 
		}

		if ( numFullSegments >= 2 ) 
		{ 	
			DrawFilledTriangle( pt4, pt1, pt3, (*i).color ); 
		}
		else if ( numFullSegments == 1 && partialSegment != 0.0f )
		{
			GPoint partial = pt4;
			GetPartialPoint( partial, middleX, middleY, height, width, partialSegment );
			DrawFilledTriangle( partial, pt1, pt3, (*i).color ); 
		}

		if ( numFullSegments >= 3 ) 
		{ 
			DrawFilledTriangle( pt4, pt5, pt1, (*i).color ); 
		}		
		else if ( numFullSegments == 2 && partialSegment != 0.0f )
		{
			GPoint partial = pt5;
			GetPartialPoint( partial, middleX, middleY, height, width, partialSegment );
			DrawFilledTriangle( pt4, partial, pt1, (*i).color ); 
		}
		
		if ( numFullSegments >= 4 ) 
		{ 
			DrawFilledTriangle( pt5, pt6, pt1, (*i).color ); 
		}
		else if ( numFullSegments == 3 && partialSegment != 0.0f )
		{
			GPoint partial = pt6;
			GetPartialPoint( partial, middleX, middleY, height, width, partialSegment );
			DrawFilledTriangle( pt5, partial, pt1, (*i).color ); 
		}

		if ( numFullSegments >= 5 ) 
		{ 
			DrawFilledTriangle( pt6, pt7, pt1, (*i).color ); 
		}
		else if ( numFullSegments == 4 && partialSegment != 0.0f )
		{
			GPoint partial = pt7;
			GetPartialPoint( partial, middleX, middleY, height, width, partialSegment );
			DrawFilledTriangle( pt6, partial, pt1, (*i).color ); 
		}

		if ( numFullSegments >= 6 ) 
		{ 
			DrawFilledTriangle( pt1, pt7, pt8, (*i).color ); 
		}
		else if ( numFullSegments == 5 && partialSegment != 0.0f )
		{
			GPoint partial = pt8;
			GetPartialPoint( partial, middleX, middleY, height, width, partialSegment );
			DrawFilledTriangle( pt1, pt7, partial, (*i).color ); 
		}

		if ( numFullSegments >= 7 ) 
		{ 
			DrawFilledTriangle( pt1, pt8, pt9, (*i).color ); 
		}
		else if ( numFullSegments == 6 && partialSegment != 0.0f )
		{
			GPoint partial = pt9;
			GetPartialPoint( partial, middleX, middleY, height, width, partialSegment );
			DrawFilledTriangle( pt1, pt8, partial, (*i).color ); 
		}

		if ( numFullSegments >= 8 ) 
		{ 
			DrawFilledTriangle( pt2, pt1, pt9, (*i).color ); 
		}
		else if ( numFullSegments == 7 && partialSegment != 0.0f )
		{
			GPoint partial = pt2;
			GetPartialPoint( partial, middleX, middleY, height, width, partialSegment );
			DrawFilledTriangle( partial, pt1, pt9, (*i).color ); 
		}

		++i;
	}
}

void UIAnimation::DrawFilledTriangle( POINT pt1, POINT pt2, POINT pt3, DWORD color )
{
	gDefaultRapi.GetDevice()->SetRenderState( D3DRS_ZFUNC,	D3DCMP_ALWAYS );

	gUIShell.GetRenderer().SetTextureStageState(	0,
													D3DTOP_DISABLE,
													D3DTOP_MODULATE,
													D3DTADDRESS_CLAMP,
													D3DTADDRESS_CLAMP,
													D3DTEXF_POINT,
													D3DTEXF_POINT,
													D3DTEXF_POINT,
													D3DBLEND_SRCALPHA,
													D3DBLEND_INVSRCALPHA,
													true );

	tVertex Verts[4];
	memset( Verts, 0, sizeof( tVertex ) * 4 );	
	
	Verts[0].x			= pt1.x-0.5f;
	Verts[0].y			= pt1.y-0.5f;
	Verts[0].z			= 0.0f;
	Verts[0].rhw		= 1.0f;
	Verts[0].uv.u		= 0.0f;
	Verts[0].uv.v		= 0.0f;
	Verts[0].color		= color;

	Verts[1].x			= pt2.x-0.5f;
	Verts[1].y			= pt2.y-0.5f;
	Verts[1].z			= 1.0f;
	Verts[1].rhw		= 1.0f;
	Verts[1].uv.u		= 0.0f;
	Verts[1].uv.v		= 1.0f;
	Verts[1].color		= color;

	Verts[2].x			= pt3.x-0.5f;
	Verts[2].y			= pt3.y-0.5f;
	Verts[2].z			= 0.0f;
	Verts[2].rhw		= 1.0f;
	Verts[2].uv.u		= 1.0f;
	Verts[2].uv.v		= 0.0f;
	Verts[2].color		= color;	

	gUIShell.GetRenderer().DrawPrimitive( D3DPT_TRIANGLESTRIP, Verts, 4, TVERTEX, 0, 1 );

	gDefaultRapi.GetDevice()->SetRenderState( D3DRS_ZFUNC,	D3DCMP_LESSEQUAL );
}

void UIAnimation::GetPartialPoint( POINT & pt, int middleX, int middleY, int height, int width, float percent )
{
	percent = 1.0f - percent;

	if ( pt.x < middleX  )
	{
		if ( pt.y < middleY )
		{
			pt.x += (int)((width/2) * percent);
		}
		else if ( pt.y == middleY )
		{
			pt.y -= (int)((height/2) * percent);
		}
		else if ( pt.y > middleY )
		{
			pt.y -= (int)((height/2) * percent);
		}		
	}	
	else if ( pt.x == middleX )
	{
		if ( pt.y > middleY )
		{
			pt.x -= (int)((width/2) * percent);
		}
		else if ( pt.y < middleY )
		{
			pt.x += (int)((width/2) * percent);
		}
	}
	else if ( pt.x > middleX )
	{
		if ( pt.y > middleY )
		{
			pt.x -= (int)((width/2) * percent);
		}
		else if ( pt.y == middleY )
		{
			pt.y += (int)((height/2) * percent);
		}
		else if ( pt.y < middleY )
		{
			pt.y += (int)((height/2) * percent);
		}
	}	
}


void UIAnimation::UpdateRectAnimation( double seconds )
{
	std::vector< UI_RECT_ANIMATION >::iterator i = m_rect_animations.begin();
	while ( i != m_rect_animations.end() ) {			
		if ( gUIShell.IsInterfaceVisible( (*i).pWindow->GetInterfaceParent() ) == false ) {
			i = m_rect_animations.erase( i );
			continue;
		}

		
		(*i).duration -= seconds;
		
		double ani_factor = (*i).duration/seconds;
		int left	= (int)((*i).pWindow->GetRect().left+(((*i).dest_rect.left - (*i).pWindow->GetRect().left)/ani_factor));
		int right	= left + (*i).pWindow->GetRect().right - (*i).pWindow->GetRect().left;		
		int top		= (int)((*i).pWindow->GetRect().top+(((*i).dest_rect.top - (*i).pWindow->GetRect().top)/ani_factor));		
		int bottom	= top + (*i).pWindow->GetRect().bottom - (*i).pWindow->GetRect().top;
		
		// If any windows are grouped with an animated window, move them also
		if ( !(*i).pWindow->GetGroup().empty() )
		{			
			GUInterfaceVec::iterator j;		
			UIWindowMap::iterator k;
			for ( j = m_UIInterfaces->begin(); j != m_UIInterfaces->end(); ++j )
			{				
				for ( k = (*j)->ui_windows.begin(); k != (*j)->ui_windows.end(); ++k )
				{
					if ( !(*k).second->GetName().same_no_case( (*i).pWindow->GetName() ) )
					{
						if ( (*k).second->GetGroup().same_no_case( (*i).pWindow->GetGroup() ) )
						{
							int ani_move_x = left - (*i).pWindow->GetRect().left;
							int ani_move_y = top - (*i).pWindow->GetRect().top;
							(*k).second->SetRect(	(*k).second->GetRect().left+ani_move_x, 
													(*k).second->GetRect().right+ani_move_x,
													(*k).second->GetRect().top+ani_move_y, 
													(*k).second->GetRect().bottom+ani_move_y,
													true );
						}
					}
				}
			}		
		}
		
		(*i).pWindow->SetRect( left, right, top, bottom, true );	

		// If the animation has run out of time, automatically set it to its destination rectangle
		if ( (*i).duration <= 0.0 )
		{
			i = m_rect_animations.erase( i );

			// If any windows are grouped with an animated window, move them also
			if ( !(*i).pWindow->GetGroup().empty() )
			{			
				GUInterfaceVec::iterator j;		
				UIWindowMap::iterator k;
				for ( j = m_UIInterfaces->begin(); j != m_UIInterfaces->end(); ++j )
				{				
					for ( k = (*j)->ui_windows.begin(); k != (*j)->ui_windows.end(); ++k )
					{
						if ( !(*k).second->GetName().same_no_case( (*i).pWindow->GetName() ))
						{
							if ( (*k).second->GetGroup().same_no_case( (*i).pWindow->GetGroup() ) )
							{
								int ani_move_x = (*i).dest_rect.left - (*i).pWindow->GetRect().left;
								int ani_move_y = (*i).dest_rect.top - (*i).pWindow->GetRect().top;
								(*k).second->SetRect(	(*k).second->GetRect().left+ani_move_x, 
														(*k).second->GetRect().right+ani_move_x,
														(*k).second->GetRect().top+ani_move_y, 
														(*k).second->GetRect().bottom+ani_move_y, 
														true );
							}
						}
					}
				}		
			}


			(*i).pWindow->SetRect(  (*i).dest_rect.left, (*i).dest_rect.right, 
									(*i).dest_rect.top, (*i).dest_rect.bottom, true );

			
		}
		else {
			++i;
		}
	}
}


void UIAnimation::UpdateAlphaAnimation( double seconds )
{
	std::vector< UI_ALPHA_ANIMATION >::iterator i = m_alpha_animations.begin();
	while ( i != m_alpha_animations.end() ) {			
		if ( gUIShell.IsInterfaceVisible( (*i).pWindow->GetInterfaceParent() ) == false ) {
			i = m_alpha_animations.erase( i );
			continue;
		}
		
		(*i).duration -= seconds;
		double ani_factor = seconds/(*i).duration;
		double ani_alpha = (*i).pWindow->GetAlpha() + ((*i).end_alpha - (*i).pWindow->GetAlpha())*ani_factor;		
		
		(*i).pWindow->SetAlpha( (float)ani_alpha );

		// If any windows are grouped with an animated window, move them also
		if ( !(*i).pWindow->GetGroup().empty() )
		{			
			UIWindowVec group;
			UIWindowVec::iterator j;
			group = gUIShell.ListWindowsOfGroup( (*i).pWindow->GetGroup() );
			for ( j = group.begin(); j != group.end(); ++j )
			{					
				if ( !(*j)->GetName().same_no_case( (*i).pWindow->GetName() ))
				{
					if ( (*j)->GetGroup().same_no_case( (*i).pWindow->GetGroup() ) )
					{					
						if ( (*i).duration <= 0.0 )
						{
							(*j)->SetAlpha( (float)(*i).end_alpha );
						}
						
						else 					
						{
							(*j)->SetAlpha( (float)ani_alpha );							

							double delta = (*i).end_alpha - (*i).start_alpha;
							if ( delta < 0.0 ) 
							{
								if ( (*i).pWindow->GetAlpha() < (*i).end_alpha ) 
								{
									(*j)->SetAlpha( (float)((*i).end_alpha) );
								}
							}
							else 
							{
								if ( (*i).pWindow->GetAlpha() > (*i).end_alpha ) 
								{
									(*j)->SetAlpha( (float)((*i).end_alpha) );
								}
							}													
						}
					}
				}				
			}		
		}	
		

		double delta = (*i).end_alpha - (*i).start_alpha;
		if ( delta < 0.0 ) 
		{
			if ( (*i).pWindow->GetAlpha() < (*i).end_alpha ) 
			{
				(*i).pWindow->SetAlpha( (float)((*i).end_alpha) );
			}
		}
		else 
		{
			if ( (*i).pWindow->GetAlpha() > (*i).end_alpha ) 
			{
				(*i).pWindow->SetAlpha( (float)((*i).end_alpha) );
			}
		}
		
		if ( (*i).duration <= 0.0 ) 
		{
			(*i).pWindow->SetAlpha( (float)((*i).end_alpha) );
			UIWindow *pWindow = (*i).pWindow;
			if ( (*i).bLoop ) 
			{
				(*i).duration = (*i).total_duration;
				double fTemp = 0;
				fTemp = (*i).end_alpha;
				(*i).end_alpha = (*i).start_alpha;
				(*i).start_alpha = fTemp;
			}
			else 
			{
				i = m_alpha_animations.erase( i );	
				if ( pWindow->GetVisible() )
				{
					pWindow->ReceiveMessage( UIMessage( MSG_ENDANIMATION ) );
				}
			}
		}
		else 
		{
			++i;
		}
	}
}


void UIAnimation::UpdateFlashAnimation( double seconds )
{
	std::vector< UI_FLASH_ANIMATION >::iterator i = m_flash_animations.begin();
	while ( i != m_flash_animations.end() ) 
	{		
		if ( gUIShell.IsInterfaceVisible( (*i).pWindow->GetInterfaceParent() ) == false ) 
		{			
			++i;
			continue;
		}
		
		(*i).duration -= seconds;						

		bool bDeleted = false;
		if ( (*i).duration <= 0.0f ) 
		{
			if ( (*i).pWindow->GetAlpha() == 0.0f ) 
			{
				(*i).pWindow->SetAlpha( (*i).maxAlpha );
			}
			else 
			{
				(*i).pWindow->SetAlpha( 0.0f );
			}

			UIWindow *pWindow = (*i).pWindow;
			if ( (*i).numLoops != -1 )
			{
				(*i).numLoops--;
			}

			if ( (*i).bLoop && pWindow->GetVisible() && (*i).numLoops != 0 ) 
			{
				(*i).duration = (*i).total_duration;					
			}
			else 
			{				
				bDeleted = true;
				(*i).pWindow->SetAlpha( (*i).maxAlpha );
				i = m_flash_animations.erase( i );				
				pWindow->ReceiveMessage( UIMessage( MSG_ENDANIMATION ) );
			}
		}

		if ( !bDeleted )
		{
			++i;		
		}
	}
}


void UIAnimation::UpdateTextureAnimation( double seconds )
{
	std::vector< UI_TEXTURE_ANIMATION >::iterator i = m_texture_animations.begin();
	while ( i != m_texture_animations.end() ) 
	{			
		if ( gUIShell.IsInterfaceVisible( (*i).pWindow->GetInterfaceParent() ) == false ) 		
		{
			i = m_texture_animations.erase( i );
			continue;
		}

		(*i).elapsedTime += seconds;
		if ( (*i).elapsedTime >= (1/(*i).framepersec) ) 
		{
			(*i).elapsedTime = 0.0f;
			
			std::vector< unsigned int >::iterator j;
			std::vector< unsigned int > textures;

			if ( (*i).pWindow->GetType() == UI_TYPE_CURSOR ) 
			{
				UICursor *pCursor = (UICursor *)(*i).pWindow;
				pCursor->GetAnimationTextures( textures );
			}
			else 
			{
				(*i).pWindow->GetAnimationTextures( textures );
			}

			for ( j = textures.begin(); j != textures.end(); ++j ) 
			{
				if ( (*j) == (*i).pWindow->GetTextureIndex() ) 
				{
					++j;
					
					if ( j == textures.end() ) {
						j = textures.begin();						
					}
					(*i).pWindow->SetTextureIndex( *j );
					continue;
				}				
			}
		}	
		++i;
	}
}


void UIAnimation::UpdateClockAnimation( double seconds )
{
	std::vector< UI_CLOCK_ANIMATION >::iterator i = m_clock_animations.begin();
	while ( i != m_clock_animations.end() ) 
	{		
		if ( gUIShell.IsInterfaceVisible( (*i).pWindow->GetInterfaceParent() ) == false ) 
		{			
			++i;
			continue;
		}
		
		(*i).duration -= seconds;
		
		if ( (*i).duration <= 0.0f )
		{
			(*i).pWindow->ReceiveMessage( UIMessage( MSG_ENDANIMATION ) );
			i = m_clock_animations.erase( i );							
		}
		else
		{
			++i;
		}
	}
}



/**************************************************************************

Function		: RemoveInterfaceAnimations()

Purpose			: Remove all animations for a specified interface

Returns			: bool

**************************************************************************/
void UIAnimation::RemoveInterfaceAnimations( GUInterface *pInterface )
{
	std::vector< UI_RECT_ANIMATION >::iterator i = m_rect_animations.begin(); 
	while ( i != m_rect_animations.end() ) 
	{
		if ( (*i).pWindow->GetInterfaceParent().same_no_case( pInterface->name ) )
		{
			i = m_rect_animations.erase( i );
		}
		else 
		{
			++i;
		}
	}

	std::vector< UI_ALPHA_ANIMATION >::iterator j = m_alpha_animations.begin();
	while ( j != m_alpha_animations.end() ) 
	{
		if ( (*j).pWindow->GetInterfaceParent().same_no_case( pInterface->name ) )
		{
			j = m_alpha_animations.erase( j );
		}
		else 
		{
			++j;
		}
	}

	std::vector< UI_FLASH_ANIMATION >::iterator k = m_flash_animations.begin();
	while ( k != m_flash_animations.end() ) 
	{
		if ( (*k).pWindow->GetInterfaceParent().same_no_case( pInterface->name ) )
		{
			k = m_flash_animations.erase( k );
		}
		else 
		{
			++k;
		}
	}

	std::vector< UI_CLOCK_ANIMATION >::iterator l = m_clock_animations.begin();
	while ( l != m_clock_animations.end() ) 
	{
		if ( (*l).pWindow->GetInterfaceParent().same_no_case( pInterface->name ) )
		{
			l = m_clock_animations.erase( l );
		}
		else 
		{
			++l;
		}
	}
}


void UIAnimation::RemoveAnimationsForWindow( UIWindow * pWindow )
{
	std::vector< UI_RECT_ANIMATION >::iterator i = m_rect_animations.begin(); 
	while ( i != m_rect_animations.end() ) 
	{
		if ( (*i).pWindow->GetName().same_no_case( pWindow->GetName() ) &&
			 (*i).pWindow->GetInterfaceParent().same_no_case( pWindow->GetInterfaceParent() ) )
		{
			i = m_rect_animations.erase( i );
		}
		else 
		{
			++i;
		}
	}

	std::vector< UI_ALPHA_ANIMATION >::iterator j = m_alpha_animations.begin();
	while ( j != m_alpha_animations.end() ) 
	{
		if ( (*j).pWindow->GetName().same_no_case( pWindow->GetName() ) &&
			 (*j).pWindow->GetInterfaceParent().same_no_case( pWindow->GetInterfaceParent() ) )
		{
			j = m_alpha_animations.erase( j );
		}
		else 
		{
			++j;
		}
	}

	std::vector< UI_FLASH_ANIMATION >::iterator k = m_flash_animations.begin();
	while ( k != m_flash_animations.end() ) 
	{
		if ( (*k).pWindow->GetName().same_no_case( pWindow->GetName() ) &&
			 (*k).pWindow->GetInterfaceParent().same_no_case( pWindow->GetInterfaceParent() ) )
		{
			pWindow->SetAlpha( (*k).maxAlpha );
			k = m_flash_animations.erase( k );
		}
		else 
		{
			++k;
		}
	}

	std::vector< UI_CLOCK_ANIMATION >::iterator l = m_clock_animations.begin();
	while ( l != m_clock_animations.end() ) 
	{
		if ( (*l).pWindow->GetName().same_no_case( pWindow->GetName() ) &&
			 (*l).pWindow->GetInterfaceParent().same_no_case( pWindow->GetInterfaceParent() ) )
		{			
			l = m_clock_animations.erase( l );
		}
		else 
		{
			++l;
		}
	}
}


bool UIAnimation::IsAnimating( UIWindow * pWindow )
{
	std::vector< UI_RECT_ANIMATION >::iterator i = m_rect_animations.begin(); 
	while ( i != m_rect_animations.end() ) 
	{
		if ( (*i).pWindow->GetName().same_no_case( pWindow->GetName() ) &&
			 (*i).pWindow->GetInterfaceParent().same_no_case( pWindow->GetInterfaceParent() ) )
		{
			return true;
		}
		else 
		{
			++i;
		}
	}

	std::vector< UI_ALPHA_ANIMATION >::iterator j = m_alpha_animations.begin();
	while ( j != m_alpha_animations.end() ) 
	{
		if ( (*j).pWindow->GetName().same_no_case( pWindow->GetName() ) &&
			 (*j).pWindow->GetInterfaceParent().same_no_case( pWindow->GetInterfaceParent() ) )
		{
			return true;
		}
		else 
		{
			++j;
		}
	}

	std::vector< UI_FLASH_ANIMATION >::iterator k = m_flash_animations.begin();
	while ( k != m_flash_animations.end() ) 
	{
		if ( (*k).pWindow->GetName().same_no_case( pWindow->GetName() ) &&
			 (*k).pWindow->GetInterfaceParent().same_no_case( pWindow->GetInterfaceParent() ) )
		{
			return true;
		}
		else 
		{
			++k;
		}
	}

	std::vector< UI_CLOCK_ANIMATION >::iterator l = m_clock_animations.begin();
	while ( l != m_clock_animations.end() ) 
	{
		if ( (*l).pWindow->GetName().same_no_case( pWindow->GetName() ) &&
			 (*l).pWindow->GetInterfaceParent().same_no_case( pWindow->GetInterfaceParent() ) )
		{
			return true;
		}
		else 
		{
			++l;
		}
	}

	return false;
}


/**************************************************************************

Function		: InRange()

Purpose			: Sees if a number is within a specified range of another
				  number

Returns			: bool

**************************************************************************/
bool InRange( int source, int dest, int margin )
{
	if (( dest <= source+margin ) && ( dest >= source-margin )) {
		return true;
	}
	return false;
}