/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : R. Sen
 * DATE     : Jan 2017
-*/


#include "emobjectio.h"

#include "dbman.h"
#include "emmanager.h"
#include "emsurfaceiodata.h"
#include "executor.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "task.h"

mDefineInstanceCreatedNotifierAccess(EM::ObjectSaver)
mImplFactory2Param(EM::ObjectLoader,const DBKeySet&,
		   const EM::SurfaceIODataSelection*,EM::ObjectLoader::factory)
mImplFactory1Param(EM::ObjectSaver,const SharedObject&,EM::ObjectSaver::factory)

namespace EM
{

ObjectLoader::ObjectLoader( const DBKeySet& keys,
			    const SurfaceIODataSelection* sel )
    : dbkeys_(keys)
    , sel_(sel)
{
}


Executor* ObjectLoader::fetchLoader( EMObject* obj ) const
{
    mDynamicCastGet(Surface*,surface,obj)
    if ( surface )
    {
/*	mDynamicCastGet(RowColSurfaceGeometry*,geom,&surface->geometry())
	if ( geom && sel_ )
	{
	    TrcKeySampling hs;
	    hs.setInlRange( geom->rowRange() );
	    hs.setCrlRange( geom->colRange() );
	    if ( hs.isEmpty() )
		return geom->loader( sel_ );

	    SurfaceIODataSelection newsel( *sel_ );
	    newsel.rg.include( hs );
	    return geom->loader( &newsel );
	}
*/
	return surface->geometry().loader(sel_);
    }
    else if ( obj )
	return obj->loader();

    return 0;
}

class ObjectLoaderExec : public ExecutorGroup
{ mODTextTranslationClass(ObjectLoaderExec)
public:

ObjectLoaderExec( ObjectLoader& ldr )
    : ExecutorGroup("Object Loader")
    , loader_(ldr)
    , curidx_(0)
    , totnr_(-1)
    , nrdone_(0)
{
    loader_.notloadedkeys_.setEmpty();
    init();
}

protected:

void init()
{
    for ( int idx=0; idx<loader_.tobeLodedKeys().size(); idx++ )
    {
	const DBKey objid = loader_.tobeLodedKeys()[idx];
	PtrMan<IOObj> ioobj = DBM().get( objid );
	if ( !ioobj )
	{
	    pErrMsg( "Required ID not in IOM. Probably not OK" );
	    continue;
	}

	BufferString typenm = ioobj->pars().find( sKey::Type() );
	if ( typenm.isEmpty() )
	    typenm = ioobj->group();

	EMObject* obj = EMM().createObject( typenm, ioobj->name() );
	obj->ref();
	obj->setDBKey( objid );
	add( loader_.fetchLoader(obj) );
	objects_.add( obj );
    }

}

int nextStep()
{
    const int ret = ExecutorGroup::nextStep();
    if (  ret == ErrorOccurred() )
    {
	loader_.notloadedkeys_ += objects_[currentexec_]->dbKey();
	nrdone_++;
	if( goToNextExecutor() )
	    return MoreToDo();
    }
    else if ( ret == Finished() )
    {
	finishWork();
	return Finished();
    }

    nrdone_++;
    return ret;
}

void finishWork()
{
    for ( int idx=0; idx<objects_.size(); idx++ )
    {
	EMObject* obj = objects_[idx];
	const DBKey& dbkey = obj->dbKey();
	if ( loader_.notloadedkeys_.isPresent(dbkey) )
	    continue;

	EMManager& emm = getMgr( dbkey );
	emm.addObject( obj );
	loader_.addObject( obj );
    }
}

public:

od_int64 totalNr() const
{
    return executors_.size() ? ( totnr_ + ExecutorGroup::totalNr() ) : totnr_;
}


od_int64 nrDone() const
{
    return executors_.size() ? ( nrdone_ + ExecutorGroup::nrDone() ) : nrdone_;
}

virtual uiString message() const
{
    return uiStrings::phrLoading( uiStrings::sFaultStickSet() );
}

virtual uiString nrDoneText() const
{
    return tr("%1 loaded").arg( loader_.userName() );
}

protected:

    ObjectLoader&		loader_;
    int				curidx_;
    RefObjectSet<EMObject>	objects_;
    od_int64			nrdone_;
    od_int64			totnr_;
};


FaultStickSetLoader::FaultStickSetLoader( const DBKeySet& keys,
					  const SurfaceIODataSelection* sel )
    : ObjectLoader(keys,sel)
{
}


bool FaultStickSetLoader::load( TaskRunner* tskr )
{
    PtrMan<Executor> exec = getLoader();
    TaskRunner::execute( tskr, *exec );
    return allOK();
}


Executor* FaultStickSetLoader::getLoader() const
{
    FaultStickSetLoader* _this = const_cast<FaultStickSetLoader*>( this );
    return new ObjectLoaderExec( *_this );
}


Fault3DLoader::Fault3DLoader( const DBKeySet& keys,
			      const SurfaceIODataSelection* sel )
    : ObjectLoader(keys,sel)
{
}


bool Fault3DLoader::load( TaskRunner* tskr )
{
    PtrMan<Executor> exec = getLoader();
    TaskRunner::execute( tskr, *exec );
    return allOK();
}


Executor* Fault3DLoader::getLoader() const
{
    Fault3DLoader* _this = const_cast<Fault3DLoader*>( this );
    return new ObjectLoaderExec( *_this );
}


Horizon3DLoader::Horizon3DLoader( const DBKeySet& keys,
				  const SurfaceIODataSelection* sel )
    : ObjectLoader(keys,sel)
{
}


bool Horizon3DLoader::load( TaskRunner* tskr )
{
    PtrMan<Executor> exec = getLoader();
    TaskRunner::execute( tskr, *exec );
    return allOK();
}


Executor* Horizon3DLoader::getLoader() const
{
    Horizon3DLoader* _this = const_cast<Horizon3DLoader*>( this );
    return new ObjectLoaderExec( *_this );
}

//Saver
ObjectSaver::ObjectSaver( const SharedObject& emobj )
    : Saveable(emobj)
{
    mTriggerInstanceCreatedNotifier();
}


ObjectSaver::ObjectSaver( const ObjectSaver& oth )
    : Saveable(oth)
{
    copyClassData( oth );
    mTriggerInstanceCreatedNotifier();
}


ObjectSaver::~ObjectSaver()
{
    sendDelNotif();
}


mImplMonitorableAssignment(ObjectSaver,Saveable)

void ObjectSaver::copyClassData( const ObjectSaver& oth )
{
}


Monitorable::ChangeType ObjectSaver::compareClassData(
					const ObjectSaver& oth ) const
{
    return cNoChange();
}


ConstRefMan<EMObject> ObjectSaver::emObject() const
{
    return ConstRefMan<EMObject>( static_cast<const EMObject*>( object() ) );
}


void ObjectSaver::setEMObject( const EMObject& obj )
{
    setObject( obj );
}


uiRetVal ObjectSaver::doStore( const IOObj& ioobj ) const
{
    return uiRetVal::OK();
}


FaultStickSetSaver::FaultStickSetSaver( const SharedObject& emobj )
    : ObjectSaver(emobj)
{}


FaultStickSetSaver::~FaultStickSetSaver()
{}


uiRetVal FaultStickSetSaver::doStore( const IOObj& ioobj ) const
{
    uiRetVal uirv;
    ConstRefMan<EMObject> emobj = emObject();
    if ( !emobj )
	return uiRetVal::OK();

    SharedObject* copiedemobj = emobj->clone();
    mDynamicCastGet(FaultStickSet*,fss,copiedemobj)
    if ( !fss )
	return uiRetVal::OK();
    const DBKey objid = ioobj.key();
    Executor* exec = fss->geometry().saver( 0, &objid );
    if ( exec && !exec->execute() )
	return exec->errorWithDetails();

    if ( isSave(ioobj) )
    {
	emobj.getNonConstPtr()->setName( ioobj.name() );
	emobj.getNonConstPtr()->setDBKey( objid );
	fss->saveDisplayPars();
    }

    return uiRetVal::OK();
}


//Fault3D
Fault3DSaver::Fault3DSaver( const SharedObject& emobj )
    : ObjectSaver(emobj)
{}


Fault3DSaver::~Fault3DSaver()
{}


uiRetVal Fault3DSaver::doStore( const IOObj& ioobj ) const
{
    uiRetVal uirv;
    ConstRefMan<EMObject> emobj = emObject();
    if ( !emobj )
	return uiRetVal::OK();

    SharedObject* copiedemobj = emobj->clone();
    mDynamicCastGet(Fault3D*,flt3d,copiedemobj)
    if ( !flt3d )
	return uiRetVal::OK();
    const DBKey objid = ioobj.key();
    Executor* exec = flt3d->geometry().saver( 0, &objid );
    if ( exec && !exec->execute() )
	return exec->errorWithDetails();

    flt3d->setDBKey( objid );
    if ( isSave(ioobj) )
    {
	emobj.getNonConstPtr()->setName( ioobj.name() );
	emobj.getNonConstPtr()->setDBKey( objid );
	flt3d->saveDisplayPars();
    }

    return uiRetVal::OK();
}


Horizon3DSaver::Horizon3DSaver( const SharedObject& emobj )
    : ObjectSaver(emobj)
{}


Horizon3DSaver::~Horizon3DSaver()
{}


uiRetVal Horizon3DSaver::doStore( const IOObj& ioobj ) const
{
    uiRetVal uirv;
    ConstRefMan<EMObject> emobj = emObject();
    if ( !emobj )
	return uiRetVal::OK();

    SharedObject* copiedemobj = emobj->clone();
    mDynamicCastGet(Horizon3D*,hor,copiedemobj)
    if ( !hor )
	return uiRetVal::OK();
    const DBKey objid = ioobj.key();
    Executor* exec = hor->geometry().saver( 0, &objid );
    if ( exec && !exec->execute() )
	return exec->errorWithDetails();

    if ( isSave(ioobj) )
    {
	emobj.getNonConstPtr()->setName( ioobj.name() );
	emobj.getNonConstPtr()->setDBKey( objid );
	hor->saveDisplayPars();
    }

    return uiRetVal::OK();
}


} // namespace EM