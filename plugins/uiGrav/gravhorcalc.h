#ifndef gravhorcalc_h
#define gravhorcalc_h
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Mar 2010
 RCS:		$Id: gravhorcalc.h,v 1.2 2010-04-20 10:00:59 cvsbert Exp $
________________________________________________________________________

*/

#include "executor.h"
#include "grav.h"
class MultiID;


namespace Grav
{


class HorCalc : public ::Executor
{
public:

			HorCalc(const MultiID&,const MultiID* top=0,
				const MultiID* bot=0,float ang=1);

    void		setCutOffAngle( float a )	{ cutoffangle_ = a; }

protected:

    float		cutoffangle_;

};

} // namespace Grav


#endif
