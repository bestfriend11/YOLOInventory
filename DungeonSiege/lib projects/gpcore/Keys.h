/*=============================================================================

  Keys.h

  purpose:	Key input related for key enumeration definitions and translations

  author:	Rick Saenz, Scott Bilas

  (C)opyright Gas Powered Games 2000

-----------------------------------------------------------------------------*/

#pragma once
#ifndef _KEYS_H_
#define _KEYS_H_

namespace Keys
{

// Key type definition.

	struct  KEY__  {};
	typedef KEY__* KEY;
	#define MAKE_KEY(val) ((KEY)val)
	inline DWORD MakeInt( KEY key )  {  return ( (DWORD)key );  }

// Key qualifier flags.

	// $ note: the "special" key is going to be language-specific. for english
	//         keyboards it will be the tilde key. other language versions (for
	//         localized keyboards that don't have this key) will be able to
	//         reconfigure this key via the registry.

	enum QUALIFIER
	{
		Q_NONE				= 0,

	// Add-qualifiers.

		Q_SHIFT				= 0x00000001,
		Q_CONTROL			= 0x00000002,
		Q_ALT				= 0x00000004,
		Q_SPECIAL			= 0x00000008,
		Q_KEY_DOWN			= 0x00000010,
		Q_KEY_UP			= 0x00000020,
		Q_REPEAT			= 0x00000040,
		Q_ANY_KEY			= Q_SHIFT | Q_CONTROL | Q_ALT | Q_SPECIAL | Q_KEY_DOWN | Q_KEY_UP | Q_REPEAT,

		Q_LBUTTON			= 0x00000080,
		Q_MBUTTON			= 0x00000100,
		Q_RBUTTON			= 0x00000200,
		Q_XBUTTON1			= 0x00000400,
		Q_XBUTTON2			= 0x00000800,
		Q_ANY_BUTTON		= Q_LBUTTON | Q_MBUTTON | Q_RBUTTON | Q_XBUTTON1 | Q_XBUTTON2,

	// Ignore-qualifiers.

		Q_IGNORE_SHIFT		= 0x00001000,
		Q_IGNORE_CONTROL	= 0x00002000,
		Q_IGNORE_ALT		= 0x00004000,
		Q_IGNORE_SPECIAL	= 0x00008000,
		Q_IGNORE_KEY_DOWN	= 0x00010000,
		Q_IGNORE_KEY_UP		= 0x00020000,
		Q_IGNORE_KEYS		= Q_IGNORE_SHIFT | Q_IGNORE_CONTROL | Q_IGNORE_ALT | Q_IGNORE_SPECIAL | Q_IGNORE_KEY_DOWN | Q_IGNORE_KEY_UP,

		Q_IGNORE_LBUTTON	= 0x00040000,
		Q_IGNORE_MBUTTON	= 0x00080000,
		Q_IGNORE_RBUTTON	= 0x00100000,
		Q_IGNORE_XBUTTON1	= 0x00200000,
		Q_IGNORE_XBUTTON2	= 0x00400000,
		Q_IGNORE_BUTTONS	= Q_IGNORE_LBUTTON | Q_IGNORE_MBUTTON | Q_IGNORE_RBUTTON | Q_IGNORE_XBUTTON1 | Q_IGNORE_XBUTTON2,

		Q_IGNORE_QUALIFIERS	= Q_IGNORE_KEYS | Q_IGNORE_BUTTONS,
		Q_QUALIFIER_MASK	= 0x00000FFF,
		Q_IGNORE_MASK		= 0x00FFF000,
		Q_IGNORE_BITSHIFT	= 12,
	};

	bool        FromString        ( const char* str, QUALIFIER& q );	// bit at a time
	bool        FromFullString    ( const char* str, QUALIFIER& q );	// full qualifier at a time
	const char* ToString          ( QUALIFIER q );
	gpstring    ToFullString      ( QUALIFIER q );
	gpwstring   ToScreenString    ( QUALIFIER q );
	gpwstring   ToFullScreenString( QUALIFIER q );

	MAKE_ENUM_BIT_OPERATORS( QUALIFIER );

// Key codes.

	// $ note: there are two constants defined here - first is the "int"
	//   version, which can be used in a switch statement for performance. use
	//   MakeInt() on a KEY type for comparison. the second is the "normal" KEY
	//   type. examples: KEY_INT_F1 is a DWORD, and KEY_F1 is a KEY. note that
	//   "special" keys such as the grave etc. only exist in KEY form (as an
	//   external const) and don't exist as a local const (so switch is not
	//   possible).

	#define DEFINE_KEY( local, vk ) \
		const DWORD KEY_INT_##local = vk; \
		const KEY KEY_##local = MAKE_KEY( vk )

	DEFINE_KEY( INVALID,     (DWORD)-1     );
	DEFINE_KEY( NONE,        0             );

	DEFINE_KEY( LBUTTON,     VK_LBUTTON    );
	DEFINE_KEY( RBUTTON,     VK_RBUTTON    );
	DEFINE_KEY( BREAK,       VK_CANCEL     );
	DEFINE_KEY( MBUTTON,     VK_MBUTTON    );
	DEFINE_KEY( XBUTTON1,    VK_XBUTTON1   );
	DEFINE_KEY( XBUTTON2,    VK_XBUTTON2   );

	DEFINE_KEY( BACK,        VK_BACK       );
	DEFINE_KEY( TAB,         VK_TAB        );

	DEFINE_KEY( CLEAR,       VK_CLEAR      );
	DEFINE_KEY( RETURN,      VK_RETURN     );

	DEFINE_KEY( SHIFT,       VK_SHIFT      );
	DEFINE_KEY( CONTROL,     VK_CONTROL    );
	DEFINE_KEY( ALT,         VK_MENU       );
	DEFINE_KEY( PAUSE,       VK_PAUSE      );
	DEFINE_KEY( CAPSLOCK,    VK_CAPITAL    );

	DEFINE_KEY( BROWSER_BACK,        VK_BROWSER_BACK        );
	DEFINE_KEY( BROWSER_FORWARD,     VK_BROWSER_FORWARD     );
	DEFINE_KEY( BROWSER_REFRESH,     VK_BROWSER_REFRESH     );
	DEFINE_KEY( BROWSER_STOP,        VK_BROWSER_STOP        );
	DEFINE_KEY( BROWSER_SEARCH,      VK_BROWSER_SEARCH      );
	DEFINE_KEY( BROWSER_FAVORITES,   VK_BROWSER_FAVORITES   );
	DEFINE_KEY( BROWSER_HOME,        VK_BROWSER_HOME        );
	DEFINE_KEY( VOLUME_MUTE,         VK_VOLUME_MUTE         );
	DEFINE_KEY( VOLUME_DOWN,         VK_VOLUME_DOWN         );
	DEFINE_KEY( VOLUME_UP,           VK_VOLUME_UP           );
	DEFINE_KEY( MEDIA_NEXT_TRACK,    VK_MEDIA_NEXT_TRACK    );
	DEFINE_KEY( MEDIA_PREV_TRACK,    VK_MEDIA_PREV_TRACK    );
	DEFINE_KEY( MEDIA_STOP,          VK_MEDIA_STOP          );
	DEFINE_KEY( MEDIA_PLAY_PAUSE,    VK_MEDIA_PLAY_PAUSE    );
	DEFINE_KEY( LAUNCH_MAIL,         VK_LAUNCH_MAIL         );
	DEFINE_KEY( LAUNCH_MEDIA_SELECT, VK_LAUNCH_MEDIA_SELECT );
	DEFINE_KEY( LAUNCH_APP1,         VK_LAUNCH_APP1         );
	DEFINE_KEY( LAUNCH_APP2,         VK_LAUNCH_APP2         );

	DEFINE_KEY( KANA,        VK_KANA       );
	DEFINE_KEY( JUNJA,       VK_JUNJA      );
	DEFINE_KEY( FINAL,       VK_FINAL      );
	DEFINE_KEY( HANJA,       VK_HANJA      );
	DEFINE_KEY( KANJI,       VK_KANJI      );

	DEFINE_KEY( ESCAPE,      VK_ESCAPE     );

	DEFINE_KEY( SPACE,       VK_SPACE      );
	DEFINE_KEY( PAGEUP,      VK_PRIOR      );
	DEFINE_KEY( PAGEDOWN,    VK_NEXT       );
	DEFINE_KEY( END,         VK_END        );
	DEFINE_KEY( HOME,        VK_HOME       );
	DEFINE_KEY( LEFT,        VK_LEFT       );
	DEFINE_KEY( UP,          VK_UP         );
	DEFINE_KEY( RIGHT,       VK_RIGHT      );
	DEFINE_KEY( DOWN,        VK_DOWN       );
	DEFINE_KEY( SELECT,      VK_SELECT     );
	DEFINE_KEY( PRINT,       VK_PRINT      );	// $ NOT the PrintScreen key
	DEFINE_KEY( PRINTSCREEN, VK_SNAPSHOT   );	// $ THIS is the PrintScreen key
	DEFINE_KEY( INSERT,      VK_INSERT     );
	DEFINE_KEY( DELETE,      VK_DELETE     );
	DEFINE_KEY( HELP,        VK_HELP       );

	DEFINE_KEY( 0,           '0'           );
	DEFINE_KEY( 1,           '1'           );
	DEFINE_KEY( 2,           '2'           );
	DEFINE_KEY( 3,           '3'           );
	DEFINE_KEY( 4,           '4'           );
	DEFINE_KEY( 5,           '5'           );
	DEFINE_KEY( 6,           '6'           );
	DEFINE_KEY( 7,           '7'           );
	DEFINE_KEY( 8,           '8'           );
	DEFINE_KEY( 9,           '9'           );

	DEFINE_KEY( A,           'A'           );
	DEFINE_KEY( B,           'B'           );
	DEFINE_KEY( C,           'C'           );
	DEFINE_KEY( D,           'D'           );
	DEFINE_KEY( E,           'E'           );
	DEFINE_KEY( F,           'F'           );
	DEFINE_KEY( G,           'G'           );
	DEFINE_KEY( H,           'H'           );
	DEFINE_KEY( I,           'I'           );
	DEFINE_KEY( J,           'J'           );
	DEFINE_KEY( K,           'K'           );
	DEFINE_KEY( L,           'L'           );
	DEFINE_KEY( M,           'M'           );
	DEFINE_KEY( N,           'N'           );
	DEFINE_KEY( O,           'O'           );
	DEFINE_KEY( P,           'P'           );
	DEFINE_KEY( Q,           'Q'           );
	DEFINE_KEY( R,           'R'           );
	DEFINE_KEY( S,           'S'           );
	DEFINE_KEY( T,           'T'           );
	DEFINE_KEY( U,           'U'           );
	DEFINE_KEY( V,           'V'           );
	DEFINE_KEY( W,           'W'           );
	DEFINE_KEY( X,           'X'           );
	DEFINE_KEY( Y,           'Y'           );
	DEFINE_KEY( Z,           'Z'           );

	DEFINE_KEY( LWIN,        VK_LWIN       );
	DEFINE_KEY( RWIN,        VK_RWIN       );
	DEFINE_KEY( APPS,        VK_APPS       );
	DEFINE_KEY( SLEEP,       VK_SLEEP      );

	DEFINE_KEY( NUMPAD0,     VK_NUMPAD0    );
	DEFINE_KEY( NUMPAD1,     VK_NUMPAD1    );
	DEFINE_KEY( NUMPAD2,     VK_NUMPAD2    );
	DEFINE_KEY( NUMPAD3,     VK_NUMPAD3    );
	DEFINE_KEY( NUMPAD4,     VK_NUMPAD4    );
	DEFINE_KEY( NUMPAD5,     VK_NUMPAD5    );
	DEFINE_KEY( NUMPAD6,     VK_NUMPAD6    );
	DEFINE_KEY( NUMPAD7,     VK_NUMPAD7    );
	DEFINE_KEY( NUMPAD8,     VK_NUMPAD8    );
	DEFINE_KEY( NUMPAD9,     VK_NUMPAD9    );
	DEFINE_KEY( MULTIPLY,    VK_MULTIPLY   );
	DEFINE_KEY( ADD,         VK_ADD        );
	DEFINE_KEY( SEPARATOR,   VK_SEPARATOR  );
	DEFINE_KEY( SUBTRACT,    VK_SUBTRACT   );
	DEFINE_KEY( DECIMAL,     VK_DECIMAL    );
	DEFINE_KEY( DIVIDE,      VK_DIVIDE     );

	DEFINE_KEY( F1,          VK_F1         );
	DEFINE_KEY( F2,          VK_F2         );
	DEFINE_KEY( F3,          VK_F3         );
	DEFINE_KEY( F4,          VK_F4         );
	DEFINE_KEY( F5,          VK_F5         );
	DEFINE_KEY( F6,          VK_F6         );
	DEFINE_KEY( F7,          VK_F7         );
	DEFINE_KEY( F8,          VK_F8         );
	DEFINE_KEY( F9,          VK_F9         );
	DEFINE_KEY( F10,         VK_F10        );
	DEFINE_KEY( F11,         VK_F11        );
	DEFINE_KEY( F12,         VK_F12        );
	DEFINE_KEY( F13,         VK_F13        );
	DEFINE_KEY( F14,         VK_F14        );
	DEFINE_KEY( F15,         VK_F15        );
	DEFINE_KEY( F16,         VK_F16        );
	DEFINE_KEY( F17,         VK_F17        );
	DEFINE_KEY( F18,         VK_F18        );
	DEFINE_KEY( F19,         VK_F19        );
	DEFINE_KEY( F20,         VK_F20        );
	DEFINE_KEY( F21,         VK_F21        );
	DEFINE_KEY( F22,         VK_F22        );
	DEFINE_KEY( F23,         VK_F23        );
	DEFINE_KEY( F24,         VK_F24        );
	DEFINE_KEY( NUMLOCK,     VK_NUMLOCK    );
	DEFINE_KEY( SCROLLLOCK,  VK_SCROLL     );

	#undef DEFINE_KEY

	extern const KEY KEY_GRAVE;
	extern const KEY KEY_MINUS;
	extern const KEY KEY_EQUALS;
	extern const KEY KEY_LBRACKET;
	extern const KEY KEY_RBRACKET;
	extern const KEY KEY_BACKSLASH;
	extern const KEY KEY_SEMICOLON;
	extern const KEY KEY_APOSTROPHE;
	extern const KEY KEY_COMMA;
	extern const KEY KEY_PERIOD;
	extern const KEY KEY_SLASH;

	bool FromChar( char c, KEY& key );
	KEY  FromChar( char c );
	char ToChar  ( KEY key );

	bool      FromString    ( const char* str, KEY& key );
	gpstring  ToString      ( KEY key );
	gpwstring ToScreenString( KEY key );

// Special key.

	// the "special" key is by default (for english keyboards) the grave/tilde
	// key. some keyboards don't have this key, in which case it should be
	// reconfigured from the registry or via GAS. the special key can act as
	// a "special shift" key if held down (Q_SPECIAL).

	extern const KEY& KEY_SPECIAL;

	void SetSpecialKey( KEY key );

} // end namespace Keys

#endif	// _KEYS_H_
