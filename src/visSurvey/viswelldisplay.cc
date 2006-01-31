/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID = "$Id: viswelldisplay.cc,v 1.60 2006-01-31 16:53:46 cvshelene Exp $";

#include "viswelldisplay.h"

#include "draw.h"
#include "iopar.h"
#include "executor.h"
#include "ptrman.h"
#include "survinfo.h"
#include "visdataman.h"
#include "visevent.h"
#include "vismarker.h"
#include "vismaterial.h"
#include "vistransform.h"
#include "viswell.h"

#include "wellman.h"
#include "welllog.h"
#include "welllogset.h"
#include "welldata.h"
#include "welltransl.h"
#include "welltrack.h"
#include "wellmarker.h"
#include "welld2tmodel.h"

#define		mPickSz 	3
#define         mPickType	3 


mCreateFactoryEntry( visSurvey::WellDisplay );

namespace visSurvey
{

const char* WellDisplay::sKeyEarthModelID = "EarthModel ID";
const char* WellDisplay::sKeyWellID	 = "Well ID";
const char* WellDisplay::sKeyLog1Name	 = "Logname 1";
const char* WellDisplay::sKeyLog2Name	 = "Logname 2";
const char* WellDisplay::sKeyLog1Range	 = "Logrange 1";
const char* WellDisplay::sKeyLog2Range	 = "Logrange 2";
const char* WellDisplay::sKeyLog1Scale	 = "Loglogsc 1";
const char* WellDisplay::sKeyLog2Scale	 = "Loglogsc 2";
const char* WellDisplay::sKeyLog1Color	 = "Logcolor 1";
const char* WellDisplay::sKeyLog2Color	 = "Logcolor 2";

#define mMeter2Feet(val) \
   val /= 0.3048;

WellDisplay::WellDisplay()
    : VisualObjectImpl(true)
    , well_(0)
    , wellid_(-1)
    , zistime_(SI().zIsTime())
    , zinfeet_(SI().zInFeet())
    , eventcatcher_(0)
    , changed_(this)
    , transformation_(0)
    , picksallowed_(false)
    , group_(0)
    , pseudotrack_(0)
    , needsave_(false)
{
    setMaterial(0);
    setWell( visBase::Well::create() );
}


WellDisplay::~WellDisplay()
{
    removeChild( well_->getInventorNode() );
    well_->unRef();
    setSceneEventCatcher(0);
    if ( transformation_ ) transformation_->unRef();
    if ( group_ )
	removeChild( group_->getInventorNode() );
}


void WellDisplay::setWell( visBase::Well* well )
{
    if ( well_ )
    {
	removeChild( well_->getInventorNode() );
	well_->unRef();
    }

    well_ = well;    
    well_->ref();
    addChild( well_->getInventorNode() );
}


void WellDisplay::fullRedraw( CallBacker* )
{
    Well::Data* wd = Well::MGR().get( wellid_, false );
    if ( !wd ) return;
   
    TypeSet<Coord3> trackpos = getTrackPos( wd );
    if ( !trackpos.size() )
	return;

    well_->setTrack( trackpos );
    well_->setWellName( wd->name(), trackpos[0] );
    updateMarkers(0);

    if ( log1nm_.size() )
	displayLog( log1nm_, log1logsc_, log1rg_, 1 );

    if ( log2nm_.size())
	displayLog( log2nm_, log2logsc_, log2rg_, 2 );
}


TypeSet<Coord3> WellDisplay::getTrackPos( Well::Data* wd )
{
    TypeSet<Coord3> trackpos;
    const Well::D2TModel* d2t = wd->d2TModel();
    setName( wd->name() );

    if ( wd->track().size() < 1 ) return trackpos;
    PtrMan<Well::Track> ttrack = 0;
    if ( zistime_ )
    {
	ttrack = new Well::Track( wd->track() );
	ttrack->toTime( *d2t );
    }
    Well::Track& track = zistime_ ? *ttrack : wd->track();

    Coord3 pt;
    for ( int idx=0; idx<track.size(); idx++ )
    {
	pt = track.pos( idx );
	if ( zinfeet_ )
	    mMeter2Feet(pt.z);

	if ( !mIsUndefined(pt.z) )
	    trackpos += pt;
    }

    return trackpos;
}


#define mErrRet(s) { errmsg = s; return false; }

bool WellDisplay::setWellId( const MultiID& multiid )
{
    Well::Data* wd = Well::MGR().get( multiid, true );
    if ( !wd ) return false;
    
    const Well::D2TModel* d2t = wd->d2TModel();
    if ( zistime_ )
    {
	if ( !d2t )
	    mErrRet( "No depth to time model defined" )
	wd->d2tchanged.notify( mCB(this,WellDisplay,fullRedraw) );
    }

    wellid_ = multiid;
    fullRedraw(0);
    wd->markerschanged.notify( mCB(this,WellDisplay,updateMarkers) );

    return true;
}


Color WellDisplay::getColor() const
{
    return well_->lineStyle().color;
}


const LineStyle* WellDisplay::lineStyle() const
{
    return &well_->lineStyle();
}


void WellDisplay::setLineStyle( const LineStyle& lst )
{
    well_->setLineStyle( lst );
}


void WellDisplay::updateMarkers( CallBacker* )
{
    Well::Data* wd = Well::MGR().get( wellid_ );
    if ( !wd ) return;

    well_->removeAllMarkers();
    for ( int idx=0; idx<wd->markers().size(); idx++ )
    {
	Well::Marker* wellmarker = wd->markers()[idx];
	Coord3 pos = wd->track().getPos( wellmarker->dah );
	if ( !pos.x && !pos.y && !pos.z ) continue;

	if ( zistime_ )
	    pos.z = wd->d2TModel()->getTime( wellmarker->dah );
	else if ( zinfeet_ )
	    mMeter2Feet(pos.z)

	well_->addMarker( pos, wellmarker->color, wellmarker->name() );
    }
}


void WellDisplay::setMarkerScreenSize( int sz )
{ well_->setMarkerScreenSize( sz ); }

int WellDisplay::markerScreenSize() const
{ return well_->markerScreenSize(); }


#define mShowFunction( showObj, objShown ) \
void WellDisplay::showObj( bool yn ) \
{ \
    well_->showObj( yn ); \
} \
\
bool WellDisplay::objShown() const \
{ \
    return well_->objShown(); \
}

bool WellDisplay::canShowMarkers() const
{ return well_->canShowMarkers(); }

mShowFunction( showWellName, wellNameShown )
mShowFunction( showMarkers, markersShown )
mShowFunction( showMarkerName, markerNameShown )
mShowFunction( showLogs, logsShown )
mShowFunction( showLogName, logNameShown )


void WellDisplay::displayLog( int logidx, int lognr, bool logrthm,
			      const Interval<float>* range )
{
    Well::Data* wd = Well::MGR().get( wellid_ );
    if ( !wd || !wd->logs().size() ) return;

    Well::Log& log = wd->logs().getLog(logidx);
    const int logsz = log.size();
    if ( !logsz ) return;

    Well::Track& track = wd->track();
    TypeSet<Coord3Value> crdvals;
    for ( int idx=0; idx<logsz; idx++ )
    {
	const float dah = log.dah(idx);
	Coord3 pos = track.getPos( dah );
	if ( !pos.x && !pos.y && !pos.z ) continue;

	if ( zistime_ )
	    pos.z = wd->d2TModel()->getTime( dah );
	else if ( zinfeet_ )
	    mMeter2Feet(pos.z)

	Coord3Value cv( pos, log.value(idx) );
	crdvals += cv;
    }

    Interval<float> selrange;
    assign( selrange, range ? *range : log.selValueRange() );
    if ( !range )
	logrthm = log.dispLogarithmic();
    well_->setLogData( crdvals, log.name(), selrange, logrthm, lognr );

    if ( lognr == 1 )
	{log1nm_ = log.name(); assign(log1rg_,selrange); log1logsc_ = logrthm;}
    else
	{log2nm_ = log.name(); assign(log2rg_,selrange); log2logsc_ = logrthm;}
}


void WellDisplay::displayLog( const char* lognm, bool logarthm,
			      const Interval<float>& range, int lognr )
{
    Well::Data* wd = Well::MGR().get( wellid_ );
    if ( !wd || !wd->logs().size() ) return;

    int logidx = -1;
    for ( int idx=0; idx<wd->logs().size(); idx++ )
    {
	const char* nm = wd->logs().getLog(idx).name();
	if ( !strcmp(lognm,nm) ) { logidx = idx; break; }
    }

    if ( logidx < 0 ) return; // TODO: errmsg
    
    displayLog( logidx, lognr, logarthm, &range );
}


void WellDisplay::setLogColor( const Color& col, int lognr )
{ well_->setLogColor( col, lognr ); }


const Color& WellDisplay::logColor( int lognr ) const
{ return well_->logColor( lognr ); }


void WellDisplay::setLogLineWidth( float width, int lognr )
{ well_->setLogLineWidth( width, lognr ); }


float WellDisplay::logLineWidth( int lognr ) const
{ return well_->logLineWidth( lognr ); }


void WellDisplay::setLogWidth( int width )
{ well_->setLogWidth( width ); }


int WellDisplay::logWidth() const
{ return well_->logWidth(); }


void WellDisplay::getMousePosInfo( const visBase::EventInfo&,
				   const Coord3& pos,
				   float& val,
				   BufferString& info ) const
{
    val = -mUdf(float);
    Well::Data* wd = Well::MGR().get( wellid_ );
    if ( !wd ) { info = ""; return; }

    const float mousez = pos.z * SI().zFactor();
    const float zstep2 = SI().zFactor() * SI().zStep()/2;

    info = "Well: "; info += wd->name();
    for ( int idx=0; idx<wd->markers().size(); idx++ )
    {
	Well::Marker* wellmarker = wd->markers()[idx];
	Coord3 markerpos = wd->track().getPos( wellmarker->dah );
	if ( !mIsEqual(markerpos.z,mousez,zstep2) )
	    continue;

	info += ", Marker: ";
	info += wellmarker->name();
	break;
    }
}


void WellDisplay::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visBase::VisualObjectImpl::fillPar( par, saveids );

    par.set( sKeyEarthModelID, wellid_ );

    const int viswellid = well_->id();
    par.set( sKeyWellID, viswellid );
    if ( saveids.indexOf(viswellid) == -1 ) saveids += viswellid;
    
    BufferString colstr;

#define mStoreLogPars( num ) \
    par.set( sKeyLog##num##Name, log##num##nm_ ); \
    par.set( sKeyLog##num##Range, log##num##rg_.start, log##num##rg_.stop ); \
    par.setYN( sKeyLog##num##Scale, log##num##logsc_ ); \
    logColor(num).fill( colstr.buf() ); \
    par.set( sKeyLog##num##Color, colstr )

    mStoreLogPars(1);
    mStoreLogPars(2);
}


int WellDisplay::usePar( const IOPar& par )
{
    int res = visBase::VisualObjectImpl::usePar( par );
    if ( res!=1 ) return res;

    int viswellid;
    if ( par.get(sKeyWellID,viswellid) )
    {
	DataObject* dataobj = visBase::DM().getObject( viswellid );
	if ( !dataobj ) return 0;
	mDynamicCastGet(visBase::Well*,well,dataobj)
	if ( !well ) return -1;
	setWell( well );
    }
    else
    {
	setWell( visBase::Well::create() );
	viswellid = well_->id();
    }
    
    MultiID newmid;
    if ( !par.get(sKeyEarthModelID,newmid) )
	return -1;

    if ( !setWellId(newmid) )
    {
	return 1;
    }

    BufferString logname;
    BufferString colstr;
    Color col;

#define mRetrieveLogPars( num ) \
    par.get( sKeyLog##num##Name, logname ); \
    par.get( sKeyLog##num##Range, log##num##rg_.start, log##num##rg_.stop ); \
    par.getYN( sKeyLog##num##Scale, log##num##logsc_ ); \
    if ( *logname.buf() ) \
	displayLog( logname, log##num##logsc_, log##num##rg_, num ); \
    par.get( sKeyLog##num##Color, colstr ); \
    if ( col.use(colstr.buf()) ) \
	setLogColor( col, num )

    mRetrieveLogPars( 1 );
    mRetrieveLogPars( 2 );

// Support for old sessions
    BufferString linestyle;
    if ( par.get(visBase::Well::linestylestr,linestyle) )
    {
	LineStyle lst;
	lst.fromString( linestyle );
	setLineStyle( lst );
    }

    bool wellnmshown;
    if ( par.getYN(visBase::Well::showwellnmstr,wellnmshown) )
	showWellName( wellnmshown );

    return 1;
}


void WellDisplay::setDisplayTransformation( visBase::Transformation* nt )
{
    well_->setDisplayTransformation( nt );
    setDisplayTransformForPicks( nt );
    fullRedraw(0);
}


visBase::Transformation* WellDisplay::getDisplayTransformation()
{ return well_->getDisplayTransformation(); }


void WellDisplay::pickCB( CallBacker* cb )
{
    if ( !isSelected() || !picksallowed_ || !group_ || isLocked() ) return;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);
    if ( eventinfo.type != visBase::MouseClick ||
	 eventinfo.mousebutton != visBase::EventInfo::leftMouseButton() )
	return;

    int eventid = -1;
    for ( int idx=0; idx<eventinfo.pickedobjids.size(); idx++ )
    {
	visBase::DataObject* dataobj =
	    visBase::DM().getObject(eventinfo.pickedobjids[idx]);
	if ( dataobj->selectable() )
	{
	    eventid = eventinfo.pickedobjids[idx];
	    break;
	}
    }

    if ( eventinfo.pressed )
    {
	mousepressid_ = eventid;
	mousepressposition_ = eventid==-1 ? Coord3::udf() : eventinfo.pickedpos;
	eventcatcher_->eventIsHandled();
    }
    else
    {
	if ( eventinfo.ctrl && !eventinfo.alt && !eventinfo.shift )
	{
	    if ( eventinfo.pickedobjids.size() && eventid==mousepressid_ )
	    {
		int removeidx = group_->getFirstIdx(mousepressid_);
		if ( removeidx != -1 )
		{
		    group_->removeObject( removeidx );
		    pseudotrack_->removePoint( removeidx );
		    TypeSet<Coord3> wcoords;
		    for ( int idx=0; idx<pseudotrack_->nrPoints(); idx++ )
		    {
			wcoords += pseudotrack_->pos(idx);
			wcoords[idx].z /= SI().zFactor();
		    }

		    well_->setTrack(wcoords);
		    needsave_ = true;
		    changed_.trigger();
		}
	    }

	    eventcatcher_->eventIsHandled();
	}
	else if ( !eventinfo.ctrl && !eventinfo.alt && !eventinfo.shift )
	{
	    if ( eventinfo.pickedobjids.size() && eventid==mousepressid_ )
	    {
		const int sz = eventinfo.pickedobjids.size();
		bool validpicksurface = false;

		for ( int idx=0; idx<sz; idx++ )
		{
		    const DataObject* pickedobj =
			visBase::DM().getObject(eventinfo.pickedobjids[idx]);
		    mDynamicCastGet(const SurveyObject*,so,pickedobj)
		    if ( so && so->allowPicks() )
		    {
			validpicksurface = true;
			break;
		    }
		}

		if ( validpicksurface )
		{
		    Coord3 newpos = scene_->getZScaleTransform()->
			transformBack( eventinfo.pickedpos );
		    if ( transformation_ )
			newpos = transformation_->transformBack(newpos);
		    mDynamicCastGet(SurveyObject*,so,
				    visBase::DM().getObject(eventid))
		    if ( so ) so->snapToTracePos( newpos );
		    addPick( newpos );
		}
	    }

	    eventcatcher_->eventIsHandled();
	}
    }
}


void WellDisplay::addPick( Coord3 pos )
{
    int insertidx = -1;
    if ( pseudotrack_ )
    {
	TypeSet<Coord3> wcoords;
	insertidx = pseudotrack_->insertPoint( Coord(pos.x, pos.y), 
					       pos.z * SI().zFactor() );
	for ( int idx=0; idx<pseudotrack_->nrPoints(); idx++ )
	{
	    wcoords += pseudotrack_->pos(idx);
	    wcoords[idx].z /= SI().zFactor();
	}

	well_->setTrack(wcoords);
	needsave_ = true;
	changed_.trigger();
    }
    
    if ( insertidx > -1 )
    {
	visBase::Marker* marker = visBase::Marker::create();
	group_->insertObject( insertidx, marker );

	marker->setDisplayTransformation( transformation_ );
	marker->setCenterPos( pos );
	marker->setScreenSize( mPickSz );
	marker->setType( (MarkerStyle3D::Type)mPickType );
	marker->getMaterial()->setColor( lineStyle()->color );
    }
}


void WellDisplay::setDisplayTransformForPicks( visBase::Transformation* newtr )
{
    if ( transformation_==newtr )
	return;

    if ( transformation_ )
	transformation_->unRef();

    transformation_ = newtr;

    if ( transformation_ )
	transformation_->ref();

    if ( !group_ ) return;
    for ( int idx=0; idx<group_->size(); idx++ )
    {
	mDynamicCastGet( visBase::Marker*, marker, group_->getObject(idx));
	marker->setDisplayTransformation( transformation_ );
    }
}


void WellDisplay::setSceneEventCatcher( visBase::EventCatcher* nevc )
{
    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.remove(mCB(this,WellDisplay,pickCB));
	eventcatcher_->unRef();
    }

    eventcatcher_ = nevc;

    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.notify(mCB(this,WellDisplay,pickCB));
	eventcatcher_->ref();
    }
}


void WellDisplay::setupPicking( bool yn )
{
    picksallowed_ = yn;
    if ( !group_ )
    {
	group_ = visBase::DataObjectGroup::create();
	pseudotrack_ = new Well::Track();
	addChild( group_->getInventorNode() );
    }
}


void WellDisplay::showKnownPositions()
{
    Well::Data* wd = Well::MGR().get( wellid_, false );
    if ( !wd ) return;
   
    TypeSet<Coord3> trackpos = getTrackPos( wd );
    if ( !trackpos.size() )
	return;

    for ( int idx=0; idx<trackpos.size(); idx++ )
	addPick( trackpos[idx] );
}


TypeSet<Coord3> WellDisplay::getWellCoords() const
{
    TypeSet<Coord3> coords;
    for ( int idx=0; idx<pseudotrack_->nrPoints(); idx++ )
    {
	coords += pseudotrack_->pos(idx);
	coords[idx].z /= SI().zFactor();
    }

    return coords;
}

}; // namespace visSurvey
