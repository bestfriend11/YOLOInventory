#pragma once

#if !defined(CIRCULAR_BUFFER_H)
/* ========================================================================
   Header File: circular_buffer.h
   Description: Implements circular_buffer
   ======================================================================== */

/* ========================================================================
   Explicit Dependencies
   ======================================================================== */
#include "GpColl.h"

/* ************************************************************************
   Class: circular_buffer
   Description: A circular buffer of entries such that writing
                overwrites the least recently entered entry, and indices
				always refer to the last size() entries entered
   ************************************************************************ */
template <class entry>
class circular_buffer
{
public:
	// Types
	typedef size_t size_type;
	
	// Read-only access
	// TODO: iterators
	inline entry const &operator[](size_type const Index) const;
	
	// Write-only access
	// TODO: iterators
	inline size_type capacity(void) const;
	inline size_type size(void) const;
	inline bool empty(void) const { return( !size() ); }
	inline void push_back(entry const &Entry);

	// Read/Write access
	// TODO: iterators
	inline entry &operator[](size_type const Index);
	inline void clear(void);

	// Existence
	inline circular_buffer(size_type const InitialSize);

private:
	// Buffer
	bool BufferHasWrapped;
	int unsigned BaseIndex;
	typedef stdx::fast_vector<entry> buffer;
	buffer Buffer;

	// Disallow copying
	circular_buffer(circular_buffer const &);
	circular_buffer &operator=(circular_buffer const &);

};
	
/* ************************************************************************
   Function: operator[]
   Description: Accesses the Index^th entry
   ************************************************************************ */
template <class entry>
inline entry const &circular_buffer<entry>::
operator[](size_type const Index) const
{
	size_type ActualIndex = Index;
	
	// Shift into base index space if the buffer has wrapped
	if(BufferHasWrapped)
	{
		ActualIndex += BaseIndex;
	}

	// Clamp index to valid range
	ActualIndex = ActualIndex % size();

	return(Buffer[ActualIndex]);
}
	
template <class entry>
inline entry &circular_buffer<entry>::
operator[](size_type const Index)
{
	size_type ActualIndex = Index;
	
	// Shift into base index space if the buffer has wrapped
	if(BufferHasWrapped)
	{
		ActualIndex += BaseIndex;
	}

	// Clamp index to valid range
	ActualIndex = ActualIndex % size();

	return(Buffer[ActualIndex]);
}

/* ************************************************************************
   Function: capacity
   Description: Returns the number of items the buffer holds when full
   ************************************************************************ */
template <class entry>
inline circular_buffer<entry>::size_type
circular_buffer<entry>::
capacity(void) const
{
	return(Buffer.capacity());
}

/* ************************************************************************
   Function: size
   Description: Returns the number of items the buffer is currently holding
   ************************************************************************ */
template <class entry>
inline circular_buffer<entry>::size_type
circular_buffer<entry>::
size(void) const
{
	return(BufferHasWrapped ? Buffer.size() : BaseIndex);
}

/* ************************************************************************
   Function: push_back
   Description: Adds an entry to the end of the buffer, overwriting old
                data if the buffer is full
   ************************************************************************ */
template <class entry>
inline void circular_buffer<entry>::
push_back(entry const &Entry)
{
	// Store the entry
	Buffer[BaseIndex] = Entry;

	// Rotate the base of the buffer
	if(++BaseIndex >= capacity())
	{
		BaseIndex = 0;
		BufferHasWrapped = true;
	}
}

/* ************************************************************************
   Function: clear
   Description: Erases all entries in the buffer, but maintains the size
   ************************************************************************ */
template <class entry>
inline void circular_buffer<entry>::
clear(void)
{
	int unsigned const InitialSize = Buffer.capacity();
	Buffer.clear();
	BufferHasWrapped = false;
	BaseIndex = 0;
	Buffer.resize(InitialSize);
}

/* ************************************************************************
   Function: circular_buffer
   Description: Constructs the circular buffer with the given dimension
   ************************************************************************ */
template <class entry>
inline circular_buffer<entry>::
circular_buffer(size_type const InitialSize)
		: BufferHasWrapped(false),
		  BaseIndex(0),
		  Buffer(InitialSize)
{
}

#define CIRCULAR_BUFFER_H
#endif
