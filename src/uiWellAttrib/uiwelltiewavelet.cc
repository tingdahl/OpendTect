/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Mar 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelltiewavelet.cc,v 1.11 2009-06-15 08:29:32 cvsbruno Exp $";

#include "uiwelltiewavelet.h"

#include "arrayndimpl.h"
#include "ctxtioobj.h"
#include "flatposdata.h"
#include "hilberttransform.h"
#include "ioman.h"
#include "ioobj.h"
#include "math.h"
#include "survinfo.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "statruncalc.h"
#include "wavelet.h"
#include "welltiedata.h"
#include "welltiesetup.h"

#include "uiaxishandler.h"
#include "uibutton.h"
#include "uiflatviewer.h"
#include "uifunctiondisplay.h"
#include "uigroup.h"
#include "uiioobjsel.h"
#include "uilabel.h"

#include <complex>

uiWellTieWaveletView::uiWellTieWaveletView( uiParent* p,
					    const WellTieDataHolder* dh )
	: uiGroup(p)
	, dataholder_(dh)  
	, twtss_(dh->setup())
	, wvltctio_(*mMkCtxtIOObj(Wavelet))
{
    for ( int idx=0; idx<2; idx++ )
    {
	viewer_ += new uiFlatViewer( this );
	initWaveletViewer( idx );
    }
    createWaveletFields( this );
} 


uiWellTieWaveletView::~uiWellTieWaveletView()
{
    for (int vwridx=viewer_.size()-1; vwridx>=0; vwridx--)
	viewer_.remove(vwridx);
}


void uiWellTieWaveletView::initWaveletViewer( int vwridx )
{
    FlatView::Appearance& app = viewer_[vwridx]->appearance();
    app.annot_.x1_.name_ = "Amplitude";
    app.annot_.x2_.name_ =  "Time";
    app.annot_.setAxesAnnot( false );
    app.setGeoDefaults( true );
    app.ddpars_.show( true, false );
    app.ddpars_.wva_.overlap_ = 0;
    app.ddpars_.wva_.clipperc_.start = app.ddpars_.wva_.clipperc_.stop = 0;
    app.ddpars_.wva_.left_ = Color( 250, 0, 0 );
    app.ddpars_.wva_.right_ = Color( 0, 0, 250 );
    app.ddpars_.wva_.mid_ = Color( 0, 0, 250 );
    app.ddpars_.wva_.symmidvalue_ = mUdf(float);
    app.setDarkBG( false );
    viewer_[vwridx]->setInitialSize( uiSize(80,100) );
    viewer_[vwridx]->setStretch( 1, 2 );
}


void uiWellTieWaveletView::createWaveletFields( uiGroup* grp )
{
    grp->setHSpacing(40);
   // wvltfld_ = new uiIOObjSel( grp, wvltctio_ );
   // wvltfld_->setInput( twtss_.wvltid_ );
    //wvltfld_->selectiondone.notify( mCB(this, uiWellTieWaveletView, wvtSel));
    
    uiLabel* wvltlbl = new uiLabel( this, "Initial wavelet" );
    uiLabel* wvltestlbl = new uiLabel( this, "Estimated wavelet" );
    wvltlbl->attach( alignedAbove, viewer_[0] );
    wvltestlbl->attach( alignedAbove, viewer_[1] );
    wvltbuts_ += new uiPushButton( grp,  "Properties", 
	    mCB(this,uiWellTieWaveletView,viewInitWvltPropPushed),false);
    wvltbuts_ += new uiPushButton( grp, "Properties", 
	    mCB(this,uiWellTieWaveletView,viewEstWvltPropPushed),false);

    wvltbuts_[0]->attach( alignedBelow, viewer_[0] );
    wvltbuts_[1]->attach( alignedBelow, viewer_[1] );
    
    viewer_[0]->attach( alignedBelow, wvltlbl );
    viewer_[1]->attach( rightOf, viewer_[0] );
    viewer_[1]->attach( ensureRightOf, viewer_[0] );
}


void uiWellTieWaveletView::initWavelets( )
{
    for ( int idx=wvlts_.size()-1; idx>=0; idx-- )
	delete wvlts_.remove(idx);

    IOObj* ioobj = IOM().get( MultiID(twtss_.wvltid_) );
    wvlts_ += Wavelet::get( ioobj);
    wvlts_ += new Wavelet(*dataholder_->getEstimatedWvlt());

    if ( !wvlts_[0] || !wvlts_[1] ) return;

    for ( int idx=0; idx<2; idx++ )
	drawWavelet( wvlts_[idx], idx );
}


void uiWellTieWaveletView::drawWavelet( const Wavelet* wvlt, int vwridx )
{
    BufferString tmp;
    const int wvltsz = wvlt->size();
    const float zfac = SI().zFactor();

    Array2DImpl<float>* fva2d = new Array2DImpl<float>( 1, wvltsz );
    FlatDataPack* dp = new FlatDataPack( "Wavelet", fva2d );
    DPM( DataPackMgr::FlatID() ).add( dp );
    viewer_[vwridx]->setPack( true, dp->id(), false );

    for ( int wvltidx=0; wvltidx< wvltsz; wvltidx++)
	 fva2d->set( 0, wvltidx,  wvlt->samples()[wvltidx] );
    dp->setName( wvlt->name() );
    
    DPM( DataPackMgr::FlatID() ).add( dp );
    StepInterval<double> posns; posns.setFrom( wvlt->samplePositions() );
    if ( SI().zIsTime() ) posns.scale( zfac );
    
    dp->posData().setRange( false, posns );
    Stats::RunCalc<float> rc( Stats::RunCalcSetup().require(Stats::Max) );
    rc.addValues( wvltsz, wvlt->samples() );
    
    viewer_[vwridx]->setPack( true, dp->id(), false );
    viewer_[vwridx]->handleChange( FlatView::Viewer::All );
}


void uiWellTieWaveletView::wvtSel( CallBacker* )
{
    /*
    if ( twtss_.wvltid_ == wvltfld_->getKey() ) return;
    twtss_.wvltid_ =  wvltfld_->getKey();
    IOObj* ioobj = IOM().get( twtss_.wvltid_ );
    Wavelet* wvlt = Wavelet::get( ioobj );
    viewer_[0]->removePack( viewer_[0]->pack(true)->id() ); 
    drawWavelet( wvlt, 0 );*/
    //wvltChanged.trigger();
}


void uiWellTieWaveletView::viewInitWvltPropPushed( CallBacker* )
{
    uiWellTieWaveletDispDlg* wvltinitdlg = 
	new uiWellTieWaveletDispDlg( this, wvlts_[0] );
    wvltinitdlg->go();
   // delete wvltinitdlg;
}


void uiWellTieWaveletView::viewEstWvltPropPushed( CallBacker* )
{
    uiWellTieWaveletDispDlg* wvltestdlg = 
	new uiWellTieWaveletDispDlg( this, wvlts_[1] );
    wvltestdlg->go();
    //delete wvltestdlg;
}



uiWellTieWaveletDispDlg::uiWellTieWaveletDispDlg( uiParent* p, 
						  const Wavelet* wvlt )
	: uiDialog( p,Setup("Wavelet Properties","",mTODOHelpID).modal(false))
	, wvlt_(wvlt)  
	, wvltctio_(*mMkCtxtIOObj(Wavelet))
	, wvltsz_(0)
{
    setCtrlStyle( LeaveOnly );

    if ( !wvlt ) return;
    wvltsz_ = wvlt->size();

    static const char* disppropnms[] = { "Amplitude", "Phase", "Frequency", 0 };

    uiFunctionDisplay::Setup fdsu; fdsu.border_.setRight( 0 );
    for ( int idx=0; disppropnms[idx]; idx++ )
    {
	if ( idx>1 ) fdsu.fillbelow(true);	
	wvltdisps_ += new uiFunctionDisplay( this, fdsu );
	wvltdisps_[idx]->xAxis()->setName( "samples" );
	wvltdisps_[idx]->yAxis(false)->setName( disppropnms[idx] );
	if  (idx )
	    wvltdisps_[idx]->attach( alignedBelow, wvltdisps_[idx-1] );
	propvals_ += new TypeSet<float>;
    }

    wvlttrc_ = new SeisTrc;
    wvlttrc_->reSize( wvltsz_, false );
    
    setDispCurves();
}


uiWellTieWaveletDispDlg::~uiWellTieWaveletDispDlg()
{
//    for ( int idx=propvals_.size()-1; idx>=0; idx++ )
//	delete propvals_.remove(idx);
//    delete wvlttrc_;
}


void uiWellTieWaveletDispDlg::setDispCurves()
{
    TypeSet<float> xvals;
    for ( int propidx=0; propidx<propvals_.size(); propidx++ )
	propvals_[propidx]->erase();
    for ( int idx=0; idx<wvltsz_; idx++ )
    {
	xvals += idx;
	wvlttrc_->set( idx, wvlt_->samples()[idx], 0 );
	wvlttrc_->info().nr = idx;
    }

    *propvals_[0] += wvlttrc_->getValue( 0, 0 ); 
    SeisTrcPropCalc pc( *wvlttrc_ );
    const int idx = wvlttrc_->nearestSample( 0 );
    *propvals_[1] += pc.getPhase( idx ); 
    *propvals_[2] += pc.getFreq( idx ); 

    for ( int idx=0; idx<propvals_.size()-1; idx++ )
	wvltdisps_[idx]->setVals( xvals.arr(),
				  propvals_[idx]->arr(),
    				  wvltsz_ );

	wvltdisps_[2]->setVals( Interval<float>(20,75),
				propvals_[2]->arr(),
    			  	wvltsz_ );
}


