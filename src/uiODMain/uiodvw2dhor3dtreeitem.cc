/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		May 2010
________________________________________________________________________

-*/

#include "uiodvw2dhor3dtreeitem.h"

#include "uiattribpartserv.h"
#include "uiflatviewer.h"
#include "uiflatviewwin.h"
#include "uiempartserv.h"
#include "uiflatviewstdcontrol.h"
#include "uigraphicsscene.h"
#include "uirgbarraycanvas.h"
#include "uimenu.h"
#include "uimpe.h"
#include "uimpepartserv.h"
#include "uiodapplmgr.h"
#include "uiodviewer2d.h"
#include "uiodviewer2dmgr.h"
#include "uistrings.h"
#include "uitreeview.h"
#include "uivispartserv.h"

#include "emhorizon3d.h"
#include "emmanager.h"
#include "emobject.h"
#include "emtracker.h"
#include "ioman.h"
#include "ioobj.h"
#include "mouseevent.h"
#include "mpeengine.h"
#include "view2ddataman.h"
#include "view2dhorizon3d.h"


#define mAddInAllIdx	0
#define mAddIdx		1
#define mNewIdx		2

uiODVw2DHor3DParentTreeItem::uiODVw2DHor3DParentTreeItem()
    : uiODVw2DTreeItem( tr("3D Horizon") )
{
}


uiODVw2DHor3DParentTreeItem::~uiODVw2DHor3DParentTreeItem()
{
}


void uiODVw2DHor3DParentTreeItem::getNonLoadedTrackedHor3Ds(
	TypeSet<EM::ObjectID>& emids )
{
    const int highesttrackerid = MPE::engine().highestTrackerID();
    TypeSet<EM::ObjectID> loadedemids;
    getLoadedHorizon3Ds( loadedemids );
    for ( int idx=0; idx<=highesttrackerid; idx++ )
    {
	MPE::EMTracker* tracker = MPE::engine().getTracker( idx );
	if ( !tracker )
	    continue;

	EM::EMObject* emobj = tracker->emObject();
	mDynamicCastGet(EM::Horizon3D*,hor3d,emobj);
	if ( !hor3d || loadedemids.isPresent(emobj->id()) )
	    continue;

	emids.addIfNew( emobj->id() );
    }
}


bool uiODVw2DHor3DParentTreeItem::showSubMenu()
{
    const bool hastransform = viewer2D()->hasZAxisTransform();

    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    uiMenu* addmenu = new uiMenu( uiStrings::sAdd() );
    addmenu->insertItem( new uiAction(tr("In all 2D Viewers")), mAddInAllIdx );
    addmenu->insertItem( new uiAction(tr("Only in this 2D Viewer")), mAddIdx );
    mnu.insertItem( addmenu );

    TypeSet<EM::ObjectID> emids;
    getNonLoadedTrackedHor3Ds( emids );
    if ( emids.isEmpty() )
    {
	uiAction* newmenu = new uiAction( m3Dots(tr("Track New")) );
	newmenu->setEnabled( !hastransform );
	mnu.insertItem( newmenu, mNewIdx );
    }
    else
    {
	uiMenu* trackmenu = new uiMenu( tr("Track") );
	uiAction* newmenu = new uiAction( uiStrings::sNew() );
	trackmenu->insertItem( newmenu, mNewIdx );
	for ( int idx=0; idx<emids.size(); idx++ )
	{
	    const EM::EMObject* emobject = EM::EMM().getObject( emids[idx] );
	    uiAction* trackexistingmnu = new uiAction( emobject->uiName() );
	    trackexistingmnu->setEnabled( !hastransform );
	    trackmenu->insertItem( trackexistingmnu, mNewIdx + idx + 1 );
	}

	mnu.insertItem( trackmenu );
    }

    insertStdSubMenu( mnu );
    return handleSubMenu( mnu.exec() );
}


bool uiODVw2DHor3DParentTreeItem::handleSubMenu( int mnuid )
{
    handleStdSubMenu( mnuid );

    if ( mnuid >= mNewIdx )
    {
	TypeSet<EM::ObjectID> emids;
	getNonLoadedTrackedHor3Ds( emids );
	const int emidx = mnuid - mNewIdx - 1;
	if ( emidx >= emids.size() )
	    return false;

	uiMPEPartServer* mps = applMgr()->mpeServer();
	mps->setCurrentAttribDescSet(
		applMgr()->attrServer()->curDescSet(false) );
	int emid = -1;
	if ( emids.validIdx(emidx) )
	    emid = emids[emidx];

	EM::EMObject* emobj = EM::EMM().getObject( emid );
	if ( emobj )
	{
	    MPE::engine().addTracker( emobj );
	    MPE::engine().setActiveTracker( emobj->id() );
	}
	else if ( !mps->addTracker(EM::Horizon3D::typeStr(),
				   viewer2D()->getSyncSceneID()) )
	    return true;

	const MPE::EMTracker* tracker = MPE::engine().getActiveTracker();
	if ( !tracker )
	    return false;

	emid = tracker->objectID();
	const int trackid = MPE::engine().getTrackerByObject( emid );
	if ( mps->getSetupGroup() )
	    mps->getSetupGroup()->setTrackingMethod(
						EventTracker::AdjacentParent );
	addNewTrackingHorizon3D( emid );
	applMgr()->viewer2DMgr().addNewTrackingHorizon3D(
		emid, viewer2D()->getSyncSceneID() );
	MPE::engine().removeTracker( trackid );
	mps->enableTracking( trackid, true );
    }
    else if ( mnuid == mAddInAllIdx || mnuid==mAddIdx )
    {
	ObjectSet<EM::EMObject> objs;
	applMgr()->EMServer()->selectHorizons( objs, false );
	TypeSet<EM::ObjectID> emids;
	for ( int idx=0; idx<objs.size(); idx++ )
	    emids += objs[idx]->id();
	if ( mnuid==mAddInAllIdx )
	{
	    addHorizon3Ds( emids );
	    applMgr()->viewer2DMgr().addHorizon3Ds( emids );
	}
	else
	    addHorizon3Ds( emids );

	deepUnRef( objs );
    }

    return true;
}


void uiODVw2DHor3DParentTreeItem::getHor3DVwr2DIDs(
	EM::ObjectID emid, TypeSet<int>& vw2dobjids ) const
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(const uiODVw2DHor3DTreeItem*,hor3dtreeitm,getChild(idx))
	if ( !hor3dtreeitm || hor3dtreeitm->emObjectID() != emid )
	    continue;

	vw2dobjids.addIfNew( hor3dtreeitm->vw2DObject()->id() );
    }
}


void uiODVw2DHor3DParentTreeItem::getLoadedHorizon3Ds(
	TypeSet<EM::ObjectID>& emids ) const
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(const uiODVw2DHor3DTreeItem*,hor3dtreeitm,getChild(idx))
	if ( !hor3dtreeitm )
	    continue;
	emids.addIfNew( hor3dtreeitm->emObjectID() );
    }
}


void uiODVw2DHor3DParentTreeItem::removeHorizon3D( EM::ObjectID emid )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DHor3DTreeItem*,hor3dtreeitm,getChild(idx))
	if ( !hor3dtreeitm || emid!=hor3dtreeitm->emObjectID() )
	    continue;
	removeChild( hor3dtreeitm );
    }
}


void uiODVw2DHor3DParentTreeItem::addHorizon3Ds(
	const TypeSet<EM::ObjectID>& emids )
{
    TypeSet<EM::ObjectID> emidstobeloaded, emidsloaded;
    getLoadedHorizon3Ds( emidsloaded );
    for ( int idx=0; idx<emids.size(); idx++ )
    {
	if ( !emidsloaded.isPresent(emids[idx]) )
	    emidstobeloaded.addIfNew( emids[idx] );
    }

    for ( int idx=0; idx<emidstobeloaded.size(); idx++ )
    {
	const bool istracking =
	    MPE::engine().getTrackerByObject(emidstobeloaded[idx]) != -1;
	if ( istracking )
	{
	    EM::EMObject* emobj = EM::EMM().getObject( emidstobeloaded[idx] );
	    if ( !emobj || findChild(emobj->name()) )
		continue;

	    const int trackid = MPE::engine().addTracker( emobj );
	    MPE::engine().getEditor( emobj->id(), true );
	    if ( viewer2D() && viewer2D()->viewControl() )
		viewer2D()->viewControl()->setEditMode( true );
	    MPE::engine().removeTracker( trackid );
	    applMgr()->mpeServer()->enableTracking( trackid, true );
	}

	uiODVw2DHor3DTreeItem* childitem =
	    new uiODVw2DHor3DTreeItem( emidstobeloaded[idx] );
	addChld( childitem, false, false);
	if ( istracking )
	    childitem->select();
    }
}


void uiODVw2DHor3DParentTreeItem::addNewTrackingHorizon3D( EM::ObjectID emid )
{
    TypeSet<EM::ObjectID> emidsloaded;
    getLoadedHorizon3Ds( emidsloaded );
    if ( emidsloaded.isPresent(emid) )
	return;

    uiODVw2DHor3DTreeItem* hortreeitem = new uiODVw2DHor3DTreeItem( emid );
    const int trackid = applMgr()->mpeServer()->getTrackerID( emid );
    if ( trackid>=0 )
    {
	EM::EMObject* emobj = EM::EMM().getObject( emid );
	MPE::engine().addTracker( emobj );
	MPE::engine().getEditor( emid, true );
    }

    addChld( hortreeitem, false, false );
    if ( viewer2D() && viewer2D()->viewControl() )
	viewer2D()->viewControl()->setEditMode( true );

    hortreeitem->select();
}


const char* uiODVw2DHor3DParentTreeItem::iconName() const
{ return "tree-horizon3d"; }


bool uiODVw2DHor3DParentTreeItem::init()
{ return uiODVw2DTreeItem::init(); }



uiODVw2DHor3DTreeItem::uiODVw2DHor3DTreeItem( const EM::ObjectID& emid )
    : uiODVw2DTreeItem(uiString::emptyString())
    , emid_(emid)
    , horview_(0)
    , oldactivevolupdated_(false)
    , trackerefed_(false)
{
    if ( MPE::engine().getTrackerByObject(emid_) != -1 )
	trackerefed_ = true;
}


uiODVw2DHor3DTreeItem::uiODVw2DHor3DTreeItem( int id, bool )
    : uiODVw2DTreeItem(uiString::emptyString())
    , emid_(-1)
    , horview_(0)
    , oldactivevolupdated_(false)
    , trackerefed_(false)
{
    displayid_ = id;
}


uiODVw2DHor3DTreeItem::~uiODVw2DHor3DTreeItem()
{
    NotifierAccess* deselnotify = horview_ ? horview_->deSelection() : 0;
    if ( deselnotify )
	deselnotify->remove( mCB(this,uiODVw2DHor3DTreeItem,deSelCB) );

    for ( int ivwr=0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewer2D()->viewwin()->viewer( ivwr );
	MouseEventHandler* meh =
			&vwr.rgbCanvas().scene().getMouseEventHandler();
	meh->buttonPressed.remove(
		mCB(this,uiODVw2DHor3DTreeItem,mousePressInVwrCB) );
	meh->buttonReleased.remove(
		mCB(this,uiODVw2DHor3DTreeItem,mouseReleaseInVwrCB) );
    }

    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( emobj )
    {
	emobj->change.remove( mCB(this,uiODVw2DHor3DTreeItem,emobjChangeCB) );

	if ( trackerefed_ )
	{
	    const int trackeridx =
				MPE::engine().getTrackerByObject( emobj->id() );
	    if ( trackeridx >= 0 )
	    {
		MPE::engine().removeEditor( emobj->id() );
		MPE::engine().removeTracker( trackeridx );
	    }
	}
    }

    if ( horview_ )
	viewer2D()->dataMgr()->removeObject( horview_ );
}


bool uiODVw2DHor3DTreeItem::init()
{
    EM::EMObject* emobj = 0;
    if ( displayid_ < 0 )
    {
	emobj = EM::EMM().getObject( emid_ );
	if ( !emobj ) return false;

	horview_ = Vw2DHorizon3D::create( emid_, viewer2D()->viewwin(),
				      viewer2D()->dataEditor() );
	viewer2D()->dataMgr()->addObject( horview_ );
	displayid_ = horview_->id();
    }
    else
    {
	mDynamicCastGet(Vw2DHorizon3D*,hd,
		viewer2D()->dataMgr()->getObject(displayid_))
	if ( !hd )
	    return false;
	emid_ = hd->emID();
	emobj = EM::EMM().getObject( emid_ );
	if ( !emobj ) return false;

	horview_ = hd;
    }

    emobj->change.notify( mCB(this,uiODVw2DHor3DTreeItem,emobjChangeCB) );
    displayMiniCtab();

    name_ = toUiString(applMgr()->EMServer()->getName( emid_ ));
    uitreeviewitem_->setCheckable(true);
    uitreeviewitem_->setChecked( true );
    checkStatusChange()->notify( mCB(this,uiODVw2DHor3DTreeItem,checkCB) );

    for ( int ivwr=0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewer2D()->viewwin()->viewer( ivwr );
	MouseEventHandler* meh =
			&vwr.rgbCanvas().scene().getMouseEventHandler();
	meh->buttonPressed.notify(
		mCB(this,uiODVw2DHor3DTreeItem,mousePressInVwrCB) );
	meh->buttonReleased.notify(
		mCB(this,uiODVw2DHor3DTreeItem,mouseReleaseInVwrCB) );
    }

    horview_->setSelSpec( &viewer2D()->selSpec(true), true );
    horview_->setSelSpec( &viewer2D()->selSpec(false), false );
    horview_->draw();

    NotifierAccess* deselnotify = horview_->deSelection();
    if ( deselnotify )
	deselnotify->notify( mCB(this,uiODVw2DHor3DTreeItem,deSelCB) );

    return true;
}


void uiODVw2DHor3DTreeItem::displayMiniCtab()
{
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return;

    uiTreeItem::updateColumnText( uiODViewer2DMgr::cColorColumn() );
    uitreeviewitem_->setPixmap( uiODViewer2DMgr::cColorColumn(),
				emobj->preferredColor() );
}


void uiODVw2DHor3DTreeItem::emobjChangeCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( const EM::EMObjectCallbackData&,
				cbdata, caller, cb );
    mDynamicCastGet(EM::EMObject*,emobject,caller);
    if ( !emobject ) return;

    switch( cbdata.event )
    {
	case EM::EMObjectCallbackData::Undef:
	    break;
	case EM::EMObjectCallbackData::PrefColorChange:
	{
	    displayMiniCtab();
	    break;
	}
	case EM::EMObjectCallbackData::NameChange:
	{
	    name_ = mToUiStringTodo(applMgr()->EMServer()->getName( emid_ ));
	    uiTreeItem::updateColumnText( uiODViewer2DMgr::cNameColumn() );
	    break;
	}
	default: break;
    }
}


bool uiODVw2DHor3DTreeItem::select()
{
    if ( uitreeviewitem_->treeView() )
	uitreeviewitem_->treeView()->clearSelection();

    uitreeviewitem_->setSelected( true );

    if ( !trackerefed_ )
    {
	if (  MPE::engine().getTrackerByObject(emid_) != -1 )
	{
	    MPE::engine().addTracker( EM::EMM().getObject(emid_) );
	    MPE::engine().getEditor( emid_, true );
	    trackerefed_ = true;
	}
    }

    if ( horview_ )
    {
	viewer2D()->dataMgr()->setSelected( horview_ );
	horview_->selected( isChecked() );
    }

    return true;
}


void uiODVw2DHor3DTreeItem::renameVisObj()
{
    const MultiID midintree = applMgr()->EMServer()->getStorageID(emid_);
    TypeSet<int> visobjids;
    applMgr()->visServer()->findObject( midintree, visobjids );
    for ( int idx=0; idx<visobjids.size(); idx++ )
	applMgr()->visServer()->setObjectName( visobjids[idx], name_ );
    applMgr()->visServer()->triggerTreeUpdate();
}


static void addAction( uiMenu& mnu, uiString txt, int id,
			const char* icon=0, bool enab=true )
{
    uiAction* action = new uiAction( txt );
    mnu.insertAction( action, id );
    action->setEnabled( enab );
    action->setIcon( icon );
}


#define mPropID		0
#define mStartID	1
#define mSettsID	2
#define mSaveID		3
#define mSaveAsID	4
#define mRemoveAllID	5
#define mRemoveID	6

bool uiODVw2DHor3DTreeItem::showSubMenu()
{
    uiEMPartServer* ems = applMgr()->EMServer();
    uiMPEPartServer* mps = applMgr()->mpeServer();
    uiVisPartServer* vps = applMgr()->visServer();
    if ( !ems || !mps || !vps ) return false;

    uiMenu mnu( getUiParent(), uiStrings::sAction() );

    addAction( mnu, uiStrings::sProperties(), mPropID, "disppars", true );

    uiMenu* trackmnu = new uiMenu( uiStrings::sTracking() );
    mnu.addMenu( trackmnu );
    const bool hastracker = MPE::engine().getTrackerByObject(emid_) > -1;
    addAction( *trackmnu, m3Dots(tr("Start Tracking")), mStartID,
		0, !hastracker );
    addAction( *trackmnu, m3Dots(tr("Change Settings")), mSettsID,
		"seedpicksettings", hastracker );

    const bool haschanged = ems->isChanged(emid_) && ems->isFullyLoaded(emid_);
    addAction( mnu, uiStrings::sSave(), mSaveID, "save", haschanged );
    addAction( mnu, m3Dots(uiStrings::sSaveAs()), mSaveAsID, "saveas", true );

    uiMenu* removemenu = new uiMenu( uiStrings::sRemove(), "remove" );
    mnu.addMenu( removemenu );
    addAction( *removemenu, tr("From all 2D Viewers"), mRemoveAllID );
    addAction( *removemenu, tr("Only from this 2D Viewer"), mRemoveID );

    mps->setCurrentAttribDescSet( applMgr()->attrServer()->curDescSet(false) );
    mps->setCurrentAttribDescSet( applMgr()->attrServer()->curDescSet(true) );

    const int mnuid = mnu.exec();
    if ( mnuid == mPropID )
    {
    }
    else if ( mnuid == mSaveID )
    {
	bool savewithname = EM::EMM().getMultiID( emid_ ).isEmpty();
	if ( !savewithname )
	{
	    PtrMan<IOObj> ioobj = IOM().get( EM::EMM().getMultiID(emid_) );
	    savewithname = !ioobj;
	}

	ems->storeObject( emid_, savewithname );
	const MultiID mid = ems->getStorageID(emid_);
	mps->saveSetup( mid );
	name_ = mToUiStringTodo(ems->getName( emid_ ));
	uiTreeItem::updateColumnText( uiODViewer2DMgr::cNameColumn() );
	renameVisObj();
    }
    else if ( mnuid == mSaveAsID )
    {
	const MultiID oldmid = ems->getStorageID(emid_);
	mps->prepareSaveSetupAs( oldmid );

	MultiID storedmid;
	ems->storeObject( emid_, true, storedmid );
	name_ = mToUiStringTodo(ems->getName( emid_ ));

	const MultiID midintree = ems->getStorageID(emid_);
	EM::EMM().getObject(emid_)->setMultiID( storedmid);
	mps->saveSetupAs( storedmid );
	EM::EMM().getObject(emid_)->setMultiID( midintree );

	uiTreeItem::updateColumnText( uiODViewer2DMgr::cNameColumn() );
	renameVisObj();
    }
    else if ( mnuid == mStartID )
    {
	if ( mps->addTracker(emid_) == -1 )
	    return false;

	const EM::EMObject* emobj = EM::EMM().getObject( emid_ );
	const EM::SectionID sid = emobj->sectionID( 0 );
	mps->useSavedSetupDlg( emid_, sid );
    }
    else if ( mnuid == mSettsID )
    {
	EM::EMObject* emobj = EM::EMM().getObject( emid_ );
	if ( emobj )
	{
	    const EM::SectionID sid = emobj->sectionID( 0 );
	    applMgr()->mpeServer()->showSetupDlg( emid_, sid );
	}
    }
    else if ( mnuid==mRemoveAllID || mnuid==mRemoveID )
    {
	ems->askUserToSave( emid_, true );
	const int trackerid = mps->getTrackerID( emid_ );
	if ( trackerid>= 0 )
	    renameVisObj();
	name_ = mToUiStringTodo(ems->getName( emid_ ));
	bool doremove = !applMgr()->viewer2DMgr().isItemPresent( parent_ ) ||
			mnuid==mRemoveID;
	if ( mnuid == mRemoveAllID )
	    applMgr()->viewer2DMgr().removeHorizon3D( emid_ );
	if ( doremove )
	    parent_->removeChild( this );
    }

    return true;
}


void uiODVw2DHor3DTreeItem::checkCB( CallBacker* )
{
    if ( horview_ )
	horview_->enablePainting( isChecked() );
}


void uiODVw2DHor3DTreeItem::deSelCB( CallBacker* )
{
    //TODO handle on/off MOUSEEVENT
}


void uiODVw2DHor3DTreeItem::updateSelSpec( const Attrib::SelSpec* selspec,
					   bool wva )
{
    if ( horview_ )
	horview_->setSelSpec( selspec, wva );
}


void uiODVw2DHor3DTreeItem::updateCS( const TrcKeyZSampling& cs, bool upd )
{
    if ( upd && horview_ )
	horview_->setTrcKeyZSampling( cs, upd );
}


void uiODVw2DHor3DTreeItem::emobjAbtToDelCB( CallBacker* cb )
{
    mCBCapsuleUnpack( const EM::ObjectID&, emid, cb );
    if ( emid != emid_ ) return;

    EM::EMObject* emobj = EM::EMM().getObject( emid );
    mDynamicCastGet(EM::Horizon3D*,hor3d,emobj);
    if ( !hor3d ) return;

    parent_->removeChild( this );
}


void uiODVw2DHor3DTreeItem::mousePressInVwrCB( CallBacker* )
{
    if ( !uitreeviewitem_->isSelected() || !horview_ )
	return;

    if ( !viewer2D()->viewwin()->nrViewers() )
	return;

    horview_->setSeedPicking( applMgr()->visServer()->isPicking() );
    horview_->setTrackerSetupActive(
	    applMgr()->visServer()->isTrackingSetupActive() );
}


void uiODVw2DHor3DTreeItem::mouseReleaseInVwrCB( CallBacker* )
{
}


uiTreeItem* uiODVw2DHor3DTreeItemFactory::createForVis(
				    const uiODViewer2D& vwr2d, int id ) const
{
    mDynamicCastGet(const Vw2DHorizon3D*,obj,vwr2d.dataMgr()->getObject(id));
    return obj ? new uiODVw2DHor3DTreeItem(id,true) : 0;
}
