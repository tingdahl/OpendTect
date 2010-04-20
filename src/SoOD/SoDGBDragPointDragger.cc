/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Karthika
 Date:          March 2010
________________________________________________________________________

-*/

static const char* rcsID = "$Id: SoDGBDragPointDragger.cc,v 1.12 2010-04-20 21:39:01 cvskarthika Exp $";

#include "SoDGBDragPointDragger.h"

#include <Inventor/SbRotation.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/nodes/SoRotation.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoCylinder.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/sensors/SoFieldSensor.h>
#include <Inventor/projectors/SbLineProjector.h>
#include <Inventor/projectors/SbPlaneProjector.h>

SO_KIT_SOURCE(SoDGBDragPointDragger);

void SoDGBDragPointDragger::initClass()
{
    SO_KIT_INIT_CLASS(SoDGBDragPointDragger, SoDragger, "Dragger" );
}


SoDGBDragPointDragger::SoDGBDragPointDragger()
{
    SO_KIT_CONSTRUCTOR(SoDGBDragPointDragger);

    // noRotSep
    SO_KIT_ADD_CATALOG_ENTRY(noRotSep, SoSeparator, FALSE, topSeparator, 
	    rotXSep, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(xTranslatorSwitch, SoSwitch, FALSE, noRotSep, 
	    xyTranslatorSwitch, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(xTranslator, SoSeparator, TRUE, xTranslatorSwitch, 
	    "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(xyTranslatorSwitch, SoSwitch, FALSE, noRotSep, 
	    "", FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(xyTranslator, SoSeparator, TRUE, 
	    xyTranslatorSwitch, "", TRUE);

    // rotXSep
    SO_KIT_ADD_CATALOG_ENTRY(rotXSep, SoSeparator, FALSE, topSeparator, 
	    rotYSep, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(rotX, SoRotation, TRUE, rotXSep, 
	    xzTranslatorSwitch, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(xzTranslatorSwitch, SoSwitch, FALSE, rotXSep, 
	    "", FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(xzTranslator, SoSeparator, TRUE, 
	    xzTranslatorSwitch, "", TRUE);
    
    // rotYSep
    SO_KIT_ADD_CATALOG_ENTRY(rotYSep, SoSeparator, FALSE, topSeparator, 
	    rotZSep, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(rotY, SoRotation, TRUE, rotYSep, 
	    zTranslatorSwitch, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(zTranslatorSwitch, SoSwitch, FALSE, rotYSep, 
	    yzTranslatorSwitch, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(zTranslator, SoSeparator, TRUE, 
	    zTranslatorSwitch, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(yzTranslatorSwitch, SoSwitch, FALSE, rotYSep, 
	    "", FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(yzTranslator, SoSeparator, TRUE, 
	    yzTranslatorSwitch, "", TRUE);
    
    // rotZSep
    SO_KIT_ADD_CATALOG_ENTRY(rotZSep, SoSeparator, FALSE, topSeparator, 
	    xFeedbackSwitch, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(rotZ, SoRotation, TRUE, rotZSep, 
	    yTranslatorSwitch, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(yTranslatorSwitch, SoSwitch, FALSE, rotZSep, 
	    "", FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(yTranslator, SoSeparator, TRUE, yTranslatorSwitch, 
	    "", TRUE);
    
    // X feedback
    SO_KIT_ADD_CATALOG_ENTRY(xFeedbackSwitch, SoSwitch, FALSE, topSeparator, 
	    yFeedbackSwitch, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(xFeedbackSep, SoSeparator, FALSE, xFeedbackSwitch, 
	    "", FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(xFeedbackTranslation, SoTranslation, FALSE, 
	    xFeedbackSep, xFeedback, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(xFeedback, SoSeparator, TRUE, xFeedbackSep, 
	    "", TRUE);
    
    // Y feedback
    SO_KIT_ADD_CATALOG_ENTRY(yFeedbackSwitch, SoSwitch, FALSE, topSeparator, 
	    zFeedbackSwitch, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(yFeedbackSep, SoSeparator, FALSE, yFeedbackSwitch, 
	    "", FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(yFeedbackTranslation, SoTranslation, FALSE, 
	    yFeedbackSep, yFeedback, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(yFeedback, SoSeparator, TRUE, yFeedbackSep, 
	    "", TRUE);
    
    // Z feedback
    SO_KIT_ADD_CATALOG_ENTRY(zFeedbackSwitch, SoSwitch, FALSE, topSeparator, 
	    planeFeedbackSep, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(zFeedbackSep, SoSeparator, FALSE, zFeedbackSwitch, 
	    "", FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(zFeedbackTranslation, SoTranslation, FALSE, 
	    zFeedbackSep, zFeedback, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(zFeedback, SoSeparator, TRUE, zFeedbackSep, 
	    "", TRUE);
    
    // Plane feedback
    SO_KIT_ADD_CATALOG_ENTRY(planeFeedbackSep, SoSeparator, FALSE, 
	    topSeparator, geomSeparator, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(planeFeedbackTranslation, SoTranslation, FALSE, 
	    planeFeedbackSep, planeFeedbackSwitch, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(planeFeedbackSwitch, SoSwitch, FALSE, 
	    planeFeedbackSep, "", FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(yzFeedback, SoSeparator, TRUE, 
	    planeFeedbackSwitch, xzFeedback, TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(xzFeedback, SoSeparator, TRUE, 
	    planeFeedbackSwitch, xyFeedback, TRUE);    
    SO_KIT_ADD_CATALOG_ENTRY(xyFeedback, SoSeparator, TRUE, 
	    planeFeedbackSwitch, "", TRUE);
   
    if (SO_KIT_IS_FIRST_INSTANCE())
    {
	SoInteractionKit::readDefaultParts( "",	draggergeometry_, 
		static_cast <int> ( strlen( draggergeometry_ ) ) );
    }

    SO_KIT_ADD_FIELD(translation, (0.0f, 0.0f, 0.0f));
    SO_KIT_ADD_FIELD(restrictdragging, (true));
    SO_KIT_INIT_INSTANCE();

    // initialize default parts pertaining to feedback
    this->setPartAsDefault( "xFeedback", "dragPointXFeedback" );
    this->setPartAsDefault( "yFeedback", "dragPointYFeedback" );
    this->setPartAsDefault( "zFeedback", "dragPointZFeedback" );
    this->setPartAsDefault( "yzFeedback", "dragPointYZFeedback" );
    this->setPartAsDefault( "xzFeedback", "dragPointXZFeedback" );
    this->setPartAsDefault( "xyFeedback", "dragPointXYFeedback" );
    this->setPartAsDefault( "xAxisFeedback", "dragPointPlaneXAxisFeedback" );
    this->setPartAsDefault( "yAxisFeedback", "dragPointPlaneYAxisFeedback" );
    this->setPartAsDefault( "zAxisFeedback", "dragPointPlaneZAxisFeedback" );

    // initialise the translators to inactive geometry states
    this->setPartAsDefault( linetranslatornames_[0],
		"dragPointXTranslatorTranslator" );
    this->setPartAsDefault( linetranslatornames_[1],
		"dragPointYTranslatorTranslator" );
    this->setPartAsDefault( linetranslatornames_[2],
		"dragPointZTranslatorTranslator" );
    this->setPartAsDefault( planetranslatornames_[0],
		"dragPointYZTranslatorTranslator" );
    this->setPartAsDefault( planetranslatornames_[1], 
		"dragPointXZTranslatorTranslator" );
    this->setPartAsDefault( planetranslatornames_[2], 
		"dragPointXYTranslatorTranslator" );
 
    // set rotations to align draggers to their respective axis/planes
    SoRotation* xrot = new SoRotation;
    xrot->rotation.setValue(SbRotation(
	SbVec3f(1.0f, 0.0f, 0.0f), ( static_cast <float> ( M_PI ) ) * 0.5f ) );
    this->setAnyPartAsDefault("rotX", xrot);
    SoRotation* yrot = new SoRotation;
    yrot->rotation.setValue(SbRotation(
	SbVec3f(0.0f, 1.0f, 0.0f), ( static_cast <float> ( M_PI ) ) * 0.5f ) );
    this->setAnyPartAsDefault("rotY", yrot);
    SoRotation* zrot = new SoRotation;
    zrot->rotation.setValue(SbRotation(
	SbVec3f(0.0f, 0.0f, 1.0f), ( static_cast <float> ( M_PI ) ) * 0.5f ) );
    this->setAnyPartAsDefault("rotZ", zrot);

    // initialize feedback switch nodes
    SoSwitch* sw;
    sw = SO_GET_ANY_PART( this, "planeFeedbackSwitch", SoSwitch );
    SoInteractionKit::setSwitchValue( sw, SO_SWITCH_NONE );
    for ( int i = 0; i < 3; i++ )
    {
	sw = SO_GET_ANY_PART( this, linefbswitchnames_[i], SoSwitch );
	SoInteractionKit::setSwitchValue( sw, SO_SWITCH_NONE );
    }

    // set up projector
    this->lineproj_ = new SbLineProjector();
    this->planeproj_ = new SbPlaneProjector();

    this->curraxis_ = 2;
    this->updateSwitchNodes();
    movecyl_ = false;

    this->addStartCallback( SoDGBDragPointDragger::startCB, this );
    this->addMotionCallback( SoDGBDragPointDragger::motionCB, this );
    this->addFinishCallback( SoDGBDragPointDragger::finishCB, this );
    this->addValueChangedCallback( SoDGBDragPointDragger::valueChangedCB );
    this->fieldSensor = new SoFieldSensor( 
		SoDGBDragPointDragger::fieldSensorCB, this );
    this->fieldSensor->setPriority( 0 );

    this->setUpConnections( TRUE, TRUE );
}


SoDGBDragPointDragger::~SoDGBDragPointDragger()
{
    delete this->fieldSensor;
    delete lineproj_;
    delete planeproj_;
}


SbBool SoDGBDragPointDragger::setUpConnections( SbBool onoff, 
	SbBool doitalways )
{
    if ( !doitalways && this->connectionsSetUp == onoff ) return onoff;

    SbBool oldval = this->connectionsSetUp;

    if ( onoff )
    {
	SoDragger::setUpConnections( onoff, doitalways );

	SoDGBDragPointDragger::fieldSensorCB( this, NULL );

	if ( this->fieldSensor->getAttachedField() != &this->translation )
	    this->fieldSensor->attach( &this->translation );
    }
    else 
    {
        if ( this->fieldSensor->getAttachedField() != NULL )
	    this->fieldSensor->detach();
	SoDragger::setUpConnections( onoff, doitalways );
    }
   
    this->connectionsSetUp = onoff;
    return oldval;
}


void SoDGBDragPointDragger::setDefaultOnNonWritingFields()
{
    this->xTranslator.setDefault( TRUE );
    this->yTranslator.setDefault( TRUE );
    this->zTranslator.setDefault( TRUE );

    this->xyTranslator.setDefault( TRUE );
    this->xzTranslator.setDefault( TRUE );
    this->yzTranslator.setDefault( TRUE );

    this->planeFeedbackTranslation.setDefault (TRUE );
    this->xFeedbackTranslation.setDefault( TRUE );
    this->yFeedbackTranslation.setDefault( TRUE );
    this->zFeedbackTranslation.setDefault( TRUE );

    SoDragger::setDefaultOnNonWritingFields();
}


void SoDGBDragPointDragger::fieldSensorCB( void* d, SoSensor* )
{
    SoDGBDragPointDragger* thisp = static_cast <SoDGBDragPointDragger*> ( d );
    SbMatrix matrix = thisp->getMotionMatrix();
    thisp->workFieldsIntoTransform( matrix );
    thisp->setMotionMatrix( matrix );
}


void SoDGBDragPointDragger::valueChangedCB( void*, SoDragger* d )
{
    SoDGBDragPointDragger* thisp = static_cast <SoDGBDragPointDragger*> ( d );

    SbMatrix matrix = thisp->getMotionMatrix();
    SbVec3f t;
    t[0] = matrix[3][0];
    t[1] = matrix[3][1];
    t[2] = matrix[3][2];

    thisp->fieldSensor->detach();
    if ( thisp->translation.getValue() != t )
	thisp->translation = t;

    thisp->fieldSensor->attach( &thisp->translation );
}


void SoDGBDragPointDragger::dragStart()
{
    SoCylinder* cyl = SO_GET_ANY_PART( this, linetranslatornames_[curraxis_], 
		    SoCylinder );
    SoCube* cube = SO_GET_ANY_PART( this, planetranslatornames_[curraxis_], 
		    SoCube );
    
    if ( !cyl || !cube )
	return;

    // determine if the cylinder was picked or the cube
    if ( !determineDragDirection( cyl, cube ) )
	return;

    SoSwitch* sw;
    if ( movecyl_ )
    {
        sw = SO_GET_ANY_PART( this, linefbswitchnames_[curraxis_], SoSwitch );
	SoInteractionKit::setSwitchValue( sw, 0 );
        
	// set the active part 
	if ( curraxis_ == 0 )
	    setPartAsDefault( linetranslatornames_[0],
		    "dragPointXTranslatorTranslatorActive" );
	else if ( curraxis_ == 1 )
	    setPartAsDefault( linetranslatornames_[1],
		    "dragPointYTranslatorTranslatorActive" );
	else if (curraxis_ == 2 )
	    setPartAsDefault( linetranslatornames_[2],
		    "dragPointZTranslatorTranslatorActive" );
 
	SbVec3f hitpt = this->getLocalStartingPoint();
	SbVec3f endpt( 0.0f, 0.0f, 0.0f );
	endpt[curraxis_] = 1.0f;
	lineproj_->setLine( SbLine( hitpt, hitpt + endpt ) );
    }
    else
    {
	sw = SO_GET_ANY_PART( this, "planeFeedbackSwitch", SoSwitch );
	SoInteractionKit::setSwitchValue( sw, this->curraxis_ );

	// set the active part and the axes feedback
	const char* activepartname = "";
	
	if ( curraxis_ == 0 )
	{
	    setPartAsDefault( planetranslatornames_[0],
		"dragPointYZTranslatorTranslatorActive" );
	}
	else if ( curraxis_ == 1 )
	{
	    setPartAsDefault( planetranslatornames_[1], 
		"dragPointXZTranslatorTranslatorActive" );
	}
	else if (curraxis_ == 2 )
	{
	    setPartAsDefault( planetranslatornames_[2], 
		"dragPointXYTranslatorTranslatorActive" );
	}

	SbVec3f hitpt = this->getLocalStartingPoint();
	SbVec3f endpt( 0.0f, 0.0f, 0.0f );
	endpt[curraxis_] = 1.0f;
	planeproj_->setPlane( SbPlane( endpt, hitpt ) );
    }
}


bool SoDGBDragPointDragger::determineDragDirection( const SoCylinder* cyl, 
		const SoCube* cube )
{
    // find the orientation of the dragger to the 3 axes
    SbViewVolume vw = getViewVolume();
    SbVec3f worldprojdir = vw.getProjectionDirection();
    const SbMatrix& mat = getWorldToLocalMatrix();
    SbVec3f localprojdir;
    mat.multDirMatrix( worldprojdir, localprojdir );
    localprojdir.normalize();

	// conditions have been tested for only when curraxis_ is 1
    bool set = false;
    
    if ( restrictdragging.getValue() )
    {
	const float angletox = fabs( localprojdir[0] );
	const float angletoy = fabs( localprojdir[1] );
	const float angletoz = fabs( localprojdir[2] );
	const float upperlimit = 0.7;
	const float lowerlimit = 0.3;
    
    	// When the cylinder is lying flat (almost along the Z axis), restrict 
    	// picking the cylinder. User probably wants to move just the rectangle 
    	// but has picked the cylinder by mistake.
    	//
    	// When the cylinder is upright or lying along the X axis, restrict 
    	// picking the rectangle. User probably wants to move just the cylinder 
    	// but has picked the rectangle by mistake.

    	if ( curraxis_ == 1 )
    	{
	    if ( ( angletox <= lowerlimit ) && ( angletoy >= upperlimit ) )
	    {
		movecyl_ = false;
		set = true;
	    }
	    else if ( angletoy <= lowerlimit )
	    {
		movecyl_ = true;
		set = true;
	    }
        }
    	else if ( curraxis_ == 2 )
    	{
	    if ( ( angletoy <= lowerlimit ) && ( angletoz >= upperlimit ) )
	    {
		movecyl_ = false;
		set = true;
	    }
	    else if ( angletoz <= lowerlimit )
	    {
		movecyl_ = true;
		set = true;
	    }
        }
    }
    
    if ( !set) 
    {
        // Let the user drag as desired. Find which object the user has picked.
	const SoPath* pickpath = getPickPath();
	if ( pickpath->containsNode( cyl ) )
	{
	    movecyl_ = true;
	    set = true;
	}
	else if ( pickpath->containsNode( cube ) )
	{
	    movecyl_ = false;
	    set = true;
	}
    }
	
    return set;
}


void SoDGBDragPointDragger::drag()
{
    if ( movecyl_ )
    {
        lineproj_->setViewVolume( this->getViewVolume() );
	lineproj_->setWorkingSpace( this->getLocalToWorldMatrix() );
  
	const float epsilon = this->getProjectorEpsilon();
	SbVec3f projpt;
	
	if ( lineproj_->tryProject( this->getNormalizedLocaterPosition(), 
			epsilon, projpt ) )
	{
	    SbVec3f startpt = this->getLocalStartingPoint();
	    SbVec3f motion = projpt - startpt;
	    SbMatrix mm = this->appendTranslation( 
			    this->getStartMotionMatrix(), motion );
	    this->setMotionMatrix( mm );
	}
    }
    else
    {
	this->planeproj_->setViewVolume( this->getViewVolume() );
	this->planeproj_->setWorkingSpace( this->getLocalToWorldMatrix() );

	SbVec3f projpt;
	if (this->planeproj_->tryProject( this->getNormalizedLocaterPosition(),
				this->getProjectorEpsilon(), projpt ) )
	{
	    SbVec3f startpt = this->getLocalStartingPoint();
	    this->setMotionMatrix( this->appendTranslation( 
		this->getStartMotionMatrix(), projpt - startpt ) );
        }
    }
}


void SoDGBDragPointDragger::dragFinish()
{
    SoSwitch* sw;
    
    if ( movecyl_ )
    {
	sw = SO_GET_ANY_PART( this, linefbswitchnames_[curraxis_], SoSwitch );
	SoInteractionKit::setSwitchValue( sw, SO_SWITCH_NONE );
	    
	// set the inactive part
    	if ( curraxis_ == 0 )
	    setPartAsDefault( linetranslatornames_[0],
		    "dragPointXTranslatorTranslator" );
	else if ( curraxis_ == 1 )
	    setPartAsDefault( linetranslatornames_[1],
		    "dragPointYTranslatorTranslator" );
	else if (curraxis_ == 2 )
	    setPartAsDefault( linetranslatornames_[2],
		    "dragPointZTranslatorTranslator" );
    } 
    else
    {
	sw = SO_GET_ANY_PART( this, "planeFeedbackSwitch", SoSwitch );
	SoInteractionKit::setSwitchValue( sw, SO_SWITCH_NONE );

	// set the inactive part
	const char* activepartname = "";
	if ( curraxis_ == 0 )
	    setPartAsDefault( planetranslatornames_[0],
		"dragPointYZTranslatorTranslator" );
	else if ( curraxis_ == 1 )
	    setPartAsDefault( planetranslatornames_[1], 
		"dragPointXZTranslatorTranslator" );
	else if (curraxis_ == 2 )
	    setPartAsDefault( planetranslatornames_[2], 
		"dragPointXYTranslatorTranslator" );
    }
}


void SoDGBDragPointDragger::startCB( void* d, SoDragger* )
{
    SoDGBDragPointDragger* thisp = static_cast <SoDGBDragPointDragger*> ( d );
    thisp->dragStart();
}


void SoDGBDragPointDragger::motionCB( void* d, SoDragger* )
{
    SoDGBDragPointDragger* thisp = static_cast <SoDGBDragPointDragger*> ( d );
    thisp->drag();
}


void SoDGBDragPointDragger::finishCB( void* d, SoDragger* )
{
    SoDGBDragPointDragger* thisp = static_cast <SoDGBDragPointDragger*> ( d );
    thisp->dragFinish();
}


void SoDGBDragPointDragger::updateSwitchNodes()
{
    // switch the dragger geometry
    SoSwitch *sw;
    
    sw = SO_GET_ANY_PART( this, "xTranslatorSwitch", SoSwitch );
    SoInteractionKit::setSwitchValue( sw, this->curraxis_ == 0 ? 
	    0 : SO_SWITCH_NONE );
    sw = SO_GET_ANY_PART( this, "yTranslatorSwitch", SoSwitch );
    SoInteractionKit::setSwitchValue( sw, this->curraxis_ == 1 ? 
	    0 : SO_SWITCH_NONE );
    sw = SO_GET_ANY_PART( this, "zTranslatorSwitch", SoSwitch );
    SoInteractionKit::setSwitchValue( sw, this->curraxis_ == 2 ? 
	    0 : SO_SWITCH_NONE );

    sw = SO_GET_ANY_PART( this, "yzTranslatorSwitch", SoSwitch );
    SoInteractionKit::setSwitchValue( sw, this->curraxis_ == 0 ? 
	    0 : SO_SWITCH_NONE );
    sw = SO_GET_ANY_PART( this, "xzTranslatorSwitch", SoSwitch );
    SoInteractionKit::setSwitchValue( sw, this->curraxis_ == 1 ? 
	    0 : SO_SWITCH_NONE );
    sw = SO_GET_ANY_PART( this, "xyTranslatorSwitch", SoSwitch );
    SoInteractionKit::setSwitchValue( sw, this->curraxis_ == 2 ? 
	    0 : SO_SWITCH_NONE );
}


const char* SoDGBDragPointDragger::linefbswitchnames_[] = 
{ "xFeedbackSwitch", "yFeedbackSwitch", "zFeedbackSwitch" };

const char* SoDGBDragPointDragger::linetranslatornames_[] = 
{ "xTranslator", "yTranslator", "zTranslator" };

const char* SoDGBDragPointDragger::planetranslatornames_[] = 
{ "yzTranslator", "xzTranslator", "xyTranslator" };

const char* SoDGBDragPointDragger::draggergeometry_ =
  "#Inventor V2.1 ascii\n"
  "\n"
  "\n"
  "\n"
  "DEF DRAGPOINT_INACTIVE_MATERIAL Material { diffuseColor 0.5 0.5 0.5  emissiveColor 0.5 0.5 0.5 }\n"
  "DEF DRAGPOINT_ACTIVE_MATERIAL Material { diffuseColor 0.5 0.5 0  emissiveColor 0.5 0.5 0 }\n"
  "DEF DRAGPOINT_FEEDBACK_MATERIAL Material { diffuseColor 0.5 0 0.5  emissiveColor  0.5 0 0.5  transparency 0.2 }\n"
  "\n"
  "\n"
  "\n"
  "DEF DRAGPOINT_STICK Group {\n"
  "   Rotation { rotation 0 0 1  1.57 }\n"
  "   Cylinder { height 1.5 radius 0.2 }\n"
  "}\n"
  "\n"
  "DEF DRAGPOINT_INACTIVE_STICK Separator {\n"
  "   USE DRAGPOINT_INACTIVE_MATERIAL\n"
  "   USE DRAGPOINT_STICK\n"
  "}\n"
  "DEF DRAGPOINT_ACTIVE_STICK Separator {\n"
  "   USE DRAGPOINT_ACTIVE_MATERIAL\n"
  "   USE DRAGPOINT_STICK\n"
  "}\n"
  "\n"
  "DEF dragPointXTranslatorTranslator Separator { USE DRAGPOINT_INACTIVE_STICK } \n"
  "DEF dragPointXTranslatorTranslatorActive Separator { USE DRAGPOINT_ACTIVE_STICK }\n"
  "DEF dragPointYTranslatorTranslator Separator { USE DRAGPOINT_INACTIVE_STICK }\n"
  "DEF dragPointYTranslatorTranslatorActive Separator { USE DRAGPOINT_ACTIVE_STICK }\n"
  "DEF dragPointZTranslatorTranslator Separator { USE DRAGPOINT_INACTIVE_STICK }\n"
  "DEF dragPointZTranslatorTranslatorActive Separator { USE DRAGPOINT_ACTIVE_STICK }\n"
  "\n"
  "\n"
  "DEF DRAGPOINT_PLANE Group { Cube { width 1  height 1  depth .1 } }\n"
  "\n"
  "DEF DRAGPOINT_INACTIVE_PLANE Separator {\n"
  "   USE DRAGPOINT_INACTIVE_MATERIAL\n"
  "   USE DRAGPOINT_PLANE\n"
  "}\n"
  "DEF DRAGPOINT_ACTIVE_PLANE Separator {\n"
  "   USE DRAGPOINT_ACTIVE_MATERIAL\n"
  "   USE DRAGPOINT_PLANE\n"
  "}\n"
  "\n"
  "DEF dragPointXYTranslatorTranslator Separator { USE DRAGPOINT_INACTIVE_PLANE }\n"
  "DEF dragPointXYTranslatorTranslatorActive Separator { USE DRAGPOINT_ACTIVE_PLANE }\n"
  "DEF dragPointXZTranslatorTranslator Separator { USE DRAGPOINT_INACTIVE_PLANE }\n"
  "DEF dragPointXZTranslatorTranslatorActive Separator { USE DRAGPOINT_ACTIVE_PLANE }\n"
  "DEF dragPointYZTranslatorTranslator Separator { USE DRAGPOINT_INACTIVE_PLANE }\n"
  "DEF dragPointYZTranslatorTranslatorActive Separator { USE DRAGPOINT_ACTIVE_PLANE }\n"
  "\n"
  "\n"
  "DEF DRAGPOINT_FEEDBACK_LINE Group {\n"
  "   Coordinate3 { point [ 0 -10 0, 0 10 0 ] }\n"
  "   LineSet { }\n"
  "\n"
  "   Transform { translation 0 10 0 }\n"
  "   DEF DRAGPOINT_FEEDBACK_ARROWHEAD Cone { height 0.5 bottomRadius 0.5 }\n"
  "   Transform { translation 0 -20 0 }\n"
  "   Rotation { rotation 0 0 1  3.14 }\n"
  "   USE DRAGPOINT_FEEDBACK_ARROWHEAD\n"
  "}\n"
  "\n"
  "DEF dragPointXFeedback Separator {\n"
  "   USE DRAGPOINT_FEEDBACK_MATERIAL\n"
  "   Rotation { rotation 0 0 1 1.57 }\n"
  "   USE DRAGPOINT_FEEDBACK_LINE\n"
  "}\n"
  "DEF dragPointYFeedback Separator {\n"
  "   USE DRAGPOINT_FEEDBACK_MATERIAL\n"
  "   USE DRAGPOINT_FEEDBACK_LINE\n"
  "}\n"
  "DEF dragPointZFeedback Separator {\n"
  "   USE DRAGPOINT_FEEDBACK_MATERIAL\n"
  "   Rotation { rotation 1 0 0 1.57 }\n"
  "   USE DRAGPOINT_FEEDBACK_LINE\n"
  "}\n"
  "\n"
  "\n"
  "DEF DRAGPOINT_FEEDBACK_PLANE Group {\n"
  "   ShapeHints { shapeType UNKNOWN_SHAPE_TYPE }\n"
  "   Coordinate3 { point [ -10 0 -10, -10 0 10, 10 0 10, 10 0 -10, -10 0 -10 ] }\n"
  "   FaceSet { }\n"
  "   Scale { scaleFactor 1.05 1 1.05 }\n"
  "   LineSet { }\n"
  "}\n"
  "\n"
  "DEF dragPointXYFeedback Separator {\n"
  "   USE DRAGPOINT_FEEDBACK_MATERIAL\n"
  "   Rotation { rotation 1 0 0  1.57 }\n"
  "   USE DRAGPOINT_FEEDBACK_PLANE\n"
  "}\n"
  "DEF dragPointXZFeedback Separator {\n"
  "   USE DRAGPOINT_FEEDBACK_MATERIAL\n"
  "   USE DRAGPOINT_FEEDBACK_PLANE\n"
  "}\n"
  "DEF dragPointYZFeedback Separator {\n"
  "   USE DRAGPOINT_FEEDBACK_MATERIAL\n"
  "   Rotation { rotation 0 0 1  1.57 }\n"
  "   USE DRAGPOINT_FEEDBACK_PLANE\n"
  "}\n"
  "\n"
  "DEF dragPointPlaneXAxisFeedback Separator {\n"
  "  USE DRAGPOINT_ACTIVE_MATERIAL\n"
  "  DrawStyle { lineWidth 2 }\n"
  "  Coordinate3 { point [ -3 0 0, 3 0 0 ] }\n"
  "  LineSet { }\n"
  "}\n"
  "DEF dragPointPlaneYAxisFeedback Separator {\n"
  "  USE DRAGPOINT_ACTIVE_MATERIAL\n"
  "  DrawStyle { lineWidth 2 }\n"
  "  Coordinate3 { point [ 0 -3 0, 0 3 0 ] }\n"
  "  LineSet { }\n"
  "}\n"
  "DEF dragPointPlaneZAxisFeedback Separator {\n"
  "  USE DRAGPOINT_ACTIVE_MATERIAL\n"
  "  DrawStyle { lineWidth 2 }\n"
  "  Coordinate3 { point [ 0 0 -3, 0 0 3 ] }\n"
  "  LineSet { }\n"
  "}\n";

