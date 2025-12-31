//////////////////////////////////////////////////////////////////////////////
//
// File     :  WinGeometry.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains wrappers for RECT, SIZE, POINT pirated from MFC.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __WINGEOMETRY_H
#define __WINGEOMETRY_H

//////////////////////////////////////////////////////////////////////////////

#include "GpCore.h"

//////////////////////////////////////////////////////////////////////////////
// doco and declarations

// Summary:		Contains GSize/GPoint/GRect classes. These are based on the
//				code in afxwin.h and afxwin1.inline. See MFC docs for how to
//				use these. Note that there are some extensions to the basic
//				MFC classes here. Some of the less useful members have been
//				eliminated, some have been renamed when the class name was
//				included in the function name, e.g., Set() instead of
//              SetRect().

struct GSize;
struct GPoint;
struct GRect;

// if not using AFX or WTL, typedef them for convenience and compatibility
#if (!_AFX || defined( _WTL_NO_WTYPES ))
typedef GSize  CSize;
typedef GPoint CPoint;
typedef GRect  CRect;
#endif

//////////////////////////////////////////////////////////////////////////////
// struct GSize declaration

struct GSize : tagSIZE
{
	static const SIZE ZERO;		// zero object

	GSize( void )											{  /* random filled */  }
	GSize( int _cx, int _cy )								{  cx = _cx;  cy = _cy;  }
	GSize( const SIZE& sz )									{  *(SIZE*)this = sz;  }
	GSize( const POINT& pt )								{  *(POINT*)this = pt;  }

	operator GSize * ( void )								{  return ( this );  }
	operator const GSize * ( void ) const					{  return ( this );  }

	bool IsZero( void ) const								{  return ( (cx | cy) == 0 );  }

	GSize& operator += ( const SIZE &sz )					{  cx += sz.cx;  cy += sz.cy;  return ( *this );  }
	GSize& operator -= ( const SIZE &sz )					{  cx -= sz.cx;  cy -= sz.cy;  return ( *this );  }
	GSize& operator *= ( int mult )							{  cx *= mult;  cy *= mult;  return ( *this );  }
	GSize& operator /= ( int div )							{  cx /= div;  cy /= div;  return ( *this );  }
};

DECL_SELECT_ANY const SIZE GSize::ZERO = { 0, 0 };

inline GSize operator - ( const SIZE& sz )					{  return ( GSize( -sz.cx, -sz.cy ) );  }
inline GSize operator * ( const SIZE& sz, int mult )		{  return ( GSize( sz.cx * mult, sz.cy * mult ) );  }
inline GSize operator * ( int mult, const SIZE& sz )		{  return ( GSize( sz.cx * mult, sz.cy * mult ) );  }
inline GSize operator / ( const SIZE& sz, int div )			{  return ( GSize( sz.cx / div, sz.cy / div ) );  }
inline GSize operator / ( int div, const SIZE& sz )			{  return ( GSize( sz.cx / div, sz.cy / div ) );  }
inline GSize operator + ( const SIZE& l, const SIZE& r )	{  return ( GSize( l.cx + r.cx, l.cy + r.cy ) );  }
inline GSize operator - ( const SIZE& l, const SIZE& r )	{  return ( GSize( l.cx - r.cx, l.cy - r.cy ) );  }

inline bool operator == ( const SIZE& l, const SIZE& r )	{  return ( (l.cx == r.cx) && (l.cy == r.cy) );  }
inline bool operator != ( const SIZE& l, const SIZE& r )	{  return ( (l.cx != r.cx) || (l.cy != r.cy) );  }

// note: these are all "if and only if" conditions (for both members).
//		 so if !(size1 <= size2) that does NOT mean (size1 > size2).
inline bool operator <  ( const SIZE& l, const SIZE& r )	{  return ( (l.cx <  r.cx) && (l.cy <  r.cy) );  }
inline bool operator <= ( const SIZE& l, const SIZE& r )	{  return ( (l.cx <= r.cx) && (l.cy <= r.cy) );  }
inline bool operator >  ( const SIZE& l, const SIZE& r )	{  return ( (l.cx >  r.cx) && (l.cy >  r.cy) );  }
inline bool operator >= ( const SIZE& l, const SIZE& r )	{  return ( (l.cx >= r.cx) && (l.cy >= r.cy) );  }

//////////////////////////////////////////////////////////////////////////////
// struct GPoint declaration

struct GPoint : tagPOINT
{
	static const POINT ZERO;	// zero object

	GPoint( void )											{  /* random filled */  }
	GPoint( int _x, int _y )								{  x = _x;  y = _y;  }
	GPoint( const POINT& pt )								{  *(POINT*)this = pt;  }
	GPoint( const SIZE& sz )								{  *(SIZE*)this = sz;  }

	// use on LPARAMs
	explicit GPoint( DWORD param )							{  x = LOWORD( param );  y = HIWORD( param );  }

	operator GPoint * ( void )								{  return ( this );  }
	operator const GPoint * ( void ) const					{  return ( this );  }

	bool IsZero( void ) const								{  return ( (x | y) == 0 );  }

	GPoint& Offset( int xoff, int yoff )					{  x += xoff;  y += yoff;  return ( *this );  }
	GPoint& Offset( const POINT& pt )						{  x += pt.x;  y += pt.y;  return ( *this );  }
	GPoint& Offset( const SIZE& sz )						{  x += sz.cx;  y += sz.cy;  return ( *this );  }

	GPoint& operator += ( const SIZE& sz )					{  x += sz.cx;  y += sz.cy;  return ( *this );  }
	GPoint& operator -= ( const SIZE& sz )					{  x -= sz.cx;  y -= sz.cy;  return ( *this );  }
	GPoint& operator += ( const POINT& pt )					{  x += pt.x;  y += pt.y;  return ( *this );  }
	GPoint& operator -= ( const POINT& pt )					{  x -= pt.x;  y -= pt.y;  return ( *this );  }
};

DECL_SELECT_ANY const POINT GPoint::ZERO = { 0, 0 };

inline GPoint operator - ( const POINT& pt )				{  return ( GPoint( -pt.x, -pt.y ) ); }
inline GPoint operator + ( const POINT& l, const POINT& r )	{  return ( GPoint( l.x  + r.x , l.y  + r.y  ) );  }
inline GPoint operator + ( const SIZE&  l, const POINT& r )	{  return ( GPoint( l.cx + r.x , l.cy + r.y  ) );  }
inline GPoint operator + ( const POINT& l, const SIZE&  r )	{  return ( GPoint( l.x  + r.cx, l.y  + r.cy ) );  }
inline GPoint operator - ( const POINT& l, const POINT& r )	{  return ( GPoint( l.x  - r.x , l.y  - r.y  ) );  }
inline GPoint operator - ( const SIZE&  l, const POINT& r )	{  return ( GPoint( l.cx - r.x , l.cy - r.y  ) );  }
inline GPoint operator - ( const POINT& l, const SIZE&  r )	{  return ( GPoint( l.x  - r.cx, l.y  - r.cy ) );  }

inline bool operator == ( const POINT& l, const POINT& r )	{  return ( (l.x == r.x) && (l.y == r.y) );  }
inline bool operator != ( const POINT& l, const POINT& r )	{  return ( (l.x != r.x) || (l.y != r.y) );  }

// note: these are all "if and only if" conditions (for both members).
//       so if !(pt1 <= pt2) that does NOT mean (pt1 > pt2).
inline bool operator <  ( const POINT& l, const POINT& r )	{  return ( (l.x <  r.x) && (l.y <  r.y) );  }
inline bool operator <= ( const POINT& l, const POINT& r )	{  return ( (l.x <= r.x) && (l.y <= r.y) );  }
inline bool operator >  ( const POINT& l, const POINT& r )	{  return ( (l.x >  r.x) && (l.y >  r.y) );  }
inline bool operator >= ( const POINT& l, const POINT& r )	{  return ( (l.x >= r.x) && (l.y >= r.y) );  }

//////////////////////////////////////////////////////////////////////////////
// struct GRect declaration

struct GRect : tagRECT
{
	static const RECT ZERO;		// zero object

	GRect( void )													{  /* random filled */  }
	GRect( int l, int t, int r, int b )								{  left = l;  top = t;  right = r;  bottom = b;  }
	GRect( const RECT& rect )										{  *(RECT*)this = rect;  }
	GRect( const POINT& pt, const SIZE& sz )						{  right = (left = pt.x) + sz.cx;  bottom = (top = pt.y) + sz.cy;  }
	GRect( const POINT& tl, const POINT& br )						{  TopLeft() = tl;  BottomRight() = br;  }

	int           Width      ( void ) const							{  return ( right - left );  }
	int           Height     ( void ) const							{  return ( bottom - top );  }
	GSize         Size       ( void ) const							{  return ( GSize( right - left, bottom - top ) );  }
	GRect         SizeRect   ( void ) const							{  return ( GRect( GPoint::ZERO, Size() ) );  }
	GPoint&       TopLeft    ( void )								{  return ( *(GPoint*)&left );  }
	const GPoint& TopLeft    ( void ) const							{  return ( *(GPoint*)&left );  }
	GPoint&       BottomRight( void )								{  return ( *(GPoint*)&right );  }
	const GPoint& BottomRight( void ) const							{  return ( *(GPoint*)&right );  }
	GPoint        TopRight   ( void ) const							{  return ( GPoint( right, top ) );  }
	GPoint        BottomLeft ( void ) const							{  return ( GPoint( left, bottom ) );  }
	GPoint        Center     ( void ) const							{  return ( GPoint( (right - left) / 2, (bottom - top) / 2 ) );  }
	GPoint        OuterCenter( void ) const							{  return ( TopLeft() + Center() );  }

	operator GRect * ( void )										{  return ( this );  }
	operator const GRect * ( void ) const							{  return ( this );  }

	// rect has no area (may be backwards)
	bool IsEmpty ( void ) const										{  return ( (left >= right) || (top >= bottom) ); }
	// rect has area (not backwards)
	bool IsValid ( void ) const										{  return ( (left < right) && (top < bottom) ); }
	// rect is organized normally (not backwards, empty is ok)
	bool IsNormal( void ) const										{  return ( (left <= right) && (top <= bottom) ); }
	// all params are 0
	bool IsZero  ( void ) const										{  return ( ( left | right | top | bottom ) == 0 ); }

	// valid?
	operator bool( void ) const										{  return ( IsValid() );  }

	GRect& Set     ( int l, int t, int r, int b )					{  left = l;  top = t;  right = r;  bottom = b;  return ( *this );  }
	GRect& Set     ( const POINT& tl, const POINT& br )				{  TopLeft() = tl;  BottomRight() = br;  return ( *this );  }
	GRect& Set     ( const POINT& pt, const SIZE& sz )				{  right = (left = pt.x) + sz.cx;  bottom = (top = pt.y) + sz.cy;  return ( *this );  }
	GRect& SetEmpty( void )											{  *this = ZERO;  return ( *this );  }

	GRect& Inflate( int val )										{  left -= val;  top -= val;  right += val;  bottom += val;  return ( *this );  }
	GRect& Inflate( int x, int y )									{  left -= x;  top -= y;  right += x;  bottom += y;  return ( *this );  }
	GRect& Inflate( const SIZE& sz )								{  return ( Inflate( sz.cx, sz.cy ) );  }
	GRect& Inflate( const RECT& rect )								{  left -= rect.left;  top -= rect.top;  right += rect.right;  bottom += rect.bottom;  return ( *this );  }
	GRect& Inflate( int l, int t, int r, int b )					{  left -= l;  top -= t;  right += r;  bottom += b;  return ( *this );  }

	GRect& Deflate( int val )										{  return ( Inflate( -val ) );  }
	GRect& Deflate( int x, int y )									{  return ( Inflate( -x, -y ) );  }
	GRect& Deflate( const SIZE& sz )								{  return ( Deflate( sz.cx, sz.cy ) );  }
	GRect& Deflate( const RECT& rect )								{  left += rect.left;  top += rect.top;  right -= rect.right;  bottom -= rect.bottom;  return ( *this );  }
	GRect& Deflate( int l, int t, int r, int b )					{  left += l;  top += t;  right -= r;  bottom -= b;  return ( *this );  }

	GRect& Offset ( int x, int y )									{  left += x;  top += y;  right += x;  bottom += y;  return ( *this );  }
	GRect& Offset ( const SIZE& sz )								{  return ( Offset( sz.cx, sz.cy ) );  }
	GRect& Offset ( const POINT& pt )								{  return ( Offset( pt.x, pt.y ) );  }
	GRect& OffsetX( int x )											{  left += x;  right += x;  return ( *this );  }
	GRect& OffsetY( int y )											{  top += y;  bottom += y;  return ( *this );  }

	GRect& MoveTo( const POINT& origin )							{  right += origin.x - left; left = origin.x;  bottom += origin.y - top; top = origin.y;  return ( *this );  }
	GRect& ResizeTo( const SIZE& sz )								{  right = left + sz.cx; bottom = top + sz.cy;  return ( *this );  }

	bool Intersects(const RECT& r) const							{  return ( (left < r.right) && (right > r.left) && (top < r.bottom) && (bottom > r.top) );  }

	void   NormalizeRect ( void )									{  if ( left > right )  std::swap( left, right );  if ( top > bottom )  std::swap( top, bottom );  }
	GRect& Intersect     ( const RECT& r )							{  if ( !IntersectValid( r ) )  SetEmpty();  return ( *this );  }	// $$ inefficient! fix assumption in future - allow invalid rects!
	bool   IntersectValid( const RECT& r )							{  left = max( left, r.left );  top = max( top, r.top );  right = min( right, r.right );  bottom = min( bottom, r.bottom );  return ( IsValid() );  }
	GRect& Union         ( const RECT& r )							{  left = min( left, r.left );  top = min( top, r.top );  right = max( right, r.right );  bottom = max( bottom, r.bottom );  return ( *this );  }

	GRect& Intersect     ( const RECT& r0, const RECT& r1 )			{  if ( !IntersectValid( r0, r1 ))  SetEmpty();  return ( *this );  }
	bool   IntersectValid( const RECT& r0, const RECT& r1 )			{  left = max( r0.left, r1.left );  top = max( r0.top, r1.top );  right = min( r0.right, r1.right );  bottom = min( r0.bottom, r1.bottom );  return ( IsValid() );  }
	GRect& Union         ( const RECT& r0, const RECT& r1 )			{  left = min( r0.left, r1.left );  top = min( r0.top, r1.top );  right = max( r0.right, r1.right );  bottom = max( r0.bottom, r1.bottom );  return ( *this );  }

	bool Contains( const POINT& p ) const							{  return ( ( left <= p.x ) && ( top <= p.y ) && ( p.x < right ) && ( p.y < bottom ) );  }
	bool Contains( const RECT& r ) const							{  return ( ( left <= r.left ) && ( top <= r.top ) && ( r.right <= right ) && ( r.bottom <= bottom ) );  }

	GRect& CenterIn( const GRect& container )						{  return ( Offset( container.OuterCenter() - OuterCenter() ) );  }

	bool Constrain  ( GPoint& point )								{  bool rc = clamp_min_max( point.x, left, right );  if ( clamp_min_max( point.y, top, bottom ) )  rc = true;  return ( rc );  }
	bool ConstrainTo( const GRect& bounds );
	void MakeBorders( const GRect& hole, GRect& l, GRect& t, GRect& r, GRect& b ) const;

	GRect& operator += ( const POINT& pt   )						{  Offset( pt.x, pt.y );  return ( *this );  }
	GRect& operator += ( const SIZE & sz   )						{  Offset( sz.cx, sz.cy );  return ( *this );  }
	GRect& operator -= ( const POINT& pt   )						{  Offset( -pt.x, -pt.y );  return ( *this );  }
	GRect& operator -= ( const SIZE & sz   )						{  Offset( -sz.cx, -sz.cy );  return ( *this );  }
	GRect& operator &= ( const RECT & rect )						{  return ( Intersect( rect ) );  }
	GRect& operator |= ( const RECT & rect )						{  return ( Union( rect ) );  }
};

DECL_SELECT_ANY const RECT GRect::ZERO = { 0, 0, 0, 0 };

inline GRect Intersect( const RECT& r0, const RECT& r1 )			{  return ( GRect().Intersect( r0, r1 ) );  }
inline GRect Union    ( const RECT& r0, const RECT& r1 )			{  return ( GRect().Union( r0, r1 ) );  }

inline GRect operator + ( const RECT& r, const SIZE & sz   )		{  return ( GRect( r ).Offset( sz ) );  }
inline GRect operator + ( const RECT& r, const POINT& pt   )		{  return ( GRect( r ).Offset( pt ) );  }
inline GRect operator - ( const RECT& r, const SIZE & sz   )		{  return ( GRect( r ).Offset( -sz ) );  }
inline GRect operator - ( const RECT& r, const POINT& pt   )		{  return ( GRect( r ).Offset( -pt ) );  }

inline GRect operator & ( const RECT& r0, const RECT& r1 )			{  return ( Intersect( r0, r1 ) );  }
inline GRect operator | ( const RECT& r0, const RECT& r1 )			{  return ( Union( r0, r1 ) );  }

inline bool operator == ( const RECT& l, const RECT& r )			{  return ( (l.left == r.left) && (l.top == r.top) && (l.right == r.right) && (l.bottom == r.bottom) );  }
inline bool operator != ( const RECT& l, const RECT& r )			{  return ( !(l == r) );  }

inline bool GRect :: ConstrainTo( const GRect& bounds )
{
	// bail early if nothing to do
	if ( bounds.Contains( *this ) )
	{
		return ( false );
	}

	// must fit!
#	if GP_DEBUG
	GSize sz1 = Size(), sz2 = bounds.Size();
	gpassert( (sz1.cx <= sz2.cx) && (sz1.cy <= sz2.cy) );
#	endif // GP_DEBUG

	// check left-right
	if ( left < bounds.left )
	{
		OffsetX( bounds.left - left );
	}
	else if ( right > bounds.right )
	{
		OffsetX( bounds.right - right );
	}

	// check up-down
	if ( top < bounds.top )
	{
		OffsetY( bounds.top - top );
	}
	else if ( bottom > bounds.bottom )
	{
		OffsetY( bounds.bottom - bottom );
	}

	return ( true );
}

/*
	Borders work like this:

	y0  +---------------+
		|       t       |
	y1  +---+-------+---+
		|   |       |   |
		| l | hole  | r |
		|   |       |   |
	y2  +---+-------+---+
		|       b       |
	y3  +---+-------+---+
		x0  x1      x2  x3
*/

inline void GRect :: MakeBorders( const GRect& hole, GRect& l, GRect& t, GRect& r, GRect& b ) const
{
	int y0 = top;
	int y1 = hole.top;
	int y2 = hole.bottom;
	int y3 = bottom;

	int x0 = left;
	int x1 = hole.left;
	int x2 = hole.right;
	int x3 = right;

	if ( (x1 - x0) > 0 )
	{
		l.Set( x0, y1, x1, y2 );
	}
	else
	{
		l.SetEmpty();
	}

	if ( (x3 - x2) > 0 )
	{
		r.Set( x2, y1, x3, y2 );
	}
	else
	{
		r.SetEmpty();
	}

	if ( (y1 - y0) > 0 )
	{
		t.Set( x0, y0, x3, y1 );
	}
	else
	{
		t.SetEmpty();
	}

	if ( (y3 - y2) > 0 )
	{
		b.Set( x0, y2, x3, y3 );
	}
	else
	{
		b.SetEmpty();
	}
}

//////////////////////////////////////////////////////////////////////////////

#endif  // __WINGEOMETRY_H

//////////////////////////////////////////////////////////////////////////////
