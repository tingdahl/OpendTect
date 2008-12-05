/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: dataclipper.cc,v 1.22 2008-12-05 23:14:37 cvskris Exp $";


#include "dataclipper.h"

#include "arraynd.h"
#include "math.h"
#include "math2.h"
#include "sorting.h"
#include "statrand.h"
#include "undefval.h"
#include "valseries.h"
#include "varlenarray.h"


DataClipper::DataClipper()
    : sampleprob_( 1 )
    , subselect_( false )
    , approxstatsize_( 2000 )
{
    Stats::RandGen::init();
} 


void DataClipper::setApproxNrValues( od_int64 n, int statsz )
{
    sampleprob_ = ((float) statsz) / n;
    approxstatsize_ = statsz;

    subselect_ = sampleprob_<1;
    sampleprob_ = mMIN( sampleprob_, 1 );
}


void DataClipper::putData( float val )
{
    if ( subselect_ )
    {
	double rand = Stats::RandGen::get();

	if ( rand>sampleprob_ )
	    return;
    }

    if ( Math::IsNormalNumber( val ) && !mIsUdf( val ) ) samples_ += val;
}

#define mPutDataImpl( getfunc ) \
    const od_int64 nrsamples = mNINT(nrvals * sampleprob_); \
    if ( subselect_ && nrsamples<nrvals ) \
    { \
	for ( od_int64 idx=0; idx<nrsamples; idx++ ) \
	{ \
	    double rand = Stats::RandGen::get(); \
	    rand *= (nrvals-1); \
	    const od_int64 sampidx = mNINT(rand); \
	    getfunc; \
	    if ( Math::IsNormalNumber( val ) && !mIsUdf( val ) ) \
		samples_ += val; \
	} \
    } \
    else \
    { \
	for ( od_int64 sampidx=0; sampidx<nrvals; sampidx++ ) \
	{ \
	    getfunc; \
	    if ( Math::IsNormalNumber( val ) && !mIsUdf( val ) ) samples_ += val; \
	} \
    }

void DataClipper::putData( const float* vals, od_int64 nrvals )
{
    mPutDataImpl( const float val = vals[sampidx] );
}


void DataClipper::putData( const ValueSeries<float>& vals, od_int64 nrvals )
{
    if ( vals.arr() )
    {
	putData( vals.arr(), nrvals );
	return;
    }

    mPutDataImpl( const float val = vals.value( sampidx ) );
}


void DataClipper::putData( const ArrayND<float>& vals )
{
    const od_int64 nrvals = vals.info().getTotalSz();
    if ( vals.getStorage() )
    {
	putData( *vals.getStorage(), nrvals );
	return;
    }

    const ArrayNDInfo& info = vals.info();

    mVariableLengthArr( int, idxs, info.getNDim() );
    mPutDataImpl(info.getArrayPos(sampidx,idxs); float val=vals.getND(idxs) );
}


bool DataClipper::calculateRange( float cliprate, Interval<float>& range )
				  
{
    return calculateRange( cliprate, cliprate, range );
}


bool DataClipper::calculateRange( float* vals, od_int64 nrvals,
				  float lowcliprate, float highcliprate,
				  Interval<float>& range )
{
    if ( !nrvals ) return false;

    od_int64 firstidx = mNINT(lowcliprate*nrvals);
    od_int64 topnr = mNINT(highcliprate*nrvals);
    od_int64 lastidx = nrvals-topnr-1;

    if ( firstidx && topnr )
    {
	sortFor( vals, nrvals, firstidx );
	range.start = vals[firstidx];

	sortFor( vals, nrvals, lastidx );
	range.stop = vals[lastidx];
    }
    else
    {
	float min, max;
	bool isset = false;
	for ( od_int64 idx=0; idx<nrvals; idx++ )
	{
	    const float val = vals[idx];

	    if ( mIsUdf(val) )
		continue;

	    if ( !isset || min>val )
		min = val;

	    if ( !isset || max<val )
		max = val;

	    isset = true;
	}

	if ( !isset )
	    return false;

	range.start = min;
	range.stop = max;
    }

    return true;
}


bool DataClipper::calculateRange( float lowcliprate, float highcliprate,
				  Interval<float>& range )
				  
{
    const bool res = calculateRange( samples_.arr(), samples_.size(),
	    lowcliprate, highcliprate, range );

    reset();

    return res;
}


bool DataClipper::fullSort()
{
    od_int64 nrvals = samples_.size();
    if ( !nrvals ) return false;

    if ( nrvals>100 )
	quickSort( samples_.arr(), nrvals );
    else
	sort_array( samples_.arr(), nrvals );

    return true;
}


bool DataClipper::getRange( float cliprate, Interval<float>& range ) const
{
    return getRange( cliprate/2, cliprate/2, range );
}


bool DataClipper::getRange( float lowclip, float highclip,
			    Interval<float>& range ) const
{
    if ( lowclip>1 || highclip>1 || highclip+lowclip>1 )
    {
	pErrMsg("The cliprate should between 0 and 1");
	return false;
    }

    od_int64 nrvals = samples_.size();
    if ( !nrvals ) return false;

    od_int64 firstidx = mNINT(lowclip*nrvals);
    od_int64 topnr = mNINT(highclip*nrvals);
    od_int64 lastidx = nrvals-topnr-1;

    range.start = samples_[firstidx];
    range.stop = samples_[lastidx];
    return true;
}


bool DataClipper::getSymmetricRange( float cliprate, float midval,
				     Interval<float>& range ) const
{
    const od_int64 nrvals = samples_.size();
    if ( !nrvals ) return false;

    const od_int64 nrsamplestoremove = mNINT(cliprate*nrvals);

    od_int64 firstsample = 0;
    od_int64 lastsample = nrvals-1;

    for ( od_int64 idx=0; idx<nrsamplestoremove; idx++ )
    {
	if ( firstsample==lastsample )
	    break;

	const float firstdist = fabs(midval-samples_[firstsample]);
	const float lastdist = fabs(midval-samples_[lastsample]);

	if ( firstdist>lastdist )
	    firstsample++;
	else
	    lastsample--;
    }


    const float firstdist = fabs(midval-samples_[firstsample]);
    const float lastdist = fabs(midval-samples_[lastsample]);
    const float halfwidth = mMAX( firstdist, lastdist );

    range.start = midval-halfwidth;
    range.stop = midval+halfwidth;

    return true;
}


void DataClipper::reset()
{
    samples_.erase();
    subselect_ = false;
    sampleprob_ = 1;
}
    
