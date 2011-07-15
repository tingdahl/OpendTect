/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          July 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: stratsynth.cc,v 1.1 2011-07-15 12:01:37 cvsbruno Exp $";


#include "stratsynth.h"

#include "flatposdata.h"
#include "prestackgather.h"
#include "survinfo.h"
#include "seisbufadapters.h"
#include "seistrc.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "velocitycalc.h"
#include "wavelet.h"


StratSynth::StratSynth()
    : lm_(0)
    , wvlt_(0)
{}


void StratSynth::setModel( const Strat::LayerModel& lm )
{
    lm_ = &lm;
}


void StratSynth::setWavelet( const Wavelet& wvlt )
{
    wvlt_ = &wvlt;
}


int StratSynth::getVelIdx( bool& isvel ) const
{
    //TODO this requires a lot of work. Can be auto-detected form property
    // StdType but sometimes user has many velocity providers:
    // - Many versions (different measurements, sources, etc)
    // - Sonic vs velocity
    isvel = true; return 1; // This is what the simple generator generates
}


int StratSynth::getDenIdx( bool& isden ) const
{
    //TODO support:
    // - density itself
    // - den = ai / vel
    isden = true; return 2; // This is what the simple generator generates
}

#define mErrRet( msg, act ) { if ( errmsg )*errmsg = msg; act; }
DataPack* StratSynth::genTrcBufDataPack( const RayParams& raypars,
				ObjectSet<const TimeDepthModel>& d2ts,
				BufferString* errmsg ) const 
{
    ObjectSet<SeisTrcBuf> seisbufs;
    genSeisBufs( raypars, d2ts, seisbufs );
    if ( seisbufs.isEmpty() )
	return 0;

    SeisTrcBuf* tbuf = new SeisTrcBuf( true );
    const int crlstep = SI().crlStep();
    const BinID bid0( SI().inlRange(false).stop + SI().inlStep(),
	    	      SI().crlRange(false).stop + crlstep );

    const int nraimdls = raypars.cs_.nrInl();
    ObjectSet<const SeisTrc> trcs;
    for ( int imdl=0; imdl<nraimdls; imdl++ )
    {
	tbuf->stealTracesFrom( *seisbufs[imdl] );
    }
    deepErase( seisbufs );

    if ( tbuf->isEmpty() ) 
	mErrRet("No seismic trace genereated ", return 0)

    SeisTrcBufDataPack* tdp = new SeisTrcBufDataPack( 
			tbuf, Seis::Line, SeisTrcInfo::TrcNr, "Seismic" ) ;
    const SeisTrc& trc0 = *tbuf->get(0);
    StepInterval<double> zrg( trc0.info().sampling.start,
			      trc0.info().sampling.atIndex(trc0.size()-1),
			      trc0.info().sampling.step );
    tdp->posData().setRange( true, StepInterval<double>(1,tbuf->size(),1) );
    tdp->posData().setRange( false, zrg );
    tdp->setName( raypars.synthname_ );

    return tdp;
}


DataPack* StratSynth::genGatherDataPack( const RayParams& raypars,
				ObjectSet<const TimeDepthModel>& d2ts,
       				BufferString* errmsg ) const 
{
    ObjectSet<SeisTrcBuf> seisbufs;
    genSeisBufs( raypars, d2ts, seisbufs );
    if ( seisbufs.isEmpty() )
	return 0;

    ObjectSet<PreStack::Gather> gathers;
    const int nraimdls = raypars.cs_.nrInl();
    for ( int imdl=0; imdl<nraimdls; imdl++ )
    {
	SeisTrcBuf& tbuf = *seisbufs[imdl];
	PreStack::Gather* gather = new PreStack::Gather();
	if ( !gather->setFromTrcBuf( tbuf, 0 ) )
	    { delete gather; continue; }

	gather->posData().setRange(true,StepInterval<double>(1,tbuf.size(),1));
	gathers += gather;
    }
    deepErase( seisbufs );

    if ( gathers.isEmpty() ) 
	mErrRet("No seismic trace genereated ", return 0)

    PreStack::GatherSetDataPack* pdp = new PreStack::GatherSetDataPack(
							"GatherSet", gathers );
    pdp->setName( raypars.synthname_ );
    return pdp;
}


bool StratSynth::genSeisBufs( const RayParams& raypars,
			    ObjectSet<const TimeDepthModel>& d2ts,
			    ObjectSet<SeisTrcBuf>& seisbufs,
			    BufferString* errmsg ) const 
{
    if ( !lm_ || lm_->isEmpty() ) 
	mErrRet( 0, return false; )

    const CubeSampling& cs = raypars.cs_;
    TypeSet<float> offsets;
    for ( int idx=0; idx<cs.nrCrl(); idx++ )
	offsets += cs.hrg.crlRange().atIndex(idx);

    Seis::RaySynthGenerator synthgen;
    synthgen.setRayParams( raypars.setup_, offsets, raypars.usenmotimes_ );
    synthgen.setWavelet( wvlt_, OD::UsePtr );
    const int nraimdls = cs.nrInl();

    bool isvel; const int velidx = getVelIdx( isvel );
    bool isden; const int denidx = getDenIdx( isden );

    for ( int iseq=0; iseq<nraimdls; iseq++ )
    {
	int seqidx = cs.hrg.inlRange().atIndex(iseq)-1;
	const Strat::LayerSequence& seq = lm_->sequence( seqidx );
	AIModel aimod; seq.getAIModel( aimod, velidx, denidx, isvel, isden );
	if ( aimod.isEmpty() )
	    mErrRet( "Layer model is empty", return false;) 
	else if ( aimod.size() == 1  )
	    mErrRet("Please add at least one layer to the model", return false;)

	synthgen.addModel( aimod );
    }

    if ( !synthgen.doWork() )
	mErrRet( synthgen.errMsg(), return 0 );

    const int crlstep = SI().crlStep();
    const BinID bid0( SI().inlRange(false).stop + SI().inlStep(),
	    	      SI().crlRange(false).stop + crlstep );

    ObjectSet<const SeisTrc> trcs;
    ObjectSet<const TimeDepthModel> tmpd2ts;
    for ( int imdl=0; imdl<nraimdls; imdl++ )
    {
	Seis::RaySynthGenerator::RayModel& rm = 
	    const_cast<Seis::RaySynthGenerator::RayModel&>( 
						    synthgen.result( imdl ) );
	trcs.erase(); 
	if ( raypars.dostack_ )
	    trcs += rm.stackedTrc();
	else
	    rm.getTraces( trcs, true );

	if ( trcs.isEmpty() )
	    continue;

	seisbufs += new SeisTrcBuf( true );
	for ( int idx=0; idx<trcs.size(); idx++ )
	{
	    SeisTrc* trc = const_cast<SeisTrc*>( trcs[idx] );
	    const int trcnr = imdl + 1;
	    trc->info().nr = trcnr;
	    trc->info().binid = BinID( bid0.inl, bid0.crl + imdl * crlstep );
	    trc->info().coord = SI().transform( trc->info().binid );
	    seisbufs[imdl]->add( trc );
	}
	rm.getD2T( tmpd2ts, true );
	if ( !tmpd2ts.isEmpty() )
	    d2ts += tmpd2ts.remove(0);
	deepErase( tmpd2ts );
    }
    return true;
}


const SyntheticData* StratSynth::generate( const RayParams& rp, bool isps, 
					BufferString* errmsg ) const
{
    SyntheticData* sd = new SyntheticData( rp.synthname_ );
    DataPack* dp = isps ? genGatherDataPack( rp, sd->d2tmodels_, errmsg )
		        : genTrcBufDataPack( rp, sd->d2tmodels_, errmsg );
    if ( !dp ) 
	{ delete sd; return 0; }

    DataPackMgr::ID pmid = isps ? DataPackMgr::CubeID() : DataPackMgr::FlatID();
    DPM( pmid ).add( dp );

    sd->wvlt_ = wvlt_;
    sd->isps_ = isps;
    sd->packid_ = DataPack::FullID( pmid, dp->id());

    return sd;
}


SyntheticData::~SyntheticData()
{
    deepErase( d2tmodels_ );
    const DataPack::FullID dpid = packid_;
    DataPackMgr::ID packmgrid = DataPackMgr::getID( dpid );
    const DataPack* dp = DPM(packmgrid).obtain( DataPack::getID(dpid) );
    if ( dp )
	DPM(packmgrid).release( dp->id() );
}
