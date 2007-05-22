/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Oct 1999
 RCS:           $Id: emhorizonztransform.cc,v 1.6 2007-05-22 03:23:23 cvsnanne Exp $
________________________________________________________________________

-*/

#include "emhorizonztransform.h"

#include "emmanager.h"
#include "emhorizon3d.h"
#include "executor.h"
#include "iopar.h"
#include "survinfo.h"
#include "sorting.h"

namespace EM
{

void HorizonZTransform::initClass()
{
    ::ZATF().addCreator( EM::HorizonZTransform::create,
	    	       EM::HorizonZTransform::sName() );
}


HorizonZTransform::HorizonZTransform( const Horizon3D* hor )
    : horizon_( 0 )
    , horchanged_( false )
{
    if ( hor )
	setHorizon( *hor );
}


HorizonZTransform::~HorizonZTransform()
{ if ( horizon_ ) horizon_->unRef(); }



void HorizonZTransform::setHorizon( const Horizon3D& hor )
{
    if ( horizon_ )
    {
	const_cast<Horizon3D*>(horizon_)
	    ->notifier.remove( mCB(this,HorizonZTransform,horChangeCB) );
	horizon_->unRef();
    }

    horizon_ = &hor;
    horizon_->ref();
    const_cast<Horizon3D*>(horizon_)
	->notifier.notify( mCB(this,HorizonZTransform,horChangeCB) );

    horchanged_ = true;
    calculateHorizonRange();
}


void HorizonZTransform::transform( const BinID& bid,
	const SamplingData<float>& sd, int sz,float* res ) const
{
    if ( mIsUdf(sd.start) || mIsUdf(sd.step) )
    {
	for ( int idx=sz-1; idx>=0; idx-- )
	    res[idx] = mUdf(float);

	return;
    }

    if ( !horizon_ )
    {
	for ( int idx=sz-1; idx>=0; idx-- )
	    res[idx] = sd.atIndex( idx );

	return;
    }

    float top, bottom;
    if ( !getTopBottom( bid, top, bottom ) )
    {
	for ( int idx=sz-1; idx>=0; idx-- )
	    res[idx] = mUdf(float);

	return;
    }

    for ( int idx=sz-1; idx>=0; idx-- )
    {
	const float depth = sd.atIndex( idx );
	if ( depth<top ) res[idx] = depth-top;
	else if ( depth>bottom ) res[idx] = depth-bottom;
	else res[idx] = 0;
    }
}


void HorizonZTransform::transformBack( const BinID& bid,
	const SamplingData<float>& sd, int sz,float* res ) const
{
    for ( int idx=sz-1; idx>=0; idx-- )
	res[idx] = mUdf(float);

    if ( !horizon_ || mIsUdf(sd.start) || mIsUdf(sd.step) )
	return;

    float top, bottom;
    if ( !getTopBottom( bid, top, bottom ) )
	return;

    for ( int idx=sz-1; idx>=0; idx-- )
    {
	const float depth = sd.atIndex( idx );
	if ( depth<=0 ) res[idx] = depth+top;
	else res[idx] = depth+bottom;
    }
}


Interval<float> HorizonZTransform::getZInterval( bool from ) const
{
    if ( from ) return SI().zRange(true);
    
    if ( horchanged_ )
	const_cast<HorizonZTransform*>(this)->calculateHorizonRange();

    if ( horchanged_ ) return SI().zRange(true);

    return Interval<float>( SI().zRange(true).start-depthrange_.stop,
	    		    SI().zRange(true).stop-depthrange_.start );
}


void HorizonZTransform::fillPar( IOPar& par ) const
{
    ZAxisTransform::fillPar( par );

    if ( horizon_ )
	par.set( sKeyHorizonID(), horizon_->multiID() );
}


bool HorizonZTransform::usePar( const IOPar& par )
{
    if ( !ZAxisTransform::usePar( par ) )
	return false;

    MultiID mid;
    if ( !par.get( sKeyHorizonID(), mid ) )
	return true;

    EM::ObjectID emid = EM::EMM().getObjectID( mid );
    RefMan<EM::EMObject> emobj = EM::EMM().getObject( emid );
    if ( !emobj )
    {
	PtrMan<Executor> loader = EM::EMM().objectLoader( mid );
	if ( !loader || !loader->execute() ) return false;

	emid = EM::EMM().getObjectID( mid );
	emobj = EM::EMM().getObject( emid );
    }

    mDynamicCastGet( EM::Horizon3D*, hor, emobj.ptr() );
    if ( !hor )
	return false;

    setHorizon( *hor );
    return true;
}


float HorizonZTransform::getZIntervalCenter( bool from ) const
{
    if ( from )
	return ZAxisTransform::getZIntervalCenter( from );

    return 0;
}


void HorizonZTransform::horChangeCB(CallBacker*)
{ horchanged_ = true; }


void HorizonZTransform::calculateHorizonRange()
{
    if ( !horizon_ ) return;

    PtrMan<EMObjectIterator> iterator = horizon_->createIterator( -1, 0 );
    if ( !iterator ) return;

    bool isset = false;

    EM::PosID pid = iterator->next();
    while ( pid.objectID()!=-1  )
    {
	const float depth = horizon_->getPos( pid ).z;
	if ( !mIsUdf( depth ) )
	{
	    if ( isset ) depthrange_.include( depth, false );
	    else { depthrange_.start = depthrange_.stop = depth; isset=true; }
	}

	pid = iterator->next();
    }

    horchanged_ = false;
}


bool HorizonZTransform::getTopBottom( const BinID& bid, float& top,
				      float& bottom ) const
{
    TypeSet<float> depths;
    for ( int idx=horizon_->nrSections()-1; idx>=0; idx-- )
    {
	const SectionID sid = horizon_->sectionID( idx );
	const Geometry::BinIDSurface* geom =
	    horizon_->geometry().sectionGeometry(sid);
	const float depth = geom->computePosition( Coord(bid.inl,bid.crl) ).z;
	if ( !mIsUdf(depth) )
	    depths += depth;
    }

    if ( depths.size()>1 )
	sort_array( depths.arr(), depths.size() );
    else if ( !depths.size() )
	return false;

    top = depths[0];
    bottom = depths[depths.size()-1];

    return true;
}

} // namespace EM
