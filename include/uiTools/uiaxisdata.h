#ifndef uiaxisdata_h
#define uiaxisdata_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene
 Date:          Jul 2008
 RCS:           $Id: uiaxisdata.h,v 1.1 2008-07-31 10:45:49 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiaxishandler.h"
#include "statruncalc.h"
#include "iodrawtool.h"

class DataClipper;

/*!\brief convenient base class to carry axis data:
  	# the AxisHandler which handles the behaviour and positioning of
	  an axis in a 2D plot;
	# axis scaling parameters
	# axis ranges
 */

class uiAxisData
{
public:

			uiAxisData(uiRect::Side);
			~uiAxisData();

    virtual void	stop();
    void		setRange( const Interval<float>& rg ) { rg_ = rg; }

    struct AutoScalePars
    {
			AutoScalePars();

	bool            doautoscale_;
	float           clipratio_;

	static float    defclipratio_;
	//!< 1) settings "AxisData.Clip Ratio"
	//!< 2) env "OD_DEFAULT_AXIS_CLIPRATIO"
	//!< 3) zero
    };

    uiAxisHandler*		axis_;
    AutoScalePars		autoscalepars_;
    Interval<float>		rg_;

    bool			needautoscale_;
    uiAxisHandler::Setup	defaxsu_;
    bool			isreset_;

    void			handleAutoScale(const Stats::RunCalc<float>&);
    void			handleAutoScale(const DataClipper&);
    void			newDevSize();
    void			renewAxis(const char*,ioDrawTool&,
	    				  const Interval<float>*);
};

#endif
