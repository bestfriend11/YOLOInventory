//////////////////////////////////////////////////////////////////////////////
//
// File     :  RapiImage.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains image manipulation routines - readers and writers etc
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __RAPIIMAGE_H
#define __RAPIIMAGE_H

//////////////////////////////////////////////////////////////////////////////

#include "FileSys.h"
#include "RapiOwner.h"

//////////////////////////////////////////////////////////////////////////////
// misc declarations

	class RapiMemImage;
	class RapiImageReader;

// DirectX surface format.

	enum ePixelFormat
	{
		PIXEL_UNKNOWN,

		// alpha formats
		PIXEL_ARGB_8888,
		PIXEL_ARGB_4444,
		PIXEL_ARGB_1555,

		// special formats
		PIXEL_RGB_565,
		PIXEL_RGB_888,			// $ used for bmp - alpha is thrown away
	};

	ePixelFormat GetPixelFormat( D3DFORMAT format );
	int GetSizeBytes( ePixelFormat e );

// Disk image format.

	enum eImageFormat
	{
		SET_BEGIN_ENUM( IMAGE_, 0 ),

		IMAGE_RAW,
		IMAGE_BMP,
		IMAGE_PNG,
		IMAGE_JPG,
		IMAGE_PSD,

		SET_END_ENUM( IMAGE_ ),
	};

	const char* GetExtension( eImageFormat format );

// Helper methods.

	// downsample using box filter - width/height in pixels, stride in bytes
	void BoxFilter(       DWORD* dst, int dstWidth, int dstHeight, int dstStride,
					const DWORD* src, int srcWidth, int srcHeight, int srcStride );

	// copy pixels out, color converting if necessary - returns dst after end
	void* CopyPixels(       void* dst, ePixelFormat dstFormat, int dstWidth, int dstHeight, int dstStride,
					  const void* src, ePixelFormat srcFormat, int srcWidth, int srcHeight, int srcStride,
					  bool flip, bool allowDither, float saturation = 1.0f );

	// write an image out
	bool WriteImage( FileSys::StreamWriter& writer, eImageFormat format, RapiImageReader* reader, bool flip, bool includeMips );
	bool WriteImage( FileSys::StreamWriter& writer, eImageFormat format, const RapiMemImage& image, bool flip, bool includeMips, bool screenShot );
	bool WriteImage( const char* fileName, eImageFormat format, RapiImageReader* reader, bool flip, bool includeMips );
	bool WriteImage( const char* fileName, eImageFormat format, const RapiMemImage& image, bool flip, bool includeMips, bool screenShot );

//////////////////////////////////////////////////////////////////////////////
// class RapiMemImage declaration

class RapiMemImage
{
public:
	SET_NO_INHERITED( RapiMemImage );

// Setup.

	// creation
	static RapiMemImage* CreateMemImage( const char* fileName, bool wantsMips, bool screenShot );

	// ctor/dtor
	RapiMemImage( int width = 0, int height = 0, DWORD fill = 0xFF000000 )	{  Init( width, height, fill );  }
   ~RapiMemImage( void )													{  }

	// reconstruction
	void Init( int width, int height, DWORD fill = 0xFF000000 )
	{
		m_Bitmap.clear();
		m_Bitmap.resize( width * height, fill );
		m_Width = width;
		m_Height = height;
	}

	// construct direct from file
	bool Init( const char* fileName );

// Utility.

	void CopyToClipboard( bool flip ) const;

// Query.

	// simple query
	int          GetWidth       ( void ) const					{  return ( m_Width );  }
	int          GetHeight      ( void ) const					{  return ( m_Height );  }
	int          GetStride      ( void ) const					{  return ( m_Width * 4 );  }
	int          GetSizeInPixels( void ) const					{  return ( m_Bitmap.size() );  }
	int          GetSizeInBytes ( void ) const					{  return ( GetSizeInPixels() * 4 );  }
	bool         IsSquare       ( void ) const					{  return ( m_Width == m_Height );  }

	// direct bit access
	const DWORD* GetBits        ( void ) const					{  return ( m_Bitmap.begin() );  }
	DWORD*       GetBits        ( void )						{  return ( m_Bitmap.begin() );  }
	const DWORD* GetBitsEnd     ( void ) const					{  return ( m_Bitmap.end() );  }
	DWORD*       GetBitsEnd     ( void )						{  return ( m_Bitmap.end() );  }
	DWORD*       GetBits        ( int y )						{  return ( GetBits() + (m_Width * y) );  }
	DWORD*       GetBits        ( int x, int y )				{  return ( GetBits( y ) + x );  }
	DWORD        GetPixel       ( int x, int y )				{  return ( *GetBits( x, y ) );  }
	void         SetPixel       ( int x, int y, DWORD pixel )	{  *GetBits( x, y ) = pixel;  }

	// advanced stuff
	DWORD CalcCrc32             ( bool includeAlpha = true ) const;
	void  FlipVertical          ( void );
	void  FlipHorizontal        ( void );
	void  RotateClockwise       ( void );
	void  RotateCounterClockwise( void );

private:
	typedef std::vector <DWORD> Bitmap;

	int    m_Width;
	int    m_Height;
	Bitmap m_Bitmap;	
};

//////////////////////////////////////////////////////////////////////////////
// class RapiImageReader declaration

class RapiImageReader
{
public:
	SET_NO_INHERITED( RapiImageReader );

	typedef std::vector <my RapiMemImage*> ImageColl;

// Setup.

	// $$$$$ update to use the naming key

	// creation
	static RapiImageReader* CreateReader( const char* fileName, bool wantsMips, bool screenShot );
	static RapiImageReader* CreateReader( const char* name, const_mem_ptr mem, bool wantsMips, bool screenShot );
	static RapiImageReader* CreateReader( const char* name, const RapiMemImage& image, bool wantsMips, bool screenShot );
	static RapiImageReader* CreateReader( const char* name, LPDIRECT3DSURFACE8 surface, bool wantsMips, bool screenShot, const RECT* rect = NULL );
	static RapiImageReader* CreateReader( Rapi* rapi, DWORD tex, bool wantsMips, const RECT* rect = NULL );

	// alias registration
	static void RegisterFileTypeAliases  ( void );
	static void UnregisterFileTypeAliases( void );

	// ctor/dtor
	RapiImageReader( void )  {  }
	virtual ~RapiImageReader( void )  {  }

// API.

	// query
	virtual int GetWidth ( void ) const = 0;					// width of next surface
	virtual int GetHeight( void ) const = 0;					// height of next surface
	virtual int GetFps   ( void ) const  {  return ( 0 );  }	// target fps (if applicable)

	// surfaces - pass NULL into out to skip this surface, returns true if more surfs
	virtual bool GetNextSurface( void* out, ePixelFormat format, int stride, bool flip, bool allowDither, float saturation, int dstWidth = -1, int dstHeight = -1 ) = 0;

	// read directly into a mem image
	bool GetNextSurface( RapiMemImage& image, bool flip );

	// read all into a set of mem images
	int GetAllSurfaces( ImageColl& coll, bool flip );

	// easier way to skip
	bool SkipNextSurface( void )  {  return ( GetNextSurface( NULL, PIXEL_ARGB_8888, 0, false, false, 1.0f ) );  }

private:
	FileSys::AutoFileHandle m_FileHandle;
	FileSys::AutoMemHandle  m_MemHandle;

	SET_NO_COPYING( RapiImageReader );
};

//////////////////////////////////////////////////////////////////////////////
// class RapiColorFiller declaration

class RapiColorFiller : public RapiImageReader
{
public:
	SET_INHERITED( RapiColorFiller, RapiImageReader );
	
	RapiColorFiller( int width, int height, DWORD color )  {  m_Width = width;  m_Height = height;  m_Color = color;  }
	virtual ~RapiColorFiller( void )  {  }

	virtual int GetWidth ( void ) const  {  return ( m_Width );  }
	virtual int GetHeight( void ) const  {  return ( m_Height );  }

	virtual bool GetNextSurface( void* out, ePixelFormat format, int stride, bool flip, bool allowDither, float saturation, int dstWidth = -1, int dstHeight = -1 );

private:
	int   m_Width;
	int   m_Height;
	DWORD m_Color;

	SET_NO_COPYING( RapiColorFiller );
};

//////////////////////////////////////////////////////////////////////////////

#endif  // __RAPIIMAGE_H

//////////////////////////////////////////////////////////////////////////////
