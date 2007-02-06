#ifndef uiworld2ui_h
#define uiworld2ui_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          13/8/2000
 RCS:           $Id: uiworld2ui.h,v 1.12 2007-02-06 18:01:48 cvsbert Exp $
________________________________________________________________________

-*/
#include "uigeom.h"

class World2UiData
{
public:

		World2UiData();
		World2UiData( uiSize s, const uiWorldRect& w );
		World2UiData( const uiWorldRect& w, uiSize s );

    bool	operator ==( const World2UiData& w2ud ) const;
    bool	operator !=( const World2UiData& w2ud ) const;

    uiSize	sz;
    uiWorldRect	wr;
};


/*!\brief Class to provide coordinate conversion between a cartesian coordinate
  system(or any other transformed cartesian) and UI coordinate system(screen
  coordinate system)
  
  Use the constructor or call set() to set up the two coordinate systems.
  1) If the origin of UI is not at (0,0), use uiRect instead of uiSize to set
     up the UI coordinates.
  2) In many cases the 'border' of world coordinate system is not at a good
     place, e.x. X ( 0.047 - 0.987 ). we would prefer to set it to an more
     appropriate range ( 0 - 1 ) and re-map this range to the UI window. This 
     is done by calling setRemap() or setCartesianRemap(). The proper range is
     estimated by these functions and coordinate conversion will be based on
     the new wolrd X/Y range.
 */

class uiWorld2Ui
{
public:

			uiWorld2Ui();
			uiWorld2Ui( uiSize sz, const uiWorldRect& wr );
			uiWorld2Ui( const uiWorldRect& wr, uiSize sz );
			uiWorld2Ui( const World2UiData& w );

    bool		operator==(const uiWorld2Ui& ) const;

    void		set( const World2UiData& w );
    void		set( const uiWorldRect& wr, uiSize sz );
    void		set( uiSize sz, const uiWorldRect& wr );
    void		set( const uiRect& rc, const uiWorldRect& wr );
    void		setRemap( const uiSize& sz, const uiWorldRect& wrdrc );
    void		setRemap( const uiRect& rc, const uiWorldRect& wrdrc );
			//! For most cases we are dealing with cartesian coord.
			//! system. Just pass the X/Y range to set up
    void		setCartesianRemap( const uiSize& sz,
			    float minx, float maxx, float miny, float maxy );
    void		setCartesianRemap( const uiRect& rc,
			    float minx, float maxx, float miny, float maxy );
			
			//! The following reset...() functions must be called
			//! ONLY AFTER a call to set...() is made.
    void		resetUiSize( const uiSize& sz );
    void		resetUiRect( const uiRect& rc );
    void		resetWorldRect( const uiWorldRect& wr );
    void		resetWorldRectRemap( const uiWorldRect& wr );

    uiWorld2Ui		getMirrored(bool lr,bool tb) const;
    			/*!\param lr	mirror left-right
			   \param tb	mirror top-bottom
			   \returns 	mirrored version of this transform.*/
    const World2UiData&	world2UiData() const;


    uiWorldPoint	transform( uiPoint p ) const;
    uiWorldRect		transform( uiRect area ) const;
    uiPoint		transform( uiWorldPoint p ) const;
    uiRect		transform( uiWorldRect area ) const;
			// Since the compiler will be comfused if two functions
			// only differ in return type, Geom::Point2D<float> is
			// set rather than be returned.
    void		transform( const uiPoint& upt,
	    			   Geom::Point2D<float>& pt ) const;
    void		transform( const Geom::Point2D<float>& pt,
	    			   uiPoint& upt) const;
    
    int			toUiX( float wrdx ) const;
    int			toUiY( float wrdy ) const;
    float		toWorldX( int uix ) const;
    float		toWorldY( int uiy ) const;

    uiWorldRect		xyRng2WrdRect( float minx, float maxx,
    				       float miny, float maxy );

    uiWorldPoint	origin() const;
    uiWorldPoint	worldPerPixel() const;
			//!< numbers may be negative
			
			//! If the world X/Y range has been re-calculated by 
			//! calling setRemap() or setCartesianRemap(), call
			//! these two functions to get the new value
    void		getWorldXRange( float& xmin, float& xmax ) const;
    void		getWorldYRange( float& ymin, float& ymax ) const;
			//! Get a recommended step value for annotating X/Y axis
			//! The step value will always be positive which means
			//! stepping from min. to max. value
    void		getRecmMarkStep( float& xstep, float& ystep ) const;

protected:

    World2UiData	w2ud;

    uiWorldPoint	p0;
    uiWorldPoint	fac;

    uiWorldRect		wrdrect_; //!< the world rect used for coord. conv.
    uiSize		uisize_;
    uiPoint		uiorigin;

private:
			void getAppopriateRange( float min, float max,
						 float& newmin, float& newmax );
			void getRecmMarkStep( float min, float max,
					      float& step ) const;

};


#endif
