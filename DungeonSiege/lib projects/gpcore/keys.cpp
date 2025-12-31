/*=============================================================================

  Keys.h

  author:	Rick Saenz, Scott Bilas

  (C)opyright Gas Powered Games 2000

-----------------------------------------------------------------------------*/

#include "Precomp_GpCore.h"
#include "Keys.h"

#include "FuBiTraits.h"
#include "Fuel.h"
#include "StdHelp.h"
#include "StringTool.h"

//-----------------------------------------------------------------------------

namespace Keys {

//-----------------------------------------------------------------------------

// Computer names.

	static const stringtool::BitEnumStringConverter::Entry s_QualifierEntries[] =
	{
		{  "none",              Q_NONE               },

		{  "shift",             Q_SHIFT              },
		{  "control",           Q_CONTROL            },
		{  "alt",               Q_ALT                },
		{  "special",           Q_SPECIAL            },
		{  "key_down",          Q_KEY_DOWN           },
		{  "key_up",            Q_KEY_UP             },
		{  "repeat",            Q_REPEAT             },
		{  "any_key",           Q_ANY_KEY            },

		{  "lbutton",           Q_LBUTTON            },
		{  "mbutton",           Q_MBUTTON            },
		{  "rbutton",           Q_RBUTTON            },
		{  "xbutton1",          Q_XBUTTON1           },
		{  "xbutton2",          Q_XBUTTON2           },
		{  "any_button",        Q_ANY_BUTTON         },

		{  "ignore_shift",      Q_IGNORE_SHIFT       },
		{  "ignore_control",    Q_IGNORE_CONTROL     },
		{  "ignore_alt",        Q_IGNORE_ALT         },
		{  "ignore_special",    Q_IGNORE_SPECIAL     },
		{  "ignore_key_down",   Q_IGNORE_KEY_DOWN    },
		{  "ignore_key_up",     Q_IGNORE_KEY_UP      },
		{  "ignore_keys",       Q_IGNORE_KEYS        },

		{  "ignore_lbutton",    Q_IGNORE_LBUTTON     },
		{  "ignore_mbutton",    Q_IGNORE_MBUTTON     },
		{  "ignore_rbutton",    Q_IGNORE_RBUTTON     },
		{  "ignore_xbutton1",   Q_IGNORE_XBUTTON1    },
		{  "ignore_xbutton2",   Q_IGNORE_XBUTTON2    },
		{  "ignore_buttons",    Q_IGNORE_BUTTONS     },

		{  "ignore_qualifiers", Q_IGNORE_QUALIFIERS  },
	};

	static stringtool::BitEnumStringConverter s_QualifierConverter(
			s_QualifierEntries, ELEMENT_COUNT( s_QualifierEntries ), Q_NONE );

	bool FromString( const char* str, QUALIFIER& q )
	{
		return ( s_QualifierConverter.FromString( str, rcast <DWORD&> ( q ) ) );
	}

	bool FromFullString( const char* str, QUALIFIER& q )
	{
		return ( s_QualifierConverter.FromFullString( str, rcast <DWORD&> ( q ) ) );
	}

	const char* ToString( QUALIFIER q )
	{
		return ( s_QualifierConverter.ToString( q ) );
	}

	gpstring ToFullString( QUALIFIER q )
	{
		return ( s_QualifierConverter.ToFullString( q ) );
	}

// Human names. :)

	static const stringtool::BitEnumStringConverter::Entry s_ScreenQualifierEntries[] =
	{
		// $ note that only the "positive" qualifiers are listed here. the
		//   ignore's and any's are left out because they should never be
		//   visible to a gui. if you get assertions, then that's why.

		{        "",             Q_NONE     },
		{  $MSG$ "Shift",        Q_SHIFT    },
		{  $MSG$ "Ctrl",         Q_CONTROL  },
		{  $MSG$ "Alt",          Q_ALT      },
		{  $MSG$ "Special",      Q_SPECIAL  },
		{        "",             Q_KEY_DOWN },
		{        "",             Q_KEY_UP   },
		{  $MSG$ "Mouse Left",   Q_LBUTTON  },
		{  $MSG$ "Mouse Middle", Q_MBUTTON  },
		{  $MSG$ "Mouse Right",  Q_RBUTTON  },
		{  $MSG$ "Mouse X1",     Q_XBUTTON1 },
		{  $MSG$ "Mouse X2",     Q_XBUTTON2 },
	};

	static stringtool::BitEnumStringConverter s_ScreenQualifierConverter(
			true /*translate!*/, s_ScreenQualifierEntries, ELEMENT_COUNT( s_ScreenQualifierEntries ), Q_NONE );

	gpwstring ToScreenString( QUALIFIER q )
	{
		return ( ::ToUnicode( s_ScreenQualifierConverter.ToString( q ) ) );
	}

	gpwstring ToFullScreenString( QUALIFIER q )
	{
		return ( ::ToUnicode( s_ScreenQualifierConverter.ToFullString( q ) ) );
	}

//-----------------------------------------------------------------------------

// map by scan code rather than character code as recommended by Kazuyuki. they
// can be remapped via fuel as required for localized versions.
const KEY KEY_GRAVE      = MAKE_KEY( ::MapVirtualKey( 0x29, 1 ) );
const KEY KEY_MINUS      = MAKE_KEY( ::MapVirtualKey( 0x0C, 1 ) );
const KEY KEY_EQUALS     = MAKE_KEY( ::MapVirtualKey( 0x0D, 1 ) );
const KEY KEY_LBRACKET   = MAKE_KEY( ::MapVirtualKey( 0x1A, 1 ) );
const KEY KEY_RBRACKET   = MAKE_KEY( ::MapVirtualKey( 0x1B, 1 ) );
const KEY KEY_SEMICOLON  = MAKE_KEY( ::MapVirtualKey( 0x27, 1 ) );
const KEY KEY_APOSTROPHE = MAKE_KEY( ::MapVirtualKey( 0x28, 1 ) );
const KEY KEY_COMMA      = MAKE_KEY( ::MapVirtualKey( 0x33, 1 ) );
const KEY KEY_PERIOD     = MAKE_KEY( ::MapVirtualKey( 0x34, 1 ) );
const KEY KEY_SLASH      = MAKE_KEY( ::MapVirtualKey( 0x35, 1 ) );
const KEY KEY_BACKSLASH  = MAKE_KEY( ::MapVirtualKey( 0x2B, 1 ) );

bool FromChar( char c, KEY& key )
{
	SHORT vk = ::VkKeyScan( c );
	key = MAKE_KEY( LOBYTE( vk ) );
	return ( (LOBYTE( vk ) != -1) && (HIBYTE( vk ) != -1) );
}

KEY FromChar( char c )
{
	KEY k;
	return ( FromChar( c, k ) ? k : KEY_INVALID );
}

char ToChar( KEY key )
{
	return ( LOBYTE( ::MapVirtualKey( MakeInt( key ), 2 ) ) );
}

//-----------------------------------------------------------------------------

	struct StringToKeyEntry
	{
		const char* m_Name;
		KEY         m_Key;
	};

	bool operator < ( const char* l, const StringToKeyEntry& r )
		{  return ( ::stricmp( l, r.m_Name ) < 0 );  }
	bool operator < ( const StringToKeyEntry& l, const char* r )
		{  return ( ::stricmp( l.m_Name, r ) < 0 );  }
	bool operator < ( const StringToKeyEntry& l, const StringToKeyEntry& r )
		{  return ( ::stricmp( l.m_Name, r.m_Name ) < 0 );  }

	static StringToKeyEntry s_StringToKeyMap[] =
	{
		// $ note: each of these is actually prefixed with "key_" when converted
		//         to/from text. removed here for memory efficiency.

		// special - these will be filled in later
		{ "grave",               KEY_NONE                },
		{ "minus",               KEY_NONE                },
		{ "equals",              KEY_NONE                },
		{ "lbracket",            KEY_NONE                },
		{ "rbracket",            KEY_NONE                },
		{ "semicolon",           KEY_NONE                },
		{ "apostrophe",          KEY_NONE                },
		{ "comma",               KEY_NONE                },
		{ "period",              KEY_NONE                },
		{ "slash",               KEY_NONE                },
		{ "backslash",           KEY_NONE                },

		// standard keys
		{ "none",                KEY_NONE                },
		{ "lbutton",             KEY_LBUTTON             },
		{ "rbutton",             KEY_RBUTTON             },
		{ "break",               KEY_BREAK               },
		{ "mbutton",             KEY_MBUTTON             },
		{ "xbutton1",            KEY_XBUTTON1            },
		{ "xbutton2",            KEY_XBUTTON2            },
		{ "back",                KEY_BACK                },
		{ "tab",                 KEY_TAB                 },
		{ "clear",               KEY_CLEAR               },
		{ "return",              KEY_RETURN              },
		{ "shift",               KEY_SHIFT               },
		{ "control",             KEY_CONTROL             },
		{ "alt",                 KEY_ALT                 },
		{ "pause",               KEY_PAUSE               },
		{ "capslock",            KEY_CAPSLOCK            },
		{ "browser_back",        KEY_BROWSER_BACK        },
		{ "browser_forward",     KEY_BROWSER_FORWARD     },
		{ "browser_refresh",     KEY_BROWSER_REFRESH     },
		{ "browser_stop",        KEY_BROWSER_STOP        },
		{ "browser_search",      KEY_BROWSER_SEARCH      },
		{ "browser_favorites",   KEY_BROWSER_FAVORITES   },
		{ "browser_home",        KEY_BROWSER_HOME        },
		{ "volume_mute",         KEY_VOLUME_MUTE         },
		{ "volume_down",         KEY_VOLUME_DOWN         },
		{ "volume_up",           KEY_VOLUME_UP           },
		{ "media_next_track",    KEY_MEDIA_NEXT_TRACK    },
		{ "media_prev_track",    KEY_MEDIA_PREV_TRACK    },
		{ "media_stop",          KEY_MEDIA_STOP          },
		{ "media_play_pause",    KEY_MEDIA_PLAY_PAUSE    },
		{ "launch_mail",         KEY_LAUNCH_MAIL         },
		{ "launch_media_select", KEY_LAUNCH_MEDIA_SELECT },
		{ "launch_app1",         KEY_LAUNCH_APP1         },
		{ "launch_app2",         KEY_LAUNCH_APP2         },
		{ "kana",                KEY_KANA                },
		{ "junja",               KEY_JUNJA               },
		{ "final",               KEY_FINAL               },
		{ "hanja",               KEY_HANJA               },
		{ "kanji",               KEY_KANJI               },
		{ "escape",              KEY_ESCAPE              },
		{ "space",               KEY_SPACE               },
		{ "pageup",              KEY_PAGEUP              },
		{ "pagedown",            KEY_PAGEDOWN            },
		{ "end",                 KEY_END                 },
		{ "home",                KEY_HOME                },
		{ "left",                KEY_LEFT                },
		{ "up",                  KEY_UP                  },
		{ "right",               KEY_RIGHT               },
		{ "down",                KEY_DOWN                },
		{ "select",              KEY_SELECT              },
		{ "print",               KEY_PRINT               },
		{ "printscreen",         KEY_PRINTSCREEN         },
		{ "insert",              KEY_INSERT              },
		{ "delete",              KEY_DELETE              },
		{ "help",                KEY_HELP                },
		{ "0",                   KEY_0                   },
		{ "1",                   KEY_1                   },
		{ "2",                   KEY_2                   },
		{ "3",                   KEY_3                   },
		{ "4",                   KEY_4                   },
		{ "5",                   KEY_5                   },
		{ "6",                   KEY_6                   },
		{ "7",                   KEY_7                   },
		{ "8",                   KEY_8                   },
		{ "9",                   KEY_9                   },
		{ "a",                   KEY_A                   },
		{ "b",                   KEY_B                   },
		{ "c",                   KEY_C                   },
		{ "d",                   KEY_D                   },
		{ "e",                   KEY_E                   },
		{ "f",                   KEY_F                   },
		{ "g",                   KEY_G                   },
		{ "h",                   KEY_H                   },
		{ "i",                   KEY_I                   },
		{ "j",                   KEY_J                   },
		{ "k",                   KEY_K                   },
		{ "l",                   KEY_L                   },
		{ "m",                   KEY_M                   },
		{ "n",                   KEY_N                   },
		{ "o",                   KEY_O                   },
		{ "p",                   KEY_P                   },
		{ "q",                   KEY_Q                   },
		{ "r",                   KEY_R                   },
		{ "s",                   KEY_S                   },
		{ "t",                   KEY_T                   },
		{ "u",                   KEY_U                   },
		{ "v",                   KEY_V                   },
		{ "w",                   KEY_W                   },
		{ "x",                   KEY_X                   },
		{ "y",                   KEY_Y                   },
		{ "z",                   KEY_Z                   },
		{ "lwin",                KEY_LWIN                },
		{ "rwin",                KEY_RWIN                },
		{ "apps",                KEY_APPS                },
		{ "sleep",               KEY_SLEEP               },
		{ "numpad0",             KEY_NUMPAD0             },
		{ "numpad1",             KEY_NUMPAD1             },
		{ "numpad2",             KEY_NUMPAD2             },
		{ "numpad3",             KEY_NUMPAD3             },
		{ "numpad4",             KEY_NUMPAD4             },
		{ "numpad5",             KEY_NUMPAD5             },
		{ "numpad6",             KEY_NUMPAD6             },
		{ "numpad7",             KEY_NUMPAD7             },
		{ "numpad8",             KEY_NUMPAD8             },
		{ "numpad9",             KEY_NUMPAD9             },
		{ "multiply",            KEY_MULTIPLY            },
		{ "add",                 KEY_ADD                 },
		{ "separator",           KEY_SEPARATOR           },
		{ "subtract",            KEY_SUBTRACT            },
		{ "decimal",             KEY_DECIMAL             },
		{ "divide",              KEY_DIVIDE              },
		{ "f1",                  KEY_F1                  },
		{ "f2",                  KEY_F2                  },
		{ "f3",                  KEY_F3                  },
		{ "f4",                  KEY_F4                  },
		{ "f5",                  KEY_F5                  },
		{ "f6",                  KEY_F6                  },
		{ "f7",                  KEY_F7                  },
		{ "f8",                  KEY_F8                  },
		{ "f9",                  KEY_F9                  },
		{ "f10",                 KEY_F10                 },
		{ "f11",                 KEY_F11                 },
		{ "f12",                 KEY_F12                 },
		{ "f13",                 KEY_F13                 },
		{ "f14",                 KEY_F14                 },
		{ "f15",                 KEY_F15                 },
		{ "f16",                 KEY_F16                 },
		{ "f17",                 KEY_F17                 },
		{ "f18",                 KEY_F18                 },
		{ "f19",                 KEY_F19                 },
		{ "f20",                 KEY_F20                 },
		{ "f21",                 KEY_F21                 },
		{ "f22",                 KEY_F22                 },
		{ "f23",                 KEY_F23                 },
		{ "f24",                 KEY_F24                 },
		{ "numlock",             KEY_NUMLOCK             },
		{ "scrolllock",          KEY_SCROLLLOCK          },
	};

	bool FromString( const char* str, KEY& key )
	{
		// set this up once
		static bool s_Initialized = false;
		if( !s_Initialized )
		{
			s_Initialized = true;

			// patch in the latently initialized key values
			int index = 0;
			s_StringToKeyMap[ index++ ].m_Key = KEY_GRAVE;
			s_StringToKeyMap[ index++ ].m_Key = KEY_MINUS;
			s_StringToKeyMap[ index++ ].m_Key = KEY_EQUALS;
			s_StringToKeyMap[ index++ ].m_Key = KEY_LBRACKET;
			s_StringToKeyMap[ index++ ].m_Key = KEY_RBRACKET;
			s_StringToKeyMap[ index++ ].m_Key = KEY_SEMICOLON;
			s_StringToKeyMap[ index++ ].m_Key = KEY_APOSTROPHE;
			s_StringToKeyMap[ index++ ].m_Key = KEY_COMMA;
			s_StringToKeyMap[ index++ ].m_Key = KEY_PERIOD;
			s_StringToKeyMap[ index++ ].m_Key = KEY_SLASH;
			s_StringToKeyMap[ index++ ].m_Key = KEY_BACKSLASH;

			// sort 'em all
			std::sort( s_StringToKeyMap, ARRAY_END( s_StringToKeyMap ) );

			// allow localized builds to override keys with no vk equivalent via gas
			FastFuelHandle fh( "config:input_bindings:key_codes" );
			if ( fh )
			{
				FastFuelFindHandle finder( fh );
				if ( finder.FindFirstKeyAndValue() )
				{
					gpstring key;
					int code;

					while ( finder.GetNextKeyAndValue( key, code ) )
					{
						bool valid = false;

						// only accept key_ entries
						if ( ::strnicmp( key, "key_", 4 ) == 0 )
						{
							// find the entry in the map
							StringToKeyEntry* found = stdx::binary_search(
									s_StringToKeyMap, ARRAY_END( s_StringToKeyMap ), key.c_str() + 4 );
							if ( found != ARRAY_END( s_StringToKeyMap ) )
							{
								found->m_Key = MAKE_KEY( code );
								valid = true;
							}
						}

						if ( !valid )
						{
							gperrorf(( "FromString( KEY ): invalid key name '%s' found at '%s'\n",
									   key.c_str(), fh.GetAddress().c_str() ));
						}
					}
				}
			}

			// special key may have been overridden
			StringToKeyEntry* grave = stdx::binary_search(
					s_StringToKeyMap, ARRAY_END( s_StringToKeyMap ), "grave" );
			gpassert( grave != NULL );
			SetSpecialKey( grave->m_Key );
		}

		// empty is ok
		bool success = false;
		if ( *str == '\0' )
		{
			success = true;
			key = KEY_NONE;
		}
		else
		{
			// if starts with key_ search the map
			if ( ::strnicmp( str, "key_", 4 ) == 0 )
			{
				// search the array
				const StringToKeyEntry* found = stdx::binary_search(
						s_StringToKeyMap, ARRAY_END( s_StringToKeyMap ), str + 4 );
				if ( found != ARRAY_END( s_StringToKeyMap ) )
				{
					key = found->m_Key;
					success = true;
				}
				else if ( ::stricmp( str + 4, "special" ) == 0 )
				{
					key = KEY_SPECIAL;
					success = true;
				}
			}
			// try it as a literal
			else if ( str[ 1 ] == '\0' )
			{
				success = FromChar( str[ 0 ], key );
			}
			// try it as hex
			else if ( (str[ 0 ] == '0') && ((str[ 1 ] == 'x') || (str[ 1 ] == 'X')) )
			{
				key = KEY_NONE;
				success = ::FromString( str, *(BYTE*)&key );
			}
		}

		// done
		return ( success );
	}

//-----------------------------------------------------------------------------

struct KeyToStringEntry
{
	KEY         m_Key;
	const char* m_Name;

	void operator = ( const StringToKeyEntry& obj )
		{  m_Key = obj.m_Key;  m_Name = obj.m_Name;  }
};

bool operator < ( KEY l, const KeyToStringEntry& r )
	{  return ( l < r.m_Key );  }
bool operator < ( const KeyToStringEntry& l, KEY r )
	{  return ( l.m_Key < r );  }
bool operator < ( const KeyToStringEntry& l, const KeyToStringEntry& r )
	{  return ( l.m_Key < r.m_Key );  }

static KeyToStringEntry s_KeyToStringMap[ ELEMENT_COUNT( s_StringToKeyMap ) ];

gpstring PrivateToString( KEY key, bool screen )
{
	// build and sort the array if it hasn't been sorted yet - do this once
	static bool s_Initialized = false;
	if( !s_Initialized )
	{
		s_Initialized = true;

		// ensure that our source is sorted and set up properly
		KEY dummy;
		FromString( "", dummy );

		// build the reverse map
		std::copy( s_StringToKeyMap, ARRAY_END( s_StringToKeyMap ), s_KeyToStringMap );
		std::sort( s_KeyToStringMap, ARRAY_END( s_KeyToStringMap ) );
	}

	gpstring str( "???" );

	// early out for special key
	if ( key == KEY_SPECIAL )
	{
		str = screen ? gptranslate( $MSG$ "Special" ) : "key_special";
	}
	else if ( screen && (key == KEY_PRINTSCREEN) )
	{
		str = gptranslate( $MSG$ "Print Screen" );
	}
	else if ( screen )
	{
		// need to figure out if it's extended or not. note that this is not
		// the best way to do this. many keys have the same VK code but
		// the 'extended' bit makes a difference (such as VK_RETURN). the
		// entire system really needs to be redone to get all the keys
		// working correctly.
		LONG extFlag = 0;
		switch ( (UINT)key )
		{
			case ( VK_PRIOR    ):
			case ( VK_NEXT     ):
			case ( VK_END      ):
			case ( VK_HOME     ):
			case ( VK_LEFT     ):
			case ( VK_UP       ):
			case ( VK_RIGHT    ):
			case ( VK_DOWN     ):
			case ( VK_INSERT   ):
			case ( VK_DELETE   ):
			case ( VK_DIVIDE   ):
			case ( VK_NUMLOCK  ):
			case ( VK_LWIN     ):
			case ( VK_RWIN     ):
			case ( VK_APPS     ):
			case ( VK_RCONTROL ):
			case ( VK_RMENU    ):
			case ( VK_CANCEL   ):
			{
				extFlag = 1 << 24;
			}
			break;

			case ( KEY_PAUSE ):
			{
				// special handling for pause key, which is actually non-ext-numlock
				key = KEY_NUMLOCK;
			}
			break;
		}

		// now get the scan code
		UINT scanCode = ::MapVirtualKey( (UINT)key, 0 );
		if ( (scanCode == 0) && (key == KEY_DIVIDE) )
		{
			// HACK: fix for #5608 from kazuyuki
			scanCode = 0x35;
		}
		if ( scanCode != 0 )
		{
			// convert to text (here we use the extended-key flag)
			char buffer[ 50 ];
			if ( ::GetKeyNameText( (scanCode << 16) | extFlag, buffer, ELEMENT_COUNT( buffer ) ) != 0 )
			{
				str = buffer;
			}
			else
			{
				// Brazilian Portuguese fails GetKeyNameText() on specific key.
				if (   (LOWORD( ::GetKeyboardLayout( 0 ) ) == MAKELANGID( LANG_PORTUGUESE, SUBLANG_PORTUGUESE_BRAZILIAN ))
					&& (key == MAKE_KEY( 0xDE )) )
				{
					str = "'";
				}
			}
		}
	}
	else
	{
		// search the array
		const KeyToStringEntry* found = stdx::binary_search(
				s_KeyToStringMap, ARRAY_END( s_KeyToStringMap ), key );
		if ( found != ARRAY_END( s_KeyToStringMap ) )
		{
			str = "key_";
			str += found->m_Name;
		}
		else
		{
			gpassert( MakeInt( key ) <= 0xFF );
			str.assignf( "0x%02X", key );
		}
	}

	// done
	return ( str );
}

gpstring ToString( KEY key )
{
	return ( PrivateToString( key, false ) );
}

gpwstring ToScreenString( KEY key )
{
	return ( ::ToUnicode( PrivateToString( key, true ) ) );
}

//-----------------------------------------------------------------------------

KEY gSpecialKey = KEY_GRAVE;
const KEY& KEY_SPECIAL = gSpecialKey;

void SetSpecialKey( KEY key )
{
	gSpecialKey = key;
}

//-----------------------------------------------------------------------------

} // namespace Keys

//-----------------------------------------------------------------------------
