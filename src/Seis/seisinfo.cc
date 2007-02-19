/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 28-2-1996
 * FUNCTION : Seismic trace informtaion
-*/

static const char* rcsID = "$Id: seisinfo.cc,v 1.35 2007-02-19 16:41:46 cvsbert Exp $";

#include "seisinfo.h"
#include "seistrc.h"
#include "susegy.h"
#include "posauxinfo.h"
#include "binidselimpl.h"
#include "survinfo.h"
#include "strmprov.h"
#include "strmdata.h"
#include "filegen.h"
#include "iopar.h"
#include "cubesampling.h"
#include "enums.h"
#include "envvars.h"
#include "seistype.h"
#include <math.h>
#include <timeser.h>
#include <float.h>
#include <iostream>

const char* SeisTrcInfo::sSamplingInfo = "Sampling information";
const char* SeisTrcInfo::sNrSamples = "Nr of samples";
const char* SeisPacketInfo::sBinIDs = "BinID range";
const char* SeisPacketInfo::sZRange = "Z range";


static BufferString getUsrInfo()
{
    BufferString bs;
    const char* envstr = GetEnvVar( "DTECT_SEIS_USRINFO" );
    if ( !envstr || !File_exists(envstr) ) return bs;

    StreamData sd = StreamProvider(envstr).makeIStream();
    if ( sd.usable() )
    {
	char buf[1024];
	while ( *sd.istrm )
	{
	    sd.istrm->getline( buf, 1024 );
	    if ( *(const char*)bs ) bs += "\n";
	    bs += buf;
	}
    }

    return bs;
}

BufferString SeisPacketInfo::defaultusrinfo = getUsrInfo();

class SeisEnum
{
public:
    typedef Seis::WaveType WaveType;
	    DeclareEnumUtils(WaveType)
    typedef Seis::DataType DataType;
	    DeclareEnumUtils(DataType)
};

DefineEnumNames(SeisEnum,WaveType,0,"Wave type")
{
	"P",
	"Sh",
	"Sv",
	"Other",
	0
};

DefineEnumNames(SeisEnum,DataType,0,"Data type")
{
	"Amplitude",
	"Dip",
	"Frequency",
	"Phase",
	"AVO Gradient",
	"Azimuth",
	"Classification",
	"Other",
	0
};

const char* Seis::nameOf( Seis::DataType dt )
{ return eString(SeisEnum::DataType,dt); }

bool Seis::isAngle( Seis::DataType dt )
{ return dt==Seis::Phase || dt==Seis::Azimuth; }

const char* Seis::nameOf( Seis::WaveType wt )
{ return eString(SeisEnum::WaveType,wt); }

Seis::WaveType Seis::waveTypeOf( const char* s )
{ return eEnum(SeisEnum::WaveType,s); }

Seis::DataType Seis::dataTypeOf( const char* s )
{ return eEnum(SeisEnum::DataType,s); }

const char** Seis::dataTypeNames()
{ return SeisEnum::DataTypeNames; }

const char** Seis::waveTypeNames()
{ return SeisEnum::WaveTypeNames; }
 

DefineEnumNames(SeisTrcInfo,Fld,1,"Header field") {
	"Trace number",
	"In-line",
	"Cross-line",
	"X-coordinate",
	"Y-coordinate",
	"Offset",
	"Azimuth",
	"Pick position",
	"Reference position",
	0
};


void SeisPacketInfo::clear()
{
    usrinfo = defaultusrinfo;
    nr = 0;
    SI().sampling(false).hrg.get( inlrg, crlrg );
    zrg = SI().zRange(false);
    inlrev = crlrev = false;
}


float SeisTrcInfo::defaultSampleInterval( bool forcetime )
{
    float defsr = SI().zStep();
    if ( SI().zIsTime() || !forcetime )
	return defsr;

    defsr /= SI().zInFeet() ? 5000 : 2000; // div by velocity
    int ival = (int)(defsr * 1000 + .5);
    return ival * 0.001;
}


double SeisTrcInfo::getValue( SeisTrcInfo::Fld fld ) const
{
    switch ( fld )
    {
    case Pick:		return pick;
    case RefPos:	return refpos;
    case CoordX:	return coord.x;
    case CoordY:	return coord.y;
    case BinIDInl:	return binid.inl;
    case BinIDCrl:	return binid.crl;
    case Offset:	return offset;
    case Azimuth:	return azimuth;
    default:		return nr;
    }
}


void SeisTrcInfo::getAxisCandidates( Seis::GeomType gt,
				     TypeSet<SeisTrcInfo::Fld>& flds )
{
    flds.erase();
    const bool is2d = Seis::is2D( gt );
    const bool isps = Seis::isPS( gt );

    if ( isps )
	{ flds += Offset; flds += Azimuth; }
    if ( is2d )
	flds += TrcNr;
    if ( !is2d )
	{ flds += BinIDInl; flds += BinIDCrl; }

    // Coordinates are always an option
    flds += CoordX; flds += CoordY;
}


int SeisTrcInfo::getDefaultAxisFld( Seis::GeomType gt,
				    const SeisTrcInfo* ti ) const
{
    const bool is2d = Seis::is2D( gt );
    const bool isps = Seis::isPS( gt );
    if ( !ti )
	return isps ? Offset : (is2d ? TrcNr : BinIDCrl);

    if ( isps && !mIsZero(ti->offset-offset,1e-4) )
	return Offset;
    if ( is2d && ti->nr != nr )
	return TrcNr;
    if ( !is2d && ti->binid.crl != binid.crl )
	return BinIDCrl;
    if ( !is2d && ti->binid.inl != binid.inl )
	return BinIDInl;

    // 'normal' doesn't apply, try coordinates
    return mIsZero(ti->coord.x-coord.x,.1) ? CoordY : CoordX;
}


#define mIOIOPar(fn,fld,memb) iopar.fn( FldNames[(int)fld], memb )

void SeisTrcInfo::getInterestingFlds( Seis::GeomType gt, IOPar& iopar ) const
{
    const bool is2d = Seis::is2D( gt );
    const bool isps = Seis::isPS( gt );

    if ( isps )
    {
	mIOIOPar( set, Offset, offset );
	mIOIOPar( set, Azimuth, azimuth );
    }
    if ( is2d )
	mIOIOPar( set, TrcNr, nr );

    if ( !is2d )
    {
	mIOIOPar( set, BinIDInl, binid.inl );
	mIOIOPar( set, BinIDCrl, binid.crl );
    }

    mIOIOPar( set, CoordX, coord.x );
    mIOIOPar( set, CoordY, coord.y );

    if ( !mIsUdf(pick) )
    {
	if ( !mIsUdf(pick) )
	    mIOIOPar( set, Pick, pick );
	if ( !mIsUdf(refpos) )
	    mIOIOPar( set, RefPos, refpos );
    }
}


void SeisTrcInfo::usePar( const IOPar& iopar )
{
    mIOIOPar( get, TrcNr,	nr );
    mIOIOPar( get, BinIDInl,	binid.inl );
    mIOIOPar( get, BinIDCrl,	binid.crl );
    mIOIOPar( get, CoordX,	coord.x );
    mIOIOPar( get, CoordY,	coord.y );
    mIOIOPar( get, Offset,	offset );
    mIOIOPar( get, Azimuth,	azimuth );
    mIOIOPar( get, Pick,	pick );
    mIOIOPar( get, RefPos,	refpos );

    iopar.get( sSamplingInfo, sampling.start, sampling.step );
}

void SeisTrcInfo::fillPar( IOPar& iopar ) const
{
    mIOIOPar( set, TrcNr,	nr );
    mIOIOPar( set, BinIDInl,	binid.inl );
    mIOIOPar( set, BinIDCrl,	binid.crl );
    mIOIOPar( set, CoordX,	coord.x );
    mIOIOPar( set, CoordY,	coord.y );
    mIOIOPar( set, Offset,	offset );
    mIOIOPar( set, Azimuth,	azimuth );
    mIOIOPar( set, Pick,	pick );
    mIOIOPar( set, RefPos,	refpos );

    iopar.set( sSamplingInfo, sampling.start, sampling.step );
}


bool SeisTrcInfo::dataPresent( float t, int trcsz ) const
{
    return t > sampling.start-1e-6 && t < samplePos(trcsz-1) + 1e-6;
}


int SeisTrcInfo::nearestSample( float t ) const
{
    float s = mIsUdf(t) ? 0 : (t - sampling.start) / sampling.step;
    return mNINT(s);
}


SampleGate SeisTrcInfo::sampleGate( const Interval<float>& tg ) const
{
    SampleGate sg;

    sg.start = sg.stop = 0;
    if ( mIsUdf(tg.start) && mIsUdf(tg.stop) )
	return sg;

    Interval<float> vals(
	mIsUdf(tg.start) ? 0 : (tg.start-sampling.start) / sampling.step,
	mIsUdf(tg.stop) ? 0 : (tg.stop-sampling.start) / sampling.step );

    if ( vals.start < vals.stop )
    {
	sg.start = (int)floor(vals.start+1e-3);
	sg.stop =  (int)ceil(vals.stop-1e-3);
    }
    else
    {
	sg.start =  (int)ceil(vals.start-1e-3);
	sg.stop = (int)floor(vals.stop+1e-3);
    }

    if ( sg.start < 0 ) sg.start = 0;
    if ( sg.stop < 0 ) sg.stop = 0;

    return sg;
}


void SeisTrcInfo::putTo( PosAuxInfo& auxinf ) const
{
    auxinf.binid = binid;
    auxinf.startpos = sampling.start;
    auxinf.coord = coord;
    auxinf.offset = offset;
    auxinf.azimuth = azimuth;
    auxinf.pick = pick;
    auxinf.refpos = refpos;
}


void SeisTrcInfo::getFrom( const PosAuxInfo& auxinf )
{
    binid = auxinf.binid;
    sampling.start = auxinf.startpos;
    coord = auxinf.coord;
    offset = auxinf.offset;
    azimuth = auxinf.azimuth;
    pick = auxinf.pick;
    refpos = auxinf.refpos;
}
