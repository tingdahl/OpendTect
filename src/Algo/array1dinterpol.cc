/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Satyaki Maitra
 * DATE     : December 2009
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "array1dinterpol.h"

#include "arrayndimpl.h"
#include "interpol1d.h"


Array1DInterpol::Array1DInterpol()
    : Executor( "Interpolator" )
    , arr_( 0 )
    , nrdone_( 0 )
    , maxgapsize_( mUdf(int) )
    , arrstarted_(false)
{
}


Array1DInterpol::~Array1DInterpol()
{
}


od_int64 Array1DInterpol::nrIterations() const
{ return arr_ ? arr_->info().getTotalSz() : 0; }


void Array1DInterpol::setMaxGapSize( float maxgapsize )
{ maxgapsize_ = (int)maxgapsize; }


float Array1DInterpol::getMaxGapSize() const
{ return (float)maxgapsize_; }


void Array1DInterpol::setArray( Array1D<float>& arr )
{ arr_ = &arr; }


LinearArray1DInterpol::LinearArray1DInterpol()
    : Array1DInterpol()
{
}


int LinearArray1DInterpol::nextStep()
{
    if ( arr_->info().getTotalSz() <= nrdone_ )
	return Finished();

    if ( (!arr_->info().validPos(nrdone_) || mIsUdf(arr_->get(nrdone_)))
	  && !arrstarted_ )
    {
	nrdone_++;
	return MoreToDo();
    }

    if ( !arr_->info().validPos(nrdone_) || !mIsUdf(arr_->get(nrdone_)) )
    {
	arrstarted_ = true;
	nrdone_++;
	return MoreToDo();
    }

    int startidx = nrdone_-1;
    int stopidx = nrdone_;
    while ( arr_->info().validPos(stopidx+1) )
    {
	stopidx++;
	if ( mIsUdf(arr_->get(stopidx)) )
	    continue;
	break;
    }

    if ( (stopidx-startidx)>maxgapsize_ )
    {
	nrdone_++;
	return MoreToDo();
    }

    float val0,val1;
    float pos0,pos1;
    if ( mIsUdf(arr_->get(stopidx)) )
	return Finished();

    val0 = arr_->get( startidx ); pos0 = (float)( startidx );
    val1 = arr_->get( stopidx ); pos1 = (float)stopidx;

    float val = Interpolate::linear1D( pos0, val0, pos1, val1, (float)nrdone_ );
    if ( !mIsUdf(val) )
	arr_->set( nrdone_, val );
    nrdone_++;

    return MoreToDo();
}


PolyArray1DInterpol::PolyArray1DInterpol()
    : Array1DInterpol()
{
}


bool PolyArray1DInterpol::getPositions( int curpos, TypeSet<float>& posidxs )
{
    bool fisrtundef =
	arr_->info().validPos(curpos-2) && mIsUdf(arr_->get(curpos-2));
    posidxs[0] = fisrtundef ? (float)(curpos-1) : (float)(curpos-2);
    if ( !fisrtundef )
	posidxs[1] = (float)(curpos-1);
    int iteratoridx = curpos;
    int posidx = fisrtundef ? 1 : 2;
    while ( arr_->info().validPos(iteratoridx+1) )
    {
	iteratoridx++;
	if ( mIsUdf(arr_->get(iteratoridx)) )
	    continue;

	if ( (iteratoridx-curpos-1)>maxgapsize_ )
	    return false;

	posidxs[posidx] = (float)iteratoridx;
	posidx++;
	if ( posidx>=posidxs.size() )
	    break;
    }

    return true;
}


int PolyArray1DInterpol::nextStep()
{
    if ( arr_->info().getTotalSz() <= nrdone_ )
	return Finished();

    if ( (!arr_->info().validPos(nrdone_) || mIsUdf(arr_->get(nrdone_)))
	  && !arrstarted_ )
    {
	nrdone_++;
	return MoreToDo();
    }

    if ( !arr_->info().validPos(nrdone_) || !mIsUdf(arr_->get(nrdone_)) )
    {
	arrstarted_ = true;
	nrdone_++;
	return MoreToDo();
    }

    TypeSet<float> posidxs(4,0);
    if ( !getPositions(nrdone_,posidxs) )
    {
	nrdone_++;
	return MoreToDo();
    }

    if ( mIsUdf(arr_->get((int)posidxs[posidxs.size()-1])) ||
	    posidxs[posidxs.size()-1]==0 )
	return Finished();

    TypeSet<float> vals( posidxs.size(), (float)0 );

    for ( int idx=0; idx<vals.size(); idx++ )
	vals[idx] = arr_->get( (int)posidxs[idx] );

    float val = Interpolate::poly1D( posidxs[0], vals[0], posidxs[1], vals[1],
	    			     posidxs[2], vals[2], posidxs[3], vals[3],
				     (float)nrdone_ );
    if ( !mIsUdf(val) )
	arr_->set( nrdone_, val );
    nrdone_++;

    return MoreToDo();
}
