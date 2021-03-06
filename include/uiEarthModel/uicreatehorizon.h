#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"

#include "uidialog.h"
#include "dbkey.h"

class uiGenInput;
class uiSurfaceWrite;


/*!\brief Dialog to create a horizon with a constant Z */

mExpClass(uiEarthModel) uiCreateHorizon : public uiDialog
{ mODTextTranslationClass(uiCreateHorizon)
public:
			uiCreateHorizon(uiParent*,bool is2d);
			~uiCreateHorizon();

    DBKey		getSelID() const;

    Notifier<uiCreateHorizon> ready;

protected:

    bool		acceptOK();

    uiGenInput*		zfld_;
    uiSurfaceWrite*	outfld_;
};
