#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		November 2008
________________________________________________________________________

-*/

#include "volumeprocessingmod.h"

#include "dbkey.h"
#include "volprocstep.h"

class IOObj;
class Scaler;
class SeisTrcTranslator;

namespace VolProc
{

/*!\brief Reads a volume. Will replace previous values if data is present in
	    the read volume. */

mExpClass(VolumeProcessing) VolumeReader : public Step
{ mODTextTranslationClass(VolumeReader);
public:
				mDefaultFactoryInstantiation(
					Step, VolumeReader,
					"VolumeReader", tr("Input Volume") )
				~VolumeReader();

    bool			setVolumeID(const DBKey&);
    const DBKey&		getVolumeID() const		{ return mid_; }

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

private:

    virtual ReportingTask*	createTask();

    virtual bool		needsFullVolume() const		{ return false;}
    virtual bool		canInputAndOutputBeSame() const	{ return true; }
    virtual bool		areSamplesIndependent() const	{ return true; }
    virtual bool		canHandle2D() const		{ return true; }
    virtual bool		needsInput() const		{ return false;}
    virtual int			getNrOutComponents(OutputSlotID,
						   Pos::GeomID) const;

    virtual od_int64		extraMemoryUsage(OutputSlotID,
						 const TrcKeyZSampling&) const
				{ return 0; }

    bool			prepareWork(const IOObj&);

    static const char*		sKeyVolumeID()		{ return "Volume ID"; }

    DBKey			mid_;
    ObjectSet<SeisTrcTranslator> translators_;
    TypeSet<int>		components_;
    ObjectSet<Scaler>		compscalers_;

};

} // namespace VolProc
