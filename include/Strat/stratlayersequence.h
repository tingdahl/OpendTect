#ifndef stratlayersequence_h
#define stratlayersequence_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Oct 2010
 RCS:		$Id$
________________________________________________________________________


-*/

#include "stratmod.h"
#include "ailayer.h"
#include "stratlayer.h"
#include "propertyref.h"

class ElasticPropSelection;

namespace Strat
{
class Layer;
class Level;
class UnitRef;
class RefTree;

/*!\brief A sequence of layers.

  You can provide a PropertyRefSelection* to give meaning to the values in the Layers.

 */

mClass(Strat) LayerSequence
{
public:

			LayerSequence(const PropertyRefSelection* prs=0);
			LayerSequence( const LayerSequence& ls )
			    			{ *this = ls; }
    virtual		~LayerSequence();
    LayerSequence&	operator =(const LayerSequence&);
    bool		isEmpty() const		{ return layers_.isEmpty(); }

    int			size() const		{ return layers_.size(); }
    ObjectSet<Layer>&	layers(bool postfr=false)
			{ return postfr ? layerspostfr_ : layers_; }
    const ObjectSet<Layer>& layers(bool postfr=false) const
    			{ return postfr ? layerspostfr_ : layers_; }
    int			layerIdxAtZ(float,bool ret_size_if_after=false) const;
    			//!< return -1 if outside, unless below and par==true

    float		startDepth() const	{ return z0_; }
    void		setStartDepth( float z ) { z0_ = z; }

    PropertyRefSelection& propertyRefs() 	{ return props_; }
    const PropertyRefSelection& propertyRefs() const	{ return props_; }

    void		getLayersFor( const UnitRef* ur, ObjectSet<Layer>& lys,
	   			      bool ispostfr=false )
			{ return getLayersFor(ur,(ObjectSet<const Layer>&)lys,
					      ispostfr);}
    void		getLayersFor(const UnitRef*,
	    			     ObjectSet<const Layer>&,
				     bool ispostfr=false) const;
    const RefTree&	refTree() const;

    void		prepareUse() const ;	//!< needed after changes
    void		prepareFluidRepl();

    int			indexOf(const Level&,int startsearchat=0) const;
    			//!< may return -1 for not found (level below layers)
    float		depthOf(const Level&) const;
    			//!< will return bot of seq if lvl not found

protected:

    ObjectSet<Layer>	layers_;
    ObjectSet<Layer>	layerspostfr_;
    float		z0_;
    PropertyRefSelection props_;

};


}; // namespace Strat

#endif

