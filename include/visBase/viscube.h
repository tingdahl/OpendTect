#ifndef viscube_h
#define viscube_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: viscube.h,v 1.9 2002-11-15 08:14:32 kristofer Exp $
________________________________________________________________________


-*/

#include "visobject.h"
#include "position.h"

class SoCube;
class SoTranslation;

namespace Geometry { class Pos; };

namespace visBase
{
class Scene;

/*!\brief

Cube is a basic cube that is settable in size.

*/

class Cube : public VisualObjectImpl
{
public:
    static Cube*	create()
			mCreateDataObj(Cube);

    void		setCenterPos( const Coord3& );
    Coord3		centerPos() const;
    
    void		setWidth( const Coord3& );
    Coord3		width() const;

    int			usePar( const IOPar& );
    void		fillPar( IOPar&, TypeSet<int>& ) const;

protected:
    SoCube*		cube;
    SoTranslation*	position;
    static const char*	centerposstr;
    static const char*	widthstr;
};

};


#endif
