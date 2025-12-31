//////////////////////////////////////////////////////////////////////////////
//
// File     :  ReportSys.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains a set of classes that encapsulate and automate end
//             user and developer feedback.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __REPORTSYS_H
#define __REPORTSYS_H

//////////////////////////////////////////////////////////////////////////////

#include "BlindCallback.h"
#include "FileSysUtils.h"
#include "FuBiDefs.h"

#include <vector>
#include <map>

//////////////////////////////////////////////////////////////////////////////
// forward declarations

namespace FileSys
{
	class StreamWriter;
}

//////////////////////////////////////////////////////////////////////////////

namespace ReportSys  {  // begin of namespace ReportSys

//////////////////////////////////////////////////////////////////////////////
// types

typedef FuBi::HeaderSpec Schema;

//////////////////////////////////////////////////////////////////////////////
// class Sink declaration

class Sink
{
public:
	SET_NO_INHERITED( Sink );

// Types.

	enum eChange
	{
		CHANGE_SCHEMA,
		CHANGE_INDENT,
		CHANGE_INDENTSPACES,
		CHANGE_COLOR,
		CHANGE_COLORDEFAULT,
	};

// Setup.

	// ctor/dtor
	Sink( void );
	virtual ~Sink( void );

	// child sinks
	virtual void AddSink   ( Sink* sink, bool owned, bool pushBack = true );
	virtual void RemoveSink( Sink* sink, bool delIfOwned = true );

	// query
	virtual Sink* FindSinkByType( const char* type );

	// referencing contexts
	virtual void AddContextRef   ( Context* context );
	virtual void RemoveContextRef( Context* context );

	// identification
			int             GetId           ( void ) const				{  return ( m_Id );  }
	virtual void            SetName         ( const char* name );
			void            SetGeneratedName( void )					{  SetName( GenerateName() );  }
	static  gpstring        GenerateName    ( void );
			const gpstring& GetName         ( void ) const				{  return ( m_Name );  }
			void            SetType         ( const char* type )		{  gpassert( (type != NULL) && (*type != '\0') );  m_Type = type;  }
			const gpstring& GetType         ( void ) const				{  return ( m_Type );  }

// State.

	// enabling
	virtual void Enable   ( bool enable = true );
			void Disable  ( void )										{  Enable( false );  }
	virtual bool IsEnabled( void ) const								{  return ( m_Enabled && (m_ChildEnableCount != 0) );  }

	// reports
	virtual void OnBeginTable( const Context* context );
	virtual void OnEndTable  ( const Context* context );

	// reports
	virtual void OnBeginReport( const Context* context );
	virtual void OnEndReport  ( const Context* context );

	// change management
	virtual void OnFormatChange( const Context* context, eChange change );

// Output.

	// generic string output
	virtual bool OutputString( const Context* context, const char* text, int len,
							   bool startLine, bool endLine ) = 0;

	// table-based output
	virtual bool OutputField( const Context* context, const char* text, int len ) = 0;

protected:

	// for derived override
	virtual void* OnCreateStateInfo ( Context* /*context*/ )					{  return ( NULL );  }
	virtual void  OnDestroyStateInfo( void* /*info*/ )							{  }

	// derived utility
	void* GetStateInfo( Context* context );

	// helpers
	static int         GetIndentSpaces( const Context* context );
	static const char* MakeIndentText( int indent );

private:

	// utility
	void PrivateRemoveChild( Sink* child, bool owned );
	void PrivateEnable( bool old );

// Types.

	struct ChildEntry
	{
		Sink* m_Sink;						// a child sink
		bool  m_Owned;						// true if we should delete it in ~Sink

		ChildEntry( Sink* sink, bool owned )
			{  m_Sink = sink;  m_Owned = owned;  }
		bool operator == ( const Sink* obj ) const
			{  return ( m_Sink == obj );  }
	};

	struct ContextEntry
	{
		int   m_RefCount;					// ref count for this context
		void* m_DerivedStateInfo;			// generic state info specific to this context for the derived class to use

		ContextEntry( void )
			{  m_RefCount = 0;  m_DerivedStateInfo = NULL;  }
	};

#	if GP_DEBUG

	enum eLockType
	{
		LOCK_READ,
		LOCK_WRITE,
	};

	struct IterLock;
	friend IterLock;

	struct IterLock
	{
		int* m_IterLock;
		eLockType m_LockType;

		IterLock( int* iterLock, eLockType lockType )
		{
			gpassert( iterLock != NULL );

			m_IterLock = iterLock;
			m_LockType = lockType;

			gpassert( ReportSys::IsFatalOccurring() || ((m_LockType == LOCK_READ) ? (*m_IterLock >= 0) : (*m_IterLock == 0)) );
			*m_IterLock += (m_LockType == LOCK_READ) ? 1 : -1;
		}

	   ~IterLock( void )
		{
			*m_IterLock += (m_LockType == LOCK_READ) ? -1 : 1;
		}
	};

#	endif // GP_DEBUG

	// collections
	typedef std::vector <ChildEntry>             ChildColl;
	typedef std::vector <Sink*>                  ParentColl;
	typedef std::map    <Context*, ContextEntry> ContextDb;

// Private data.

	// local state
	bool       m_Enabled;					// global enable state for this
	int        m_Id;						// id of this state
	gpstring   m_Name;						// name of this sink (used for global registration)
	gpstring   m_Type;						// general type of this sink

	// cache
	int        m_ChildEnableCount;			// cache to speed up children enabling/disabling self for fast bailout (-1 for no children)

	// collections
	ChildColl  m_ChildColl;					// our child sinks
	ParentColl m_ParentColl;				// our parent sinks
	ContextDb  m_ContextDb;					// state-specific context info

	// debug
#	if GP_DEBUG
	int        m_ChildCollIterCount;		// iteration level for children
	int        m_ParentCollIterCount;		// iteration level for parents
	int        m_ContextCollIterCount;		// iteration level for contexts
#	endif // GP_DEBUG

	// static utility
	static gpstring ms_IndentText;			// common buffer
	static int      ms_NextId;				// next sink id

	SET_NO_COPYING( Sink );
};

//////////////////////////////////////////////////////////////////////////////
// class MultiSink declaration

// this is a simple redirector. actually it's just a Sink that implements its
// functions (to do nothing).

class MultiSink : public Sink
{
public:
	SET_INHERITED( MultiSink, Sink );

	MultiSink( void )								{  }
	virtual ~MultiSink( void )						{  }

	virtual bool OutputString( const Context* context, const char* text, int len, bool startLine, bool endLine )
							 {  return ( Inherited::OutputString( context, text, len, startLine, endLine ) );  }
	virtual bool OutputField ( const Context* context, const char* text, int len )
							 {  return ( Inherited::OutputField( context, text, len ) );  }

private:
	SET_NO_COPYING( MultiSink );
};

//////////////////////////////////////////////////////////////////////////////
// class StreamSink declaration

// this class just routes field output through normal OutputString. derive
// from this rather than Sink if you want automatic field processing.

class StreamSink : public Sink
{
public:
	SET_INHERITED( StreamSink, Sink );

	StreamSink( const char* name = NULL )			{  SetName( name );  }
	virtual ~StreamSink( void )						{  }

	virtual bool OutputField( const Context* context, const char* text, int len );

private:
	SET_NO_COPYING( StreamSink );
};

//////////////////////////////////////////////////////////////////////////////
// class FileSink declaration

// TARGET

class FileSink : public StreamSink
{
public:
	SET_INHERITED( FileSink, StreamSink );

	// ctor/dtor
	FileSink( HANDLE file = INVALID_HANDLE_VALUE, bool ownsHandle = true, const char* name = NULL );
	FileSink( const char* fileName, bool truncate = GP_RETAIL, const char* name = NULL );
	virtual ~FileSink( void );

	// sink-specific functions
	bool Open  ( const char* fileName, bool truncate = GP_RETAIL );
	void Open  ( HANDLE file )						{  m_File = file;  }
	bool IsOpen( void ) const						{  return ( m_File.IsOpen() );  }
	bool Close ( void )								{  return ( m_File.Close() );  }
	FileSys::File& GetFile( void )					{  return ( m_File );  }

	// overrides
	virtual bool OutputString( const Context* context, const char* text, int len,
							   bool startLine, bool endLine );

	// factory
	static Sink* CreateSink( const char* name, const char* type, const char* fileName );

	// utility
	static void OutputGenericHeader( Context* context, const char* categoryName, bool forErrors = false );

private:
	FileSys::File m_File;
	bool          m_OwnsHandle;

	SET_NO_COPYING( FileSink );
};

//////////////////////////////////////////////////////////////////////////////
// class RetryFileSink declaration

// TARGET

class RetryFileSink : public StreamSink
{
public:
	SET_INHERITED( RetryFileSink, StreamSink );

	// ctor/dtor
	RetryFileSink( const char* name = NULL );
	RetryFileSink( const char* fileName, bool truncate, const char* name = NULL );
	virtual ~RetryFileSink( void );

	// sink-specific functions
	void Open  ( const char* fileName, bool truncate = GP_RETAIL );
	bool IsOpen( void ) const													{  return ( m_File.IsOpen() );  }
	bool Close ( void )															{  m_FileName.clear();  return ( m_File.Close() );  }

	// overrides
	virtual bool OutputString( const Context* context, const char* text, int len,
							   bool startLine, bool endLine );

	// factory
	static Sink* CreateSink( const char* name, const char* type, const char* fileName );

	const gpstring& GetFileName() { return m_FileName; }

private:
	bool TryOpen( const char* fileName );

	gpstring      m_FileName;
	bool          m_Truncate;
	FileSys::File m_File;

	SET_NO_COPYING( RetryFileSink );
};

//////////////////////////////////////////////////////////////////////////////
// class StdioFileSink declaration

// TARGET

class StdioFileSink : public StreamSink
{
public:
	SET_INHERITED( StdioFileSink, StreamSink );

	// ctor/dtor
	StdioFileSink( FILE* file = NULL, const char* name = NULL );
	virtual ~StdioFileSink( void );

	// sink-specific functions
	void Open  ( FILE* file )													{  Close();  m_File = file;  }
	bool IsOpen( void ) const													{  return ( m_File != NULL );  }
	bool Close ( void );

	// overrides
	virtual bool OutputString( const Context* context, const char* text, int len,
							   bool startLine, bool endLine );

private:
	FILE* m_File;

	SET_NO_COPYING( StdioFileSink );
};

//////////////////////////////////////////////////////////////////////////////
// class FileSysStreamerSink declaration

// TARGET

class FileSysStreamerSink : public StreamSink
{
public:
	SET_INHERITED( FileSysStreamerSink, StreamSink );

	// ctor/dtor
	FileSysStreamerSink( FileSys::StreamWriter& writer, const char* name = NULL );
	virtual ~FileSysStreamerSink( void );

	// overrides
	virtual bool OutputString( const Context* context, const char* text, int len,
							   bool startLine, bool endLine );

private:
	FileSys::StreamWriter& m_Writer;

	SET_NO_COPYING( FileSysStreamerSink );
};

//////////////////////////////////////////////////////////////////////////////
// class Win32ConsoleSink declaration

// TARGET

class Win32ConsoleSink : public FileSink
{
public:
	SET_INHERITED( Win32ConsoleSink, FileSink );

	Win32ConsoleSink( const char* name = NULL );
	virtual ~Win32ConsoleSink( void );

	virtual bool OutputString( const Context* context, const char* text, int len,
							   bool startLine, bool endLine );

private:
	HANDLE m_Console;

	SET_NO_COPYING( Win32ConsoleSink );
};

//////////////////////////////////////////////////////////////////////////////
// class DebuggerSink declaration

// TARGET

class DebuggerSink : public StreamSink
{
public:
	SET_INHERITED( DebuggerSink, StreamSink );

	DebuggerSink( const char* name = NULL );
	virtual ~DebuggerSink( void );

	virtual bool OutputString( const Context* context, const char* text, int len,
							   bool startLine, bool endLine );

	// factory
	static Sink* CreateSink       ( const char* name, const char* type, const char* unused );
	static void  RegisterFactory  ( void );
	static void  UnregisterFactory( void );

private:
	SET_NO_COPYING( DebuggerSink );
};

//////////////////////////////////////////////////////////////////////////////
// class StringSink declaration

// TARGET

class StringSink : public StreamSink
{
public:
	SET_INHERITED( StringSink, StreamSink );

	// ctor/dtor
	StringSink( const char* name = NULL );
	StringSink( gpstring& out, const char* name = NULL );
	virtual ~StringSink( void );

	// sink-specific functions
	const gpstring& GetBuffer  ( void ) const	{  return ( *m_Buffer );  }
	void            ClearBuffer( void )			{  m_Buffer->clear();  }

	// fun operators
	operator const gpstring&   ( void ) const	{  return ( GetBuffer() );  }
	operator const char*       ( void ) const	{  return ( GetBuffer().c_str() );  }

	// overrides
	virtual bool OutputString( const Context* context, const char* text, int len,
							   bool startLine, bool endLine );

private:
	gpstring* m_Buffer;				// where we output to
	gpstring  m_LocalBuffer;		// a local buffer in case user does not provide own

	SET_NO_COPYING( StringSink );
};

//////////////////////////////////////////////////////////////////////////////
// class CallbackSink declaration

// TARGET

// note: this target will exec its callback on an endreport. probably should
//       use contexts that have the AUTOREPORT option set.

class CallbackSink : public StringSink
{
public:
	SET_INHERITED( CallbackSink, StringSink );

	typedef CBFunctor2 <const Context*, CallbackSink*> Callback;

	// ctor/dtor
	CallbackSink( Callback cb, const char* name = NULL, const char* type = NULL );
	virtual ~CallbackSink( void );

	// overrides
	virtual void OnBeginReport( const Context* context );
	virtual void OnEndReport  ( const Context* context );

	// callbacks
	static Callback MakeAssertCallback    ( void );
	static Callback MakeFatalCallback     ( void );
	static Callback MakeMessageBoxCallback( void );
	static Callback MakeErrorBoxCallback  ( void );

	// factory
	static Sink* CreateSink( const char* name, const char* type, const char* unused );

private:
	Callback m_Callback;

	SET_NO_COPYING( CallbackSink );
};

//////////////////////////////////////////////////////////////////////////////
// class TextTableSink declaration

class TextTableSink : public Sink
{
public:
	SET_INHERITED( TextTableSink, Sink );

	TextTableSink( const char* name = NULL,
				   const char* vertDelim = " | ",
				   const char* horizDelim = "-" );
	virtual ~TextTableSink( void );

	virtual void OnBeginReport( const Context* context );

	virtual bool OutputString( const Context* context, const char* text, int len,
							   bool startLine, bool endLine );
	virtual bool OutputField( const Context* context, const char* text, int len );

	int  CalcFrontPadWidth( void ) const;
	int  CalcDividerWidth ( int frontPad ) const;
	int  CalcDividerWidth ( void ) const;
	void OutputReport     ( ContextRef ctx = NULL, int maxLines = -1 );
	void Sort             ( int column = 0, bool forward = true );
	void Clear            ( void );

private:
	typedef std::vector <gpstring> Row;
	typedef std::vector <Row*> Table;
	typedef std::vector <int> IntVec;

	struct CompareRows
	{
		int  m_Column;
		bool m_Forward;

		CompareRows( int column, bool forward )
		{
			m_Column = column;
			m_Forward = forward;
		}

		bool operator () ( const Row* l, const Row* r ) const
		{
			int rc = (*l)[ m_Column ].compare_no_case( (*r)[ m_Column ] );
			return ( m_Forward ? (rc < 0) : (rc > 0) );
		}
	};

	void OutputRow( Context* ctx, int frontPad, Row* row );

	// spec
	gpstring m_VertDelim;
	gpstring m_HorizDelim;

	// data
	Row    m_Header;
	Table  m_Table;
	IntVec m_Widths;

	SET_NO_COPYING( TextTableSink );
};

//////////////////////////////////////////////////////////////////////////////
// class LogSinkFilter <T> declaration

template <typename T>
class LogSinkFilter : public T
{
public:
	SET_INHERITED( LogSinkFilter, T );

	LogSinkFilter( void )						{  m_FirstTime = true;  m_UsePrefix = false;  }
	LogSinkFilter( const char* name )			: Inherited( name )  {  m_FirstTime = true;  m_UsePrefix = false;  }
	virtual ~LogSinkFilter( void )				{  }

	void EnablePrefix( bool enable = true )		{  m_EnablePrefix = enable;  }
	void DisablePrefix( void )					{  EnablePrefix( false );  }

	virtual bool OutputString( const ReportSys::Context* context, const char* text, int len, bool startLine, bool endLine )
	{
		// special first-time report
		if ( m_FirstTime )
		{
			m_FirstTime = false;
			m_StartTime = GetSystemSeconds();

			ReportSys::LocalContext ctx( this, false );
			OnFirstReport( &ctx, context );

			m_UsePrefix = m_EnablePrefix;
		}

		// prepend time on a new line
		if ( startLine && m_UsePrefix )
		{
			// get time elements
			float seconds = (float)(GetSystemSeconds() - m_StartTime);
			int minutes, hours;
			ConvertTime( hours, minutes, seconds );

			// build a prefix string
			gpstring prefix;
			prefix.assignf( "+%02d:%02d:%06.3f - ", hours, minutes, seconds );

			// output it as start of line str
			Inherited::OutputString( NULL, prefix, prefix.length(), true, false );
		}

		// pass along
		return ( Inherited::OutputString( context, text, len, startLine, endLine ) );
	}

	virtual void OnFirstReport( ReportSys::Context* ctx, const ReportSys::Context* /*actualContext*/ )
	{
		ctx->OutputF( "\n--- BEGIN SESSION ---\n\n" );
	}

private:
	bool m_FirstTime;
	bool m_EnablePrefix;
	bool m_UsePrefix;
	double m_StartTime;

	SET_NO_COPYING( LogSinkFilter );
};

//////////////////////////////////////////////////////////////////////////////
// class LogFileSink <T> declaration

gpstring MakeLogFileName( const char* fileName );

template <typename T = RetryFileSink>
class LogFileSink : public LogSinkFilter <T>
{
public:
	SET_INHERITED( LogFileSink <T>, LogSinkFilter <T> );

	LogFileSink( const char* fileName, bool truncate = GP_RETAIL, const char* name = NULL )
		: Inherited( name )  {  Open( MakeLogFileName( fileName ), truncate );  }
	virtual ~LogFileSink( void )
		{  }
	virtual void OnFirstReport( ReportSys::Context* ctx, const ReportSys::Context* actualContext )
		{  FileSink::OutputGenericHeader( ctx, actualContext->GetName() );  }

private:
	SET_NO_COPYING( LogFileSink );
};

//////////////////////////////////////////////////////////////////////////////
// class Context declaration

class Context
{
public:
	SET_NO_INHERITED( Context );

// Types.

	enum  {  DEFAULT_INDENT_SPACES = 4  };

	enum eOptions
	{
		OPTION_AUTOREPORT = 1 << 0,					// automatically wrap output calls with Begin/End report calls
		OPTION_NOGLOBAL   = 1 << 1,					// don't include the global sink by default
		OPTION_LOCALIZE   = 1 << 2,					// this context should be 100% localized text
		OPTION_IGNORETABS = 1 << 3,					// do not reprocess tabs, leave them as they are
		OPTION_NOTHREADS  = 1 << 4,					// don't worry about thread safety
		// ...

		OPTION_NONE = 0,							// disable all options
	};

	enum eTraits
	{
		TRAIT_DEBUG     = 1 << 0,					// this is a debug-only context
		TRAIT_DEV       = 1 << 1,					// this is a dev-only (non-retail) context
		TRAIT_WARNING   = 1 << 2,					// warning context of some kind
		TRAIT_ERROR     = 1 << 3,					// error context of some kind
		// ...

		TRAIT_NONE = 0,								// no special traits
	};

// Setup.

	// ctor/dtor
	Context( Context* prototype, const char* name = NULL, const char* type = NULL, eTraits traits = TRAIT_NONE, bool isLocal = false );
	Context( Sink* sink, bool owned, const char* name = NULL, const char* type = NULL, eTraits traits = TRAIT_NONE, bool isLocal = false );
	Context( const char* name = NULL, const char* type = NULL, eTraits traits = TRAIT_NONE, bool isLocal = false );
   ~Context( void );

	// local options and traits
	void     SetOptions  ( eOptions options, bool set = true );
	eOptions GetOptions  ( void ) const							{  return ( m_Options );  }
	void     ClearOptions( eOptions options )					{  SetOptions( options, false );  }
	bool     TestOptions ( eOptions options ) const				{  return ( (m_Options & options) != 0 );  }
	void     SetTraits   ( eTraits traits, bool set = true );
	eTraits  GetTraits   ( void ) const							{  return ( m_Traits );  }
	void     ClearTraits ( eTraits traits )						{  SetTraits( traits, false );  }
	bool     TestTraits  ( eTraits traits ) const				{  return ( (m_Traits & traits) != 0 );  }

	// id
	int GetId( void ) const										{  return ( m_Id );  }

	// sink
	void  SetSink  ( Sink* sink, bool owned, bool autoDeleteOld = true );
	Sink* GetSink  ( void ) const								{  return ( m_Sink );  }
	bool  OwnsSink ( void ) const								{  return ( m_OwnsSink );  }
	void  HookSink ( Sink* hook, bool owned );
	void  ClearSink( void );

	// child sinks
	void  AddSink   ( Sink* sink, bool owned,
					  bool pushBack = true )					{  GetSink()->AddSink( sink, owned, pushBack );  }
	void  RemoveSink( Sink* sink, bool delIfOwned = true )		{  GetSink()->RemoveSink( sink, delIfOwned );  }

	// query
	Sink* FindSinkByType( const char* type );

	// enabling
FEX	void Enable   ( bool enable = true )			{  m_Enabled = enable;  }
FEX	void Disable  ( void )							{  Enable( false );  }
FEX	void Toggle   ( void )							{  Enable( !IsEnabled() );  }
FEX	bool IsEnabled( void ) const					{  return ( m_Enabled && (m_Sink != NULL) && m_Sink->IsEnabled() && (TestOptions( OPTION_NOGLOBAL ) || gGlobalSink.IsEnabled() ) );  }

	// report count
	int  GetReportCount  ( void ) const							{  return ( m_ReportCount );  }
	void ResetReportCount( void )								{  m_ReportCount = 0;  }

	// name
	void            SetName         ( const char* name );
	void            SetGeneratedName( void )					{  SetName( GenerateName() );  }
	static gpstring GenerateName    ( void );
	const gpstring& GetName         ( void ) const				{  return ( m_Name );  }
	void            SetType         ( const char* type );
	const gpstring& GetType         ( void ) const				{  return ( m_Type );  }

// Formatting attributes.

	// schema
	void    SetSchema    ( Schema* schema, bool owned );
	Schema* GetSchema    ( void ) const							{  return ( m_Schema );  }
	void    ClearSchema  ( void );
	bool    HasSchema    ( void ) const							{  return ( m_Schema != NULL );  }
	int     GetFieldIndex( void ) const							{  return ( m_FieldIndex );  }
	int     GetFieldCount( void ) const;

	// table bracket
	void BeginTable ( void );
	void EndTable   ( void );
	bool IsInTable  ( void ) const								{  return ( m_InTable );  }
	int  GetRowIndex( void ) const								{  return ( m_RowIndex );  }

	// reporting
	FUBI_EXPORT void BeginReport   ( void );
	FUBI_EXPORT void EndReport     ( void );
	FUBI_EXPORT bool IsInReport    ( void ) const				{  return ( m_ReportLevel > 0 );  }
	            int  GetReportLevel( void ) const				{  return ( m_ReportLevel );  }
	            bool IsLineOpen    ( void ) const				{  return ( m_Column != 1 );  }

	// indentation
	FUBI_EXPORT void Indent ( void )							{  SetIndent( m_Indent + 1 );  }
	FUBI_EXPORT void Outdent( void )							{  SetIndent( m_Indent - 1 );  }
	            void SetIndent      ( int indent );
	            void SetIndentSpaces( int size );
	            int  GetIndent      ( void ) const				{  return ( m_Indent );  }
	            int  GetIndentSpaces( void ) const				{  return ( m_IndentSpaces );  }

	// color - note that it's in RGBA format
	void  SetColor       ( DWORD color );
	DWORD GetColor       ( void ) const							{  return ( m_Color );  }
	void  SetColorDefault( void );
	bool  IsColorDefault ( void ) const							{  return ( m_ColorDefault );  }

	// location
	void        SetLocation    ( const char* file, int ln )		{  m_LocationFile = file;  m_LocationLine = ln;  }
	void        ResetLocation  ( void )							{  m_LocationFile = NULL;  m_LocationLine = 0;  }
	const char* GetLocationFile( void ) const					{  return ( m_LocationFile );  }
	int         GetLocationLine( void ) const					{  return ( m_LocationLine );  }

// Output.

	// generic string output
	FUBI_EXPORT bool OutputRaw( const char* data, int len );
	FUBI_EXPORT bool OutputRaw( const char* data )			    {  return ( OutputRaw( data, -1 ) );  }
	FUBI_EXPORT bool Output   ( const char* data, int len );
	FUBI_EXPORT bool Output   ( const char* data )              {  return ( Output( data, -1 ) );  }
	FUBI_EXPORT bool Output   ( const gpstring& data )		    {  return ( Output( data, data.length() ) );  }
	FUBI_EXPORT bool OutputF  ( const char* format, ... )		{  return ( OutputArgs( format, va_args( format ) ) );  }
	FUBI_EXPORT bool OutputEol( void )						    {  return ( Output( "\n" ) );  }

	bool OutputArgs( const char* format, va_list args );

	// $ note that field output is similar to raw output in that it does not
	//   perform any automatic conversions: translation, reporting, tabs to
	//   spaces, breaking up lines, etc. all of these must be done manually for
	//   optimization purposes (99% of the time it's not needed).

	// table-based output
	bool OutputField    ( const char* data, int len = -1 );
	bool OutputField    ( const gpstring& data )				{  return ( OutputField( data, data.length() ) );  }
	bool OutputFieldF   ( const char* format, ... )				{  return ( OutputFieldArgs( format, va_args( format ) ) );  }
	bool OutputFieldArgs( const char* format, va_list args );

	// templatized helpers
	template <typename T>
	bool OutputFieldObj( const T& obj, FuBi::eXfer xfer = FuBi::XFER_NORMAL )
		{  return ( OutputField( ::ToString( obj, xfer ) ) );  }

// Specialized output.

	// format helpers
	bool OutputRepeatCount( int repeatCount, const char* text, int len = -1 );
	bool OutputRepeatWidth( int width, const char* text, int len = -1 );

private:
	bool OutputPrivate( const char* data, int len );
	bool AddReport( bool fromBegin = false );
	void Init( eTraits traits, Context* prototype, bool isLocal );
	void DestroySink( bool autoDeleteOld = true );

	struct PrivateAutoReport
	{
		Context* m_Context;
		bool     m_PrivateAutoReport;

		PrivateAutoReport( Context* context )
			: m_Context( context )
		{
			gpassert( m_Context != NULL );
			m_PrivateAutoReport = m_Context->TestOptions( OPTION_AUTOREPORT );
			if ( m_PrivateAutoReport )
			{
				m_Context->BeginReport();
			}
		}

	   ~PrivateAutoReport( void )
		{
			if ( m_PrivateAutoReport )
			{
				m_Context->EndReport();
			}
		}
	};

	// formatting
	Schema*     m_Schema;				// current schema definition if any
	bool        m_OwnsSchema;			// true if we own the schema
	int         m_Indent;				// current indentation level (leading spaces per line)
	int         m_IndentSpaces;			// spaces at each indentation level
	DWORD       m_Color;				// current color to draw with
	bool        m_ColorDefault;			// whether or not current color is default
	const char* m_LocationFile;			// __FILE__ or other (doesn't matter - just a unique id helper)
	int         m_LocationLine;			// __LINE__ or other

	// state
	eOptions    m_Options;				// general options for this context
	eTraits     m_Traits;				// traits for this context
	int         m_Id;					// id of this context
	bool        m_Enabled;				// global enabling for this context
	int         m_Column;				// > 1 if line was not terminated and is still open - next column to write to (1-based)
	bool        m_InTable;				// true if we're in a closed table
	int         m_RowIndex;				// index of next row
	int         m_ReportLevel;			// depth of report
	int         m_ReportCount;			// number of reports made
	int         m_FieldIndex;			// index of next field
	gpstring    m_Name;					// name of this context (used for global registration)
	gpstring    m_Type;					// general type of this sink

	// output
	Sink*       m_Sink;					// referenced sink we're outputting to
	bool        m_OwnsSink;				// true if we own the sink

	// statics
	static int   ms_NextId;				// next context id

	SET_NO_COPYING( Context );
};

MAKE_ENUM_BIT_OPERATORS( Context::eOptions );
MAKE_ENUM_BIT_OPERATORS( Context::eTraits );

gpstring ToString( Context::eOptions options, const char* delim = " | " );
gpstring ToString( Context::eTraits traits, const char* delim = " | " );

//////////////////////////////////////////////////////////////////////////////
// class LocalContext declaration

class LocalContext : public Context
{
public:
	SET_INHERITED( LocalContext, Context );

	// ctor/dtor
	LocalContext( Context* prototype, const char* name = NULL, const char* type = NULL, eTraits traits = TRAIT_NONE )
		: Inherited( prototype, name, type, traits, true )  {  }
	LocalContext( Sink* sink, bool owned, const char* name = NULL, const char* type = NULL, eTraits traits = TRAIT_NONE )
		: Inherited( sink, owned, name, type, traits, true )  {  }
	LocalContext( const char* name = NULL, const char* type = NULL, eTraits traits = TRAIT_NONE )
		: Inherited( name, type, traits, true )  {  }
   ~LocalContext( void )
		{  }

private:
	SET_NO_COPYING( LocalContext );
};

//////////////////////////////////////////////////////////////////////////////
// class Mgr declaration

class Mgr : public OnDemandSingleton <Mgr>
{
public:
	SET_NO_INHERITED( Mgr );

// Ctor/dtor.

	Mgr( void );
   ~Mgr( void );

// Global sink registry.

	// registration
	void  RegisterSink  ( const char* name, Sink* sink );
	Sink* FindSink      ( const char* name ) const;
	void  UnregisterSink( const char* name );
	void  UnregisterSink( Sink* sink );

// Sink factory.

	// registration
	void RegisterSinkFactory  ( const char* type, SinkFactoryProc proc );
	void UnregisterSinkFactory( const char* type );

	// safe enabling
	void EnableSink( const char* name, bool enable = true );

	// factory
	Sink* CreateSink( const char* name, const char* type, const char* params = "" );

// Global context registry.

	// registration
	void     RegisterContext  ( const char* name, Context* context );
	Context* FindContext      ( const char* name ) const;
	void     UnregisterContext( const char* name );
	void     UnregisterContext( Context* context );

	// safe enabling
	void EnableContext( const char* name, bool enable = true );

	typedef std::vector <Context*> ContextColl;
	typedef std::vector <Sink*>    SinkColl;

	// database query
	int QueryContextByTraits( ContextColl& coll, Context::eTraits traits );
	int QueryContextByName  ( ContextColl& coll, const char* nameWildcard );
	int QueryContextByType  ( ContextColl& coll, const char* typeWildcard );
	int QuerySinkByName     ( SinkColl& coll, const char* nameWildcard );
	int QuerySinkByType     ( SinkColl& coll, const char* typeWildcard );

// Localization support.

	// type
	typedef std::map <gpstring, gpstring, string_less> TranslateMap;

	// translation
	const char*   TranslateA     ( const char* seaText );
	gpwstring     TranslateW     ( const char* seaText );
	gpwstring     TranslateW     ( const wchar_t* seaText );
	TranslateMap& GetTranslateMap( void )					{  return ( m_TranslateMap );  }

private:

	// simple two-way map of name-to-object-pointer
	template <typename T>
	struct Registry
	{
		void Register( const char* name, T* t )
		{
			gpassert( t != NULL );
			gpassert( (name != NULL) && (*name != '\0') );

			gpstring localName( name );
			gpassert( m_ForwardMap.find( localName ) == m_ForwardMap.end() );

			m_ForwardMap[ localName ] = t;
			m_ReverseMap[ t ] = localName;
		}

		T* Find( const char* name ) const
		{
			gpassert( (name != NULL) && (*name != '\0') );

			ForwardMap::const_iterator found = m_ForwardMap.find( name );
			return ( (found == m_ForwardMap.end()) ? NULL : found->second );
		}

		void Unregister( const char* name )
		{
			gpassert( (name != NULL) && (*name != '\0') );

			ForwardMap::iterator ffound = m_ForwardMap.find( name );
			gpassert( ffound != m_ForwardMap.end() );

			ReverseMap::iterator rfound = m_ReverseMap.find( ffound->second );
			gpassert( rfound != m_ReverseMap.end() );

			m_ForwardMap.erase( ffound );
			m_ReverseMap.erase( rfound );
		}

		void Unregister( T* t )
		{
			gpassert( t != NULL );

			// ok for it not to exist
			ReverseMap::iterator rfound = m_ReverseMap.find( t );
			if ( rfound != m_ReverseMap.end() )
			{
				ForwardMap::iterator ffound = m_ForwardMap.find( rfound->second );
				gpassert( ffound != m_ForwardMap.end() );

				m_ForwardMap.erase( ffound );
				m_ReverseMap.erase( rfound );
			}
		}

		typedef std::map <gpstring, T*, istring_less> ForwardMap;
		typedef std::map <T*, gpstring>               ReverseMap;

		ForwardMap m_ForwardMap;
		ReverseMap m_ReverseMap;
	};

	// types
	typedef Registry <Sink> SinkMap;
	typedef std::map <gpstring, SinkFactoryProc, istring_less> SinkFactoryProcMap;
	typedef Registry <Context> ContextMap;

	// state
	bool m_RegisteredDefaults;

	// collections
	SinkMap            m_SinkMap;
	SinkFactoryProcMap m_SinkFactoryProcMap;
	ContextMap         m_ContextMap;
	TranslateMap       m_TranslateMap;

	SET_NO_COPYING( Mgr );
};

#define gReportSysMgr ReportSys::Mgr::GetSingleton()

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace ReportSys

#endif  // __REPORTSYS_H

//////////////////////////////////////////////////////////////////////////////
