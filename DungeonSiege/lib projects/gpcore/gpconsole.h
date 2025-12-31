/*=======================================================================================

  GpConsole

  purpose:	This is a generic 'console' class for processing text input.
			This class has no visual component to it, it merely just takes
			an input string, processes it, and provides an output string.

			class GpConsole
				works on:
			class GpConsole_command
				which takes as parameters:
			class GpConsole_command_arguments

			The input string is processed if it begins with a keyword which
			is recognized by the GpConsole to be a console command.

			Console commands can be dynamically added or removed from the
			GpConsole's repertoire.  You can create your own console commands
			by deriving from GpConsole_command.  See the implementation
			file and the gpc_help class as an example.  The console commands
			are automatically registered with the console on creation and
			unregistered on destruction.


			Convention for adding commands:

			Instead of cramming all the console commands we might possibly
			want into a single implementation file, console command are
			declared and defined locally in the scope with which they
			operate.  Meaning, if I want to add a command which works on
			the Game Object Data Base, I add it in the godb.cpp file.  If
			I'm adding a command which is related to object selection, I'll
			most likely add it to the UIWorkspace.  Etc.  Since GpConsole_command
			automatically registers itself with GpConsole on construction and
			deregisters itself on destruction, I'm going to take advantage
			of this by creating the GpConsole_commands in the constuctors
			of the objects mentioned before (UIWorkspace etc.)  This provides
			convenience and more safety, since a console command becomes
			available only after the object it works with is constructed.
			Likewise console commands magically will go away when that
			owning object is destroyed.  Umm... if this doesn't make any
			sense, just give me a shout.

  author:	Bartosz Kijanka

  (C)opyright Gas Powered Games 1999

---------------------------------------------------------------------------------------*/

#pragma once
#ifndef __GPCONSOLE_H
#define __GPCONSOLE_H

#if !GP_RETAIL

#include "BlindCallback.h"
#include "FuBiDefs.h"

#include <vector>
#include <map>
#include <list>

class GpConsole_command;
class GpConsole_command_arguments;

class GpConsole : public Singleton <GpConsole>
{
public:

	// each command has some info assosiated with it
	typedef struct commandinfo {
		gpstring name;
		gpstring help;
		std::vector<gpstring> usage;
		GpConsole_command * commandptr;
	} COMMANDINFO_DEF;

	// map command name to it's 'info' struct - map from command to info
	typedef std::map< gpstring, COMMANDINFO_DEF, istring_less > commandlist;

	// console history - use this for a FILO
	typedef std::list< gpstring > historylist;
	unsigned int mHistorySizeMax;

	// command usage examples - map from usage to command
	typedef std::map< gpstring, gpstring, istring_less > usagelist;

	// callback for console command completion
	typedef CBFunctor0 CompletionCb;

	// callback for cheat mirroring
	typedef CBFunctor1< const gpstring& > CheatMirrorCb;

public:

	//----- existence
	GpConsole();
	~GpConsole();

	void SetAutoComplete( bool flag )	{ m_bAutoComplete = flag; }
	bool GetAutoComplete() const		{ return m_bAutoComplete; }
	void SetShowHelp( bool flag )		{ m_bShowHelp = flag; }
	bool GetShowHelp() const			{ return m_bShowHelp; }

	//----- input/output

	bool CompoundInputLine( char add, gpstring & console_output );
	void ClearInputLine() { mInputLine.clear(); mInputLineMatch.clear(); }
	gpstring GetInputLine() { return( mInputLine ); }
	gpstring GetInputLineMatch() { return( mInputLineMatch ); }
	void SetInputLineToHistoryIndex( int iIndex );
	bool ProcessInputLine( gpstring & output );

	bool Process( gpstring const & line, gpstring & console_output, bool addToHistory = true );

	FUBI_EXPORT bool Execute( const gpstring& line );

	void AddCompletionCb( CompletionCb cb, int commandCount = 1 );
	void SetCheatMirrorCb( CheatMirrorCb cb ) { mCheatMirrorCb = cb; }

	// get list of commands console knows about
	void ListCommands( gpstring & output );
	void GetCommandHelp( gpstring const & commandname, gpstring & output );
	unsigned int GetHistorySize() { return( mHistoryList.size() ); }
	void SetInputLineToCompleteCommand();
	void SetInputLineToLastHistory();
	void SetInputLineToNextHistory();
	void SetInputLineToPreviousCommand();
	void SetInputLineToNextCommand();

	//----- command completion etc.
	gpstring GetClosestMatch( gpstring matchwith, bool bCursorOn = false );	// axe?

	// add a command to the repertoire
	bool RegisterCommand( GpConsole_command & command, gpstring name, gpstring help, std::vector<gpstring> & usage );
	// remove a command from repertoire
	void UnregisterCommand( GpConsole_command & command );
	// is particular command in repertoire?
	bool ContainsCommand( gpstring const & name );

	// used for getting commands near last entered
	gpstring GetNextCommand( void );					// axe?
	gpstring GetPreviousCommand( void );				// axe?


protected:

	typedef std::pair <int, CompletionCb> CompletionPair;
	typedef std::vector <CompletionPair> CompletionColl;

	friend class gpc_help;

	bool m_bAutoComplete;
	bool m_bShowHelp;

	gpstring mInputLine;
	gpstring mInputLineMatch;

	// tables
	commandlist mCommandList;
	historylist mHistoryList;
	usagelist	mUsageList;

	int iCurrentCommand;
	int mHistoryIndex;

	CompletionColl mCompletionColl;
	CheatMirrorCb mCheatMirrorCb;

	// here we will register any commands the console 'comes with'
	// out of the box
	std::auto_ptr< GpConsole_command > mCommand_help;
	std::auto_ptr< GpConsole_command > mCommand_history;

	FUBI_SINGLETON_CLASS( GpConsole, "The development console" );
	SET_NO_COPYING( GpConsole );
};

#define gGpConsole GpConsole::GetSingleton()


//================================================================
//
// GpConsole_command_arguments
//
//----------------------------------------------------------------
class GpConsole_command_arguments
{
public:

	GpConsole_command_arguments( gpstring const & inputline );

	~GpConsole_command_arguments(){}

	unsigned int Size();									// get argument count

	gpstring GetAsString( unsigned int arg );	        	// return argument N as string

	UINT32 GetAsInt( unsigned int arg );					// return argument N as int

	float GetAsFloat( unsigned int arg );					// return argument N as float

	double GetAsDouble( unsigned int arg );					// return argument N as double

private:

	gpstring mLine;

	std::vector< gpstring > mArguments;
};








//================================================================
//
// GpConsole_command
//
//----------------------------------------------------------------
class GpConsole_command
{
public:

	// register derived clas ourselves with the console through the constructor
	GpConsole_command();
	virtual ~GpConsole_command();					// unregister ourselves with the console

	virtual UINT32 GetFlags()				= 0;	// get any general purpose flags
	virtual gpstring GetLocation()			= 0;	// debug : get console command location in terms of __FILE__ and __LINE__

	// execute command, given the arguments, output any output to the output string
	virtual bool Execute( GpConsole_command_arguments & args, gpstring & output ) = 0;

	virtual unsigned int GetMinArguments()	= 0;	// min arguments the command requires : 1=default, meaning just the command name
	virtual unsigned int GetMaxArguments()	= 0;	// max arguments command can take
};


#endif // !GP_RETAIL


#endif

