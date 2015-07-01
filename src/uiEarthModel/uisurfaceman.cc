/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          August 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uisurfaceman.h"

#include "ascstream.h"
#include "ctxtioobj.h"
#include "file.h"
#include "ioman.h"
#include "ioobj.h"
#include "multiid.h"
#include "oddirs.h"
#include "od_iostream.h"
#include "survinfo.h"

#include "embodytr.h"
#include "emfaultauxdata.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "emmarchingcubessurface.h"
#include "emsurfaceauxdata.h"
#include "emsurfacetr.h"
#include "emsurfauxdataio.h"

#include "uibodyoperatordlg.h"
#include "uibodyregiondlg.h"
#include "uicolor.h"
#include "uifont.h"
#include "uigeninputdlg.h"
#include "uihorizonmergedlg.h"
#include "uihorizonrelations.h"
#include "uiimpbodycaldlg.h"
#include "uilistbox.h"
#include "uiioobjmanip.h"
#include "uiioobjselgrp.h"
#include "uiiosurfacedlg.h"
#include "uimsg.h"
#include "uisplitter.h"
#include "uistratlvlsel.h"
#include "uistrattreewin.h"
#include "uitable.h"
#include "uitaskrunner.h"
#include "uitextedit.h"
#include "uitoolbutton.h"
#include "od_helpids.h"


DefineEnumNames(uiSurfaceMan,Type,0,"Surface type")
{
    EMHorizon2DTranslatorGroup::keyword(),
    EMHorizon3DTranslatorGroup::keyword(),
    EMAnyHorizonTranslatorGroup::keyword(),
    EMFaultStickSetTranslatorGroup::keyword(),
    EMFault3DTranslatorGroup::keyword(),
    EMBodyTranslatorGroup::keyword(),
    0
};

mDefineInstanceCreatedNotifierAccess(uiSurfaceMan)



#define mCaseRetCtxt(enm,trgrpnm) \
    case uiSurfaceMan::enm: return trgrpnm##TranslatorGroup::ioContext()

static IOObjContext getIOCtxt( uiSurfaceMan::Type typ )
{
    switch ( typ )
    {
	mCaseRetCtxt(Hor2D,EMHorizon2D);
	mCaseRetCtxt(Hor3D,EMHorizon3D);
	mCaseRetCtxt(AnyHor,EMAnyHorizon);
	mCaseRetCtxt(StickSet,EMFaultStickSet);
	mCaseRetCtxt(Flt3D,EMFault3D);
	default:
	mCaseRetCtxt(Body,EMBody);
    }
}

#define mCaseRetStr(enm,str) \
    case uiSurfaceMan::enm: return BufferString( act, " ", str )

static BufferString getActStr( uiSurfaceMan::Type typ, const char* act )
{
    switch ( typ )
    {
	mCaseRetStr(Hor2D,"2D Horizons");
	mCaseRetStr(Hor3D,"3D Horizons");
	mCaseRetStr(StickSet,"FaultStickSets");
	mCaseRetStr(Flt3D,"Faults");
	mCaseRetStr(Body,"Bodies");
	default:
	mCaseRetStr(AnyHor,"Horizons");
    }
}

static HelpKey getHelpID( uiSurfaceMan::Type typ )
{
    switch ( typ )
    {
case uiSurfaceMan::Hor2D:	return mODHelpKey(mSurface2DManHelpID);
case uiSurfaceMan::StickSet:	return mODHelpKey(mFaultStickSetsManageHelpID);
case uiSurfaceMan::Flt3D:	return mODHelpKey(mFaultsManageHelpID);
case uiSurfaceMan::Body:	return mODHelpKey(mBodyManHelpID);
default:			return mODHelpKey(mSurfaceManHelpID);
    }
}


uiSurfaceMan::uiSurfaceMan( uiParent* p, uiSurfaceMan::Type typ )
    : uiObjFileMan(p,uiDialog::Setup(getActStr(typ,"Manage"),mNoDlgTitle,
				     getHelpID(typ)).nrstatusflds(1)
						    .modal(false),
		   getIOCtxt(typ) )
    , type_(typ)
    , attribfld_(0)
    , man2dbut_(0)
    , surfdatarenamebut_(0)
    , surfdataremovebut_(0)
    , copybut_(0)
    , mergehorbut_(0)
    , applybodybut_(0)
    , createregbodybut_(0)
    , volestimatebut_(0)
    , switchvalbut_(0)
{
    createDefaultUI();
    uiIOObjManipGroup* manipgrp = selgrp_->getManipGroup();

    if ( type_ != Body )
	copybut_ = manipgrp->addButton( "copyobj", "Copy to new object",
					mCB(this,uiSurfaceMan,copyCB) );

    if ( type_ == Hor2D || type_ == AnyHor )
    {
	man2dbut_ = manipgrp->addButton( "man2d", "Manage 2D Horizons",
					 mCB(this,uiSurfaceMan,man2dCB) );
	man2dbut_->setSensitive( false );
    }
    if ( type_ == Hor3D )
    {
	mergehorbut_ = manipgrp->addButton( "mergehorizons","Merge 3D Horizons",
					    mCB(this,uiSurfaceMan,merge3dCB) );
    }
    if ( type_ == Hor3D || type_ == AnyHor )
    {
	uiLabeledListBox* llb =
		new uiLabeledListBox( listgrp_, tr("Horizon Data"),
			OD::ChooseAtLeastOne, uiLabeledListBox::AboveLeft );
	llb->attach( rightOf, selgrp_ );
	attribfld_ = llb->box();
	attribfld_->setHSzPol( uiObject::Wide );
	attribfld_->setToolTip(
		tr("Horizon Data (Attributes stored in Horizon format)") );
	attribfld_->selectionChanged.notify( mCB(this,uiSurfaceMan,attribSel) );

	uiManipButGrp* butgrp = new uiManipButGrp( llb );
	surfdataremovebut_ = butgrp->addButton( uiManipButGrp::Remove,
					"Remove selected Horizon Data",
					mCB(this,uiSurfaceMan,removeAttribCB) );
	surfdatarenamebut_ = butgrp->addButton( uiManipButGrp::Rename,
					"Rename selected Horizon Data",
					mCB(this,uiSurfaceMan,renameAttribCB) );
	butgrp->attach( rightTo, attribfld_ );

	uiGroup* extrabutgrp = new uiGroup( listgrp_, "Extra Buttons" );
	const uiFont& ft =
		uiFontList::getInst().get( FontData::key(FontData::Control) );
	extrabutgrp->setPrefHeight( ft.height()*2 );

	uiPushButton* stratbut =
	    new uiPushButton( extrabutgrp, tr("Stratigraphy"), false );
	stratbut->activated.notify( mCB(this,uiSurfaceMan,stratSel) );

	uiPushButton* relbut =
		new uiPushButton( extrabutgrp, tr("Relations"), false);
	relbut->activated.notify( mCB(this,uiSurfaceMan,setRelations) );
	relbut->attach( rightTo, stratbut );

	extrabutgrp->attach( alignedBelow, selgrp_ );
	extrabutgrp->attach( ensureBelow, llb );

	setPrefWidth( 50 );
    }
    if ( type_ == Flt3D )
    {
#ifdef __debug__
	uiLabeledListBox* llb =
		new uiLabeledListBox( listgrp_, tr("Fault Data"),
			OD::ChooseAtLeastOne, uiLabeledListBox::AboveLeft );
	llb->attach( rightOf, selgrp_ );
	attribfld_ = llb->box();
	attribfld_->setToolTip(
		tr("Fault Data (Attributes stored in Fault format)") );

	uiManipButGrp* butgrp = new uiManipButGrp( llb );
	surfdataremovebut_ = butgrp->addButton( uiManipButGrp::Remove,
					"Remove selected Fault Data",
					mCB(this,uiSurfaceMan,removeAttribCB) );
	surfdatarenamebut_ = butgrp->addButton( uiManipButGrp::Rename,
					"Rename selected Fault Data",
					mCB(this,uiSurfaceMan,renameAttribCB) );
	butgrp->attach( rightTo, attribfld_ );
#endif
    }
    if ( type_ == Body )
    {
	applybodybut_ =manipgrp->addButton( "set_union",
					   "Apply Body operations",
					   mCB(this,uiSurfaceMan,mergeBodyCB) );
	createregbodybut_ = manipgrp->addButton( "set_implicit",
						 "Create region Body",
				mCB(this,uiSurfaceMan,createBodyRegionCB) );
	volestimatebut_ = manipgrp->addButton( "bodyvolume", "Volume estimate",
					      mCB(this,uiSurfaceMan,calVolCB) );
	switchvalbut_ = manipgrp->addButton( "switch_implicit",
					       "Switch inside/outside value",
					mCB(this,uiSurfaceMan,switchValCB) );
    }

    mTriggerInstanceCreatedNotifier();
    selChg( this );
}


uiSurfaceMan::~uiSurfaceMan()
{}


void uiSurfaceMan::ownSelChg()
{
    setToolButtonProperties();
}


void uiSurfaceMan::attribSel( CallBacker* )
{
    setToolButtonProperties();
}


#define mSetButToolTip(but,str1,curattribnms,str2,deftt) \
    if ( but ) \
    { \
	if ( but->sensitive() ) \
	{ \
	    tt.setEmpty(); \
	    tt.add( str1 ).add( curattribnms ).add( str2 ); \
	    but->setToolTip( tr(tt) ); \
	} \
	else \
	{ \
	    but->setToolTip( deftt ); \
	} \
    }

void uiSurfaceMan::setToolButtonProperties()
{
    const bool hasattribs = attribfld_ && !attribfld_->isEmpty();

    BufferString tt, cursel;
    if ( curioobj_ )
	cursel.add( curioobj_->name() );

    if ( surfdatarenamebut_ )
    {
	surfdatarenamebut_->setSensitive( hasattribs );
	mSetButToolTip(surfdatarenamebut_,"Rename '",attribfld_->getText(),"'",
		       "Rename selected Data")
    }

    if ( surfdataremovebut_ )
    {
	surfdataremovebut_->setSensitive( hasattribs );
	BufferStringSet attrnms;
	attribfld_->getChosen( attrnms );
	mSetButToolTip(surfdataremovebut_,"Remove ",attrnms.getDispString(2),
		       "", "Remove selected data")
    }

    if ( copybut_ )
    {
	copybut_->setSensitive( curioobj_ );
	mSetButToolTip(copybut_,"Copy '",cursel,"' to new object",
		       "Copy to new object")
    }

    if ( mergehorbut_ )
    {
	mergehorbut_->setSensitive( curioobj_ );
	BufferStringSet selhornms;
	selgrp_->getChosen( selhornms );
	if ( selhornms.size() > 1 )
	{
	    mSetButToolTip(mergehorbut_,"Merge",selhornms.getDispString(2),"",
			   "Merge 3D horizons")
	}
	else
	    mergehorbut_->setToolTip( "Merge 3D horizons" );
    }

    if ( type_ == Body )
    {
	applybodybut_->setSensitive( curioobj_ );
	createregbodybut_->setSensitive( curioobj_ );
	volestimatebut_->setSensitive( curioobj_ );
	switchvalbut_->setSensitive( curioobj_ );
	mSetButToolTip(volestimatebut_,"Estimate volume of '",cursel,"'",
		       "Volume estimate");
	mSetButToolTip(switchvalbut_,"Switch inside/outside value of '",
		       cursel,"'","Switch inside/outside value");
    }
}


void uiSurfaceMan::addTool( uiButton* but )
{
    uiObjFileMan::addTool( but );
    if ( !lastexternal_ && attribfld_ )
	but->attach( alignedBelow, attribfld_ );
}


bool uiSurfaceMan::isCur2D() const
{
    return curioobj_ &&
	   curioobj_->group() == EMHorizon2DTranslatorGroup::keyword();
}


bool uiSurfaceMan::isCurFault() const
{
    const BufferString grp = curioobj_ ? curioobj_->group().buf() : "";
    return grp==EMFaultStickSetTranslatorGroup::keyword() ||
	   grp==EMFault3DTranslatorGroup::keyword();
}


void uiSurfaceMan::copyCB( CallBacker* )
{
    if ( !curioobj_ ) return;

    const bool canhaveattribs = type_ == uiSurfaceMan::Hor3D;
    PtrMan<IOObj> ioobj = curioobj_->clone();
    uiSurfaceRead::Setup su( ioobj->group() );
    su.withattribfld(canhaveattribs).withsubsel(!isCurFault())
      .multisubsel(true).withsectionfld(false);

    uiCopySurface dlg( this, *ioobj, su );
    if ( dlg.go() )
	selgrp_->fullUpdate( ioobj->key() );
}


void uiSurfaceMan::merge3dCB( CallBacker* )
{
    uiHorizonMergeDlg dlg( this, false );
    TypeSet<MultiID> chsnmids;
    selgrp_->getChosen( chsnmids );
    dlg.setInputHors( chsnmids );
    if ( dlg.go() )
	selgrp_->fullUpdate( dlg.getNewHorMid() );
}


void uiSurfaceMan::mergeBodyCB( CallBacker* )
{
    uiBodyOperatorDlg dlg( this );
    if ( dlg.go() )
	selgrp_->fullUpdate( dlg.getBodyMid() );
}


void uiSurfaceMan::calVolCB( CallBacker* )
{
    if ( !curioobj_ )
	return;

    RefMan<EM::EMObject> emo =
	EM::EMM().loadIfNotFullyLoaded( curioobj_->key(), 0 );
    mDynamicCastGet( EM::Body*, emb, emo.ptr() );
    if ( !emb )
    {
	BufferString msg( "Body '" );
	msg.add( curioobj_->name() ).add( "'" ).add( " is empty" );
	uiMSG().error( tr(msg.buf()) );
	return;
    }

    uiImplBodyCalDlg dlg( this, *emb );
    BufferString dlgtitle( "Body volume estimation for '" );
    dlgtitle.add( curioobj_->name() ).add( "'" );
    dlg.setTitleText( dlgtitle.buf() );
    dlg.go();
}


void uiSurfaceMan::createBodyRegionCB( CallBacker* )
{
    uiBodyRegionDlg dlg( this );
    if ( dlg.go() )
	selgrp_->fullUpdate( dlg.getBodyMid() );
}


void uiSurfaceMan::switchValCB( CallBacker* )
{
    uiImplicitBodyValueSwitchDlg dlg( this, curioobj_ );
    if ( dlg.go() )
	selgrp_->fullUpdate( dlg.getBodyMid() );
}


void uiSurfaceMan::setRelations( CallBacker* )
{
    uiHorizonRelationsDlg dlg( this, isCur2D() );
    dlg.go();
}


void uiSurfaceMan::removeAttribCB( CallBacker* )
{
    if ( !curioobj_ ) return;

    if ( curioobj_->implReadOnly() )
    {
	uiMSG().error(
		tr("Cannot remove Surface Data. Surface is read-only"));
	return;
    }

    BufferStringSet attrnms;
    attribfld_->getChosen( attrnms );
    BufferString msg;
    msg.add( attrnms.getDispString(2) )
       .add( "\nwill be removed from disk.\nDo you wish to continue?" );
    if ( !uiMSG().askRemove(tr(msg)) )
	return;

    if ( curioobj_->group()==EMFault3DTranslatorGroup::keyword() )
    {
	EM::FaultAuxData fad( curioobj_->key() );
	for ( int ida=0; ida<attrnms.size(); ida++ )
	    fad.removeData( attrnms.get(ida) );
    }
    else
    {
	for ( int ida=0; ida<attrnms.size(); ida++ )
	    EM::SurfaceAuxData::removeFile( *curioobj_, attrnms.get(ida) );
    }

    selChg( this );
}


#define mErrRet(msg) { uiMSG().error(msg); return; }

void uiSurfaceMan::renameAttribCB( CallBacker* )
{
    if ( !curioobj_ ) return;

    const BufferString attribnm = attribfld_->getText();
    BufferString titl( "Rename '" ); titl += attribnm; titl += "'";
    uiGenInputDlg dlg( this, titl, "New name", new StringInpSpec(attribnm) );
    if ( !dlg.go() ) return;

    const char* newnm = dlg.text();
    if ( attribfld_->isPresent(newnm) )
	mErrRet( tr("Name is already in use") )

    if ( curioobj_->group()==EMFault3DTranslatorGroup::keyword() )
    {
	EM::FaultAuxData fad( curioobj_->key() );
	fad.setDataName( attribnm, newnm );

	selChg( this );
	return;
    }

    const BufferString filename =
		EM::SurfaceAuxData::getFileName( *curioobj_, attribnm );
    if ( File::isEmpty(filename) )
	mErrRet( tr("Cannot find Horizon Data file") )
    else if ( !File::isWritable(filename) )
	mErrRet( tr("The Horizon Data file is not writable") )

    od_istream instrm( filename );
    if ( !instrm.isOK() )
	mErrRet( tr("Cannot open Horizon Data file for read") )
    const BufferString ofilename( filename, "_new" );
    od_ostream outstrm( ofilename );
    if ( !outstrm.isOK() )
	mErrRet( tr("Cannot open new Horizon Data file for write") )

    ascistream aistrm( instrm );
    ascostream aostrm( outstrm );
    aostrm.putHeader( aistrm.fileType() );
    IOPar iop( aistrm );
    iop.set( sKey::Attribute(), newnm );
    iop.putTo( aostrm );

    outstrm.add( instrm );
    const bool writeok = outstrm.isOK();
    instrm.close(); outstrm.close();

    BufferString tmpfnm( filename ); tmpfnm += "_old";
    if ( !writeok )
    {
	File::remove( ofilename );
	mErrRet( tr("Error during write. Reverting to old name") )
    }

    if ( File::rename(filename,tmpfnm) )
	File::rename(ofilename,filename);
    else
    {
	File::remove( ofilename );
	mErrRet( tr("Cannot rename file(s). Reverting to old name") )
    }

    if ( File::exists(tmpfnm) )
	File::remove( tmpfnm );

    selChg( this );
}


void uiSurfaceMan::fillAttribList( const BufferStringSet& strs )
{
    if ( !attribfld_ ) return;

    attribfld_->setEmpty();
    for ( int idx=0; idx<strs.size(); idx++)
	attribfld_->addItem( strs[idx]->buf() );
    attribfld_->chooseAll( false );
}


void uiSurfaceMan::mkFileInfo()
{
#define mAddRangeTxt(inl) \
    range = inl ? eminfo.getInlRange() : eminfo.getCrlRange(); \
    if ( range.isUdf() ) \
	txt += "-\n"; \
    else \
    { \
	txt += range.start; txt += " - "; txt += range.stop; \
	txt += " - "; txt += range.step; txt += "\n"; \
    }

    BufferString txt;
    EM::IOObjInfo eminfo( curioobj_ );
    if ( !eminfo.isOK() )
    {
	txt += eminfo.name(); txt.add( " has no file on disk (yet).\n" );
	setInfo( txt );
	return;
    }

    BufferStringSet attrnms;
    if ( eminfo.getAttribNames(attrnms) )
	fillAttribList( attrnms );

    if ( man2dbut_ )
	man2dbut_->setSensitive( isCur2D() );

    if ( isCur2D() || isCurFault() )
    {
	txt = isCur2D() ? "Nr. 2D lines: " : "Nr. Sticks: ";
	if ( isCurFault() )
	{
	    if ( eminfo.nrSticks() < 0 )
		txt += "Cannot determine number of sticks for this object type";
	    else
		txt += eminfo.nrSticks();
	}
	else
	{
	    BufferStringSet linenames;
	    if ( eminfo.getLineNames(linenames) )
		txt += linenames.size();
	    else
		txt += "-";
	}

	txt += "\n";
    }
    else
    {
	StepInterval<int> range;
	txt = "In-line range: "; mAddRangeTxt(true)
	txt += "Cross-line range: "; mAddRangeTxt(false)
	Interval<float> zrange = eminfo.getZRange();
	if ( !zrange.isUdf() )
	{
	    txt += "Z range"; txt += SI().getZUnitString(); txt += ": ";
	    txt += mNINT32( zrange.start * SI().zDomain().userFactor() );
	    txt += " - ";
	    txt += mNINT32( zrange.stop * SI().zDomain().userFactor() );
	    txt += "\n";
	}
    }

    txt += getFileInfo();

    BufferStringSet sectionnms;
    eminfo.getSectionNames( sectionnms );
    if ( sectionnms.size() > 1 )
    {
	txt += "Nr of sections: "; txt += sectionnms.size(); txt += "\n";
	for ( int idx=0; idx<sectionnms.size(); idx++ )
	{
	    txt += "\tPatch "; txt += idx+1; txt += ": ";
	    txt += sectionnms[idx]->buf(); txt += "\n";
	}
    }

    setInfo( txt );
    setToolButtonProperties();
}


double uiSurfaceMan::getFileSize( const char* filenm, int& nrfiles ) const
{
    if ( File::isEmpty(filenm) ) return -1;
    double totalsz = (double)File::getKbSize( filenm );
    nrfiles = 1;

    const BufferString basefnm( filenm );
    for ( int idx=0; ; idx++ )
    {
	BufferString fnm( basefnm ); fnm += "^"; fnm += idx; fnm += ".hov";
	if ( !File::exists(fnm) ) break;
	totalsz += (double)File::getKbSize( fnm );
	nrfiles++;
    }

    return totalsz;
}


class uiSurfaceStratDlg : public uiDialog
{
public:
uiSurfaceStratDlg( uiParent* p,  const ObjectSet<MultiID>& ids )
    : uiDialog(p,uiDialog::Setup("Stratigraphy",mNoDlgTitle,mNoHelpKey))
    , objids_(ids)
{
    tbl_ = new uiTable( this, uiTable::Setup(ids.size(),3),
			"Stratigraphy Table" );
    BufferStringSet lbls; lbls.add( "Name" ).add( "Color" ).add( "Marker" );
    tbl_->setColumnLabels( lbls );
    tbl_->setTableReadOnly( true );
    tbl_->setRowResizeMode( uiTable::Interactive );
    tbl_->setColumnResizeMode( uiTable::ResizeToContents );
    tbl_->setColumnStretchable( 2, true );
    tbl_->setPrefWidth( 400 );
    tbl_->doubleClicked.notify( mCB(this,uiSurfaceStratDlg,doCol) );

    uiToolButton* sb = new uiToolButton( this, "man_strat",
					"Edit Stratigraphy to define Markers",
					mCB(this,uiSurfaceStratDlg,doStrat) );
    sb->attach( rightOf, tbl_ );

    IOPar par;
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	par.setEmpty();
	if ( !EM::EMM().readDisplayPars(*ids[idx],par) )
	    continue;

	tbl_->setText( RowCol(idx,0), EM::EMM().objectName(*ids[idx]) );

	Color col( Color::White() );
	par.get( sKey::Color(), col );
	tbl_->setColor( RowCol(idx,1), col );

	uiStratLevelSel* levelsel = new uiStratLevelSel( 0, true, 0 );
	levelsel->selChange.notify( mCB(this,uiSurfaceStratDlg,lvlChg) );
	tbl_->setCellGroup( RowCol(idx,2), levelsel );
	int lvlid = -1;
	par.get( sKey::StratRef(), lvlid );
	levelsel->setID( lvlid );
    }
}


protected:

void doStrat( CallBacker* )
{ StratTWin().popUp(); }

void doCol( CallBacker* )
{
    const RowCol& cell = tbl_->notifiedCell();
    if ( cell.col() != 1 )
	return;

    mDynamicCastGet(uiStratLevelSel*,levelsel,
	tbl_->getCellGroup(RowCol(cell.row(),2)))
    const bool havelvl = levelsel && levelsel->getID() >= 0;
    if ( havelvl )
    {
	uiMSG().error( "Cannot change color of regional marker" );
	return;
    }

    Color newcol = tbl_->getColor( cell );
    if ( selectColor(newcol,this,"Horizon color") )
	tbl_->setColor( cell, newcol );

    tbl_->setSelected( cell, false );
}

void lvlChg( CallBacker* cb )
{
    mDynamicCastGet(uiStratLevelSel*,levelsel,cb)
    if ( !levelsel ) return;

    const Color col = levelsel->getColor();
    if ( col == Color::NoColor() ) return;

    const RowCol rc = tbl_->getCell( levelsel );
    tbl_->setColor( RowCol(rc.row(),1), col );
}


bool acceptOK( CallBacker* )
{
    for ( int idx=0; idx<objids_.size(); idx++ )
    {
	Color col = tbl_->getColor( RowCol(idx,1) );

	mDynamicCastGet(uiStratLevelSel*,levelsel,
			tbl_->getCellGroup(RowCol(idx,2)))
	const int lvlid = levelsel ? levelsel->getID() : -1;

	IOPar displaypar;
	displaypar.set( sKey::StratRef(), lvlid );
	displaypar.set( sKey::Color(), col );
	EM::EMM().writeDisplayPars( *objids_[idx], displaypar );
    }

    return true;
}


    uiTable*	tbl_;
    const ObjectSet<MultiID>& objids_;

};


void uiSurfaceMan::stratSel( CallBacker* )
{
    const ObjectSet<MultiID>& ids = selgrp_->getIOObjIds();
    uiSurfaceStratDlg dlg( this, ids );
    dlg.go();
}


class uiSurface2DMan : public uiDialog
{
public:

uiSurface2DMan( uiParent* p, const EM::IOObjInfo& info )
    :uiDialog(p,uiDialog::Setup("2D Horizons management","Manage 2D horizons",
				mODHelpKey(mSurface2DManHelpID) ))
    , eminfo_(info)
{
    setCtrlStyle( CloseOnly );

    uiGroup* topgrp = new uiGroup( this, "Top" );
    uiLabeledListBox* lllb = new uiLabeledListBox( topgrp, "2D lines",
			    OD::ChooseOnlyOne, uiLabeledListBox::AboveMid );
    linelist_ = lllb->box();
    BufferStringSet linenames;
    info.getLineNames( linenames );
    linelist_->addItems( linenames );
    linelist_->selectionChanged.notify( mCB(this,uiSurface2DMan,lineSel) );

    uiGroup* botgrp = new uiGroup( this, "Bottom" );
    infofld_ = new uiTextEdit( botgrp, "File Info", true );
    infofld_->setPrefHeightInChar( 8 );
    infofld_->setPrefWidthInChar( 50 );

    uiSplitter* splitter = new uiSplitter( this, "Splitter", false );
    splitter->addGroup( topgrp );
    splitter->addGroup( botgrp );

    lineSel( 0 );
}


void lineSel( CallBacker* )
{
    const int curitm = linelist_->currentItem();
    TypeSet< StepInterval<int> > trcranges;
    eminfo_.getTrcRanges( trcranges );

    BufferString txt;
    if ( trcranges.validIdx(curitm) )
    {
	StepInterval<int> trcrg = trcranges[ curitm ];
	txt += BufferString( sKey::FirstTrc(), ": " ); txt += trcrg.start;
	txt += "\n";
	txt += BufferString( sKey::LastTrc(), ": " ); txt += trcrg.stop;
	txt += "\n";
	txt += BufferString( "Trace Step: " ); txt += trcrg.step;
    }

    infofld_->setText( txt );
}

    uiListBox*			linelist_;
    uiTextEdit*			infofld_;
    const EM::IOObjInfo&	eminfo_;

};


void uiSurfaceMan::man2dCB( CallBacker* )
{
    EM::IOObjInfo eminfo( curioobj_->key() );
    uiSurface2DMan dlg( this, eminfo );
    dlg.go();
}
