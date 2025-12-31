#include "precomp_gpcore.h"
#include "progressstack.h"




ProgressStack::ProgressStack()
{
	ProgressSample::Init( this );
	m_Callback = makeFunctor( *this, &ProgressStack::DummyProgressStackCallback );
}




ProgressStack::~ProgressStack()
{
	m_Callback();

	m_Callback = makeFunctor( *this, &ProgressStack::DummyProgressStackCallback );

	ProgressSample::Init( 0 );
}




void ProgressStack::RegisterProgressMadeCallback( ProgressStackCallback & Callback )
{
	m_Callback = Callback;
}





void ProgressStack::DummyProgressStackCallback()
{
}




void ProgressStack::RegisterSample( ProgressSample * pSample )
{
	m_ProgressList.push_back( pSample );
}



void ProgressStack::UnRegisterSample( ProgressSample * pSample )
{
	gpassert( !m_ProgressList.empty() );
	gpassert( pSample == m_ProgressList.back() );
	m_ProgressList.pop_back();
}




void ProgressStack::ProgressMade()
{
	m_Callback();
}




//------------------------------------------------------------------------




ProgressSample::ProgressSample( gpstring const & sName, unsigned int ProgressMax )
{
	m_MadeInt	= 0;
	m_MadeFloat = 0.0;

	m_sName = sName;
	m_MaxInt = ProgressMax;
	
	m_pProgressStack->RegisterSample( this );
}




ProgressSample::ProgressSample( gpstring const & sName, float ProgressMax )
{
	m_sName = sName;
	m_MaxFloat = ProgressMax;

	m_pProgressStack->RegisterSample( this );
}
	



ProgressSample::~ProgressSample()
{
	m_pProgressStack->UnRegisterSample( this );
}



void ProgressSample::Make( unsigned int Integer )
{
	gpassert( m_MaxInt > 0 );
	m_MadeInt += Integer;
	m_MadeInt = min( m_MadeInt, m_MaxInt );
	
	if( m_pProgressStack ) {
		m_pProgressStack->ProgressMade();
	}
}




void ProgressSample::Make( float Fraction )
{
	gpassert( m_MaxFloat > 0.0 );
	m_MadeFloat += Fraction;
	m_MadeFloat = min( m_MadeFloat, m_MaxFloat );

	if( m_pProgressStack ) {
		m_pProgressStack->ProgressMade();
	}
}




float ProgressSample::GetMadeProgress()
{
	if( m_MaxInt > 0 ) {
		return( float(m_MadeInt)/float(m_MaxInt) );
	}
	else if( m_MaxFloat > 0.0 ) {
		return( m_MadeFloat/m_MaxFloat );
	}
	else {
		gpassert( 0 );
		return( 0.0 );
	}
}




void ProgressSample::Init( ProgressStack * pProgressStack )
{
	m_pProgressStack = pProgressStack;
}
