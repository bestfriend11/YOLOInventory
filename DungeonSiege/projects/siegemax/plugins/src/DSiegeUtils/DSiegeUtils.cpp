/**********************************************************************
 *<
	FILE: DSiegeUtils.cpp

	DESCRIPTION: Dungeon Siege Utility

	CREATED BY: biddle

	HISTORY: 

 *>	Copyright (c) 2002, All Rights Reserved.
 **********************************************************************/

#include "DSiegeUtils.h"
#include "DSmxs.h"
#include "SNOExp.h"
#include "PRSExp.h"
#include "ASPExp.h"
#include <WorldVersion.h>
#include <gpcore.h>
#include <config.h>
#include <ReportSys.h>
#include <NamingKey.h>
#include <stringtool.h>
#include <StdHelp.h>
#include <PathFileMgr.h>
#include <FileSysUtils.h>

#include "WorldConfig.inc"

DSiegeUtils* pvtDSiegeUtils = 0;

bool InitDSiegeUtils(void)
{
	pvtDSiegeUtils = new DSiegeUtils();
	return true;
}

DSiegeUtils* GetSiegeUtils( void )
{
	return pvtDSiegeUtils ? pvtDSiegeUtils : 0;
}

static DSiegeUtilsClassDesc DSiegeUtilsDesc;
ClassDesc2* GetDSiegeUtilsDesc() { return &DSiegeUtilsDesc; }

//*********************************************************
BOOL CALLBACK DSiegeUtilsOptionsDlgProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam) 
{
	static DSiegeUtils *imp = NULL;

	switch(message) {
		case WM_INITDIALOG:
			imp = (DSiegeUtils *)lParam;
			CenterWindow(hWnd,GetParent(hWnd));
			return TRUE;

		case WM_CLOSE:
			EndDialog(hWnd, 0);
			return TRUE;
	}
	return FALSE;
}

//*********************************************************

class ListenerSink : public ReportSys::Sink 
{
public:
	SET_INHERITED( ListenerSink, ReportSys::Sink );

	ListenerSink( void )			{  }
	virtual ~ListenerSink( void )	{  }

	virtual bool OutputString( const ReportSys::Context* context, const char* text, int len, bool startLine, bool endLine );
	virtual bool OutputField ( const ReportSys::Context* context, const char* text, int len );

private:
	SET_NO_COPYING( ListenerSink );
};

//*********************************************************
bool ListenerSink::OutputString( const ReportSys::Context* context, const char* text, int len, bool startLine, bool endLine )
{
	UNREFERENCED_PARAMETER(context);
	UNREFERENCED_PARAMETER(len);

	if (MAXScript_interface)
	{
		// The caller doesn't strip the CR/LF reliably
		gpstring local(text,text+len);

		if (startLine && endLine)
		{
			mprintf("-- %s\n",local.c_str());
		}
		else if (startLine)
		{
			mprintf("-- %s",local.c_str());
		}
		else if (endLine)
		{
			mprintf("%s\n",local.c_str());
		}
		else
		{
			mprintf("%s",local.c_str());
		}
	}

	return true;
}

//*********************************************************
bool ListenerSink::OutputField ( const ReportSys::Context* context, const char* text, int len )
{

	// The caller doesn't strip the CR/LF reliably
	gpstring local(text,text+len);

	mprintf("%s",local.c_str());
	
	return true;
}

//--- DSiegeUtils -------------------------------------------------------

DSiegeUtils::DSiegeUtils()
{
	m_OverwriteExisting = false;
	m_CreateDirectories = false;

	FileSys::SetProductId( DS1_FOURCC_ID );
	
	ConfigInit siegeMaxInit;

	siegeMaxInit.SetMyDocuments();
	siegeMaxInit.m_IniFileName = "SiegeMax";
	siegeMaxInit.m_RegistryName = "SiegeMax";
	siegeMaxInit.m_DocsSubDir = "Dungeon Siege";

	// Create the required singletons...
	m_Config = new Config(siegeMaxInit);

	gConfig.Init();

	InitializeWorldConfigPaths(Services::APP_SIEGE_MAX);

	m_MasterFileMgr = new FileSys::MasterFileMgr( FileSys::MasterFileMgr::eOptions::OPTION_NONE );
	m_MultiFileMgr  = new FileSys::MultiFileMgr;
	m_MasterFileMgr->AddFileMgr( m_MultiFileMgr );

// Initialize FileSys.

	// find all the tanks
	findTanks( TANK_RESOURCE, true );
	findTanks( TANK_MOD, true );

	// build our dev-only file mgrs
	findBits( );

	// after everything is done, update the index now to fill it (do it now
	m_MasterFileMgr->CheckIndexDirty();

	gConfig.DumpPathVars( &gGenericContext );

	m_NamingKey = new NamingKey();

	Registry DSTKreg ( "Gas Powered Games", "DSTK", false );

	gpstring DSAnimViewer = DSTKreg.GetString( "1.0/AnimPreviewer" );
	if ( !DSAnimViewer.empty() )
	{
		m_AnimViewerExe = gConfig.ResolvePathVars("%dstk_path%/"+DSAnimViewer);
		if (!FileSys::DoesFileExist(m_AnimViewerExe))
		{
			m_AnimViewerExe.clear();
		}
	}
	else
	{
		m_AnimViewerExe.clear();
	}

	m_AnimViewerProcessId = NULL;

	gpstring DSNodeViewer = DSTKreg.GetString( "1.0/NodePreviewer" );
	if ( !DSNodeViewer.empty() )
	{
		m_NodeViewerExe = gConfig.ResolvePathVars("%dstk_path%/"+DSNodeViewer);
		if (!FileSys::DoesFileExist(m_NodeViewerExe))
		{
			m_NodeViewerExe.clear();
		}
	}
	else
	{
		m_NodeViewerExe.clear();
	}

	m_NodeViewerProcessId = NULL;
	
	gpgeneric("Dungeon Siege Utility plugin initialized\n");

	gGlobalSink.AddSink(new ListenerSink(),true);

}

DSiegeUtils::~DSiegeUtils() 
{
	// Release all the resources
}

void DSiegeUtils::findTanks( eTankType type, bool enable )
{
	// get some specs
	gpwstring pathVar       = gConfig.ResolvePathVars( ::ToUnicode( ToPathVar( type ) ), Config::PVM_APPEND );
	gpwstring fileSpec      = L"*" + ::ToUnicode( ToExtension( type ) );
	bool      ignoreVersion = gConfig.GetBool( "tank_ignore_version", false );
	bool      verifyData    = gConfig.GetBool( "verifydata", false );

	// find files from those specs
	std::list <gpwstring> foundFiles;
	FileSys::FindFilesFromSpec( pathVar, foundFiles, true, fileSpec );

	// track new tanks so we can mount them
	typedef std::list <TankEntry*> TankList;
	TankList foundTanks;

	// working mgr
	std::auto_ptr <FileSys::TankFileMgr> tankFileMgr( new FileSys::TankFileMgr );
	tankFileMgr->SetOptions(FileSys::TankFileMgr::OPTION_NO_PAGING);
	
	// now process each that we found
	std::list <gpwstring>::iterator i, ibegin = foundFiles.begin(), iend = foundFiles.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		gpstring fileName( ::ToAnsi( *i ) );

		// note: with many tanks, the index crc check will eat up much memory
		// that is not returned, so defer that until a tank is mounted for all
		// but the resources.
		if ( tankFileMgr->OpenTank( fileName, enable ) )
		{
			// make sure it's not already there
			if ( !hasTank( tankFileMgr->GetTankGuid() ) )
			{
				// optionally verify
				if ( verifyData )
				{
					if ( tankFileMgr->VerifyAll() )
					{
						gpmessageboxf(( "Resource verification SUCCEEDED! Resource CRC's check out a-ok!\n\nFile = '%s'\n", fileName.c_str() ));
					}
					else
					{
						gperrorboxf(( "Resource verification FAILED! Resource CRC mismatch, file may be corrupt!\n\nFile = '%s'\n", fileName.c_str() ));
					}
				}

				// now figure out where in the collection it should
				// go (in sorted priority order)
				TankFileMgrColl::iterator j, jbegin = m_Tanks.begin(), jend = m_Tanks.end();
				for ( j = jbegin ; j != jend ; ++j )
				{
					// if it's newer
					if ( (j->m_FileMgr != NULL) && tankFileMgr->ShouldOverride( *j->m_FileMgr ) )
					{
						break;
					}
				}

				// insert and configure
				j = m_Tanks.insert( j, TankEntry() );
				j->m_Type = type;
				j->m_FileMgr = tankFileMgr.release();

				// fill out required tank list
				FileSys::FileHandle requiredFile = tankFileMgr->Open( "$required" );
				if ( requiredFile )
				{
					j->m_HasRequiredFile = true;

					// read each entry in the file
					gpstring fileName;
					while ( tankFileMgr->ReadLine( requiredFile, fileName ) )
					{
						stringtool::RemoveBorderingWhiteSpace( fileName );
						FileSys::CleanupPath( fileName );
						fileName.to_lower();

						// skip empties and comments
						if ( !fileName.empty() && !((fileName[ 0 ] == '/') && (fileName[ 1 ] == '/')) )
						{
							j->m_RequiredTanks.insert( fileName );
						}
					}

					// was this on a dsres? well just turn it into a dsmod.
					// this is necessary because 1.0 did not know about
					// dsmods, so we need to release a couple maps pre-1.1
					// using dsres for the mod part of the content. putting
					// the $required in there will say "this is really a
					// dsmod file".
					if ( j->m_Type == TANK_RESOURCE )
					{
						// re-cast to mod and disable for now
						j->m_Type = TANK_MOD;
						j->m_FileMgr->Enable( enable );
					}
				}

				// now make sure enabling matches and copy the successful ptr
				j->m_FileMgr->Enable( enable );
				foundTanks.push_back( &*j );

				// give the autoptr a new filemgr
				tankFileMgr = stdx::make_auto_ptr( new FileSys::TankFileMgr );
				tankFileMgr->SetOptions(FileSys::TankFileMgr::OPTION_NO_PAGING);

			}
		}
		else if ( ::GetLastError() != ERROR_BAD_FORMAT )
		{
			gperrorf(( "Unable to open tank '%s', error is '%s' (0x%08X)\n",
					   fileName.c_str(), stringtool::GetLastErrorText().c_str(), ::GetLastError() ));
		}
	}

	// mount our tanks
	TankList::const_iterator j, jbegin = foundTanks.begin(), jend = foundTanks.end();
	for ( j = jbegin ; j != jend ; ++j )
	{
		FileSys::TankFileMgr* tankFileMgr = (*j)->m_FileMgr;

		
//		GetGlobalFileMgr()->AddFileMgr( tankFileMgr, true, tankFileMgr->GetTankPriorityForMaster() );
		m_MultiFileMgr->AddFileMgr( tankFileMgr, true, tankFileMgr->GetTankPriorityForMaster() );
		gpgenericf(( "FileSys %s = '%s'\n", ToString( type ), tankFileMgr->GetTankFileName().c_str() ));
	}
}

void DSiegeUtils::findBits( void )
{
#	if !GP_RETAIL

	// get some specs
	gpwstring pathVar = gConfig.ResolvePathVars( L"%bits%", Config::PVM_APPEND );

	// find paths from those specs
	std::list <gpwstring> foundPaths;
	FileSys::FindPathsFromSpec( pathVar, foundPaths );

	// working mgr
	std::auto_ptr < FileSys::PathFileMgr > pathFileMgr( new FileSys::PathFileMgr );

	// build filemgrs and mount directly
	std::list <gpwstring>::const_iterator i, ibegin = foundPaths.begin(), iend = foundPaths.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		gpstring pathName( ::ToAnsi( *i ) );

		// try to use it
		if ( pathFileMgr->SetRoot( pathName ) )
		{
			// set options we need
			pathFileMgr->SetOptions( FileSys::PathFileMgr::OPTION_COPY_NET_LOCAL );

			// mount it
			m_MultiFileMgr->AddFileMgr( pathFileMgr.release(), true );

			// give the autoptr a new filemgr
			pathFileMgr = stdx::make_auto_ptr( new FileSys::PathFileMgr );
		}
	}

#	endif // !GP_RETAIL
}

//////////////////////////////////////////////////////////////////////////////
bool DSiegeUtils::FetchGlobalsFromMaxscript()
{

	Value* v;

	v = globals->get(Name::intern("dsglb_reference_equipment"))->eval();

	if (!v)
	{
		gpgeneric("MISSING GLOBAL DATA\n");
		return false;
	}

	if ( is_array(v) )
	{
		//dbgwart gpgeneric("It is an ARRAY!\n");
		Array* a = (Array*)v;
		for (int i = 0; i < (*a).size ; i++)
		{
			Value* elem = (*a)[i];
			if ( is_array(elem) )
			{
				Array* earray = (Array*)elem;
				//dbgwart gpgenericf(("Elem %d is an array of size %d\n",i,(*earray).size));
				for (int j = 0; j < (*earray).size ; j++)
				{
					Value* subelem = (*earray)[j];
					if ( is_string(subelem) )
					{
						gpstring estr(subelem->to_string());
						//dbgwart gpgenericf(("SubElem [%d,%d] is string [%s]\n",i,j,estr.c_str()));
					}
					else if ( is_point3(subelem) )
					{
						Point3 epoint = subelem->to_point3();
						//dbgwart gpgenericf(("SubElem [%d,%d] is point3 [%8.5f,%8.5f,%8.5f]\n",i,j,epoint.x,epoint.y,epoint.z));
					}
				}
			}
			else if ( is_string(elem) )
			{
				gpstring estr(elem->to_string());
				//dbgwart gpgenericf(("Elem %d is a string [%s]\n",i,estr.c_str()));
			}
			else if ( is_point3(elem) )
			{
				Point3 epoint = elem->to_point3();
				//dbgwart gpgenericf(("Elem %d is a point3 [%8.5f,%8.5f,%8.5f]\n",i,epoint.x,epoint.y,epoint.z));
			}
		}
	}

	return true;
}

bool DSiegeUtils :: hasTank( const GUID& guid ) const
{
	return ( stdx::has_any( m_Tanks, guid ) );
}

//////////////////////////////////////////////////////////////////////////////
// struct TankMgr::TankEntry implementation

bool DSiegeUtils::TankEntry :: operator == ( const GUID& guid ) const
{
	return ( (m_FileMgr != NULL) && (m_FileMgr->GetTankGuid() == guid) );
}

//////////////////////////////////////////////////////////////////////////////
bool FetchDescription(const gpstring& contentname ,gpstring& description)
{
	std::list<gpstring> desc;
	if ( gNamingKey.BuildContentDescription(contentname,desc) )
	{
		DWORD spaces = 0;
		for2 (std::list<gpstring>::iterator it = desc.begin() ; it != desc.end() ; ++it)
		{	
			for2 (int i=0; i<spaces; ++i)
			{
				description.append(" ");
			}
			description.appendf("%s\n",(*it).c_str());
			spaces += 2;
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////
DSiegeUtils::eExpErrType DSiegeUtils::OpenFileHelper( const gpstring& relativefile)
{
	gpstring fullFileName;

	eExpErrType ret = EXP_SUCCESS;

	if ( gConfig.ResolvePathVars( fullFileName, relativefile ) )
	{

		// Does the directory exist yet?
		if ( !FileSys::DoesPathExist( FileSys::GetPathOnly( fullFileName,false) ) )
		{
			// Should we build a new dir?
			if (m_CreateDirectories)
			{
				if ( !FileSys::CreateFullFileNameDirectory( fullFileName ) )
				{
					ret = EXP_ERROR_CANNOT_CREATE_DIRECTORY;
				}
			}
			else
			{
				ret = EXP_ERROR_NO_DIRECTORY;
				if (!m_SuppressPrompts)
				{
					gpstring buff;
					buff.assignf("The required directory does not exist.\n\n"
								 "%s\n\n"
								 "Would you like to create it?",
									FileSys::GetPathOnly( fullFileName ).c_str()
									);
					DWORD icon = MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2;
					if (MessageBox(GetCOREInterface()->GetMAXHWnd(),buff.c_str(),"File Exists",icon ) == IDYES)
					{
						if ( FileSys::CreateFullFileNameDirectory( fullFileName ) )
						{
							ret = EXP_SUCCESS;
						}
						else
						{
							ret = EXP_ERROR_CANNOT_CREATE_DIRECTORY;
						}
					}
				}
			}
		}

		if ( ret == EXP_SUCCESS && FileSys::DoesFileExist(fullFileName) )
		{
			// Should we really stomp it?
			if (m_OverwriteExisting)
			{
				if ( !DeleteFile(fullFileName.c_str()) )
				{
					ret = EXP_ERROR_FILE_LOCKED;
				}
			}
			else
			{
				ret = EXP_ERROR_FILE_EXISTS;
				if (!m_SuppressPrompts)
				{
					gpstring buff;
					buff.assignf("The output file already exists.\n\n"
								 "%s\n\n"
								 "Would you like overwrite it?",
									FileSys::GetFileNameOnly( fullFileName).c_str()
									);
					DWORD icon = MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2;
					if (MessageBox(GetCOREInterface()->GetMAXHWnd(),buff.c_str(),"File Exists",icon ) == IDYES)
					{
						if ( DeleteFile(fullFileName.c_str()) )
						{
							ret = EXP_SUCCESS;
						}
						else
						{
							ret = EXP_ERROR_FILE_LOCKED;
						}
					}
				}
			}
		}

	}
	else
	{
		ret = EXP_ERROR_CANNOT_DETERMINE_FILENAME;
	}

	// Open output file for binary write access
	if (ret == EXP_SUCCESS)
	{
		if (!m_ExpFile.CreateNew( fullFileName, FileSys::File::ACCESS_READ_WRITE, FileSys::File::SHARE_NONE ) )
		{
			ret = EXP_ERROR_CANNOT_CREATE_FILE;
		}
	}

	return ret;

}

//////////////////////////////////////////////////////////////////////////////
DSiegeUtils::eExpErrType DSiegeUtils::CloseFileHelper( const gpstring& fname, bool keepfile )
{
	if ( !m_ExpFile.Close() )
	{
		return EXP_ERROR_FILE_FAILED_TO_CLOSE;
	}

	if ( !keepfile )
	{
		gpstring fullFile;
		if (gConfig.ResolvePathVars( fullFile, fname ) )
		{
			return DeleteFile(fullFile.c_str()) ? EXP_SUCCESS : EXP_ERROR_FILE_FAILED_TO_CLOSE;
		}
	}

	return EXP_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////
DSiegeUtils::eExpErrType DSiegeUtils::CopyFileHelper( const gpstring& srcname, const gpstring& dstname )
{
	eExpErrType ret = EXP_SUCCESS;
	
	gpstring fullFileName;
	if ( gConfig.ResolvePathVars( fullFileName, dstname ) )
	{
		if ( !FileSys::DoesPathExist( FileSys::GetPathOnly( fullFileName,false ) ) )
		{
			// Should we build a new dir?
			if (m_CreateDirectories)
			{
				if ( !FileSys::CreateFullFileNameDirectory( fullFileName ) )
				{
					ret = EXP_ERROR_CANNOT_CREATE_DIRECTORY;
				}
			}
			else
			{
				ret = EXP_ERROR_NO_DIRECTORY;
				if (!m_SuppressPrompts)
				{
					gpstring buff;
					buff.assignf("The required directory does not exist.\n\n"
								 "%s\n\n"
								 "Would you like to create it?",
									FileSys::GetPathOnly( fullFileName ).c_str()
									);
					DWORD icon = MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2;
					if (MessageBox(GetCOREInterface()->GetMAXHWnd(),buff.c_str(),"File Exists",icon ) == IDYES)
					{
						if ( FileSys::CreateFullFileNameDirectory( fullFileName ) )
						{
							ret = EXP_SUCCESS;
						}
						else
						{
							ret = EXP_ERROR_CANNOT_CREATE_DIRECTORY;
						}
					}
				}
			}
		}
		else
		{
			if ( !FileSys::IsDirWritable( FileSys::GetPathOnly( fullFileName,false) ) )
			{
				ret = EXP_ERROR_CANNOT_WRITE_TO_DIRECTORY;
			}
			else
			{		
				if ( FileSys::DoesFileExist(fullFileName) )
				{
					if (!m_OverwriteExisting)
					{
						ret = EXP_ERROR_FILE_EXISTS;
						if (!m_SuppressPrompts)
						{
							gpstring buff;
							buff.assignf("The output file already exists.\n\n"
										 "%s\n\n"
										 "Would you like overwrite it?",
											FileSys::GetFileNameOnly( fullFileName).c_str()
											);
							DWORD icon = MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2;
							if (MessageBox(GetCOREInterface()->GetMAXHWnd(),buff.c_str(),"File Exists",icon ) == IDYES)
							{
								if ( DeleteFile(fullFileName.c_str()) )
								{
									ret = EXP_SUCCESS;
								}
								else
								{
									ret = EXP_ERROR_FILE_LOCKED;
								}
							}
						}
					}
					else if ( !DeleteFile(fullFileName.c_str()) )
					{
						ret = EXP_ERROR_FILE_LOCKED;
					}
				}
			}
		}

		if ( ret == EXP_SUCCESS )
		{
			if (!CopyFile(srcname.c_str(),fullFileName.c_str(),true))
			{
				ret = EXP_ERROR_FILE_COPY_FAILED;
			}
			else 
			{
				if (!m_SuppressPrompts)
				{
					gpstring fname = FileSys::GetFileNameOnly(fullFileName);
					gpstring description;

					FetchDescription( fname,description );

					gpstring buff;
					buff.assignf("Writing file [%s] to\n\n%s",fname.c_str(),description.c_str());
					DWORD icon = MB_OK | MB_ICONINFORMATION | MB_DEFBUTTON1;
					
					if (MessageBox(GetCOREInterface()->GetMAXHWnd(),buff.c_str(),"Writing file",icon ) == IDYES)
					{
						ret = EXP_SUCCESS;
					}
				}
			}
		}

	}
	else
	{
		ret = EXP_ERROR_CANNOT_DETERMINE_FILENAME;
	}

	return ret;
}


//////////////////////////////////////////////////////////////////////////////
struct WindowProcIDEnumerator
{
	WindowProcIDEnumerator(DWORD id)
		: m_HSearchProcessID(id) 
		, m_HWndFound(NULL)
	{}

	HWND m_HWndFound;
	DWORD m_HSearchProcessID;
};

//////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK MatchProcessID(HWND hwnd, LPARAM parm)
{
	WindowProcIDEnumerator& we = *(WindowProcIDEnumerator*)parm;

	DWORD procid;
	DWORD threadid = GetWindowThreadProcessId(hwnd,&procid);

	if (procid == (DWORD)we.m_HSearchProcessID)
	{
		we.m_HWndFound = hwnd;
		return FALSE;
	}

	if (threadid == (DWORD)we.m_HSearchProcessID)
	{
		we.m_HWndFound = hwnd;
		return FALSE;
	}

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
int DSiegeUtils::ExportForPreview(bool newviewer)
{

	int nnkcount = gNamingKey.GetNumFileNames();

	if (nnkcount == 0)
	{
		gpstring nnkDir = "art/";

		if ( nnkDir )
		{
			if ( ! gNamingKey.LoadNamingKey( nnkDir ) )
			{
				return 0;
			}
		}
	}
	
	gpstring maxfilename(GetCOREInterface()->GetCurFileName());

	bool savedSuppressPrompts = m_SuppressPrompts;
	bool savedOverwriteExisting = m_OverwriteExisting;
	bool savedCreateDirectories = m_CreateDirectories;

	m_SuppressPrompts = true;
	m_OverwriteExisting = true;
	m_CreateDirectories = true;

	int NumExported = 0;

	int numnodes = GetCOREInterface()->GetSelNodeCount();

	if (numnodes != 1)
	{
		MessageBox(GetCOREInterface()->GetMAXHWnd(),
			"Only one object can be previewed at a time",
			"Exporter Error",
			MB_OK | MB_ICONSTOP
			);
	}

	else
	{

		FetchGlobalsFromMaxscript();

		gpstring previewName = gConfig.ResolvePathVars("%temp_path%/temp_siegemax_preview");

		INode* toExport = GetCOREInterface()->GetSelNode(0);

		gpstring nodename(toExport->GetName());

		SNOExporter* snoexport = 0;
		PRSExporter* prsexport = 0;
		ASPExporter* aspexport = 0;

		DSiegeUtils::eExpErrType ret = EXP_SUCCESS;

		if (nodename.same_no_case("T_",2) )
		{
			snoexport = new SNOExporter(maxfilename,GetCOREInterface());
			if( !snoexport )
			{
				return 0;
			}

			gpstring snoname = previewName+".sno";

			ret = OpenFileHelper(snoname);
			if ( ret == EXP_SUCCESS )
			{
				try 
				{
					ret = snoexport->ExportINode(toExport,snoname,m_ExpFile);
				}
				catch(...)
				{
					ret = EXP_ERROR_CAUGHT_INTERNAL_EXCEPTION;
				}
				if ( ret == EXP_SUCCESS ) 
				{
					++NumExported;
					CloseFileHelper(snoname,true);
				}
				else
				{
					CloseFileHelper(snoname,false);
				}
			}

		}
		else if (    nodename.same_no_case("SKINMESH") 
				  || nodename.same_no_case("A_",2) 
				  || nodename.same_no_case("M_",2) )
		{
			prsexport = new PRSExporter(maxfilename,GetCOREInterface());
			if( !prsexport )
			{
				return 0;
			}

			aspexport = new ASPExporter(maxfilename,GetCOREInterface());
			if( !aspexport )
			{
				delete prsexport;
				return 0;
			}

			gpstring aspname = previewName+".asp";

			ret = OpenFileHelper(aspname);
			if ( ret == EXP_SUCCESS )
			{
				try 
				{
					ret = aspexport->ExportINode(toExport,aspname,m_ExpFile);
				}
				catch(...)
				{
					ret = EXP_ERROR_CAUGHT_INTERNAL_EXCEPTION;
				}
				if ( ret == EXP_SUCCESS ) 
				{
					++NumExported;
					CloseFileHelper(aspname,true);
				}
				else
				{
					CloseFileHelper(aspname,false);
				}
			}

			gpstring prsname = previewName+".prs";

			if ( ret == EXP_SUCCESS )
			{
				ret = OpenFileHelper(prsname);

				if ( ret == EXP_SUCCESS )
				{
					try 
					{
						ret = prsexport->ExportINode(toExport,prsname,m_ExpFile);
					}
					catch(...)
					{
						ret = EXP_ERROR_CAUGHT_INTERNAL_EXCEPTION;
					}
					if ( ret == EXP_SUCCESS ) 
					{
						++NumExported;
						ret = CloseFileHelper(prsname,true);
					}
					else
					{
						CloseFileHelper(prsname,false);
					}
				}
			}
		}

		gpstring bitmaploadlist;
		
		if (ret == EXP_SUCCESS)
		{
			// Copy all the textures required to preview the mesh

			std::vector<gpstring> bmfiles;
			
			if (aspexport)
			{
				aspexport->FetchTextureFiles(bmfiles);
			}
			else if (snoexport)
			{
				snoexport->FetchTextureFiles(bmfiles);
			}


			if ( !bmfiles.empty() )
			{
				bitmaploadlist.appendf("\"TEXDIR=%s\" ",FileSys::GetPathOnly(previewName).c_str());

				for (int i = 0; i != bmfiles.size(); ++i)
				{
					gpstring destfile;
					gpstring filext = FileSys::GetExtension(bmfiles[i]);

					if ( ::same_no_case( filext, "psd") ||
						 ::same_no_case( filext, "raw") ||
						 ::same_no_case( filext, "bmp") )
					{
						// Make sure any previous files are removed
						destfile.assignf("%s%d.%s",previewName.c_str(),i,"BMP");
						SetFileAttributes(destfile.c_str(),FILE_ATTRIBUTE_NORMAL);
						DeleteFile(destfile.c_str());

						destfile.assignf("%s%d.%s",previewName.c_str(),i,"PSD");
						SetFileAttributes(destfile.c_str(),FILE_ATTRIBUTE_NORMAL);
						DeleteFile(destfile.c_str());

						destfile.assignf("%s%d.%s",previewName.c_str(),i,"RAW");
						SetFileAttributes(destfile.c_str(),FILE_ATTRIBUTE_NORMAL);
						DeleteFile(destfile.c_str());

						bitmaploadlist.appendf("\"MAP%d=%s%d.%s\" ",i,FileSys::GetFileNameOnly(previewName).c_str(),i,filext.c_str());

						destfile.assignf("%s%d.%s",previewName.c_str(),i,filext.c_str());
						
						CopyFile(bmfiles[i].c_str(),destfile.c_str(),false);
						SetFileAttributes(destfile.c_str(),FILE_ATTRIBUTE_NORMAL);
					}				
				}
			}
		}

		if (ret == EXP_SUCCESS)
		{
			if ( aspexport && !m_AnimViewerExe.empty() )
			{
				gpstring cmdargs;
				cmdargs.assignf("PREVIEW=TRUE");

				// Is the animviewer still running?

				bool mustLaunch = true;
						
				// Get the handle to the window that the AnimViewer process opened

				if (!newviewer && m_AnimViewerProcessId)
				{
					WindowProcIDEnumerator sniffer(m_AnimViewerProcessId);
					EnumWindows(MatchProcessID,(LPARAM)&sniffer);
					HWND hwnd = sniffer.m_HWndFound;

					if ( hwnd )
					{
						WINDOWPLACEMENT winplace;
						winplace.length = sizeof(winplace);

						BOOL ok = GetWindowPlacement(hwnd,&winplace);

						winplace.showCmd = SW_RESTORE;
						SetWindowPlacement(hwnd,&winplace);

						SetForegroundWindow(hwnd);

						if (PostMessage(hwnd,WM_APP,1,0))
						{
							mustLaunch = false;
						}
					}
				}

				if (mustLaunch)
				{

					STARTUPINFO startinfo;
					memset(&startinfo,0,sizeof(startinfo));
					startinfo.cb = sizeof(startinfo);

					PROCESS_INFORMATION procinfo;
					memset(&procinfo,0,sizeof(procinfo));

					gpstring fullargs;
					fullargs.append("\"");
					fullargs.append(m_AnimViewerExe);
					fullargs.append("\" ");
					fullargs.append(cmdargs);

					if ( CreateProcess(	m_AnimViewerExe.begin_split(),
										fullargs.begin_split(),
										NULL,
										NULL,
										FALSE,
										NORMAL_PRIORITY_CLASS,
										NULL,
										FileSys::GetPathOnly(m_AnimViewerExe).begin_split(),
										&startinfo,
										&procinfo
										) )
					{
						m_AnimViewerProcessId = procinfo.dwProcessId;
					}
					else
					{
						m_AnimViewerProcessId = NULL;
					}
				}
			}
			else if ( snoexport && !m_NodeViewerExe.empty() )
			{
				gpstring cmdargs;
				cmdargs.assignf("\"SNO=%s.SNO\" %s",previewName.c_str(),bitmaploadlist.c_str());

				// Is the nodeviewer still running?

				bool mustLaunch = true;
						
				// Get the handle to the window that the AnimViewer process opened

				if (!newviewer && m_NodeViewerProcessId)
				{
					WindowProcIDEnumerator sniffer(m_NodeViewerProcessId);
					EnumWindows(MatchProcessID,(LPARAM)&sniffer);
					HWND hwnd = sniffer.m_HWndFound;

					if ( hwnd )
					{
						WINDOWPLACEMENT winplace;
						winplace.length = sizeof(winplace);

						BOOL ok = GetWindowPlacement(hwnd,&winplace);

						winplace.showCmd = SW_RESTORE;
						SetWindowPlacement(hwnd,&winplace);

						SetForegroundWindow(hwnd);

						if (PostMessage(hwnd,WM_APP,1,0))
						{
							mustLaunch = false;
						}
					}
				}

				// Will fix this 
				mustLaunch = true;

				if (mustLaunch)
				{

					STARTUPINFO startinfo;
					memset(&startinfo,0,sizeof(startinfo));
					startinfo.cb = sizeof(startinfo);

					PROCESS_INFORMATION procinfo;
					memset(&procinfo,0,sizeof(procinfo));

					gpstring wrapexe = FileSys::GetFileNameOnly(m_NodeViewerExe);

					gpstring fullargs;
					fullargs.append("\"");
					fullargs.append(wrapexe);
					fullargs.append("\" ");
					fullargs.append(cmdargs);

					gpstring pathonly = FileSys::GetPathOnly(m_NodeViewerExe);

					//gpgenericf(("exe [%s]\n",m_NodeViewerExe.c_str()));
					//gpgenericf(("arg [%s]\n",fullargs.c_str()));
					//gpgenericf(("pth [%s]\n",pathonly.c_str()));

					if ( CreateProcess(	m_NodeViewerExe.begin_split(),
										fullargs.begin_split(),
										NULL,
										NULL,
										FALSE,
										NORMAL_PRIORITY_CLASS,
										NULL,
										pathonly.begin_split(),
										&startinfo,
										&procinfo
										) )
					{
						m_NodeViewerProcessId = procinfo.dwProcessId;
					}
					else
					{
						m_NodeViewerProcessId = NULL;
					}
				}
			}
		}
		else
		{
			// Failed to export, can't preview
			gpstring buff;
			DWORD icon;
			buff.assignf("Nothing was exported\n\nExporter returned:\n\n%s",ToString(ret));
			icon = MB_OK| MB_ICONSTOP;
			MessageBox(GetCOREInterface()->GetMAXHWnd(),buff.c_str(),"SiegeMax",icon );
		}
		
		delete snoexport;
		delete prsexport;
		delete aspexport;

	}
	
	m_SuppressPrompts   = savedSuppressPrompts;
	m_OverwriteExisting = savedOverwriteExisting;
	m_CreateDirectories = savedCreateDirectories;

	return 0;

}

//////////////////////////////////////////////////////////////////////////////
int DSiegeUtils::ExportSelected(bool suppressPrompts, bool canStomp, bool makePath)
{

	int nnkcount = gNamingKey.GetNumFileNames();

	if (nnkcount == 0)
	{
		gpstring nnkDir = "art/";

		if ( nnkDir )
		{
			if ( ! gNamingKey.LoadNamingKey( nnkDir ) )
			{
				return 0;
			}
		}
	}

	gpstring destDir;
	
	gpstring pvars("%out%/art");

	if ( !gConfig.ResolvePathVars( destDir, pvars ) )
	{
		return false;
	}

	//TODO: Implement the actual file Export here and 
	//		return TRUE If the file is exported properly
/*
	if(!suppressPrompts)
		DialogBoxParam(hInstance, 
				MAKEINTRESOURCE(IDD_PANEL), 
				GetActiveWindow(), 
				DSiegeUtilsOptionsDlgProc, (LPARAM)this);
*/

	gpstring maxfilename(GetCOREInterface()->GetCurFileName());

	SNOExporter* snoexport = new SNOExporter(maxfilename,GetCOREInterface());
	if( !snoexport )
	{
		return 0;
	}

	PRSExporter* prsexport = new PRSExporter(maxfilename,GetCOREInterface());
	if( !prsexport )
	{
		delete snoexport;
		return 0;
	}


	ASPExporter* aspexport = new ASPExporter(maxfilename,GetCOREInterface());
	if( !aspexport )
	{
		delete prsexport;
		delete snoexport;
		return 0;
	}

	int NumExported = 0;

	int numnodes = GetCOREInterface()->GetSelNodeCount();

	FetchGlobalsFromMaxscript();

	bool savedSuppressPrompts = m_SuppressPrompts;
	bool savedOverwriteExisting = m_OverwriteExisting;
	bool savedCreateDirectories = m_CreateDirectories;

	gpstring tempfilename = gConfig.ResolvePathVars("%temp_path%/dsiege_export.tmp");
	
	eExpErrType ret = EXP_SUCCESS;

	// Make sure that we can write out the temp file
	m_SuppressPrompts = true;
	m_OverwriteExisting = true;
	m_CreateDirectories = true;

	OpenFileHelper(tempfilename);
	if (ret == EXP_SUCCESS)
	{
		CloseFileHelper(tempfilename,false);
	}

	m_SuppressPrompts = suppressPrompts;
	m_OverwriteExisting = canStomp;
	m_CreateDirectories = makePath;

	if (ret != EXP_SUCCESS)
	{
		return ret;
	}

	for ( int ni = 0; (ret == EXP_SUCCESS) && (ni < numnodes); ++ni )
	{
		INode* toExport = GetCOREInterface()->GetSelNode(ni);

		if ( IsDummyObject(toExport) )
		{
			continue;
		}

		gpstring nodename(toExport->GetName());

		gpstring fname;
		gpstring description;

		ExpBase* expmodule;

		if (nodename.same_no_case("T_",2) )
		{
			FetchDescription(nodename,description);
			gpgenericf(("Exporting TERRAIN data: %s\n",nodename.c_str()));
			if ( gNamingKey.BuildSNOLocation(nodename,fname) )
			{
				expmodule = snoexport;
			}
			else
			{
				ret = EXP_ERROR_CANNOT_DETERMINE_DESTINATION;
			}
		}
		else if ( nodename.same_no_case("M_",2) || maxfilename.same_no_case("M_",2) )
		{
			if ( !nodename.same_no_case("M_",2) )
			{
				nodename = maxfilename;
				stringtool::RemoveExtension(nodename);
			}
			FetchDescription(nodename,description);
			gpgenericf(("Exporting MESH data: %s\n",nodename.c_str()));
			if ( gNamingKey.BuildASPLocation(nodename,fname) )
			{
				expmodule = aspexport;
			}
			else
			{
				ret = EXP_ERROR_CANNOT_DETERMINE_DESTINATION;
			}
		}
		else if ( nodename.same_no_case("A_",2) || maxfilename.same_no_case("A_",2) )
		{
			if ( !nodename.same_no_case("A_",2) )
			{
				nodename = maxfilename;
				stringtool::RemoveExtension(nodename);
			}
			FetchDescription(nodename,description);
			gpgenericf(("Exporting ANIMATION data: %s\n",nodename.c_str()));
			if ( gNamingKey.BuildPRSLocation(nodename,fname) )
			{
				expmodule = prsexport;
			}
			else
			{
				ret = EXP_ERROR_CANNOT_DETERMINE_DESTINATION;
			}
		}

		if ( ret == EXP_SUCCESS )
		{
			if ( ret == EXP_SUCCESS )
			{
				ret = OpenFileHelper(tempfilename);
				if ( ret == EXP_SUCCESS )
				{
					try 
					{
						ret = expmodule->ExportINode(toExport,fname,m_ExpFile);
					}
					catch(...)
					{
						ret = EXP_ERROR_CAUGHT_INTERNAL_EXCEPTION;
					}
					if ( ret == EXP_SUCCESS ) 
					{
						ret = CloseFileHelper(tempfilename,true);
					}
					else
					{
						CloseFileHelper(tempfilename,false);
					}
				}
				if ( ret == EXP_SUCCESS )
				{
					ret = CopyFileHelper(tempfilename,"%out%/"+fname);
					DeleteFile(tempfilename.c_str());
				}
			}
			if ( ret == EXP_SUCCESS )
			{
				++NumExported;
			}
		}

		if ( (ret != EXP_SUCCESS) && (ni < numnodes-1) && !m_SuppressPrompts)
		{
			gpstring buff;
			buff.assignf("Exporter returned the following error\n\n"
				         "[%s]\n\n"
						 "Would you like to continue exporting?",ToString(ret));
			DWORD icon = MB_YESNO | MB_ICONEXCLAMATION | MB_DEFBUTTON1;
			if (MessageBox(GetCOREInterface()->GetMAXHWnd(),buff.c_str(),"Exporter Error",icon ) == IDYES)
			{
				ret = EXP_SUCCESS;
			}
		}

		if ( (ret == EXP_SUCCESS) && !m_SuppressPrompts)
		{
			// Verify that all the textures are available...

			std::vector<gpstring> bmfiles;

			expmodule->FetchTextureFiles(bmfiles);

			for (int i = 0; i != bmfiles.size(); ++i)
			{
				gpstring imgext = FileSys::GetExtension(bmfiles[i]);
				gpstring imgfile = FileSys::GetFileNameOnly(bmfiles[i]);
				if (   ::same_no_case(imgext,"PSD")
					|| ::same_no_case(imgext,"BMP")
					|| ::same_no_case(imgext,"RAW")
					)
				{
					gpstring imgpath;
					if (gNamingKey.BuildContentLocation(imgfile.c_str(),imgpath))
					{
/*						if (   !FileSys::DoesResourceFileExist(imgpath+".PSD")
							&& !FileSys::DoesResourceFileExist(imgpath+".BMP")
							&& !FileSys::DoesResourceFileExist(imgpath+".RAW")	)
*/
						if ( !FileSys::DoesResourceFileExist(imgpath+".%img%") )
						{

							imgpath.append("."+imgext);

							gpstring imgdest = gConfig.ResolvePathVars("%out%/"+imgpath);

							gpstring msgstring;
							msgstring.assignf(	"%s.%%img%% does not exist in the directory:\n\n%s\n\n"
												"Would you like to copy this file now?\n\n"
												"NOTE: The game engine is optimized to load "
												"bitmaps that have been preprocessed to include mip-maps. "
												"If you choose to copy the bitmap now you should plan to "
												"replace it with a mip-mapped version at some point in the future",
												imgfile.c_str(),
												imgdest.c_str()
												);
							int retval = MessageBox(GetCOREInterface()->GetMAXHWnd(),
								msgstring.c_str(),
								"Missing Bitmap Warning",
								MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2);

							if (retval == IDYES)
							{							
								SetFileAttributes(imgdest.c_str(),FILE_ATTRIBUTE_NORMAL);
								DeleteFile(imgdest.c_str());
								CopyFileHelper(bmfiles[i].c_str(),imgdest.c_str());
								SetFileAttributes(imgdest.c_str(),FILE_ATTRIBUTE_NORMAL);
							}
						}
					}
					else
					{	
						gpstring msgstring;
						msgstring.assignf(	"%s\n\n"
											"Illegal texture name detected. "
											"Unable to resolve texture location using naming key."
											"The mesh may not appear as expected in the game.",
											imgfile.c_str()
											);
						MessageBox(GetCOREInterface()->GetMAXHWnd(),
							msgstring.c_str(),
							"Bitmap Verification Error",
							MB_OK|MB_ICONWARNING);
							
					}
				}
				else
				{
					gpstring msgstring;
					msgstring.assignf(	"%s\n\n"
										"Invalid texture file format has been detected. "
										"Only PSD, BMP & RAW files are supported at this time."
										"The mesh may not appear as expected in the game.",
										imgfile.c_str()
										);
					MessageBox(GetCOREInterface()->GetMAXHWnd(),
						msgstring.c_str(),
						"Bitmap Verification Error",
						MB_OK|MB_ICONWARNING);
				}
			}
		}
	}

	gpstring buff;
	DWORD icon;
	if( NumExported == 0 )
	{
		buff.assignf("Nothing was exported\n\nExporter returned:\n\n%s",ToString(ret));
		icon = MB_OK| MB_ICONSTOP;
	}
	else
	{
		if( NumExported == 1 )
		{
			buff.assign("1 object exported");
		}
		else
		{
			buff.assignf("%d objects exported",NumExported);
		}
		icon = MB_OK;
	}

	if(m_SuppressPrompts)
	{
		gpgenericf(("Exporter returned [%s]\n",ToString(ret)));
	}
	else
	{
		MessageBox(GetCOREInterface()->GetMAXHWnd(),buff.c_str(),"SiegeMax",icon );
	}

	delete snoexport;
	delete prsexport;
	delete aspexport;
	
	m_SuppressPrompts   = savedSuppressPrompts;
	m_OverwriteExisting = savedOverwriteExisting;
	m_CreateDirectories = savedCreateDirectories;

	return NumExported;
}

//////////////////////////////////////////////////////////////////////////////
// enum DSiegeUtils::eTankType implementation

static const char* s_TankTypeStrings[] =
{
	"tank_none",
	"tank_resource",
	"tank_map",
	"tank_mod",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_TankTypeStrings ) == DSiegeUtils::TANK_COUNT );

static stringtool::EnumStringConverter s_TankTypeConverter( s_TankTypeStrings, DSiegeUtils::TANK_BEGIN, DSiegeUtils::TANK_END );

const char* DSiegeUtils::ToString( DSiegeUtils::eTankType e )
	{  return ( s_TankTypeConverter.ToString( e ) );  }

bool DSiegeUtils::FromString( const char* str, DSiegeUtils::eTankType& e )
	{  return ( s_TankTypeConverter.FromString( str, rcast <int&> ( e ) ) );  }

static const char* s_TankTypePathVars[] =
{
	"",
	"%res_paths%",
	"%map_paths%",
	"%mod_paths%",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_TankTypePathVars ) == DSiegeUtils::TANK_COUNT );

static stringtool::EnumStringConverter s_PathVarConverter( s_TankTypePathVars, DSiegeUtils::TANK_BEGIN, DSiegeUtils::TANK_END );

const char* DSiegeUtils::ToPathVar( DSiegeUtils::eTankType e )
	{  return ( s_PathVarConverter.ToString( e ) );  }

static const char* s_TankTypeExtensions[] =
{
	"",
	".dsres",
	".dsmap",
	".dsmod",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_TankTypeExtensions ) == DSiegeUtils::TANK_COUNT );

static stringtool::EnumStringConverter s_ExtensionConverter( s_TankTypeExtensions, DSiegeUtils::TANK_BEGIN, DSiegeUtils::TANK_END );

const char* DSiegeUtils::ToExtension( DSiegeUtils::eTankType e )
	{  return ( s_ExtensionConverter.ToString( e ) );  }

FUBI_DECLARE_ENUM_TRAITS( DSiegeUtils::eTankType, DSiegeUtils::TANK_NONE );

//////////////////////////////////////////////////////////////////////////////
// enum DSiegeUtils::eExpErrType implementation

static const char* s_ExpErrTypeStrings[] =
{
	"EXP_SUCCESS",
	"EXP_ERROR_UNKNOWN",
	"EXP_ERROR_MANGLED_CUSTOM_ATTS",
	"EXP_ERROR_MANGLED_CUSTOM_MODIFIER",
	"EXP_ERROR_NO_DIRECTORY",
	"EXP_ERROR_CANNOT_CREATE_DIRECTORY",
	"EXP_ERROR_FILE_EXISTS",
	"EXP_ERROR_FILE_LOCKED",
	"EXP_ERROR_CANNOT_CREATE_FILE",
	"EXP_ERROR_NOTHING_SELECTED",
	"EXP_ERROR_NO_CUSTOM_CONTAINER",
	"EXP_ERROR_IN_POSTPROCESS",
	"EXP_ERROR_EXCEPTION",
	"EXP_ERROR_NO_BONES",
	"EXP_ERROR_BUSTED_GLOBALS",
	"EXP_ERROR_NO_MATERIAL",
	"EXP_ERROR_CANNOT_CONVERT_TO_TRIOBJECT",
	"EXP_ERROR_NEGATIVE_PARITY",
	"EXP_ERROR_CANNOT_DETERMINE_FILENAME",
	"EXP_ERROR_CANNOT_WRITE_TO_DIRECTORY",
	"EXP_ERROR_FILE_COPY_FAILED",
	"EXP_ERROR_CANNOT_DETERMINE_DESTINATION",
	"EXP_ERROR_CAUGHT_INTERNAL_EXCEPTION",
	"EXP_ERROR_FILE_FAILED_TO_CLOSE",
	"EXP_ERROR_EXPORT_CANCELLED",
	"EXP_ERROR_ANIMATION_KEY_REDUCTION_FAILED"
};

COMPILER_ASSERT( ELEMENT_COUNT( s_ExpErrTypeStrings ) == DSiegeUtils::EXPERR_COUNT );

static stringtool::EnumStringConverter s_ExpErrTypeConverter( s_ExpErrTypeStrings, DSiegeUtils::EXPERR_BEGIN, DSiegeUtils::EXPERR_END );

const char* DSiegeUtils::ToString( DSiegeUtils::eExpErrType e )
	{  return ( s_ExpErrTypeConverter.ToString( e ) );  }

bool DSiegeUtils::FromString( const char* str, DSiegeUtils::eExpErrType& e )
	{  return ( s_ExpErrTypeConverter.FromString( str, rcast <int&> ( e ) ) );  }
