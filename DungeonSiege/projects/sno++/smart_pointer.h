#pragma once
#ifndef SMART_POINTER_H
#define SMART_POINTER_H




/* ========================================================================
   Header File: smart_pointer.h
   Description: Declares smart_pointer
   ======================================================================== */

// $$$ ENDANGERED

/* ========================================================================
   Explicit Dependencies
   ======================================================================== */
   
   /* ************************************************************************
   Class: initialized_pointer
   Description: A pointer which initializes itself at construction time
   ************************************************************************ */
template <class object_type_init>
class initialized_pointer
{
public:
	// Types
	typedef object_type_init object_type;
	
	// Usage
	inline operator object_type *(void) const;
	inline object_type &operator*(void) const;
	inline object_type *operator->() const;

	// Existence
	inline initialized_pointer(void);
	inline explicit initialized_pointer(object_type *Pointer);
	inline initialized_pointer &operator=(object_type *Pointer);

private:
	object_type *Pointer;
};

/* ************************************************************************
   Class: smart_pointer
   Description: I found auto_ptr to be inadequate, so here is a replacement
   ************************************************************************ */
template <class object_type_init>
class smart_pointer
{
public:
	// Types
	typedef object_type_init object_type;
	enum deletion_type {UseRegularDelete = false, UseArrayDelete = true};

	// Information
	inline object_type *GetPointer(void) const;

	// Modification
	inline object_type *Release(void) const;
	inline void Delete(void);
	inline smart_pointer<object_type> TransferAndOwn(
		object_type *NewPointer, bool const DeletionType = UseRegularDelete);
	inline smart_pointer<object_type> TransferAndOwn(
		smart_pointer<object_type> &CopyFrom);
	inline void ExchangeWith(smart_pointer<object_type> &SwapWith);
	inline smart_pointer<object_type> &operator=(
		smart_pointer<object_type> const &Copy);
	
	// Usage
	inline operator bool(void) const;
	inline object_type &operator*(void) const;
	inline object_type *operator->() const;

	// Existence
	inline smart_pointer(void);
	inline explicit smart_pointer(
		object_type *Pointer, bool const DeletionType = UseRegularDelete);
	inline smart_pointer(smart_pointer const &Copy);
	inline ~smart_pointer(void);

private:
	inline bool IsOwner(void) const;
	inline bool IsArray(void) const;
	inline void SetOwner(bool const Owner) const;
	inline void SetArray(bool const Array);
	inline void SetOwnerArrayPointer(
		bool const Owner, bool const Array, object_type *Pointer);
	inline void DoDelete(void);

	mutable union
	{
		object_type *Pointer;
		mutable int unsigned PointerBits;
	};
};

/* ========================================================================
   Inline Functions: initialized_pointer
   ======================================================================== */
template <class object_type_init>
inline initialized_pointer<object_type_init>::
operator initialized_pointer<object_type_init>::object_type *(
	void) const
{
	return(Pointer);
}

template <class object_type_init>
inline initialized_pointer<object_type_init>::object_type &
initialized_pointer<object_type_init>::
operator*(void) const
{
	return(*Pointer);
}

template <class object_type_init>
inline initialized_pointer<object_type_init>::object_type *
initialized_pointer<object_type_init>::
operator->() const
{
	return(Pointer);
}

template <class object_type_init>
inline initialized_pointer<object_type_init>::
initialized_pointer(void)
		: Pointer(0)
{
}

template <class object_type_init>
inline initialized_pointer<object_type_init>::
initialized_pointer(object_type *PointerInit)
		: Pointer(PointerInit)
{
}

template <class object_type_init>
inline initialized_pointer<object_type_init> &
initialized_pointer<object_type_init>::
operator=(object_type *PointerInit)
{
	Pointer = PointerInit;
	return(*this);
}
					

/* ========================================================================
   Inline Functions: smart_pointer
   ======================================================================== */
int unsigned const SmartPointerOwnerMask = 1;
int unsigned const SmartPointerArrayMask = 2;
int unsigned const SmartPointerPointerMask = (int unsigned)~3;

template <class object_type_init>
inline smart_pointer<object_type_init>::object_type *
smart_pointer<object_type_init>::
GetPointer(void) const
{
	return((object_type *)(PointerBits & SmartPointerPointerMask));
}

template <class object_type_init>
inline smart_pointer<object_type_init>::object_type *
smart_pointer<object_type_init>::
Release(void) const
{
	SetOwner(false);
	return(GetPointer());
}

template <class object_type_init>
inline void smart_pointer<object_type_init>::
Delete(void)
{
	DoDelete();
	Pointer = 0;
}

template <class object_type_init>
inline smart_pointer<object_type_init>
smart_pointer<object_type_init>::
TransferAndOwn(object_type *NewPointer, bool const Array)
{
	smart_pointer<object_type> TransferPointer(*this);
	SetOwnerArrayPointer(true, Array, NewPointer);
	return(TransferPointer);
}

template <class object_type_init>
inline smart_pointer<object_type_init>
smart_pointer<object_type_init>::
TransferAndOwn(smart_pointer<object_type> &CopyFrom)
{
	smart_pointer<object_type> TransferPointer;
	if(this != &CopyFrom)
	{
		TransferPointer = *this;
		Pointer = CopyFrom.Pointer;
		CopyFrom.SetOwner(false);		
	}

	return(TransferPointer);
}

template <class object_type_init>
inline void smart_pointer<object_type_init>::
ExchangeWith(smart_pointer<object_type> &SwapWith)
{
	if(this != &SwapWith)
	{
		object_type *SwapPointer = SwapWith.Pointer;
		SwapWith.Pointer = Pointer;
		Pointer = SwapPointer;
	}
}

template <class object_type_init>
inline smart_pointer<object_type_init> &smart_pointer<object_type_init>::
operator=(smart_pointer<object_type> const &Copy)
{
	if(this != &Copy)
	{
		DoDelete();
		Pointer = Copy.Pointer;
		Copy.SetOwner(false);
	}

	return(*this);
}
#if 0
// TODO: This used to be a member, but MSVC ICE'd too often
template <class object_type_init>
inline smart_pointer<object_type_init>::
operator object_type_init *(void) const
{
	return(GetPointer());
}
#endif

template <class object_type_init>
inline smart_pointer<object_type_init>::
operator bool(void) const
{
	return(GetPointer() != 0);
}

template <class object_type_init>
inline smart_pointer<object_type_init>::object_type &
smart_pointer<object_type_init>::
operator*(void) const
{
	return(*GetPointer());
}

template <class object_type_init>
inline smart_pointer<object_type_init>::object_type *
smart_pointer<object_type_init>::
operator->() const
{
	return(GetPointer());
}

template <class object_type_init>
inline smart_pointer<object_type_init>::
smart_pointer(void)
		: Pointer(0)
{
}

template <class object_type_init>
inline smart_pointer<object_type_init>::
smart_pointer(object_type *Pointer,
			  bool const Array)
{
	SetOwnerArrayPointer(true, Array, Pointer);
}

template <class object_type_init>
inline smart_pointer<object_type_init>::
smart_pointer(smart_pointer const &Copy)
{
	Pointer = Copy.Pointer;
	Copy.SetOwner(false);
}

template <class object_type_init>
inline smart_pointer<object_type_init>::
~smart_pointer(void)
{
	DoDelete();
}
	
template <class object_type_init>
inline bool smart_pointer<object_type_init>::
IsOwner(void) const
{
	return((PointerBits & SmartPointerOwnerMask) == SmartPointerOwnerMask);
}

template <class object_type_init>
inline bool smart_pointer<object_type_init>::
IsArray(void) const
{
	return((PointerBits & SmartPointerArrayMask) == SmartPointerArrayMask);
}

template <class object_type_init>
inline void smart_pointer<object_type_init>::
SetOwner(bool const Owner) const
{
	if(Owner)
	{
		PointerBits |= SmartPointerOwnerMask;
	}
	else
	{
		PointerBits &= ~SmartPointerOwnerMask;
	}
}

template <class object_type_init>
inline void smart_pointer<object_type_init>::
SetArray(bool const Array)
{
	if(Array)
	{
		PointerBits |= SmartPointerArrayMask;
	}
	else
	{
		PointerBits &= ~SmartPointerArrayMask;
	}
}

template <class object_type_init>
inline void smart_pointer<object_type_init>::
SetOwnerArrayPointer(bool const Owner, bool const Array,
					 object_type *NewPointer)
{
	Pointer = NewPointer;

	if(Owner)
	{
		PointerBits |= SmartPointerOwnerMask;
	}

	if(Array)
	{
		PointerBits |= SmartPointerArrayMask;
	}
}

template <class object_type_init>
inline void smart_pointer<object_type_init>::
DoDelete(void)
{
	if(IsOwner())
	{
		if(IsArray())
		{
			PointerBits &= SmartPointerPointerMask;
			delete [] Pointer;
		}
		else
		{
			PointerBits &= SmartPointerPointerMask;
			delete Pointer;
		}
	}
}

template <class object_type>
inline bool operator==(smart_pointer<object_type> const &PointerA,
					   smart_pointer<object_type> const &PointerB)
{
	return(PointerA.GetPointer() == PointerB.GetPointer());
}

template <class object_type>
inline bool operator==(smart_pointer<object_type> const &PointerA,
					   object_type const *PointerB)
{
	return(PointerA.GetPointer() == PointerB);
}


template <class object_type>
inline bool operator==(object_type const *PointerA,
					   smart_pointer<object_type> const &PointerB)
{
	return(PointerA == PointerB.GetPointer());
}




#endif
