//////////////////////////////////////////////////////////////////////////////
//
// File     :  ui_line.h
// Author(s):  Jessica Tams
//
// Summary  :  ui class to draw a line from a list of numbers
//
// Copyright © 2002 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////
#pragma once
#ifndef _UI_LINE_H_
#define _UI_LINE_H_

#include "ui_window.h"


class UILine : public UIWindow
{
	FUBI_CLASS_INHERIT( UILine, UIWindow );

public:
	// Public Methods

	// Constructor
	UILine( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent );
	UILine();

	virtual ~UILine();

	virtual void Draw();

	void	ClearAllLineValues(  )						{ m_lineValues.clear(); }
	void	AddLineValue( double x, double y )			{ m_lineValues.push_back( StatEntry( x, y) ); }
	void	SetColor( DWORD color )						{ m_color = color; }
	void	GetMaxLineValue( double& xMax, double& yMax );
	void	GetMaxGroupValue( double& xMax, double& yMax );	

private:
	struct StatEntry
	{
		StatEntry()									{  x = 0.0; y = 0.0;  }
		StatEntry( double x_, double y_ )			{  x = x_; y = y_;  }
		double	x;
		double	y;
	};
	typedef std::list< StatEntry > LineValues;
	LineValues m_lineValues;

	DWORD	m_color;
	float   m_lineThickness;
};

#endif