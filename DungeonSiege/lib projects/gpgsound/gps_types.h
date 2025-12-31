#pragma once
#ifndef _GPS_TYPES_H_
#define _GPS_TYPES_H_

/*******************************************************************************************
**
**									GPGSound types
**
**		Type information for the GPGSound library
**
**		Author:		James Loe
**		Date:		01/25/01
**
********************************************************************************************/


namespace GPGSound
{
	// Enumeration that describes different types of sounds
	enum eSampleType
	{
		SND_EFFECT_LOW			= 0,	// Low priority sound effect
		SND_EFFECT_NORM			= 1,	// Normal priority sound effect
		SND_EFFECT_HIGH			= 2,	// High priority sound effect
		SND_EFFECT_AMBIENT		= 3,	// Ambient sound effect
	};

	// Enumeration that describes different types of streams
	enum eStreamType
	{
		STRM_AMBIENT			= 0,	// Low priority ambient track
		STRM_VOICE				= 1,	// Normal priority dialogue track
		STRM_MUSIC				= 2,	// High priority music track
	};

	// Enumeration that describes different types of speakers
	enum eSpeakerType
	{
		SPK_STEREO				= 0,	// Stereo (2 speakers)
		SPK_HEADPHONE			= 1,	// Headphones
		SPK_SURROUND			= 2,	// Surround sound (3 speakers)
		SPK_4SPEAKER			= 3,	// 4 speaker surround setup
		SPK_SYSTEM				= 4,	// Use system speaker setting
	};

	// Enumeration that describes different room types
	enum eRoomType
	{
		ROOM_GENERIC			= 0,
		ROOM_PADDEDCELL			= 1,
		ROOM_ROOM				= 2,
		ROOM_BATHROOM			= 3,
		ROOM_LIVINGROOM			= 4,
		ROOM_STONEROOM			= 5,
		ROOM_AUDITORIUM			= 6,
		ROOM_CONCERTHALL		= 7,
		ROOM_CAVE				= 8,
		ROOM_ARENA				= 9,
		ROOM_HANGAR				= 10,
		ROOM_CARPETEDHALLWAY	= 11,
		ROOM_HALLWAY			= 12,
		ROOM_STONECORRIDOR		= 13,
		ROOM_ALLEY				= 14,
		ROOM_FOREST				= 15,
		ROOM_CITY				= 16,
		ROOM_MOUNTAINS			= 17,
		ROOM_QUARRY				= 18,
		ROOM_PLAIN				= 19,
		ROOM_PARKINGLOT			= 20,
		ROOM_SEWERPIPE			= 21,
		ROOM_UNDERWATER			= 22,
		ROOM_DRUGGED			= 23,
		ROOM_DIZZY				= 24,
		ROOM_PSYCHOTIC			= 25,
	};

	// Invalid sound identifier
	extern const DWORD INVALID_SOUND_ID;
	extern const DWORD INVALID_PROVIDER;
	extern const DWORD INVALID_FILE_INDEX;
	extern const DWORD INVALID_SED_INDEX;
	extern const DWORD SOUND_MIN_VOLUME;
	extern const DWORD SOUND_MAX_VOLUME;
	extern const float SOUND_TIMER;
};

#endif