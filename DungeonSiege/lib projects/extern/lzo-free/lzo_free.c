/*---------------------------------------------------------------------*

Project:  portable function
File:     lzo_free.c

Copyright 2001 Nintendo.  All rights reserved.

These coded instructions, statements and computer programs contain
proprietary information of Nintendo of America Inc. and/or Nintendo
Company Ltd., and are protected by Federal copyright law.  They may
not be disclosed to third parties or copied or duplicated in any form,
in whole or in part, without the prior written consent of Nintendo.

Author:  Steve Rabin
Created: March 14th, 2001

-----------------------------------------------------------------------*/

#include "lzo_free.h"
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/*
	This code will decompress LZO1X_999 compressed files, and was written from
	scratch for that purpose. This code is completely free for any commercial 
	purpose. It replaces the function lzo1x_decompress in the LZO library.


	WARNING!!!
	If PC_OPTIMIZATION is defined, the out buffer that gets passed in MUST 
	be 4 bytes longer than the final decompressed data. This is due to some 
	optimizations that overwrite the end of the buffer by up to 4 bytes. Only define
	PC_OPTIMIZATION if you will ultimately use this code in a PC application and
	not if you are using it on a PC to test compression for another platform. If
	you are developing games for the NINTENDO GAMECUBE, do not define this flag.


	Differences from lzo1x_decompress in the LZO 1.07 library:
	* Free for commercial applications
	* Some instruction decoupling to speed up superscalar/pipelined machines
	* Same speed as lzo1x_decompress (0-15% faster on the PC with PC_OPTIMIZATION defined)
	* Lazy incrementing of indicies
	* Fewer gotos, only 1 central while loop
	* No pointer arithmetic/assignment (instead array access/assignment)
	* No 4 byte (unsigned int) copy optimizations
	* Not sensitive to endianess (i.e. will work if files were compressed on a PC and decompressed on a MAC/GCN)
	* Instrumented with extra bounds checking code when DECOMPRESSION_BOUNDS_CHECKING is defined.


	Lingo:
	* literals are runs of characters that are copied 1 for 1 from the compressed file (uncompressed chunks)
	* post literals are literals that come after a match (max 3 literals - see the diagrams)
	* match refers to a previous portion in the decompressed data that should be copied to the current position
	* match length is the length of the match to be copied
	* match position is the position of the match to be copied in the decompressed data

	If you find errors/bugs in this code, please e-mail support@noa.com.

*/

	///////////////////////////////////////////////////////////////////////////////////////////////////
	//Interpretation of LZO Codes

	//Literal run (first byte in file >= 18)
	//
	//     literal run of (first byte) - 17
	//
	//
	//If first byte in file is > 20 && < 33, then next byte is a match using:
	//
	//    (first byte)                             (second byte)
	// 0 0 0 0    0 0 0 0                        0 0 0 0    0 0 0 0
	// | | | |    | |  \ \                       high bits of match distance(shift << 2) + 0x0800 
	// | | | |    | |   \ \
	// | | | |    | |    post literal copy (0,1,2, or 3)
	// | | | |    | |
	// | | | low bits of match distance (backward from last written byte)
	// | | | 
	// ignore (constant match length of 3)

	//M1 match (first byte < 16)
	//
	//    (first byte)                             (second byte)
	// 0 0 0 0    0 0 0 0                        0 0 0 0    0 0 0 0
	// | | | |    | |  \ \                       high bits of match distance (shift << 2)
	// | | | |    | |   \ \
	// | | | |    | |    post literal copy (0,1,2, or 3)
	// | | | |    | |
	// | | | |    low bits of match distance (backward from last written byte)
	// | | | |
	// ignore (constant match length of 2)

	//M2 match (first byte > 63)
	//
	//    (first byte)                             (second byte)
	// 0 0 0 0    0 0 0 0                        0 0 0 0    0 0 0 0
	// | | | |    | |  \ \                       high bits of match distance (shift << 3)
	// | | | |    | |   \ \
	// | | | |    | |    post literal copy (0,1,2, or 3)
	// | | | |    | |
	// | | | low bits of match distance (backward from last written byte)
	// | | |
	// match length + 1

	//M3 match (first byte < 64 && first byte > 31)
	//
	//                    |--------optional---------|
	//                     (extra N
	//    (first byte)       zero bytes)  (N+1 byte)       (N+2 byte)                       (N+3 byte)
	// 0 0 0 0    0 0 0 0    0000 0000     0000 0000    0 0 0 0   0 0 0 0                0 0 0 0   0 0 0 0
	// | | | |    | | | |   each equal to  add value    | | | |  / /   \ \                \ \ \ \   \ \ \ \
	// | | | |    | | | |   255 towards    to match    low bits match   post literal       high bits match distance (shift << 6)
	// | | | |    | | | |   match length   length      distance         copy (0,1,2, or 3)
	// | | | |    | | | |
	// | | | match length + 2 (if 0, length = 31 + 255*(number of following bytes that equal zero) )
	// | | |
	// ignore

	//M4 match (first byte < 32 && first byte > 15)
	//
	//                    |--------optional---------|
	//                     (extra N
	//    (first byte)       ZERO bytes)  (N+1 byte)       (N+2 byte)                       (N+3 byte)
	// 0 0 0 0    0 0 0 0    0000 0000     0000 0000    0 0 0 0   0 0 0 0                0 0 0 0   0 0 0 0
	// | | | |    | | | |   each equal to  add value    | | | |  / /   \ \                \ \ \ \   \ \ \ \
	// | | | |    | | | |   255 towards    to match    low bits match   post literal       high bits match distance (shift << 6)
	// | | | |    | | | |   match length   length      distance         copy (0,1,2, or 3)
	// | | | |    | | | |
	// | | | |    | match length + 2 (if 0, length = 7 + 255*(number of following bytes that equal zero) )
	// | | | |    |
	// ignore     highest bit of match distance + 0x4000

	//Literal run (first byte < 16 and not following a post literal)
	//
	//                    |--------optional---------|
	//                     (extra N
	//    (first byte)       ZERO bytes)  (N+1 byte)
	// 0 0 0 0    0 0 0 0    0000 0000     0000 0000
	// | | | |    | | | |   each equal to  add value
	// | | | |    | | | |   255 towards    to literal
	// | | | |    | | | |   literal run    run length
	// | | | |    | | | |
	// | | | literal run length + 3 (if 0, length = 15 + 255*(number of following bytes that equal zero) )
	// | | |     
	// ignore
	//
	//Next byte definately a match code (if first byte < 16, then override M1 match with this)
	//
	//    (first byte)                             (second byte)
	// 0 0 0 0    0 0 0 0                        0 0 0 0    0 0 0 0
	// | | | |    | |  \ \                       high bits of match distance(shift << 2) + 0x0800 
	// | | | |    | |   \ \
	// | | | |    | |    post literal copy (0,1,2, or 3)
	// | | | |    | |
	// | | | low bits of match distance (backward from last written byte)
	// | | | 
	// ignore (constant match length of 3)


#ifndef _DEBUG
#define PC_OPTIMIZATION		//WARNING! DON'T DEFINE UNLESS YOU'VE READ THE WARNING ABOVE
#endif

#if !GP_RETAIL
#define DECOMPRESSION_BOUNDS_CHECKING	//Only define for compressor app, not for GCN.
#endif


#define M2_MAX_OFFSET 0x0800
int FreeDecompress_LZO1X_999( const unsigned char * in, const unsigned int in_length, unsigned char* out, unsigned int* out_length )
{
	register int current_code = in[0];
	register int match_pos = 0;
	register int source_pos = -1;
	register int write_pos = -1;
	int final_pos = (int)in_length-1;

#	ifdef DECOMPRESSION_BOUNDS_CHECKING
	assert( *out_length > 0 && "FreeDecompressLZO1X_999: out_length must be a non-zero value equal to the length of the final output (for bounds checking only)." );
#	endif //DECOMPRESSION_BOUNDS_CHECKING

	//First byte
	if( current_code >= 18 )
	{	//Run of literals - copy (current_code - 17) of them
		int altered_code = current_code - 17;
		current_code = altered_code;
		++source_pos;

#		ifdef PC_OPTIMIZATION
		//Optimized code for copying run of literals (for superscalar/pipelined machines)
		if( altered_code > 3 ) {
			do {
				out[write_pos+1] = in[source_pos+1];
				out[write_pos+2] = in[source_pos+2];
				out[write_pos+3] = in[source_pos+3];
				out[write_pos+4] = in[source_pos+4];
				altered_code -= 4;
				write_pos += 4;
				source_pos += 4;
			} while( altered_code > 3 );
			if( altered_code > 0 )
			{	//Will overread and overwrite by up to 2 bytes
				out[write_pos+1] = in[source_pos+1];
				out[write_pos+2] = in[source_pos+2];
				out[write_pos+3] = in[source_pos+3];
			}
		}
		else
		{	//Will overread and overwrite by up to 2 bytes
			out[write_pos+1] = in[source_pos+1];
			out[write_pos+2] = in[source_pos+2];
			out[write_pos+3] = in[source_pos+3];
		}
		write_pos += altered_code;
		source_pos += altered_code;
#		else
		//Typical code for copying run of literals
		out[++write_pos] = in[++source_pos];
		while( --altered_code > 0 ) {
			out[++write_pos] = in[++source_pos];
		}
#		endif	//PC_OPTIMIZATION


		if( current_code > 3 )
		{	//Get next code, check for matches
			current_code = in[++source_pos];
			if( current_code < 16 )
			{	//Match of 3 bytes
				match_pos = write_pos;
				match_pos -= M2_MAX_OFFSET;
				match_pos -= current_code >> 2;
				match_pos -= in[++source_pos] << 2;	//use the next code to augment match_pos
				//Copy 3 bytes

#				ifdef DECOMPRESSION_BOUNDS_CHECKING
				if( write_pos < -1 || match_pos < 0 ||
					write_pos + 3 >= (int)*out_length ||
					match_pos + 2 >= (int)*out_length )
				{	//Will read or write outside bounds
					return(-1);
				}
#				endif //DECOMPRESSION_BOUNDS_CHECKING

				out[write_pos+1] = out[match_pos];
				out[write_pos+2] = out[match_pos+1];
				out[write_pos+3] = out[match_pos+2];
				write_pos += 3;
				//Copy post literals
				goto copy_post_literals;
			}
			else {
				if( source_pos < final_pos )
				{	//decode match
					goto decode_match;
				}
			}
		}
		else
		{	//Get next code and decode match
			current_code = in[++source_pos];
			goto decode_match;
		}
	}

	//Main while loop
	while( source_pos < final_pos )
	{
		//When the last code has no post literals, the following rules apply
		current_code = in[++source_pos];
		if( current_code < 16 )
		{	//A literal run
			if( current_code == 0 )
			{	//Literal string will exceed 15
				while( in[++source_pos] == 0 )
				{	//Each zero symbolizes an extra 255 run of literals
					current_code += 255;

#					ifdef DECOMPRESSION_BOUNDS_CHECKING
					if( source_pos < -1 || source_pos + 1 >= (int)in_length )
					{	//Will read outside bounds
						return(-1);
					}
#					endif //DECOMPRESSION_BOUNDS_CHECKING
				}

#				ifdef DECOMPRESSION_BOUNDS_CHECKING
				if( source_pos < 0 || source_pos >= (int)in_length )
				{	//Will read outside bounds
					return(-1);
				}
#				endif //DECOMPRESSION_BOUNDS_CHECKING

				current_code += 15;
				current_code += in[source_pos];
			}
			//Copy literals (current_code + 3)

#			ifdef DECOMPRESSION_BOUNDS_CHECKING
			if( source_pos < -1 || write_pos < -1 ||
				source_pos + current_code + 3 >= (int)in_length ||
				write_pos + current_code + 3 >= (int)*out_length )
			{	//Will read or write outside bounds
				return(-1);
			}
#			endif //DECOMPRESSION_BOUNDS_CHECKING


#			ifdef PC_OPTIMIZATION
			//Optimized code to copy literals (for superscalar/pipelined machines)
			out[write_pos+1] = in[source_pos+1];
			out[write_pos+2] = in[source_pos+2];
			out[write_pos+3] = in[source_pos+3];
			write_pos += 3;
			source_pos += 3;
			if( current_code > 3 ) {
				do {
					out[write_pos+1] = in[source_pos+1];
					out[write_pos+2] = in[source_pos+2];
					out[write_pos+3] = in[source_pos+3];
					out[write_pos+4] = in[source_pos+4];
					current_code -= 4;
					write_pos += 4;
					source_pos += 4;
				} while( current_code > 3 );
				if( current_code > 0 )
				{	//Will overread and overwrite by up to 2 bytes
					out[write_pos+1] = in[source_pos+1];
					out[write_pos+2] = in[source_pos+2];
					out[write_pos+3] = in[source_pos+3];
				}
			}
			else
			{	//Will overread and overwrite by up to 2 bytes
				out[write_pos+1] = in[source_pos+1];
				out[write_pos+2] = in[source_pos+2];
				out[write_pos+3] = in[source_pos+3];
			}
			source_pos += current_code;
			write_pos += current_code;
#			else
			//Typical code to copy literals
			out[++write_pos] = in[++source_pos];
			out[++write_pos] = in[++source_pos];
			out[++write_pos] = in[++source_pos];
			out[++write_pos] = in[++source_pos];
			while( --current_code > 0 ) {
				out[++write_pos] = in[++source_pos];
			}
#			endif	//PC_OPTIMIZATION

			//Get next code, check for match
			current_code = in[++source_pos];
			if( current_code < 16 )
			{	//Match of 3 bytes
				++source_pos;
				match_pos = write_pos;
				match_pos -= M2_MAX_OFFSET;
				match_pos -= current_code >> 2;
				match_pos -= in[source_pos] << 2;	//use the next code to augment match_pos
				//Copy 3 bytes

#				ifdef DECOMPRESSION_BOUNDS_CHECKING
				if( write_pos < 1 || match_pos < 0 ||
					match_pos + 2 >= (int)*out_length ||
					write_pos + 3 >= (int)*out_length )
				{	//Will read or write outside bounds
					return(-1);
				}
#				endif //DECOMPRESSION_BOUNDS_CHECKING

				out[write_pos+1] = out[match_pos];
				out[write_pos+2] = out[match_pos+1];
				out[write_pos+3] = out[match_pos+2];
				write_pos += 3;
				//Copy post literals
				goto copy_post_literals;
			}
		}

decode_match:
		//General matching
		if( current_code > 63 )
		{	//M2 match
			++source_pos;
			match_pos = write_pos;
			match_pos -= (current_code >> 2) & 7;
			match_pos -= in[source_pos] << 3;	//use the next code to augment match_pos

			current_code >>= 5;
			current_code -= 1;
		}
		else if( current_code > 31 )
		{	//M3 match
			current_code &= 31;
			if( current_code == 0 )
			{	//Copy length will exceed 31
				while( in[++source_pos] == 0 )
				{	//Each zero symbolizes an extra 255 run of literals
					current_code += 255;

#					ifdef DECOMPRESSION_BOUNDS_CHECKING
					if( source_pos < -1 || source_pos + 1 >= (int)in_length )
					{	//Will read outside bounds
						return(-1);
					}
#					endif //DECOMPRESSION_BOUNDS_CHECKING
				}

#				ifdef DECOMPRESSION_BOUNDS_CHECKING
				if( source_pos < 0 || source_pos >= (int)in_length )
				{	//Will read outside bounds
					return(-1);
				}
#				endif //DECOMPRESSION_BOUNDS_CHECKING

				current_code += 31;
				current_code += in[source_pos];
			}

#			ifdef DECOMPRESSION_BOUNDS_CHECKING
			if( source_pos < -1 || source_pos + 2 >= (int)in_length )
			{	//Will read outside bounds
				return(-1);
			}
#			endif //DECOMPRESSION_BOUNDS_CHECKING

			match_pos = write_pos;
			match_pos -= in[source_pos+1] >> 2;
			match_pos -= in[source_pos+2] << 6;
			source_pos += 2;
		}
		else if( current_code > 15 )
		{	//M4 match
			match_pos = write_pos;
			match_pos += 1;
			match_pos -= (current_code & 8) << 11;

			current_code &= 7;
			if( current_code == 0 )
			{	//Copy length will exceed 7
				while( in[++source_pos] == 0 )
				{	//Each zero symbolizes an extra 255 run of literals
					current_code += 255;

#					ifdef DECOMPRESSION_BOUNDS_CHECKING
					if( source_pos < -1 || source_pos + 1 >= (int)in_length )
					{	//Will read outside bounds
						return(-1);
					}
#					endif //DECOMPRESSION_BOUNDS_CHECKING

				}

#				ifdef DECOMPRESSION_BOUNDS_CHECKING
				if( source_pos < 0 || source_pos >= (int)in_length )
				{	//Will read outside bounds
					return(-1);
				}
#				endif //DECOMPRESSION_BOUNDS_CHECKING

				current_code += 7;
				current_code += in[source_pos];
			}
			
#			ifdef DECOMPRESSION_BOUNDS_CHECKING
			if( source_pos < -1 || source_pos + 2 >= (int)in_length )
			{	//Will read outside bounds
				return(-1);
			}
#			endif //DECOMPRESSION_BOUNDS_CHECKING

			match_pos -= in[source_pos+1] >> 2;
			match_pos -= in[source_pos+2] << 6;
			source_pos += 2;

			if( match_pos == write_pos + 1 )
			{	//End of file
				break;
			}

			match_pos -= 0x4000;
		}
		else
		{	//M1 match
			++source_pos;
			match_pos = write_pos;
			match_pos -= current_code >> 2;
			match_pos -= in[source_pos] << 2;	//use the next code to augment match_pos

			//Copy 2 bytes from match_pos

#			ifdef DECOMPRESSION_BOUNDS_CHECKING
			if( write_pos < -1 || match_pos < 0 ||
				match_pos + 1 >= (int)*out_length ||
				write_pos + 2 >= (int)*out_length )
			{	//Will read or write outside bounds
				return(-1);
			}
#			endif //DECOMPRESSION_BOUNDS_CHECKING

			out[write_pos+1] = out[match_pos];
			out[write_pos+2] = out[match_pos+1];
			write_pos += 2;

			//Copy post literals
			goto copy_post_literals;
		}
		
		//Now copy match using match_pos and current_code as length (+2)

#		ifdef DECOMPRESSION_BOUNDS_CHECKING
		if( write_pos < -1 || match_pos < 0 ||
			match_pos + current_code + 2 >= (int)*out_length ||
			write_pos + current_code + 3 >= (int)*out_length )
		{	//Will read or write outside bounds
			return(-1);
		}
#		endif //DECOMPRESSION_BOUNDS_CHECKING


#		ifdef PC_OPTIMIZATION
		//Optimized code for copying matches (not faster for car data)
		//(faster for long matches and superscalar/pipelined machines)
		out[write_pos+1] = out[match_pos];
		out[write_pos+2] = out[match_pos+1];
		write_pos += 2;
		match_pos += 2;
		if( current_code > 3 ) {
			do {
				out[write_pos+1] = out[match_pos];
				out[write_pos+2] = out[match_pos+1];
				out[write_pos+3] = out[match_pos+2];
				out[write_pos+4] = out[match_pos+3];
				current_code -= 4;
				write_pos += 4;
				match_pos += 4;
			} while( current_code > 3 );
			if( current_code > 0 )
			{	 //Will overread and overwrite by up to 2 bytes
				out[write_pos+1] = out[match_pos];
				out[write_pos+2] = out[match_pos+1];
				out[write_pos+3] = out[match_pos+2];
			}
		}
		else
		{	 //Will overread and overwrite by up to 2 bytes
			out[write_pos+1] = out[match_pos];
			out[write_pos+2] = out[match_pos+1];
			out[write_pos+3] = out[match_pos+2];
		}
		write_pos += current_code;
		match_pos += current_code;
#		else
		//Typical code for copying matches
		out[++write_pos] = out[match_pos++];
		out[++write_pos] = out[match_pos++];
		out[++write_pos] = out[match_pos++];
		while( --current_code > 0 ) {
			out[++write_pos] = out[match_pos++];
		}
#		endif //PC_OPTIMIZATION


		//Check for post literals
		current_code = in[source_pos-1];	//Get original code

copy_post_literals:
		current_code &= 3;
		if( current_code > 0 )
		{	//Post literals

#			ifdef DECOMPRESSION_BOUNDS_CHECKING
			if( source_pos < -1 || write_pos < -1 ||
				source_pos + current_code >= (int)in_length ||
				write_pos + current_code >= (int)*out_length )
			{	//Will read or write outside bounds
				return(-1);
			}
#			endif //DECOMPRESSION_BOUNDS_CHECKING

#			ifdef PC_OPTIMIZATION
			//Optimized code for literal copy (for superscalar/pipelined machines)
			//Will overread and overwrite by up to 2 bytes
			out[write_pos+1] = in[source_pos+1];
			out[write_pos+2] = in[source_pos+2];
			out[write_pos+3] = in[source_pos+3];
			write_pos += current_code;
			source_pos += current_code;
#			else
			//Typical code for literal copy
			out[++write_pos] = in[++source_pos];
			while( --current_code > 0 ) {
				out[++write_pos] = in[++source_pos];
			}
#			endif	//PC_OPTIMIZATION

			//Get next code and decode match
			current_code = in[++source_pos];
			goto decode_match;
		}
	}

	*out_length = write_pos + (unsigned int)1;

	
	if( current_code != 1 || source_pos != final_pos ) {
		return(-1);
	}
	else {
		return(0);
	}
}

#ifdef __cplusplus
}
#endif //__cplusplus
