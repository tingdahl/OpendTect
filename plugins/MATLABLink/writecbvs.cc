/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		July 2013
 SVN:		$Id$
________________________________________________________________________

-*/

#include "mex.h"

#include "arrayndimpl.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "dbman.h"
#include "keystrs.h"
#include "matlabarray.h"
#include "moddepmgr.h"
#include "oddirs.h"
#include "ranges.h"
#include "seisdatapack.h"
#include "seisdatapackwriter.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "threadwork.h"

extern "C" void od_Seis_initStdClasses();

static const char* sOutSeisKey = "Output Seismics";

#define mErrRet( msg ) \
{ mexErrMsgTxt( BufferString(msg,"\n") ); return; }


void mexFunction( int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[] )
{
    if ( nrhs < 2 )
	mErrRet( "Usage: writecbvs parameter-file data" );

    if ( nlhs != 0 )
	mErrRet( "No output required." );

    char* progname = (char*)"writecbvs";
    int argc = 1;
    char** argv = (char**)mxCalloc( argc, sizeof(char*) );
    argv[0] = progname;

    SetProgramArgs( argc, argv );
    od_Seis_initStdClasses();

    const BufferString fnm = mxArrayToString( prhs[0] );
    if ( !File::exists(fnm) )
	mErrRet( "Input parameter file name does not exist" );

    IOPar par;
    if ( !par.read(fnm,0) )
	mErrRet( "Cannot read input parameter file" );

    BufferString res;
    if ( par.get(sKey::DataRoot(),res) )
    {
	if ( !File::exists(res) || !File::isDirectory(res) )
	    mErrRet( "Given dataroot does not exist or is not a directory" );

	SetEnvVar( "DTECT_DATA", res );
    }
    else
    {
	const BufferString msg( "No dataroot specified. Using: ",
		GetBaseDataDir() );
	mexPrintf( msg );
    }

    if ( par.get(sKey::Survey(),res) )
    {
	File::Path fp( GetBaseDataDir() ); fp.add( res );
	const BufferString surveyfp = fp.fullPath();
	if ( !File::exists(surveyfp) || !File::isDirectory(surveyfp) )
	    mErrRet( "Given survey does not exist" );

	IOMan::setSurvey( res );
    }
    else
    {
	const BufferString msg( "No survey specified. Using: ",
				SI().getDirName() );
	mexPrintf( msg );
    }

    BufferString errmsg;
    IOObjContext ctxt( SeisTrcTranslatorGroup::ioContext() );
    PtrMan<IOObj> ioobj =
	DBM().getFromPar( par, sOutSeisKey, ctxt, true, errmsg );
    if ( !ioobj )
    {
	if ( errmsg.isEmpty() )
	    errmsg = "Cannot find input data information in parameter file";
	mErrRet( errmsg );
    }

    CubeSampling tkzs;
    tkzs.usePar( par );

    BufferString tkzsinfo;
    tkzs.hsamp_.toString( tkzsinfo );
    mexPrintf( tkzsinfo.buf() ); mexPrintf( "\n" );
    mexPrintf( "Nr of traces: %d\n", tkzs.hsamp_.totalNr() );

    const char* category = VolumeDataPack::categoryStr(
				tkzs.defaultDir()!=TrcKeyZSampling::Z, false );
    DataPackMgr& dpm = DPM(DataPackMgr::SeisID());
    RefMan<RegularSeisDataPack> output =
	dpm.add( new RegularSeisDataPack(category) );
    output->setSampling( tkzs );

    TypeSet<int> cubeidxs;
    for ( int idx=1; idx<nrhs; idx++ )
    {
	const mxArray* mxarr = prhs[idx];
	if ( !mxarr ) continue;

	const int cubeidx = idx-1;
	cubeidxs += cubeidx;
	output->addComponent( sKey::EmptyString() );
	mxArrayCopier copier( *mxarr, output->data(cubeidx) );
	copier.init();
	copier.execute();
    }

    SeisDataPackWriter writer( ioobj->key(), *output, cubeidxs );
    if ( !writer.execute() )
	mErrRet( "Error in writing data" );

    Threads::WorkManager::twm().shutdown();
}
