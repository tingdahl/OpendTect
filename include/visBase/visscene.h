#ifndef visscene_h
#define visscene_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visscene.h,v 1.9 2002-11-15 08:14:32 kristofer Exp $
________________________________________________________________________


-*/


#include "sets.h"
#include "vissceneobjgroup.h"

class SoEnvironment;

namespace visBase
{
    class SelectionManager;
    class EventCatcher;

/*!\brief
    Scene manages all SceneObjects and has some managing
    functions such as the selection management and variables that should
    be common for the whole scene.
*/

class Scene : public SceneObjectGroup
{
public:
    static Scene*	create()
			mCreateDataObj(Scene);

    void		setAmbientLight( float );
    float		ambientLight() const;

    SoNode*		getData();

protected:
    virtual		~Scene();

private:
    EventCatcher&	mouseevents;
    int			mousedownid;

    void		mousePickCB( CallBacker* );

    SoEnvironment*	environment;
    SoGroup*		selroot;
};

};


#endif
