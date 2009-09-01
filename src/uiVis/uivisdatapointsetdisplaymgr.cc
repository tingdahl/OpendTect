/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          April 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uivisdatapointsetdisplaymgr.cc,v 1.6 2009-09-01 06:14:51 cvssatyaki Exp $";

#include "uivisdatapointsetdisplaymgr.h"

#include "uimenuhandler.h"
#include "uivispartserv.h"
#include "uimaterialdlg.h"
#include "uicreatepicks.h"
#include "uiioobj.h"
#include "uimsg.h"
#include "ctxtioobj.h"
#include "emrandomposbody.h"
#include "emmanager.h"
#include "pickset.h"
#include "picksettr.h"
#include "visdata.h"
#include "visrandomposbodydisplay.h"
#include "vissurvscene.h"
#include "vispointsetdisplay.h"

uiVisDataPointSetDisplayMgr::uiVisDataPointSetDisplayMgr(uiVisPartServer& serv )
    : visserv_( serv )
    , vismenu_( visserv_.getMenuHandler() )
    , createbodymnuitem_( "Create Body ..." )
    , storepsmnuitem_( "Save as Pickset ..." )
    , removemnuitem_( "Remove selected points" )
    , treeToBeAdded( this )
{
    vismenu_->createnotifier.notify(
	    mCB(this,uiVisDataPointSetDisplayMgr,createMenuCB) );
    vismenu_->handlenotifier.notify(
	    mCB(this,uiVisDataPointSetDisplayMgr,handleMenuCB) );
}


uiVisDataPointSetDisplayMgr::~uiVisDataPointSetDisplayMgr()
{
    deepErase( displayinfos_ );
}


void uiVisDataPointSetDisplayMgr::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(MenuHandler*,menu,cb);
    if ( !menu )
	return;
    const int displayid = menu->menuID();
    visBase::DataObject* dataobj = visserv_.getObject( displayid );
    mDynamicCastGet(visSurvey::PointSetDisplay*,display,dataobj);
    if ( !display )
	return;

    bool dispcorrect = false;
    for ( int idx=0; idx<displayinfos_.size(); idx++ )
    {
	const TypeSet<int> visids = displayinfos_[idx]->visids_;
	for ( int idy=0; idy<visids.size(); idy++ )
	{
	    if ( visids[idy] == displayid )
		dispcorrect = true;
	}
    }

    if ( !dispcorrect ) return;

     mAddMenuItem( menu, &createbodymnuitem_, true, false );
     mAddMenuItem( menu, &storepsmnuitem_, true, false );
     mAddMenuItem( menu, &removemnuitem_, true, false );
}


void uiVisDataPointSetDisplayMgr::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    if ( mnuid==-1 ) return;
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    if ( !menu ) return;

    const int displayid = menu->menuID();
    visBase::DataObject* dataobj = visserv_.getObject( displayid );
    mDynamicCastGet(visSurvey::PointSetDisplay*,display,dataobj);
    if ( !display )
	return;

    bool dispcorrect = false;
    for ( int idx=0; idx<displayinfos_.size(); idx++ )
    {
	const TypeSet<int> visids = displayinfos_[idx]->visids_;
	for ( int idy=0; idy<visids.size(); idy++ )
	{
	    if ( visids[idy] == displayid )
		dispcorrect = true;
	}
    }

    if ( !dispcorrect ) return;

    if ( mnuid == createbodymnuitem_.id )
    {
	RefMan<EM::EMObject> emobj =
		EM::EMM().createTempObject( EM::RandomPosBody::typeStr() );
	const DataPointSet& data = display->getDataPack();
	mDynamicCastGet( EM::RandomPosBody*, emps, emobj.ptr() );
	if ( !emps )
	    return;

	emps->copyFrom( data, true );
	emps->setPreferredColor( display->getColor() );
	treeToBeAdded.trigger( emps->id() );
    }
    else if ( mnuid == storepsmnuitem_.id )
    {
	uiCreatePicks dlg( visserv_.appserv().parent() );
	if ( !dlg.go() )
	return;

	Pick::Set& pickset = *dlg.getPickSet();

	const DataPointSet& data = display->getDataPack();
	for ( int rid=0; rid<data.size(); rid++ )
	{
	    if ( data.isSelected(rid) )
	    pickset += Pick::Location( Coord3(data.coord(rid),data.z(rid)));
	}

	PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(PickSet);
	ctio->setName( pickset.name() );

	if ( uiIOObj::fillCtio(*ctio,true) )
	{
	    BufferString bs;
	    if ( !PickSetTranslator::store( pickset, ctio->ioobj, bs ) )
	    uiMSG().error(bs);
	}
    }
    else if ( mnuid == removemnuitem_.id )
    {
	visSurvey::Scene* scene = display->getScene();
	if ( !scene || !scene->getSelector() )
	    return;
	display->removeSelection( *scene->getSelector() );
    }
}


void uiVisDataPointSetDisplayMgr::lock()
{
    lock_.lock();
    visserv_.getChildIds( -1, allsceneids_ );
    availableparents_ = allsceneids_;
}


void uiVisDataPointSetDisplayMgr::unLock()
{ lock_.unLock(); }


int uiVisDataPointSetDisplayMgr::getNrParents() const
{
    return allsceneids_.size();
}


const char* uiVisDataPointSetDisplayMgr::getParentName( int parentidx ) const
{
    RefMan<visBase::DataObject> scene =
	visserv_.getObject( allsceneids_[parentidx] );
    return scene ? scene->name() : 0;
}


int uiVisDataPointSetDisplayMgr::addDisplay(const TypeSet<int>& parents,
					    const DataPointSet& dps )
{
    // TODO: Check situation where parents != allsceneids_
    if ( !parents.size() )
	return -1;

    DisplayInfo* displayinfo = new DisplayInfo;
    if ( !displayinfo )
	return -1;

    int id = 0;
    while ( ids_.indexOf(id)!=-1 ) id++;

    for ( int idx=0; idx<parents.size(); idx++ )
    {
	RefMan<visBase::DataObject> sceneptr =
		visserv_.getObject( allsceneids_[idx] );
	if ( !sceneptr )
	    continue;

	RefMan<visSurvey::PointSetDisplay> display =
	    visSurvey::PointSetDisplay::create();
	if ( !display )
	    continue;

	mDynamicCastGet( visSurvey::Scene*, scene, sceneptr.ptr() );
	if ( !scene )
	    continue;

	visserv_.addObject( display, parents[idx], true );
	display->setDataPack( dps );
	display->setColor( Color::DgbColor() );

	displayinfo->sceneids_ += allsceneids_[idx];
	displayinfo->visids_ += display->id();
    }

    if ( !displayinfo->sceneids_.size() )
    {
	delete displayinfo;
	return -1;
    }

    displayinfos_ += displayinfo;
    ids_ += id;

    return id;
}


void uiVisDataPointSetDisplayMgr::setDispCol( Color col, int dispid )
{
    RefMan<visBase::DataObject> displayptr = visserv_.getObject(dispid);
    if ( !displayptr )
	return;

    mDynamicCastGet( visSurvey::PointSetDisplay*, display, displayptr.ptr() );
    if ( !display )
	return;
    display->setColor( col );
}


void uiVisDataPointSetDisplayMgr::updateDisplay( int id,
						 const TypeSet<int>& parents,
						 const DataPointSet& dps )
{
    // TODO: Check situation where parents != allsceneids_

    const int idx = ids_.indexOf( id );
    if ( idx<0 )
	return;

    DisplayInfo& displayinfo = *displayinfos_[idx];
    TypeSet<int> wantedscenes;
    for ( int idy=0; idy<parents.size(); idy++ )
	wantedscenes += parents[idy];

    TypeSet<int> scenestoremove = displayinfo.sceneids_;
    scenestoremove.createDifference( wantedscenes );

    TypeSet<int> scenestoadd = wantedscenes;
    scenestoadd.createDifference( displayinfo.sceneids_ );

    for ( int idy=0; idy<scenestoremove.size(); idy++ )
    {
	const int sceneid = scenestoremove[idx];
	const int index = displayinfo.sceneids_.indexOf( sceneid );
	RefMan<visBase::DataObject> sceneptr =
		visserv_.getObject( allsceneids_[idx] );
	if ( !sceneptr )
	    continue;

	mDynamicCastGet( visSurvey::Scene*, scene, sceneptr.ptr() );
	if ( !scene )
	    continue;

	const int objid = scene->getFirstIdx( displayinfo.visids_[index] );
	if ( objid >= 0 )
	    scene->removeObject( objid );

	displayinfo.sceneids_.remove( index );
	displayinfo.visids_.remove( index );
    }

    for ( int idy=0; idy<scenestoadd.size(); idy++ )
    {
	const int sceneid = scenestoadd[idy];
	RefMan<visBase::DataObject> sceneptr =
		visserv_.getObject( sceneid );
	if ( !sceneptr )
	    continue;

	RefMan<visSurvey::PointSetDisplay> display =
	    visSurvey::PointSetDisplay::create();
	if ( !display )
	    continue;

	mDynamicCastGet( visSurvey::Scene*, scene, sceneptr.ptr() );
	if ( !scene )
	    continue;

	visserv_.addObject( display, parents[idx], true );

	displayinfo.sceneids_ += sceneid;
	displayinfo.visids_ += display->id();
    }

    for ( int idy=0; idy<displayinfo.visids_.size(); idy++ )
    {
	const int displayid = displayinfo.visids_[idy];
	RefMan<visBase::DataObject> displayptr = visserv_.getObject(displayid);
	if ( !displayptr )
	    continue;

	mDynamicCastGet( visSurvey::PointSetDisplay*, display,
			 displayptr.ptr() );
	if ( !display )
	    continue;

	display->setDataPack( dps );
    }
}


void uiVisDataPointSetDisplayMgr::removeDisplay( int id )
{
    const int idx = ids_.indexOf( id );
    if ( idx<0 )
	return;

    DisplayInfo& displayinfo = *displayinfos_[idx];
    for ( int idy=0; idy<displayinfo.visids_.size(); idy++ )
    {
	const int sceneid = displayinfo.sceneids_[idy];
	RefMan<visBase::DataObject> sceneptr = visserv_.getObject( sceneid );
	if ( !sceneptr )
	    continue;

	mDynamicCastGet( visSurvey::Scene*, scene, sceneptr.ptr() );
	if ( !scene )
	    continue;

	visserv_.removeObject( displayinfo.visids_[idy],
			       displayinfo.sceneids_[idy] );
    }

    delete displayinfos_.remove( idx );
    ids_.remove( idx );
}
