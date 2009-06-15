#ifndef welltiepickset_h
#define welltiepickset_h

/*+
________________________________________________________________________

CopyRight:     (C) dGB Beheer B.V.
Author:        Bruno
Date:          Feb 2009
RCS:           $Id: welltiepickset.h,v 1.1 2009-01-19 13:02:33 cvsbruno Exp
$
________________________________________________________________________

-*/

#include "namedobj.h"
#include "color.h"
#include "ranges.h"
#include "welld2tmodel.h"


class UserPick;
namespace Well
{
    class Data;
}

mClass UserPick
{
    public:
		    UserPick()
		    {}

    Color           color_;
    int             vidx_;
    float           dah_;
    float           xpos_;
};


mClass WellTiePickSet : public CallBacker
{
public:

			    WellTiePickSet();
                            ~WellTiePickSet();
			    
			    WellTiePickSet( const WellTiePickSet& wps )
				: mousepos_(wps.mousepos_)
				, nrpickstotal_(wps.nrpickstotal_)
	      			, pickadded(wps.pickadded)		
				{ 
				    deepCopy(pickset_,wps.pickset_);
				}

    Notifier<WellTiePickSet> pickadded;

    void                add(UserPick* pick) { pickset_ += pick; };
    void                add(int,float,float);
    void 		clear(int idx);
    void 		clearAll();
    const float         getMousePos() const    { return mousepos_; }
    UserPick*           get(int idx)  	       { return pickset_[idx]; }
    const UserPick*     get(int idx) const     { return pickset_[idx]; }
    UserPick*           getLast()  	       { return pickset_[getSize()-1]; }
    const UserPick*     getLast(int idx) const { return pickset_[getSize()-1]; }
    const int           getTotalNr() const     { return nrpickstotal_; }
    const int           getSize() const        { return pickset_.size(); }
    float 		getDah( int idx )     
    			{ return get(idx)->dah_; }
    const float 	getDah( int idx ) const 
    			{ return get(idx)->dah_; }
    const float 	getLastDah() 		 
    			{ return getLast()->dah_; }
   
    UserPick*		remove(int idx) { return pickset_.remove(idx); }
    void         	setMousePos( float mp ) { mousepos_ = mp; }
    void		setDah( int idx, float dah ) 
    			{ get(idx)->dah_= dah; }
    
    void 		updateSupPickedPos(float&,float,int);
    void 		updateInfPickedPos(float&,float,int);
    void                updateShift(int,float);

protected:

    ObjectSet<UserPick> pickset_;

    float               mousepos_;
    int                 nrpickstotal_;
};


mClass WellTiePickSetMGR : public CallBacker
{
public:

			    WellTiePickSetMGR(const Well::Data*);
                            ~WellTiePickSetMGR();

    Notifier<WellTiePickSetMGR> mousemoving;

    WellTiePickSet* getLogPickSet()   		{ return &logpickset_; }
    WellTiePickSet* getSynthPickSet() 		{ return &synthpickset_; }
    WellTiePickSet* getSeisPickSet()  		{ return &seispickset_; }

    const Well::Data* wd_;

    void           addPick(Interval<float>,float,float);
    void           addPick(int,float);
    void           clearAllPicks();
    void 	   clearLastPicks();
    bool 	   checkIfPick();
    void 	   sortByDah(WellTiePickSet&);
    void           updateShift(int,float);

protected:

    WellTiePickSet logpickset_;
    WellTiePickSet synthpickset_;
    WellTiePickSet seispickset_;
};

#endif

