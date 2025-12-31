//////////////////////////////////////////////////////////////////////////////
//
// File     :  QMemLogDb.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_QMem.h"
#include "QMemLogDb.h"

#if GP_ENABLE_STATS

#include "DebugHelp.h"
#include "FuBiTraits.h"

//////////////////////////////////////////////////////////////////////////////

namespace QMem  {  // begin of namespace QMem

//////////////////////////////////////////////////////////////////////////////
// class LogDb implementation

LogDb :: LogDb( const char* imageName, DWORD imageBase )
{
	// ready the symbol engine
	m_Symbols.LoadModuleSymbols( imageName, imageBase );

	// init vars
	m_NextId = 0;
}

LogDb :: ~LogDb( void )
{
	// this space intentionally left blank...
}

void LogDb :: AddMethod(
		IN  DWORD eip, IN DWORD esp,
		OUT ThreadEntry*& threadEntry, OUT bool& threadEntryNew,
		OUT ClassEntry *& classEntry , OUT bool& classEntryNew,
		OUT MethodEntry*& methodEntry, OUT bool& methodEntryNew )
{
/*	summary:	this function adds a thread, class, and method to the database
				if they aren't already there.

	inputs:		pass in the instruction and stack pointers from the log. the
				function uses esp to detect which thread we are, and eip to
				resolve to a function name using the symbol engine.

	outputs:	it will set the incoming pointers to the entries in the
				database - treat them as temporaries and do not cache, as the
				db may shift them around. the bools are for whether or not new
				entries were added. use this to decide whether or not to output
				a tag for a newly found thread, class, or method.
*/

	// defaults
	classEntry  = NULL;  classEntryNew  = false;
	methodEntry = NULL;  methodEntryNew = false;

// Add thread.

	AddThread( esp, threadEntry, threadEntryNew );

// Look up symbol.

	// check the cache to see if we already know about this one and we're in range
	MethodIndex::iterator methodFound = m_MethodByEipIndex.find( eip );
	if ( methodFound != m_MethodByEipIndex.end() )
	{
		// cool! set the pointers
		methodEntry = &methodFound->second->second;
		if ( methodEntry->m_ClassId != 0 )
		{
			classEntry = &m_ClassByIdIndex.find( methodEntry->m_ClassId )->second->second;
		}
	}
	else
	{
		// look up symbol name
		gpstring methodName, className;
		if ( m_Symbols.AddressToSymName( methodName, eip, true ) )
		{
			// $$$ separate class name from method name
		}
		else
		{
			// can't find it, so just make it a number
			methodName = ToString( eip, FuBi::XFER_HEX );
		}

// Add class.

		// find it
		ClassDb::iterator classFound = m_ClassDb.find( className );
		if ( classFound != m_ClassDb.end() )
		{
			// found, set the vars
			classEntry = &classFound->second;
		}
		else
		{
			// add it
			classFound    = m_ClassDb.insert( std::make_pair( className, ClassEntry() ) ).first;
			classEntry    = &classFound->second;
			classEntryNew = true;

			// configure new entry
			DWORD classId = ++m_NextId;
			classEntry->m_ClassId   = classId;
			classEntry->m_ClassName = className;

			// add to index
			m_ClassByIdIndex[ classId ] = classFound;
		}

// Add method.

		// find it $$$ must use equal_range to include cross-class names, or
		//             use "fullname" separate from methodname and classname
		MethodDb::iterator methodFound = m_MethodDb.find( methodName );
		if ( methodFound != m_MethodDb.end() )
		{
			// found!
			methodEntry = &methodFound->second;
		}
		else
		{
			// add it
			methodFound    = m_MethodDb.insert( std::make_pair( methodName, MethodEntry() ) ).first;
			methodEntry    = &methodFound->second;
			methodEntryNew = true;

			// configure new entry
			DWORD methodId = ++m_NextId;
			methodEntry->m_MethodId   = methodId;
			methodEntry->m_MethodName = methodName;
			methodEntry->m_MethodEip  = eip;
			methodEntry->m_ClassId    = classEntry->m_ClassId;
			m_Symbols.AddressToLocation( methodEntry->m_SourceFile, methodEntry->m_SourceLine, eip );

			// add to indexes
			m_MethodByIdIndex[ methodId ] = methodFound;
		}

		// always add the given eip to speed lookups this method
		m_MethodByEipIndex[ eip ] = methodFound;
	}
}

DWORD LogDb :: AddMethod( IN DWORD eip, IN DWORD esp )
{
/*	summary:	this function calls AddMethod() and returns the MethodId of the
				new entry. use this when you don't care about getting entry
				pointers and 'is new' flags.

	inputs:		pass in the instruction and stack pointers from the log.

	return:		returns the id of the (possibly new) method.
*/

	ThreadEntry* threadEntry = NULL;  bool threadEntryNew = false;
	ClassEntry * classEntry  = NULL;  bool classEntryNew  = false;
	MethodEntry* methodEntry = NULL;  bool methodEntryNew = false;

	AddMethod( eip, esp, threadEntry, threadEntryNew, classEntry, classEntryNew, methodEntry, methodEntryNew );
	return ( methodEntry->m_MethodId );
}

void LogDb :: AddThread( IN DWORD esp, OUT ThreadEntry*& threadEntry, OUT bool& threadEntryNew )
{
	// defaults
	threadEntry = NULL;  threadEntryNew = false;

	// first locate the thread
	ThreadColl::iterator ithread;
	for ( ithread = m_ThreadColl.begin() ; ithread != m_ThreadColl.end() ; ++ithread )
	{
		// if the stack pointer is within 64K then it's probably the same
		// thread. default thread stack size is 1 meg, and a typical function
		// is not going to allocate a huge amount on the stack at once.
		if ( abs( ithread->m_LastEsp - esp ) < 65536 )
		{
			break;
		}
	}

	// nothing suitable found?
	if ( ithread == m_ThreadColl.end() )
	{
		// add new one
		ithread        = m_ThreadColl.insert( m_ThreadColl.end(), ThreadEntry() );
		threadEntryNew = true;

		// configure it
		ithread->m_ThreadId   = ++m_NextId;
		ithread->m_ThreadName = ToString( ithread->m_ThreadId );
	}

	// take it
	ithread->m_LastEsp = esp;
	threadEntry = &*ithread;
}

const ThreadEntry* LogDb :: FindThreadEntryById( DWORD threadId ) const
{
	ThreadColl::const_iterator i, ibegin = m_ThreadColl.begin(), iend = m_ThreadColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( i->m_ThreadId == threadId )
		{
			return ( &*i );
		}
	}
	return ( NULL );
}

const gpstring& LogDb :: GetThreadName( DWORD threadId ) const
{
	const ThreadEntry* threadEntry = FindThreadEntryById( threadId );
	return ( (threadEntry != NULL) ? threadEntry->m_ThreadName : gpstring::EMPTY );
}

const gpstring& LogDb :: GetClassName( DWORD classId ) const
{
	ClassIndex::const_iterator found = m_ClassByIdIndex.find( classId );
	return ( found != m_ClassByIdIndex.end() ? found->second->second.m_ClassName : gpstring::EMPTY );
}

const gpstring& LogDb :: GetClassNameByMethodId( DWORD methodId ) const
{
	MethodIndex::const_iterator found = m_MethodByIdIndex.find( methodId );
	return ( found != m_MethodByIdIndex.end() ? GetClassName( found->second->second.m_ClassId ) : gpstring::EMPTY );
}

const gpstring& LogDb :: GetMethodName( DWORD methodId ) const
{
	MethodIndex::const_iterator found = m_MethodByIdIndex.find( methodId );
	return ( found != m_MethodByIdIndex.end() ? found->second->second.m_MethodName : gpstring::EMPTY );
}

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace QMem

#endif // GP_ENABLE_STATS

//////////////////////////////////////////////////////////////////////////////
