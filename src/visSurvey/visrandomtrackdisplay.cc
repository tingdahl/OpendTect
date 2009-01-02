/*+
 ________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          January 2003
 ________________________________________________________________________

-*/
static const char* rcsID = "$Id: visrandomtrackdisplay.cc,v 1.107 2009-01-02 11:34:46 cvsranojay Exp $";


#include "visrandomtrackdisplay.h"

#include "arrayndimpl.h"
#include "attribdatapack.h"
#include "attribsel.h"
#include "iopar.h"
#include "seisbuf.h"
#include "seistrc.h"
#include "interpol1d.h"
#include "scaler.h"
#include "keystrs.h"
#include "survinfo.h"
#include "visdataman.h"
#include "vismaterial.h"
#include "visrandomtrack.h"
#include "visrandomtrackdragger.h"
#include "vissplittexturerandomline.h"
#include "vistexturecoords.h"
#include "vistransform.h"
#include "viscolortab.h"
#include "viscoord.h"
#include "vismultitexture2.h"
#include "ptrman.h"

#include <math.h>

mCreateFactoryEntry( visSurvey::RandomTrackDisplay );

namespace visSurvey
{

const char* RandomTrackDisplay::sKeyTrack() 	    { return "Random track"; }
const char* RandomTrackDisplay::sKeyNrKnots() 	    { return "Nr. Knots"; }
const char* RandomTrackDisplay::sKeyKnotPrefix()    { return "Knot "; }
const char* RandomTrackDisplay::sKeyDepthInterval() { return "Depth Interval"; }
const char* RandomTrackDisplay::sKeyLockGeometry()  { return "Lock geometry"; }

RandomTrackDisplay::RandomTrackDisplay()
    : triangles_( visBase::SplitTextureRandomLine::create() )
    , dragger_( visBase::RandomTrackDragger::create() )
    , knotmoving_(this)
    , moving_(this)
    , selknotidx_(-1)
    , ismanip_(false)
    , datatransform_( 0 )
    , lockgeometry_( false )
{
    TypeSet<int> randomlines;
    visBase::DM().getIds( typeid(*this), randomlines );
    int highestnamenr = 0;
    for ( int idx=0; idx<randomlines.size(); idx++ )
    {
	mDynamicCastGet( const RandomTrackDisplay*, rtd,
			 visBase::DM().getObject(randomlines[idx]) );
	if ( rtd == this )
	    continue;

	if ( rtd->nameNr()>highestnamenr )
	    highestnamenr = rtd->nameNr();
    }

    namenr_ = highestnamenr+1;
    BufferString nm( "Random Line "); nm += namenr_;
    setName( nm );

    material_->setColor( Color::White() );
    material_->setAmbience( 0.8 );
    material_->setDiffIntensity( 0.2 );

    dragger_->ref();
    insertChild( childIndex(texture_->getInventorNode()),
	        
	         dragger_->getInventorNode() );
    dragger_->motion.notify( mCB(this,visSurvey::RandomTrackDisplay,knotMoved));

    triangles_->ref();
    addChild( triangles_->getInventorNode() );

    const StepInterval<float>& survinterval = SI().zRange(true);
    const StepInterval<float> inlrange( SI().sampling(true).hrg.start.inl,
	    				SI().sampling(true).hrg.stop.inl,
					SI().inlStep() );
    const StepInterval<float> crlrange( SI().sampling(true).hrg.start.crl,
	    				SI().sampling(true).hrg.stop.crl,
	    				SI().crlStep() );

    const BinID start( mNINT(inlrange.center()), mNINT(crlrange.start) );
    const BinID stop(start.inl, mNINT(crlrange.stop) );

    addKnot( start );
    addKnot( stop );

    setDepthInterval( Interval<float>( survinterval.start,
				       survinterval.stop ));

    dragger_->setLimits(
	    Coord3( inlrange.start, crlrange.start, survinterval.start ),
	    Coord3( inlrange.stop, crlrange.stop, survinterval.stop ),
	    Coord3( inlrange.step, crlrange.step, survinterval.step ) );

    const int baselen = mNINT((inlrange.width()+crlrange.width())/2);
    
    dragger_->setSize( Coord3(baselen/50,baselen/50,survinterval.width()/50) );
}


RandomTrackDisplay::~RandomTrackDisplay()
{
    triangles_->unRef();
    dragger_->unRef();

    deepErase( cache_ );

    DataPackMgr& dpman = DPM( DataPackMgr::FlatID() );
    for ( int idx=0; idx<datapackids_.size(); idx++ )
	dpman.release( datapackids_[idx] );
}


void RandomTrackDisplay::setDepthInterval( const Interval<float>& intv )
{ 
    const Interval<float> curint = getDepthInterval();
    if ( mIsEqual(curint.start,intv.start, 1e-3 ) &&
	 mIsEqual(curint.stop,intv.stop, 1e-3 ) )
	return;

    triangles_->setDepthRange( intv );
    dragger_->setDepthRange( intv );

    texture_->clearAll();
    moving_.trigger();
}


Interval<float> RandomTrackDisplay::getDepthInterval() const
{
    return triangles_->getDepthRange();
}


Interval<float> RandomTrackDisplay::getDataTraceRange() const
{
    //TODO Adapt if ztransform is present
    return triangles_->getDepthRange();
}


int RandomTrackDisplay::nrKnots() const
{ return knots_.size(); }


void RandomTrackDisplay::addKnot( const BinID& bid )
{
    const BinID sbid = snapPosition( bid );
    if ( checkPosition(sbid) )
    {
	knots_ += sbid;
	triangles_->setDepthRange( getDataTraceRange() );
	triangles_->setLineKnots( knots_ );	
	dragger_->setKnot( knots_.size()-1, Coord(sbid.inl,sbid.crl) );
	texture_->clearAll();
	moving_.trigger();
    }
}


void RandomTrackDisplay::insertKnot( int knotidx, const BinID& bid )
{
    const BinID sbid = snapPosition(bid);
    if ( checkPosition(sbid) )
    {
	knots_.insert( knotidx, sbid );
	triangles_->setDepthRange( getDataTraceRange() );
	triangles_->setLineKnots( knots_ );	
	dragger_->insertKnot( knotidx, Coord(sbid.inl,sbid.crl) );
	texture_->clearAll();
	for ( int idx=0; idx<nrAttribs(); idx++ )
	    if ( cache_[idx] ) setData( idx, *cache_[idx] );

	moving_.trigger();
    }
}


BinID RandomTrackDisplay::getKnotPos( int knotidx ) const
{
    return knots_[knotidx];
}


BinID RandomTrackDisplay::getManipKnotPos( int knotidx ) const
{
    const Coord crd = dragger_->getKnot( knotidx );
    return BinID( mNINT(crd.x), mNINT(crd.y) );
}


void RandomTrackDisplay::getAllKnotPos( TypeSet<BinID>& knots ) const
{
    const int nrknots = nrKnots();
    for ( int idx=0; idx<nrknots; idx++ )
	knots += getManipKnotPos( idx );
}


void RandomTrackDisplay::setKnotPos( int knotidx, const BinID& bid )
{ setKnotPos( knotidx, bid, true ); }


void RandomTrackDisplay::setKnotPos( int knotidx, const BinID& bid, bool check )
{
    const BinID sbid = snapPosition(bid);
    if ( !check || checkPosition(sbid) )
    {
	knots_[knotidx] = sbid;

	triangles_->setDepthRange( getDataTraceRange() );
	triangles_->setLineKnots( knots_ );	
	dragger_->setKnot( knotidx, Coord(sbid.inl,sbid.crl) );
	texture_->clearAll();
	moving_.trigger();
    }
}


static void decoincideKnots( const TypeSet<BinID>& knots, 
			     TypeSet<BinID>& uniqueknots )
{
    uniqueknots.erase();
    if ( knots.isEmpty() )
	return;
    uniqueknots += knots[0];

    for ( int idx=1; idx<knots.size(); idx++ )
    {
	const BinID prev = uniqueknots[uniqueknots.size()-1];
    	const BinID biddif = prev - knots[idx];
	const int nrsteps = mMAX( abs(biddif.inl)/SI().inlStep(), 
				  abs(biddif.crl)/SI().crlStep() );
	const Coord dest = SI().transform( knots[idx] );
	const Coord crddif = SI().transform(prev) - dest;
	for ( int step=0; step<nrsteps; step++ )
	{
	    const BinID newknot = SI().transform( dest+(crddif*step)/nrsteps );
	    if ( uniqueknots.indexOf(newknot) < 0 )
	    {
		uniqueknots += newknot;
		break;
	    }
	}
    }
}


void RandomTrackDisplay::setKnotPositions( const TypeSet<BinID>& newbids )
{
    TypeSet<BinID> uniquebids;
    decoincideKnots( newbids, uniquebids );
   
    if ( uniquebids.size() < 2 ) 
	return;
    while ( nrKnots() > uniquebids.size() )
	removeKnot( nrKnots()-1 );

    for ( int idx=0; idx<uniquebids.size(); idx++ )
    {
	const BinID bid = uniquebids[idx];

	if ( idx < nrKnots() )
	    setKnotPos( idx, bid, false );
	else
	    addKnot( bid );
    }
}


void RandomTrackDisplay::removeKnot( int knotidx )
{
    if ( nrKnots()< 3 )
    {
	pErrMsg("Can't remove knot");
	return;
    }

    knots_.remove(knotidx);
    triangles_->setLineKnots( knots_ );	
    dragger_->removeKnot( knotidx );
}


void RandomTrackDisplay::getDataTraceBids( TypeSet<BinID>& bids ) const
{ getDataTraceBids( bids, 0 ); }


#define mGetBinIDs( x, y ) \
    bool reverse = stop.x - start.x < 0; \
    int step = inlwise ? SI().inlStep() : SI().crlStep(); \
    if ( reverse ) step *= -1; \
    for ( int idi=0; idi<nrlines; idi++ ) \
    { \
	BinID bid; \
	int bidx = start.x + idi*step; \
	float val = Interpolate::linear1D( (float)start.x, (float)start.y, \
					   (float)stop.x, (float)stop.y, \
					   (float)bidx ); \
	int bidy = (int)(val + .5); \
	BinID nextbid = inlwise ? BinID(bidx,bidy) : BinID(bidy,bidx); \
	SI().snap( nextbid ); \
	const_cast<RandomTrackDisplay*>(this)->trcspath_.addIfNew( nextbid ); \
	bids += nextbid ; \
	if ( segments ) (*segments) += (idx-1);\
    }


void RandomTrackDisplay::getDataTraceBids( TypeSet<BinID>& bids,
       					   TypeSet<int>* segments ) const
{
    const_cast<RandomTrackDisplay*>(this)->trcspath_.erase(); 
    TypeSet<BinID> knots;
    getAllKnotPos( knots );
    for ( int idx=1; idx<knots.size(); idx++ )
    {
	BinID start = knots[idx-1];
	BinID stop = knots[idx];
	const int nrinl = int(abs(stop.inl-start.inl) / SI().inlStep() + 1);
	const int nrcrl = int(abs(stop.crl-start.crl) / SI().crlStep() + 1);
	bool inlwise = nrinl > nrcrl;
	int nrlines = inlwise ? nrinl : nrcrl;
	if ( inlwise )
	{ mGetBinIDs(inl,crl); }
	else 
	{ mGetBinIDs(crl,inl); }
    }
}


bool RandomTrackDisplay::setDataPackID( int attrib, DataPack::ID dpid )
{
    DataPackMgr& dpman = DPM( DataPackMgr::FlatID() );
    const DataPack* datapack = dpman.obtain( dpid );
    mDynamicCastGet(const Attrib::FlatRdmTrcsDataPack*,dprdm,datapack);
    if ( !dprdm )
    {
	dpman.release( dpid );
	SeisTrcBuf trcbuf( false );
	setTraceData( attrib, trcbuf );
	return false;
    }

    SeisTrcBuf tmpbuf( dprdm->seisBuf() );
    setTraceData( attrib, tmpbuf );

    DataPack::ID oldid = datapackids_[attrib];
    datapackids_[attrib] = dpid;
    dpman.release( oldid );
    return true;
}


DataPack::ID RandomTrackDisplay::getDataPackID( int attrib ) const
{
    return datapackids_[attrib];
}


void RandomTrackDisplay::setTraceData( int attrib, SeisTrcBuf& trcbuf )
{
    setData( attrib, trcbuf );

    if ( !cache_[attrib] )
	cache_.replace( attrib, new SeisTrcBuf(false) );

    cache_[attrib]->deepErase();
    cache_[attrib]->stealTracesFrom( trcbuf );

    //TODO Find other means of reseting ismanip_
    //ismanip_ = false;
}


void RandomTrackDisplay::setData( int attrib, const SeisTrcBuf& trcbuf )
{
    const int nrtrcs = trcbuf.size();
    if ( !nrtrcs )
    {
	texture_->setData( attrib, 0, 0 );
	texture_->turnOn( false );
	return;
    }

    const Interval<float> zrg = getDataTraceRange();
    const float step = trcbuf.get(0)->info().sampling.step;
    const int nrsamp = mNINT( zrg.width() / step ) + 1;

    TypeSet<BinID> path;
    getDataTraceBids( path );

    const int nrslices = trcbuf.get(0)->nrComponents();
    texture_->setNrVersions( attrib, nrslices );
    for ( int sidx=0; sidx<nrslices; sidx++ )
    {
	Array2DImpl<float> array( path.size(), nrsamp );
	float* dataptr = array.getData();

	for ( int idx=array.info().getTotalSz()-1; idx>=0; idx-- )
	    dataptr[idx] = mUdf(float);

	for ( int posidx=path.size()-1; posidx>=0; posidx-- )
	{
	    const BinID bid = path[posidx];
	    const int trcidx = trcbuf.find( bid, false );
	    if ( trcidx<0 )
		continue;

	    const SeisTrc* trc = trcbuf.get( trcidx );
	    if ( !trc || sidx>trc->nrComponents() )
		continue;

	    float* arrptr = dataptr + array.info().getOffset( posidx, 0 );

	    if ( !datatransform_ )
	    {
		for ( int ids=0; ids<nrsamp; ids++ )
		{
		    const float ctime = zrg.start + ids*step;
		    if ( !trc->dataPresent(ctime) )
			continue;

		    arrptr[ids] = trc->getValue(ctime,sidx);
		}
	    }
	    else
	    {
		//todo
	    }
	}

	texture_->splitTexture( true );
	texture_->setData( attrib, sidx, &array, true );
	
	triangles_->enableSpliting( texture_->canUseShading() );
	triangles_->setTextureUnits( texture_->getUsedTextureUnits() );	
	triangles_->setDepthRange( zrg );
	triangles_->setTexturePath( path, nrsamp );
    }
    
    texture_->turnOn( true );
}


bool RandomTrackDisplay::canAddKnot( int knotnr ) const
{
    if ( lockgeometry_ ) return false;
    if ( knotnr<0 ) knotnr=0;
    if ( knotnr>nrKnots() ) knotnr=nrKnots();

    const BinID newpos = proposeNewPos(knotnr);
    return checkPosition(newpos);
}


void RandomTrackDisplay::addKnot( int knotnr )
{
    if ( knotnr<0 ) knotnr=0;
    if ( knotnr>nrKnots() ) knotnr=nrKnots();

    if ( !canAddKnot(knotnr) ) return;

    const BinID newpos = proposeNewPos(knotnr);
    if ( knotnr==nrKnots() )
	addKnot( newpos );
    else
	insertKnot( knotnr, newpos );
}
    

BinID RandomTrackDisplay::proposeNewPos(int knotnr ) const
{
    BinID res;
    if ( !knotnr )
	res = getKnotPos(0)-(getKnotPos(1)-getKnotPos(0));
    else if ( knotnr>=nrKnots() )
	res = getKnotPos(nrKnots()-1) +
	      (getKnotPos(nrKnots()-1)-getKnotPos(nrKnots()-2));
    else
    {
	res = getKnotPos(knotnr)+getKnotPos(knotnr-1);
	res.inl /= 2;
	res.crl /= 2;
    }

    res.inl = mMIN( SI().inlRange(true).stop, res.inl );
    res.inl = mMAX( SI().inlRange(true).start, res.inl );
    res.crl = mMIN( SI().crlRange(true).stop, res.crl );
    res.crl = mMAX( SI().crlRange(true).start, res.crl );

    SI().snap(res, BinID(0,0) );

    return res;
}


bool RandomTrackDisplay::isManipulated() const
{
    return ismanip_;
}

 
void RandomTrackDisplay::acceptManipulation()
{
    setDepthInterval( dragger_->getDepthRange() );
    for ( int idx=0; idx<nrKnots(); idx++ )
    {
	const Coord crd = dragger_->getKnot(idx);
	setKnotPos( idx, BinID( mNINT(crd.x), mNINT(crd.y) ));
    }

    ismanip_ = false;
}


void RandomTrackDisplay::resetManipulation()
{
    dragger_->setDepthRange( getDepthInterval() );
    for ( int idx=0; idx<nrKnots(); idx++ )
    {
	const BinID bid = getKnotPos(idx);
	dragger_->setKnot(idx, Coord(bid.inl,bid.crl));
    }

    ismanip_ = false;
    dragger_->turnOn( false );
}


void RandomTrackDisplay::showManipulator( bool yn )
{
    if ( lockgeometry_ ) yn = false;

    if ( !yn ) dragger_->showFeedback( false );
    dragger_->turnOn( yn );
}
   

bool RandomTrackDisplay::isManipulatorShown() const
{ return false; /* track->isDraggerShown();*/ }


BufferString RandomTrackDisplay::getManipulationString() const
{
    BufferString str;
    int knotidx = getSelKnotIdx();
    if ( knotidx >= 0 )
    {
	BinID binid  = getManipKnotPos( knotidx );
	str = "Node "; str += knotidx;
	str += " Inl/Crl: ";
	str += binid.inl; str += "/"; str += binid.crl;
    }

    return str;
}
 

int RandomTrackDisplay::getColTabID( int attrib ) const
{
    return texture_->getColorTab( attrib ).id();
}


const TypeSet<float>* RandomTrackDisplay::getHistogram( int attrib ) const
{
    return texture_->getHistogram( attrib, texture_->currentVersion( attrib ) );}


void RandomTrackDisplay::knotMoved( CallBacker* cb )
{
    ismanip_ = true;
    mCBCapsuleUnpack(int,sel,cb);
    
    selknotidx_ = sel;
    knotmoving_.trigger();
}


void RandomTrackDisplay::knotNrChanged( CallBacker* )
{
    ismanip_ = true;
    for ( int idx=0; idx<cache_.size(); idx++ )
    {
	if ( !cache_[idx] || !cache_[idx]->size() )
	    continue;

	setData( idx, *cache_[idx] );
    }
}


bool RandomTrackDisplay::checkPosition( const BinID& binid ) const
{
    const HorSampling& hs = SI().sampling(true).hrg;
    if ( !hs.includes(binid) )
	return false;

    BinID snapped( binid );
    SI().snap( snapped, BinID(0,0) );
    if ( snapped != binid )
	return false;

    for ( int idx=0; idx<nrKnots(); idx++ )
	if ( getKnotPos(idx) == binid )
	    return false;

    return true;
}


BinID RandomTrackDisplay::snapPosition( const BinID& binid_ ) const
{
    BinID binid( binid_ );
    const HorSampling& hs = SI().sampling(true).hrg;
    if ( binid.inl < hs.start.inl ) binid.inl = hs.start.inl;
    if ( binid.inl > hs.stop.inl ) binid.inl = hs.stop.inl;
    if ( binid.crl < hs.start.crl ) binid.crl = hs.start.crl;
    if ( binid.crl > hs.stop.crl ) binid.crl = hs.stop.crl;

    SI().snap( binid, BinID(0,0) );
    return binid;
}


SurveyObject::AttribFormat RandomTrackDisplay::getAttributeFormat() const
{ return SurveyObject::Traces; }


#define mFindTrc(inladd,crladd) \
    if ( idx<0 ) \
    { \
	BinID bid( binid.inl + step.inl * (inladd),\
		   binid.crl + step.crl * (crladd) );\
	idx = bids.indexOf( bid ); \
    }
Coord3 RandomTrackDisplay::getNormal( const Coord3& pos ) const
{
    const mVisTrans* utm2display = scene_->getUTM2DisplayTransform();
    Coord3 xytpos = utm2display->transformBack( pos );
    BinID binid = SI().transform( Coord(xytpos.x,xytpos.y) );

    TypeSet<BinID> bids;
    TypeSet<int> segments;
    getDataTraceBids( bids, &segments );
    int idx = bids.indexOf(binid);
    if ( idx==-1 )
    {
	const BinID step( SI().inlStep(), SI().crlStep() );
	mFindTrc(1,0) mFindTrc(-1,0) mFindTrc(0,1) mFindTrc(0,-1)
	if ( idx==-1 )
	{
	    mFindTrc(1,1) mFindTrc(-1,1) mFindTrc(1,-1) mFindTrc(-1,-1)
	}

	if ( idx<0 )
	    return Coord3::udf();
    }

    const visBase::Coordinates* coords = triangles_->getCoordinates();
    const Coord pos0 = coords->getPos( segments[idx]*2 );
    const Coord pos1 = coords->getPos( segments[idx]*2+2 );
    const BinID bid0( mNINT(pos0.x), mNINT(pos0.y));
    const BinID bid1( mNINT(pos1.x), mNINT(pos1.y));

    const Coord dir = SI().transform(bid0)-SI().transform(bid1);
    const float dist = dir.abs();

    if ( dist<=mMIN(SI().inlDistance(),SI().crlDistance()) )
	return Coord3::udf();

    return Coord3( dir.y, -dir.x, 0 );
}

#undef mFindTrc


float RandomTrackDisplay::calcDist( const Coord3& pos ) const
{
    const mVisTrans* utm2display = scene_->getUTM2DisplayTransform();
    Coord3 xytpos = utm2display->transformBack( pos );
    BinID binid = SI().transform( Coord(xytpos.x,xytpos.y) );

    TypeSet<BinID> bids;
    getDataTraceBids( bids );
    if ( bids.indexOf(binid)==-1 )
	return mUdf(float);

    float zdiff = 0;
    const Interval<float> intv = getDataTraceRange();
    if ( xytpos.z < intv.start )
	zdiff = intv.start - xytpos.z;
    else if ( xytpos.z > intv.stop )
	zdiff = xytpos.z - intv.stop;

    return zdiff;
}


void RandomTrackDisplay::lockGeometry( bool yn )
{
    lockgeometry_ = yn;
    if ( yn ) showManipulator( false );
}


bool RandomTrackDisplay::isGeometryLocked() const
{ return lockgeometry_; }


SurveyObject* RandomTrackDisplay::duplicate() const
{
    RandomTrackDisplay* rtd = create();

    rtd->setDepthInterval( getDataTraceRange() );
    TypeSet<BinID> positions;
    for ( int idx=0; idx<nrKnots(); idx++ )
	positions += getKnotPos( idx );
    rtd->setKnotPositions( positions );

    rtd->lockGeometry( isGeometryLocked() );

    return rtd;
}


void RandomTrackDisplay::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visSurvey::MultiTextureSurveyObject::fillPar( par, saveids );

    const Interval<float> depthrg = getDataTraceRange();
    par.set( sKeyDepthInterval(), depthrg );

    const int nrknots = nrKnots();
    par.set( sKeyNrKnots(), nrknots );

    for ( int idx=0; idx<nrknots; idx++ )
    {
	BufferString key = sKeyKnotPrefix(); key += idx;
	par.set( key, getKnotPos(idx) );
    }

    par.set( sKey::Version, 3 );
    par.setYN( sKeyLockGeometry(), lockgeometry_ );
}


int RandomTrackDisplay::usePar( const IOPar& par )
{
    int version;
    if ( !par.get( sKey::Version, version ) )
	version = 2;

    if ( version==2 )
    {
	const int res =  visBase::VisualObjectImpl::usePar( par );
	if ( res != 1 ) return res;

	int nrattribs;
	if ( par.get(sKeyNrAttribs(),nrattribs) ) //version 2
	{
	    par.getYN( sKeyLockGeometry(), lockgeometry_ );
	    bool firstattrib = true;
	    for ( int attrib=0; attrib<nrattribs; attrib++ )
	    {
		BufferString key = sKeyAttribs();
		key += attrib;
		PtrMan<const IOPar> attribpar = par.subselect( key );
		if ( !attribpar )
		    continue;

		int coltabid = -1;
		if ( attribpar->get(sKeyColTabID(),coltabid) )
		{
		    visBase::DataObject* dataobj =
			visBase::DM().getObject(coltabid);
		    if ( !dataobj ) return 0;
		    mDynamicCastGet( const visBase::VisColorTab*, coltab,
			    	     dataobj );
		    if ( !coltab ) coltabid=-1;
		}

		if ( !firstattrib )
		    addAttrib();
		else
		    firstattrib = false;

		Attrib::SelSpec as;
		const int attribnr = nrAttribs()-1;
		as.usePar( *attribpar );
		setSelSpec( attribnr, as );

		if ( coltabid!=-1 )
		{
		    mDynamicCastGet( visBase::VisColorTab*, coltab,
		    visBase::DM().getObject(coltabid) );
		    texture_->setColorTab( attribnr, *coltab );
		}

		bool ison = true;
		attribpar->getYN( sKeyIsOn(), ison );
		texture_->enableTexture( attribnr, ison );
	    }
	}
	else //For old pars
	{
	    int trackid;
	    if ( !par.get(sKeyTrack(),trackid) ) return -1;
	    mDynamicCastGet(visBase::RandomTrack*,rt,
			    visBase::DM().getObject(trackid));
	    
	    if ( !rt )
		return 0;

	    rt->ref();

	    Attrib::SelSpec as;
	    as.usePar( par );
	    setSelSpec( 0, as );

	    texture_->setColorTab( 0, rt->getColorTab() );
	    rt->unRef();
	    setMaterial( rt->getMaterial() );
	    turnOn( rt->isOn() );
	}
    }
    else
    {
	par.getYN( sKeyLockGeometry(), lockgeometry_ );
	const int res =  visSurvey::MultiTextureSurveyObject::usePar( par );
	if ( res != 1 ) return res;
    }

    Interval<float> intv;
    if ( par.get( sKeyDepthInterval(), intv ) )
	setDepthInterval( intv );

    int nrknots = 0;
    par.get( sKeyNrKnots(), nrknots );

    BufferString key; BinID pos;
    for ( int idx=0; idx<nrknots; idx++ )
    {
	key = sKeyKnotPrefix(); key += idx;
	par.get( key, pos );
	if ( idx < 2 )
	    setKnotPos( idx, pos );
	else
	    addKnot( pos );
    }

    if ( version<3 )
	useSOPar( par );

    return 1;
}


void RandomTrackDisplay::getMousePosInfo( const visBase::EventInfo&,
					  const Coord3& pos, BufferString& val,
					  BufferString& info ) const
{
    info = name();
    getValueString( pos, val );
}


#define mFindTrc(inladd,crladd) \
    if ( trcidx < 0 ) \
    { \
	bid.inl = reqbid.inl + step.inl * (inladd); \
	bid.crl = reqbid.crl + step.crl * (crladd); \
	trcidx = cache_[attrib]->find( bid ); \
    }


bool RandomTrackDisplay::getCacheValue( int attrib,int version,
					const Coord3& pos,float& val ) const
{
    if ( !cache_[attrib] )
	return false;

    BinID reqbid( SI().transform(pos) );
    int trcidx = cache_[attrib]->find( reqbid );
    if ( trcidx<0 )
    {
	const BinID step( SI().inlStep(), SI().crlStep() );
	BinID bid;
	mFindTrc(1,0) mFindTrc(-1,0) mFindTrc(0,1) mFindTrc(0,-1)
	if ( trcidx<0 )
	{
	    mFindTrc(1,1) mFindTrc(-1,1) mFindTrc(1,-1) mFindTrc(-1,-1)
	}

    }

    if ( trcidx<0 ) return false;

    const SeisTrc& trc = *cache_[attrib]->get( trcidx );
    const int sampidx = trc.nearestSample( pos.z );
    if ( sampidx>=0 && sampidx<trc.size() )
    {
	val = trc.get( sampidx, 0 );
	return true;
    }

    return false;
}

#undef mFindTrc


void RandomTrackDisplay::addCache()
{
    cache_.allowNull();
    cache_ += 0;
    datapackids_ += -1;
}


void RandomTrackDisplay::removeCache( int attrib )
{
    delete cache_[attrib];
    cache_.remove( attrib );

    DPM( DataPackMgr::FlatID() ).release( datapackids_[attrib] );
    datapackids_.remove( attrib );
}


void RandomTrackDisplay::swapCache( int a0, int a1 )
{
    cache_.swap( a0, a1 );
    datapackids_.swap( a0, a1 );
}


void RandomTrackDisplay::emptyCache( int attrib )
{
    if ( cache_[attrib] ) delete cache_[attrib];
	cache_.replace( attrib, 0 );

    DPM( DataPackMgr::FlatID() ).release( datapackids_[attrib] );
    datapackids_[attrib] = DataPack::cNoID();
}


bool RandomTrackDisplay::hasCache( int attrib ) const
{
    return cache_[attrib];
}

} // namespace visSurvey
