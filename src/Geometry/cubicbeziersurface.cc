/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
-*/

static const char* rcsID = "$Id: cubicbeziersurface.cc,v 1.6 2005-02-20 13:42:26 cvskris Exp $";

#include "cubicbeziersurface.h"

#include "parametricsurfaceimpl.h"

#include "arrayndimpl.h"
#include "cubicbeziercurve.h"
#include "errh.h"
#include "rowcol.h"
#include "survinfo.h"

namespace Geometry
{


CubicBezierSurface::CubicBezierSurface(const Coord3& p0, const Coord3& p1,
				    bool posonsamerow,
				    const RCol& origo_, const RCol& step_ )
    : ParametricSurface( origo_, step_ )
    , positions( posonsamerow
	    ? new Array2DImpl<Coord3>(1,2)
	    : new Array2DImpl<Coord3>(2,1) )
    , rowdirections( 0 )
    , coldirections( 0 )
    , directioninfluence( 1.0/3 )
{
     if ( !p0.isDefined() || !p1.isDefined() )
	 pErrMsg("Cannot start surface with undefined positions" );
     else
     {
	 positions->set( 0, 0, p0 );
	 positions->set( posonsamerow ?  0 : 1, posonsamerow ? 1 : 0, p1 );
     }
}


CubicBezierSurface::CubicBezierSurface( const CubicBezierSurface& b )
    : ParametricSurface( b.origo, b.step )
    , positions( new Array2DImpl<Coord3>(*b.positions) )
    , rowdirections( b.rowdirections
	    ? new Array2DImpl<Coord3>(*b.rowdirections) : 0 )
    , coldirections( b.coldirections
	    ? new Array2DImpl<Coord3>(*b.coldirections) : 0 )
{ }


CubicBezierSurface::~CubicBezierSurface()
{
    delete rowdirections;
    delete coldirections;
    delete positions;
}


CubicBezierSurface* CubicBezierSurface::clone() const
{ return new CubicBezierSurface( *this ); }


IntervalND<float> CubicBezierSurface::boundingBox(bool approx) const
{
    if ( approx ) return ParametricSurface::boundingBox(approx);

    IntervalND<float> bbox(3);
    const StepInterval<int> rowrange = rowRange();
    const StepInterval<int> colrange = rowRange();

    for ( RowCol rc(rowrange.start,0); rc.row<=rowrange.stop;
	  rc.row+=rowrange.step )
    {
	for ( rc.col = colrange.start; rc.col<=colrange.stop;
	      rc.col+=colrange.step )
	{
	    for ( RowCol relrc(-1,-1); relrc.row<=1; relrc.row++ )
	    {
		for ( relrc.col=-1; relrc.col<=1; relrc.col++ )
		{
		    const Coord3 vertex = getBezierVertex( rc, relrc );
		    if ( !vertex.isDefined() )
			continue;

		    if ( !bbox.isSet() ) bbox.setRange(vertex);
		    else bbox.include(vertex);
		}
	    }
	}
    }

    return bbox;
}


Coord3 CubicBezierSurface::computePosition( const Coord& params ) const
{
    const StepInterval<int> rowrange = rowRange();
    const StepInterval<int> colrange = rowRange();

    int prevrowidx = rowrange.getIndex(params.x);
    if ( prevrowidx<0 || prevrowidx>nrRows()-1 )
	return Coord3::udf();
    else if ( prevrowidx==nrRows()-1 )
    {
	if ( rowrange.atIndex(prevrowidx)<=params.x )
	    prevrowidx--;
	else
	    return Coord3::udf();
    }

    const int prevrow = rowrange.atIndex(prevrowidx);

    int prevcolidx = colrange.getIndex(params.y);
    if ( prevcolidx<0 || prevcolidx>nrCols()-1 )
	return Coord3::udf();
    else if ( prevcolidx==nrCols()-1 )
    {
	if ( colrange.atIndex(prevcolidx)<=params.y )
	    prevcolidx--;
	else
	    return Coord3::udf();
    }

    const int prevcol = colrange.atIndex(prevcolidx);

    const float u = (params.x-prevrow)/rowrange.step;
    const float v = (params.y-prevcol)/colrange.step;

    RowCol rc0(prevrow,prevcol);
    RowCol rc1(prevrow,prevcol+colrange.step);
    const Coord3 row0 = cubicDeCasteljau( getKnot(rc0,true),
					  getBezierVertex( rc0,RowCol(0,1)),
					  getBezierVertex( rc1,RowCol(0,-1)),
					  getKnot(rc1,true),
					  v );

    const Coord3 row1 = cubicDeCasteljau( getBezierVertex( rc0,RowCol(1,0)),
					  getBezierVertex( rc0,RowCol(1,1)),
					  getBezierVertex( rc1,RowCol(1,-1)),
					  getBezierVertex( rc1,RowCol(1,0)),
					  v );

    rc0.row += rowrange.step;
    rc1.row += rowrange.step;

    const Coord3 row2 = cubicDeCasteljau( getBezierVertex( rc0,RowCol(-1,0)),
					  getBezierVertex( rc0,RowCol(-1,1)),
					  getBezierVertex( rc1,RowCol(-1,-1)),
					  getBezierVertex( rc1,RowCol(-1,0)),
					  v );

    const Coord3 row3 = cubicDeCasteljau( getKnot( rc0,true ),
					  getBezierVertex( rc0,RowCol(0,1)),
					  getBezierVertex( rc1,RowCol(0,-1)),
					  getKnot( rc1,true ),
					  v );

    return cubicDeCasteljau( row0, row1, row2, row3, u );
}


Coord3 CubicBezierSurface::computeNormal( const Coord& ) const
{
    pErrMsg( "Not impl" );
    return Coord3::udf();
}


Coord3 CubicBezierSurface::getBezierVertex( const RCol& knot,
	const RCol& relpos ) const
{
    Coord3 pos = getKnot( knot );
    if ( !pos.isDefined() ) return pos;

    if ( relpos.r() )
    {
	const Coord3 rowdir = getRowDirection( knot, true );
	if ( !rowdir.isDefined() ) return rowdir;

	if ( relpos.r()>0 )
	    pos += rowdir.normalize()*directioninfluence;
	else
	    pos -= rowdir.normalize()*directioninfluence;
    }

    if ( relpos.c() )
    {
	const Coord3 coldir = getColDirection( knot, true );
	if ( !coldir.isDefined() ) return coldir;

	if ( relpos.c()>0 )
	    pos += coldir.normalize()*directioninfluence;
	else 
	    pos -= coldir.normalize()*directioninfluence;
    }

    return pos;
}


bool CubicBezierSurface::insertRow(int row)
{
    mInsertStart( rowidx, row, nrRows() );

    TypeSet<GeomPosID> movedpos;
    for ( int idx=rowidx; idx<curnrrows; idx++ )
    {
	const int currow = origo.row+idx*step.row;
	for ( int idy=0; idy<curnrcols; idy++ )
	    movedpos += RowCol(currow,origo.col+idy*step.col).getSerialized();
    }

    TypeSet<GeomPosID> addedpos;
    const int newrow = origo.row+curnrrows*step.row;
    for ( int idy=0; idy<curnrcols; idy++ )
	addedpos += RowCol(newrow,origo.col+idy*step.col).getSerialized();

    mCloneRowVariable( Coord3, positions, computePosition(param), Coord3::udf())
    mCloneRowVariable( Coord3, rowdirections, Coord3::udf(), Coord3::udf() )
    mCloneRowVariable( Coord3, coldirections, Coord3::udf(), Coord3::udf() )

    triggerNrPosCh( addedpos );
    triggerMovement( movedpos );
    return true;
}


bool CubicBezierSurface::insertColumn(int col)
{
    mInsertStart( colidx, col, nrCols() );
    TypeSet<GeomPosID> movedpos;
    for ( int idx=colidx; idx<curnrcols; idx++ )
    {
	const int curcol = origo.col+idx*step.col;
	for ( int idy=0; idy<curnrrows; idy++ )
	    movedpos += RowCol(origo.row+idy*step.row,curcol).getSerialized();
    }

    TypeSet<GeomPosID> addedpos;
    const int newcol = origo.col+curnrcols*step.col;
    for ( int idy=0; idy<curnrrows; idy++ )
	addedpos += RowCol(origo.row+idy*step.row,newcol).getSerialized();

    mCloneColVariable( Coord3, positions, computePosition(param), Coord3::udf())
    mCloneColVariable( Coord3, rowdirections, Coord3::udf(), Coord3::udf() )
    mCloneColVariable( Coord3, coldirections, Coord3::udf(), Coord3::udf() )

    triggerNrPosCh( addedpos );
    triggerMovement( movedpos );
    return true;
}


bool CubicBezierSurface::removeRow( int row )
{
    pErrMsg( "not implemented ");
    return true;
}


bool CubicBezierSurface::removeColumn( int col )
{
    pErrMsg( "not implemented ");
    return true;
}


Coord3 CubicBezierSurface::getKnot( const RCol& rc, bool estimateifundef ) const
{
    const int index = getIndex(rc);
    if ( index==-1 ) return Coord3::udf();

    const Coord3* data = positions->getData();

    if ( estimateifundef && !data[index].isDefined() )
    {
	RowCol neighborrc( rc );
	neighborrc.row = rc.r()-step.row;
	Coord3 pos = getKnot( neighborrc, false );
	if ( pos.isDefined() )
	{
	    const Coord3 dir = getRowDirection( neighborrc, true );
	    if ( dir.isDefined() )
		return pos+dir;
	}

	neighborrc.row = rc.r()+step.row;
	pos = getKnot( neighborrc, false );
	if ( pos.isDefined() )
	{
	    const Coord3 dir = getRowDirection( neighborrc, true );
	    if ( dir.isDefined() )
		return pos-dir;
	}

	neighborrc.row = rc.r();
	neighborrc.col = rc.c()-step.col;
	pos = getKnot( neighborrc, false );
	if ( pos.isDefined() )
	{
	    const Coord3 dir = getColDirection( neighborrc, true );
	    if ( dir.isDefined() )
		return pos+dir;
	}

	neighborrc.col = rc.c()+step.col;
	pos = getKnot( neighborrc, false );
	if ( pos.isDefined() )
	{
	    const Coord3 dir = getColDirection( neighborrc, true );
	    if ( dir.isDefined() )
		return pos-dir;
	}
    }

    return data[index];
}

#define mGetDirectionImpl( dirptr, compfunc ) \
    if ( dirptr ) \
    { \
	const int index = getIndex(rc); \
	if ( index!=-1 ) \
	{ \
	    const Coord3& dir = dirptr->getData()[index]; \
	    if ( dir.isDefined() ) \
		return dir; \
	} \
    } \
 \
    return computeifudf ? compfunc(rc) : Coord3::udf()


Coord3 CubicBezierSurface::getRowDirection( const RCol& rc,
				    bool computeifudf ) const
{
    mGetDirectionImpl( rowdirections, computeRowDirection);
}



Coord3 CubicBezierSurface::getColDirection( const RCol& rc,
				    bool computeifudf ) const
{
    mGetDirectionImpl( coldirections, computeColDirection);
}


float CubicBezierSurface::directionInfluence() const
{ return directioninfluence; }


void CubicBezierSurface::setDirectionInfluence(float ndi)
{
    directioninfluence = ndi;
    triggerMovement();
}


bool CubicBezierSurface::checkSelfIntersection( const RCol& ownrc ) const
{
    const StepInterval<int> rowrange = rowRange();
    const StepInterval<int> colrange = colRange();

    TypeSet<RowCol> affectedrcs;
    RowCol affectedrc;
    for ( RowCol rc(-2,-2); rc.row<=1; rc.row++ )
    {
	affectedrc.row = ownrc.r()+rc.row*step.row;
	if ( !rowrange.includes(affectedrc.row) )
	    continue;

	for ( rc.col=-2; rc.col<=1; rc.col++ )
	{
	    affectedrc.col = ownrc.c()+rc.col*step.col;
	    if ( !colrange.includes(affectedrc.col) )
		continue;

	    affectedrcs += affectedrc;
	}
    }


    for ( int idx=0; idx<affectedrcs.size(); idx++ )
    {
	affectedrc = affectedrcs[idx];

	const IntervalND<float> ownbbox = boundingBox(affectedrc, false );
	if ( !ownbbox.isSet() ) return false;

	for ( RowCol rc(rowrange.start,0); rc.row<=rowrange.stop;
	      rc.row+=rowrange.step )
	{
	    for ( rc.col = colrange.start; rc.col<=colrange.stop;
		  rc.col+=colrange.step )
	    {
		if ( rc==affectedrc ) continue;
		if ( abs(rc.row-affectedrc.r())<=step.row &&
		     abs(rc.col-affectedrc.c())<=step.col )
		{
		    //TODO
		    continue;
		}

		const IntervalND<float> bbox = boundingBox(rc,false);
		if ( !bbox.isSet() ) continue;

		if ( ownbbox.intersects(bbox) )
		    return true;
	    }
	}
    }

    return false;
}


#define mGetBBoxVertex( relrow, relcol ) \
vertex = getBezierVertex(currc, RowCol(0,0)); \
if ( vertex.isDefined() ) \
{ \
    if ( bbox.isSet() ) bbox.include(vertex); \
    else bbox.setRange(vertex); \
\
    if ( rowext ) \
    {\
	vertex = getBezierVertex(currc, RowCol(relrow,0)); \
	if ( vertex.isDefined() ) bbox.include(vertex); \
    }\
    if ( rowext && colext )\
    {\
	vertex = getBezierVertex(currc, RowCol(relrow,relcol)); \
	if ( vertex.isDefined() ) bbox.include(vertex); \
    }\
    if ( colext )\
    {\
	vertex = getBezierVertex(currc, RowCol(0,relcol)); \
	if ( vertex.isDefined() ) bbox.include(vertex); \
    }\
}


IntervalND<float> CubicBezierSurface::boundingBox( const RCol& rc,
						   bool ownvertices ) const
{
    IntervalND<float> bbox(3);

    if ( ownvertices )
    {
	for ( RowCol relrc(-1,-1); relrc.row<=1; relrc.row++ )
	{
	    for ( relrc.col=-1; relrc.col<=1; relrc.col++ )
	    {
		const Coord3 vertex = getBezierVertex( rc, relrc );
		if ( !vertex.isDefined() )
		    continue;

		if ( !bbox.isSet() ) bbox.setRange(vertex);
		else bbox.include(vertex);
	    }
	}
    }
    else
    {
	const bool rowext = rc.r()<origo.row+(nrRows()-1)*step.row;
	const bool colext = rc.c()<origo.col+(nrCols()-1)*step.col;
	if ( rowext || colext )
	{
	    Coord3 vertex;
	    RowCol currc(rc);
	    mGetBBoxVertex( 1, 1 );

	    currc.row += step.row;
	    if ( rowext )
	    { mGetBBoxVertex( -1, 1 ); }

	    currc.col += step.col;
	    if ( rowext && colext )
	    { mGetBBoxVertex( -1, -1 ); }

	    currc.row = rc.r();
	    if ( colext )
	    { mGetBBoxVertex( 1, -1 ); }
	}
    }

    return bbox;
}


#ifndef mEPS
#define mEPS 1e-10
#endif


#define mComputeDirImpl(_rowcol_) \
    const Coord3* ptr = positions->getData(); \
    int diff = 2*step._rowcol_; \
    int previndex = getIndex(prev); \
    if ( previndex!=-1 && !ptr[previndex].isDefined() ) previndex==-1; \
 \
    int nextindex = getIndex(next); \
    if ( nextindex!=-1 && !ptr[nextindex].isDefined() ) nextindex==-1; \
 \
    if ( previndex==-1 ) \
    { \
	previndex=getIndex(rc); \
	if ( previndex!=-1 && !ptr[previndex].isDefined() ) previndex==-1; \
	diff = step._rowcol_; \
    } \
    else if ( nextindex==-1) \
    { \
	nextindex=getIndex(rc); \
	if ( nextindex!=-1 && !ptr[nextindex].isDefined() ) nextindex==-1; \
	diff = step._rowcol_; \
    } \
 \
    if ( previndex==-1 || nextindex==-1 ) \
	return Coord3::udf(); \
 \
    const Coord3 prevcoord = ptr[previndex]; \
    const Coord3 nextcoord = ptr[nextindex]; \
    if ( prevcoord.sqDistance(nextcoord)<mEPS ) \
	return Coord3::udf(); \
 \
    return (nextcoord-prevcoord)*step._rowcol_/diff*directioninfluence;


Coord3 CubicBezierSurface::computeRowDirection( const RCol& rc ) const
{
    const int lastrow = origo.row + (nrRows()-1)*step.row;
    const RowCol prev( rc.r()==origo.row && circularRows() 
	    ? lastrow : rc.r()-step.row, rc.c() );
    const RowCol next( rc.r()==lastrow && circularRows() 
	    ? origo.row : rc.r()+step.row, rc.c() );

    mComputeDirImpl(row);
}


Coord3 CubicBezierSurface::computeColDirection( const RCol& rc ) const
{
    const int lastcol = origo.col + (nrCols()-1)*step.col;
    const RowCol prev( rc.r(), rc.c()==origo.col && circularCols() 
	    ? lastcol : rc.c()-step.col );
    const RowCol next( rc.r(), rc.c()==lastcol && circularCols() 
	    ? origo.col : rc.c()+step.col);

    mComputeDirImpl(col);
}


void CubicBezierSurface::_setKnot( int idx, const Coord3& np )
{ positions->getData()[idx] = np; }


int CubicBezierSurface::nrRows() const
{ return positions ? positions->info().getSize(rowDim()) : 0; }


int CubicBezierSurface::nrCols() const
{ return positions ? positions->info().getSize(colDim()) : 0; }

};

