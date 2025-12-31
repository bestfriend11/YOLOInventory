//////////////////////////////////////////////////////////////////////////////
//
// File     :  GUIMasterTypes.h
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#pragma once
#ifndef __GUIMASTERTYPES_H
#define __GUIMASTERTYPES_H


struct MouseData
{
	int x1;
	int y1;

	int x2;
	int y2;

	bool bLButtonDown;
	bool bDragging;
};


enum MODE_TYPE 
{
	MODE_CREATE,				// Create window mode
	MODE_STRUCTURE,				// Move/Resize window mode
	MODE_UVEDIT,				// Texture UV Edit mode
	MODE_GUIDEEDIT,				// Reference Guide mode
	MODE_UVSTRUCTURE,			// For the UV structure editing
};

enum STRUCTURE_TYPE
{
	S_NONE,
	S_MOVE,
	S_RESIZE,
	S_DRAGSELECT,
	S_RESIZE_X1,
	S_RESIZE_Y1,
	S_RESIZE_X2,
	S_RESIZE_Y2,
};

enum UV_STATE {
	UV_MOVE,	
	UV_RESIZE_X2,
	UV_RESIZE_Y1,	
	UV_RESIZE_Y2,	
	UV_NONE,
};


enum REF_AXIS {
	X_AXIS,
	Y_AXIS,
};

struct Ref_Line {
	REF_AXIS	rAxis;
	int			Coord;
	int			ID;
};


enum eDirection
{
	LEFT,
	RIGHT,
	UP,
	DOWN,
};


#define INITIAL_WIDTH 800
#define INITIAL_HEIGHT 600

#define DEFAULT_CURSOR_COLOR 0xFF0055FF
#define ACTION_CURSOR_COLOR	 0xFFFF0000


typedef std::vector< Ref_Line > RefLineVec;


#endif