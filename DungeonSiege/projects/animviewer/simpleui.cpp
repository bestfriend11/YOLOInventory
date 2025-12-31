#include "precomp_animviewer.h"

#include "vector_3.h"
#include <COMMDLG.H>

// I am declaring SUI level globals with an sui_ prefix -- biddle

// Camera control parameters
enum ModeFlag {
	DOLLY_MODE,
	ORBIT_MODE,
	CRANE_MODE,
	ZOOM_MODE
};

ModeFlag sui_Mode = ORBIT_MODE;

vector_3 sui_TargetPos = vector_3( 0.0f, 0.0f, 0.0f );
float sui_Orbit = 0.0f;
float sui_Azimuth = 15.0f * RealPi/180.0f;
float sui_CameraDistance = 3.0;

#define PRECISION_MIN_DISTANCE 0.05f
#define PRECISION_MAX_DISTANCE 100.0f

vector_3 SUI_GetCameraTarget()		{return sui_TargetPos;}
float    SUI_GetCameraOrbit()		{return sui_Orbit;}
float    SUI_GetCameraHeight()		{return sui_TargetPos.GetY();}
float    SUI_GetCameraAzimuth()		{return sui_Azimuth;}
float    SUI_GetCameraDistance()	{return sui_CameraDistance;}

void SUI_SetCameraOrbit(float n)			{ sui_Orbit=n;}
void SUI_SetCameraHeight(float n)			{ sui_TargetPos.SetY(n);}
void SUI_SetCameraAzimuth(float n)			{ sui_Azimuth=n;}
void SUI_SetCameraDistance(float n)			{ sui_CameraDistance=n;}
void SUI_SetCameraTarget(const vector_3& v)	{ sui_TargetPos=v;}

void SUI_ResetCameraPosition(void)
{
	sui_Mode = ORBIT_MODE;
	sui_TargetPos = vector_3( 0.0f, 0.0f, 0.0f );
	sui_Orbit = 0.0f;
	sui_Azimuth = 15.0f * RealPi/180.0f;
	sui_CameraDistance = 3.0;
}

//*********************************************************************************************
//*********************************************************************************************

void SUI_OrbitMode(void) {
	sui_Mode = ORBIT_MODE;
}

void SUI_DollyMode(void) {
	sui_Mode = DOLLY_MODE;
}

void SUI_CraneMode(void) {
	sui_Mode = CRANE_MODE;
}

void SUI_ZoomMode(void) {
	sui_Mode = ZOOM_MODE;
}


//*********************************************************************************************

void SUI_UpdateMouseDeltaX(int delta) {

	switch (sui_Mode) {

	case DOLLY_MODE:

		sui_TargetPos.SetX( sui_TargetPos.GetX() + (Cos(sui_Orbit) * delta * (sui_CameraDistance/1024) ) );
		sui_TargetPos.SetZ( sui_TargetPos.GetZ() + (Sin(sui_Orbit) * delta * (sui_CameraDistance/1024) ) );

		break;

	case ORBIT_MODE:

		sui_Orbit += (delta * (2*RealPi/1024));

		while (sui_Orbit < 0) {
			sui_Orbit += (2*RealPi);
		}

		while (sui_Orbit > 2*RealPi) {
			sui_Orbit -= (2*RealPi);
		}

		break;
	}
}

void SUI_UpdateMouseDeltaY(int delta) {

	switch (sui_Mode) {

	case ZOOM_MODE:

		if (delta < 0)
		{
			sui_CameraDistance = sui_CameraDistance * 9.0f/10.0f;
		}
		else
		{
			sui_CameraDistance = sui_CameraDistance * 10.0f/9.0f;
			sui_CameraDistance = Maximum(sui_CameraDistance,PRECISION_MIN_DISTANCE);
		}

		break;

	case CRANE_MODE:

		sui_TargetPos.SetY( sui_TargetPos.GetY() + delta * (sui_CameraDistance/1024) );

		break;

	case DOLLY_MODE:

		sui_TargetPos.SetX( sui_TargetPos.GetX() - (Sin(sui_Orbit) * delta * (sui_CameraDistance/1024.0f)) );
		sui_TargetPos.SetZ( sui_TargetPos.GetZ() + (Cos(sui_Orbit) * delta * (sui_CameraDistance/1024.0f)) );

		break;

	case ORBIT_MODE:

		sui_Azimuth += (delta * (RealPi/1024));

		if (sui_Azimuth < -RealPi/2.01f) {			// note the extra 0.01
			sui_Azimuth = ( -RealPi/2.01f );			// to avoid gimbal lock
		} else if (sui_Azimuth >  RealPi/2.01f) {
			sui_Azimuth = ( RealPi/2.01f );
		}

		break;
	}
}

void SUI_UpdateMouseDeltaZ(int delta) {
	sui_CameraDistance += (delta * (sui_CameraDistance/1024.0f)) ;
	if (sui_CameraDistance < PRECISION_MIN_DISTANCE) {
		sui_CameraDistance = PRECISION_MIN_DISTANCE;
	} else if (sui_CameraDistance > PRECISION_MAX_DISTANCE) {
		sui_CameraDistance = PRECISION_MAX_DISTANCE;
	}
}


//*********************************************************************************************
//*********************************************************************************************

UINT_PTR CALLBACK OFNHookProcOldStyle(
  HWND		hdlg,    // handle to child dialog box
  UINT		uiMsg,   // message identifier
  WPARAM	wParam,  // message parameter
  LPARAM	lParam   // message parameter
)
{
	UNREFERENCED_PARAMETER(hdlg);
	UNREFERENCED_PARAMETER(uiMsg);
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);
	return false;
}

bool SUI_GetFileName(
				gpstring Title,
				gpstring InitialDir,
				gpstring FileDesc,
				gpstring FileExt,
				gpstring& OutBuff
				)
{


	// Build up the file filter

	TCHAR NameBuff[_MAX_PATH];
	NameBuff[0] = 0;

	TCHAR FilterBuff[_MAX_PATH];
	memset(FilterBuff,0,_MAX_PATH);

	strcpy(FilterBuff,FileDesc.c_str());
	strcpy(&FilterBuff[FileDesc.size()+1],"*.");
	strcpy(&FilterBuff[FileDesc.size()+3],FileExt.c_str());
	FilterBuff[FileDesc.size()+FileExt.size()+4] = 0;	// Extra NULL at end of pairs

	OPENFILENAME OpenFileName;
	ZeroObject( OpenFileName );

	OpenFileName.lStructSize = OPENFILENAME_SIZE_VERSION_400;
	OpenFileName.hwndOwner = NULL;
	OpenFileName.hInstance = NULL;
	OpenFileName.lpstrFilter = FilterBuff;
	OpenFileName.lpstrCustomFilter = NULL;
	OpenFileName.nMaxCustFilter = 0L;
	OpenFileName.nFilterIndex = 0L;
	OpenFileName.lpstrFile = NameBuff;
	OpenFileName.nMaxFile = _MAX_PATH;
	OpenFileName.lpstrFileTitle = NULL;
	OpenFileName.nMaxFileTitle = 0L;
	OpenFileName.lpstrInitialDir = InitialDir.begin_split();
	OpenFileName.lpstrTitle = Title.begin_split();
	OpenFileName.Flags = OFN_FILEMUSTEXIST|OFN_NONETWORKBUTTON
#if defined(BUSTED) && GP_DEBUG
		|OFN_ENABLEHOOK;
#else
		;
#endif
	OpenFileName.nFileOffset = 0;
	OpenFileName.nFileExtension = 0;
	OpenFileName.lpstrDefExt = FileExt;
	OpenFileName.lCustData = 0;
	OpenFileName.lpfnHook = OFNHookProcOldStyle;
	OpenFileName.lpTemplateName = 0;

	BOOL ret = GetOpenFileName(&OpenFileName);

	//wart gpgenericf(("ret code %d\n",ret));

	if (ret)
	{
		OutBuff = NameBuff;
	}
	else
	{
		OutBuff.clear();
	}

	return ret != 0;
}

