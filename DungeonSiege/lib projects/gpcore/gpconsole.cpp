#include "precomp_gpcore.h"

#if !GP_RETAIL

#include "GpConsole.h"
#include "SkritDefs.h"
#include "StdHelp.h"
#include "StringTool.h"


//================================================================
//
// gpc_help
//
//----------------------------------------------------------------
class gpc_help : public GpConsole_command
{
public:
	gpc_help();
	~gpc_help(){}
	UINT32 GetFlags()				{ return( 0 ); }
	gpstring GetLocation()			{ return( gpstring(__FILE__) ); }
	unsigned int GetMinArguments()	{ return( 1 ); }
	unsigned int GetMaxArguments()	{ return( 2 ); }

	bool Execute( GpConsole_command_arguments & args, gpstring & output );
};




gpc_help::gpc_help()
{
	gpstring help;
	std::vector< gpstring > usage;
	help = "help : brief command list\n";
	help += "help <console command> : more help about a specific command.";

	gGpConsole.RegisterCommand( *this, "help", help, usage );
}




bool gpc_help::Execute( GpConsole_command_arguments & args, gpstring & output )
{
	if( !gGpConsole.GetShowHelp() )
	{
		return true;
	}

	if( args.Size() == 2 )
	{
		gGpConsole.GetCommandHelp( args.GetAsString( 1 ), output );
	}
	else
	{
		gGpConsole.ListCommands( output );
		output += "Build time[ ";
		output += __TIME__;
		output += " : ";
		output += __DATE__;
		output += " ]";
	}
	return( true );
}



//================================================================
//
// GpConsole
//
//----------------------------------------------------------------
GpConsole::GpConsole()
{
	m_bAutoComplete = true;
	m_bShowHelp = true;

	mHistorySizeMax = 30;

	iCurrentCommand = -1;
	mHistoryIndex = -1;

	stdx::make_auto_ptr( mCommand_help, new gpc_help );
}




GpConsole::~GpConsole()
{
}




bool GpConsole::RegisterCommand( GpConsole_command & command, gpstring name, gpstring help, std::vector<gpstring> & usage )
{
	name.to_lower();

	if( !ContainsCommand( name ) )
	{

		COMMANDINFO_DEF info;

		info.name		= name;
		info.help		= help;
		info.commandptr = &command;

		for( size_t i=0; i<usage.size(); i++ )
		{
			info.usage.push_back( usage[i] );
		}

		// $$$
		//sort( info.usage.begin(), info.usage.end() );

		mCommandList.insert( commandlist::value_type( name, info ) );

		return( true );
	}
	else {
		gpassertf( 0, ( "GPConsole command '%s' re-definition.  Ignoring new definition.", name.c_str() ) );
		return( false );
	}
}




void GpConsole::UnregisterCommand( GpConsole_command & command )
{
	bool removed = false;

	commandlist::iterator i;
	for( i=mCommandList.begin(); i!=mCommandList.end(); ++i ) {
		if( (*i).second.commandptr == &command ) {
			mCommandList.erase( i );
			removed = true;
		}
	}
	gpassert( removed );
}




bool GpConsole::ContainsCommand( gpstring const & name )
{
	commandlist::iterator found = mCommandList.find( name );
	if( found == mCommandList.end() ) {
		return( false );
	}
	else {
		return( true );
	}
}




void GpConsole::ListCommands( gpstring & output )
{
	commandlist::iterator i = mCommandList.begin();
	for( i = mCommandList.begin(); i!=mCommandList.end(); ++i ) {
		output += (*i).first;
		output += "\n";
	}
}




void GpConsole::GetCommandHelp( gpstring const & commandname, gpstring & output )
{
	GpConsole::commandlist::iterator i = mCommandList.find( commandname );
	if( i != mCommandList.end() ) {
		output += (*i).second.help;
	}
}

void GpConsole::SetInputLineToHistoryIndex( int iIndex )
{
	if( (iIndex < 0) || mHistoryList.empty() )
		mInputLine.clear();
	else if( iIndex >= scast <int> ( mHistoryList.size() ) )
		mInputLine = mHistoryList.back();
	else
	{
		int iCount = -1;
		mInputLine.clear();
		historylist::iterator i;
		for( i=mHistoryList.begin(); i!=mHistoryList.end(); ++i )
		{
			iCount++;
			if( iCount == iIndex )
				mInputLine = *i;
		}
	}

	mInputLineMatch = GetClosestMatch( mInputLine, true );
}

void GpConsole::SetInputLineToLastHistory()
{
	mHistoryIndex++;
	if( mHistoryIndex >= scast <int> ( GetHistorySize() ) )
		mHistoryIndex = GetHistorySize()-1;

	SetInputLineToHistoryIndex( mHistoryIndex );
}

void GpConsole::SetInputLineToNextHistory()
{
	mHistoryIndex--;
	if( mHistoryIndex < 0 )
		mHistoryIndex = -1;

	SetInputLineToHistoryIndex( mHistoryIndex );
}

void GpConsole::SetInputLineToPreviousCommand()
{
	mInputLine = GetPreviousCommand();
	mInputLineMatch = GetClosestMatch( mInputLine, true );
}

void GpConsole::SetInputLineToNextCommand()
{
	mInputLine = GetNextCommand();
	mInputLineMatch = GetClosestMatch( mInputLine, true );
}


void GpConsole::SetInputLineToCompleteCommand()
{
	mInputLine = GetClosestMatch( mInputLine, false );
	mInputLineMatch = GetClosestMatch( mInputLine, true );
}

bool GpConsole::CompoundInputLine( char add, gpstring & console_output )
{
	bool processed = false;
	if( add == '\n' ) {
		processed = ProcessInputLine( console_output );
	}
	else if( add == '\b' ) {
		if( !mInputLine.empty() )
		{
			mInputLine.erase( mInputLine.end_split() - 1, mInputLine.end_split() );
			if( m_bAutoComplete )
			{
				mInputLineMatch = GetClosestMatch( mInputLine, true );
			}
		}
	}
	else
	{
		mInputLine += add;
		if( m_bAutoComplete )
		{
			mInputLineMatch = GetClosestMatch( mInputLine, true );
		}
		else
		{
			mInputLineMatch = mInputLine;
		}
	}
	// calculate closest match
	return( processed );
}


bool GpConsole::ProcessInputLine( gpstring & output )
{
	bool processed = false;

	if( m_bAutoComplete )
	{
		mInputLineMatch = GetClosestMatch( mInputLine, false );
		output += mInputLineMatch;
	}

	output += "\n";
	
	if( m_bAutoComplete )
	{
		processed = Process( mInputLineMatch, output );
	}
	else
	{
		processed = Process( mInputLine, output );
	}

	mInputLine.clear();
	mInputLineMatch.clear();

	return( processed );
}


bool GpConsole::Process( const gpstring & line, gpstring & output, bool addToHistory )
{
	bool success = false;

	// bail if nothing to do
	if ( line.find_first_not_of( stringtool::ALL_WHITESPACE ) == gpstring::npos )
	{
		return ( true );
	}

	// out line to debug window
	gpdebuggerf(( "Command: %s\n", line.c_str() ));

	// save in history
	if ( addToHistory )
	{
		mHistoryIndex = -1;
		mHistoryList.push_front( line );
		if ( mHistoryList.size() > mHistorySizeMax )
		{
			mHistoryList.pop_back();
		}
	}

	if ( line[ 0 ] == '/' )
	{
		// newfangled skrit console

		success = !Skrit::IsError( Skrit::ExecuteCommand( "console", line.c_str() + 1, line.length() - 1 ) );
	}
	else if ( ((line[ 0 ] == '+') || (line[ 0 ] == '-')) && mCheatMirrorCb )
	{
		// send cheats out to the game console (mirrored here for convenience)

		mCheatMirrorCb( line );
	}
	else
	{
		// legacy console

		GpConsole_command_arguments arg( line );

		if ( arg.Size() > 0 )
		{
			commandlist::iterator found = mCommandList.find( arg.GetAsString( 0 ) );
			if ( found != mCommandList.end() )
			{
				// check for correct number of arguments
				unsigned int min = found->second.commandptr->GetMinArguments();
				unsigned int max = found->second.commandptr->GetMaxArguments();

				if ( (arg.Size() >= min) && (arg.Size() <= max) )
				{
					(found->second.commandptr->Execute)( arg, output );
					success = true;
				}
				else
				{
					output += "# error: incorrect number of arguments.\n";
				}
			}
		}
	}

	CompletionColl::iterator i, ibegin = mCompletionColl.begin(), iend = mCompletionColl.end();
	for ( i = ibegin ; i != iend ; )
	{
		if ( --i->first == 0 )
		{
			i->second();
			i = mCompletionColl.erase( i );
		}
		else
		{
			++i;
		}
	}

	return ( success );
}


bool GpConsole::Execute( const gpstring& line )
{
	gpstring output;
	bool rc = true;

	if ( line[ 0 ] == '/' )
	{
		// easy
		rc = Process( line, output, false );
		if ( !output.empty() )
		{
			output += '\n';
		}
	}
	else
	{
		stringtool::Extractor extractor( ";", line );
		gpstring command;
		while ( extractor.GetNextString( command ) )
		{
			size_t oldSize = output.length();
			if ( !Process( command, output, false ) )
			{
				rc = false;
			}
			if ( oldSize != output.length() )
			{
				output += '\n';
			}
		}
	}

	gpgeneric( output );
	return ( rc );
}


void GpConsole::AddCompletionCb( CompletionCb cb, int commandCount )
{
	gpassert( cb && (commandCount > 0) );

	mCompletionColl.push_back( std::make_pair( commandCount, cb ) );
}


gpstring GpConsole::GetClosestMatch( gpstring matchwith, bool bCursorOn )
{

	gpstring Match;
	gpstring cursor = ".";

	if( matchwith.size() == 0 )
	{
		return( matchwith );
	}
	else
	{
		int iCount = -1;
		std::vector<gpstring> matchingcommandlist;
		std::vector<gpstring> matchingusagelist;
		//Build a list of all matching strings
		commandlist::iterator iCom;
		for( iCom=mCommandList.begin(); iCom!=mCommandList.end(); ++iCom )
		{	//Run through each command trying to disprove matching
			iCount++;
			bool bMatch = true;
			for( size_t iMatch=0; iMatch<matchwith.size(); iMatch++ )
			{	//Try with command list
				if( (*iCom).second.name.size() >= iMatch )
				{
					if( matchwith[iMatch] != (*iCom).second.name[iMatch] )
						bMatch = false;
				}
				else
					bMatch = false;
			}

			if( bMatch )
			{	//Found a match
				matchingcommandlist.push_back( (*iCom).second.name );

				if( matchingcommandlist.size() == 1 )
				{	//This is the first match, save it off and add a cursor if necessary
					if( bCursorOn )
					{	//Put a cursor where the actual input ends
						gpstring tempEnd = (*iCom).second.name;
						tempEnd.erase( tempEnd.begin_split(), tempEnd.begin_split()+iMatch );
						Match = matchwith + cursor + tempEnd;
					}
					else
					{	//no cursor
						Match = (*iCom).second.name;
					}

					//Set Left/Right arrow index to matched string
					iCurrentCommand = iCount;
				}
			}
			else
			{	//Try all usage strings
				for( size_t iUsage=0; iUsage<(*iCom).second.usage.size(); iUsage++ )
				{
					bMatch = true;
					gpstring UsageString = (*iCom).second.usage[iUsage];
					for( size_t iMatchUsage=0; iMatchUsage<matchwith.size(); iMatchUsage++ )
					{	//Try with this usage string
						if( (*iCom).second.usage[iUsage].size() >= iMatchUsage )// && !( matchwith.size()-1 == iMatchUsage && matchwith[iMatchUsage] == ' ' ) )
						{
							if( matchwith[iMatchUsage] != UsageString[iMatchUsage] )
								bMatch = false;
						}
						else
							bMatch = false;
					}

					if( bMatch )
					{	//Check that the matched string doesn't contain extra spaces beyond the matching part
						for( size_t iExtra=iMatchUsage; iExtra<(*iCom).second.usage[iUsage].size(); iExtra++ )
						{
							if( (*iCom).second.usage[iUsage][iExtra] == ' ' )
								bMatch = false;
						}
					}

					if( bMatch )
					{
						matchingusagelist.push_back( (*iCom).second.usage[iUsage] );

						if( matchingusagelist.size() == 1 )
						{	//This is the first match, save it off and add a cursor if necessary
							if( bCursorOn )
							{	//Put a cursor where the actual input ends
								gpstring tempEnd = UsageString;
								tempEnd.erase( tempEnd.begin_split(), tempEnd.begin_split()+iMatchUsage );
								Match = matchwith + cursor + tempEnd;
							}
							else
							{	//no cursor
								Match = UsageString;
							}


						}
					}
				}
			}
		}

		if( matchingcommandlist.size() > 0 )
		{	//At least one command match - Add extra match info if needed
			if( bCursorOn && matchingcommandlist.size() > 1 )
			{
				Match += "  [";
				std::vector<gpstring>::iterator iMatches;
				for( iMatches=matchingcommandlist.begin(); iMatches!=matchingcommandlist.end(); ++iMatches )
				{
					Match += *iMatches;
					if( iMatches != matchingcommandlist.end()-1 )
						Match += " ";
				}
				Match += "]";
			}

			return( Match );
		}
		else if( matchingusagelist.size() > 0 )
		{	//At least one usage match - Add extra match info if needed
			if( bCursorOn && matchingusagelist.size() > 1 )
			{
				Match += "  [";
				std::vector<gpstring>::iterator iMatches;
				for( iMatches=matchingusagelist.begin(); iMatches!=matchingusagelist.end(); ++iMatches )
				{
					int iRemove = 0;
					for( size_t iChar=0; iChar<(*iMatches).size(); iChar++ )
					{	//find out how long is the first word
						if( (*iMatches)[iChar] == ' ' )
							iRemove = iChar;
					}

					//Peal off the first word and space
					(*iMatches).erase( (*iMatches).begin_split(), (*iMatches).begin_split()+iRemove+1 );

					Match += *iMatches;
					if( iMatches != matchingusagelist.end()-1 )
					{
						Match += " ";
					}
				}
				Match += "]";
			}

			return( Match );
		}
		else
		{	//No matches, just return original string - perhaps with cursor appended
			Match = matchwith;
			if( bCursorOn )
			{	//Add a cursor to the end
				Match += ".";
			}
			return( Match );
		}

	}
}




gpstring GpConsole::GetNextCommand( void )
{
	iCurrentCommand++;
	if( iCurrentCommand >= scast <int> ( mCommandList.size() ) )
		iCurrentCommand = 0;

	int iCount = 0;
	commandlist::iterator i;
	for( i=mCommandList.begin(); i!=mCommandList.end(); ++i )
	{
		if( iCount == iCurrentCommand )
			return( (*i).second.name );

		iCount++;
	}

	return( "" );
}




gpstring GpConsole::GetPreviousCommand( void )
{
	iCurrentCommand--;
	if( iCurrentCommand < 0 )
		iCurrentCommand = mCommandList.size() - 1;

	int iCount = 0;
	commandlist::iterator i;
	for( i=mCommandList.begin(); i!=mCommandList.end(); ++i )
	{
		if( iCount == iCurrentCommand )
			return( (*i).second.name );

		iCount++;
	}

	return( "" );
}



//================================================================
//
// GpConsole_command
//
//----------------------------------------------------------------
GpConsole_command::GpConsole_command() {}




GpConsole_command::~GpConsole_command()
{
	// todo: fix this... -BK
	//mConsole.UnregisterCommand( *this );
}




//================================================================
//
// GpConsole_command_arguments
//
//----------------------------------------------------------------
GpConsole_command_arguments::GpConsole_command_arguments( gpstring const & _line )
{
	gpstring line = _line;
	// append line with bit of whitespace to help keep our loop small
	line += " ";

	const static gpstring ignore(" \t");

	gpstring::size_type begin, end;

	begin = end = 0;

	while( begin != gpstring::npos )
	{
		begin	= line.find_first_not_of( ignore.c_str(), end );	// begin of token
		end		= line.find_first_of( ignore.c_str(), begin );				// start of whitespace after token

		if( begin != gpstring::npos )
		{
			gpstring token( line, begin, end - begin );
			_strlwr( (char*)token.c_str() );
			mArguments.push_back( token );
		}
	}
}




unsigned int GpConsole_command_arguments::Size()
{
	return( mArguments.size() );
}




gpstring GpConsole_command_arguments::GetAsString( unsigned int arg )
{
	if( Size() > 0 && arg < Size() ) {
		return( mArguments[arg] );
	}
	else {
//		gpassert( 0 );					// Game would exit instead of asserting... RES
		return( gpstring("") );
	}
}




UINT32 GpConsole_command_arguments::GetAsInt( unsigned int arg )
{
	gpassert( arg <= Size() );
	if( arg > Size() ) {
		arg = Size();
	}
	return( atoi( mArguments[arg].c_str() ) );
}




float GpConsole_command_arguments::GetAsFloat( unsigned int arg )
{
	return( (float)GetAsDouble( arg ) );
}



double GpConsole_command_arguments::GetAsDouble( unsigned int arg )
{
	gpassert( arg <= Size() );
	if( arg > Size() ) {
		arg = Size();
	}
	return( atof( mArguments[arg].c_str() ) );
}



#endif // !GP_RETAIL
