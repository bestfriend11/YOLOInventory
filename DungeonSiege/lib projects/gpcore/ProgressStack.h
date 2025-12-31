#pragma once
/*
  ============================================================================
  ----------------------------------------------------------------------------

	File		: ProgressStack.h

	Author(s)	: Bartosz Kijanka

	Purpose		: This file contains a couple of classes you can use to
				  enable accurate showing of progress.  This is intended for
				  non real-time situations like loading level data, processing
				  data in an editor etc.  See below for usage.

	Performance	: Don't worry.  Be happy.  Cheap as Borsch, but not free, so
				  don't use this in speed-critical inner loops.  Intended mainly
				  for lots of IO or content calculation iterations.

	(C)opyright 2000 Gas Powered Games, Inc.

  ----------------------------------------------------------------------------
  ============================================================================
*/
#ifndef __PROGRESSSTACK_H
#define __PROGRESSSTACK_H




class ProgressStack;
class ProgressSample;




#include <list>
#include "BlindCallback.h"




typedef CBFunctor0 ProgressStackCallback;


/*===========================================================================

	Class		: ProgressStack

	Author(s)	: Bartosz Kijanka

	Purpose		: This is the registry class with which all ProgressSamples
				  register themselves.  Ask this class for the ProgressList, then
				  examine the first N elements to get their progress.  Elements
				  should be arranges in least-recently-changed to most-recently-changed.

	Threads		: One instance of this class is needed per thread.

---------------------------------------------------------------------------*/
class ProgressStack
{

public:

	typedef std::list<ProgressSample*> ProgressList;

	ProgressStack();
	~ProgressStack();

	// get a reference to all the progress samples, ordered Least Recently changed to Most Recently changed
	ProgressList const & GetProgressList()		{ return( m_ProgressList ); }

	// client can register here to be notified each time the progress stack changes, each time progress is made
	void RegisterProgressMadeCallback( ProgressStackCallback & Callback );

private:

	friend class ProgressSample;

	// on construction, the samples register here
	void RegisterSample( ProgressSample * pSample );

	// on destruction, the samples unregister here
	void UnRegisterSample( ProgressSample * pSample );

	// each time a sample makes progress, it calls here
	void ProgressMade();

	// by default, the callback goes here
	void DummyProgressStackCallback();


	//----- data


	// collection of all the current progress samples, from Least Recent to Most Recent progress
	ProgressList m_ProgressList;

	// callback to client to let it know progress stack has changed
	ProgressStackCallback m_Callback;
};




/*===========================================================================

	Class		: ProgressSample

	Author(s)	: Bartosz Kijanka

	Purpose		: Construct this class before some loop that takes a significant
				  amount of time to execute.  When you begin the loop, you usually
				  know the max amount of iterations that will be done.  Use
				  this to init the ProgressSample.  Then, with each iteration, call
				  ::Make() with the amount iterations that were made ( usually 1 ).

				  I.e.:

				  ProgressSample Progress( "Reading files.", FileList.Size() );
				  for( i = FileList.begin(); i != FileList.end(); ++i ) {
					... do stuff
					ProgressSample.Make( 1 );
				  }

				  Some times you can't just tick the counter one.  If the loop gauges
				  progress along some arbitrary analog range, like [0.0,200.0], you would
				  write:

				  ProgressSample Progress( "Doing work.", 200.0 );
				  while( WorkDone < 200.0 ) {
					WorkDoneThisPass = DoWork();
					WorkDone += WorkDoneThisPass;
					Progress.Make( WorkDoneThisPass );
				  }

---------------------------------------------------------------------------*/
class ProgressSample
{

public:

	// existence

	ProgressSample( gpstring const & sName, unsigned int ProgressMade );

	ProgressSample( gpstring const & sName, float ProgressMade );
	
	~ProgressSample();

	// register fractional amount of progress made towards the max
	void Make( unsigned int Discrete );

	void Make( float Discrete );

	// return [0,1.0] as to the amount of progress made
	float GetMadeProgress();


private:

	friend class ProgressStack;

	static void Init( ProgressStack * pProgressStack );

	//----- data

	gpstring m_sName;

	unsigned int m_MaxInt;
	unsigned int m_MadeInt;

	float m_MaxFloat;
	float m_MadeFloat;

	DECL_THREAD static ProgressStack * m_pProgressStack;
};

#pragma once




#endif