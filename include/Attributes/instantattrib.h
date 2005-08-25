#ifndef instantattrib_h
#define instantattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          May 2005
 RCS:           $Id: instantattrib.h,v 1.3 2005-08-25 14:57:13 cvshelene Exp $
________________________________________________________________________

-*/

/*! \brief
#### Short description
\par
#### Detailed description.

*/

#include "attribprovider.h"

namespace Attrib
{

class Instantaneous : public Provider
{
public:
    static void			initClass();
    				Instantaneous(Desc&);

    static const char*		attribName()	{ return "Instantaneous"; }
    static const char*		gateStr()	{ return "gate"; }

protected:
    static Provider*		createInstance(Desc&);
    static void			updateDesc(Desc&);
    static Provider*		internalCreate(Desc&,ObjectSet<Provider>& exis);

    bool			getInputOutput(int in,TypeSet<int>& res) const;
    bool			getInputData(const BinID&, int);
    bool			computeData(const DataHolder&,const BinID& pos,
	    				    int t0,int nrsamples) const;

    const Interval<float>*	reqZMargin(int input,int output) const
				{ return &gate; }

    Interval<float>		gate;
    const DataHolder*		realdata;
    const DataHolder*		imagdata;
    int				realidx_;
    int				imagidx_;

private:
    float			calcAmplitude(int) const;
    float			calcPhase(int) const;
    float			calcFrequency(int) const;
    float			calcAmplitude1Der(int) const;
    float			calcAmplitude2Der(int) const;
    float			calcEnvWPhase(int) const;
    float			calcEnvWFreq(int) const;
    float			calcPhaseAccel(int) const;
    float			calcThinBed(int) const;
    float			calcBandWidth(int) const;
    float			calcQFactor(int) const;
    float			calcRMSAmplitude(int) const;
};

}; // namespace Attrib

#endif
