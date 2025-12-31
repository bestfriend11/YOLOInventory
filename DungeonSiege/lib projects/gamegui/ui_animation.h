/**************************************************************************

Filename		: ui_animation.h

Description		: This class controls the updating and maintenance of all
				  UI animations

Creation Date	: 11/3/99

**************************************************************************/


#pragma once
#ifndef _UI_ANIMATION_H_
#define _UI_ANIMATION_H_


// Include Files
#include <vector>
#include "ui_types.h"
#include "ui_interface.h"


// Forward Declarations
class UIWindow;


// Structures
struct UI_RECT_ANIMATION
{
	double			duration;
	UIRect			source_rect;
	UIRect			dest_rect;
	UIWindow		*pWindow;
	int				left_delta;
	int				right_delta;
	int				top_delta;
	int				bottom_delta;
};

struct UI_ALPHA_ANIMATION
{
	double			duration;
	double			total_duration;
	double			start_alpha;
	double			end_alpha;
	UIWindow		*pWindow;
	bool			bLoop;
};

struct UI_TEXTURE_ANIMATION
{
	double			framepersec;
	double			elapsedTime;
	unsigned int	numFrames;
	UIWindow		*pWindow;
};

struct UI_FLASH_ANIMATION
{
	double			duration;
	double			total_duration;
	UIWindow		*pWindow;
	bool			bLoop;
	float			maxAlpha;
	int				numLoops;
};

struct UI_CLOCK_ANIMATION
{
	double			duration;
	double			total_duration;
	UIWindow *		pWindow;
	DWORD			color;
};



class UIAnimation : public Singleton <UIAnimation>
{
public:

	// Constructor
	UIAnimation();

	// Destructor
	~UIAnimation();

	// Update all animations
	void Update( double seconds );

	// Draw any necessary animations
	void Draw( void );
	void DrawFilledTriangle( POINT pt1, POINT pt2, POINT pt3, DWORD color );
	void GetPartialPoint( POINT & pt, int middleX, int middleY, int height, int width, float percent );

	void SetDisableUpdate( bool bSet )	{ m_bDisableUpdate = bSet; }
	bool GetDisableUpdate()				{ return m_bDisableUpdate; }

	// Add a rectangle animation
	void AddRectAnimation( double duration, UIRect dest_rect, UIWindow *pWindow );
	void UpdateRectAnimation( double seconds );

	// Add an alpha animation
	void AddAlphaAnimation( double duration, double start_alpha, double end_alpha, UIWindow *pWindow, bool bLoop = false );
	void UpdateAlphaAnimation( double seconds );

	// Add a texture animation
	void AddTextureAnimation( double framepersec, unsigned int numFrames, UIWindow *pWindow );
	void UpdateTextureAnimation( double seconds );

	// Add a flashing animation
	void AddFlashAnimation( double duration, UIWindow * pWindow, bool bLoop = false, float maxAlpha = 1.0f, int numLoops = -1 );
	void UpdateFlashAnimation( double seconds );

	// Add a clock animation
	void AddClockAnimation( double duration, UIWindow * pWindow, DWORD color );
	void UpdateClockAnimation( double seconds );

	// Initialize Window List
	void SetUIWindows( std::vector< GUInterface * > *ui_interfaces ) { m_UIInterfaces = ui_interfaces; };

	// Remove any animations that an interface has.  Should be used when an interface is destructed
	void RemoveInterfaceAnimations( GUInterface *pInterface );

	void RemoveAnimationsForWindow( UIWindow * pWindow );

	bool IsAnimating( UIWindow * pWindow );

private:

	std::vector< UI_RECT_ANIMATION >	m_rect_animations;
	std::vector< UI_ALPHA_ANIMATION >	m_alpha_animations;
	std::vector< UI_TEXTURE_ANIMATION > m_texture_animations;
	std::vector< UI_FLASH_ANIMATION >	m_flash_animations;
	std::vector< UI_CLOCK_ANIMATION >	m_clock_animations;
	GUInterfaceVec						*m_UIInterfaces;		// This list contains all the windows/interfaces of the UI
	unsigned long						m_last_update;
	bool								m_bDisableUpdate;

};

#define gUIAnimation UIAnimation::GetSingleton()

// Tell if integers are within a specified range
bool InRange( int source, int dest, int margin );


#endif