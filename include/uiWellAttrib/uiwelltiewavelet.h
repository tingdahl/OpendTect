#ifndef uiwelltiewavelet_h
#define uiwelltiewavelet_h

/*+
________________________________________________________________________

CopyRight:     (C) dGB Beheer B.V.
Author:        Bruno
Date:          January 2009
RCS:           $Id: uiwelltiewavelet.h,v 1.1 2009-01-19 13:02:33 cvsbruno Exp
$
________________________________________________________________________

-*/

#include "uidialog.h"
#include "uigroup.h"

template <class T> class Array1DImpl;
class CtxtIOObj;
class SeisTrc;
class WellTieDataHolder;
class WellTieSetup;
class Wavelet;

class uiFlatViewer;
class uiFunctionDisplay;
class uiPushButton;
class uiIOObjSel;
class uiTextEdit;

mClass uiWellTieWaveletView : public uiGroup
{
public:

	    uiWellTieWaveletView(uiParent*,const WellTieDataHolder*); 
	    ~uiWellTieWaveletView();

    void 		initWavelets();
    //Notifier<uiWellTieWaveletView>      wvltChanged;

protected:

    const WellTieDataHolder*	dataholder_;
    const WellTieSetup&		twtss_;
    CtxtIOObj&          	wvltctio_;

    ObjectSet<uiPushButton> 	wvltbuts_;
    ObjectSet<uiFlatViewer> 	viewer_;	   
    uiIOObjSel*			wvltfld_;
    uiTextEdit*			infofld_;
    ObjectSet<const Wavelet>  	wvlts_;

    void			initWaveletViewer(int);
    void 			createWaveletFields(uiGroup*);	   
    void			drawWavelet(const Wavelet*,int);
    
    void 		wvtSel(CallBacker*);
    void 		viewInitWvltPropPushed(CallBacker*);
    void 		viewEstWvltPropPushed(CallBacker*);
};



class uiWellTieWaveletDispDlg : public uiDialog
{
public:
			    uiWellTieWaveletDispDlg(uiParent*,const Wavelet*);
			    ~uiWellTieWaveletDispDlg();

protected:

    int                                 wvltsz_;
    SeisTrc*				wvlttrc_;
    CtxtIOObj&                          wvltctio_;
    ObjectSet<uiFunctionDisplay>        wvltdisps_;
    ObjectSet< TypeSet<float> >     	propvals_;
    const Wavelet*			wvlt_;

    void                                setDispCurves();
};

#endif

