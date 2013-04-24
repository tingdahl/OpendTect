/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : D. Zheng
 * DATE     : Feb 2013
-*/
static const char* rcsID mUsedVar = "$Id: vishortilescreatorandupdator.cc 28563 2013-04-16 18:05:13Z ding.zheng@dgbes.com $";


#include "vishortilescreatorandupdator.h"
#include "vishorizonsection.h"
#include "vishorizonsectiontile.h"
#include "vishordatahandler.h"
#include "vishorizonsectiondef.h"
#include "vishorthreadworks.h"
#include "vishorizontexturehandler.h"
#include "simpnumer.h"

#include "binidsurface.h"
#include "thread.h"
#include "ranges.h"
#include "task.h"

#include <osg/Switch>
#include <osgGeo/LayeredTexture>


using namespace visBase;


HorTilesCreatorAndUpdator::HorTilesCreatorAndUpdator(HorizonSection* horsection)
:   horsection_( horsection )
{}

HorTilesCreatorAndUpdator::~HorTilesCreatorAndUpdator()
{}

#define mGetRowColIdx()\
{\
    rc.row /= rrg.step; rc.col /= crg.step;\
    tilerowidx = rc.row/tilesidesize;\
    tilerow = rc.row%tilesidesize;\
    if ( tilerowidx==nrrowsz && !tilerow )\
    {\
	tilerowidx--;\
	tilerow = tilesidesize;\
    }\
    tilecolidx = rc.col/tilesidesize;\
    tilecol = rc.col%tilesidesize;\
    if ( tilecolidx==nrcolsz && !tilecol )\
    {\
	tilecolidx--;\
	tilecol = tilesidesize;\
    }\
}


#define mUpdatePositionInTile()\
{\
    if ( !tile ) \
    {\
	tile = createOneTile( tilerowidx, tilecolidx );\
	updatednewtiles += tile;\
    }\
	else if ( updatednewtiles.indexOf(tile)==-1 )\
	{\
	    for ( int res=0; res<=lowestresidx; res++ )\
	    tile->setAllNormalsInvalid( res, false );\
	    tile->setPos( tilerow, tilecol, pos );\
	    if ( desiredresolution!=-1 )\
	{\
	    addoldtile = true;\
	    if ( updatedoldtiles.indexOf(tile)==-1 )\
	    updatedoldtiles += tile;\
        }\
    }\
}


#define mUpdateNeigbors()\
{\
    for ( int rowidx=-1; rowidx<=1; rowidx++ ) \
    {\
	const int nbrow = tilerowidx+rowidx;\
	if ( nbrow<0 || nbrow>=nrrowsz ) continue;\
	    for ( int colidx=-1; colidx<=1; colidx++ )\
	    {\
		const int nbcol = tilecolidx+colidx;\
		if ( (!rowidx && !colidx) || nbcol<0 || nbcol>=nrcolsz )\
		   continue;\
		HorizonSectionTile* nbtile = \
			    tiles.get( nbrow, nbcol );\
		if ( !nbtile || updatednewtiles.indexOf(nbtile)!=-1)\
		   continue;\
		nbtile->setPos( tilerow-rowidx*tilesidesize,\
		tilecol-colidx*tilesidesize, pos );\
		if ( !addoldtile || rowidx+colidx>=0 || \
		     desiredresolution == cNoneResolution ||\
		     updatednewtiles.indexOf(nbtile)!=-1 )\
		   continue;\
		if ( (!tilecol && !rowidx && colidx==-1) || \
		    (!tilerow && rowidx==-1 && \
		    ((!tilecol && colidx==-1) || !colidx)) )\
		  updatedoldtiles += nbtile;\
    	}\
    }\
}


void HorTilesCreatorAndUpdator::updatePoints( const TypeSet<GeomPosID>* gpids,
				      TaskRunner* tr )
{
    if (!horsection_) return;

    mDefineRCRange( horsection_,-> );

    if ( rrg.width(false)<0 || crg.width(false)<0 )
	return;
    
    horsection_->tesselationlock_ = true;

    updateTileArray( rrg, crg );
    
    const int nrrowsz = horsection_->tiles_.info().getSize(0);
    const int nrcolsz = horsection_->tiles_.info().getSize(1);
    
    ObjectSet<HorizonSectionTile> updatedoldtiles;
    ObjectSet<HorizonSectionTile> updatednewtiles;

    const char lowestresidx = horsection_->lowestresidx_;
    const char desiredresolution = horsection_->desiredresolution_;
    const Array2DImpl<HorizonSectionTile*> tiles = horsection_->tiles_;

    for ( int idx=(*gpids).size()-1; idx>=0; idx-- )
    {
	int tilerow( 0 ), tilecol( 0 ), tilerowidx( 0 ), tilecolidx( 0 );
	const RowCol absrc = RowCol::fromInt64( (*gpids)[idx] );
	RowCol rc = absrc - horsection_->origin_; 
	const int tilesidesize = horsection_->tilesidesize_;
	mGetRowColIdx();

	/*If we already set work area and the position is out of the area,
	  we will skip the position. */
	if ( tilerowidx>=nrrowsz || tilecolidx>=nrcolsz )
	     continue;

	const Coord3 pos = horsection_->geometry_->getKnot(absrc,false);
	bool addoldtile = false;
	HorizonSectionTile* tile = 
	    horsection_->tiles_.get( tilerowidx, tilecolidx );
	mUpdatePositionInTile();
	mUpdateNeigbors();
    }

    horsection_->forceupdate_ =  false;

    HorizonSectionTilePosSetup task( updatednewtiles, *horsection_->geometry_,
		rrg, crg, horsection_->hordatahandler_->getZAxistransform(), 
		horsection_->nrcoordspertileside_, horsection_->lowestresidx_ );
    TaskRunner::execute( tr, task );

    //Only for fixed resolutions, which won't be tessellated at render.
    if ( updatedoldtiles.size() )
    {
	TypeSet<Threads::Work> work;
	for ( int idx=0; idx<updatedoldtiles.size(); idx++ )
	{
	    TileTesselator* tt =
		new TileTesselator( updatedoldtiles[idx], desiredresolution );
	    work += Threads::Work( *tt, true );
	}

	Threads::WorkManager::twm().addWork( work,
	       Threads::WorkManager::cDefaultQueueID() );
    }
    
    horsection_->forceupdate_ =  true;
    horsection_->tesselationlock_ = false;
}


void HorTilesCreatorAndUpdator::updateTileArray( const StepInterval<int>& rrg,
				      const StepInterval<int>& crg )
{
    const int rowsteps = horsection_->tilesidesize_ * rrg.step;
    const int colsteps = horsection_->tilesidesize_ * crg.step;
    const int oldrowsize = horsection_->tiles_.info().getSize(0);
    const int oldcolsize = horsection_->tiles_.info().getSize(1);
    int newrowsize = oldrowsize;
    int newcolsize = oldcolsize;
    int nrnewrowsbefore = 0;
    int nrnewcolsbefore = 0;

    int diff = horsection_->origin_.row - rrg.start;
    if ( diff>0 ) 
    {
	nrnewrowsbefore = diff/rowsteps + ( diff%rowsteps ? 1 : 0 );
	newrowsize += nrnewrowsbefore;
    }

    diff = rrg.stop - ( horsection_->origin_.row+oldrowsize*rowsteps );
    if ( diff>0 ) newrowsize += diff/rowsteps + ( diff%rowsteps ? 1 : 0 );

    diff = horsection_->origin_.col - crg.start;
    if ( diff>0 ) 
    {
	nrnewcolsbefore = diff/colsteps + ( diff%colsteps ? 1 : 0 );
	newcolsize += nrnewcolsbefore;
    }

    diff = crg.stop - ( horsection_->origin_.col+oldcolsize*colsteps );
    if ( diff>0 ) newcolsize += diff/colsteps + ( diff%colsteps ? 1 : 0 );

    if ( newrowsize==oldrowsize && newcolsize==oldcolsize )
	return;

    Array2DImpl<HorizonSectionTile*> newtiles( newrowsize, newcolsize );
    newtiles.setAll( 0 );

    for ( int rowidx=0; rowidx<oldrowsize; rowidx++ )
    {
	const int targetrow = rowidx+nrnewrowsbefore;
	for ( int colidx=0; colidx<oldcolsize; colidx++ )
	{
	    const int targetcol = colidx+nrnewcolsbefore;
	    newtiles.set( targetrow, targetcol, 
		horsection_->tiles_.get(rowidx,colidx) );
	}
    }

    horsection_->writeLock();
    horsection_->tiles_.copyFrom( newtiles );
    horsection_->origin_.row -= nrnewrowsbefore*rowsteps;
    horsection_->origin_.col -= nrnewcolsbefore*colsteps;
    horsection_->writeUnLock();
}


HorizonSectionTile* HorTilesCreatorAndUpdator::createOneTile( int tilerowidx, 
							  int tilecolidx )
{
    mDefineRCRange( horsection_,-> );

    const RowCol step( rrg.step, crg.step );
    const RowCol tileorigin( horsection_->origin_.row +  
		 tilerowidx*horsection_->tilesidesize_*step.row,
		 horsection_->origin_.col +
		 tilecolidx*horsection_->tilesidesize_*step.col );

    HorizonSectionTile* tile = new HorizonSectionTile(*horsection_, tileorigin);

    tile->setResolution( horsection_->desiredresolution_ );
    tile->useWireframe( horsection_->usewireframe_ );

    horsection_->writeLock();
    horsection_->tiles_.set( tilerowidx, tilecolidx, tile );
    horsection_->writeUnLock();

    for ( int rowidx=-1; rowidx<=1; rowidx++ )
    {
	const int neighborrow = tilerowidx+rowidx;
	if (neighborrow<0 || neighborrow>=horsection_->tiles_.info().getSize(0))
	    continue;

	for ( int colidx=-1; colidx<=1; colidx++ )
	{
	    if ( !colidx && !rowidx )
		continue;

	    const int neighborcol = tilecolidx+colidx;
	    if ( neighborcol<0 || 
		 neighborcol>=horsection_->tiles_.info().getSize(1) )
		continue;
	    HorizonSectionTile* neighbor = 
		horsection_->tiles_.get(neighborrow,neighborcol);

	    if ( !neighbor ) continue;

	    char pos;
	    if ( colidx==-1 ) 
		pos = rowidx==-1 ? LEFTUPTILE : 
		( !rowidx ? LEFTTILE : LEFTBOTTOMTILE );
	    else if ( colidx==0 ) 
		pos = rowidx==-1 ? UPTILE : 
		( !rowidx ? THISTILE : BOTTOMTILE );
	    else 
		pos = rowidx==-1 ? RIGHTUPTILE : 
		( !rowidx ? RIGHTTILE : RIGHTBOTTOMTILE );

	    tile->setNeighbor( pos, neighbor );

	    if ( colidx==1 ) 
		pos = rowidx==1 ? LEFTUPTILE : 
		( !rowidx ? LEFTTILE : LEFTBOTTOMTILE);
	    else if ( colidx==0 ) 
		pos = rowidx==1 ? UPTILE : 
		( !rowidx ? THISTILE : BOTTOMTILE);
	    else 
		pos = rowidx==1 ? RIGHTTILE : 
		( !rowidx ? RIGHTTILE : RIGHTBOTTOMTILE);

	    neighbor->setNeighbor( pos, tile );
	}
    }

    horsection_->writeLock();
    horsection_->osghorizon_->addChild( tile->osgswitchnode_ );
    horsection_->writeUnLock();

    return tile;
}


void HorTilesCreatorAndUpdator::createAllTiles( TaskRunner* tr )
{
    mDefineRCRange( horsection_,-> );

    if ( rrg.width(false)<0 || crg.width(false)<0 )
	return;

    horsection_->tesselationlock_ = true;
    horsection_->origin_ = RowCol( rrg.start, crg.start );
    const int nrrows = nrBlocks( rrg.nrSteps()+1, 
				 horsection_->nrcoordspertileside_, 1 );
    const int nrcols = nrBlocks( crg.nrSteps()+1, 
				 horsection_->nrcoordspertileside_, 1 );

    horsection_->writeLock();
    if ( !horsection_->tiles_.setSize( nrrows, nrcols ) )
    {
	horsection_->tesselationlock_ = false;
	horsection_->writeUnLock();
	return;
    }

    horsection_->tiles_.setAll( 0 );
    horsection_->writeUnLock();                                                 

    ObjectSet<HorizonSectionTile> newtiles;
    for ( int tilerowidx=0; tilerowidx<nrrows; tilerowidx++ )
    {
	for ( int tilecolidx=0; tilecolidx<nrcols; tilecolidx++ )
	{
	    newtiles += createOneTile(tilerowidx, tilecolidx);
	}
    }

    horsection_->forceupdate_ =  false;

    HorizonSectionTilePosSetup task( newtiles, *horsection_->geometry_, 
	rrg, crg, horsection_->hordatahandler_->getZAxistransform(), 
	horsection_->nrcoordspertileside_, horsection_->lowestresidx_ );
    TaskRunner::execute( tr, task );

    horsection_->forceupdate_ =  true;
    horsection_->tesselationlock_ = false;

}


void HorTilesCreatorAndUpdator::updateTilesAutoResolution( 
						  const osg::CullStack* cs )
{
    HorizonTileRenderPreparer task( *horsection_, cs, 
				    horsection_->desiredresolution_ );
    task.execute();

    const int tilesz = horsection_->tiles_.info().getTotalSz();
    if ( !tilesz ) return;

    HorizonSectionTile** tileptrs = horsection_->tiles_.getData();

    Threads::SpinLockLocker applyreslock ( spinlock_ ); // below process should be fast
    for ( int idx=0; idx<tilesz; idx++ )
    {
	if ( tileptrs[idx] )
	{
	    tileptrs[idx]->applyTesselation( 
		tileptrs[idx]->getActualResolution() );
	}
    }
}


void HorTilesCreatorAndUpdator::updateTilesPrimitiveSets()
{
    const int tilesz = horsection_->tiles_.info().getTotalSz();
    if ( !tilesz ) return;

    HorizonSectionTile** tileptrs = horsection_->tiles_.getData();
    Threads::MutexLocker updatemutex( updatelock_ ); // below process takes some time
    for ( int idx=0; idx<tilesz; idx++ )
    {
	if ( tileptrs[idx] )
	    tileptrs[idx]->updatePrimitiveSets();
    }
    horsection_->forceupdate_ = false;
}


void HorTilesCreatorAndUpdator::setFixedResolution( char res, TaskRunner* tr )
{
    const int tilesz = horsection_->tiles_.info().getTotalSz();
    if ( !tilesz ) return;

    HorizonSectionTile** tileptrs = horsection_->tiles_.getData();
    for ( int idx=0; idx<tilesz; idx++ )
	if ( tileptrs[idx] ) tileptrs[idx]->setResolution( res );

    if ( res==cNoneResolution )
	return;

    TypeSet<Threads::Work> work;
    for ( int idx=0; idx<tilesz; idx++ )
    {
	if ( !tileptrs[idx] )
	    continue;

	tileptrs[idx]->setActualResolution( res );
	work += Threads::Work(
	    *new TileTesselator( tileptrs[idx], res ), true );
    }

    Threads::WorkManager::twm().addWork( work,
	Threads::WorkManager::cDefaultQueueID() );
}
