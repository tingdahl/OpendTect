/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: emsurface.cc,v 1.56 2004-07-14 15:33:53 nanne Exp $";

#include "emsurface.h"
#include "emsurfaceiodata.h"
#include "emsurfacetr.h"
#include "emsurfauxdataio.h"

#include "arrayndimpl.h"
#include "cubesampling.h"
#include "emhingeline.h"
#include "emhistoryimpl.h"
#include "emmanager.h"
#include "geomgridsurface.h"
#include "geommeshsurface.h"
#include "grid.h"
#include "ioman.h"
#include "iopar.h"
#include "ioobj.h"
#include "linsolv.h"
#include "pca.h"
#include "toplist.h"
#include "ptrman.h"
#include "survinfo.h"
#include "settings.h"


static const char* sDbInfo = "DB Info";
static const char* sRange = "Range";
static const char* sValnms = "Value Names";
static const char* sPatches = "Patches";


void EM::SurfaceIOData::clear()
{
    dbinfo = "";
    deepErase(valnames);
    deepErase(patches);
}

void EM::SurfaceIOData::use( const EM::Surface& surf )
{
    clear();

    StepInterval<int> hrg;
    surf.getRange( hrg, true );
    rg.start.inl = hrg.start; rg.stop.inl = hrg.stop;
    rg.step.inl = hrg.step;
    surf.getRange( hrg, false );
    rg.start.crl = hrg.start; rg.stop.crl = hrg.stop;
    rg.step.crl = hrg.step;

    for ( int idx=0; idx<surf.nrPatches(); idx++ )
	patches += new BufferString( surf.patchName( surf.patchID(idx) ) );

    for ( int idx=0; idx<surf.nrAuxData(); idx++ )
	valnames += new BufferString( surf.auxDataName(idx) );
}


void EM::SurfaceIOData::fillPar( IOPar& iopar ) const
{
    iopar.set( sDbInfo, dbinfo );

    IOPar bidpar;
    rg.fillPar( bidpar );
    iopar.mergeComp( bidpar, sRange );

    IOPar valnmspar;
    valnames.fillPar( valnmspar );
    iopar.mergeComp( valnmspar, sValnms );

    IOPar patchpar;
    patches.fillPar( patchpar );
    iopar.mergeComp( patchpar, sPatches );
}


void EM::SurfaceIOData::usePar( const IOPar& iopar )
{
    iopar.get( sDbInfo, dbinfo );

    IOPar* bidpar = iopar.subselect(sRange);
    if ( bidpar ) rg.usePar( *bidpar );

    IOPar* valnmspar = iopar.subselect(sValnms);
    if ( valnmspar ) valnames.usePar( *valnmspar );

    IOPar* patchpar = iopar.subselect(sPatches);
    if ( patchpar ) patches.usePar( *patchpar );
}


void EM::SurfaceIODataSelection::setDefault()
{
    rg = sd.rg;
    selvalues.erase(); selpatches.erase();
    for ( int idx=0; idx<sd.valnames.size(); idx++ )
	selvalues += idx;
    for ( int idx=0; idx<sd.patches.size(); idx++ )
	selpatches += idx;
}


EM::Surface::Surface( EMManager& man, const EM::ObjectID& id_ )
    : EMObject( man, id_ )
    , step_( SI().getStep(true,false), SI().getStep(false,false) )
    , loadedstep( SI().getStep(true,false), SI().getStep(false,false) )
    , rowinterval(0)
    , colinterval(0)
    , patchchnotifier( this )
    , hingelinechange( this )
    , shift(0)
    , changed( 0 )
{
    auxdatanames.allowNull(true);
    auxdatainfo.allowNull(true);
    auxdata.allowNull(true);
    hingelines.allowNull();
    edgelines.allowNull();
}


EM::Surface::~Surface()
{
    cleanUp();
}


void EM::Surface::cleanUp()
{
    while ( nrPatches() ) removePatch(patchID(0), false);
    for ( int idx=0; idx<nrHingeLines(); idx++ )
	removeHingeLine(idx,false);

    delete rowinterval;
    delete colinterval;
    rowinterval = 0;
    colinterval = 0;
}


void EM::Surface::removeAuxData()
{
    deepErase( auxdatanames );
    deepErase( auxdatainfo );
    for ( int idx=0; idx<auxdata.size(); idx++ )
    {
	if ( !auxdata[idx] ) continue;
	deepEraseArr( *auxdata[idx] );
	delete auxdata[idx];
	auxdata.replace( 0, idx );
    }

    deepErase( auxdata );
}


bool EM::Surface::findClosestNodes(TopList<float,EM::PosID>& toplist,
				const Coord3& pos_,
				const MathFunction<float>* time2depthfunc) const
{
    const int nrpatches = nrPatches();
    for ( int patch=0; patch<nrpatches; patch++ )
	findClosestNodes( patchID(patch), toplist, pos_, time2depthfunc );

    return toplist.size();
}


bool EM::Surface::findClosestNodes(const PatchID& patchid,
				TopList<float,EM::PosID>& toplist,
				const Coord3& pos_,
				const MathFunction<float>* time2depthfunc) const
{
    toplist.setTop(false);

    //TODO Make faster impl
    Coord3 origpos = pos_;
    if ( time2depthfunc ) origpos.z = time2depthfunc->getValue( pos_.z );
    const int nrpatches = nrPatches();

    StepInterval<int> rowrange; StepInterval<int> colrange;
    getRange( rowrange, true ); getRange( colrange, false );

    RowCol rc;
    for ( rc.row=rowrange.start;rc.row<=rowrange.stop;rc.row+=rowrange.step)
    {
	for ( rc.col=colrange.start; rc.col<=colrange.stop;
						    rc.col+=colrange.step )
	{
	    if ( isDefined(patchid,rc) )
	    {
		Coord3 pos = getPos( patchid, rc );
		if ( time2depthfunc )
		    pos.z = time2depthfunc->getValue( pos.z );

		double dist = pos.distance( origpos );
		toplist.addValue( dist,
				  EM::PosID(id_,patchid,rowCol2SubID(rc)));
	    
	    }
	}
    }

    return toplist.size();
}


bool EM::Surface::findClosestMesh(EM::PosID& res, const Coord3& timepos,
			  const MathFunction<float>* time2depthfunc) const
{
    TopList<float, EM::PosID> closestnodes( 20, mUndefValue, false );
    if ( !findClosestNodes(closestnodes,timepos,time2depthfunc) )
	return false;

    const Coord3 pos = time2depthfunc
	? Coord3( timepos, time2depthfunc->getValue( timepos.z ) )
	: timepos;

    float mindist;
    bool isresset = false;
    const int nrnodes = closestnodes.size();
    for ( int idx=0; idx<nrnodes; idx++ )
    {
	EM::PosID pid = closestnodes.getAssociatedValue(idx);
	Coord3 c00, c10, c01, c11;
	bool c00def, c10def, c01def, c11def;
	getMeshCoords( pid, c00, c10, c01, c11,
			c00def, c10def, c01def, c11def, time2depthfunc );

	int nrvalidcoords = 0;
	float totaldist = 0;
	if ( c00def ) { nrvalidcoords++; totaldist+=c00.distance(pos);}
	if ( c10def ) { nrvalidcoords++; totaldist+=c10.distance(pos);}
	if ( c01def ) { nrvalidcoords++; totaldist+=c01.distance(pos);}
	if ( c11def ) { nrvalidcoords++; totaldist+=c11.distance(pos);}

	if ( nrvalidcoords<3 ) continue;

	totaldist /=nrvalidcoords;
	if ( !isresset || totaldist<mindist )
	{
	    res = pid;
	    mindist = totaldist;
	    isresset = true;
	}
    }

    return isresset;
}


bool EM::Surface::computeMeshNormal( Coord3& res, const EM::PosID& pid,
			     const MathFunction<float>* time2depthfunc ) const
{
    Coord3 c00, c10, c01, c11;
    bool c00def, c10def, c01def, c11def;
    getMeshCoords( pid, c00, c10, c01, c11,
	    	   c00def, c10def, c01def, c11def,
		   time2depthfunc );

    TypeSet<Coord3> normals;
    if ( c00def && c10def && c01def )
    {
	const Coord3 rowvec = c10-c00;
	const double rowveclen = rowvec.abs();
	if ( !mIsZero(rowveclen,mDefEps) )
	{
	    const Coord3 colvec = c01-c00;
	    const double colveclen = colvec.abs();
	    if ( !mIsZero(colveclen,mDefEps) )
		normals += rowvec.cross(colvec).normalize();
	}
    }

    if ( c10def && c00def && c11def )
    {
	const Coord3 rowvec = c10-c00;
	const double rowveclen = rowvec.abs();
	if ( !mIsZero(rowveclen,mDefEps) )
	{
	    const Coord3 colvec = c11-c10;
	    const double colveclen = colvec.abs();
	    if ( !mIsZero(colveclen,mDefEps) )
		normals += rowvec.cross(colvec).normalize();
	}
    }

    if ( c01def && c00def && c11def )
    {
	const Coord3 rowvec = c11-c01;
	const double rowveclen = rowvec.abs();
	if ( !mIsZero(rowveclen,mDefEps) )
	{
	    const Coord3 colvec = c01-c00;
	    const double colveclen = colvec.abs();
	    if ( !mIsZero(colveclen,mDefEps) )
		normals += rowvec.cross(colvec).normalize();
	}
    }

    if ( c11def && c10def && c01def )
    {
	const Coord3 rowvec = c11-c01;
	const double rowveclen = rowvec.abs();
	if ( !mIsZero(rowveclen,mDefEps) )
	{
	    const Coord3 colvec = c11-c10;
	    const double colveclen = colvec.abs();
	    if ( !mIsZero(colveclen,mDefEps) )
		normals += rowvec.cross(colvec).normalize();
	}
    }

    res = estimateAverageVector( normals, false, false );
    return res.isDefined();
}


bool EM::Surface::computeNormal( Coord3& res, const CubeSampling* cs,
			    const MathFunction<float>* time2depthfunc ) const 
{
    TypeSet<EM::PosID> nodes;
    if ( cs ) findPos(*cs, &nodes );
    else
    {
	for ( int idy=0; idy<nrPatches(); idy++ )
	{
	    const EM::PatchID patchid = patchID(idy);
	    const int nrpatches = nrPatches();

	    StepInterval<int> rowrange; getRange( patchid, rowrange, true );
	    StepInterval<int> colrange; getRange( patchid, colrange, false );

	    RowCol idx( rowrange.start, colrange.start );
	    for ( ; rowrange.includes( idx.row ); idx.row+=rowrange.step )
	    {
		for ( ; colrange.includes( idx.col ); idx.col+=colrange.step )
		{
		    if ( isDefined(patchid,idx) )
		    {
			nodes += EM::PosID(id(),patchid,rowCol2SubID(idx));
		    }
		}
	    }
	}
    }

    return computeNormal( res, nodes, time2depthfunc );
}


#define mComputeNormalFetchNode(nodeindex) \
if ( !fetched[nodeindex] ) \
{ \
    fetched[nodeindex]=true; \
    const Coord3 tpos = getPos(getNeighbor(node,dirs[nodeindex])); \
    if ( tpos.isDefined() )  \
    { \
	while ( coords.size()<=nodeindex ) \
	    coords += Coord3(mUndefValue,mUndefValue,mUndefValue); \
	coords[nodeindex] = Coord3(tpos,t2d ? t2d->getValue(tpos.z) : tpos.z); \
	defnodes[nodeindex] = true; \
    } \
}

bool EM::Surface::computeNormal( Coord3& res, const EM::PosID& node,
			    const MathFunction<float>* t2d ) const
{
    const Coord3 nodetpos = getPos(node);
    const bool defnode = nodetpos.isDefined();
    const Coord3 nodecoord(nodetpos,
	     		 t2d&&defnode?t2d->getValue(nodetpos.z):nodetpos.z);

    const TypeSet<RowCol>& dirs = RowCol::clockWiseSequence();
    BoolTypeSet defnodes(dirs.size(), false );
    BoolTypeSet fetched(dirs.size(), false );
    TypeSet<Coord3> coords;

    Coord3 rowvector;
    RowCol rowvecdir;
    bool validrowvector = false;
    Coord3 colvector;
    RowCol colvecdir;
    bool validcolvector = false;

    for ( int rowidx=0; rowidx<3; rowidx++ )
    {
	if ( validrowvector && validcolvector )
	    break;

	if ( !validrowvector )
	{
	    rowvecdir = RowCol(1,rowidx ? (rowidx==1?-1:1) : 0);
	    if ( validcolvector && rowvecdir==colvecdir )
		continue;

	    const int nextrowidx = dirs.indexOf(rowvecdir);
	    const int prevrowidx = dirs.indexOf(-rowvecdir);
	    mComputeNormalFetchNode(nextrowidx);
	    mComputeNormalFetchNode(prevrowidx);

	    if ( defnodes[nextrowidx] && defnodes[prevrowidx] )
	    {
		rowvector = coords[nextrowidx]-coords[prevrowidx];
		validrowvector = true;
	    }
	    else if ( defnode && defnodes[nextrowidx] )
	    {
		rowvector = coords[nextrowidx]-nodecoord;
		validrowvector = true;
	    }
	    else if ( defnode && defnodes[prevrowidx] )
	    {
		rowvector = nodecoord-coords[prevrowidx];
		validrowvector = true;
	    }

	    if ( validrowvector )
		rowvector=rowvector.normalize();
	}

	for ( int colidx=0; colidx<3; colidx++ )
	{
	    if ( !validcolvector )
	    {
		colvecdir = RowCol(colidx ? (colidx==1?-1:1) : 0, 1);
		if ( validrowvector && rowvecdir==colvecdir )
		    continue;
		const int nextcolidx = dirs.indexOf(colvecdir);
		const int prevcolidx = dirs.indexOf(-colvecdir);
		mComputeNormalFetchNode(nextcolidx);
		mComputeNormalFetchNode(prevcolidx);

		if ( defnodes[nextcolidx] && defnodes[prevcolidx] )
		{
		    colvector = coords[nextcolidx]-coords[prevcolidx];
		    validcolvector = true;
		}
		else if ( defnode && defnodes[nextcolidx] )
		{
		    colvector = coords[nextcolidx]-nodecoord;
		    validcolvector = true;
		}
		else if ( defnode && defnodes[prevcolidx] )
		{
		    colvector = nodecoord-coords[prevcolidx];
		    validcolvector = true;
		}

		if ( validcolvector )
		    colvector=colvector.normalize();
	    }
	}
    }

    if ( validcolvector && validrowvector )
    {
	res = rowvector.cross(colvector).normalize();
	return true;
    }

    static const int prevrowidx = dirs.indexOf(RowCol(-1,0));
    mComputeNormalFetchNode(prevrowidx);
    static const int nextrowidx = dirs.indexOf(RowCol(1,0));
    mComputeNormalFetchNode(nextrowidx);

    if ( !validcolvector && defnode && defnodes[prevrowidx] &&
	    defnodes[nextrowidx] )
    {
	const Coord3 prevvector = (coords[prevrowidx]-nodecoord).normalize();
	const Coord3 nextvector = (coords[nextrowidx]-nodecoord).normalize();
	const Coord3 average = prevvector+nextvector;
	const double len = average.abs();
	if ( !mIsZero(len,mDefEps) )
	{
	    res = average.normalize();
	    return true;
	}
    }

    static const int prevcolidx = dirs.indexOf(RowCol(0,-1));
    mComputeNormalFetchNode(prevcolidx);
    static const int nextcolidx = dirs.indexOf(RowCol(0,1));
    mComputeNormalFetchNode(nextcolidx);

    if ( !validrowvector && defnode && defnodes[prevcolidx] &&
	      defnodes[nextcolidx] )
    {
	const Coord3 prevvector = (coords[prevcolidx]-nodecoord).normalize();
	const Coord3 nextvector = (coords[nextcolidx]-nodecoord).normalize();
	const Coord3 average = prevvector+nextvector;
	const double len = average.abs();
	if ( !mIsZero(len,mDefEps) )
	{
	    res = average.normalize();
	    return true;
	}
    }

    return false;
}


bool EM::Surface::computeNormal( Coord3& res, const TypeSet<EM::PosID>& nodes,
			     const MathFunction<float>* time2depthfunc ) const
{
    TypeSet<Coord3> normals;
    const int nrnodes = nodes.size();
    for ( int idx=0; idx<nrnodes; idx++ )
    {
	const EM::PosID& node = nodes[idx];
	Coord3 normal;
	if ( computeNormal(normal,nodes[idx],time2depthfunc) )
	    normals += normal;
    }

    res = estimateAverageVector( normals, false, false );
    return res.isDefined();
}


float EM::Surface::normalDistance( const Coord3& timepos,
			     const MathFunction<float>* time2depthfunc,
			     Interval<float>* meshvariation ) const
{
    EM::PosID closestmesh(0,0,0);
    if ( !findClosestMesh(closestmesh,timepos,time2depthfunc) )
	return mUndefValue;

    Coord3 meshnormal;
    if ( !computeMeshNormal(meshnormal,closestmesh,time2depthfunc) )
	return mUndefValue;

    Coord3 c00, c10, c01, c11;
    bool c00def, c10def, c01def, c11def;
    getMeshCoords( closestmesh, c00, c10, c01, c11,
	    	   c00def, c10def, c01def, c11def,
		   time2depthfunc );

    Coord3 center(0,0,0);
    int nrvals = 0;
    if ( c00def ) {center+=c00; nrvals++;}
    if ( c10def ) {center+=c10; nrvals++;}
    if ( c01def ) {center+=c01; nrvals++;}
    if ( c11def ) {center+=c11; nrvals++;}
    center /= nrvals;

    const Plane3 plane( meshnormal, center, false );

    //Check how far the mesh's own coords are from the plane
    if ( meshvariation )
    {
	*meshvariation = Interval<float>(0,0);
	if ( c00def ) {meshvariation->include(plane.distanceToPoint(c00,true));}
	if ( c10def ) {meshvariation->include(plane.distanceToPoint(c10,true));}
	if ( c01def ) {meshvariation->include(plane.distanceToPoint(c01,true));}
	if ( c11def ) {meshvariation->include(plane.distanceToPoint(c11,true));}
    }

    const Coord3 pos = time2depthfunc
	? Coord3( timepos, time2depthfunc->getValue( timepos.z ) )
	: timepos;

    const Line3 line( pos, meshnormal );
    Coord3 intersection;
    plane.intersectWith( line, intersection );
    const Coord3 vector = pos-intersection;
    return meshnormal.dot( vector );
}


char EM::Surface::whichSide( const Coord3& timepos,
			     const MathFunction<float>* time2depthfunc,
			     float fuzzy ) const
{
    Interval<float> meshvariation;
    const float dist = normalDistance( timepos, time2depthfunc, &meshvariation);

    if ( dist>meshvariation.stop+fuzzy ) return 1;
    if ( dist<meshvariation.start-fuzzy ) return -1;
    return 0;
}


#define mGetNeigborCoord( coordname, defname, rowdiff, coldiff ) \
defname = false; \
for ( int idy=0; idy<nrnodealiases; idy++ ) \
{ \
    const EM::PosID& nodealias = nodealiases[idy]; \
    const EM::PatchID patchid = nodealias.patchID(); \
    const RowCol noderc = subID2RowCol(nodealias.subID()); \
    const RowCol neighborrc( noderc.row rowdiff, noderc.col coldiff ); \
    coordname = getPos(patchid, neighborrc); \
    defname = coordname.isDefined(); \
    if ( defname ) \
    { \
	if ( time2depthfunc ) \
	    coordname.z = time2depthfunc->getValue(coordname.z); \
	break; \
    } \
} \



void EM::Surface::getMeshCoords( const EM::PosID& pid,
	Coord3& c00, Coord3& c10, Coord3& c01, Coord3& c11,
	bool& c00def, bool& c10def, bool& c01def, bool& c11def,
	const MathFunction<float>* time2depthfunc ) const
{
    TypeSet<EM::PosID> nodealiases;
    getLinkedPos( pid, nodealiases );
    nodealiases += pid;
    const int nrnodealiases = nodealiases.size();

    c00 = getPos(pid);
    c00def = c00.isDefined();
    if ( c00def && time2depthfunc ) c00.z = time2depthfunc->getValue(c00.z);

    mGetNeigborCoord( c10, c10def, +step_.row, +0 );
    mGetNeigborCoord( c01, c01def, +0, +step_.col );
    mGetNeigborCoord( c11, c11def, +step_.row, +step_.col );
}


int EM::Surface::nrPatches() const
{
    return patchids.size();
}


EM::PatchID EM::Surface::patchID( int idx ) const
{
    return patchids[idx];
}


EM::PatchID EM::Surface::patchID( const char* nm ) const
{
    for ( int idx=0; idx<patchnames.size(); idx++ )
	if ( *patchnames[idx] == nm ) return patchids[idx];
    return -1;
}


const char* EM::Surface::patchName( const EM::PatchID& patchid ) const
{
    int idx = patchids.indexOf(patchid);
    const char* res = idx!=-1 ? patchnames[idx]->buf() : 0;
    return  res && *res ? res : 0;
}


bool EM::Surface::hasPatch( const EM::PatchID& patchid ) const
{
    return patchids.indexOf(patchid)!=-1;
}


EM::PatchID EM::Surface::addPatch( const char* nm, bool addtohistory )
{
    PatchID res = 0;
    while ( patchids.indexOf(res)!=-1 ) res++;

    addPatch( nm, res, addtohistory );
    return res;
}


bool EM::Surface::addPatch( const char* nm, PatchID patchid, bool addtohistory )
{
    if ( patchids.indexOf(patchid) != -1 ) return false;

    BufferString name;
    patchids += patchid;
    if ( nm && *nm )
	name = nm;
    else
	{ name = "["; name += patchid + 1; name += "]"; }

    patchnames += new BufferString(name);
    edgelines += 0;

    Geometry::MeshSurface* newsurf = createPatchSurface( patchid );
    surfaces += newsurf;

    for ( int idx=0; idx<nrAuxData(); idx++ )
    {
	if ( !auxdata[idx] )
	    continue;

	(*auxdata[idx]) += 0;
    }

    if ( addtohistory )
    {
	HistoryEvent* history = new SurfacePatchEvent( true, id(),
							patchid, name );
	manager.history().addEvent( history, 0, 0 );
    }

    patchchnotifier.trigger(patchid,this);
    changed = true;
    return true;
}


void EM::Surface::removePatch( EM::PatchID patchid, bool addtohistory )
{
    int idx=patchids.indexOf(patchid);
    if ( idx==-1 ) return;

    BufferString name = *patchnames[idx];

    delete surfaces[idx];
    surfaces.remove( idx );
    patchids.remove( idx );
    patchnames.remove( idx );
    delete edgelines[idx];
    edgelines.remove( idx );

    for ( int idy=0; idy<nrAuxData(); idy++ )
    {
	if ( !auxdata[idy] )
	    continue;

	delete (*auxdata[idy])[idx];
	auxdata[idy]->replace( 0, idx );
    }

    if ( addtohistory )
    {
	HistoryEvent* history = new SurfacePatchEvent( false, id(),
							patchid, name );
	manager.history().addEvent( history, 0, 0 );
    }

    patchchnotifier.trigger(patchid,this);
    changed = true;
}


EM::PatchID EM::Surface::clonePatch( EM::PatchID patchid )
{
    int patchidx = patchids.indexOf(patchid);
    if ( patchidx==-1 ) return -1;

    PatchID res = addPatch(0, true);
    StepInterval<int> rowrange;
    StepInterval<int> colrange;
    getRange( patchid, rowrange, true );
    if ( rowrange.width() )
	getRange( patchid, colrange, false );

    for ( int row=rowrange.start; row<=rowrange.stop; row+=step_.row )
    {
	for ( int col=colrange.start; col<=colrange.stop; col+=step_.col )
	{
	    const RowCol rc(row,col);
	    const Coord3 pos = getPos(patchid,rc);
	    if ( !pos.isDefined() )
		continue;

	    setPos(res,rc,pos,false, true);

	    const EM::PosID src(id(),patchid,rowCol2SubID(rc));
	    const EM::PosID dst(id(),res,rowCol2SubID(rc));
	    for ( int idy=0; idy<nrPosAttribs(); idy++ )
	    {
		const int attrib = posAttrib(idy);
		if ( isPosAttrib( src, attrib ) )
		    setPosAttrib( dst, attrib, true );
	    }
	}
    }

    if ( getEdgeLineSet( patchid, false ) )
    {
	EdgeLineSet* els = getEdgeLineSet(patchid,false)->clone();
	els->setSection( res );
	edgelines.replace(els, patchids.indexOf(res) );
    }

    return res;
}


bool EM::Surface::setPos( const PatchID& patch, const RowCol& surfrc,
				   const Coord3& pos, bool autoconnect,
				   bool addtohistory)
{
    RowCol geomrowcol;
    if ( !getMeshRowCol( surfrc, geomrowcol, patch ) )
	return false;

    int patchindex=patchids.indexOf(patch);
    if ( patchindex==-1 ) return false;

    const Geometry::PosID posid = Geometry::MeshSurface::getPosID(geomrowcol);
    Geometry::MeshSurface* surface = surfaces[patchindex];
    const Coord3 oldpos = surface->getMeshPos( geomrowcol );

    if ( addtohistory )
    {
	EM::PosID pid( id(), patch, rowCol2SubID(surfrc) );
	HistoryEvent* history = new SetPosHistoryEvent( oldpos, pid );
	manager.history().addEvent( history, 0, 0 );
    }


    if ( oldpos==pos ) return true;

    changed = true;

    TypeSet<EM::PosID> nodeonotherpatches;
    if ( autoconnect )
	findPos( geomrowcol, nodeonotherpatches );

    surface->setMeshPos( geomrowcol, pos );
    surface->setFillType( geomrowcol, Geometry::MeshSurface::Filled );

    if ( !pos.isDefined() )
	surface->shrink();

    if ( autoconnect )
    {
	for ( int idx=0; idx<nodeonotherpatches.size(); idx++ )
	{
	    const int patchsurfidx =
		patchids.indexOf(nodeonotherpatches[idx].patchID());
	    double otherz = surfaces[patchsurfidx]->getMeshPos(geomrowcol).z;
	    
	    if ( mIsEqual(otherz,pos.z,mDefEps) )
	    {
		if ( !surface->isLinked(posid, surfaces[patchsurfidx], posid ))
		{
		    surface->setLink(posid,surfaces[patchsurfidx],posid,true);
		    // Put to history?
		}
	    }
	}
    }

    poschnotifier.trigger( EM::PosID( id(), patch, rowCol2SubID(surfrc)), this);

    return true;
}


bool EM::Surface::setPos( const EM::PosID& posid, const Coord3& newpos,
			  bool addtohistory )
{
    if ( posid.objectID()!=id() ) return false;

    return setPos( posid.patchID(), subID2RowCol(posid.subID()),
	    	   newpos, false,addtohistory);
}


Coord3 EM::Surface::getPos( const EM::PosID& posid ) const
{
    return getPos( posid.patchID(), subID2RowCol(posid.subID()) );
}


Coord3 EM::Surface::getPos( const PatchID& patch, const RowCol& rc) const
{
    const int surfidx = patchids.indexOf( patch );
    RowCol geomnode;
    if ( !getMeshRowCol( rc, geomnode, patch ) )
	return Coord3( mUndefValue, mUndefValue, mUndefValue );

    return surfaces[surfidx]->getMeshPos( geomnode );
}


void EM::Surface::getPos( const RowCol& rc, TypeSet<Coord3>& crdset ) const
{
    const int nrsubsurf = nrPatches();
    for ( int surfidx=0; surfidx<nrsubsurf; surfidx++ )
    {
	Coord3 crd = getPos( patchID(surfidx), rc );
	if ( crd.isDefined() )
	    crdset += crd;
    }
}


bool EM::Surface::isDefined( const EM::PosID& posid ) const
{
    return isDefined( posid.patchID(), subID2RowCol(posid.subID()) );
}


bool EM::Surface::isDefined( const PatchID& patch, const RowCol& rc) const
{
    const int surfidx = patchids.indexOf( patch );
    RowCol geomnode;
    if ( !getMeshRowCol( rc, geomnode, patch ) )
	return false;

    return surfaces[surfidx]->isDefined( geomnode );
}


int EM::Surface::findPos( const RowCol& rowcol,
				  TypeSet<PosID>& res ) const
{
    TypeSet<Coord3> respos;
    const int nrsubsurf = nrPatches();
    for ( PatchID surface=0; surface<nrsubsurf; surface++ )
    {
	Geometry::MeshSurface* meshsurf = surfaces[surface];
	if ( !meshsurf->isDefined( rowcol ) )
	    continue;

	Coord3 pos = meshsurf->getMeshPos( rowcol );
	EM::SubID subid = rowCol2SubID( rowcol );

	for ( int idx=0; idx<res.size(); idx++ )
	{
	    if ( subid!=res[idx].subID() )
		continue;

	    if ( mIsEqual(respos[idx].z,pos.z,mDefEps) ) continue;

	    res += PosID(id(), patchID(surface), subid );
	    respos += pos;
	}
    }

    return res.size();
}


int EM::Surface::findPos( const EM::PatchID& patchid,
			  const Interval<float>& x, const Interval<float>& y,
			  const Interval<float>& z,
			  TypeSet<EM::PosID>* res ) const	
{
    int idx = patchids.indexOf(patchid);
    if ( idx<0 ) return 0;

    TypeSet<EM::PosID> posids;
    TypeSet<Geometry::PosID> nodes;
    surfaces[idx]->findPos( x.center(), y.center(), z.center(),
			    x.width(), y.width(), z.width(), nodes );

    const int nrnodes = nodes.size();
    for ( int idy=0; idy<nrnodes; idy++ )
    {
	const PatchID patch = patchids[idx];
	const EM::PosID posid( id(), patchid, getSurfSubID(nodes[idy],patchid));

	TypeSet<EM::PosID> clones;
	getLinkedPos( posid, clones );
	clones += posid;

	const int nrclones = clones.size();
	bool found = false;
	for ( int idz=0; idz<nrclones; idz++ )
	{
	    if ( posids.indexOf(clones[idz]) != -1 )
	    { found = true; break; }
	}

	if ( !found )
	    posids += posid;
    }

    if ( res ) res->append(posids);
    return posids.size();
}


int EM::Surface::findPos( const Interval<float>& x, const Interval<float>& y,
			  const Interval<float>& z,
			  TypeSet<EM::PosID>* res ) const	
{
    int sum = 0;
    const int nrpatches = nrPatches();
    for ( int idx=0; idx<nrpatches; idx++ )
	sum += findPos( patchID(idx), x, y, z, res );

    return sum;
}

    
int EM::Surface::findPos( const CubeSampling& cs,
			  TypeSet<EM::PosID>* res ) const
{
    Coord xypos = SI().transform(cs.hrg.start);
    Interval<float> xinterval( xypos.x, xypos.x );
    Interval<float> yinterval( xypos.y, xypos.y );

    xypos = SI().transform(cs.hrg.stop);
    xinterval.include( xypos.x );
    yinterval.include( xypos.y );

    xypos = SI().transform( BinID(cs.hrg.start.inl,cs.hrg.stop.crl) );
    xinterval.include( xypos.x );
    yinterval.include( xypos.y );

    xypos = SI().transform( BinID(cs.hrg.stop.inl,cs.hrg.start.crl) );
    xinterval.include( xypos.x );
    yinterval.include( xypos.y );

    TypeSet<EM::PosID> posids;
    findPos( xinterval, yinterval, cs.zrg, &posids );

    for ( int idx=0; idx<posids.size(); idx++ )
    {
	const EM::PosID& posid = posids[idx];
	const BinID nodebid = SI().transform(getPos(posid));

	if ( nodebid.inl<cs.hrg.start.inl || nodebid.inl>cs.hrg.stop.inl ||
	     nodebid.crl<cs.hrg.start.crl || nodebid.crl>cs.hrg.stop.crl )
	{
	    posids.removeFast( idx-- );
	    continue;
	}
    }

    if ( res ) res->append(posids);
    return posids.size();
}


EM::PosID EM::Surface::getNeighbor( const EM::PosID& posid,
				    const RowCol& dir ) const
{
    RowCol diff;
    if ( dir.row>0 ) diff.row = step_.row;
    if ( dir.row<0 ) diff.row = -step_.row;
    if ( dir.col>0 ) diff.col = step_.col;
    if ( dir.col<0 ) diff.col = -step_.col;
    
    TypeSet<EM::PosID> aliases;
    getLinkedPos( posid, aliases );
    aliases += posid;

    const int nraliases = aliases.size();
    for ( int idx=0; idx<nraliases; idx++ )
    {
	const RowCol ownrc = subID2RowCol(aliases[idx].subID());
	const RowCol neigborrc = ownrc+diff;
	if ( isDefined(aliases[idx].patchID(),neigborrc) )
	    return EM::PosID( id(), aliases[idx].patchID(),
		    	      rowCol2SubID(neigborrc));
    }

    const RowCol ownrc = subID2RowCol(posid.subID());
    const RowCol neigborrc = ownrc+diff;

    return EM::PosID( id(), posid.patchID(), rowCol2SubID(neigborrc));
}


int EM::Surface::getNeighbors( const EM::PosID& posid_, TypeSet<EM::PosID>* res,
				int maxradius, bool circle ) const
{
    ObjectSet< TypeSet<EM::PosID> > neigbors;
    const RowCol start = subID2RowCol(posid_.subID());
    neigbors += new TypeSet<EM::PosID>( 1, posid_ );

    for ( int idx=0; idx<neigbors.size(); idx++ )
    {
	for ( int idz=0; idz<neigbors[idx]->size(); idz++ )
	{
	    EM::PosID currentposid = (*neigbors[idx])[idz];
	    const RowCol rowcol = subID2RowCol(currentposid.subID());

	    for ( int row=-step_.row; row<=step_.row; row+=step_.row )
	    {
		for ( int col=-step_.col; col<=step_.col; col+=step_.col )
		{
		    if ( !row && !col ) continue;

		    const RowCol neighborrowcol(rowcol.row+row,rowcol.col+col);
		    const int drow =abs(neighborrowcol.row-start.row)/step_.row;
		    const int dcol =abs(neighborrowcol.col-start.col)/step_.col;

		    if ( drow>maxradius || dcol>maxradius )
			continue;

		    if ( circle && (drow*drow+dcol*dcol)> maxradius*maxradius)
			continue;

		    if ( !isDefined(currentposid.patchID(),neighborrowcol) )
			continue;
		   
		    const EM::PosID
			    neighborposid(currentposid.objectID(),
			    currentposid.patchID(),
			    rowCol2SubID(neighborrowcol) );

		    bool found = false;
		    for ( int idy=0; idy<neigbors.size(); idy++ )
		    {
			const TypeSet<EM::PosID>& posids=*neigbors[idy];
			if ( posids.indexOf(neighborposid)!=-1 )
			{
			    found = true;
			    break;
			}
		    }

		    if ( found )
			continue;

		    TypeSet<EM::PosID>& posids = *new TypeSet<EM::PosID>;
		    getLinkedPos( neighborposid, posids );
		    posids.insert( 0, neighborposid );

		    neigbors += &posids;

		}
	    }
	}
    }

    if ( res )
    {
	// Leave out the fist one, since it's the origin
	for ( int idx=1; idx<neigbors.size(); idx++ )
	{
	    (*res) += (*neigbors[idx])[0];
	}
    }

    const int size = neigbors.size();
    deepErase( neigbors );

    // Leave out the fist one, since it's the origin
    return size-1;
}


void EM::Surface::getLinkedPos( const EM::PosID& posid,
				TypeSet<EM::PosID>& res ) const
{
    if ( posid.objectID()!=id() )
        return; //TODO: Implement handling for this case

    const EM::SubID subid = posid.subID();
    const RowCol rowcol = subID2RowCol(subid);
    const Geometry::MeshSurface* ownmeshsurf = getSurface( posid.patchID() );
    if ( !ownmeshsurf ) return;

    const int nrsubsurf = nrPatches();
    for ( int surface=0; surface<nrsubsurf; surface++ )
    {
	Geometry::MeshSurface* meshsurf = surfaces[surface];
	if ( ownmeshsurf->isLinked( subid, meshsurf, subid ) )
	{
	    res += EM::PosID( id(),patchids[surface], subid );
	}
    }
}


bool EM::Surface::isLoaded() const
{
    return nrPatches();
}


RowCol EM::Surface::loadedStep() const
{
    return loadedstep;
}


RowCol EM::Surface::step() const
{
    return step_;
}


void EM::Surface::setTranslatorData( const RowCol& step__,
					const RowCol& loadedstep_,
					const RowCol& origo_,
					const Interval<int>* rowrange_,
					const Interval<int>* colrange_ )
{
    step_ = step__;
    loadedstep = loadedstep_;
    origos += origo_;
    delete rowinterval;
    delete colinterval;
    rowinterval = rowrange_ ? new Interval<int>( *rowrange_ ) : 0;
    colinterval = colrange_ ? new Interval<int>( *colrange_ ) : 0;
}


RowCol EM::Surface::subID2RowCol( const EM::SubID& subid )
{
    return longlong2rc(subid);
}


EM::SubID EM::Surface::rowCol2SubID( const RowCol& rc )
{
    return rc2longlong(rc);
}


bool EM::Surface::isFullResolution() const
{
    return loadedstep == step_;
}


int EM::Surface::nrAuxData() const
{
    return auxdatanames.size();
}


const char* EM::Surface::auxDataName( int dataidx ) const
{
    if ( nrAuxData() && auxdatanames[dataidx] )
	return *auxdatanames[dataidx];

    return 0;
}


void EM::Surface::setAuxDataName( int dataidx, const char* name )
{
    if ( auxdatanames[dataidx] )
	auxdatanames.replace( new BufferString(name), dataidx );
}


int EM::Surface::auxDataIndex( const char* nm ) const
{
    for ( int idx=0; idx<auxdatanames.size(); idx++ )
	if ( *auxdatanames[idx] == nm ) return idx;
    return -1;
}


int EM::Surface::addAuxData( const char* name )
{
    auxdatanames += new BufferString( name );
    ObjectSet<TypeSet<float> >* newauxdata = new ObjectSet<TypeSet<float> >;
    auxdata += newauxdata;
    newauxdata->allowNull(true);

    for ( int idx=0; idx<nrPatches(); idx++ )
	(*newauxdata) += 0;

    changed = true;
    return auxdatanames.size()-1;
}


void EM::Surface::removeAuxData( int dataidx )
{
    delete auxdatanames[dataidx];
    auxdatanames.replace( 0, dataidx );

    deepEraseArr( *auxdata[dataidx] );
    delete auxdata[dataidx];
    auxdata.replace( 0, dataidx );
    changed = true;
}


void EM::Surface::removeAllAuxdata()
{
    deepErase( auxdatanames );
    deepErase( auxdatainfo );
    for ( int idx=0; idx<auxdata.size(); idx++ )
    {
	if ( !auxdata[idx] ) continue;
	deepErase( *auxdata[idx] );
    }

    deepErase( auxdata );
    changed = true;
}


float EM::Surface::getAuxDataVal( int dataidx, const EM::PosID& posid ) const
{
    if ( !auxdata[dataidx] ) return mUndefValue;
    const int patchidx = patchids.indexOf( posid.patchID() );
    if ( patchidx==-1 ) return mUndefValue;

    const TypeSet<float>* patchauxdata = (*auxdata[dataidx])[patchidx];
    if ( !patchauxdata ) return mUndefValue;

    RowCol geomrc;
    getMeshRowCol( posid.subID(), geomrc, posid.patchID() );
    const int subidx = surfaces[patchidx]->indexOf( geomrc );
    if ( subidx==-1 ) return mUndefValue;
    return (*patchauxdata)[subidx];
}


void EM::Surface::setAuxDataVal(int dataidx,const EM::PosID& posid, float val)
{
    if ( !auxdata[dataidx] ) return;

    const int patchidx = patchids.indexOf( posid.patchID() );
    if ( patchidx==-1 ) return;

    RowCol geomrc; 
    getMeshRowCol( posid.subID(), geomrc, posid.patchID() );
    const int subidx = surfaces[patchidx]->indexOf( geomrc );
    if ( subidx==-1 ) return;

    TypeSet<float>* patchauxdata = (*auxdata[dataidx])[patchidx];
    if ( !patchauxdata )
    {
	const int sz = surfaces[patchidx]->size();
	auxdata[dataidx]->replace( new TypeSet<float>(sz,mUndefValue),patchidx);
	patchauxdata = (*auxdata[dataidx])[patchidx];
    }

    (*patchauxdata)[subidx] = val;
    changed = true;
}


bool EM::Surface::getMeshRowCol( const EM::SubID& subid, RowCol& meshrowcol, 
				 const PatchID& patchid ) const
{
    return getMeshRowCol( subID2RowCol(subid), meshrowcol, patchid );
}


bool EM::Surface::getMeshRowCol( const RowCol& emrowcol, RowCol& meshrowcol,
       				 const PatchID& patchid ) const
{
    const int idx = patchids.indexOf( patchid );
    RowCol origo = idx<origos.size() ? origos[idx] :
		   (origos.size() ? origos[0] : RowCol(0,0) );
    const RowCol relrowcol = emrowcol - origo;
    if ( relrowcol.row%loadedstep.row || relrowcol.col%loadedstep.col )
	return false;

    meshrowcol = relrowcol/loadedstep;
    return true;
}


EM::SubID EM::Surface::getSurfSubID( const RowCol& nodeid, 
				     const PatchID& patchid ) const
{
    const int idx = patchids.indexOf( patchid );
    RowCol origo = idx<origos.size() ? origos[idx] :
	(origos.size() ? origos[0] : RowCol(0,0) );
    return rowCol2SubID( origo+nodeid*loadedstep );
}


EM::SubID EM::Surface::getSurfSubID( const Geometry::PosID& gposid,
       				     const PatchID& patchid ) const
{
    const RowCol& nodeid = Geometry::MeshSurface::getMeshNode(gposid);
    return getSurfSubID( nodeid, patchid );
}



const Geometry::MeshSurface* EM::Surface::getSurface( PatchID patchid )const
{
    const int idx = patchids.indexOf( patchid );
    return idx==-1 ? 0 : surfaces[idx];
}


void EM::Surface::getRange( StepInterval<int>& rg, bool rowdir ) const
{
    const int nrpatches = nrPatches();
    for ( int idx=0; idx<nrpatches; idx++ )
    {
	const EM::PatchID patchid = patchID( idx );
	StepInterval<int> patchrg;
	getRange( patchID(idx), patchrg, rowdir );
	
	if ( !idx )
	    rg = patchrg;
	else
	{
	    rg.include( patchrg.start ); 
	    rg.include( patchrg.stop );
	}
    }
}


void EM::Surface::getRange( const EM::PatchID& patchid, StepInterval<int>& rg,
			    bool rowdir ) const
{
    const Geometry::MeshSurface& gsurf = *getSurface( patchid );
    if ( rowdir )
    {
	const RowCol firstrow(gsurf.firstRow(),0);
	const RowCol lastrow(gsurf.lastRow(),0);

	rg.start = subID2RowCol( getSurfSubID(firstrow,patchid)).row;
	rg.stop = subID2RowCol( getSurfSubID(lastrow,patchid)).row;
    }
    else
    {
	const Interval<int> colrg = gsurf.getColInterval();
	const RowCol firstrow(0,colrg.start);
	const RowCol lastrow(0,colrg.stop);

	rg.start = subID2RowCol( getSurfSubID(firstrow,patchid)).col;
	rg.stop = subID2RowCol( getSurfSubID(lastrow,patchid)).col;
    }

    rg.step = rowdir ? loadedStep().row : loadedStep().col;
}


int EM::Surface::addHingeLine(HingeLine* hl, bool addtohistory)
{
    const int res = hingelines.size();
    hingelines += hl;
    hingelinechange.trigger(res);
    changed = true;
    return res;
}


void EM::Surface::removeHingeLine(int idx, bool addtohistory)
{
    HingeLine* hl = hingelines[idx];
    hingelines.replace(0,idx);
    delete hl;
    changed = true;
    for ( int idy=hingelines.size()-1; idy>=0; idy-- )
    {
	if ( hingelines[idy] )
	    break;

	hingelines.remove(idy);
    }

    hingelinechange.trigger(idx);
}


EM::EdgeLineSet* EM::Surface::getEdgeLineSet( const EM::PatchID& segment,
					      bool create )
{
    const int patchsurfidx = patchids.indexOf(segment);
    if ( patchsurfidx==-1 ) return 0;

    if ( !edgelines[patchsurfidx] && create )
    {
	EM::EdgeLineSet* els = new EM::EdgeLineSet( *this, segment );
	if ( els->findLines() )
	    edgelines.replace( els, patchsurfidx );
	else
	    delete els;
    }

    return edgelines[patchsurfidx];
}


bool EM::Surface::isAtEdge( const EM::PosID& pid ) const
{
    if ( !isDefined(pid) ) return false;

    int nrneighbors = getNeighbors(pid,0,1,false);
    if ( nrneighbors == 6 )
    {
	const int patch = pid.patchID();
	RowCol center = subID2RowCol( pid.subID() );
	return !( isDefined( patch, center+step_*RowCol(0,1) ) &&
		  isDefined( patch, center+step_*RowCol(1,0) ) &&
		  isDefined( patch, center+step_*RowCol(0,-1) ) &&
		  isDefined( patch, center+step_*RowCol(-1,0) ) );
    }

   return nrneighbors != 8;
}


Executor* EM::Surface::loader( const EM::SurfaceIODataSelection* newsel,
       			       int attridx )
{
    PtrMan<IOObj> ioobj = IOM().get( multiID() );
    if ( !ioobj )
	{ errmsg = "Cannot find the horizon object"; return 0; }

    PtrMan<EMSurfaceTranslator> tr = 
			(EMSurfaceTranslator*)ioobj->getTranslator();
    if ( !tr || !tr->startRead(*ioobj) )
	{ errmsg = tr ? tr->errMsg() : "Cannot find Translator"; return 0; }

    EM::SurfaceIODataSelection& sel = tr->selections();
    if ( newsel )
    {
	sel.rg = newsel->rg;
	sel.selvalues = newsel->selvalues;
	sel.selpatches = newsel->selpatches;
    }
    else
	sel.selvalues.erase();

    if ( attridx < 0 )
    {
	Executor* exec = tr->reader( *this );
	errmsg = tr->errMsg();
	return exec;
    }

    StreamConn* conn =dynamic_cast<StreamConn*>(ioobj->getConn(Conn::Read));
    if ( !conn ) return 0;
    
    const char* attrnm = sel.sd.valnames[attridx]->buf();
    int gap = 0;
    for ( int idx=0; ; idx++ )
    {
	if ( gap > 50 ) return 0;
	BufferString fnm = 
	    EM::dgbSurfDataWriter::createHovName(conn->fileName(),idx);
	if ( File_isEmpty(fnm) ) { gap++; continue; }
	else gap = 0;

	EM::dgbSurfDataReader* rdr = new EM::dgbSurfDataReader(fnm);
	if ( strcmp(attrnm,rdr->dataName()) )
	{ delete rdr; continue; }

	rdr->setSurface( *this );
	return rdr;
    }

    return 0;
}


Executor* EM::Surface::saver( const EM::SurfaceIODataSelection* newsel,
       			      bool auxdata, const MultiID* key )
{
    const MultiID& mid = key && !(*key=="") ? *key : multiID();
    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj )
	{ errmsg = "Cannot find the horizon object"; return 0; }

    PtrMan<EMSurfaceTranslator> tr = 
			(EMSurfaceTranslator*)ioobj->getTranslator();
    if ( !tr || !tr->startWrite(*this) )
	{ errmsg = tr ? tr->errMsg() : "No Translator"; return 0; }

    EM::SurfaceIODataSelection& sel = tr->selections();
    if ( newsel )
    {
	sel.rg = newsel->rg;
	sel.selvalues = newsel->selvalues;
	sel.selpatches = newsel->selpatches;
    }

    if ( auxdata )
    {
	StreamConn* conn =dynamic_cast<StreamConn*>(ioobj->getConn(Conn::Read));
	if ( !conn ) return 0;

	BufferString fnm;
	int dataidx = sel.selvalues.size() ? sel.selvalues[0] : 0;
	if ( dataidx >=0 )
	{
	    fnm = EM::dgbSurfDataWriter::createHovName( conn->fileName(),
		    					dataidx );
	}
	else
	{
	    for ( int idx=0; ; idx++ )
	    {
		fnm =EM::dgbSurfDataWriter::createHovName(conn->fileName(),idx);
		if ( !File_exists(fnm) )
		    break;
	    }
	}

	bool binary = true;
	mSettUse(getYN,"dTect.Surface","Binary format",binary);
	Executor* exec = new EM::dgbSurfDataWriter(*this,0,0,binary,fnm);
	return exec;
    }
    else
    {
	Executor* exec = tr->writer(*ioobj);
	errmsg = tr->errMsg();
	return exec;
    }
}


/*


const char* EM::SurfaceRelation::cuttedsurfacestr = "Cutted surface";
const char* EM::SurfaceRelation::cuttedpatchstr = "Cutted patch";
const char* EM::SurfaceRelation::cuttingsurfacestr = "Cutting surface";
const char* EM::SurfaceRelation::positivesidestr = "Positive side";


EM::SurfaceRelation::SurfaceRelation()
    :  cuttedsurface( -1 )
    , cuttedpatch( -1 )
    , cuttingsurface( -1 )
    , positiveside( true )
    , hingeline( new EM::HingeLine )
{
}


BufferString EM::SurfaceRelation::getPositiveText() const
{
    BufferString res = " on the ";
    if ( EM::EMM().type(cuttingsurface)==EM::EMManager::Fault )
    {
	Coord3 normal;
	BufferString buff;
	mDynamicCastGet( EM::Surface*, surf,
			 EM::EMM().getObject(cuttingsurface) );
	if ( surf && surf->computeNormal(normal) &&
	     getDirectionStr(normal,buff))
	{
	    res += buff;
	    res += "ern";
	}
	else
	    res += "positive";
        res += " side";
    }
    else if ( EM::EMM().type(cuttingsurface)==EM::EMManager::Hor )
	res += "bottom";

    return res;
}


BufferString EM::SurfaceRelation::getNegativeText() const
{
    BufferString res = " on the ";
    if ( EM::EMM().type(cuttingsurface)==EM::EMManager::Fault )
    {
	Coord3 normal;
	BufferString buff;
	mDynamicCastGet( EM::Surface*, surf,
			 EM::EMM().getObject(cuttingsurface) );
	if ( surf && surf->computeNormal(normal) &&
	     getDirectionStr(-normal,buff))
	{
	    res += buff;
	    res += "ern";
	}
	else
	    res += "negative";
        res += " side";
    }
    else if ( EM::EMM().type(cuttingsurface)==EM::EMManager::Hor )
	res += "top";

    return res;
}


BufferString EM::Surfaceelation::getUserString() const
{
    mDynamicCastGet(EM::Surface*,ownsurf,EM::EMM().getObject(cuttedsurface));

    BufferString res = EM::EMM().name( cuttingsurface );
    res += " terminates Patch ";
    res += ownsurf->patchName(cuttedpatch);
    res += positiveside ? getPositiveText() : getNegativeText();
    return res;
}


*/
