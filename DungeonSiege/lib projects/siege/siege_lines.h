#pragma once
#ifndef _SIEGE_LINES_
#define _SIEGE_LINES_

// ========================================================================
//   Header File: siege_lines.h
//   Description: Declares siege_worldspace_lines,
//                siege_screenspace_lines
// ========================================================================

// ========================================================================
//   Explicit Dependencies
// ========================================================================

#include <list>
#include "vector_3.h"
#include "siege_database_guid.h"

namespace siege
{
	class database_guid;
	
    const DWORD DEFAULT_LINE_COLOR = 0xFFFFFFFF;
}

// ========================================================================
//   Class: siege_worldspace_lines
//   Description: Handles buffering of world-space lines
// ========================================================================
namespace siege {
	class worldspace_lines

	{
	public:
		// Worldspace drawing
		void DrawLine(vector_3 const &Endpoint0,
					  vector_3 const &Endpoint1,
					  const DWORD Color = DEFAULT_LINE_COLOR );

		void Reset();
				  
		// Rendering
  		virtual void Render() const;
	
	private:

		class line
		{
		public:
			vector_3 Endpoints[2];
			DWORD Color;
		};
		typedef std::list<line> lines;
		mutable lines Lines;

	};
}

// ========================================================================
//   Class: siege_screenspace_lines
//   Description: Handles buffering of screen-space lines
// ========================================================================
namespace siege {
	
	class screenspace_lines

	{
	public:
		
		// Screenspace drawing
		void DrawLine( const float StartScreenX, const float StartScreenY,
					   const float EndScreenX, const float EndScreenY,
					   const DWORD Color = DEFAULT_LINE_COLOR );
		void Reset();

		// Rendering
		virtual void Render() const;
		
	private:
	
		class line
		{
		public:
			float Endpoints[2][2];
			DWORD Color;
		};
		typedef std::list<line> lines;
		mutable lines Lines;

	};
}

// ========================================================================
//   Class: siege_nodespace_lines
//   Description: Handles buffering of node-space lines
// ========================================================================
namespace siege {
	class nodespace_lines

	{
	public:
		// Nodespace drawing
		void DrawLine(vector_3 const &Endpoint0,
					  vector_3 const &Endpoint1,
					  const DWORD Color = DEFAULT_LINE_COLOR,
					  const database_guid& node_guid=UNDEFINED_GUID );
		
		void DrawCircle(const vector_3 &Center,
						const database_guid& node_guid,
						const float radius,
						const float startang = 0.0f,
						const float arclength = 0.0f,
						const DWORD Color = DEFAULT_LINE_COLOR,
						const bool  Filled = false );

		void Reset();
				  
		// Rendering
  		virtual void Render() const;
	
	private:

		class line
		{
		public:
			vector_3		Endpoints[2];
			DWORD			Color;
			database_guid	Node;
		};
		typedef std::list<line> lines;
		mutable lines Lines;

		class circle
		{
		public:
			vector_3		Center;
			float			Radius;
			float			StartAngle;
			DWORD			NumSlices;
			float			SliceStep;
			DWORD			Color;
			bool			Filled;
			database_guid	Node;
		};
		typedef std::map<float,circle> circles;
		mutable circles Circles;

	};
}

#endif