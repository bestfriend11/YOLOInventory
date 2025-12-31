#include "ImeUi.h"
#include "LocHelp.h"

#include <imm.h>
#include <mbstring.h>
#include <math.h>
#include <msctf.h>

//
//===========================================================================//
// File:	 ImeUi.cpp													     //
// Project:  IME UI					                                         //
// Contents: Render and manipulate the IME									 //
//---------------------------------------------------------------------------//
// Copyright (C) 2000 Microsoft Corp.						                 //
// All Rights reserved worldwide                                             
//
// This unpublished sourcecode is PROPRIETARY and CONFIDENTIAL               
//
//===========================================================================//
//
// Based on GOS IME support code

//
// IME Variables
//
#define MAX_CANDLIST 9
#define CODEPAGE_CHS		936
#define CODEPAGE_CHT		950
#define CODEPAGE_KOR		949
#define CODEPAGE_JPN		932

#define _TAIWAN_HKL			((HKL)0xE0080404)
#define _TAIWAN_HKL2		((HKL)0xE0090404) // New Chang Jie
#define _TAIWAN_IMEFILENAME	"TINTLGNT.IME"
#define _TAIWAN_IMEFILENAME2	"CINTLGNT.IME" // New Chang Jie
#define _TAIWAN_IMEFILENAME3	"MSTCIPHA.IME" // Phonetic 5.1
#define _TAIWAN_IMEVERSION42 (0x00040002) // New(Phonetic/ChanJie)IME98  : 4.2.x.x // Win98
#define _TAIWAN_IMEVERSION43 (0x00040003) // New(Phonetic/ChanJie)IME98a : 4.3.x.x // Win2k
#define _TAIWAN_IMEVERSION44 (0x00040004) // New ChanJie IME98b          : 4.4.x.x // WinXP
#define _TAIWAN_IMEVERSION50 (0x00050000) // New(Phonetic/ChanJie)IME5.0 : 5.0.x.x // WinME
#define _TAIWAN_IMEVERSION51 (0x00050001) // New(Phonetic/ChanJie)IME5.1 : 5.1.x.x // IME2002(w/OfficeXP)
#define _TAIWAN_IMEVERSION52 (0x00050002) // New(Phonetic/ChanJie)IME5.2 : 5.2.x.x // IME2002a(w/Whistler)
#define _TAIWAN_IMEVERSION60 (0x00060000) // New(Phonetic/ChanJie)IME6.0 : 6.0.x.x // IME XP(w/WinXP SP1)

IMEUI_APPEARANCE gSkinIME = {
	0,			// symbolColor;
	0xff000000,	// symbolColorText;
	24,			// symbolHeight;
	0xa0,		// symbolTranslucence;
	0,			// symbolPlacement;
	0xffffffff,	// candColorBase;
	0xff000000,	// candColorBorder;
	0,			// candColorText;
	0x00ffff00,	// compColorInput;
	0x000000ff,	// compColorTargetConv;
	0x0000ff00,	// compColorConverted;
	0x00ff0000,	// compColorTargetNotConv;
	0x00ff0000,	// compColorInputErr;
	0x80,		// compTranslucence;
	0			// compColorText;
};

struct _SkinCompStr
{
	DWORD	colorInput;
	DWORD	colorTargetConv;
	DWORD	colorConverted;
	DWORD	colorTargetNotConv;
	DWORD	colorInputErr;
};

_SkinCompStr gSkinCompStr;


// Definition from Win98DDK version of IMM.H
typedef struct tagINPUTCONTEXT {
    HWND                hWnd;
    BOOL                fOpen;
    POINT               ptStatusWndPos;
    POINT               ptSoftKbdPos;
    DWORD               fdwConversion;
    DWORD               fdwSentence;
    union   {
        LOGFONTA        A;
        LOGFONTW        W;
    } lfFont;
    COMPOSITIONFORM     cfCompForm;
    CANDIDATEFORM       cfCandForm[4];
    HIMCC               hCompStr;
    HIMCC               hCandInfo;
    HIMCC               hGuideLine;
    HIMCC               hPrivate;
    DWORD               dwNumMsgBuf;
    HIMCC               hMsgBuf;
    DWORD               fdwInit;
    DWORD               dwReserve[3];
} INPUTCONTEXT, *PINPUTCONTEXT, NEAR *NPINPUTCONTEXT, FAR *LPINPUTCONTEXT;

#define SPEW(x)
static void _PumpMessage();
static HIMC (WINAPI* _ImmGetContext)(HWND hW);
static BOOL (WINAPI* _ImmReleaseContext)( HWND hW, HIMC hImc );
static HIMC (WINAPI* _ImmAssociateContext)( HWND hW, HIMC himc );
static LONG (WINAPI* _ImmGetCompositionString)( HIMC himc, DWORD dwIndex, 
LPVOID lpBuf, DWORD dwBufLen );
static BOOL (WINAPI* _ImmSetCompositionString)( HIMC hIMC, DWORD dwIndex, 
LPVOID lpBuf, DWORD dwBufLen, LPVOID lpRead, DWORD dwReadLen );
static BOOL (WINAPI* _ImmGetOpenStatus)( HIMC himc );
static BOOL (WINAPI* _ImmSetOpenStatus)( HIMC himc, BOOL flag );
static BOOL (WINAPI* _ImmGetConversionStatus)( HIMC himc, LPDWORD 
lpfdwConversion, LPDWORD lpfdwSentence );
static DWORD (WINAPI* _ImmGetCandidateList)( HIMC himc, DWORD deIndex, 
LPCANDIDATELIST lpCandList, DWORD dwBufLen );
static LPINPUTCONTEXT (WINAPI* _ImmLockIMC)(HIMC hIMC);
static BOOL (WINAPI* _ImmUnlockIMC)(HIMC hIMC);
static LPVOID (WINAPI* _ImmLockIMCC)(HIMCC hIMCC);
static BOOL (WINAPI* _ImmUnlockIMCC)(HIMCC hIMCC);
static HWND (WINAPI* _ImmGetDefaultIMEWnd)(HWND hW);
static HWND (WINAPI* _ImmGetIMEFileName)(HKL hKL, LPTSTR lpszFileName, UINT 
uBufLen);
static UINT (WINAPI* _ImmGetVirtualKey)( HWND hW );
static BOOL (WINAPI* _ImmNotifyIME)(HIMC, DWORD dwAction, DWORD dwIndex, 
DWORD dwValue);
static BOOL (WINAPI* _ImmSetConversionStatus)(HIMC hIMC, DWORD 
fdwConversion, DWORD fdwSentence);
static BOOL (WINAPI* _ImmSimulateHotKey)(HWND hWnd, DWORD dwHotKeyID);
static BOOL (WINAPI* _ImmIsIME)(HKL hKL);

// CHT IME functions
UINT (WINAPI* _GetReadingString)(HIMC himc, UINT uReadingBufLen, LPWSTR lpwReadingBuf, PINT pnErrorIndex, BOOL* pfIsVertical, PUINT puMaxReadingLen);
BOOL (WINAPI* _ShowReadingWindow)(HIMC himc, BOOL bShow);

void (*ImeUiCallback_SetFont)(IMEUI_FONT hfont);
void (*ImeUiCallback_SetFontAttribute)(UINT uHeight, bool bBold);
void (*ImeUiCallback_SetTextColor)(DWORD color);
void (*ImeUiCallback_SetTextPosition)(int x, int y);
void (*ImeUiCallback_DrawText)(const char* pszText );
void (*ImeUiCallback_GetStringDimension)(const char* szTemp, DWORD* puWidth, 
DWORD* puHeight);
void (*ImeUiCallback_DrawRect)( int x1, int y1, int x2, int y2, DWORD color 
);
void* (*ImeUiCallback_Malloc)( size_t bytes );
void (*ImeUiCallback_Free)( void* ptr );
void (*ImeUiCallback_DrawFans)(IMEUI_VERTEX* paVertex, UINT uNum);

static HWND g_hwndMain;
static HIMC g_himcOrg;
static bool g_bImeEnabled = false;
static TCHAR g_szCompositionString[256];
static BYTE  g_szCompAttrString[256];
static DWORD g_IMECursorBytes = 0;
static DWORD g_IMECursorChars = 0;
static TCHAR g_szCandidate[MAX_CANDLIST][256];
static DWORD g_dwSelection, g_dwCount;
DWORD g_bDisableImeCompletely = false;
static DWORD g_dwIMELevel;
static char g_szMultiLineCompString[256*2];
static bool g_bReadingWindow = false;
static bool g_bHorizontalReading = false;
static bool g_bVerticalCand = true;
static UINT g_uCodePage = 0;
static double g_uCaretBlinkTime = 0;
static double g_uCaretBlinkLast = 0;
static bool g_bCaretDraw = false;
static bool g_bIme = false;
static HKL g_hkl;
static bool g_bChineseIME;
static bool g_bInsertMode = true;
static char g_szReadingString[32];	// Used only in case of horizontal reading window
static int g_iReadingError;	// Used only in case of horizontal reading window
static UINT g_screenWidth, g_screenHeight;
static DWORD g_dwPrevFloat;
static bool bSendingKeyMsg = false;
static double g_uSwirlTime = 0.05;
static bool g_bInitialized = false;

struct CompStringAttribute
{
	UINT			caretX;
	UINT			caretY;
	IMEUI_FONT		fontHandle;
	DWORD			colorComp;
	DWORD			colorCand;
	RECT			margins;
};

// Global Variables
CompStringAttribute g_CaretInfo;
HFONT g_hIMEFont = NULL;
DWORD g_dwState = IMEUI_STATE_OFF;
DWORD swirl = 0;
double lastSwirl;
unsigned char g_SymbolIME[3];
const char* c_szCandListFormat = "%d %s";
const char* c_szCandListFormatHorizontal = "%d%s ";

DWORD GetIMEInfo();
static void GetReadingString(HWND hWnd);
static DWORD GetTaiwanIMEVersion();
static void CheckToggleState();
static void DrawImeIndicator();
static void SetReadingWindowOrientation(DWORD dwVer);
static void SetImeApi();

//
//	local helper functions
//

// returns number of characters from number of bytes
static int mbGetCharCount(LPCSTR pszString, int iBytes)
{
	int iCount = 0;
	for (int i = 0; pszString[i] && i < iBytes; i++) {
		iCount++;
		if (::isleadbyte(pszString[i]))
			i++;
	}
	if (i != iBytes)
		iCount = -iCount;	// indicate error - iBytes specifies wrong boundary (i.e. the last byte is leadbyte)
	return iCount;
}

static void SendControlKeys(UINT vk, UINT num)
{
	if (num == 0)
		return;
	bSendingKeyMsg = true;
	for (UINT i = 0; i < num ; i++) {
		SendMessage(g_hwndMain, WM_KEYDOWN, vk, 0);
	}
	SendMessage(g_hwndMain, WM_KEYUP, vk, 0);
	bSendingKeyMsg = false;
}

// move caret to the end of composition string
static void IMEMoveCaretToTheEnd(HWND /*hwnd*/)
{
	int mblen = _mbstrlen(g_szCompositionString);
	SendControlKeys(VK_RIGHT, mblen - g_IMECursorChars);
}

// send key messages to erase composition string.
static void IMECancelCompString(HWND hwnd, bool bUseBackSpace = true, int 
iNewStrLen = 0)
{
	if (g_dwIMELevel != 3)
		return;
	int mblen = _mbstrlen(g_szCompositionString);
	int i;
	IMEMoveCaretToTheEnd(hwnd);

	if (bUseBackSpace || g_bInsertMode)
		iNewStrLen = 0;

	// The caller sets bUseBackSpace to false if there's possibility of sending
	// new composition string to the app right after this function call.
	//
	// If the app is in overwriting mode and new comp string is
	// shorter than current one, delete previous comp string
	// till it's same long as the new one. Then move caret to the beginning of comp string.
	// New comp string will overwrite old one.
	if (iNewStrLen < mblen)
	{
		bSendingKeyMsg = true;
		for (i = 0; i < mblen - iNewStrLen; i++)
		{
			SendMessage(hwnd, WM_KEYDOWN, VK_BACK, 0);
			SendMessage(hwnd, WM_CHAR, 8, 0);	//Backspace character
		}
		SendMessage(hwnd, WM_KEYUP, VK_BACK, 0);
		bSendingKeyMsg = false;
	}
	else
		iNewStrLen = mblen;

	SendControlKeys(VK_LEFT, iNewStrLen);
}

// initialize composition string data.
static void IMEInitCompStringData(void)
{
	g_IMECursorBytes = 0;
	g_IMECursorChars = 0;
	memset(&g_szCompositionString, 0, sizeof(g_szCompositionString));
	memset(&g_szCompAttrString, 0, sizeof(g_szCompAttrString));
}

static void DrawCaret(DWORD x, DWORD y, DWORD height)
{
	if (g_bCaretDraw)
		ImeUiCallback_DrawRect(x, y + 1, x + 2, y + height - 1, g_CaretInfo.colorComp);
}

//
// Apps that draw the composition string on top of composition string attribute
// in level 3 support should call this function twice in rendering a frame.
//     // Draw edit box UI;
//     ImeUi_RenderUI(true, false);	// paint composition string attribute;
//     // Draw text in the edit box;
//     ImeUi_RenderUi(false, true); // paint the rest of IME UI;
//
void ImeUi_RenderUI(bool bDrawCompAttr, bool bDrawOtherUi)
{
	if (!g_bInitialized || !g_bImeEnabled || !g_CaretInfo.fontHandle)
		return;
	if (!bDrawCompAttr && !bDrawOtherUi)
		return;	// error case
	if (g_dwIMELevel == 2) {
		if (!bDrawOtherUi)
			return;	// 1st call for level 3 support
	}

	if (bDrawOtherUi)
		DrawImeIndicator();

	// Process timer for caret blink
	double uCurrentTime = GetSystemSeconds();
	if (uCurrentTime - g_uCaretBlinkLast > g_uCaretBlinkTime)
	{
		g_uCaretBlinkLast = uCurrentTime;
		g_bCaretDraw = !g_bCaretDraw;
	}

	int i = 0;

	ImeUiCallback_SetFont(g_CaretInfo.fontHandle);
	ImeUiCallback_SetTextColor(g_CaretInfo.colorComp);

	DWORD uDummy;

	int len = strlen(g_szCompositionString);

	DWORD cType = 0;

	DWORD bgX = g_CaretInfo.caretX;
	DWORD bgY = g_CaretInfo.caretY;
	DWORD candX = 0;
	DWORD candY = 0;
	char* pszMlcs = g_szMultiLineCompString;

	DWORD wCompChar = 0, hCompChar = 0;
	ImeUiCallback_GetStringDimension(" ", &uDummy, &hCompChar);
	if (g_dwIMELevel == 3 && g_IMECursorBytes && g_szCompositionString[0])
	{
		// shift starting point of drawing composition string according to the current caret position.
		char temp[512];
		temp[0] = 0;
		strncpy( temp, g_szCompositionString, g_IMECursorBytes );
		temp[g_IMECursorBytes] = 0;
		ImeUiCallback_GetStringDimension(temp, &wCompChar, &hCompChar);
		bgX -= wCompChar;
	}

	//
	// Draw the background colors for IME text nuggets
	//
	bool saveCandPos = false;

	if (g_uCodePage != CODEPAGE_KOR || g_bCaretDraw)	// Korean uses composition string as blinking block caret
    for (i = 0; i < len ; i += cType)
	{
		DWORD bgColor = 0x00000000;
		cType = 1 + ((::isleadbyte(g_szCompositionString[i])) ? 1 : 0);
		char szChar[3];
		szChar[0] = g_szCompositionString[i];
		szChar[1] = szChar[2] = 0;

		if (cType == 2 && g_szCompositionString[i+1])	// in case we have 0 in trailbyte, we don't count it.
			szChar[1] = g_szCompositionString[i+1];
		ImeUiCallback_GetStringDimension(szChar, &wCompChar, &hCompChar);

		switch(g_szCompAttrString[i])
		{
		case ATTR_INPUT:
			bgColor = gSkinCompStr.colorInput;
			break;
		case ATTR_TARGET_CONVERTED:
			bgColor = gSkinCompStr.colorTargetConv;
			saveCandPos = true;
			break;
		case ATTR_CONVERTED:
			bgColor = gSkinCompStr.colorConverted;
			break;
		case ATTR_TARGET_NOTCONVERTED:
			//
			// This is the one the user is working with currently
			//
			bgColor = gSkinCompStr.colorTargetNotConv;
			break;
		case ATTR_INPUT_ERROR:
			bgColor = gSkinCompStr.colorInputErr;
			break;
		default:
			gpwatson(( "Attributes on IME characters are wrong" ));
			break;
		}

		if (g_dwIMELevel == 3 && bDrawCompAttr)
		{
			if ((LONG)bgX >= g_CaretInfo.margins.left && (LONG)bgX <= 
g_CaretInfo.margins.right)
			{
				ImeUiCallback_DrawRect(bgX, bgY, bgX + wCompChar, bgY + hCompChar, 
bgColor);
			}
		}
		else if (g_dwIMELevel == 2)
		{
			// make sure at least 6 bytes (possible space, NUL for current line, possible DBCS, 2 more NUL)
			// are available in multiline composition string buffer
			bool bWrite = (pszMlcs - g_szMultiLineCompString <
					sizeof(g_szMultiLineCompString) / sizeof(g_szMultiLineCompString[0]) - 
6);

			if ((LONG)(bgX + wCompChar) >= g_CaretInfo.margins.right) {
				bgX = g_CaretInfo.margins.left;
				bgY = bgY + hCompChar;
				if (bWrite)
				{
					if (pszMlcs == g_szMultiLineCompString || pszMlcs[-1] == 0)
						*pszMlcs++ = ' ';	// to avoid zero length line
					*pszMlcs++ = 0;
				}
			}
			ImeUiCallback_DrawRect(bgX, bgY, bgX + wCompChar, bgY + hCompChar, 
bgColor);
			if (bWrite) {
				*pszMlcs++ = g_szCompositionString[i];
				if (cType == 2)
					*pszMlcs++ = g_szCompositionString[i+1];
			}
			if ((DWORD)i == g_IMECursorBytes)
				DrawCaret(bgX, bgY, hCompChar);
		}
		if (saveCandPos && candX == 0)
		{
			saveCandPos = false;
			candX = bgX;
			candY = bgY;
		}
		bgX += wCompChar;
	}

	if (g_dwIMELevel == 2)
	{
		// in case the caret in composition string is at the end of it, draw it here
		if (len != 0 && (DWORD)i == g_IMECursorBytes)
			DrawCaret(bgX, bgY, hCompChar);

		// Draw composition string.
		gpassert(pszMlcs - g_szMultiLineCompString <=
					sizeof(g_szMultiLineCompString) / sizeof(g_szMultiLineCompString[0]) - 
2);
		*pszMlcs++ = 0;
		*pszMlcs++ = 0;
		DWORD x, y;
		x = g_CaretInfo.caretX;
		y = g_CaretInfo.caretY;
		pszMlcs = g_szMultiLineCompString;
		while (*pszMlcs &&
			pszMlcs - g_szMultiLineCompString < sizeof(g_szMultiLineCompString) / 
sizeof(g_szMultiLineCompString[0]))
		{
			ImeUiCallback_SetTextPosition(x, y);
			ImeUiCallback_DrawText(pszMlcs);
			pszMlcs += strlen(pszMlcs) + 1;
			x = g_CaretInfo.margins.left;
			y += hCompChar;
		}
	}

	if (!bDrawOtherUi)
		return;

	if (g_dwCount && g_szCandidate[0][0] != 0)
	{
		// If position of candidate list is not initialized yet, set it here.
		if (candX == 0)
		{
			candX = g_CaretInfo.caretX;
			candY = g_CaretInfo.caretY;
		}

		// Find out the largest candidate string
		SIZE largest = {0,0};
		largest.cx = largest.cy = 0;
		LPCSTR pcszFormat = g_bVerticalCand ? c_szCandListFormat : 
c_szCandListFormatHorizontal;

		if (g_bReadingWindow && g_bHorizontalReading)
			ImeUiCallback_GetStringDimension(g_szReadingString, (DWORD*)&largest.cx, 
(DWORD*)&largest.cy);
		else {
			for (int i = 0; g_szCandidate[i][0] && i < MAX_CANDLIST; i++)
			{
				DWORD tx, ty;
				if (!g_bReadingWindow)
				{
					char szTemp[256];
					wsprintf(szTemp, pcszFormat, i+1, g_szCandidate[i]);
					ImeUiCallback_GetStringDimension(szTemp, &tx, &ty);
				}
				else
					ImeUiCallback_GetStringDimension(g_szCandidate[i], &tx, &ty);
				if ((signed)tx > largest.cx)
					largest.cx = tx;
				if ((signed)ty > largest.cy)
					largest.cy = ty;
			}
		}

		DWORD uDigitWidth, uSpaceWidth;
		ImeUiCallback_GetStringDimension("0", &uDigitWidth, &uDummy);
		ImeUiCallback_GetStringDimension(" ", &uSpaceWidth, &uDummy);
		DWORD dwMerginX = (uSpaceWidth + 1) / 2;

		// Show candidate list above composition string if there isn't enough room below.
		DWORD slotsUsed;
		if (g_bReadingWindow && g_dwCount < MAX_CANDLIST)
			slotsUsed = g_dwCount;
		else
			slotsUsed = MAX_CANDLIST;
		DWORD dwCandHeight;
		if (g_bVerticalCand && !(g_bReadingWindow && g_bHorizontalReading))
			dwCandHeight = slotsUsed * largest.cy + 2;
		else
			dwCandHeight = largest.cy + 2;
		if ( candY + hCompChar + dwCandHeight > g_screenHeight)
			candY -= dwCandHeight;
		else
			candY += hCompChar;
		if (candY < 0)
			candY = 0;

		// Move candidate list horizontally to keep it inside of screen
		DWORD dwCandWidth;
		if (g_bReadingWindow && g_bHorizontalReading)
			dwCandWidth = largest.cx + 2 + dwMerginX * 2;
		else if (g_bVerticalCand)
			dwCandWidth = largest.cx + 2 + dwMerginX * 2;
		else
			dwCandWidth = slotsUsed * (largest.cx + 1) + 1;
		if (candX + dwCandWidth > g_screenWidth)
			candX = g_screenWidth - dwCandWidth;
		if (candX < 0)
			candX = 0;

		// Draw frame and background of candidate list
		int saveRight;

		int left = candX;
		int top = candY;
		int right = candX + dwCandWidth;
		int bottom = candY + dwCandHeight;

		ImeUiCallback_DrawRect(left, top, right, bottom, 
gSkinIME.candColorBorder);

		left++;
		top++;
		right--;
		bottom--;

		if (g_bVerticalCand) {
			saveRight = right;
			// assumption : all digits have same width
			if ( !g_bReadingWindow ) {
				right = left + dwMerginX + uDigitWidth + uSpaceWidth / 2 - 1;
				ImeUiCallback_DrawRect(left, top, right, bottom, 
gSkinIME.candColorBase);
				left = right + 1;
				right = saveRight;
				ImeUiCallback_DrawRect(left, top, right, bottom, 
gSkinIME.candColorBase);
			}
			else
				ImeUiCallback_DrawRect(left,top, right, bottom, gSkinIME.candColorBase);
		}
		else
		{
			for (i = 0; (DWORD)i < slotsUsed; i++)
			{
				ImeUiCallback_DrawRect(left, top, left + largest.cx, bottom, 
gSkinIME.candColorBase);
				left += largest.cx + 1;
			}
		}

		candX++;
		candY++;
        if (g_bReadingWindow && g_bHorizontalReading) {
			int iStart = -1, iEnd = -1, iDummy;
			candX += dwMerginX;

			// draw background of error character if it exists
			char szTemp[sizeof(g_szReadingString)];
			if (g_iReadingError >= 0) {
				strcpy(szTemp, g_szReadingString);
				char* psz = (char*)_mbsinc((unsigned char*)szTemp + g_iReadingError);
				*psz = 0;
				ImeUiCallback_GetStringDimension(szTemp, (DWORD*)&iEnd, 
(DWORD*)&iDummy);
				char cSave = szTemp[g_iReadingError];
				szTemp[g_iReadingError] = 0;
				ImeUiCallback_GetStringDimension(szTemp, (DWORD*)&iStart, 
(DWORD*)&iDummy);
				szTemp[g_iReadingError] = cSave;
				ImeUiCallback_DrawRect(candX + iStart, candY, candX + iEnd, candY + 
largest.cy, gSkinIME.candColorBorder);
			}

			ImeUiCallback_SetTextPosition(candX , candY);
			ImeUiCallback_SetTextColor(g_CaretInfo.colorCand);
			ImeUiCallback_DrawText(g_szReadingString);

			// draw error character if it exists
			if (iStart >= 0) {
				ImeUiCallback_SetTextPosition(candX + iStart, candY);
				ImeUiCallback_SetTextColor(0xff000000 + (~((0x00ffffff) & 
g_CaretInfo.colorCand)));
				ImeUiCallback_DrawText(szTemp + g_iReadingError);
			}
		}
		else {
			//display candidate
			for (i = 0; i < MAX_CANDLIST && (DWORD)i < g_dwCount; i++)
			{
				if (g_dwSelection == (DWORD)i)
				{
					ImeUiCallback_SetTextColor(0xff000000 + (~((0x00ffffff) & 
g_CaretInfo.colorCand)));
					ImeUiCallback_DrawRect(candX, candY + i * largest.cy, candX - 1 + 
dwCandWidth, candY + (i + 1) * largest.cy, gSkinIME.candColorBorder);
				}
				else
					ImeUiCallback_SetTextColor(g_CaretInfo.colorCand);
				if (g_szCandidate[i][0] != 0)
				{
					if (g_bVerticalCand)
						ImeUiCallback_SetTextPosition(dwMerginX + candX , candY + i * 
largest.cy);
					else
						ImeUiCallback_SetTextPosition(uSpaceWidth / 2 + candX + i * 
(largest.cx + 1), candY);
					if (!g_bReadingWindow) {
						char szTemp[256];
						wsprintf(szTemp, pcszFormat, i+1, g_szCandidate[i]);
						ImeUiCallback_DrawText(szTemp);
					}
					else
						ImeUiCallback_DrawText(g_szCandidate[i]);
				}
			}
		}
    }
}

//
//	ProcessIMEMessages()
//	Processes IME related messages and acquire information
//
LPARAM ImeUi_ProcessMessage( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM 
lParam, bool * trapped )
{
	HIMC himc;
	int len;
	int i;

	if ( uMsg == WM_ACTIVATEAPP )
	{
		if ( LocMgr::DoesSingletonExist() )
		{
			ImeUi_ToggleLanguageBar( LOWORD(wParam) == WA_INACTIVE );
		}		
	}

	if (!g_bInitialized || g_bDisableImeCompletely)
	{
		*trapped = false;
		return 0;
	}

	*trapped = true;

	switch( uMsg )
	{
//
//	IME Handling
//
	case WM_INPUTLANGCHANGE:
		CheckToggleState();
		SetImeApi();
		if ( _ShowReadingWindow )
		{
			if ( NULL != ( himc = _ImmGetContext( g_hwndMain ) ) )
			{
				_ShowReadingWindow( himc, false );
				_ImmReleaseContext( g_hwndMain, himc );
			}
		}
		*trapped = false;
		break;

    case WM_IME_CONTROL:
		*trapped = false;
        break;

    case WM_IME_SETCONTEXT:
		//
		// We don't want anything to display, so we have to clear this
		//
        lParam = 0;
		*trapped = false;
        break;

    case WM_IME_STARTCOMPOSITION:
		IMEInitCompStringData();
		*trapped = true;
		break;

    case WM_IME_COMPOSITIONFULL:
		*trapped = false;
        break;

    case WM_IME_COMPOSITION:
		{
			LONG lRet;
			char 
szCompStr[sizeof(g_szCompositionString)/sizeof(g_szCompositionString[0])];

			if (NULL == (himc = _ImmGetContext(hWnd)))
				return 0;

				// ResultStr must be processed before composition string.
		        if ( lParam & GCS_RESULTSTR )
				{
					lRet = _ImmGetCompositionString( himc, GCS_RESULTSTR, szCompStr, 255 );
					szCompStr[lRet] = 0;
					IMECancelCompString(g_hwndMain, false, _mbstrlen(szCompStr));

					strcpy(g_szCompositionString, szCompStr);

					bSendingKeyMsg = true;
					for (int i = 0; i < lRet ; i++)
					{
						SendMessage(g_hwndMain, WM_CHAR, (unsigned 
char)g_szCompositionString[i], 0);
					}
					bSendingKeyMsg = false;
					IMEInitCompStringData();
				}
		        //
		        // Reads in the composition string.
		        //
		        if ( lParam & GCS_COMPSTR )
		        {
					//////////////////////////////////////////////////////
					// Retrieve the latest user-selected IME candidates
					lRet = _ImmGetCompositionString( himc, GCS_COMPSTR, szCompStr, 255 );
					szCompStr[lRet] = 0;
					//
					// Remove the whole of the string
					//
					IMECancelCompString(g_hwndMain, false, _mbstrlen(szCompStr));

					strcpy(g_szCompositionString, szCompStr);
					lRet = _ImmGetCompositionString( himc, GCS_COMPATTR, g_szCompAttrString, 255 );
					g_szCompAttrString[lRet] = 0;

					if ((g_uCodePage == CODEPAGE_CHT && !GetTaiwanIMEVersion()) // for older Taiwan IME
						|| g_uCodePage == CODEPAGE_CHS)
					{
						int i, chars = strlen(g_szCompositionString) / 2;
						if (chars)
						{
							g_dwCount = 4;
							g_dwSelection = (DWORD)-1;	// don't select any candidate

							for (i = 3; i >= 0; i--)
							{
								if (i > chars - 1)
									g_szCandidate[i][0] = 0;
								else
								{
									g_szCandidate[i][0] = g_szCompositionString[i*2];
									g_szCandidate[i][1] = g_szCompositionString[i*2+1];
									g_szCandidate[i][2] = 0;
								}
							}
							memset(g_szCompositionString, 0, 8);
							g_bReadingWindow = true;
							SetReadingWindowOrientation(0);
							if (g_bHorizontalReading) {
								g_iReadingError = -1;
								g_szReadingString[0] = 0;
								for (UINT i = 0; i < g_dwCount; i++) {
									if (g_dwSelection == (DWORD)i)
										g_iReadingError = strlen(g_szReadingString);
									strcat(g_szReadingString, g_szCandidate[i]);
								}
							}
						}
						else
							g_dwCount = 0;
					}

					// get caret position in composition string
					g_IMECursorBytes = _ImmGetCompositionString(himc, GCS_CURSORPOS, NULL, 0);
					g_IMECursorChars = mbGetCharCount(g_szCompositionString, g_IMECursorBytes);

					if (g_dwIMELevel == 3)
					{
						// send composition string via WM_CHAR
						len = strlen(g_szCompositionString);
						bSendingKeyMsg = true;
						for (i = 0; i < len ; i++)
							SendMessage(g_hwndMain, WM_CHAR, (unsigned char)g_szCompositionString[i], 0);
						bSendingKeyMsg = false;

						// move caret to appropreate location
						len = _mbstrlen(g_szCompositionString + g_IMECursorBytes);
						SendControlKeys(VK_LEFT, len);
					}
				}
				_ImmReleaseContext(hWnd, himc);
		}
		//
		// DO NOT REMOVE THIS RETURN
		// The GDI disable  available via WM_IME_SETCONTEXT does not work. The only way to
		// hide the GDI representation is to have this return here.
		return 0;

    case WM_IME_ENDCOMPOSITION:
		IMECancelCompString(g_hwndMain);
		IMEInitCompStringData();
		*trapped = false;
        break;

    case WM_IME_NOTIFY:
        switch (wParam)
        {
			case IMN_SETCONVERSIONMODE:
			case IMN_SETOPENSTATUS:
				CheckToggleState();
				*trapped = false;
				break;

			case IMN_OPENCANDIDATE:
			case IMN_CHANGECANDIDATE:
				{
					if (NULL == (himc = _ImmGetContext(hWnd)))
						return 0;

					LPCANDIDATELIST lpCandList;
					DWORD dwIndex, dwBufLen;
				    DWORD temp;
				    LPTSTR lpStr;

					g_bReadingWindow = false;
					dwIndex = 0;
					dwBufLen = _ImmGetCandidateList( himc, dwIndex, NULL, 0 );
					if ( dwBufLen )
					{
					    lpCandList = (LPCANDIDATELIST)ImeUiCallback_Malloc(dwBufLen);
					    _ImmGetCandidateList( himc, dwIndex, lpCandList, dwBufLen );
					    g_dwSelection = lpCandList->dwSelection;
					    g_dwCount = lpCandList->dwCount;

						int what_page = g_dwSelection / lpCandList->dwPageSize;
						g_dwSelection = g_dwSelection - (what_page * lpCandList->dwPageSize);

					    memset(&g_szCandidate, 0, sizeof(g_szCandidate));
					    for (int i = (what_page * lpCandList->dwPageSize), j = 0; (DWORD)i < lpCandList->dwCount && j < MAX_CANDLIST; i++, j++)
					    {
					        temp = (DWORD)lpCandList + lpCandList->dwOffset[i];
					        lpStr = (LPTSTR)temp;
					        _mbscpy(((unsigned char *) g_szCandidate[j]), (const unsigned char *) lpStr);
					    }
					    ImeUiCallback_Free( (HANDLE)lpCandList );
						_ImmReleaseContext(hWnd, himc);

						// don't display selection in candidate list in case of Korean and old Taiwan IME.
						if (g_uCodePage == CODEPAGE_KOR ||
							g_uCodePage == CODEPAGE_CHT && GetTaiwanIMEVersion() == 0 )
							g_dwSelection = (DWORD)-1;
					}
					return 0;
				}

			case IMN_CLOSECANDIDATE:
				if (!g_bReadingWindow)	// fix for Ent Gen #120.
				{
					g_dwCount = 0;
					memset(&g_szCandidate, 0, sizeof(g_szCandidate));
				}
			    return 0;

			// Jun.16,2000 05:21 by yutaka.
			case IMN_PRIVATE:
				if ( !_GetReadingString )
				{
					GetReadingString(hWnd);
				}
				{
				DWORD dwVer = GetTaiwanIMEVersion();
				// Trap some messages to hide reading window
				if (!(
					((dwVer == _TAIWAN_IMEVERSION42 || dwVer == _TAIWAN_IMEVERSION43 || dwVer == _TAIWAN_IMEVERSION44)
						&& ((lParam == 1) || (lParam == 2))) ||
					((dwVer >= _TAIWAN_IMEVERSION50 || dwVer <= _TAIWAN_IMEVERSION60) && ((lParam == 16) || (lParam == 17)))
				))
					*trapped = false;	// This is necessary to on/off Taiwan IME by shift key - 000619 kazuyuks
				}
				break;

			default:
				*trapped = true;
				break;
		}
		break;

	case WM_KEYDOWN:
		*trapped = false;
		if (_GetReadingString && !ImeUi_IsSendingKeyMessage())
		{
			GetReadingString(hWnd);
		}
		break;

	default:
	{
		*trapped = false;
	}
	}
	return 0;
}

void ImeUi_SetCaretPosition(UINT x, UINT y)
{
	g_CaretInfo.caretX = x;
	g_CaretInfo.caretY = y;
}

void ImeUi_SetCompStringAppearance(IMEUI_FONT hfont, DWORD color, RECT* prc)
{
	g_CaretInfo.fontHandle = hfont;
	g_CaretInfo.margins = *prc;

	if (0 == gSkinIME.candColorText)
		g_CaretInfo.colorCand = color;
	else
		g_CaretInfo.colorCand = gSkinIME.candColorText;
	if (0 == gSkinIME.compColorText)
		g_CaretInfo.colorComp = color;
	else
		g_CaretInfo.colorComp = gSkinIME.compColorText;
}

void ImeUi_SetState( DWORD dwState )
{
	if (!g_bInitialized)
		return;
	HIMC himc;
	if ( dwState == IMEUI_STATE_ON )
	{
		ImeUi_EnableIme( true );
	}
	if (NULL != (himc = _ImmGetContext(g_hwndMain)))
	{
		SPEW(("jreicher", "imeon = %d", bOn));
		if (g_bDisableImeCompletely)
		{
			dwState = IMEUI_STATE_OFF;
		}

		bool bOn = dwState == IMEUI_STATE_ON;
		switch (g_uCodePage) {
		case CODEPAGE_CHS:
		case CODEPAGE_CHT:
		{
		 	// toggle Chinese IME
			DWORD dwVer;
			DWORD dwConvMode = 0, dwSentMode = 0;
			if ( ( g_bChineseIME && dwState == IMEUI_STATE_OFF ) || ( !g_bChineseIME && dwState != IMEUI_STATE_OFF ) )
			{
				_ImmSimulateHotKey( g_hwndMain, IME_THOTKEY_IME_NONIME_TOGGLE );
				_PumpMessage();
			}
			if ( dwState != IMEUI_STATE_OFF )
			{
				dwVer = GetTaiwanIMEVersion();
				if ( dwVer )
				{
					_ImmGetConversionStatus( himc, &dwConvMode, &dwSentMode );
					dwConvMode = ( dwState == IMEUI_STATE_ON )
						? ( dwConvMode | IME_CMODE_NATIVE )
						: ( dwConvMode & ~IME_CMODE_NATIVE );
					_ImmSetConversionStatus( himc, dwConvMode, dwSentMode );
				}
			}
			break;
		}
		case CODEPAGE_KOR:
		 	// toggle Korean IME
			if ( ( bOn && g_dwState != IMEUI_STATE_ON ) || ( !bOn && g_dwState == IMEUI_STATE_ON ) )
			{
				_ImmSimulateHotKey(g_hwndMain, IME_KHOTKEY_ENGLISH);
			}
			break;
		case CODEPAGE_JPN:
			_ImmSetOpenStatus(himc, bOn);
			break;
		}
		_ImmReleaseContext(g_hwndMain, himc);
		CheckToggleState();
	}
}

DWORD ImeUi_GetState()
{
	if (!g_bInitialized)
		return IMEUI_STATE_OFF;
	CheckToggleState();
	return g_dwState;
}

void ImeUi_EnableIme( bool bEnable )
{
	if (!g_bInitialized || !g_hwndMain)
		return;
	if (g_bDisableImeCompletely)
		bEnable = false;

	SPEW(("jreicher", "IME enabled: %d", bEnable));

	HIMC himcDbg;
	himcDbg = _ImmAssociateContext( g_hwndMain, bEnable? g_himcOrg : NULL );
	g_bImeEnabled = bEnable;
}

bool ImeUi_IsEnabled()
{
	return g_bImeEnabled;
}

bool ImeUi_Initialize(HWND hwnd, bool bDisable)
{
	g_hwndMain = hwnd;
	if (NULL == _ImmGetContext) {
		HMODULE LibIME = LoadLibrary( "imm32.dll" );
		if( LibIME )
		{
		_ImmGetContext =			(HIMC  (WINAPI*)(HWND hW)) GetProcAddress(LibIME, "ImmGetContext");
		_ImmReleaseContext =		(BOOL  (WINAPI*)(HWND hW, HIMC hImc)) GetProcAddress(LibIME, "ImmReleaseContext");
		_ImmAssociateContext =		(HIMC  (WINAPI*)(HWND hW, HIMC hImc)) GetProcAddress(LibIME, "ImmAssociateContext");
		_ImmGetCompositionString =	(LONG  (WINAPI*)(HIMC hIMC, DWORD dwIndex, LPVOID lpBuf, DWORD dwBufLen )) GetProcAddress(LibIME, "ImmGetCompositionStringA");
		_ImmGetOpenStatus =			(BOOL  (WINAPI*)(HIMC hIMC)) GetProcAddress(LibIME, "ImmGetOpenStatus");
		_ImmSetOpenStatus =			(BOOL  (WINAPI*)(HIMC hIMC, BOOL flag)) GetProcAddress(LibIME, "ImmSetOpenStatus");
		_ImmGetConversionStatus =	(BOOL  (WINAPI*)(HIMC hIMC, LPDWORD lpfdwConversion, LPDWORD lpfdwSentence )) GetProcAddress(LibIME, "ImmGetConversionStatus");
		_ImmGetCandidateList=		(DWORD (WINAPI*)(HIMC hIMC, DWORD deIndex, LPCANDIDATELIST lpCandList, DWORD dwBufLen )) GetProcAddress(LibIME, "ImmGetCandidateListA");
		_ImmGetVirtualKey =			(UINT  (WINAPI*)(HWND hW )) GetProcAddress(LibIME, "ImmGetVirtualKey");
		_ImmLockIMC =               (LPINPUTCONTEXT (WINAPI*)(HIMC hIMC)) GetProcAddress(LibIME, "ImmLockIMC");
		_ImmUnlockIMCC =            (BOOL (WINAPI*)(HIMCC hIMC)) GetProcAddress(LibIME, "ImmUnlockIMC");
		_ImmLockIMCC =              (LPVOID (WINAPI*)(HIMCC hIMCC)) GetProcAddress(LibIME, "ImmLockIMCC");
		_ImmUnlockIMC =             (BOOL (WINAPI*)(HIMC hIMCC)) GetProcAddress(LibIME, "ImmUnlockIMCC");
		_ImmGetDefaultIMEWnd =      (HWND (WINAPI*)(HWND hW)) GetProcAddress(LibIME, "ImmGetDefaultIMEWnd");
		_ImmGetIMEFileName =        (HWND (WINAPI*)(HKL hKL, LPTSTR lpszFileName, UINT uBufLen)) GetProcAddress(LibIME, "ImmGetIMEFileNameA");
		_ImmNotifyIME =				(BOOL (WINAPI*)(HIMC, DWORD dwAction, DWORD dwIndex, DWORD dwValue)) GetProcAddress(LibIME, "ImmNotifyIME");
		_ImmSetConversionStatus =	(BOOL (WINAPI*)(HIMC hIMC, DWORD fdwConversion, DWORD fdwSentence)) GetProcAddress(LibIME, "ImmSetConversionStatus");
		_ImmSimulateHotKey =		(BOOL (WINAPI*)(HWND hWnd, DWORD dwHotKeyID)) GetProcAddress(LibIME, "ImmSimulateHotKey");
		_ImmIsIME =					(BOOL (WINAPI*)(HKL)) GetProcAddress(LibIME, "ImmIsIME");
		}
		if (!( _ImmGetContext && _ImmReleaseContext && _ImmAssociateContext && _ImmGetCompositionString
				&& _ImmGetOpenStatus && _ImmSetOpenStatus && _ImmGetConversionStatus && _ImmGetCandidateList
				&& _ImmGetVirtualKey && _ImmLockIMC && _ImmUnlockIMCC && _ImmLockIMCC && _ImmUnlockIMC
				&& _ImmGetDefaultIMEWnd && _ImmGetIMEFileName && _ImmNotifyIME && _ImmSetConversionStatus
				&& _ImmSimulateHotKey && _ImmIsIME ))
			return false;
		BOOL (WINAPI* _ImmDisableTextFrameService)(DWORD) = (BOOL (WINAPI*)(DWORD))GetProcAddress(LibIME, "ImmDisableTextFrameService");
		if ( _ImmDisableTextFrameService )
		{
			_ImmDisableTextFrameService( (DWORD)-1 );
		}
	}
	if ( bDisable )
	{
		if (_ImmAssociateContext)
		{
			_ImmAssociateContext(g_hwndMain, NULL);
		}
		g_bDisableImeCompletely = true;
		return true;
	}

	static bool bInitialized = false;
    if (!bInitialized) 
	{
        bInitialized = true;
        g_himcOrg = _ImmGetContext(g_hwndMain);
        _ImmReleaseContext(g_hwndMain, g_himcOrg);
    }

	// the following pointers to function has to be initialized before this function is called.
	gpassert( ImeUiCallback_SetFont );
	gpassert( ImeUiCallback_SetFontAttribute );
	gpassert( ImeUiCallback_SetTextColor );
	gpassert( ImeUiCallback_SetTextPosition );
	gpassert( ImeUiCallback_DrawText );
	gpassert( ImeUiCallback_GetStringDimension );
	gpassert( ImeUiCallback_DrawRect );
	gpassert( ImeUiCallback_Malloc );
	gpassert( ImeUiCallback_Free );
	gpassert( ImeUiCallback_DrawFans );

	g_uCaretBlinkTime = GetCaretBlinkTime() / 1000.0f;

	// removed in new version -cq
	//HKL hkl = GetKeyboardLayout(0);
	//g_bChineseIME = (PRIMARYLANGID(LOWORD(hkl)) == LANG_CHINESE) && _ImmIsIME(hkl);

	// Turn off IME toolbar
	HWND hwndImeDef = _ImmGetDefaultIMEWnd(g_hwndMain);
	if (hwndImeDef) {
		SendMessage(hwndImeDef, WM_IME_CONTROL, IMC_CLOSESTATUSWINDOW, 0);
	}

	g_CaretInfo.caretX = 0;
	g_CaretInfo.caretY = 0;
	g_CaretInfo.fontHandle = 0;
	g_CaretInfo.colorComp = 0;
	g_CaretInfo.colorCand = 0;
	g_CaretInfo.margins.left = 0;
	g_CaretInfo.margins.right = 640;
	g_CaretInfo.margins.top = 0;
	g_CaretInfo.margins.bottom = 480;

	g_uCodePage = gLocMgr.GetCodePage();
	switch (g_uCodePage)
	{
	// Simplified Chinese (TBD)
	case CODEPAGE_CHS:
		g_SymbolIME[0] = 0xa4;
		g_SymbolIME[1] = 0xa4;
		break;
	// Trad Chinese (Taiwan)
	case CODEPAGE_CHT:
		g_SymbolIME[0] = 0xa4;
		g_SymbolIME[1] = 0xa4;
		break;
	// Korean (TBD)
	case CODEPAGE_KOR:
		g_SymbolIME[0] = 0xb0;
		g_SymbolIME[1] = 0xa1;
		g_bVerticalCand = false;
		break;
	// Japanese	& other	(0x411 = Japanese)
	case CODEPAGE_JPN:
		g_SymbolIME[0] = 0x82;
		g_SymbolIME[1] = 0xa0;
		break;
	default:
		g_SymbolIME[0] = 'A';
		g_SymbolIME[1] = 0;
	}
	g_SymbolIME[2] = 0x00;

	ImeUi_SetSupportLevel(3);
	gpassert(g_dwIMELevel == 2 || g_dwIMELevel == 3);

	CheckToggleState();
	ImeUi_EnableIme(false);

	g_bInitialized = true;
	return true;
}

///////////////////////////////////
// Check Taiwan New Phonetic IME //
///////////////////////////////////

static DWORD GetTaiwanIMEVersion()
{
	static DWORD dwRet = 0;

	DWORD 	dwVerSize;
	DWORD 	dwVerHandle;
	LPVOID	lpVerBuffer;
	LPVOID	lpVerData;
	UINT	cbVerData;

	char szTmp[1024];
	static HKL hklPrev = 0;

	HKL kl = GetKeyboardLayout(0);
	SPEW(("jreicher", "GetKeyboardLayout(0)=%08lX", kl ));
	if (hklPrev == kl)
		return dwRet;
	hklPrev = kl;

	if ( (kl != _TAIWAN_HKL) && (kl != _TAIWAN_HKL2) ){ // check non-Taiwan env
		goto error;
	}

	if ( _ImmGetIMEFileName( kl, szTmp, sizeof(szTmp)-1 ) <= 0 ){
		goto error;
	}
	SPEW(("jreicher", "ImmGetIMEFileName=[%s]", szTmp));

	if ( !_GetReadingString )	// IME that doesn't implement reading string API
	{
		if ( (strcmp( szTmp, _TAIWAN_IMEFILENAME) != 0) && (strcmp( szTmp, 
	_TAIWAN_IMEFILENAME2) != 0)
			&& (strcmp( szTmp, _TAIWAN_IMEFILENAME3) != 0))
			goto error;
	}

	dwVerSize = GetFileVersionInfoSize( szTmp, &dwVerHandle );
	if( dwVerSize )
	{
		lpVerBuffer = (LPVOID) ImeUiCallback_Malloc(dwVerSize);
		if( lpVerBuffer )
		{
			if( GetFileVersionInfo( szTmp, dwVerHandle, dwVerSize, lpVerBuffer ) )
			{
				if( VerQueryValue( lpVerBuffer, "\\", &lpVerData, &cbVerData ) )
				{
					#define pVerFixedInfo ((VS_FIXEDFILEINFO FAR*)lpVerData)
					SPEW(("jreicher", "FileVersion=[%d.%d.%d.%d]", HIWORD(pVerFixedInfo->dwFileVersionMS), 
							LOWORD(pVerFixedInfo->dwFileVersionMS), 
							HIWORD(pVerFixedInfo->dwFileVersionLS), 
							LOWORD(pVerFixedInfo->dwFileVersionLS) ));
					DWORD dwVer = pVerFixedInfo->dwFileVersionMS;
					if ( _GetReadingString ||
						dwVer == _TAIWAN_IMEVERSION42 ||
					     dwVer == _TAIWAN_IMEVERSION43 ||
					     dwVer == _TAIWAN_IMEVERSION44 ||
					     dwVer == _TAIWAN_IMEVERSION50 ||
					     dwVer == _TAIWAN_IMEVERSION51 ||
					     dwVer == _TAIWAN_IMEVERSION52
						 ){
						dwRet = dwVer;
						ImeUiCallback_Free(lpVerBuffer);
						return dwRet;
					}
					#undef pVerFixedInfo
				}
			}
		}
		ImeUiCallback_Free(lpVerBuffer);
	}

// The flow comes here in the following conditions
// - Non Taiwan IME input locale
// - Older Taiwan IME
// - Other error cases
error:
	dwRet = 0;
	return dwRet;
}

static void GetReadingString(HWND hWnd)
{
	DWORD dwTaiwanIMEVersion = GetTaiwanIMEVersion();
	if ( ! dwTaiwanIMEVersion )
		return;

	HIMC himc;

	himc = _ImmGetContext(hWnd);
	if ( !himc )
	{
		return;
	}

	DWORD dwlen = 0;
	DWORD dwerr = 0;
	WCHAR *wstr = 0;
	bool unicode = FALSE;
	LPINPUTCONTEXT lpIMC = NULL;

	if ( _GetReadingString )
	{
		BOOL bVertical;
		UINT uMaxUiLen;
		dwlen =	_GetReadingString( himc, 0, NULL, (PINT)&dwerr, &bVertical, &uMaxUiLen );
		if ( dwlen )
		{
			wstr = (LPWSTR)_alloca( dwlen );
			dwlen = _GetReadingString( himc, dwlen, wstr, (PINT)&dwerr, &bVertical, &uMaxUiLen );
		}

		g_bHorizontalReading = bVertical == FALSE;
		unicode = true;
	}
	else
	{
		lpIMC = _ImmLockIMC(himc);

		// *** hacking code from Michael Yang ***

		LPBYTE p = 0;

		switch ( dwTaiwanIMEVersion ){

		case _TAIWAN_IMEVERSION42: // New(Phonetic/ChanJie)IME98  : 4.2.x.x // Win98
		case _TAIWAN_IMEVERSION43: // New(Phonetic/ChanJie)IME98a : 4.3.x.x // WinMe, Win2k
		case _TAIWAN_IMEVERSION44: // New ChanJie IME98b          : 4.4.x.x // WinXP
			p = *(LPBYTE *)((LPBYTE)_ImmLockIMCC(lpIMC->hPrivate) + 24);
			dwlen = *(DWORD *)(p + 7*4 + 32*4);	//m_dwInputReadStrLen
			dwerr = *(DWORD *)(p + 8*4 + 32*4);	//m_dwErrorReadStrStart
			wstr = (WCHAR *)(p + 56);
			unicode = TRUE;
			break;

		case _TAIWAN_IMEVERSION50: // 5.0.x.x // WinME

			p = *(LPBYTE *)((LPBYTE)_ImmLockIMCC(lpIMC->hPrivate) + 3*4); // PCKeyCtrlManager
			p = *(LPBYTE *)((LPBYTE)p + 1*4 + 5*4 + 4*2 ); // = PCReading = &STypingInfo
			dwlen = *(DWORD *)(p + 1*4 + (16*2+2*4) + 5*4 + 16);		//m_dwDisplayStringLength;
			dwerr = *(DWORD *)(p + 1*4 + (16*2+2*4) + 5*4 + 16 + 1*4);	//m_dwDisplayErrorStart;
			wstr = (WCHAR *)(p + 1*4 + (16*2+2*4) + 5*4);
			unicode = FALSE;
			break;

		case _TAIWAN_IMEVERSION51: // 5.1.x.x // IME2002(w/OfficeXP)
		case _TAIWAN_IMEVERSION52: // 5.2.x.x // (w/whistler)

			p = *(LPBYTE *)((LPBYTE)_ImmLockIMCC(lpIMC->hPrivate) + 4);   // PCKeyCtrlManager
			p = *(LPBYTE *)((LPBYTE)p + 1*4 + 5*4);                       // = PCReading = &STypingInfo
			dwlen = *(DWORD *)(p + 1*4 + (16*2+2*4) + 5*4 + 16 * 2);		//m_dwDisplayStringLength;
			dwerr = *(DWORD *)(p + 1*4 + (16*2+2*4) + 5*4 + 16 * 2 + 1*4);	//m_dwDisplayErrorStart;
			wstr  = (WCHAR *) (p + 1*4 + (16*2+2*4) + 5*4);
			unicode = TRUE;
			break;
		}

		g_szCandidate[0][0] = 0;
		g_szCandidate[1][0] = 0;
		g_szCandidate[2][0] = 0;
		g_szCandidate[3][0] = 0;
	}
	g_dwCount = dwlen;
	g_dwSelection = (DWORD)-1; // do not select any char
	if ( unicode )
	{
		for ( int i=0; (DWORD)i < dwlen ; i++ ) // dwlen > 0, if known IME : yutakah
		{
			if ( dwerr<=(DWORD)i && g_dwSelection==-1 ){ // select error char
				g_dwSelection = i;
			}

			char mbc[3];
			mbc[1] = 0;
			mbc[2] = 0;
			wctomb(mbc, wstr[i]);

			g_szCandidate[i][0] = mbc[0];
			g_szCandidate[i][1] = mbc[1];
			g_szCandidate[i][2] = 0;


			SPEW(( "jreicher", "  [%d]: UNICODE %04x MBCS %s", i, wstr[i], mbc ));
		}
		g_szCandidate[i][0] = 0;
	}
	else
	{
		TCHAR *p = (TCHAR *)wstr;
		for ( int i=0, j=0; (DWORD)i < dwlen ; i++,j++ ) // dwlen > 0, if known IME : yutakah
		{
			if ( dwerr<=(DWORD)i && g_dwSelection==(DWORD)-1 ){
				g_dwSelection = (DWORD)j;
			}
			g_szCandidate[j][0] = p[i];
			g_szCandidate[j][1] = 0;
			g_szCandidate[j][2] = 0;
			if (::isleadbyte(p[i]))
			{
				i++;
				g_szCandidate[j][1] = p[i];
			}

			SPEW(( "jreicher", "  [%d]: MBCS %s", i, g_szCandidate[j] ));

		}
		g_szCandidate[j][0] = 0;
		g_dwCount = j;
	}
	if ( !_GetReadingString )
	{
		if ( lpIMC )
		{
			_ImmUnlockIMCC( lpIMC->hPrivate );
		}

		_ImmUnlockIMC( himc );

		// $$$ What is this for?
		SetReadingWindowOrientation( dwTaiwanIMEVersion );
	}
	_ImmReleaseContext(hWnd, himc);

	g_bReadingWindow = true;
	if (g_bHorizontalReading) {
		g_iReadingError = -1;
		g_szReadingString[0] = 0;
		for (UINT i = 0; i < g_dwCount; i++) {
			if (g_dwSelection == (DWORD)i)
				g_iReadingError = strlen(g_szReadingString);
			strcat(g_szReadingString, g_szCandidate[i]);
		}
	}
}


#define COUNTOF(a) (sizeof(a)/sizeof((a)[0]))

static struct {
	bool m_bCtrl;
	bool m_bShift;
	UINT m_uVk;
}
aHotKeys[] = {
	false,	false,	VK_APPS,
	true,	false,	'8',
	true,	false,	'Y',
	true,	false,	VK_DELETE,
	true,	false,	VK_F7,
	true,	false,	VK_F9,
	true,	false,	VK_F10,
	true,	false,	VK_F11,
	true,	false,	VK_F12,
	false,	false,	VK_F2,
	false,	false,	VK_F3,
	false,	false,	VK_F4,
	false,	false,	VK_F5,
	false,	false,	VK_F10,
	false,	true,	VK_F6,
	false,	true,	VK_F7,
	false,	true,	VK_F8,
	true,	true,	VK_F10,
	true,	false,	VK_SPACE,
	true,	false,	VK_BACK,
};

//
// Ignores specific keys when IME is on. Returns true if the message is a hot key to ignore.
// - Caller doesn't have to check whether IME is on.
// - This function must be called before TranslateMessage() is called.
//
bool ImeUi_IgnoreHotKey(MSG* pmsg)
{
	if ( !g_bInitialized || !pmsg )
		return false;
	if (pmsg->wParam == VK_PROCESSKEY && (pmsg->message == WM_KEYDOWN || 
pmsg->message == WM_SYSKEYDOWN))
	{
		bool bCtrl, bShift;
		UINT uVkReal = _ImmGetVirtualKey(pmsg->hwnd);
		bCtrl = (GetKeyState(VK_CONTROL) & 0x8000) ? true : false;
		bShift = (GetKeyState(VK_SHIFT) & 0x8000) ? true : false;
		for (int i = 0; i < COUNTOF(aHotKeys); i++) {
			if (aHotKeys[i].m_bCtrl == bCtrl &&
				aHotKeys[i].m_bShift == bShift &&
				aHotKeys[i].m_uVk == uVkReal)
				return true;
		}
	}
	return false;
}

void ImeUi_FinalizeString(void)
{
	HIMC himc;
	if (!g_bInitialized || NULL == (himc = _ImmGetContext(g_hwndMain)))
		return;

	static bool bProcessing = false;
	if (bProcessing)		// avoid infinite recursion
		return;
	bProcessing = true;

	if (g_dwIMELevel == 2)
	{
		// Send composition string to app.
		LONG lRet = strlen(g_szCompositionString);
		if (g_uCodePage == CODEPAGE_CHT
			&& (BYTE)(g_szCompositionString[lRet - 2]) == 0xa1
			&& (BYTE)(g_szCompositionString[lRet - 1]) == 0x40) {
			lRet -= 2;
		}
		bSendingKeyMsg = true;
		for (int i = 0; i < lRet ; i++)
			SendMessage(g_hwndMain, WM_CHAR, (unsigned char)g_szCompositionString[i], 0);
		bSendingKeyMsg = false;
	}

	IMEInitCompStringData();
	// clear composition string in IME
	_ImmNotifyIME(himc, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
	_ImmReleaseContext(g_hwndMain, himc);

	bProcessing = false;
	return;
}


void ImeUI_CancelString(void)
{
	HIMC himc;
	if (!g_bInitialized || NULL == (himc = _ImmGetContext(g_hwndMain)))
		return;

	IMEInitCompStringData();
	// clear composition string in IME
	_ImmNotifyIME(himc, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
	_ImmReleaseContext(g_hwndMain, himc);
	memset(&g_szCandidate, 0, sizeof(g_szCandidate));
}

static void SetCompStringColor()
{
	// change color setting according to current IME level.
	DWORD dwTranslucency = (g_dwIMELevel == 2) ? 0xff000000 : 
((DWORD)gSkinIME.compTranslucence << 24);
	gSkinCompStr.colorInput			= dwTranslucency | gSkinIME.compColorInput;
	gSkinCompStr.colorTargetConv	= dwTranslucency | 
gSkinIME.compColorTargetConv;
	gSkinCompStr.colorConverted		= dwTranslucency | 
gSkinIME.compColorConverted;
	gSkinCompStr.colorTargetNotConv = dwTranslucency | 
gSkinIME.compColorTargetNotConv;
	gSkinCompStr.colorInputErr		= dwTranslucency | gSkinIME.compColorInputErr;
}

void ImeUi_SetSupportLevel(DWORD dwImeLevel)
{
	if (dwImeLevel < 2 || 3 < dwImeLevel)
		return;
	if (g_uCodePage == CODEPAGE_KOR)
		dwImeLevel = 3;
	g_dwIMELevel = dwImeLevel;
	// cancel current composition string.
	ImeUi_FinalizeString();
	SetCompStringColor();
}

void ImeUi_SetAppearance( IMEUI_APPEARANCE* pia )
{
	if (NULL == pia)
		return;
	gSkinIME = *pia;
	gSkinIME.symbolColor			&= 0xffffff; // mask translucency
	gSkinIME.symbolColorText		&= 0xffffff; // mask translucency
	gSkinIME.compColorInput			&= 0xffffff; // mask translucency
	gSkinIME.compColorTargetConv	&= 0xffffff; // mask translucency
	gSkinIME.compColorConverted		&= 0xffffff; // mask translucency
	gSkinIME.compColorTargetNotConv	&= 0xffffff; // mask translucency
	gSkinIME.compColorInputErr		&= 0xffffff; // mask translucency
	SetCompStringColor();
}

static void CheckToggleState()
{
	g_hkl = GetKeyboardLayout(0);
	g_bIme = _ImmIsIME(g_hkl) != 0;
	g_bChineseIME = (PRIMARYLANGID(LOWORD(g_hkl)) == LANG_CHINESE) && g_bIme;

	HIMC himc;
	if (NULL != (himc = _ImmGetContext(g_hwndMain))) {
		if (g_bChineseIME) {
			DWORD dwConvMode, dwSentMode;
			_ImmGetConversionStatus(himc, &dwConvMode, &dwSentMode);
			g_dwState = ( dwConvMode & IME_CMODE_NATIVE ) ? IMEUI_STATE_ON : IMEUI_STATE_ENGLISH;
		}
		else
		{
			g_dwState = ( g_bIme && _ImmGetOpenStatus( himc ) != 0 ) ? IMEUI_STATE_ON : IMEUI_STATE_OFF;
		}
		_ImmReleaseContext(g_hwndMain, himc);
	}
	else
		g_dwState = IMEUI_STATE_OFF;
}

void ImeUi_SetInsertMode(bool bInsert)
{
	g_bInsertMode = bInsert;
}

bool ImeUi_GetCaretStatus()
{
	return !(g_uCodePage == CODEPAGE_KOR && g_szCompositionString[0]);
}

void ImeUi_SetScreenDimension(UINT width, UINT height)
{
	g_screenWidth = width;
	g_screenHeight = height;
}

// this function is used only in brief time in CHT IME handling, so accelerator isn't processed.
static void _PumpMessage()
{
	MSG msg;
	while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
		if (!GetMessageA(&msg, NULL, 0, 0)) {
			PostQuitMessage(msg.wParam);
			return;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

static void DrawImeIndicator()
{	
	bool bOn = g_dwState != IMEUI_STATE_OFF;
	IMEUI_VERTEX PieData[17];
	float SizeOfPie = (float) gSkinIME.symbolHeight;

	memset( &PieData[0], 0, sizeof(PieData) );

	switch(gSkinIME.symbolPlacement)
	{
	case 0:
		{
			if (SizeOfPie + g_CaretInfo.margins.right+4 > g_screenWidth)
			{
				PieData[0].sx=(-SizeOfPie/2) + g_CaretInfo.margins.left-4;
				PieData[0].sy= (float) g_CaretInfo.margins.bottom - 
(gSkinIME.symbolHeight/2);
			}
			else
			{
				PieData[0].sx=-(SizeOfPie/2) + 
g_CaretInfo.margins.right+gSkinIME.symbolHeight+4;
				PieData[0].sy= (float) g_CaretInfo.margins.bottom - 
(gSkinIME.symbolHeight/2);
			}
			break;
		}
	case 1: // upperleft
		PieData[0].sx = 4+ (SizeOfPie/2);
		PieData[0].sy = 4+ (SizeOfPie/2);
		break;
	case 2: // upperright
		PieData[0].sx = g_screenWidth - (4+ (SizeOfPie/2));
		PieData[0].sy = 4+ (SizeOfPie/2);
		break;
	case 3: // lowerright
		PieData[0].sx = g_screenWidth - (4+ (SizeOfPie/2));
		PieData[0].sy = g_screenHeight - (4+ (SizeOfPie/2));
		break;
	case 4: // lowerleft
		PieData[0].sx = 4+ (SizeOfPie/2);
		PieData[0].sy = g_screenHeight - (4+ (SizeOfPie/2));
		break;
	}
	PieData[0].rhw=1.0f;
	if (bOn)
	{
		if (GetSystemSeconds() - lastSwirl > g_uSwirlTime )
		{
			swirl++;
			lastSwirl = GetSystemSeconds();
			if (swirl > 13)
				swirl = 0;
		}
	}
	else
		swirl = 0;
	// Reverse the vertices to prevent from being culled by Rapi. - CQ
	int i = 1;
	for ( int t1 = 15; t1 > 0; t1-- )
	{
		PieData[t1].sx=(float)(PieData[0].sx+SizeOfPie/2*sin( 
(2.0f*3.1415926f*(i-1+(bOn*swirl)))/14.0f ));
		PieData[t1].sy=(float)(PieData[0].sy-SizeOfPie/2*cos( 
(2.0f*3.1415926f*(i-1+(bOn*swirl)))/14.0f ));
		PieData[t1].rhw=1.0f;
		++i;
	}
	PieData[16].sx=(float)(PieData[0].sx+SizeOfPie/2*sin( 0 ));
	PieData[16].sy=(float)(PieData[0].sy-SizeOfPie/2*cos( 0 ));
	PieData[16].rhw=1.0f;

	if (!gSkinIME.symbolColor)
	{
		if (!bOn)
		{
			PieData[0].color=0xffffff +  (((DWORD)gSkinIME.symbolTranslucence)<<24);
			for( int t1=1; t1<16; t1++ )
			{
				PieData[t1].color=0x404040 + (((DWORD)gSkinIME.symbolTranslucence)<<24);
			}
		}
		else
		{
		 	PieData[0].color=0xffffff + (((DWORD)gSkinIME.symbolTranslucence)<<24);
	 		PieData[1].color=0xff0000 + (((DWORD)gSkinIME.symbolTranslucence)<<24);
		 	PieData[2].color=0xff3000 + (((DWORD)gSkinIME.symbolTranslucence)<<24);
		 	PieData[3].color=0xff6000 + (((DWORD)gSkinIME.symbolTranslucence)<<24);
	 		PieData[4].color=0xff9000 + (((DWORD)gSkinIME.symbolTranslucence)<<24);
		 	PieData[5].color=0xffC000 + (((DWORD)gSkinIME.symbolTranslucence)<<24);
		 	PieData[6].color=0xffff00 + (((DWORD)gSkinIME.symbolTranslucence)<<24);
	 		PieData[7].color=0xC0ff00 + (((DWORD)gSkinIME.symbolTranslucence)<<24);
		 	PieData[8].color=0x90ff00 + (((DWORD)gSkinIME.symbolTranslucence)<<24);
		 	PieData[9].color=0x60ff00 + (((DWORD)gSkinIME.symbolTranslucence)<<24);
	 		PieData[10].color=0x30c0ff + (((DWORD)gSkinIME.symbolTranslucence)<<24);
		 	PieData[11].color=0x00a0ff + (((DWORD)gSkinIME.symbolTranslucence)<<24);
		 	PieData[12].color=0x3090ff + (((DWORD)gSkinIME.symbolTranslucence)<<24);
	 		PieData[13].color=0x6060ff + (((DWORD)gSkinIME.symbolTranslucence)<<24);
		 	PieData[14].color=0x9030ff + (((DWORD)gSkinIME.symbolTranslucence)<<24);
		 	PieData[15].color=0xc000ff + (((DWORD)gSkinIME.symbolTranslucence)<<24);
		}
	}
	else
	{
		PieData[0].color=0xffffff +  (((DWORD)gSkinIME.symbolTranslucence)<<24);
		for( int t1=1; t1<16; t1++ )
		{
			PieData[t1].color=gSkinIME.symbolColor + 
(((DWORD)gSkinIME.symbolTranslucence)<<24);
		}
	}
	ImeUiCallback_DrawFans( PieData, 16 );

	float fHeight = gSkinIME.symbolHeight*0.625f;

	// fix for Ent Gen #120 - reduce the height of character when Korean IME is on
	if (g_uCodePage == CODEPAGE_KOR && bOn)
		fHeight *= 0.8f;

	// GOS deals height in points that is 1/72nd inch and assumes display device is 96dpi.
	fHeight = fHeight * 96 / 72;

	ImeUiCallback_SetFont(g_CaretInfo.fontHandle);
	ImeUiCallback_SetFontAttribute((UINT)fHeight, bOn);
	ImeUiCallback_SetTextColor((((DWORD)gSkinIME.symbolTranslucence)<<24) | 
gSkinIME.symbolColorText);

	//
	// draw the proper symbol over the fan
	//
	DWORD  w, h;
	const char* cszSymbol = ( g_dwState == IMEUI_STATE_ON ) ? (const char*)g_SymbolIME : (const char*)"A";

	ImeUiCallback_GetStringDimension(cszSymbol, &w, &h);
	ImeUiCallback_SetTextPosition((int)(PieData[0].sx)-w/2, (int)(PieData[0].sy)-h/2);
	ImeUiCallback_DrawText( cszSymbol );
}


void ImeUi_SetSwirlTimeIncrement( double time )
{
	g_uSwirlTime = time;
}

static void SetReadingWindowOrientation(DWORD dwVer)
{
	HKL kl = GetKeyboardLayout(0);
	g_bHorizontalReading = (kl == _TAIWAN_HKL2) || (dwVer == 0);
	if (!g_bHorizontalReading) {
		char szRegPath[MAX_PATH];
		HKEY hkey;
		wsprintf(szRegPath, "software\\microsoft\\windows\\currentversion\\%s",
			(dwVer >= _TAIWAN_IMEVERSION51) ? "MSTCIPH" : "TINTLGNT");
		LONG lRc = RegOpenKeyEx(HKEY_CURRENT_USER, szRegPath, 0, KEY_READ, &hkey);
		if (lRc == ERROR_SUCCESS) {
			DWORD dwSize = sizeof(DWORD), dwMapping, dwType;
			lRc = RegQueryValueEx(hkey, "keyboard mapping", NULL, &dwType, 
(PBYTE)&dwMapping, &dwSize);
			if (lRc == ERROR_SUCCESS) {
				if (
					(dwVer <= _TAIWAN_IMEVERSION50 &&
						((BYTE)dwMapping == 0x22 || (BYTE)dwMapping == 0x23)) ||
					((dwVer == _TAIWAN_IMEVERSION51 || dwVer == _TAIWAN_IMEVERSION52) &&
						((BYTE)dwMapping >= 0x22 && (BYTE)dwMapping <= 0x24))) {
					g_bHorizontalReading = true;
				}
			}
		}
	}
}

void ImeUi_ToggleLanguageBar(BOOL bRestore)
{
	if (!GPCoInitialize())
		return;
	ITfLangBarMgr* plbm = NULL;
	HRESULT hr;
	hr = CoCreateInstance(CLSID_TF_LangBarMgr, NULL, CLSCTX_INPROC_SERVER, 
IID_ITfLangBarMgr, (void**)&plbm);
	if (!SUCCEEDED(hr) || !plbm)
		return;
	DWORD dwCur;
	ULONG uRc;
	if (SUCCEEDED(hr)) {
		if (bRestore) {
			if (g_dwPrevFloat)
				hr = plbm->ShowFloating(g_dwPrevFloat);
		}
		else {
			hr = plbm->GetShowFloatingStatus(&dwCur);
			if (SUCCEEDED(hr))
				g_dwPrevFloat = dwCur;
			hr = plbm->ShowFloating(TF_SFT_HIDDEN);
		}
	}
	uRc = plbm->Release();
	CoUninitialize();
}

bool ImeUi_IsSendingKeyMessage()
{
	return bSendingKeyMsg;
}

void SetImeApi()
{
	char szImeFile[MAX_PATH + 1];
	HKL kl = g_hkl;
	_GetReadingString = NULL;
	_ShowReadingWindow = NULL;
	if ( _ImmGetIMEFileName( kl, szImeFile, sizeof(szImeFile) - 1 ) <= 0 )
		return;
	HMODULE hIme = LoadLibrary( szImeFile );
	if ( !hIme )
		return;
	_GetReadingString = (UINT (WINAPI*)(HIMC, UINT, LPWSTR, PINT, BOOL*, PUINT))
		(GetProcAddress(hIme, "GetReadingString"));
	_ShowReadingWindow =(BOOL (WINAPI*)(HIMC himc, BOOL))
		(GetProcAddress(hIme, "ShowReadingWindow"));
}
