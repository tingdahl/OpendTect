#ifndef vispointsetdisplay_h
#define vispointsetdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		March 2009
 RCS:		$Id: vispointsetdisplay.h,v 1.10 2010-03-03 10:11:57 cvssatyaki Exp $
________________________________________________________________________


-*/

#include "color.h"
#include "visobject.h"
#include "vissurvobj.h"
#include "vistransform.h"

class DataPointSet;

namespace visBase { class PointSet; }

namespace visSurvey
{


mClass PointSetDisplay : public visBase::VisualObjectImpl,
			 public visSurvey::SurveyObject
{
public:
    static PointSetDisplay*     create()
				mCreateDataObj(PointSetDisplay);
    				~PointSetDisplay();

    void			setPointSize(int);
    int				getPointSize() const;

    Color			getColor(int) const;
    void			setColors(const TypeSet<Color>&);
    void			setNrPointSets(int);
    bool			hasColor() const 	{ return true; }

    void			update();
    bool			setDataPack(int);
    const DataPointSet*		getDataPack() const 	{ return data_; }
    void			setDisplayTransformation(mVisTrans*);
    mVisTrans*			getDisplayTransformation();

    const char*			errMsg() const { return errmsg_.buf(); }

    void			removeSelection(const Selector<Coord3>&);
    bool			selectable()			{ return true; }
    bool			canRemoveSelecion()		{ return true; }
    bool			allowMaterialEdit() const	{ return true; }
protected:

    TypeSet<Color>			colors_;
    ObjectSet<visBase::PointSet> 	pointsets_;
    DataPointSet*		data_;
};

};


#endif
