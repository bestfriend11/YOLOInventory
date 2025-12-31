//////////////////////////////////////////////////////////////////////////////
//
// File     :  QMemLogDb.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains classes that aid in analyzing a sample log. This is
//             support code for QMem.
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __QMEMLOGDB_H
#define __QMEMLOGDB_H

//////////////////////////////////////////////////////////////////////////////

#include "DebugHelp.h"
#include "GpColl.h"

#if GP_ENABLE_STATS

//////////////////////////////////////////////////////////////////////////////

namespace QMem  {  // begin of namespace QMem

//////////////////////////////////////////////////////////////////////////////
// class LogDb implementation

struct ThreadEntry
{
	DWORD    m_ThreadId;		// internal id of thread
	gpstring m_ThreadName;		// name of this thread
	DWORD    m_LastEsp;			// last value of ESP as we move through the log
};

struct ClassEntry
{
	DWORD    m_ClassId;			// internal id of class
	gpstring m_ClassName;		// name of this class
};

struct MethodEntry
{
	DWORD    m_MethodId;		// internal id of method
	gpstring m_MethodName;		// name of this method
	gpstring m_SourceFile;		// source code file name
	DWORD    m_SourceLine;		// line number of function
	DWORD    m_MethodEip;		// address of this method
	DWORD    m_ClassId;			// internal id of owning class (0 for global)
};

class LogDb
{
public:
	SET_NO_INHERITED( LogDb );

	LogDb( const char* imageName, DWORD imageBase );
   ~LogDb( void );

	void AddMethod( IN  DWORD eip, IN DWORD esp,
					OUT ThreadEntry*& threadEntry, OUT bool& threadEntryNew,
					OUT ClassEntry *& classEntry , OUT bool& classEntryNew,
					OUT MethodEntry*& methodEntry, OUT bool& methodEntryNew );

	DWORD AddMethod( IN DWORD eip, IN DWORD esp );

	void AddThread( IN DWORD esp, OUT ThreadEntry*& threadEntry, OUT bool& threadEntryNew );

	const ThreadEntry* FindThreadEntryById( DWORD threadId ) const;

	const gpstring& GetThreadName         ( DWORD threadId ) const;
	const gpstring& GetClassName          ( DWORD classId  ) const;
	const gpstring& GetClassNameByMethodId( DWORD methodId ) const;
	const gpstring& GetMethodName         ( DWORD methodId ) const;

private:

	typedef std::list <ThreadEntry>                       ThreadColl;
	typedef std::map <gpstring, ClassEntry, string_less>  ClassDb;
	typedef std::map <DWORD, ClassDb::iterator>           ClassIndex;
	typedef std::map <gpstring, MethodEntry, string_less> MethodDb;
	typedef std::map <DWORD, MethodDb::iterator>          MethodIndex;

	DbgSymbolEngine m_Symbols;				// symbol lookup engine
	DWORD           m_NextId;				// next id to use for any entry's id (must be unique across all types)
	ThreadColl      m_ThreadColl;			// threads we've found in order of detection
	ClassDb         m_ClassDb;				// classes we've found by name
	ClassIndex      m_ClassByIdIndex;		// classes indexed by id
	MethodDb        m_MethodDb;				// methods we've found by name
	MethodIndex     m_MethodByIdIndex;		// methods indexed by id
	MethodIndex     m_MethodByEipIndex;		// methods indexed by their address

	SET_NO_COPYING( LogDb );
};

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace QMem

#endif // GP_ENABLE_STATS

#endif  // __QMEMLOGDB_H

//////////////////////////////////////////////////////////////////////////////
