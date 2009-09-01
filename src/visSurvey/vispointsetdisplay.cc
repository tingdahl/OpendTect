/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Satyaki Maitra
 * DATE     : March 2009
-*/

static const char* rcsID = "$Id: vispointsetdisplay.cc,v 1.6 2009-09-01 06:14:51 cvssatyaki Exp $";

#include "selector.h"
#include "viscoord.h"
#include "vispointsetdisplay.h"

#include "datapointset.h"
#include "vispointset.h"
#include "vismaterial.h"


mCreateFactoryEntry( visSurvey::PointSetDisplay );


namespace visSurvey {


PointSetDisplay::PointSetDisplay()
    : VisualObjectImpl( true )
    , data_( * new DataPointSet(TypeSet<DataPointSet::DataRow>(),
				BufferStringSet(),false,true) )
{
    pointset_ = visBase::PointSet::create();
    addChild( pointset_->getInventorNode() );
}
    

PointSetDisplay::~PointSetDisplay()
{
    removeChild( pointset_->getInventorNode() );
}


void PointSetDisplay::setColor( Color col )
{ getMaterial()->setColor( col ); }


Color PointSetDisplay::getColor() const
{ return getMaterial()->getColor(); }


bool PointSetDisplay::setDataPack( const DataPointSet& dps )
{
    data_ = dps;
    update();

    return true;
}


void PointSetDisplay::update()
{
    pointset_->getCoordinates()->removeAfter(-1);
    getMaterial()->setColor( Color::DgbColor() );

    for ( int idx=0; idx<data_.size(); idx++ )
    {
	if ( data_.isSelected(idx) )
	    pointset_->getCoordinates()->addPos(
		    Coord3(data_.coord(idx),data_.z(idx)) );
    }
}


void PointSetDisplay::removeSelection( const Selector<Coord3>& selector )
{
    if ( !selector.isOK() )
	return;

    for ( int idx=0; idx<pointset_->getCoordinates()->size(true); idx++ )
    {
	Coord3 pos = pointset_->getCoordinates()->getPos( idx );
	if ( selector.includes(pos) )
	{
	    DataPointSet::RowID rid = data_.find( DataPointSet::Pos(pos) );
	    if ( rid < 0 )
		continue;
	    data_.setSelected( rid, false );
	}
    }

    update();
}


void PointSetDisplay::setDisplayTransformation( visBase::Transformation* nt )
{
    pointset_->setDisplayTransformation( nt );
}


visBase::Transformation* PointSetDisplay::getDisplayTransformation()
{ return pointset_->getDisplayTransformation(); }

} //namespace visSurvey
