/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Dec 2005
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: uihorizontracksetup.cc 38749 2015-04-02 19:49:51Z nanne.hemstra@dgbes.com $";

#include "uihorizontracksetup.h"

#include "draw.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "horizonadjuster.h"
#include "horizon2dseedpicker.h"
#include "horizon3dseedpicker.h"
#include "horizon2dtracker.h"
#include "horizon3dtracker.h"
#include "mpeengine.h"
#include "randcolor.h"
#include "sectiontracker.h"
#include "separstr.h"
#include "survinfo.h"

#include "uibutton.h"
#include "uibuttongroup.h"
#include "uicolor.h"
#include "uidialog.h"
#include "uiflatviewer.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimpecorrelationgrp.h"
#include "uimsg.h"
#include "uiseissel.h"
#include "uiseparator.h"
#include "uislider.h"
#include "uitable.h"
#include "uitabstack.h"
#include "od_helpids.h"


#define mErrRet(s) { uiMSG().error( s ); return false; }

namespace MPE
{

void uiBaseHorizonSetupGroup::initClass()
{
    uiMPE().setupgrpfact.addFactory( uiBaseHorizonSetupGroup::create,
				     Horizon2DTracker::keyword() );
    uiMPE().setupgrpfact.addFactory( uiBaseHorizonSetupGroup::create,
				     Horizon3DTracker::keyword() );
}


uiSetupGroup* uiBaseHorizonSetupGroup::create( uiParent* p, const char* typestr)
{
    const FixedString type( typestr );
    if ( type != EM::Horizon3D::typeStr() && type != EM::Horizon2D::typeStr() )
	return 0;

    return new uiBaseHorizonSetupGroup( p, typestr );
}


uiBaseHorizonSetupGroup::uiBaseHorizonSetupGroup( uiParent* p,
						  const char* typestr )
    : uiHorizonSetupGroup( p, typestr )
{}


static const char* horsetup_event_names[] = { "Min", "Max", "0+-", "0-+", 0 };

const char** uiHorizonSetupGroup::sKeyEventNames()
{
    return horsetup_event_names;
}


#define mComma ,
const VSEvent::Type* uiHorizonSetupGroup::cEventTypes()
{
    mDefineStaticLocalObject( const VSEvent::Type, event_types, [] =
		    { VSEvent::Min mComma VSEvent::Max mComma
		      VSEvent::ZCPosNeg mComma VSEvent::ZCNegPos } );
    return event_types;
}


uiHorizonSetupGroup::uiHorizonSetupGroup( uiParent* p,
					  const char* typestr )
    : uiSetupGroup(p,"")
    , sectiontracker_(0)
    , horadj_(0)
    , addstepbut_(0)
    , is2d_(FixedString(typestr)==EM::Horizon2D::typeStr())
    , modeChanged_(this)
    , eventChanged_(this)
    , varianceChanged_(this)
    , propertyChanged_(this)
{
    tabgrp_ = new uiTabStack( this, "TabStack" );
    uiGroup* modegrp = createModeGroup();
    tabgrp_->addTab( modegrp, tr("Mode") );

    uiGroup* eventgrp = createEventGroup();
    tabgrp_->addTab( eventgrp, tr("Event") );

    correlationgrp_ = new uiCorrelationGroup( tabgrp_->tabGroup() );
    tabgrp_->addTab( correlationgrp_, tr("Correlation") );

//    uiGroup* vargrp = createVarianceGroup();
//    tabgrp_->addTab( vargrp, tr("Variance") );

    uiGroup* propertiesgrp = createPropertyGroup();
    tabgrp_->addTab( propertiesgrp, uiStrings::sProperties(true) );
}


uiGroup* uiHorizonSetupGroup::createModeGroup()
{
    uiGroup* grp = new uiGroup( tabgrp_->tabGroup(), "Mode" );

    modeselgrp_ = new uiButtonGroup( grp, "ModeSel", OD::Vertical );
    modeselgrp_->setExclusive( true );
    grp->setHAlignObj( modeselgrp_ );

    if ( !is2d_ && Horizon3DSeedPicker::nrSeedConnectModes()>0 )
    {
	for ( int idx=0; idx<Horizon3DSeedPicker::nrSeedConnectModes(); idx++ )
	{
	    uiRadioButton* butptr = new uiRadioButton( modeselgrp_,
			Horizon3DSeedPicker::seedConModeText(idx,false) );
	    butptr->activated.notify(
			mCB(this,uiHorizonSetupGroup,seedModeChange) );

	    mode_ = (EMSeedPicker::SeedModeOrder)
				Horizon3DSeedPicker::defaultSeedConMode();
	}
    }
    else if ( is2d_ && Horizon2DSeedPicker::nrSeedConnectModes()>0 )
    {
	for ( int idx=0; idx<Horizon2DSeedPicker::nrSeedConnectModes(); idx++ )
	{
	    uiRadioButton* butptr = new uiRadioButton( modeselgrp_,
			Horizon2DSeedPicker::seedConModeText(idx,false) );
	    butptr->activated.notify(
			mCB(this,uiHorizonSetupGroup,seedModeChange) );

	    mode_ = (EMSeedPicker::SeedModeOrder)
				Horizon2DSeedPicker::defaultSeedConMode();
	}
    }

    uiSeparator* sep = new uiSeparator( grp );
    sep->attach( stretchedBelow, modeselgrp_ );
    BufferStringSet strs; strs.add( "Seed Trace" ).add( "Adjacent Parent" );
    methodfld_ = new uiGenInput( grp, tr("Method"), StringListInpSpec(strs) );
    methodfld_->attach( alignedBelow, modeselgrp_ );
    methodfld_->attach( ensureBelow, sep );

    return grp;
}


uiGroup* uiHorizonSetupGroup::createEventGroup()
{
    uiGroup* grp = new uiGroup( tabgrp_->tabGroup(), "Event" );

    evfld_ = new uiGenInput( grp, tr("Event type"),
			     StringListInpSpec(sKeyEventNames()) );
    evfld_->valuechanged.notify( mCB(this,uiHorizonSetupGroup,selEventType) );
    evfld_->valuechanged.notify( mCB(this,uiHorizonSetupGroup,eventChangeCB) );
    grp->setHAlignObj( evfld_ );

    BufferString srchwindtxt( "Search window ", SI().getZUnitString() );
    const StepInterval<int> intv( -10000, 10000, 1 );
    IntInpSpec iis; iis.setLimits( intv );
    srchgatefld_ = new uiGenInput( grp, srchwindtxt, iis, iis );
    srchgatefld_->attach( alignedBelow, evfld_ );
    srchgatefld_->valuechanged.notify(
	    mCB(this,uiHorizonSetupGroup,eventChangeCB) );

    thresholdtypefld_ = new uiGenInput( grp, tr("Threshold type"),
		BoolInpSpec(true,tr("Cut-off amplitude"),
				 tr("Relative difference")) );
    thresholdtypefld_->valuechanged.notify(
	    mCB(this,uiHorizonSetupGroup,selAmpThresholdType) );
    thresholdtypefld_->attach( alignedBelow, srchgatefld_ );

    ampthresholdfld_ = new uiGenInput ( grp, tr("Allowed difference (%)"),
				       StringInpSpec() );
    ampthresholdfld_->attach( alignedBelow, thresholdtypefld_ );
    ampthresholdfld_->valuechanged.notify(
	    mCB(this,uiHorizonSetupGroup,eventChangeCB) );

    if ( !is2d_ )
    {
	addstepbut_ = new uiPushButton( grp, tr("Steps"),
		mCB(this,uiHorizonSetupGroup,addStepPushedCB), false );
	addstepbut_->attach( rightTo, ampthresholdfld_ );
    }

    extriffailfld_ = new uiGenInput( grp, tr("If tracking fails"),
		BoolInpSpec(true,tr("Extrapolate"),uiStrings::sStop()) );
    extriffailfld_->attach( alignedBelow, ampthresholdfld_ );
    extriffailfld_->valuechanged.notify(
		mCB(this,uiHorizonSetupGroup,eventChangeCB) );

    return grp;
}


uiGroup* uiHorizonSetupGroup::createVarianceGroup()
{
    uiGroup* grp = new uiGroup( tabgrp_->tabGroup(), "Variance" );

    usevarfld_ = new uiGenInput( grp, tr("Use Variance"), BoolInpSpec(false) );
    usevarfld_->valuechanged.notify(
	    mCB(this,uiHorizonSetupGroup,selUseVariance) );
    usevarfld_->valuechanged.notify(
	    mCB(this,uiHorizonSetupGroup,varianceChangeCB) );

    const IOObjContext ctxt =
	uiSeisSel::ioContext( is2d_ ? Seis::Line : Seis::Vol, true );
    uiSeisSel::Setup ss( is2d_, false );
    variancefld_ = new uiSeisSel( grp, ctxt, ss );
    variancefld_->attach( alignedBelow, usevarfld_ );

    varthresholdfld_ =
	new uiGenInput( grp, tr("Variance threshold"), FloatInpSpec() );
    varthresholdfld_->attach( alignedBelow, variancefld_ );
    varthresholdfld_->valuechanged.notify(
	    mCB(this,uiHorizonSetupGroup,varianceChangeCB) );

    grp->setHAlignObj( usevarfld_ );
    return grp;

}


uiGroup* uiHorizonSetupGroup::createPropertyGroup()
{
    uiGroup* grp = new uiGroup( tabgrp_->tabGroup(), "Properties" );
    colorfld_ = new uiColorInput( grp,
				uiColorInput::Setup(getRandStdDrawColor() )
				.withdesc(false).lbltxt(tr("Horizon Color")) );
    colorfld_->colorChanged.notify(
			mCB(this,uiHorizonSetupGroup,colorChangeCB) );
    grp->setHAlignObj( colorfld_ );

    seedtypefld_ = new uiGenInput( grp, tr("Seed Shape/Color"),
			StringListInpSpec(MarkerStyle3D::TypeNames()) );
    seedtypefld_->valuechanged.notify(
			mCB(this,uiHorizonSetupGroup,seedTypeSel) );
    seedtypefld_->attach( alignedBelow, colorfld_ );

    seedcolselfld_ = new uiColorInput( grp,
				uiColorInput::Setup(Color::White())
				.withdesc(false) );
    seedcolselfld_->attach( rightTo, seedtypefld_ );
    seedcolselfld_->colorChanged.notify(
				mCB(this,uiHorizonSetupGroup,seedColSel) );

    seedsliderfld_ = new uiSlider( grp,
				uiSlider::Setup(tr("Seed Size")).
				withedit(true),	"Seed Size" );
    seedsliderfld_->setInterval( 1, 15 );
    seedsliderfld_->valueChanged.notify(
			mCB(this,uiHorizonSetupGroup,seedSliderMove));
    seedsliderfld_->attach( alignedBelow, seedtypefld_ );

    return grp;
}


uiHorizonSetupGroup::~uiHorizonSetupGroup()
{
}


NotifierAccess*	uiHorizonSetupGroup::correlationChangeNotifier()
{ return correlationgrp_->correlationChangeNotifier(); }


void uiHorizonSetupGroup::selUseVariance( CallBacker* )
{
    const bool usevar = usevarfld_->getBoolValue();
    variancefld_->setSensitive( usevar );
    varthresholdfld_->setSensitive( usevar );
}


void uiHorizonSetupGroup::selAmpThresholdType( CallBacker* )
{
    const bool absthreshold = thresholdtypefld_->getBoolValue();
    ampthresholdfld_->setTitleText(absthreshold ?tr("Amplitude value")
						:tr("Allowed difference (%)"));
    if ( absthreshold )
    {
	if ( is2d_ || horadj_->getAmplitudeThresholds().isEmpty() )
	    ampthresholdfld_->setValue( horadj_->amplitudeThreshold() );
	else
	{
	    BufferString bs;
	    bs += horadj_->getAmplitudeThresholds()[0];
	    for (int idx=1;idx<horadj_->getAmplitudeThresholds().size();idx++)
	    { bs += ","; bs += horadj_->getAmplitudeThresholds()[idx]; }
	    ampthresholdfld_->setText( bs.buf() );
	}
    }
    else
    {
	if ( is2d_ || horadj_->getAllowedVariances().isEmpty() )
	    ampthresholdfld_->setValue( horadj_->allowedVariance()*100 );
	else
	{
	    BufferString bs;
	    bs += horadj_->getAllowedVariances()[0]*100;
	    for ( int idx=1; idx<horadj_->getAllowedVariances().size(); idx++ )
	    { bs += ","; bs += horadj_->getAllowedVariances()[idx]*100; }
	    ampthresholdfld_->setText( bs.buf() );
	}
    }
}


void uiHorizonSetupGroup::selEventType( CallBacker* )
{
    const VSEvent::Type ev = cEventTypes()[ evfld_->getIntValue() ];
    const bool thresholdneeded = ev==VSEvent::Min || ev==VSEvent::Max;
    thresholdtypefld_->setSensitive( thresholdneeded );
    ampthresholdfld_->setSensitive( thresholdneeded );
}


void uiHorizonSetupGroup::seedModeChange( CallBacker* )
{
    mode_ = (EMSeedPicker::SeedModeOrder) modeselgrp_->selectedId();
    modeChanged_.trigger();
}


void uiHorizonSetupGroup::eventChangeCB( CallBacker* )
{ eventChanged_.trigger(); }


void uiHorizonSetupGroup::varianceChangeCB(CallBacker *)
{ varianceChanged_.trigger(); }


void uiHorizonSetupGroup::colorChangeCB( CallBacker* )
{
    propertyChanged_.trigger();
}


void uiHorizonSetupGroup::seedTypeSel( CallBacker* )
{
    const MarkerStyle3D::Type newtype =
	(MarkerStyle3D::Type)(MarkerStyle3D::None+seedtypefld_->getIntValue());
    if ( markerstyle_.type_ == newtype )
	return;
    markerstyle_.type_ = newtype;
    propertyChanged_.trigger();
}


void uiHorizonSetupGroup::seedSliderMove( CallBacker* )
{
    const float sldrval = seedsliderfld_->getValue();
    const int newsize = mNINT32(sldrval);
    if ( markerstyle_.size_ == newsize )
	return;
    markerstyle_.size_ = newsize;
    propertyChanged_.trigger();
}


void uiHorizonSetupGroup::seedColSel( CallBacker* )
{
    const Color newcolor = seedcolselfld_->color();
    if ( markerstyle_.color_ == newcolor )
	return;
    markerstyle_.color_ = newcolor;
    propertyChanged_.trigger();
}


class uiStepDialog : public uiDialog
{
public:

uiStepDialog( uiParent* p, const char* valstr )
    : uiDialog(p,Setup("Stepwise tracking",uiStrings::sEmptyString(),
                       mODHelpKey(mTrackingWizardHelpID) ))
{
    steptable_ = new uiTable( this, uiTable::Setup(5,1).rowdesc("Step")
						       .rowgrow(true)
						       .defrowlbl(true)
						       .selmode(uiTable::Multi)
						       .defrowlbl(""),
			      "Stepwise tracking table" );
    steptable_->setColumnLabel( 0, "Value" );

    SeparString ss( valstr, ',' );
    if ( ss.size() > 3 )
	steptable_->setNrRows( ss.size() + 2 );

    for ( int idx=0; idx<ss.size(); idx++ )
	steptable_->setText( RowCol(idx,0), ss[idx] );
}


void getValueString( BufferString& valstr )
{
    SeparString ss( 0, ',' );
    for ( int idx=0; idx<steptable_->nrRows(); idx++ )
    {
	const char* valtxt = steptable_->text( RowCol(idx,0) );
	if ( !valtxt || !*valtxt ) continue;
	ss.add( valtxt );
    }

    valstr = ss.buf();
}

    uiTable*	steptable_;
};


void uiHorizonSetupGroup::addStepPushedCB(CallBacker*)
{
    uiStepDialog dlg( this, ampthresholdfld_->text() );
    if ( dlg.go() )
    {
	BufferString valstr;
	dlg.getValueString( valstr );
	ampthresholdfld_->setText( valstr );
	propertyChanged_.trigger();
    }
}


void uiHorizonSetupGroup::setSectionTracker( SectionTracker* st )
{
    sectiontracker_ = st;
    mDynamicCastGet(HorizonAdjuster*,horadj,sectiontracker_->adjuster())
    horadj_ = horadj;
    if ( !horadj_ ) return;

    initStuff();
    correlationgrp_->setSectionTracker( st );
}


void uiHorizonSetupGroup::initModeGroup()
{
    if ( (!is2d_ && Horizon3DSeedPicker::nrSeedConnectModes()>0) ||
	 (is2d_ && Horizon2DSeedPicker::nrSeedConnectModes()>0) )
	modeselgrp_->selectButton( mode_ );
}


void uiHorizonSetupGroup::initStuff()
{
    initModeGroup();
    initEventGroup();
    selEventType(0);
    selAmpThresholdType(0);
//    initVarianceGroup();
//    selUseVariance(0);
    initPropertyGroup();
}


void uiHorizonSetupGroup::initEventGroup()
{
    VSEvent::Type ev = horadj_->trackEvent();
    const int fldidx = ev == VSEvent::Min ? 0
			    : (ev == VSEvent::Max ? 1
			    : (ev == VSEvent::ZCPosNeg ? 2 : 3) );
    evfld_->setValue( fldidx );

    Interval<float> srchintv( horadj_->permittedZRange() );
    srchintv.scale( mCast(float,SI().zDomain().userFactor()) );
    srchgatefld_->setValue( srchintv );

    thresholdtypefld_->setValue( horadj_->useAbsThreshold() );
    extriffailfld_->setValue( !horadj_->removesOnFailure() );
}


void uiHorizonSetupGroup::initVarianceGroup()
{
}


void uiHorizonSetupGroup::initPropertyGroup()
{
    seedsliderfld_->setValue( markerstyle_.size_ );
    seedcolselfld_->setColor( markerstyle_.color_ );
    seedtypefld_->setValue( markerstyle_.type_ - MarkerStyle3D::None );
}


void uiHorizonSetupGroup::setMode( EMSeedPicker::SeedModeOrder mode )
{
    mode_ = mode;
    modeselgrp_->selectButton( mode_ );
}


int uiHorizonSetupGroup::getMode()
{
    return modeselgrp_ ? modeselgrp_->selectedId() : -1;
}


void uiHorizonSetupGroup::setSeedPos( const Coord3& crd )
{
    correlationgrp_->setSeedPos( crd );
}


void uiHorizonSetupGroup::setColor( const Color& col)
{
    colorfld_->setColor( col );
}


const Color& uiHorizonSetupGroup::getColor()
{
    return colorfld_->color();
}


void uiHorizonSetupGroup::setMarkerStyle( const MarkerStyle3D& markerstyle )
{
    markerstyle_ = markerstyle;
    initPropertyGroup();
}


const MarkerStyle3D& uiHorizonSetupGroup::getMarkerStyle()
{
    return markerstyle_;
}


bool uiHorizonSetupGroup::commitToTracker( bool& fieldchange ) const
{
    fieldchange = false;
    correlationgrp_->commitToTracker( fieldchange );

    if ( !horadj_ || horadj_->getNrAttributes()<1 )
    {   uiMSG().warning( tr("Unable to apply tracking setup") );
	return true;
    }

    VSEvent::Type evtyp = cEventTypes()[ evfld_->getIntValue() ];
    if ( horadj_->trackEvent() != evtyp )
    {
	fieldchange = true;
	horadj_->setTrackEvent( evtyp );
    }

    Interval<float> intv = srchgatefld_->getFInterval();
    if ( intv.start>0 || intv.stop<0 || intv.start==intv.stop )
	mErrRet( tr("Search window should be minus to positive, ex. -20, 20"));
    Interval<float> relintv( (float)intv.start/SI().zDomain().userFactor(),
			     (float)intv.stop/SI().zDomain().userFactor() );
    if ( horadj_->permittedZRange() != relintv )
    {
	fieldchange = true;
	horadj_->setPermittedZRange( relintv );
    }

    const bool useabs = thresholdtypefld_->getBoolValue();
    if ( horadj_->useAbsThreshold() != useabs )
    {
	fieldchange = true;
	horadj_->setUseAbsThreshold( useabs );
    }

    if ( useabs )
    {
	SeparString ss( ampthresholdfld_->text(), ',' );
	int idx = 0;
	if ( ss.size() < 2 )
	{
	    float vgate = ss.getFValue(0);
	    if ( Values::isUdf(vgate) )
		mErrRet( tr("Value threshold not set") );
	    if ( horadj_->amplitudeThreshold() != vgate )
	    {
		fieldchange = true;
		horadj_->setAmplitudeThreshold( vgate );
	    }
	}
	else
	{
	    TypeSet<float> vars;
	    for ( ; idx<ss.size(); idx++ )
	    {
		float varvalue = ss.getFValue(idx);
		if ( Values::isUdf(varvalue) )
		    mErrRet( tr("Value threshold not set properly") );

		if ( horadj_->getAmplitudeThresholds().size() < idx+1 )
		{
		    fieldchange = true;
		    horadj_->getAmplitudeThresholds() += varvalue;
		    if ( idx == 0 )
			horadj_->setAmplitudeThreshold( varvalue );
		}
		else if ( horadj_->getAmplitudeThresholds().size() >= idx+1 )
		    if ( horadj_->getAmplitudeThresholds()[idx] != varvalue )
		    {
			fieldchange = true;
			horadj_->getAmplitudeThresholds() += varvalue;
			if ( idx == 0 )
			    horadj_->setAmplitudeThreshold( varvalue );
		    }
	    }
	}

	if ( idx==0 && horadj_->getAmplitudeThresholds().size() > 0 )
	{
	    horadj_->getAmplitudeThresholds()[idx] =
				horadj_->amplitudeThreshold();
	    idx++;
	}

	if ( horadj_->getAmplitudeThresholds().size() > idx )
	{
	    int size = horadj_->getAmplitudeThresholds().size();
	    fieldchange = true;
	    horadj_->getAmplitudeThresholds().removeRange( idx, size-1 );
	}
    }
    else
    {
	SeparString ss( ampthresholdfld_->text(), ',' );
	int idx = 0;
	if ( ss.size() < 2 )
	{
	    float var = ss.getFValue(0) / 100;
	    if ( var<=0.0 || var>=1.0 )
		mErrRet( tr("Allowed variance must be between 0-100") );
	    if ( horadj_->allowedVariance() != var )
	    {
		fieldchange = true;
		horadj_->setAllowedVariance( var );
	    }
	}
	else
	{
	    TypeSet<float> vars;
	    for ( ; idx<ss.size(); idx++ )
	    {
		float varvalue = ss.getFValue(idx) / 100;
		if ( varvalue <=0.0 || varvalue>=1.0 )
		    mErrRet( tr("Allowed variance must be between 0-100") );

		if ( horadj_->getAllowedVariances().size() < idx+1 )
		{
		    fieldchange = true;
		    horadj_->getAllowedVariances() += varvalue;
		    if ( idx == 0 )
			horadj_->setAllowedVariance( varvalue );
		}
		else if ( horadj_->getAllowedVariances().size() >= idx+1 )
		    if ( horadj_->getAllowedVariances()[idx] != varvalue )
		    {
			fieldchange = true;
			horadj_->getAllowedVariances()[idx] = varvalue;
			if ( idx == 0 )
			    horadj_->setAllowedVariance( varvalue );
		    }
	    }
	}

	if ( idx==0 && horadj_->getAllowedVariances().size()>0 )
	{
	    horadj_->getAllowedVariances()[idx] = horadj_->allowedVariance();
	    idx++;
	}

	if (  horadj_->getAllowedVariances().size() > idx )
	{
	    int size = horadj_->getAllowedVariances().size();
	    fieldchange = true;
	    horadj_->getAllowedVariances().removeRange( idx, size-1 );
	}
    }

    const bool rmonfail = !extriffailfld_->getBoolValue();
    if ( horadj_->removesOnFailure() != rmonfail )
    {
	fieldchange = true;
	horadj_->removeOnFailure( rmonfail );
    }

    return true;
}


void uiHorizonSetupGroup::showGroupOnTop( const char* grpnm )
{
    tabgrp_->setCurrentPage( grpnm );
    mDynamicCastGet(uiDialog*,dlg,parent())
    if ( dlg && !dlg->isHidden() )
    {
	 dlg->showNormal();
	 dlg->raise();
    }
}


} //namespace MPE
