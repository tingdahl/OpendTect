#ifndef uiaxishandler_h
#define uiaxishandler_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2008
 RCS:           $Id: uiaxishandler.h,v 1.7 2008-03-31 20:38:12 cvsbert Exp $
________________________________________________________________________

-*/

#include "draw.h"
#include "ranges.h"
#include "bufstringset.h"
#include "namedobj.h"
#include "uigeom.h"
class ioDrawTool;

/*!\brief Handles an axis on a plot

  Manages the positions in a 2D plot. The axis can be logarithmic. getRelPos
  returns the relative position on the axis. If the point is between the
  start and stop of the range, this will be between 0 and 1.

  The axis will determine a good position wrt the border. To determine where
  the axis starts and stops, you can provide other axis handlers. If you don't
  provide these, the border_ will be used. The border_ on the side of the axis
  will always be used. If you do use begin and end handlers, you'll have to
  call setRange() for all before using plotAxis().

  The drawAxis will plot the axis. If LineStyle::Type is not LineStyle::None,
  grid lines will be drawn, too. If it *is* None, then still the color and
  size will be used for drawing the axis (the axis' style is always Solid).

  Use AxisLayout (linear.h) to find 'nice' ranges, like:
  AxisLayout al( Interval<float>(start,stop) );
  ahndlr.setRange( StepInterval<float>(al.sd.start,al.stop,al.sd.step) );
 
 */

class uiAxisHandler : public NamedObject
{
public:

    struct Setup
    {
			Setup( uiRect::Side s )
			    : side_(s)
			    , noannot_(false)
			    , islog_(false)	{}

	mDefSetupMemb(uiRect::Side,side)
	mDefSetupMemb(bool,islog)
	mDefSetupMemb(bool,noannot)
	mDefSetupMemb(uiBorder,border)
	mDefSetupMemb(LineStyle,style)
	mDefSetupMemb(BufferString,name)
    };

			uiAxisHandler(ioDrawTool&,const Setup&);
    void		setRange(const StepInterval<float>&,float* axstart=0);
    void		setIsLog( bool yn )	{ setup_.islog_ = yn; reCalc();}
    void		setBorder( const uiBorder& b )
						{ setup_.border_ = b; reCalc();}
    void		setBegin( const uiAxisHandler* ah )
						{ beghndlr_ = ah; newDevSize();}
    void		setEnd( const uiAxisHandler* ah )
						{ endhndlr_ = ah; newDevSize();}

    float		getVal(int pix) const;
    float		getRelPos(float absval) const;
    int			getPix(float absval) const;

    void		plotAxis() const; //!< draws gridlines if appropriate

    const Setup&	setup() const	{ return setup_; }
    StepInterval<float>	range() const	{ return rg_; }
    bool		isHor() const	{ return uiRect::isHor(setup_.side_); }

    int			pixToEdge() const;
    int			pixBefore() const;
    int			pixAfter() const;

    void		newDevSize(); //!< Call this when appropriate

    void		drawGridLine(int) const; //!< Already called by plotAxis

protected:

    ioDrawTool&		dt_;
    Setup		setup_;
    bool		islog_;
    StepInterval<float>	rg_;
    float		annotstart_;
    uiBorder		border_;
    int			ticsz_;
    const uiAxisHandler* beghndlr_;
    const uiAxisHandler* endhndlr_;

    void		reCalc();
    int			wdthx_;
    int			wdthy_;
    BufferStringSet	strs_;
    TypeSet<float>	pos_;
    float		endpos_;
    int			devsz_;
    int			axsz_;
    bool		rgisrev_;
    float		rgwidth_;

    int			getRelPosPix(float) const;
    void		drawAxisLine() const;
    void		annotPos(int,const char*) const;
    void		drawName() const;

};


#endif
