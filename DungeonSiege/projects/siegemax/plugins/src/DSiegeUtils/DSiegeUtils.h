#pragma once
/**********************************************************************
 *<
	FILE: DSiegeUtils.h

	DESCRIPTION:	Includes for Plugins

	CREATED BY:

	HISTORY:

 *>	Copyright (c) 2002, All Rights Reserved.
 **********************************************************************/

#ifndef __DSIEGEUTILS__H
#define __DSIEGEUTILS__H

#include "Max.h"

#include "Maxscrpt.h"
#include "Name.h"
#include "HashTab.h"
#include "Numbers.h"
#include "Strings.h"
#include "Arrays.h"
#include "3dmath.h"

#include "resource.h"
#include "istdplug.h"
#include "iparamb2.h"
#include "iparamm2.h"
#include "utilapi.h"

#include <FileSys.h>
#include <MasterFileMgr.h>
#include <TankFileMgr.h>
#include <StringTool.h>

class Config;
class NamingKey;

namespace FileSys
{
	class MasterFileMgr;
	class MultiFileMgr;
}

#define DSIEGEUTILS_CLASS_ID	Class_ID(0x1659f776, 0x2f232002)

extern TCHAR *GetString(int id);

class DSiegeUtils : public UtilityObj
{
	
public:

	//////////////////////////////////////////////////////////////////////////////
	// enum eTankType declaration

	enum eTankType
	{
		SET_BEGIN_ENUM( TANK_, 0 ),

		TANK_NONE,
		TANK_RESOURCE,
		TANK_MAP,
		TANK_MOD,

		SET_END_ENUM( TANK_ ),
	};

	const char* ToString   ( eTankType e );
	const char* ToPathVar  ( eTankType e );
	const char* ToExtension( eTankType e );
	bool        FromString ( const char* str, eTankType& e );

	//////////////////////////////////////////////////////////////////////////////
	// enum eExpErrType declaration

	enum eExpErrType
	{
		SET_BEGIN_ENUM( EXPERR_, 0 ),

		EXP_SUCCESS,
		EXP_ERROR_UNKNOWN,
		EXP_ERROR_MANGLED_CUSTOM_ATTS,
		EXP_ERROR_MANGLED_CUSTOM_MODIFIER,
		EXP_ERROR_NO_DIRECTORY,
		EXP_ERROR_CANNOT_CREATE_DIRECTORY,
		EXP_ERROR_FILE_EXISTS,
		EXP_ERROR_FILE_LOCKED,
		EXP_ERROR_CANNOT_CREATE_FILE,
		EXP_ERROR_NOTHING_SELECTED,
		EXP_ERROR_NO_CUSTOM_CONTAINER,
		EXP_ERROR_IN_POSTPROCESS,
		EXP_ERROR_EXCEPTION,
		EXP_ERROR_NO_BONES,
		EXP_ERROR_BUSTED_MAXSCRIPT_GLOBALS,
		EXP_ERROR_NO_MATERIAL,
		EXP_ERROR_CANNOT_CONVERT_TO_TRIOBJECT,
		EXP_ERROR_NEGATIVE_PARITY,
		EXP_ERROR_CANNOT_DETERMINE_FILENAME,
		EXP_ERROR_CANNOT_WRITE_TO_DIRECTORY,
		EXP_ERROR_FILE_COPY_FAILED,
		EXP_ERROR_CANNOT_DETERMINE_DESTINATION,
		EXP_ERROR_CAUGHT_INTERNAL_EXCEPTION,
		EXP_ERROR_FILE_FAILED_TO_CLOSE,
		EXP_ERROR_EXPORT_CANCELLED,
		EXP_ERROR_ANIMATION_KEY_REDUCTION_FAILED,

		SET_END_ENUM( EXPERR_ ),
	};

	const char* ToString   ( eExpErrType e );
	bool        FromString ( const char* str, eExpErrType& e );

	void BeginEditParams(Interface *ip,IUtil *iu) {}
	void EndEditParams(Interface *ip,IUtil *iu)	  {}

//	void Init(HWND hWnd);
//	void Destroy(HWND hWnd);

	void DeleteThis() { m_ExpFile.Close(); }	
	
	//Constructor/Destructor
	DSiegeUtils();
	~DSiegeUtils();

	bool DoesAnimViewerExist()	{ return !m_AnimViewerExe.empty(); }
	bool DoesNodeViewerExist()	{ return !m_NodeViewerExe.empty(); }

	int ExportSelected(bool suppressPrompts, bool canStomp=true, bool makePath=true );
	int ExportForPreview(bool newviewer);
	
	HWND			hPanel;
	IUtil			*iu;
	Interface		*ip;

private:

	struct TankEntry
	{
		typedef std::set  <gpstring, istring_less> StringSet;

		eTankType             m_Type;			 // type of this tank
		FileSys::TankFileMgr* m_FileMgr;		 // may be NULL if for bits only
		bool                  m_HasRequiredFile; // true if the $required file exists
		StringSet             m_RequiredTanks;	 // list of tanks required for this tank to be used

		TankEntry( void )
		{
			m_Type            = TANK_NONE;
			m_FileMgr         = NULL;
			m_HasRequiredFile = false;
		}

		bool operator == ( eTankType type ) const
		{
			return ( m_Type == type );
		}

		bool operator == ( const FileSys::TankFileMgr* tankFileMgr ) const
		{
			return ( m_FileMgr == tankFileMgr );
		}

		bool operator == ( const GUID& guid ) const;
	};

	typedef std::list <TankEntry> TankFileMgrColl;
	void findTanks( eTankType type, bool enable );
	void findBits(void );
	
	bool hasTank( const GUID& guid ) const;

	Config*					m_Config;
	NamingKey*				m_NamingKey;
	FileSys::MasterFileMgr*	m_MasterFileMgr;
	FileSys::MultiFileMgr*	m_MultiFileMgr;

	TankFileMgrColl			m_Tanks;

	FileSys::File			m_ExpFile;
	bool					m_SuppressPrompts;
	bool					m_OverwriteExisting;
	bool					m_CreateDirectories;

	gpstring				m_AnimViewerExe;
	DWORD					m_AnimViewerProcessId;

	gpstring				m_NodeViewerExe;
	DWORD					m_NodeViewerProcessId;
	
 	eExpErrType OpenFileHelper( const gpstring& fname );
	eExpErrType CloseFileHelper( const gpstring& fname, bool keepfile );
	eExpErrType CopyFileHelper( const gpstring& srcname, const gpstring& dstname );

	bool FetchGlobalsFromMaxscript();

};

DSiegeUtils* GetSiegeUtils( void );

extern HINSTANCE hInstanceDLL;

class DSiegeUtilsClassDesc:public ClassDesc2
{
	public:
	int 			IsPublic() { return FALSE; }
	void *			Create(BOOL loading = FALSE) { return GetSiegeUtils(); }
	const TCHAR *	ClassName() { return GetString(IDS_CLASS_NAME); }
	SClass_ID		SuperClassID() { return UTILITY_CLASS_ID; }
	Class_ID		ClassID() { return DSIEGEUTILS_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("DSiegeUtils"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstanceDLL; }			// returns owning module handle
};


#endif // __DSIEGEEXP__H
