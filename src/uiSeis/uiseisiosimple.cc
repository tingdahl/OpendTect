/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/

static const char* rcsID = "$Id: uiseisiosimple.cc,v 1.19 2009-03-24 12:33:51 cvsbert Exp $";

#include "uiseisiosimple.h"
#include "uiseisfmtscale.h"
#include "uiseissubsel.h"
#include "uiseissel.h"
#include "uimultcomputils.h"
#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uitaskrunner.h"
#include "uiseparator.h"
#include "uiscaler.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uibutton.h"
#include "seistrctr.h"
#include "seispsioprov.h"
#include "seisselection.h"
#include "seisresampler.h"
#include "ctxtioobj.h"
#include "cubesampling.h"
#include "ioobj.h"
#include "iopar.h"
#include "survinfo.h"
#include "oddirs.h"
#include "filegen.h"
#include "filepath.h"
#include "keystrs.h"


static bool survChanged()
{
    static BufferString survnm;
    const bool issame = survnm.isEmpty() || survnm == SI().name();
    survnm = SI().name();
    return !issame;
}


#define mDefData(nm,geom) \
SeisIOSimple::Data& uiSeisIOSimple::data##nm() \
{ \
    static SeisIOSimple::Data* d = 0; \
    if ( !d ) d = new SeisIOSimple::Data( GetDataDir(), Seis::geom ); \
    return *d; \
}

mDefData(2d,Line)
mDefData(3d,Vol)
mDefData(ps,VolPS)


uiSeisIOSimple::uiSeisIOSimple( uiParent* p, Seis::GeomType gt, bool imp )
	: uiDialog( p, Setup( imp ? "Import seismics from simple flat file"
				  : "Export seismics to simple flat file",
			      "Specify parameters for I/O",
			      imp ? "103.0.11" : "103.0.12") )
    	, ctio_(*uiSeisSel::mkCtxtIOObj(gt,!imp))
    	, sdfld_(0)
	, havenrfld_(0)
	, nrdeffld_(0)
    	, inldeffld_(0)
    	, subselfld_(0)
    	, isxyfld_(0)
    	, lnmfld_(0)
    	, isascfld_(0)
    	, haveoffsbut_(0)
    	, haveazimbut_(0)
	, pspposlbl_(0)
	, offsdeffld_(0)
    	, isimp_(imp)
    	, geom_(gt)
{
    data().clear( survChanged() );
    const bool is2d = is2D();
    const bool isps = isPS();

    uiSeparator* sep = 0;
    if ( isimp_ )
    {
	mkIsAscFld();
	fnmfld_ = new uiFileInput( this, "Input file", uiFileInput::Setup("")
			.forread( true )
			.withexamine( true ) );
	fnmfld_->attach( alignedBelow, isascfld_ );
    }
    else
    {
	seisfld_ = new uiSeisSel( this, ctio_, uiSeisSel::Setup(geom_) );
	seisfld_->selectiondone.notify( mCB(this,uiSeisIOSimple,inpSeisSel) );
	sep = mkDataManipFlds();
    }

    haveposfld_ = new uiGenInput( this,
	    		isimp_ ? "Traces start with a position"
			      : "Output a position for every trace",
	   		BoolInpSpec(true) );
    haveposfld_->setValue( data().havepos_ );
    haveposfld_->valuechanged.notify( mCB(this,uiSeisIOSimple,haveposSel) );
    haveposfld_->attach( alignedBelow, isimp_ ? fnmfld_->attachObj()
	    				     : remnullfld_->attachObj() );
    if ( sep ) haveposfld_->attach( ensureBelow, sep );

    uiObject* attachobj = haveposfld_->attachObj();
    if ( is2d )
    {
	BufferString txt( isimp_ ? "Trace number included"
				 : "Include trace number");
	txt += " (preceeding X/Y)";
	havenrfld_ = new uiGenInput( this, txt, BoolInpSpec(true) );
	havenrfld_->setValue( data().havenr_ );
	havenrfld_->attach( alignedBelow, attachobj );
	havenrfld_->valuechanged.notify( mCB(this,uiSeisIOSimple,havenrSel) );
	attachobj = havenrfld_->attachObj();
    }
    else
    {
	isxyfld_ = new uiGenInput( this, isimp_ ? "Position in file is"
					       : "Position in file will be",
				 BoolInpSpec(true,"X Y","Inline Xline") );
	isxyfld_->setValue( data().isxy_ );
	isxyfld_->attach( alignedBelow, attachobj );
	if ( !isimp_ ) attachobj = isxyfld_->attachObj();
    }

    if ( isimp_ )
    {
	if ( !is2d )
	{
	    inldeffld_ = new uiGenInput( this, "Inline definition: start, step",
				IntInpSpec(data().inldef_.start)
						.setName("Inl def start"),
			  	IntInpSpec(data().inldef_.step)
						.setName("Inl def step") );
	    inldeffld_->attach( alignedBelow, attachobj );
	    crldeffld_ = new uiGenInput( this,
			"Xline definition: start, step, # per inline",
			   IntInpSpec(data().crldef_.start)
			   			.setName("Crl def start"),
			   IntInpSpec(data().crldef_.step)
			   			.setName("Crl def step"),
			   IntInpSpec(data().nrcrlperinl_)
			   			.setName("per inl") );
	    crldeffld_->attach( alignedBelow, inldeffld_ );
	    attachobj = crldeffld_->attachObj();
	}
	else
	{
	    nrdeffld_ = new uiGenInput( this,
		    "Trace number definition: start, step",
		    IntInpSpec(data().nrdef_.start).setName("Trc def start"),
		    IntInpSpec(data().nrdef_.step).setName("Trc def step") );
	    nrdeffld_->attach( alignedBelow, attachobj );
	    startposfld_ = new uiGenInput( this,
					  "Start position (X, Y, Trace number)",
					  PositionInpSpec(data().startpos_) );
	    startposfld_->attach( alignedBelow, haveposfld_ );
	    stepposfld_ = new uiGenInput( this, "Step in X/Y/Number",
					 PositionInpSpec(data().steppos_) );
	    stepposfld_->attach( alignedBelow, startposfld_ );
	    startnrfld_ = new uiGenInput( this, "",
		    			 IntInpSpec(data().nrdef_.start) );
	    startnrfld_->setElemSzPol( uiObject::Small );
	    startnrfld_->attach( rightOf, startposfld_ );
	    stepnrfld_ = new uiGenInput( this, "",
		    			IntInpSpec(data().nrdef_.step) );
	    stepnrfld_->setElemSzPol( uiObject::Small );
	    stepnrfld_->attach( rightOf, stepposfld_ );
	    attachobj = stepposfld_->attachObj();
	}
	if ( isps )
	{
	    haveoffsbut_ = new uiCheckBox( this, "Offset",
		    			 mCB(this,uiSeisIOSimple,haveoffsSel) );
	    haveoffsbut_->attach( alignedBelow, attachobj );
	    haveoffsbut_->setChecked( data().haveoffs_ );
	    haveazimbut_ = new uiCheckBox( this, "Azimuth" );
	    haveazimbut_->attach( rightOf, haveoffsbut_ );
	    haveazimbut_->setChecked( data().haveazim_ );
	    pspposlbl_ = new uiLabel( this, "Position includes", haveoffsbut_ );
	    const float stopoffs =
			data().offsdef_.atIndex(data().nroffsperpos_-1);
	    offsdeffld_ = new uiGenInput( this,
		    	   "Offset definition: start, stop, step",
			   FloatInpSpec(data().offsdef_.start).setName("Start"),
			   FloatInpSpec(stopoffs).setName("Stop"),
		   	   FloatInpSpec(data().offsdef_.step).setName("Step") );
	    offsdeffld_->attach( alignedBelow, haveoffsbut_ );
	    attachobj = offsdeffld_->attachObj();
	}
    }

    havesdfld_ = new uiGenInput( this, isimp_
	    			    ? "File start contains sampling info"
				    : "Put sampling info in file start",
				    BoolInpSpec(true)
				      .setName("Info in file start Yes",0)
	   			      .setName("Info in file start No",1) );
    havesdfld_->setValue( data().havesd_ );
    havesdfld_->attach( alignedBelow, attachobj );
    havesdfld_->valuechanged.notify( mCB(this,uiSeisIOSimple,havesdSel) );

    if ( isimp_ )
    {
	BufferString txt = "Sampling info: start, step ";
	txt += SI().getZUnitString(true);
	txt += " and #samples";
	SamplingData<float> sd( data().sd_ );
	if ( SI().zIsTime() )
	    { sd.start *= 1000; sd.step *= 1000; }
	sdfld_ = new uiGenInput( this, txt, 
			DoubleInpSpec(sd.start).setName("SampInfo start"),
			DoubleInpSpec(sd.step).setName("SampInfo step"),
			IntInpSpec(data().nrsamples_).setName("Nr samples") );
	sdfld_->attach( alignedBelow, havesdfld_ );
	sep = mkDataManipFlds();
	seisfld_ = new uiSeisSel( this, ctio_, uiSeisSel::Setup(geom_));
	seisfld_->attach( alignedBelow, multcompfld_ );
	if ( is2d )
	{
	    lnmfld_ = new uiGenInput( this,
		    		     isps ? "Line name" : "Line name in Set" );
	    lnmfld_->attach( alignedBelow, seisfld_ );
	}
    }
    else
    {
	mkIsAscFld();
	isascfld_->attach( alignedBelow, havesdfld_ );
	fnmfld_ = new uiFileInput( this, "Output file", uiFileInput::Setup("")
			.forread( false )
			.withexamine( false ) );
	fnmfld_->attach( alignedBelow, isascfld_ );
    }

    fnmfld_->setDefaultSelectionDir( FilePath(data().fname_).pathOnly() );
    finaliseDone.notify( mCB(this,uiSeisIOSimple,initFlds) );
}


void uiSeisIOSimple::mkIsAscFld()
{
    isascfld_ = new uiGenInput( this, "File type",
	    		       BoolInpSpec(true,"Ascii","Binary") );
    isascfld_->valuechanged.notify( mCB(this,uiSeisIOSimple,isascSel) );
    isascfld_->setValue( data().isasc_ );
}


uiSeparator* uiSeisIOSimple::mkDataManipFlds()
{
    uiSeparator* sep = new uiSeparator( this, "sep inp and outp pars" );
    if ( isimp_ )
	sep->attach( stretchedBelow, sdfld_ );
    else
    {
	subselfld_ = uiSeisSubSel::get( this, Seis::SelSetup(geom_)
						.onlyrange(false) );
	subselfld_->attachObj()->attach( alignedBelow, seisfld_ );
    }

    scalefld_ = new uiScaler( this, 0, true );
    scalefld_->attach( alignedBelow, isimp_ ? sdfld_->attachObj()
	    				   : subselfld_->attachObj() );
    if ( isimp_ ) scalefld_->attach( ensureBelow, sep );
    remnullfld_ = new uiGenInput( this, "Null traces",
				 BoolInpSpec(true,"Discard","Pass") );
    remnullfld_->attach( alignedBelow, scalefld_ );

    multcompfld_ = new uiMultCompSel( this );
    multcompfld_->attach( alignedBelow, remnullfld_ );
    multcompfld_->setSensitive( false );

    if ( !isimp_ )
	sep->attach( stretchedBelow, multcompfld_ );

    return sep;
}


void uiSeisIOSimple::initFlds( CallBacker* cb )
{
    havesdSel( cb );
    haveposSel( cb );
    isascSel( cb );
    haveoffsSel( cb );
}


void uiSeisIOSimple::havesdSel( CallBacker* )
{
    if ( sdfld_ )
	sdfld_->display( !havesdfld_->getBoolValue() );
}


void uiSeisIOSimple::inpSeisSel( CallBacker* )
{
    seisfld_->commitInput();
    if ( ctio_.ioobj )
    {
	subselfld_->setInput( *ctio_.ioobj );
	LineKey lkey( ctio_.ioobj->key() );
	multcompfld_->setUpList( lkey );
	multcompfld_->setSensitive( multcompfld_->allowChoice() );
    }
}



void uiSeisIOSimple::isascSel( CallBacker* )
{
    fnmfld_->enableExamine( isascfld_->getBoolValue() );
}


void uiSeisIOSimple::haveposSel( CallBacker* cb )
{
    const bool havenopos = !haveposfld_->getBoolValue();

    if ( isxyfld_ ) isxyfld_->display( !havenopos );
    if ( havenrfld_ ) havenrfld_->display( !havenopos );

    if ( isimp_ )
    {
	if ( !is2D() )
	{
	    inldeffld_->display( havenopos );
	    crldeffld_->display( havenopos );
	}
	else
	{
	    startposfld_->display( havenopos );
	    startnrfld_->display( havenopos );
	    stepposfld_->display( havenopos );
	    stepnrfld_->display( havenopos );
	}
    }

    havenrSel( cb );
    haveoffsSel( cb );
}


void uiSeisIOSimple::havenrSel( CallBacker* cb )
{
    if ( !nrdeffld_ ) return;
    nrdeffld_->display( haveposfld_->getBoolValue()
	    	    && !havenrfld_->getBoolValue() );
}


void uiSeisIOSimple::haveoffsSel( CallBacker* cb )
{
    if ( !pspposlbl_ || !haveoffsbut_ ) return;
    const bool havepos = haveposfld_->getBoolValue();
    const bool haveoffs = haveoffsbut_->isChecked();
    haveoffsbut_->display( havepos );
    haveazimbut_->display( havepos );
    offsdeffld_->display( !havepos || !haveoffs );
    pspposlbl_->display( havepos );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiSeisIOSimple::acceptOK( CallBacker* )
{
    if ( !isascfld_ ) return true;

    BufferString fnm( fnmfld_->fileName() );
    if ( isimp_ && !File_exists(fnm) )
	mErrRet("Input file does not exist or is unreadable")
    if ( !seisfld_->commitInput() )
	mErrRet( isimp_ ? "Please choose a name for the imported data"
		       : "Please select the input seismics")

    data().subselpars_.clear();
    if ( is2D() )
    {
	BufferString linenm;
	if ( lnmfld_ )
	{
	    linenm = lnmfld_->text();
	    if ( linenm.isEmpty() )
		mErrRet( "Please enter a line name" )
	    data().linekey_.setLineName( linenm );
	}
	data().linekey_.setAttrName( seisfld_->attrNm() );
    }

    data().seiskey_ = ctio_.ioobj->key();
    data().fname_ = fnm;

    data().setScaler( scalefld_->getScaler() );
    data().remnull_ = remnullfld_->getBoolValue();

    data().isasc_ = isascfld_->getBoolValue();
    data().havesd_ = havesdfld_->getBoolValue();
    if ( sdfld_ && !data().havesd_ )
    {
	data().sd_.start = sdfld_->getfValue(0);
	data().sd_.step = sdfld_->getfValue(1);
	if ( SI().zIsTime() )
	    { data().sd_.start *= 0.001; data().sd_.step *= 0.001; }
	data().nrsamples_ = sdfld_->getIntValue(2);
    }

    data().havepos_ = haveposfld_->getBoolValue();
    data().havenr_ = false;
    if ( data().havepos_ )
    {
	data().isxy_ = is2D() || isxyfld_->getBoolValue();
	data().havenr_ = is2D() && havenrfld_->getBoolValue();
	if ( isimp_ && data().havenr_ )
	{
	    data().nrdef_.start = nrdeffld_->getIntValue(0);
	    data().nrdef_.step = nrdeffld_->getIntValue(1);
	}
	if ( isPS() )
	{
	    data().haveoffs_ = haveoffsbut_->isChecked();
	    data().haveazim_ = haveazimbut_->isChecked();
	}
    }
    else if ( isimp_ )
    {
	data().haveoffs_ = false;
	if ( is2D() )
	{
	    data().startpos_ = startposfld_->getCoord();
	    data().steppos_ = stepposfld_->getCoord();
	    data().nrdef_.start = startnrfld_->getIntValue();
	    data().nrdef_.step = stepnrfld_->getIntValue();
	}
	else
	{
	    data().inldef_.start = inldeffld_->getIntValue(0);
	    data().inldef_.step = inldeffld_->getIntValue(1);
	    if ( data().inldef_.step == 0 ) data().inldef_.step = 1;
	    data().crldef_.start = crldeffld_->getIntValue(0);
	    data().crldef_.step = crldeffld_->getIntValue(1);
	    if ( data().crldef_.step == 0 ) data().crldef_.step = 1;
	    int nrcpi = crldeffld_->getIntValue(2);
	    if ( nrcpi == 0 || crldeffld_->isUndef(2) )
	    {
		uiMSG().error( "Please define the number of Xlines per Inline");
		return false;
	    }
	    data().nrcrlperinl_ = nrcpi;
	}
    }

    if ( isPS() && !data().haveoffs_ )
    {
	data().offsdef_.start = offsdeffld_->getfValue( 0 );
	data().offsdef_.step = offsdeffld_->getfValue( 2 );
	const float offsstop = offsdeffld_->getfValue( 1 );
	data().nroffsperpos_ =
			data().offsdef_.nearestIndex( offsstop ) + 1;
    }

    if ( subselfld_ )
    {
	subselfld_->fillPar( data().subselpars_ );
	if ( !subselfld_->isAll() )
	{
	    CubeSampling cs;
	    subselfld_->getSampling( cs.hrg ); subselfld_->getZRange( cs.zrg );
	    data().setResampler( new SeisResampler(cs,is2D()) );
	}
    }

    SeisIOSimple sios( data(), isimp_ );
    uiTaskRunner dlg( this );
    return dlg.execute( sios );
}
