/*==================================================================================================
 
 DSmxs.cpp
 
 purpose: Provide support for maxscript
 
 author:  biddle
 
 (c)opyright Gas Powered Games, 2002
 
---------------------------------------------------------------------------------------------------*/

#include "FileSys.h"
#include "ReportSys.h"
#include "maxhelpers.h"
#include "Config.h"
#include "NamingKey.h"
#include "DSiegeUtils.h"

#include "BipExp.h"

#include <strings.h>
#include <numbers.h>
#include <parser.h>
#include <definsfn.h>

#include "resource.h"

#include "DSmxs.h"

//*********************************************************
def_visible_primitive( dsHello,"dsHello");
Value* dsHello_cf(Value** arg_list, int count)
{
	gpgeneric("================================================================================\n");
	gpgenericf(("DSiegeUtils compiled at: %s\n",__TIMESTAMP__));
	gpgeneric("--------------------------------------------------------------------------------\n"); 
	gpgeneric("SiegeMAX extensions available:\n"); 
	gpgeneric("================================================================================\n");
	gpgeneric("dsLoadNamingKey <naming key file name>\n");
	gpgeneric("dsPrintNamingKey\n");
	gpgeneric("dsBuildContentLocation <content name in NNK form>\n");
	gpgeneric("dsBuildContentDescription <content name in NNK form>\n");
	gpgeneric("dsGetFileSize <content name in NNK form>\n");
	gpgeneric("dsPathLookup <pathvar string>\n");
	gpgeneric("dsExtendNamingKey <new nnk filename>\n");
	gpgeneric("--------------------------------------------------------------------------------\n"); 
	gpgeneric("dsExportSelected [noPrompt:false] [canStomp:false] [makePath:false]\n");
	gpgeneric("dsPreviewSelected [newView:false]\n");
	gpgeneric("--------------------------------------------------------------------------------\n"); 
	gpgeneric("dsIsDummyObject <node>\n");
	gpgeneric("dsIsValidBone <node>\n");
	gpgeneric("dsIsBipedBone <node>\n");
	gpgeneric("dsFetchValidParent <node>\n");
	gpgeneric("dsFetchTopmostBone <node>\n");
	gpgeneric("dsGetLinkControlledParentAtTime <node> <time>\n");
	gpgeneric("--------------------------------------------------------------------------------\n"); 
	gpgeneric("dsFourCCStringToInt <string>\n");
	gpgeneric("dsFourCCIntToString <int>\n");
	gpgeneric("--------------------------------------------------------------------------------\n"); 
	gpgeneric("dsDisplayReport <array of strings>\n");
	gpgeneric("--------------------------------------------------------------------------------\n"); 
	gpgeneric("dsQueryBox <message_string> [ title:<window_title_string> ] \\\n");
	gpgeneric("                            [ beep:<boolean> ] \\\n");
	gpgeneric("                            [ default:<boolean> ]\n");
	gpgeneric("--------------------------------------------------------------------------------\n"); 
	gpgeneric("dsUpperCase <string>\n");
	gpgeneric("dsLowerCase <string>\n");
	gpgeneric("--------------------------------------------------------------------------------\n"); 
	gpgeneric("dsDoesAnimViewerExist\n");
	gpgeneric("dsDoesNodeViewerExist\n");
	gpgeneric("--------------------------------------------------------------------------------\n"); 
	gpgeneric("dsSetMeshSelectSubLevel <scripted modifier that extends mesh_select> <level>\n");
	gpgeneric("dsSetMeshSelected <scripted modifier that extends mesh_select> <bitarray> <0|1|2>\n");
	gpgeneric("--------------------------------------------------------------------------------\n\n"); 

	int nnkcount = gNamingKey.GetNumFileNames();

	if (nnkcount == 0)
	{
		gpgeneric("Naming Key is not loaded\n"); 
	}
	else
	{
		for (int i = 0; i<nnkcount; ++i)
		{
			gpgenericf(("Naming Key loaded from [%s]\n",gNamingKey.GetFileName(i).c_str()));
		}
	}

	
	if (GetSiegeUtils()->DoesAnimViewerExist())
	{
		gpgeneric("\nAn Anim Previewer is available\n"); 
	}
	else
	{
		gpgeneric("\nAn Anim Previewer is NOT available, ASP & PRS files cannot be previewed\n"); 
	}

	if (GetSiegeUtils()->DoesNodeViewerExist())
	{
		gpgeneric("A Node Previewer is available\n"); 
	}
	else
	{
		gpgeneric("An Node Previewer is NOT available, SNO files cannot be previewed\n"); 
	}


	gConfig.DumpPathVars( &gGenericContext );

	return &true_value;
}

//*********************************************************
def_visible_primitive( dsLoadNamingKey,"dsLoadNamingKey");
Value* dsLoadNamingKey_cf(Value** arg_list, int count)
{
	check_arg_count(dsLoadNamingKey,0, count);

	gpstring nnkDir = "art/";

	if ( gNamingKey.LoadNamingKey( nnkDir ) )
	{
		return &true_value;
	}

	return &false_value;

}

//*********************************************************
def_visible_primitive( dsBuildContentLocation,"dsBuildContentLocation");
Value* dsBuildContentLocation_cf(Value** arg_list, int count)
{
	check_arg_count(dsBuildContentLocation,1, count);
	TCHAR* arg0 = arg_list[0]->to_string();
	gpstring lookup = FileSys::GetFileNameOnly(arg0);

	if (!lookup.empty())
	{
		if (!gNamingKey.GetNumFileNames())
		{
			gpstring nnkDir = "art/";

			if ( !gNamingKey.LoadNamingKey( nnkDir ) )
			{
				return &undefined;
			}
		}
		gpstring dest;
		if ( gNamingKey.BuildContentLocation(lookup,dest) )
		{
			one_typed_value_local(String* result);
		
			TCHAR* buffer = new TCHAR[dest.size()+1];
			strcpy(buffer,dest.c_str());
			vl.result = new String(buffer);

			return vl.result;
		}

	}

	return &undefined;

}

//*********************************************************
def_visible_primitive( dsBuildContentDescription,"dsBuildContentDescription");
Value* dsBuildContentDescription_cf(Value** arg_list, int count)
{
	check_arg_count(dsBuildContentDescription,1, count);
	TCHAR* arg0 = arg_list[0]->to_string();
	gpstring lookup = FileSys::GetFileNameOnly(arg0);

	if (!lookup.empty())
	{
		if (!gNamingKey.GetNumFileNames())
		{
			gpstring nnkDir = "art/";

			if ( !gNamingKey.LoadNamingKey( nnkDir ) )
			{
				return &undefined;
			}
		}

		std::list<gpstring> desc;
		if ( gNamingKey.BuildContentDescription(lookup,desc) )
		{
			two_typed_value_locals(Array* result, String* item);

			vl.result = new Array(0);
			for2 (std::list<gpstring>::iterator it = desc.begin() ; it != desc.end() ; ++it)
			{
				vl.item = new String((*it).begin_split());
				vl.result->append(vl.item);
			}
		
			pop_value_locals();
			return_value(vl.result);
		}

	}

	return &undefined;

}

//*********************************************************
def_visible_primitive( dsPrintNamingKey,"dsPrintNamingKey");
Value* dsPrintNamingKey_cf(Value** arg_list, int count)
{
	gNamingKey.DumpTrees();
	return &true_value;
}

//*********************************************************
def_visible_primitive( dsGetFileSize,"dsGetFileSize");
Value* dsGetFileSize_cf(Value** arg_list, int count)
{
	check_arg_count(dsGetFileSize,1, count);
	gpstring lookup(arg_list[0]->to_string());

	if (lookup)
	{
		if (!gNamingKey.GetNumFileNames())
		{
			gpstring nnkDir = "art/";

			if ( !gNamingKey.LoadNamingKey( nnkDir ) )
			{
				return &undefined;
			}
		}

		gpstring name = FileSys::GetFileName(lookup);
		gpstring dest;
		if ( gNamingKey.BuildContentLocation(name,dest) )
		{
			dest = gConfig.ResolvePathVars("%out%/"+dest);
			FileSys::FileHandle fh = FileSys::IFileMgr::GetMasterFileMgr()->Open(dest);
			if (fh)
			{
				int sz = FileSys::IFileMgr::GetMasterFileMgr()->GetSize(fh);
				return Integer::intern(sz);
			}
		}
	}

	return &undefined;
}

//*********************************************************
def_visible_primitive( dsPathLookup,"dsPathLookup");
Value* dsPathLookup_cf(Value** arg_list, int count)
{
	check_arg_count(dsPathLookup,1, count);
	gpstring lookup(arg_list[0]->to_string());

	gpstring rezpath;
	
	if (lookup.find_first_of("%") == lookup.npos)
	{
		lookup = "%"+lookup+"%";
	}
	if ( gConfig.ResolvePathVars(rezpath,lookup,Config::ePathVarMode::PVM_USE_CONFIG|Config::ePathVarMode::PVM_REQUIRED) )
	{
		one_typed_value_local(String* result);
	
		TCHAR* buffer = new TCHAR[rezpath.size()+1];
		strcpy(buffer,rezpath.c_str());
		vl.result = new String(buffer);

		return vl.result;
	}
	else
	{
		return &undefined;
	}
}


//*********************************************************
def_visible_primitive( dsExtendNamingKey,"dsExtendNamingKey");
Value* dsExtendNamingKey_cf(Value** arg_list, int count)
{
	if (1 != count)
	{
		gpgeneric("Usage: dsExtendNamingKey \"your_nnk_filename\"\n");
		return &false_value;
	}

	if (!is_string(arg_list[0]))
	{
		gpgeneric("Error: NNK filename parameter must be string, did you forget the surrounding quotes?\n");
		gpgeneric("Usage: dsExtendNamingKey \"your_nnk_filename\"\n");
		return &false_value;
	}

	gpstring nnkfile(arg_list[0]->to_string());

	if (nnkfile.same_no_case("NamingKey",strlen("NamingKey")))
	{
		gpgeneric("ERROR: \"Extra\" NNK filenames cannot start with \"NamingKey\", please choose a different name\n");
		return &false_value;
	}

	nnkfile.assignf("%%out%%/art/%s.NNK",FileSys::GetFileNameOnly(nnkfile).c_str());

	gpstring nnkpath;
	
	if ( !gConfig.ResolvePathVars( nnkpath, nnkfile ) )
	{
		gpgeneric("ERROR: Unable to resolve ART path to place new NNK file\n");
		return &false_value;
	}

	// Does the directory exist yet?
	if ( !FileSys::DoesPathExist( FileSys::GetPathOnly( nnkpath,false) ) )
	{
		// Should we build a new dir?
		if ( !FileSys::CreateFullFileNameDirectory( nnkpath ) )
		{
			gpgeneric("ERROR: Cannot create directory for NNK files\n");
			return &false_value;
		}
	}

	if ( FileSys::DoesFileExist(nnkpath) )
	{
		gpgeneric("ERROR: The requested NNK file already exists\n");
		return &false_value;
	}

	FileSys::File NNKFile;

	if (!NNKFile.CreateNew( nnkpath, FileSys::File::ACCESS_READ_WRITE, FileSys::File::SHARE_NONE ) )
	{
		gpgeneric("ERROR: Cannot create the new NNK file. File access has failed\n");
		return &false_value;
	}

	NNKFile.WriteText("#\n");
	NNKFile.WriteText("# This NNK file was generated from within Siege Max by the dsExtendNamingKey utility function\n");
	NNKFile.WriteText("#\n");
	NNKFile.WriteText("# After editing this file (and saving!) you can use the following commmands in the Maxscript\n");
	NNKFile.WriteText("# Listener Console to verify your changes:\n");
	NNKFile.WriteText("#\n");
	NNKFile.WriteText("#  dsLoadNamingKey()                      -- will reload the Naming Key (including your changes)\n");
	NNKFile.WriteText("#  dsBuildContentLocation \"valid_name\"    -- returns the location of the file specified by <valid_name>\n");
	NNKFile.WriteText("#  dsBuildContentDescription \"valid_name\" -- returns the description of the file specified by <valid_name>\n");
	NNKFile.WriteText("#\n");
	NNKFile.WriteText("#  For reference, here are some examples of Naming Key index tree definitions:\n");
	NNKFile.WriteText("#\n");
	NNKFile.WriteText("# 	Prefix		Sub-Directory		Description\n");
	NNKFile.WriteText("# ----------------------------------------------------------------------------\n");
	NNKFile.WriteText("# TREE = m_i_bid ,	Biddle ,		\"Biddle's Item Meshes\"\n");
	NNKFile.WriteText("# TREE = m_c_bid ,	Biddle ,		\"Biddle's Character Meshes\"\n");
	NNKFile.WriteText("# TREE = t_bid ,	Biddle ,		\"Biddle's Terrain\"\n");
	NNKFile.WriteText("# TREE = b_bid ,	Biddle ,		\"Biddle's Bitmaps\"\n");
	NNKFile.WriteText("#\n");
	NNKFile.WriteText("#========== Enter your additional TREE definitions below =====================\n\n");	

	NNKFile.Close();

	STARTUPINFO startinfo;
	memset(&startinfo,0,sizeof(startinfo));
	startinfo.cb = sizeof(startinfo);

	PROCESS_INFORMATION procinfo;
	memset(&procinfo,0,sizeof(procinfo));
	
	gpstring cmdline;
	cmdline.assignf("notepad %s",nnkpath.c_str());

	gpstring launchpath;
	launchpath = FileSys::GetPathOnly(nnkpath,false);

	BOOL rv = CreateProcess(NULL,
							cmdline.begin_split(),
							NULL,
							NULL,
							FALSE,
							NORMAL_PRIORITY_CLASS,
							NULL,
							launchpath.c_str(),
							&startinfo,
							&procinfo
							);

	gpgenericf(("Created Naming Key file: %s\n",nnkpath.c_str()));

	return &true_value;
}


//*********************************************************
def_visible_primitive( dsExportSelected,"dsExportSelected");
Value* dsExportSelected_cf(Value** arg_list, int count)
{

	Value* arg;

	Value* n_noPrompt    = Name::intern(_T("noPrompt"));	
	Value* n_canStomp    = Name::intern(_T("canStomp"));	
	Value* n_makePath    = Name::intern(_T("makePath"));	

	bool noprompt		= !!bool_key_arg(noPrompt, arg, false);
	bool canstomp		= !!bool_key_arg(canStomp, arg, false);
	bool makepath		= !!bool_key_arg(makePath, arg, false);

	DSiegeUtils* dsu = GetSiegeUtils();

	int numexp = 0;
	if (dsu)
	{
		numexp = dsu->ExportSelected(noprompt,canstomp,makepath);
	}

	return Integer::intern(numexp);
}

//*********************************************************
def_visible_primitive( dsPreviewSelected,"dsPreviewSelected");
Value* dsPreviewSelected_cf(Value** arg_list, int count)
{

	Value* arg;

	Value* n_newView    = Name::intern(_T("newView"));	
	bool newview		= !!bool_key_arg(newView, arg, false);

	DSiegeUtils* dsu = GetSiegeUtils();
	int numexp = 0;
	if (dsu)
	{
		numexp = dsu->ExportForPreview(newview);
	}

	return Integer::intern(numexp);
}

//*********************************************************
Value* RunMaxScriptCommand(TCHAR* command)
{
	
	Value* ret = 0;
	
	three_typed_value_locals(StringStream* source, Parser* parser, Value* code);
	init_thread_locals();
	push_alloc_frame();
	vl.source = new StringStream (command);
	vl.parser = new Parser (thread_local(current_stdout));

	try
	{
		Interface* ip = GetCOREInterface();
		ip->RedrawViews(ip->GetTime(),REDRAW_BEGIN);

		vl.source->flush_whitespace();
		vl.code = vl.parser->compile_all(vl.source);
		vl.source->close();
		ret = vl.code->eval()->make_heap_permanent();

		ip->RedrawViews(ip->GetTime(),REDRAW_END);
	}
	catch (...)
	{
		gpgenericf(("Error: Unable to execute Maxscript function \"%s\"\n",command));
		ret = &undefined;
	}
	pop_alloc_frame();
	pop_value_locals();
	return (ret) ? ret : &undefined;
}

//*********************************************************
def_visible_primitive( dsFourCCStringToInt,"dsFourCCStringToInt");
Value* dsFourCCStringToInt_cf(Value** arg_list, int count)
{
	check_arg_count(dsFourCCStringToInt,1, count);
	gpstring fourcc(arg_list[0]->to_string());

	return Integer::intern(FourCCToDWORD(fourcc.c_str()));
}

//*********************************************************
def_visible_primitive( dsFourCCIntToString,"dsFourCCIntToString");
Value* dsFourCCIntToString_cf(Value** arg_list, int count)
{
	check_arg_count(dsFourCCToString,1, count);
	DWORD value(arg_list[0]->to_int());

	gpstring fourcc = DWORDToFourCC(value);

	one_typed_value_local(String* result);

	TCHAR* buffer = new TCHAR[fourcc.size()+1];
	strcpy(buffer,fourcc.c_str());
	vl.result = new String(buffer);

	return vl.result;
}

//*********************************************************
def_visible_primitive( dsGetLinkControlledParentAtTime,"dsGetLinkControlledParentAtTime");
Value* dsGetLinkControlledParentAtTime_cf(Value** arg_list, int count) {

	check_arg_count(dsGetLinkControlledParentAtTime, 2, count);

	INode* n = arg_list[0]->to_node();
	TimeValue t = arg_list[1]->to_timevalue();

	if (n) { 

		// Get the node's transform control
		Control *c = n->GetTMController();

		// Is it animated with a link controller?
		if (c && (c->ClassID() == LINKCTRL_CLASSID)) {

			ILinkCtrl* lc = (ILinkCtrl*)c;

			int cnt = lc->GetNumTargets();

			int active_parent = 0;

			for (int p = 0; p < cnt; p++) {

				TimeValue currtime = lc->GetLinkTime(p);

				if (currtime <= t) {
					active_parent = p+1;
				} else {
					break;
				}
			}

			int r = lc->NumRefs();

			if ((active_parent > 0) && (active_parent < lc->NumRefs())) {

				INode* pnode = (INode*)lc->GetReference(active_parent);
		
				one_typed_value_local(MAXNode* result);
				vl.result = new MAXNode(pnode);
				return_value(vl.result);

			}
		}
	}

	return &false_value;

}


//*********************************************************
def_visible_primitive( dsFetchValidParent,"dsFetchValidParent");
Value* dsFetchValidParent_cf(Value** arg_list, int count)
{
	check_arg_count(dsFetchValidParent, 1, count);
	INode* kid = arg_list[0]->to_node();

	if (!kid)
	{
		return &undefined;
	}

	INode* vparent = FetchValidParent(kid);

	if (!vparent)
	{
		return &undefined;
	}
	
	one_typed_value_local(MAXNode* result);
	vl.result = new MAXNode(vparent);
	return_value(vl.result);

}

//*********************************************************
def_visible_primitive( dsIsValidBone,"dsIsValidBone");
Value* dsIsValidBone_cf(Value** arg_list, int count)
{
	check_arg_count(dsIsValidBone, 1, count);
	INode* bone = arg_list[0]->to_node();

	return IsValidBone(bone) ? &true_value : &false_value;
}

//*********************************************************
def_visible_primitive( dsIsBipedBone,"dsIsBipedBone");
Value* dsIsBipedBone_cf(Value** arg_list, int count)
{
	check_arg_count(dsIsBipedBone, 1, count);
	INode* bone = arg_list[0]->to_node();

	if (bone) 
	{
		Control *c = bone->GetTMController();
		if (c)
		{
			Class_ID cid = c->ClassID();
			if ((cid == BIPSLAVE_CONTROL_CLASS_ID) ||
				(cid == BIPBODY_CONTROL_CLASS_ID)  ||
				(cid == FOOTPRINT_CLASS_ID))
			{
				return &true_value;
			}
		}
	}

	return &false_value;
}

//*********************************************************
def_visible_primitive( dsIsDummyObject,"dsIsDummyObject");
Value* dsIsDummyObject_cf(Value** arg_list, int count)
{
	check_arg_count(dsIsDummyObject, 1, count);
	INode* kid = arg_list[0]->to_node();

	if (!kid)
	{
		return &false_value;
	}

	return IsDummyObject(kid) ? &true_value : &false_value;
}

//*********************************************************
def_visible_primitive( dsFetchTopmostBone,"dsFetchTopmostBone");
Value* dsFetchTopmostBone_cf(Value** arg_list, int count)
{
	check_arg_count(dsIsValidBone, 1, count);
	INode* kid = arg_list[0]->to_node();

	if (!kid)
	{
		return &undefined;
	}

	INode* tmb =  FetchTopmostBone(kid);

	if (!tmb)
	{
		return &undefined;
	}

	one_typed_value_local(MAXNode* result);
	vl.result = new MAXNode(tmb);
	return_value(vl.result);

}

struct ReportData
{
	gpstring Title;
	gpstring ReportLines;
};

//*********************************************************
BOOL CALLBACK DisplayReportDlgProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam) 
{

	HFONT fixedfont = NULL;

	switch(message) {
		case WM_INITDIALOG:
			{
				fixedfont = CreateFont(0,0,0,0,
					FW_DONTCARE,
					FALSE,
					FALSE,
					FALSE,
					ANSI_CHARSET,
					OUT_DEFAULT_PRECIS,
					CLIP_DEFAULT_PRECIS,
					DEFAULT_QUALITY,
					FF_DONTCARE|FIXED_PITCH,
					0
					);
				ReportData& rd = *(ReportData*)lParam;
				SetWindowText(hDlg,rd.Title.c_str());
				SetDlgItemText(hDlg,IDC_TEXTPANEL,rd.ReportLines.c_str());
				CenterWindow(hDlg,GetParent(hDlg));
				SendDlgItemMessage(hDlg,IDC_TEXTPANEL,WM_SETFONT,(WPARAM)fixedfont,(LPARAM)TRUE);
				SendDlgItemMessage(hDlg,IDC_TEXTPANEL,EM_SETSEL,(WPARAM)-1,(LPARAM)0);
				return TRUE;
			}

		case WM_COMMAND:
			switch(wParam)
			{
				case IDOK:
				DeleteObject(fixedfont);
				EndDialog(hDlg, 0);
				return true;
			}
			break;


		case WM_CLOSE:
			EndDialog(hDlg, 0);
			DeleteObject(fixedfont);
			return TRUE;
	}
	return FALSE;
}


//*********************************************************
def_visible_primitive( dsDisplayReport,"dsDisplayReport");
Value* dsDisplayReport_cf(Value** arg_list, int count)
{
	check_arg_count(dsDisplayReport, 2, count);

	TCHAR* reporttitle = arg_list[0]->to_string();

	ReportData rd;
	rd.Title = gpstring(reporttitle);

	if ( is_array(arg_list[1]) )
	{
		Array* a = (Array*)arg_list[1];
		for (int i = 0; i < (*a).size ; i++)
		{
			Value* elem = (*a)[i];
			if ( is_string(elem) )
			{
				rd.ReportLines.appendf( !rd.ReportLines.empty() ? "\r\n%s" : "%s" ,elem->to_string());
			}
		}
	}

	DialogBoxParam(hInstanceDLL, 
			MAKEINTRESOURCE(IDD_REPORT), 
			GetCOREInterface()->GetMAXHWnd(), 
			DisplayReportDlgProc, 
			(LPARAM)&rd
			);

	return &true_value;
}

//*********************************************************
def_visible_primitive( dsQueryBox,"dsQueryBox");
Value* dsQueryBox_cf(Value** arg_list, int count)
{

	check_arg_count_with_keys(dsQueryBox, 1, count);

	TCHAR* querytext = arg_list[0]->to_string();

	Value* arg;

	Value* n_beep		= Name::intern(_T("beep"));	
	Value* n_title		= Name::intern(_T("title"));	
	Value* n_default	= Name::intern(_T("default"));	

	bool playbeep	= !!bool_key_arg(beep, arg, false);

	TCHAR* titletext;
	Value* titlearg	= key_arg(title);
	if ( is_string(titlearg) )
	{
		titletext = titlearg->to_string();
	}
	else
	{
		titletext = "MAXScript";
	}
	
	bool default_button	= !!bool_key_arg(default, arg, true);

	int retval = MessageBox(
		GetCOREInterface()->GetMAXHWnd(),
		querytext,
		titletext,
		MB_YESNO|MB_ICONQUESTION|(default_button?MB_DEFBUTTON1:MB_DEFBUTTON2)
		);

	return (retval == IDYES) ? &true_value : &false_value;
}

//*********************************************************
def_visible_primitive( dsUpperCase,"dsUpperCase");
Value* dsUpperCase_cf(Value** arg_list, int count)
{
	check_arg_count(dsUpperCase, 1, count);

	TCHAR* intext = arg_list[0]->to_string();

	one_typed_value_local(String* result);
	TCHAR* buffer = new TCHAR[strlen(intext)+1];
	strcpy(buffer,intext);
	_strupr(buffer);
	vl.result = new String(buffer);

	return_value(vl.result);
}

//*********************************************************
def_visible_primitive( dsLowerCase,"dsLowerCase");
Value* dsLowerCase_cf(Value** arg_list, int count)
{
	check_arg_count(dsLowerCase, 1, count);

	TCHAR* intext = arg_list[0]->to_string();

	one_typed_value_local(String* result);
	TCHAR* buffer = new TCHAR[strlen(intext)+1];
	strcpy(buffer,intext);
	_strlwr(buffer);
	vl.result = new String(buffer);

	return_value(vl.result);
}

//*********************************************************
def_visible_primitive( dsSetMeshSelectSubLevel,"dsSetMeshSelectSubLevel");
Value* dsSetMeshSelectSubLevel_cf(Value** arg_list, int count)
{
	check_arg_count(dsSetMeshSelectSubLevel, 2, count);

	Modifier* mod = arg_list[0]->to_modifier();
	DWORD newlev = arg_list[1]->to_int();
	DWORD oldlev = 0;

	MSPlugin* msplug = (MSPlugin*)mod->GetInterface(I_MAXSCRIPTPLUGIN);

	if ( msplug )
	{
		if ( msplug->get_delegate() && 
			(msplug->get_delegate()->ClassID() == Class_ID(MESHSELECT_CLASS_ID,0)) )
		{
			MSModifierXtnd* msmodx = (MSModifierXtnd*)mod;
			if ((msmodx->delegate->NumSubObjTypes()+1) > newlev)
			{
				Interface* ip = msmodx->ip;
				if (ip)
				{
					oldlev = ip->GetSubObjectLevel();
					ip->SetSubObjectLevel(newlev);
				}
			}
		}
	}
	
	return Integer::intern(oldlev);
}
//*********************************************************
def_visible_primitive( dsSetMeshSelected,"dsSetMeshSelected");
Value* dsSetMeshSelected_cf(Value** arg_list, int count)
{
	check_arg_count(dsSetMeshSelected, 3, count);

	Modifier* mod = arg_list[0]->to_modifier();
	BitArray newbits = arg_list[1]->to_bitarray();
	int seltype = arg_list[2]->to_int();

	MSPlugin* msplug = (MSPlugin*)mod->GetInterface(I_MAXSCRIPTPLUGIN);

	if ( seltype<0 || seltype>3 )
	{
		return &false_value;
	}
	
	if ( msplug )
	{
		if ( msplug->get_delegate() && 
			(msplug->get_delegate()->ClassID() == Class_ID(MESHSELECT_CLASS_ID,0)) )
		{
			MSModifierXtnd* msmodx = (MSModifierXtnd*)mod;
			Interface* ip = msmodx->ip;
			if (ip)
			{

				ModContextList mcList;
				INodeTab nodes;
				ip->GetModContexts (mcList, nodes);

				IMeshSelect* IMS = 0;
				IMeshSelectData* IMSD = 0;

				for (int nd=0; nd<mcList.Count() && !IMSD; nd++)
				{
					if (!mcList[nd]->localData) continue;
					IMSD = (IMeshSelectData*)mcList[nd]->localData->GetInterface(I_MESHSELECTDATA);
				}

				if ( IMSD )
				{
					BitArray oldbits;

					IMeshSelect* IMS = (IMeshSelect*)msplug->get_delegate();

					switch (seltype)
					{
					case 1: 
						oldbits =  IMSD->GetVertSel();
						newbits.SetSize(oldbits.GetSize(),1);
						IMSD->SetVertSel(newbits,IMS,0);
						break;
					case 2:
						oldbits =  IMSD->GetEdgeSel();
						newbits.SetSize(oldbits.GetSize(),1);
						IMSD->SetEdgeSel(newbits,IMS,0);
						break;
					case 3:
						oldbits =  IMSD->GetFaceSel();
						newbits.SetSize(oldbits.GetSize(),1);
						IMSD->SetFaceSel(newbits,IMS,0);
						break;
					}

					if (ip->GetSubObjectLevel() == seltype)
					{
						mod->NotifyDependents(FOREVER, PART_DISPLAY, REFMSG_CHANGE);
						ip->RedrawViews(0);
					}

					one_typed_value_local(BitArrayValue* result);
					vl.result = new BitArrayValue(oldbits);
					return_value(vl.result);
				}
			}
		}
	}

/*
	virtual BitArray GetVertSel()=0;
	virtual BitArray GetFaceSel()=0;
	virtual BitArray GetEdgeSel()=0;
	
	virtual void SetVertSel(BitArray &set, IMeshSelect *imod, TimeValue t)=0;
	virtual void SetFaceSel(BitArray &set, IMeshSelect *imod, TimeValue t)=0;
	virtual void SetEdgeSel(BitArray &set, IMeshSelect *imod, TimeValue t)=0;
*/	
	return &false_value;
}

//*********************************************************
def_visible_primitive( dsDoesAnimViewerExist,"dsDoesAnimViewerExist");
Value* dsDoesAnimViewerExist_cf(Value** arg_list, int count)
{
	return GetSiegeUtils()->DoesAnimViewerExist() ? &true_value : &false_value;
}

//*********************************************************
def_visible_primitive( dsDoesNodeViewerExist,"dsDoesNodeViewerExist");
Value* dsDoesNodeViewerExist_cf(Value** arg_list, int count)
{
	return GetSiegeUtils()->DoesNodeViewerExist() ? &true_value : &false_value;
}

