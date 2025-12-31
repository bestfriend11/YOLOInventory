#pragma once
#ifndef _IMEUI_H_
#define _IMEUI_H_

#include "gpcore.h"

typedef void* IMEUI_FONT;
struct IMEUI_APPEARANCE
{
	// symbol (Henkan-kyu)
	DWORD			symbolColor;
	DWORD			symbolColorText;
	BYTE			symbolHeight;
	BYTE			symbolTranslucence;
	BYTE			symbolPlacement;

	// candidate list
	DWORD			candColorBase;
	DWORD			candColorBorder;
	DWORD			candColorText;

	// composition string
	DWORD			compColorInput;
	DWORD			compColorTargetConv;
	DWORD			compColorConverted;
	DWORD			compColorTargetNotConv;
	DWORD			compColorInputErr;
	BYTE			compTranslucence;
	DWORD			compColorText;
};

struct IMEUI_VERTEX
{
	float sx;
	float sy;
	float sz;
	float rhw;
	DWORD color;
	DWORD specular;
	float tu;
	float tv;
};

#define IMEUI_STATE_OFF		0
#define IMEUI_STATE_ON		1
#define IMEUI_STATE_ENGLISH	2

bool ImeUi_Initialize(HWND hwnd, bool bDiable = false);
void ImeUi_SetSupportLevel(DWORD dwImeLevel);
void ImeUi_SetAppearance(IMEUI_APPEARANCE* pia);
bool ImeUi_IgnoreHotKey(MSG* pmsg);
LPARAM ImeUi_ProcessMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM 
lParam, bool * trapped);
void ImeUi_SetScreenDimension(UINT width, UINT height);
void ImeUi_RenderUI(bool bDrawCompAttr = true, bool bDrawOtherUi = true);
void ImeUi_SetCaretPosition(UINT x, UINT y);
void ImeUi_SetCompStringAppearance(IMEUI_FONT hfont, DWORD color, RECT* 
prc);
bool ImeUi_GetCaretStatus();
void ImeUi_SetInsertMode(bool bInsert);
void ImeUi_SetState( DWORD dwState );
DWORD ImeUi_GetState();
void ImeUi_EnableIme(bool bEnable);
bool ImeUi_IsEnabled();
void ImeUi_FinalizeString(void);
void ImeUI_CancelString(void);
void ImeUi_SetSwirlTimeIncrement( double time );
void ImeUi_ToggleLanguageBar(BOOL bRestore);
bool ImeUi_IsSendingKeyMessage();

extern void (*ImeUiCallback_SetFont)(IMEUI_FONT hfont);
extern void (*ImeUiCallback_SetFontAttribute)(UINT uHeight, bool bBold);
extern void (*ImeUiCallback_SetTextColor)(DWORD color);
extern void (*ImeUiCallback_SetTextPosition)(int x, int y);
extern void (*ImeUiCallback_DrawText)(const char* pszText );
extern void (*ImeUiCallback_GetStringDimension)(const char* szText, DWORD* 
puWidth, DWORD* puHeight);
extern void (*ImeUiCallback_DrawRect)( int x1, int y1, int x2, int y2, DWORD 
color );
extern void* (*ImeUiCallback_Malloc)( size_t bytes );
extern void (*ImeUiCallback_Free)( void* ptr );
extern void (*ImeUiCallback_DrawFans)(IMEUI_VERTEX* paVertex, UINT uNum);

#endif //_IMEUI_H_

