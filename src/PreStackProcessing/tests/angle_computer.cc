/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2008
-*/

static const char* rcsID mUsedVar = "$Id$"; 

#include "batchprog.h"

#include "moddepmgr.h"
#include "multiid.h"
#include "prestackanglecomputer.h"
#include "prestackgather.h"
#include "windowfunction.h"
#include <iostream>


#define mCheckVal(ofsidx,zidx,val) \
    if ( !mIsEqual(angles->data().get(ofsidx,zidx),val,1e-2) ) return false


bool isAngleOK(PreStack::Gather* angles)
{
    mCheckVal( 0, 0, 0 );
    mCheckVal( 0, 46, 0 );
    mCheckVal( 1, 10, 0.5877 );
    mCheckVal( 1, 40, 0.1411 );
    mCheckVal( 2, 20, 0.5612 );
    mCheckVal( 2, 30, 0.3757 );
    mCheckVal( 3, 20, 0.7551 );
    mCheckVal( 3, 30, 0.5341 );
    mCheckVal( 4, 10, 1.2063 );
    mCheckVal( 4, 40, 0.5166 );
    mCheckVal( 5, 0, 1.5416 );
    mCheckVal( 5, 46, 0.5627 );

    return true;
}


bool BatchProgram::go( std::ostream &strm )
{
    OD::ModDeps().ensureLoaded( "Velocity" );
    RefMan<PreStack::VelocityBasedAngleComputer> computer = 
				    new PreStack::VelocityBasedAngleComputer;

    computer->setMultiID( MultiID(100010,34) );

    if ( !computer->isOK() )
    {
	std::cerr<<" Angle computer is not OK.\n";
	return false;
    }

    StepInterval<double> zrange(0,1.848,0.04), offsetrange(0,2500,500);
    FlatPosData fp;
    fp.setRange( true, offsetrange );
    fp.setRange( false, zrange );
    computer->setOutputSampling( fp );
    computer->setTraceID( BinID(425,775) );

    IOPar iopar;
    iopar.set( PreStack::AngleComputer::sKeySmoothType(), 
	       PreStack::AngleComputer::TimeAverage );
    iopar.set( PreStack::AngleComputer::sKeyWinFunc(), HanningWindow::sName() );
    iopar.set( PreStack::AngleComputer::sKeyWinParam(), 0.95f );
    iopar.set( PreStack::AngleComputer::sKeyWinLen(), 10 );

    computer->setSmoothingPars( iopar );

    PtrMan<PreStack::Gather> angles = computer->computeAngles();
    if ( !angles )
    {
	std::cerr << "Computer did not succeed in making angle data\n";
	return false;
    }

    if ( !isAngleOK(angles) )
    {
	std::cerr << "Computer computed wrong values\n";
	return false;
    }

    return true;
}

