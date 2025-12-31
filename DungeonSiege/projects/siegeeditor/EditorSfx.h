//////////////////////////////////////////////////////////////////////////////
//
// File     :  EditorSfx.h
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////



#pragma once
#ifndef __EDITORSFX_H
#define __EDITORSFX_H


struct EffectScript
{

};


// Command Structures
struct Argument
{
	int				order;
	gpstring		sType;
	gpstring		sDoc;
	StringVec		values;
};

typedef std::vector< Argument > ArgumentVec;

struct SubCommand;
typedef std::vector< SubCommand > SubCommandVec;

struct SubCommand
{
	gpstring		sName;
	gpstring		sDoc;
	ArgumentVec		arguments;
	SubCommandVec	subCommands;
};

struct Command
{
	gpstring		sName;
	gpstring		sDoc;
	ArgumentVec		arguments;
	SubCommandVec	subCommands;
};

typedef std::vector< Command > CommandVec;


struct EffectParameter
{
	gpstring sParameter;
	gpstring sType;
	gpstring sDoc;
};

typedef std::vector< EffectParameter > EffectParameterVec;

// Effect Parameters ( for sfx command )
struct EffectParameters
{
	gpstring			sEffectName;
	EffectParameterVec	parameters;	
	gpstring			sDoc;
};

typedef std::vector< EffectParameters > EffectParametersVec;



// This holds all of our effect schema data
class EffectSchemaDb
{
public:

	EffectSchemaDb() {};
	~EffectSchemaDb() {}

	void Init();
	SubCommandVec RecursiveLoadSubCommand( FuelHandle hSubCommand );

	void GetCommands( CommandVec & commands )						{ commands = m_commands; }
	void GetMacros( StringVec & macros )							{ macros = m_macros; }
	void GetEffectParameters( EffectParametersVec & parameters )	{ parameters = m_effectParameters; }	

private:

	CommandVec			m_commands;
	StringVec			m_macros;
	EffectParametersVec m_effectParameters;
};


class EditorSfx : public Singleton <EditorSfx>
{
public:

	EditorSfx();
	~EditorSfx() {}

	void		SetSelectedScript( gpstring sName ) { m_selectedScript = sName; }
	gpstring	GetSelectedScript()					{ return m_selectedScript;	}

	FuelHandle	GetSelectedScriptHandle();

	EffectSchemaDb * GetEffectSchemaDb() { return &m_effectSchemaDb; }

	gpstring	GetCurrentEffectParams()					{ return m_currEffectParams;	}
	void		SetCurrentEffectParams( gpstring sParams )	{ m_currEffectParams = sParams; }

	gpstring	GetCurrentEffect()						{ return m_currEffect;		}
	void		SetCurrentEffect( gpstring sEffect )	{ m_currEffect = sEffect;	}

	bool		IsEffect( gpstring sEffect );

private:

	gpstring		m_selectedScript;
	gpstring		m_currEffectParams;
	gpstring		m_currEffect;
	gpstring		m_selectedScriptHandle;
	EffectSchemaDb	m_effectSchemaDb;
	
};

#define gEditorSfx EditorSfx::GetSingleton()


#endif 