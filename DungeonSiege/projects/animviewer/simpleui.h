#ifndef SIMPLEUI_H
#define SIMPLEUI_H

#include "vector_3.h"
#include "gpstring.h"

void SUI_OrbitMode(void);
void SUI_DollyMode(void);
void SUI_CraneMode(void);
void SUI_ZoomMode(void);

void SUI_UpdateMouseDeltaX(int delta);
void SUI_UpdateMouseDeltaY(int delta);
void SUI_UpdateMouseDeltaZ(int delta);

bool SUI_GetFileName(
				gpstring Title,
				gpstring InitialDir,
				gpstring FileDesc,
				gpstring FileExt,
				gpstring& OutBuff
				);


vector_3 SUI_GetCameraTarget();
float    SUI_GetCameraOrbit();
float    SUI_GetCameraAzimuth();
float    SUI_GetCameraDistance();
float    SUI_GetCameraHeight();

void SUI_SetCameraTarget(const vector_3& v);
void SUI_SetCameraOrbit(float n);
void SUI_SetCameraHeight(float n);
void SUI_SetCameraAzimuth(float n);
void SUI_SetCameraDistance(float n);

#endif