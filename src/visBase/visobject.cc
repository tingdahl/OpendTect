/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: visobject.cc,v 1.34 2005-02-04 14:31:34 kristofer Exp $";

#include "visobject.h"

#include "errh.h"
#include "iopar.h"
#include "visdataman.h"
#include "visevent.h"
#include "vismaterial.h"
#include "vistransform.h"

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>

namespace visBase
{

const char* VisualObjectImpl::materialidstr = "Material ID";
const char* VisualObjectImpl::isonstr = "Is on";

VisualObject::VisualObject( bool selectable_ )
    : isselectable(selectable_)
    , deselnotifier(this)
    , selnotifier(this)
    , rightClick(this)
    , eventinfo(0)
{}


VisualObject::~VisualObject()
{}


VisualObjectImpl::VisualObjectImpl( bool selectable_ )
    : VisualObject(selectable_)
    , root(new SoSeparator)
    , onoff(new SoSwitch)
    , material(0)
{
    setMaterial( Material::create() );
    onoff->ref();
    onoff->addChild( root );
    onoff->whichChild = 0;
}


VisualObjectImpl::~VisualObjectImpl()
{
    getInventorNode()->unref();
    if ( material ) material->unRef();
}


void VisualObjectImpl::turnOn( bool yn )
{
    if ( onoff ) onoff->whichChild = yn ? 0 : SO_SWITCH_NONE;
    else if ( !yn )
    {
	pErrMsg( "Turning off object without switch");
    }

}


bool VisualObjectImpl::isOn() const
{
    return !onoff || !onoff->whichChild.getValue();
}


void VisualObjectImpl::setMaterial( Material* nm )
{
    if ( material )
    {
	root->removeChild( material->getInventorNode() );
	material->unRef();
    }

    material = nm;

    if ( material )
    {
	material->ref();
	root->insertChild( material->getInventorNode(), 0 );
    }
}


void VisualObjectImpl::removeSwitch()
{
    root->ref();
    onoff->unref();
    onoff = 0;
}


SoNode* VisualObjectImpl::getInventorNode() 
{ return onoff ? (SoNode*) onoff : (SoNode*) root; }


void VisualObjectImpl::addChild( SoNode* nn )
{ root->addChild( nn ); }


void VisualObjectImpl::insertChild( int pos, SoNode* nn )
{ root->insertChild( nn, pos ); }


void VisualObjectImpl::removeChild( SoNode* nn )
{ root->removeChild( nn ); }


int VisualObjectImpl::childIndex( const SoNode* nn ) const
{ return root->findChild(nn); }


SoNode* VisualObjectImpl::getChild(int idx)
{ return root->getChild(idx); }


int VisualObjectImpl::usePar( const IOPar& iopar )
{
    int res = VisualObject::usePar( iopar );
    if ( res != 1 ) return res;

    int matid;
    if ( iopar.get(materialidstr,matid) )
    {
	if ( matid==-1 ) setMaterial( 0 );
	else
	{
	    DataObject* mat = DM().getObject( matid );
	    if ( !mat ) return 0;
	    if ( typeid(*mat) != typeid(Material) ) return -1;

	    setMaterial( (Material*)mat );
	}
    }
    else
	setMaterial( 0 );

    bool isonsw;
    if ( iopar.getYN(isonstr,isonsw) )
	VisualObjectImpl::turnOn( isonsw );

    return 1;
}


void VisualObjectImpl::fillPar( IOPar& iopar,
					 TypeSet<int>& saveids ) const
{
    VisualObject::fillPar( iopar, saveids );
    iopar.set( materialidstr, material ? material->id() : -1 );

    if ( material && saveids.indexOf(material->id()) == -1 )
	saveids += material->id();

    iopar.setYN( isonstr, isOn() );
}


void VisualObject::triggerRightClick( const EventInfo* eventinfo_ )
{
    eventinfo = eventinfo_;
    rightClick.trigger();
}


const TypeSet<int>* VisualObject::rightClickedPath() const
{
    return eventinfo ? &eventinfo->pickedobjids : 0;
}

}; //namespace
