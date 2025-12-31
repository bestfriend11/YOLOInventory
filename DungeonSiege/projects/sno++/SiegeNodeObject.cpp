/**********************************************************************
 *<
	FILE: SiegeNodeObject.cpp

	DESCRIPTION:	Appwizard generated plugin

	CREATED BY: Mike Biddlecombe

	HISTORY: 

 *>	Copyright (c) 1999, All Rights Reserved.
 **********************************************************************/

#include "precompiled_siegenodeobject.h"
#include "resource.h"

#ifdef gpassert
#undef gpassert
bool _gpassert(bool &,char const *,char *,unsigned int,char const *) {return false;}
bool _gpassert_expression_failed(void) {return false;}
#endif


//******************************************
// Statics
//******************************************


// Class vars
HWND SiegeNodeObject::hPanelDlg = NULL;
IObjParam *SiegeNodeObject::iObjParams = NULL;
SelectModBoxCMode* SiegeNodeObject::selectMode = NULL;

static SiegeNodeObjectClassDesc SiegeNodeObjectDesc;
ClassDesc* GetSiegeNodeObjectDesc() {return &SiegeNodeObjectDesc;}

SiegeNodeObjectCreateCallback siegenodeobjectCreateCB;

// Declare SiegeNodeObject bounding mesh pick callback
static PickSiegeNodeObjectBoundingMeshCallback pickBMeshCB;

void resetSiegeNodeObjectParams(BOOL fileReset) {
	SiegeNodeObject::hPanelDlg = NULL;
	SiegeNodeObject::iObjParams=NULL;
}

void SiegeNodeObjectClassDesc::ResetClassParams (BOOL fileReset) 
{
	resetSiegeNodeObjectParams(fileReset);
}


//******************************************
// SiegeNodeObject dialog callback 
//******************************************

//******************************************
// OBB Dialog Proc
//******************************************

static BOOL CALLBACK OBBSubPanelDlgProc(
	HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	UINT dwLevel;

	SiegeNodeObject *snode = (SiegeNodeObject*)GetWindowLong(hDlg,GWL_USERDATA);
	if (!snode && msg!=WM_INITDIALOG) return FALSE;

    switch (msg) {
		
    	case WM_INITDIALOG:

			snode = (SiegeNodeObject*)lParam;
			SetWindowLong(hDlg,GWL_USERDATA,lParam);
			SiegeNodeObject::hPanelDlg = hDlg;

			snode->d_boundaryOBB->Init(hDlg);
			break;

		case WM_DESTROY:

			snode->d_boundaryOBB->Destroy(hDlg);
			break;

		case WM_COMMAND:

			switch (LOWORD(wParam)) {

				case IDC_BUTTON1:

					// Determine what type of partition we will do
					snode->d_boundaryOBB->CheckForSeparateMeshes = (1 == SendDlgItemMessage(hDlg, IDC_FIND_SEPARATE_MESHES , BM_GETCHECK, 0 ,0));

					snode->d_boundaryOBB->Create(snode->iObjParams,*snode->d_boundaryMesh,snode->d_nodeMesh);

					return TRUE;

/*				case IDC_BUTTON2:
					theOBButil.BrowseFileName(hWnd);
					return TRUE;

				case IDC_BUTTON3:
					theOBButil.WriteOBBFile(hWnd);
					return TRUE;
*/
			}

			break;

			case WM_VSCROLL: {

				dwLevel = SendDlgItemMessage(hDlg, IDC_SLIDER1 , TBM_GETPOS, 0, 0);
				dwLevel = snode->d_boundaryOBB->SetRecursionLevel(dwLevel);

				char buff[8];
				sprintf(buff,"%d",dwLevel);

				SendDlgItemMessage(hDlg, IDC_RECURSELEVEL , WM_SETTEXT, 0,(long)buff);
				
				break;
			}

		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MOUSEMOVE:
			snode->iObjParams->RollupMouseMessage(hDlg,msg,wParam,lParam); 
			break;

		default:
			return FALSE;
	}
	return TRUE;
}

//******************************************
// Floor Dialog Proc
//******************************************

static BOOL CALLBACK FloorSubPanelDlgProc(
	HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{

	SiegeNodeObject *snode = (SiegeNodeObject*)GetWindowLong(hDlg,GWL_USERDATA);
	if (!snode && msg!=WM_INITDIALOG) return FALSE;

    switch (msg) {
		
    	case WM_INITDIALOG:

			snode = (SiegeNodeObject*)lParam;
			SetWindowLong(hDlg,GWL_USERDATA,lParam);
			SiegeNodeObject::hPanelDlg = hDlg;

			
			break;

		case WM_DESTROY:

			snode->GetBMesh()->DeselectAll();
			snode->NotifyDependents(FOREVER, SELECT_CHANNEL|SUBSEL_TYPE_CHANNEL, REFMSG_CHANGE);
			snode->iObjParams->RedrawViews(snode->iObjParams->GetTime());

	 		break;

		case WM_MOUSEACTIVATE:
			return FALSE;

		case WM_COMMAND:
		   
			switch( LOWORD(wParam) ) {
			
				case IDC_ADD_TO_FLOOR: {
					if (snode->GetBMesh()) {
						snode->GetBMesh()->AddFloor();
						snode->NotifyDependents(FOREVER, GEOM_CHANNEL|TOPO_CHANNEL|SELECT_CHANNEL|SUBSEL_TYPE_CHANNEL, REFMSG_CHANGE);
						snode->iObjParams->ForceCompleteRedraw();
//						snode->iObjParams->RedrawViews(snode->iObjParams->GetTime());
					}
					break;
				}
				case IDC_DELETE_FROM_FLOOR: {
					if (snode->GetBMesh())
						snode->GetBMesh()->DeleteFloor();
					snode->NotifyDependents(FOREVER, GEOM_CHANNEL|TOPO_CHANNEL|SELECT_CHANNEL|SUBSEL_TYPE_CHANNEL, REFMSG_CHANGE);
					snode->iObjParams->ForceCompleteRedraw();
//					snode->iObjParams->RedrawViews(snode->iObjParams->GetTime());
					break;
				}
				case IDC_RESET_FLOOR: {
					if (snode->GetBMesh())
						snode->GetBMesh()->ResetFloor();
					snode->NotifyDependents(FOREVER, GEOM_CHANNEL|TOPO_CHANNEL|SELECT_CHANNEL|SUBSEL_TYPE_CHANNEL, REFMSG_CHANGE);
					snode->iObjParams->ForceCompleteRedraw();
//					snode->iObjParams->RedrawViews(snode->iObjParams->GetTime());
					break;
				}
				case IDC_FETCH_SELECTED_FLOOR: {
					if (snode->GetBMesh())
						snode->GetBMesh()->SetFloorSelectFromOriginal();
					snode->NotifyDependents(FOREVER, GEOM_CHANNEL|TOPO_CHANNEL|SELECT_CHANNEL|SUBSEL_TYPE_CHANNEL, REFMSG_CHANGE);
					snode->iObjParams->ForceCompleteRedraw();
//					snode->iObjParams->RedrawViews(snode->iObjParams->GetTime());
					break;
				}

			}
			return FALSE;

			break;

    	case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MOUSEMOVE:
			snode->iObjParams->RollupMouseMessage(hDlg,msg,wParam,lParam); 
    		break;

		default:
			return FALSE;
	}
	return TRUE;

}

//******************************************
// Water Dialog Proc
//******************************************

static BOOL CALLBACK WaterSubPanelDlgProc(
	HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{

	SiegeNodeObject *snode = (SiegeNodeObject*)GetWindowLong(hDlg,GWL_USERDATA);
	if (!snode && msg!=WM_INITDIALOG) return FALSE;

    switch (msg) {
		
    	case WM_INITDIALOG:

			snode = (SiegeNodeObject*)lParam;
			SetWindowLong(hDlg,GWL_USERDATA,lParam);
			SiegeNodeObject::hPanelDlg = hDlg;

			
			break;

		case WM_DESTROY:

			snode->GetBMesh()->DeselectAll();
			snode->NotifyDependents(FOREVER, SELECT_CHANNEL|SUBSEL_TYPE_CHANNEL, REFMSG_CHANGE);
			snode->iObjParams->RedrawViews(snode->iObjParams->GetTime());

	 		break;

		case WM_MOUSEACTIVATE:
			return FALSE;

		case WM_COMMAND:
		   
			switch( LOWORD(wParam) ) {
			
				case IDC_ADD_TO_WATER: {
					if (snode->GetBMesh()) {
						snode->GetBMesh()->AddWater();
						snode->NotifyDependents(FOREVER, GEOM_CHANNEL|TOPO_CHANNEL|SELECT_CHANNEL|SUBSEL_TYPE_CHANNEL, REFMSG_CHANGE);
						snode->iObjParams->ForceCompleteRedraw();
//						snode->iObjParams->RedrawViews(snode->iObjParams->GetTime());
					}
					break;
				}
				case IDC_DELETE_FROM_WATER: {
					if (snode->GetBMesh())
						snode->GetBMesh()->DeleteWater();
					snode->NotifyDependents(FOREVER, GEOM_CHANNEL|TOPO_CHANNEL|SELECT_CHANNEL|SUBSEL_TYPE_CHANNEL, REFMSG_CHANGE);
					snode->iObjParams->ForceCompleteRedraw();
//					snode->iObjParams->RedrawViews(snode->iObjParams->GetTime());
					break;
				}
				case IDC_RESET_WATER: {
					if (snode->GetBMesh())
						snode->GetBMesh()->ResetWater();
					snode->NotifyDependents(FOREVER, GEOM_CHANNEL|TOPO_CHANNEL|SELECT_CHANNEL|SUBSEL_TYPE_CHANNEL, REFMSG_CHANGE);
					snode->iObjParams->ForceCompleteRedraw();
//					snode->iObjParams->RedrawViews(snode->iObjParams->GetTime());
					break;
				}
				case IDC_FETCH_SELECTED_WATER: {
					if (snode->GetBMesh())
						snode->GetBMesh()->SetWaterSelectFromOriginal();
					snode->NotifyDependents(FOREVER, GEOM_CHANNEL|TOPO_CHANNEL|SELECT_CHANNEL|SUBSEL_TYPE_CHANNEL, REFMSG_CHANGE);
					snode->iObjParams->ForceCompleteRedraw();
//					snode->iObjParams->RedrawViews(snode->iObjParams->GetTime());
					break;
				}

			}
			return FALSE;

			break;

    	case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MOUSEMOVE:
			snode->iObjParams->RollupMouseMessage(hDlg,msg,wParam,lParam); 
    		break;

		default:
			return FALSE;
	}
	return TRUE;

}

//******************************************
// Ignored Faces Dialog Proc
//******************************************

static BOOL CALLBACK IgnoredSubPanelDlgProc(
	HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{

	SiegeNodeObject *snode = (SiegeNodeObject*)GetWindowLong(hDlg,GWL_USERDATA);
	if (!snode && msg!=WM_INITDIALOG) return FALSE;

    switch (msg) {
		
    	case WM_INITDIALOG:

			snode = (SiegeNodeObject*)lParam;
			SetWindowLong(hDlg,GWL_USERDATA,lParam);
			SiegeNodeObject::hPanelDlg = hDlg;

			
			break;

		case WM_DESTROY:

			snode->GetBMesh()->DeselectAll();
			snode->NotifyDependents(FOREVER, SELECT_CHANNEL|SUBSEL_TYPE_CHANNEL, REFMSG_CHANGE);
			snode->iObjParams->ForceCompleteRedraw();
//			snode->iObjParams->RedrawViews(snode->iObjParams->GetTime());

	 		break;

		case WM_MOUSEACTIVATE:
			return FALSE;

		case WM_COMMAND:
		   
			switch( LOWORD(wParam) ) {
			
				case IDC_ADD_TO_IGNORED: {
					if (snode->GetBMesh()) {
						snode->GetBMesh()->AddIgnored();
						snode->NotifyDependents(FOREVER, GEOM_CHANNEL|TOPO_CHANNEL|SELECT_CHANNEL|SUBSEL_TYPE_CHANNEL, REFMSG_CHANGE);
						snode->iObjParams->ForceCompleteRedraw();
//						snode->iObjParams->RedrawViews(snode->iObjParams->GetTime());
					}
					break;
				}
				case IDC_DELETE_FROM_IGNORED: {
					if (snode->GetBMesh())
						snode->GetBMesh()->DeleteIgnored();
					snode->NotifyDependents(FOREVER, GEOM_CHANNEL|TOPO_CHANNEL|SELECT_CHANNEL|SUBSEL_TYPE_CHANNEL, REFMSG_CHANGE);
					snode->iObjParams->ForceCompleteRedraw();
//					snode->iObjParams->RedrawViews(snode->iObjParams->GetTime());
					break;
				}
				case IDC_RESET_IGNORED: {
					if (snode->GetBMesh())
						snode->GetBMesh()->ResetIgnored();
					snode->NotifyDependents(FOREVER, GEOM_CHANNEL|TOPO_CHANNEL|SELECT_CHANNEL|SUBSEL_TYPE_CHANNEL, REFMSG_CHANGE);
					snode->iObjParams->ForceCompleteRedraw();
//					snode->iObjParams->RedrawViews(snode->iObjParams->GetTime());
					break;
				}
				case IDC_FETCH_SELECTED_IGNORED: {
					if (snode->GetBMesh())
						snode->GetBMesh()->SetIgnoredSelectFromOriginal();
					snode->NotifyDependents(FOREVER, GEOM_CHANNEL|TOPO_CHANNEL|SELECT_CHANNEL|SUBSEL_TYPE_CHANNEL, REFMSG_CHANGE);
					snode->iObjParams->ForceCompleteRedraw();
//					snode->iObjParams->RedrawViews(snode->iObjParams->GetTime());
					break;
				}
			}
			return FALSE;

			break;

    	case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MOUSEMOVE:
			snode->iObjParams->RollupMouseMessage(hDlg,msg,wParam,lParam); 
    		break;

		default:
			return FALSE;
	}
	return TRUE;

}

//******************************************
// Locked Normals Dialog Proc
//******************************************

static BOOL CALLBACK NormsSubPanelDlgProc(
	HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{

	SiegeNodeObject *snode = (SiegeNodeObject*)GetWindowLong(hDlg,GWL_USERDATA);
	if (!snode && msg!=WM_INITDIALOG) return FALSE;

    switch (msg) {
		
    	case WM_INITDIALOG:

			snode = (SiegeNodeObject*)lParam;
			SetWindowLong(hDlg,GWL_USERDATA,lParam);
			SiegeNodeObject::hPanelDlg = hDlg;

			
			break;

		case WM_DESTROY:

			snode->GetBMesh()->DeselectAll();
			snode->NotifyDependents(FOREVER, SELECT_CHANNEL|SUBSEL_TYPE_CHANNEL, REFMSG_CHANGE);
			snode->iObjParams->ForceCompleteRedraw();
//			snode->iObjParams->RedrawViews(snode->iObjParams->GetTime());

	 		break;

		case WM_MOUSEACTIVATE:
			return FALSE;

		case WM_COMMAND:
		   
			switch( LOWORD(wParam) ) {
			
				case IDC_LOCK_NORMS: {
					if (snode->GetBMesh()) {
						snode->GetBMesh()->LockNorms();
						snode->NotifyDependents(FOREVER, GEOM_CHANNEL|TOPO_CHANNEL|SELECT_CHANNEL|SUBSEL_TYPE_CHANNEL, REFMSG_CHANGE);
						snode->iObjParams->ForceCompleteRedraw();
//						snode->iObjParams->RedrawViews(snode->iObjParams->GetTime());
					}
					break;
				}
				case IDC_UNLOCK_NORMS: {
					if (snode->GetBMesh())
						snode->GetBMesh()->UnlockNorms();
					snode->NotifyDependents(FOREVER, GEOM_CHANNEL|TOPO_CHANNEL|SELECT_CHANNEL|SUBSEL_TYPE_CHANNEL, REFMSG_CHANGE);
					snode->iObjParams->ForceCompleteRedraw();
//					snode->iObjParams->RedrawViews(snode->iObjParams->GetTime());
					break;
				}
				case IDC_RESET_NORMS: {
					if (snode->GetBMesh())
						snode->GetBMesh()->ResetNorms();
					snode->NotifyDependents(FOREVER, GEOM_CHANNEL|TOPO_CHANNEL|SELECT_CHANNEL|SUBSEL_TYPE_CHANNEL, REFMSG_CHANGE);
					snode->iObjParams->ForceCompleteRedraw();
//					snode->iObjParams->RedrawViews(snode->iObjParams->GetTime());
					break;
				}
			}
			return FALSE;

			break;

    	case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MOUSEMOVE:
			snode->iObjParams->RollupMouseMessage(hDlg,msg,wParam,lParam); 
    		break;

		default:
			return FALSE;
	}
	return TRUE;

}
//******************************************
// Spoofed Normals Dialog Proc
//******************************************

static BOOL CALLBACK SpoofedSubPanelDlgProc(
	HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{

	SiegeNodeObject *snode = (SiegeNodeObject*)GetWindowLong(hDlg,GWL_USERDATA);
	if (!snode && msg!=WM_INITDIALOG) return FALSE;

    switch (msg) {
		
    	case WM_INITDIALOG:

			snode = (SiegeNodeObject*)lParam;
			SetWindowLong(hDlg,GWL_USERDATA,lParam);
			SiegeNodeObject::hPanelDlg = hDlg;

			
			break;

		case WM_DESTROY:

			snode->GetBMesh()->DeselectAll();
			snode->NotifyDependents(FOREVER, SELECT_CHANNEL|SUBSEL_TYPE_CHANNEL, REFMSG_CHANGE);
			snode->iObjParams->ForceCompleteRedraw();
//			snode->iObjParams->RedrawViews(snode->iObjParams->GetTime());

	 		break;

		case WM_MOUSEACTIVATE:
			return FALSE;

		case WM_COMMAND:
		   
			switch( LOWORD(wParam) ) {
			
				case IDC_SPOOF_NORMS: {
					if (snode->GetBMesh()) {
						snode->GetBMesh()->SpoofNorms();
						snode->NotifyDependents(FOREVER, GEOM_CHANNEL|TOPO_CHANNEL|SELECT_CHANNEL|SUBSEL_TYPE_CHANNEL, REFMSG_CHANGE);
						snode->iObjParams->ForceCompleteRedraw();
//						snode->iObjParams->RedrawViews(snode->iObjParams->GetTime());
					}
					break;
				}
				case IDC_UNSPOOF_NORMS: {
					if (snode->GetBMesh())
						snode->GetBMesh()->UnspoofNorms();
					snode->NotifyDependents(FOREVER, GEOM_CHANNEL|TOPO_CHANNEL|SELECT_CHANNEL|SUBSEL_TYPE_CHANNEL, REFMSG_CHANGE);
					snode->iObjParams->ForceCompleteRedraw();
//					snode->iObjParams->RedrawViews(snode->iObjParams->GetTime());
					break;
				}
				case IDC_RESET_SPOOFED: {
					if (snode->GetBMesh())
						snode->GetBMesh()->ResetSpoofed();
					snode->NotifyDependents(FOREVER, GEOM_CHANNEL|TOPO_CHANNEL|SELECT_CHANNEL|SUBSEL_TYPE_CHANNEL, REFMSG_CHANGE);
					snode->iObjParams->ForceCompleteRedraw();
//					snode->iObjParams->RedrawViews(snode->iObjParams->GetTime());
					break;
				}
			}
			return FALSE;

			break;

    	case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MOUSEMOVE:
			snode->iObjParams->RollupMouseMessage(hDlg,msg,wParam,lParam); 
    		break;

		default:
			return FALSE;
	}
	return TRUE;

}

//******************************************
// Spot Dialog Proc

//******************************************
// Door Dialog Proc
//******************************************

static BOOL CALLBACK DoorSubPanelDlgProc(
	HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{

	SiegeNodeObject *snode = (SiegeNodeObject*)GetWindowLong(hDlg,GWL_USERDATA);
	if (!snode && msg!=WM_INITDIALOG) return FALSE;

    switch (msg) {
		
    	case WM_INITDIALOG:

			snode = (SiegeNodeObject*)lParam;
			SetWindowLong(hDlg,GWL_USERDATA,lParam);
			SiegeNodeObject::hPanelDlg = hDlg;
/*
			SetDlgFont( hDlg, snode->iObjParams->GetAppHFont() );
			if( iBut = GetICustButton(GetDlgItem(hDlg, IDC_PICK_BOUNDING_MESH_BUTTON))) {
	  		iBut->SetType (CBT_CHECK);
			iBut->SetHighlightColor (GREEN_WASH);
			ReleaseICustButton(iBut);
			}
			
			SetDlgItemText(hDlg, IDC_BOUNDING_MESH_NAME, "");
*/
			break;

		case WM_DESTROY:

			if (snode && snode->GetBMesh()) {
				snode->GetBMesh()->DeselectAll();
			}

			snode->NotifyDependents(FOREVER, SELECT_CHANNEL|SUBSEL_TYPE_CHANNEL, REFMSG_CHANGE);
			snode->iObjParams->RedrawViews(snode->iObjParams->GetTime());


	 		break;

		case WM_MOUSEACTIVATE:
			return FALSE;

		case WM_COMMAND:
		   
			switch( LOWORD(wParam) ) {
			
				case IDC_ADD_DOOR: {
					if (snode && snode->GetBMesh()) {
						snode->GetBMesh()->AddDoor();
						snode->NotifyDependents(FOREVER, GEOM_CHANNEL|TOPO_CHANNEL|SELECT_CHANNEL|SUBSEL_TYPE_CHANNEL, REFMSG_CHANGE);
					}
					snode->iObjParams->ForceCompleteRedraw();
//					snode->iObjParams->RedrawViews(snode->iObjParams->GetTime());
					break;
				}
				case IDC_DELETE_DOOR: {
					if (snode && snode->GetBMesh()) {
						snode->GetBMesh()->DeleteDoor();
						snode->NotifyDependents(FOREVER, GEOM_CHANNEL|TOPO_CHANNEL|SELECT_CHANNEL|SUBSEL_TYPE_CHANNEL, REFMSG_CHANGE);
					}
					snode->iObjParams->ForceCompleteRedraw();
//					snode->iObjParams->RedrawViews(snode->iObjParams->GetTime());
					break;
				}
				case IDC_FLIP_DOOR: {
					if (snode && snode->GetBMesh()) {
						snode->GetBMesh()->FlipDoor();
						snode->NotifyDependents(FOREVER, GEOM_CHANNEL|TOPO_CHANNEL|SELECT_CHANNEL|SUBSEL_TYPE_CHANNEL, REFMSG_CHANGE);
					}
					snode->iObjParams->ForceCompleteRedraw();
//					snode->iObjParams->RedrawViews(snode->iObjParams->GetTime());
					break;
				}
				case IDC_ORIENT_DOOR: {
					if (snode && snode->GetBMesh()) {
						snode->GetBMesh()->OrientDoor();
						snode->NotifyDependents(FOREVER, GEOM_CHANNEL|TOPO_CHANNEL|SELECT_CHANNEL|SUBSEL_TYPE_CHANNEL, REFMSG_CHANGE);
					}
					snode->iObjParams->ForceCompleteRedraw();
//					snode->iObjParams->RedrawViews(snode->iObjParams->GetTime());
					break;
				}
				case IDC_MOVE_START_VERT: {
					if (snode && snode->GetBMesh()) {
						snode->GetBMesh()->AdvanceStartVert();
						snode->NotifyDependents(FOREVER, GEOM_CHANNEL|TOPO_CHANNEL|SELECT_CHANNEL|SUBSEL_TYPE_CHANNEL, REFMSG_CHANGE);
					}
					snode->iObjParams->ForceCompleteRedraw();
//					snode->iObjParams->RedrawViews(snode->iObjParams->GetTime());
					break;
				}
 
  			}
			return FALSE;

			break;

    	case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MOUSEMOVE:
			snode->iObjParams->RollupMouseMessage(hDlg,msg,wParam,lParam); 
    		break;

		default:
			return FALSE;
	}
	return TRUE;

}

//******************************************
// Internal Door Dialog Proc
//******************************************

static BOOL CALLBACK InternalDoorSubPanelDlgProc(
	HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{

	SiegeNodeObject *snode = (SiegeNodeObject*)GetWindowLong(hDlg,GWL_USERDATA);
	if (!snode && msg!=WM_INITDIALOG) return FALSE;

    switch (msg) {
		
    	case WM_INITDIALOG:

			snode = (SiegeNodeObject*)lParam;
			SetWindowLong(hDlg,GWL_USERDATA,lParam);
			SiegeNodeObject::hPanelDlg = hDlg;
/*
			SetDlgFont( hDlg, snode->iObjParams->GetAppHFont() );
			if( iBut = GetICustButton(GetDlgItem(hDlg, IDC_PICK_BOUNDING_MESH_BUTTON))) {
	  		iBut->SetType (CBT_CHECK);
			iBut->SetHighlightColor (GREEN_WASH);
			ReleaseICustButton(iBut);
			}
			
			SetDlgItemText(hDlg, IDC_BOUNDING_MESH_NAME, "");
*/

			break;

		case WM_DESTROY:

			if (snode && snode->GetBMesh()) {
				snode->GetBMesh()->ClearIDoorSelection();
			}

			snode->NotifyDependents(FOREVER, SELECT_CHANNEL|SUBSEL_TYPE_CHANNEL, REFMSG_CHANGE);
			snode->iObjParams->RedrawViews(snode->iObjParams->GetTime());


	 		break;

		case WM_MOUSEACTIVATE:
			return FALSE;

		case WM_COMMAND:
		   
			switch( LOWORD(wParam) ) {
			
				case IDC_ADD_DOOR: {
					if (snode && snode->GetBMesh()) {
						snode->GetBMesh()->AddIDoor();
						snode->NotifyDependents(FOREVER, GEOM_CHANNEL|TOPO_CHANNEL|SELECT_CHANNEL|SUBSEL_TYPE_CHANNEL, REFMSG_CHANGE);
					}
					snode->iObjParams->ForceCompleteRedraw();
//					snode->iObjParams->RedrawViews(snode->iObjParams->GetTime());
					break;
				}
				case IDC_DELETE_DOOR: {
					if (snode && snode->GetBMesh()) {
						snode->GetBMesh()->DeleteIDoor();
						snode->NotifyDependents(FOREVER, GEOM_CHANNEL|TOPO_CHANNEL|SELECT_CHANNEL|SUBSEL_TYPE_CHANNEL, REFMSG_CHANGE);
					}
					snode->iObjParams->ForceCompleteRedraw();
//					snode->iObjParams->RedrawViews(snode->iObjParams->GetTime());
					break;
				}
				case IDC_FLIP_DOOR: {
					if (snode && snode->GetBMesh()) {
						snode->GetBMesh()->FlipIDoor();
						snode->NotifyDependents(FOREVER, GEOM_CHANNEL|TOPO_CHANNEL|SELECT_CHANNEL|SUBSEL_TYPE_CHANNEL, REFMSG_CHANGE);
					}
					snode->iObjParams->ForceCompleteRedraw();
//					snode->iObjParams->RedrawViews(snode->iObjParams->GetTime());
					break;
				}
				case IDC_ORIENT_DOOR: {
					if (snode && snode->GetBMesh()) {
						snode->GetBMesh()->OrientIDoor();
						snode->NotifyDependents(FOREVER, GEOM_CHANNEL|TOPO_CHANNEL|SELECT_CHANNEL|SUBSEL_TYPE_CHANNEL, REFMSG_CHANGE);
					}
					snode->iObjParams->ForceCompleteRedraw();
//					snode->iObjParams->RedrawViews(snode->iObjParams->GetTime());
					break;
				}
				case IDC_CLEAR_SELECTION: {
					if (snode && snode->GetBMesh()) {
						snode->GetBMesh()->ClearIDoorSelection();
					}
					snode->iObjParams->ForceCompleteRedraw();
//					snode->iObjParams->RedrawViews(snode->iObjParams->GetTime());
					break;
				}
 
  			}
			return FALSE;

			break;

    	case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MOUSEMOVE:
			snode->iObjParams->RollupMouseMessage(hDlg,msg,wParam,lParam); 
    		break;

		default:
			return FALSE;
	}
	return TRUE;

}
//******************************************
// Object Level Dialog Proc
//******************************************

static BOOL CALLBACK SiegeNodeObjectDlgProc(
		HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	ICustButton *iBut;
	SiegeNodeObject *snode = (SiegeNodeObject*)GetWindowLong(hDlg,GWL_USERDATA);
	if (!snode && msg!=WM_INITDIALOG) return FALSE;

	switch (msg) {
		
		case WM_INITDIALOG:
			
			snode = (SiegeNodeObject*)lParam;
			SetWindowLong(hDlg,GWL_USERDATA,lParam);
			SiegeNodeObject::hPanelDlg = hDlg;

			SetDlgFont( hDlg, snode->iObjParams->GetAppHFont() );
			if( iBut = GetICustButton(GetDlgItem(hDlg, IDC_PICK_BOUNDING_MESH_BUTTON))) {
			iBut->SetType (CBT_CHECK);
				iBut->SetHighlightColor (GREEN_WASH);
				ReleaseICustButton(iBut);
			}
			
			SetDlgItemText(hDlg, IDC_BOUNDING_MESH_NAME, "");

			break;

		case WM_DESTROY:
			break;

		case WM_MOUSEACTIVATE:
			return FALSE;

		case WM_COMMAND:			
			switch( LOWORD(wParam) ) {
				
			case IDC_PICK_BOUNDING_MESH_BUTTON:
				if(!snode->GetCreated()) {
					
					if(iBut = GetICustButton(GetDlgItem(hDlg, IDC_PICK_BOUNDING_MESH_BUTTON))) {
						iBut->SetCheck(FALSE);
						ReleaseICustButton(iBut);
					}

					break;
				}

				pickBMeshCB.d_siegeNodeObj = snode;
				pickBMeshCB.hDlg = hDlg;

				if(pickBMeshCB.doingPick) {
					snode->iObjParams->SetCommandMode(pickBMeshCB.cm);
				} else {
					pickBMeshCB.cm = snode->iObjParams->GetCommandMode();
				}

				snode->iObjParams->SetPickMode (&pickBMeshCB);
				snode->UpdateUI(snode->iObjParams->GetTime());
				snode->iObjParams->RedrawViews (snode->iObjParams->GetTime());

				break;
			}
			return FALSE;
			break;

		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MOUSEMOVE:
			snode->iObjParams->RollupMouseMessage(hDlg,msg,wParam,lParam); 
			break;

		default:
			return FALSE;
	}
	return TRUE;
}

typedef BOOL (CALLBACK dlg_proc)(HWND,UINT,WPARAM,LPARAM);

// Selection levels
enum {
	SEL_OBJECT = 0,
	SEL_DOOR,
	SEL_FLOOR,
	SEL_WATER,
	SEL_IDOOR,
	SEL_IGNORED,
	SEL_NORMALS,
	SEL_SPOOFED,
	SEL_SPOT,
	SEL_OBB,
	MAX_LEVELS,
};

int SubPanelResources[MAX_LEVELS] = {
	IDD_PANEL,
	IDD_DOOR_SUBPANEL1,	// Original panel
	IDD_FLOOR_SUBPANEL,
	IDD_WATER_SUBPANEL,
	IDD_IDOOR_SUBPANEL,
	IDD_IGNORED_SUBPANEL,
	IDD_NORMALS_SUBPANEL,
	IDD_NORMALS_SUBPANEL1,
	IDD_OBB_SUBPANEL
};

dlg_proc* SubPanelProcs[MAX_LEVELS] = {
	&SiegeNodeObjectDlgProc,
	&DoorSubPanelDlgProc,
	&FloorSubPanelDlgProc,
	&WaterSubPanelDlgProc,
	&InternalDoorSubPanelDlgProc,
	&IgnoredSubPanelDlgProc,
	&NormsSubPanelDlgProc,
	&SpoofedSubPanelDlgProc,
	&OBBSubPanelDlgProc
};

char *SubPanelName[MAX_LEVELS] = {
	"Siege Node",
	"Edit Door",
	"Edit Floor",
	"Edit Water",
	"Edit I-Door",
	"Edit Ignored",
	"Locked Normals",
	"Spoofed Normals",
	"Edit OBB"
};

const TCHAR *SubPanelType[] = { "Doors", "Floors" , "Water", "I-Doors", "Ignored","Locked","Spoofed", "OBB" };
const int SubPanelCount = 6; // No more access to OBB -- biddle


//******************************************
// Existence
//******************************************

//******************************************
SiegeNodeObject::SiegeNodeObject(void)
	: d_created(false)
	, d_nodeMesh(NULL)
	, d_triMesh(NULL)
	, d_boundaryMesh(NULL)
	, d_boundaryOBB(NULL)
	, d_UserWasWarned(false)

{
}

//******************************************
SiegeNodeObject::~SiegeNodeObject(void)
{

	DeleteAllRefs();
	delete d_boundaryMesh;
	delete d_boundaryOBB;

}

//******************************************
// From BaseObject
//******************************************

//******************************************
int SiegeNodeObject::HitTest(TimeValue t, INode *inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt) {

	Matrix3 tm(1);	
	HitRegion hitRegion;
	DWORD	savedLimits;
	Point3 pt(0,0,0);

   	tm = inode->GetObjectTM(t);		
	vpt->getGW()->setTransform(tm);
	GraphicsWindow *gw = vpt->getGW();	

	MakeHitRegion(hitRegion, type, crossing, 4, p);
	gw->setHitRegion(&hitRegion);

	gw->setRndLimits(((savedLimits = gw->getRndLimits())|GW_PICK)&~GW_ILLUM);
	gw->clearHitCode();

	vpt->getGW()->marker(&pt,TRIANGLE_MRKR);

	int res = gw->checkHitCode();

	gw->clearHitCode();
	gw->setRndLimits(savedLimits);
	
//	if((hitRegion.type != POINT_RGN) && !hitRegion.crossing)
//		return TRUE;

	return res;
}

//******************************************
//* Sub-Object Hit Testing
int SiegeNodeObject::HitTest(TimeValue t, INode *inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt, ModContext *mc) {

	BoundaryMesh* bmesh = d_boundaryMesh;
	
	GraphicsWindow *gw = vpt->getGW();
	
	HitRegion hr;
	
	MakeHitRegion(hr,type,crossing,4,p);

	gw->setHitRegion(&hr);
	
	int savedLimits = gw->getRndLimits();
	
	gw->setRndLimits((savedLimits |GW_PICK|GW_BACKCULL) & ~GW_ILLUM);
	
	gw->clearHitCode();
	
	vpt->ClearHitList();
	
	int res = 0;

	switch (d_subselState) {
		case SEL_DOOR:
			res = bmesh->DoorHitTest(vpt,mc,0);
			break;
		case SEL_FLOOR:
			res = bmesh->FloorHitTest(vpt,mc,0);
			break;
		case SEL_WATER:
			res = bmesh->WaterHitTest(vpt,mc,0);
			break;
		case SEL_IDOOR:
			res = bmesh->IDoorHitTest(vpt,mc,0);
			break;
		case SEL_IGNORED:
			res = bmesh->IgnoredHitTest(vpt,mc,0);
			break;
		case SEL_NORMALS:
			res = bmesh->NormsHitTest(vpt,mc,0);
			break;
		case SEL_SPOOFED:
			res = bmesh->SpoofedHitTest(vpt,mc,0);
			break;
		default:
			break;
	}
	
	if (res) {
 
		NotifyDependents(FOREVER, GEOM_CHANNEL|TOPO_CHANNEL|SELECT_CHANNEL|SUBSEL_TYPE_CHANNEL, REFMSG_CHANGE);

	}

	gw->setRndLimits(savedLimits);
	
	return res;
	
}


//******************************************
int SiegeNodeObject::Display(TimeValue t, INode* inode, ViewExp *vpt, int flags) {

	UpdateUI(t);

	Point3 pt(0,0,0);

	if (inode->Selected()) {
		
		BoundaryMesh* bmesh = GetBMesh();
		
		if (bmesh){
		
			bmesh->Render(vpt);
		}
		
		vpt->getGW()->setTransform(inode->GetObjectTM(t));
		vpt->getGW()->setColor(LINE_COLOR,GetUIColor(COLOR_SELECTION));
		vpt->getGW()->marker(&pt,BIG_BOX_MRKR);

	} else {
		
		vpt->getGW()->setTransform(inode->GetObjectTM(t));
		vpt->getGW()->setColor(LINE_COLOR,GetUIColor(COLOR_LINK_LINES));
		vpt->getGW()->marker(&pt,BIG_BOX_MRKR);
		
	}


	return(0);
}

//******************************************
CreateMouseCallBack* SiegeNodeObject::GetCreateMouseCallBack(void) {
	siegenodeobjectCreateCB.SetObj(this);
	return(&siegenodeobjectCreateCB);
}

//******************************************
void SiegeNodeObject::BeginEditParams(IObjParam *ip, ULONG flags,Animatable *prev)
{	
	iObjParams = ip;

// const TCHAR *ptype[] = { "Topo","Door Loop" };
//	ip->RegisterSubObjectTypes( ptype, 2 );
// const TCHAR *ptype[] = { "Doors" };
	ip->RegisterSubObjectTypes( SubPanelType, SubPanelCount );

	// Restore the selection level to OBJECT
	
	d_subselState = SEL_OBJECT;
	ip->SetSubObjectLevel(SEL_OBJECT);
	
	selectMode = new SelectModBoxCMode(this,ip);	

	if (!hPanelDlg) {
		hPanelDlg = ip->AddRollupPage( 
				hInstance, 
				MAKEINTRESOURCE(IDD_PANEL),
				SiegeNodeObjectDlgProc, 
				GetString(IDS_PARAMS), 
				(LPARAM)this );
		ip->RegisterDlgWnd(hPanelDlg);
	} else {
		SetWindowLong(hPanelDlg,GWL_USERDATA,(LONG)this);
	}
	UpdateUI(ip->GetTime());


}

//******************************************
void SiegeNodeObject::EndEditParams( IObjParam *ip, ULONG flags,Animatable *next) {
	
	if (flags&END_EDIT_REMOVEUI) {
		ip->UnRegisterDlgWnd(hPanelDlg);
		ip->DeleteRollupPage(hPanelDlg);
		hPanelDlg = NULL;
	} else {
		SetWindowLong(hPanelDlg,GWL_USERDATA,0);
	}
	iObjParams = NULL;

	if (d_subselState != SEL_OBJECT) {
		ip->DeleteMode(selectMode);
	}
	
	if (selectMode) delete selectMode;
	selectMode = NULL;

//	selLevel = 0;
}


//******************************************
// From Object
//******************************************

//******************************************
int	SiegeNodeObject::CanConvertToType(Class_ID obtype) {
	if (obtype == SIEGENODEOBJECT_CLASS_ID) {
		return TRUE;
	}
	return FALSE;
}

//******************************************
Object* SiegeNodeObject::ConvertToType(TimeValue t, Class_ID obtype) {
	if (obtype == SIEGENODEOBJECT_CLASS_ID) {
		return this;
	} else {
		return Object::ConvertToType(t,obtype);
	}
}

//******************************************
Object* SiegeNodeObject::MakeShallowCopy(ChannelMask channels) {

	// This is REALLY not sufficient
	return(this);
}

//******************************************
void SiegeNodeObject::ShallowCopy(Object* fromOb, ChannelMask channels) {

}

//******************************************
ObjectState SiegeNodeObject::Eval(TimeValue time) {
	{ return ObjectState(this); }
};

//******************************************
RefTargetHandle SiegeNodeObject::Clone(RemapDir& remap) 
{
	SiegeNodeObject* newnode = new SiegeNodeObject();
	
	// --TODO:biddle This clone does not correctly set the underlying references!

	newnode->MakeRefByID(FOREVER,REF_MESH,GetReference(REF_MESH));

	newnode->d_boundaryMesh = new BoundaryMesh;
	
	newnode->d_boundaryMesh->FindLoops(newnode->d_nodeMesh,true);
		
	newnode->d_boundaryOBB = new OBB;

	return(newnode);
}

//******************************************
// From Ref
//******************************************

//******************************************
RefResult SiegeNodeObject::NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, 
											PartID& partID, RefMessage message ) {


	switch (message) {
				
		case REFMSG_CHANGE: {

			if (partID & (TOPO_CHANNEL|GEOM_CHANNEL)) {

				if (d_boundaryMesh->UpdateBoundaryMesh(d_nodeMesh)) {

					// Update was successful!

				} else {
					if (!d_UserWasWarned) {
						MessageBox(GetForegroundWindow(), _T("Modifying this mesh has invalidated your Siege Node\nYou will have to rebuild the doors,floors, etc..."),_T("CAUTION"), MB_OK|MB_ICONWARNING);
						d_UserWasWarned = true;
					}
					
					d_boundaryMesh->DeleteAllDoors();
					d_boundaryMesh->ResetFloor();
					d_boundaryMesh->FindLoops(d_nodeMesh);
				}
			}


			break;
		}
		
		case REFMSG_TARGET_SELECTIONCHANGE: {

			d_boundaryMesh->DeselectAll();
		
			if (iObjParams)
				UpdateUI(iObjParams->GetTime());
		
			break;
		}
		
		case REFMSG_TARGET_DELETED: {

			// MessageBox(GetForegroundWindow(), _T("Deleting this mesh has invalidated your Siege Node\nYou will have to rebuild it from scratch"),_T("CAUTION"), MB_OK|MB_ICONWARNING);

			if (iObjParams)
				UpdateUI(iObjParams->GetTime());

			SetValid(false);

			// Everything this SNO is based in is no longer valid
			delete d_boundaryMesh;	
			d_boundaryMesh = 0;	

			delete d_triMesh;	
			d_triMesh = 0;

			delete d_boundaryOBB;	
			d_boundaryOBB = 0;

			d_firstBoundaryVert = 0;
			d_lastBoundaryVert = 0;

			DeleteReference(REF_MESH);

			d_nodeMesh = 0;
						
 			break;
		}
	}
	
	return REF_SUCCEED;
}


//******************************************
RefTargetHandle SiegeNodeObject::GetReference(int i) {

	if (i==REF_MESH) {	
		return (RefTargetHandle)d_nodeMesh;
	} else {
		assert(i==0xdeaddead);
	}

	return NULL;
  
}

//******************************************
void SiegeNodeObject::SetReference(int i, RefTargetHandle rtarg) {

	if (i==REF_MESH) {
		
		if (d_nodeMesh != (INode*)rtarg) {

			d_nodeMesh = (INode*)rtarg;

		} else {

			d_triMesh = 0;
				
		}
						
	} else {
		assert(i==0xdeadbeef);
	}
	
}


//******************************************
// From ReferenceMaker
//******************************************


//******************************************
IOResult SiegeNodeObject::Save(ISave *isave) {

	return d_boundaryMesh->Save(isave);
	
}

//******************************************
IOResult SiegeNodeObject::Load(ILoad *iload) {
	
	d_boundaryMesh = new BoundaryMesh;
	
	LoadFixup* fix = new LoadFixup;
	
	fix->d_sno = this;
	
	d_boundaryMesh->Load(iload,
		fix->d_floorFaceList,
		fix->d_waterFaceList,
		fix->d_ignoredFaceList,
		fix->d_lockedNormList,
		fix->d_spoofedNormList
		);
	
	iload->RegisterPostLoadCallback(fix);

	
	return IO_OK;
}

//******************************************
void SiegeNodeObject::LoadFixup::proc(ILoad *iload) {

	d_sno->GetBMesh()->PostLoadFixup((INode *)d_sno->GetReference(SiegeNodeObject::REF_MESH),
		d_floorFaceList,
		d_waterFaceList,
		d_ignoredFaceList,
		d_lockedNormList,
		d_spoofedNormList
		);

	d_sno->SetUserWasWarned(false);
	d_sno->SetCreated(true);
	d_sno->SetValid(true);

	delete this;
}

//******************************************
// Private Methods
//******************************************

void SiegeNodeObject::UpdateUI(TimeValue t)
{
	
		TCHAR buf[256];
		TCHAR* newbuf;

		char val[9];

		switch (d_subselState) {

			case SEL_OBJECT: {
			
				GetDlgItemText(hPanelDlg, IDC_BOUNDING_MESH_NAME, buf, sizeof(buf));

				INode *node;
		
				newbuf = "";
				if (node = (INode *)GetReference(SiegeNodeObject::REF_MESH)) {
					newbuf = node->GetName();
				}
		
				if(_tcscmp(buf, newbuf)) {
					SetDlgItemText(hPanelDlg, IDC_BOUNDING_MESH_NAME, newbuf);
				}

			
				// Update portal loop counts
		
				GetDlgItemText(hPanelDlg, IDC_DOOR_LOOP_COUNT, buf, sizeof(buf));

				if (d_boundaryMesh){
					sprintf(val,"%d",d_boundaryMesh->GetNumDoorLoops());
				}else {
					sprintf(val,"0");
				}

				if (_tcscmp(buf,val)){
					SetDlgItemText(hPanelDlg, IDC_DOOR_LOOP_COUNT, val);
				}
			
				// Update boundary loop counts
		
				GetDlgItemText(hPanelDlg, IDC_TOPO_LOOP_COUNT, buf, sizeof(buf));

				if (d_boundaryMesh){
					sprintf(val,"%d",d_boundaryMesh->GetNumBorderLoops());
				}else {
					sprintf(val,"0");
				}

				if (_tcscmp(buf,val)){
					SetDlgItemText(hPanelDlg, IDC_TOPO_LOOP_COUNT, val);
				}

				break;
			}
			
			case SEL_DOOR: {

				if (d_boundaryMesh) {

					Door* seldoor  = d_boundaryMesh->GetSelectedDoor();
				
					// Update Selected Door Loop Info
		
					GetDlgItemText(hPanelDlg, IDC_DOOR_ID, buf, sizeof(buf));

					if (seldoor && seldoor->GetLoopUsed() >= 0){
						sprintf(val,"%d",seldoor->GetID());
					}else {
						sprintf(val,"NONE");
					}

					if (_tcscmp(buf,val)){
						SetDlgItemText(hPanelDlg, IDC_DOOR_ID, val);
					}
		
					GetDlgItemText(hPanelDlg, IDC_SELECTED_LOOP_NUM, buf, sizeof(buf));

					if (seldoor && seldoor->GetLoopUsed() >= 0){
						sprintf(val,"%d",seldoor->GetLoopUsed());
					}else {
						sprintf(val,"NONE");
					}

					if (_tcscmp(buf,val)){
						SetDlgItemText(hPanelDlg, IDC_SELECTED_LOOP_NUM, val);
					}
				
					GetDlgItemText(hPanelDlg, IDC_SELECTED_FIRST_VERT, buf, sizeof(buf));

					if (seldoor && seldoor->GetFirstEdge() >= 0){
						sprintf(val,"%d",seldoor->GetFirstEdge());
					}else {
						sprintf(val,"NONE");
					}

					if (_tcscmp(buf,val)){
						SetDlgItemText(hPanelDlg, IDC_SELECTED_FIRST_VERT, val);
					}
			
					GetDlgItemText(hPanelDlg, IDC_SELECTED_LAST_VERT, buf, sizeof(buf));

					if (seldoor && seldoor->GetLastEdge() >= 0){
						sprintf(val,"%d",seldoor->GetLastEdge());
					}else {
						sprintf(val,"NONE");
					}

					if (_tcscmp(buf,val)){
						SetDlgItemText(hPanelDlg, IDC_SELECTED_LAST_VERT, val);
					}
				}
				break;
			}
			
			case SEL_FLOOR: {

				int faces;
				
				if (d_boundaryMesh && (faces = d_boundaryMesh->FNum())){
					
				
					GetDlgItemText(hPanelDlg, IDC_TOTAL_FACE_NUM, buf, sizeof(buf));

					if (faces > 0){
						sprintf(val,"%d",faces);
					}else {
						sprintf(val,"NONE");
					}

					if (_tcscmp(buf,val)){
						SetDlgItemText(hPanelDlg, IDC_TOTAL_FACE_NUM, val);
					}
					
					faces = d_boundaryMesh->GetFloorFaceCount();
					
					GetDlgItemText(hPanelDlg, IDC_FLOOR_FACE_NUM, buf, sizeof(buf));

					if (faces > 0){
						sprintf(val,"%d",faces);
					}else {
						sprintf(val,"NONE");
					}

					if (_tcscmp(buf,val)){
						SetDlgItemText(hPanelDlg, IDC_FLOOR_FACE_NUM, val);
					}
					
					faces = d_boundaryMesh->GetSelectedFloorFaceCount();
					
					GetDlgItemText(hPanelDlg, IDC_SELECTED_FACE_NUM, buf, sizeof(buf));

					if (faces > 0){
						sprintf(val,"%d",faces);
					}else {
						sprintf(val,"NONE");
					}

					if (_tcscmp(buf,val)){
						SetDlgItemText(hPanelDlg, IDC_SELECTED_FACE_NUM, val);
					}
				}
				
				break;
				
			}

			case SEL_WATER: {

				int faces;
				
				if (d_boundaryMesh && (faces = d_boundaryMesh->FNum())){
					
				
					GetDlgItemText(hPanelDlg, IDC_TOTAL_FACE_NUM, buf, sizeof(buf));

					if (faces > 0){
						sprintf(val,"%d",faces);
					}else {
						sprintf(val,"NONE");
					}

					if (_tcscmp(buf,val)){
						SetDlgItemText(hPanelDlg, IDC_TOTAL_FACE_NUM, val);
					}
					
					faces = d_boundaryMesh->GetWaterFaceCount();
					
					GetDlgItemText(hPanelDlg, IDC_WATER_FACE_NUM, buf, sizeof(buf));

					if (faces > 0){
						sprintf(val,"%d",faces);
					}else {
						sprintf(val,"NONE");
					}

					if (_tcscmp(buf,val)){
						SetDlgItemText(hPanelDlg, IDC_WATER_FACE_NUM, val);
					}
					
					faces = d_boundaryMesh->GetSelectedWaterFaceCount();
					
					GetDlgItemText(hPanelDlg, IDC_SELECTED_FACE_NUM, buf, sizeof(buf));

					if (faces > 0){
						sprintf(val,"%d",faces);
					}else {
						sprintf(val,"NONE");
					}

					if (_tcscmp(buf,val)){
						SetDlgItemText(hPanelDlg, IDC_SELECTED_FACE_NUM, val);
					}
				}
				
				break;
				
			}

			case SEL_IDOOR: {

				if (d_boundaryMesh) {

					IDoor* seldoor  = d_boundaryMesh->GetSelectedIDoor();
				
					// Update Selected Internal Door
		
					GetDlgItemText(hPanelDlg, IDC_DOOR_ID, buf, sizeof(buf));

					if (seldoor){
						sprintf(val,"%d",seldoor->GetID());
					}else {
						sprintf(val,"NONE");
					}

					if (_tcscmp(buf,val)){
						SetDlgItemText(hPanelDlg, IDC_DOOR_ID, val);
					}
		
					GetDlgItemText(hPanelDlg, IDC_VERT_COUNT, buf, sizeof(buf));

					if (seldoor && seldoor->NumVerts() > 0){
						sprintf(val,"%d",seldoor->NumVerts());
					}else {
						sprintf(val,"NONE");
					}

					if (_tcscmp(buf,val)){
						SetDlgItemText(hPanelDlg, IDC_VERT_COUNT, val);
					}
				}
				break;

			}

			case SEL_IGNORED: {

				int faces;
				
				if (d_boundaryMesh && (faces = d_boundaryMesh->FNum())){
					
				
					GetDlgItemText(hPanelDlg, IDC_TOTAL_FACE_NUM, buf, sizeof(buf));

					if (faces > 0){
						sprintf(val,"%d",faces);
					}else {
						sprintf(val,"NONE");
					}

					if (_tcscmp(buf,val)){
						SetDlgItemText(hPanelDlg, IDC_TOTAL_FACE_NUM, val);
					}
					
					faces = d_boundaryMesh->GetIgnoredCount();
					
					GetDlgItemText(hPanelDlg, IDC_IGNORED_NUM, buf, sizeof(buf));

					if (faces > 0){
						sprintf(val,"%d",faces);
					}else {
						sprintf(val,"NONE");
					}

					if (_tcscmp(buf,val)){
						SetDlgItemText(hPanelDlg, IDC_IGNORED_NUM, val);
					}
					
					faces = d_boundaryMesh->GetSelectedIgnoredCount();
					
					GetDlgItemText(hPanelDlg, IDC_SELECTED_IGNORED_NUM, buf, sizeof(buf));

					if (faces > 0){
						sprintf(val,"%d",faces);
					}else {
						sprintf(val,"NONE");
					}

					if (_tcscmp(buf,val)){
						SetDlgItemText(hPanelDlg, IDC_SELECTED_FACE_NUM, val);
					}
				}
				
				break;
				
			}

			case SEL_NORMALS: {

				int norms;
				
				if (d_boundaryMesh){
					
					norms = d_boundaryMesh->GetLockedNormCount();
				
					GetDlgItemText(hPanelDlg, IDC_LOCKED_NORM_NUM, buf, sizeof(buf));

					if (norms > 0){
						sprintf(val,"%d",norms);
					}else {
						sprintf(val,"NONE");
					}

					if (_tcscmp(buf,val)){
						SetDlgItemText(hPanelDlg, IDC_LOCKED_NORM_NUM, val);
					}
					
					norms = d_boundaryMesh->GetSelectedNormCount();
					
					GetDlgItemText(hPanelDlg, IDC_SELECTED_NORM_NUM, buf, sizeof(buf));

					if (norms > 0){
						sprintf(val,"%d",norms);
					}else {
						sprintf(val,"NONE");
					}

					if (_tcscmp(buf,val)){
						SetDlgItemText(hPanelDlg, IDC_SELECTED_NORM_NUM, val);
					}
				}
				
				break;
				
			}

			case SEL_SPOOFED: {

				int norms;
				
				if (d_boundaryMesh){
					
					norms = d_boundaryMesh->GetSpoofedNormCount();
				
					GetDlgItemText(hPanelDlg, IDC_SPOOFED_NORM_NUM, buf, sizeof(buf));

					if (norms > 0){
						sprintf(val,"%d",norms);
					}else {
						sprintf(val,"NONE");
					}

					if (_tcscmp(buf,val)){
						SetDlgItemText(hPanelDlg, IDC_SPOOFED_NORM_NUM, val);
					}
					
					norms = d_boundaryMesh->GetSelectedSpoofedNormCount();
					
					GetDlgItemText(hPanelDlg, IDC_SELECTED_SPOOFED_NORM_NUM, buf, sizeof(buf));

					if (norms > 0){
						sprintf(val,"%d",norms);
					}else {
						sprintf(val,"NONE");
					}

					if (_tcscmp(buf,val)){
						SetDlgItemText(hPanelDlg, IDC_SELECTED_SPOOFED_NORM_NUM, val);
					}
				}
				
				break;
				
			}

			case SEL_OBB: {

				break;

			}
			
		}

}

//******************************************
// Support for sub-object selection
//******************************************

void SiegeNodeObject::ActivateSubobjSel(int level, XFormModes& modes )
{
	// Show the correct RollUp

	assert (level >= 0);
	assert (level < MAX_LEVELS);
	
	if (d_subselState != level) {
		
		if (hPanelDlg) {
			
			//Close the existing panel
			iObjParams->UnRegisterDlgWnd(hPanelDlg);
			iObjParams->DeleteRollupPage(hPanelDlg);

		}
		
		hPanelDlg = iObjParams->AddRollupPage( 
			hInstance, 
			MAKEINTRESOURCE(SubPanelResources[level]),
			SubPanelProcs[level],
			SubPanelName[level], 
			(LPARAM)this );
		iObjParams->RegisterDlgWnd(hPanelDlg);

		if (level == SEL_OBJECT) {
			iObjParams->DeleteMode(selectMode);		
		} else {
			modes = XFormModes(NULL,NULL,NULL,NULL,NULL,selectMode);
//			NotifyDependents(FOREVER, PART_ALL, REFMSG_MOD_DISPLAY_ON);
//			NotifyDependents(FOREVER, PART_ALL, REFMSG_BEGIN_EDIT);
		}
		
	
		d_subselState = level;
		
	} else {
		
		SetWindowLong(hPanelDlg,GWL_USERDATA,(LONG)this);
		
	}


//	NotifyDependents(FOREVER, GEOM_CHANNEL|TOPO_CHANNEL|SELECT_CHANNEL|SUBSEL_TYPE_CHANNEL, REFMSG_CHANGE);

}

void SiegeNodeObject::SelectSubComponent(HitRecord *hitRec, BOOL selected, BOOL all, BOOL invert) {

	switch(d_subselState) {
		
		case SEL_DOOR : {
			
			int closest_loop = (hitRec->hitInfo>>PACKED_LOOP_SHIFT) & PACKED_LOOP_MASK;
			int closest_vert = (hitRec->hitInfo>>PACKED_VERT_SHIFT) & PACKED_VERT_MASK;
			int closest_flag = (hitRec->hitInfo>>PACKED_FLAG_SHIFT) & PACKED_FLAG_MASK;
	
			d_boundaryMesh->SetHitLoop(closest_loop);
			d_boundaryMesh->SetHitVert(closest_vert);

			if (closest_flag) {
		
				d_boundaryMesh->SelectHitDoor();
		
			} else {
		
				d_boundaryMesh->DoorSelect(selected,all,invert,false);
	
			}

			iObjParams->ForceCompleteRedraw();

			break;
			
		}
		
		case SEL_FLOOR : {
			
			int closest_face = hitRec->hitInfo;
			
			d_boundaryMesh->FloorSelect(selected,all,invert,false);

			iObjParams->ForceCompleteRedraw();

			break;
			
		}

		case SEL_WATER : {
			
			int closest_face = hitRec->hitInfo;
			
			d_boundaryMesh->WaterSelect(selected,all,invert,false);

			iObjParams->ForceCompleteRedraw();

			break;
			
		}

		case SEL_IDOOR : {
			
			int closest_idoor = hitRec->hitInfo;
			
			if (closest_idoor < 0) {

				// We are constructing an IDoor
				d_boundaryMesh->IDoorSelect(selected,all,invert,false);

			} else {

				d_boundaryMesh->SelectHitIDoor(closest_idoor);

			}

			iObjParams->ForceCompleteRedraw();

			break;
			
		}

		case SEL_IGNORED : {
			
			int closest_face = hitRec->hitInfo;
			
			d_boundaryMesh->IgnoredSelect(selected,all,invert,false);

			iObjParams->ForceCompleteRedraw();

			break;
			
		}

		case SEL_NORMALS : {
			
			int closest_face = hitRec->hitInfo;
			
			d_boundaryMesh->NormsSelect(selected,all,invert,false);

			iObjParams->ForceCompleteRedraw();

			break;
			
		}
		case SEL_SPOOFED : {
			
			int closest_face = hitRec->hitInfo;
			
			d_boundaryMesh->SpoofedSelect(selected,all,invert,false);

			iObjParams->ForceCompleteRedraw();

			break;
			
		}

		default: {
			break;
		}
			

	}
	
	return;
}
  


//******************************************
// SiegeNodeObjectCreateCallback methods
//******************************************

//******************************************
int SiegeNodeObjectCreateCallback::proc(ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat ) {	

	if (msg == MOUSE_FREEMOVE)
	{
		vpt->SnapPreview(m,m,NULL, SNAP_IN_3D);
	}

	if (msg==MOUSE_POINT||msg==MOUSE_MOVE) {
		mat.SetTrans(vpt->SnapPoint(m,m,NULL,SNAP_IN_3D));
		if (point==1 && msg == MOUSE_POINT) {
			node->SetCreated(true);
			node->UpdateUI(0);		
			return 0;
		}
	} else if (msg == MOUSE_ABORT) {		
		return CREATE_ABORT;
	}
	
	return TRUE;
}


//******************************************
void SiegeNodeObjectCreateCallback::SetObj(SiegeNodeObject *newnode) {
	node = newnode;
}


//******************************************
// SiegeNodeObjectCreateCallback methods
//******************************************

//******************************************
// PickSiegeNodeObjectBoundingMeshCallback methods

BOOL PickSiegeNodeObjectBoundingMeshCallback::Filter(INode *node) 
{

	if (!node) return FALSE;
	
	ObjectState os = node->EvalWorldState(0);
	
	if ((os.obj->SuperClassID()==GEOMOBJECT_CLASS_ID &&
		 os.obj->IsRenderable()) || os.obj->SuperClassID()==SHAPE_CLASS_ID) {

		return TRUE;
			
		}

	return FALSE;


}

BOOL PickSiegeNodeObjectBoundingMeshCallback::HitTest (IObjParam *ip, HWND hWnd, ViewExp *vpt, IPoint2 m, int flags) 
{	
	INode *node = ip->PickNode(hWnd, m, this);
	return Filter (node);
}

//******************************************
BOOL PickSiegeNodeObjectBoundingMeshCallback::Pick (IObjParam *ip, ViewExp *vpt) 

{

	INode *newbound = vpt->GetClosestHit();
	assert(newbound);

	ip->RemoveTempPrompt();

	LookForObjectByClassCB lookCB;

	// Make sure this node is not already used as a boundary
		
	lookCB.d_found = NULL;
	lookCB.d_searchClass = SIEGENODEOBJECT_CLASS_ID;
	newbound->EnumDependents(&lookCB);

	if (SiegeNodeObject* siegenode = (SiegeNodeObject*)lookCB.d_found) {

		// The node IS already used as a boundary
	
		char* nodename = "UNKNOWN";
		char* boundname = newbound->GetName();
					
		lookCB.d_found = NULL;
		lookCB.d_searchClass = Class_ID(BASENODE_CLASS_ID,0);

		siegenode->EnumDependents(&lookCB);
			
		if (INode* node = (INode *)lookCB.d_found) {
			nodename=node->GetName();
		} 
								
		char* prompt = " is already a boundary for ";
		int size = strlen(nodename) + strlen(prompt) + strlen(boundname);
		char* msg = new char[size+1];
						
		sprintf(msg, "%s%s%s", boundname, prompt,nodename);	
			
		ip->DisplayTempPrompt(msg,2000);

		delete [] msg;

		return FALSE;
	
	}

// Is this a new boundary, or a replacement?

	BoundaryMesh* bmesh = (BoundaryMesh*)d_siegeNodeObj->GetBMesh();
	
	INode* prevbound;
	
	if (bmesh && (prevbound = (INode *)d_siegeNodeObj->GetReference(SiegeNodeObject::REF_MESH))) {
		
		if (prevbound == newbound) {
			return  FALSE; // This case SHOULD have been filtered out
		}

		if (d_siegeNodeObj->ReplaceReference(SiegeNodeObject::REF_MESH,newbound) != REF_SUCCEED) {
			assert(0 && REF_SUCCEED);
			return FALSE;
		}
			
	} else {

		if (!bmesh) {
			
			bmesh = new BoundaryMesh;

			d_siegeNodeObj->SetBMesh(bmesh);

			d_siegeNodeObj->SetOBB(new OBB);
		} 
		
		if (d_siegeNodeObj->MakeRefByID(FOREVER,SiegeNodeObject::REF_MESH,newbound) != REF_SUCCEED) {
			assert(0 && REF_SUCCEED);
			return FALSE;
		}

	
	}

	TSTR newname = "SNO_";
	newname = newname + newbound->GetName();

	if (ip->GetSelNodeCount() == 1) {
		ip->GetSelNode(0)->SetName(newname);
		ip->GetSelNode(0)->NotifyDependents(FOREVER,0,REFMSG_NODE_NAMECHANGE);
//		IRollupWindow* cmd = ip->GetCommandPanelRollup();
//		for (int p = 0; p < cmd->GetNumPanels(); p++) {
//			HWND cmddlg = cmd->GetPanelDlg(p);
//		}
	}

	bmesh->FindLoops(newbound);

	d_siegeNodeObj->SetValid(true);
	d_siegeNodeObj->SetUserWasWarned(false);

	return TRUE;
}


void PickSiegeNodeObjectBoundingMeshCallback::EnterMode(IObjParam *ip) {
	
	d_ip = ip;
	
	ICustButton *iBut = GetICustButton(GetDlgItem(hDlg, IDC_PICK_BOUNDING_MESH_BUTTON));
	if (iBut) {
		iBut->SetCheck(TRUE);
		ReleaseICustButton(iBut);
	}
	
	doingPick = 1;
}

void PickSiegeNodeObjectBoundingMeshCallback::ExitMode(IObjParam *ip) 
{
	ICustButton *iBut = GetICustButton(GetDlgItem(hDlg, IDC_PICK_BOUNDING_MESH_BUTTON));
	
	if (iBut) {
		iBut->SetCheck(FALSE);
		ReleaseICustButton(iBut);
	}
	
	doingPick = 0;
	
	d_ip = NULL;
}
