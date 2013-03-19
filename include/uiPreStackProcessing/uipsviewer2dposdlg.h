#ifndef uipsviewer2dposdlg_h
#define uipsviewer2dposdlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Jan 2011
 RCS:           $Id$
________________________________________________________________________

-*/

#include "multiid.h"
#include "bufstringset.h"

#include "uiprestackprocessingmod.h"
#include "uislicesel.h"

class uiCheckBox;
class uiListBox;
class uiTable;
class uiToolButton;

namespace PreStackView
{


mStruct(uiPreStackProcessing) GatherInfo
{
    			GatherInfo()
			: isstored_(true), isselected_( false ), dpid_(-1)
			, bid_(mUdf(int),mUdf(int))	{}
    bool		isstored_;
    bool		isselected_;
    MultiID		mid_;
    int			dpid_;
    BufferString	gathernm_;
    BinID		bid_;
bool operator==( const GatherInfo& info ) const
{
    return isstored_==info.isstored_ && bid_==info.bid_ &&
	   (isstored_ ? mid_==info.mid_
	   	      : (gathernm_==info.gathernm_ && dpid_==info.dpid_));
}

};



mExpClass(uiPreStackProcessing) uiGatherPosSliceSel : public uiSliceSel
{
public:
				uiGatherPosSliceSel(uiParent*,uiSliceSel::Type,
						    const BufferStringSet&,
						    bool issynthetic=false);

    const CubeSampling&		cubeSampling(); 
    void 			setStep(int);	
    int 			step() const;	

    void			enableZDisplay(bool);
    void			updatePosTable();
    void			getSelGatherInfos(TypeSet<GatherInfo>&);
    void			setSelGatherInfos(const TypeSet<GatherInfo>&);

protected:
    uiLabeledSpinBox* 		stepfld_;
    uiTable*			posseltbl_;
    BufferStringSet		gathernms_;
    bool			issynthetic_;
    CubeSampling		fullcs_;

    void			posChged(CallBacker*);
    void			applyPushed(CallBacker*);
};

mExpClass(uiPreStackProcessing) uiViewer2DPosDlg : public uiDialog
{
public:
				uiViewer2DPosDlg(uiParent*,bool is2d,
						const CubeSampling&,
						const BufferStringSet&,
						bool issynthetic);

    void 			setCubeSampling(const CubeSampling&);
    void 			getCubeSampling(CubeSampling&);

    void                        enableZDisplay(bool yn)
				    { sliceselfld_->enableZDisplay(yn); }
    void			getSelGatherInfos(TypeSet<GatherInfo>& infos)
				{ sliceselfld_->getSelGatherInfos(infos); }
    void			setSelGatherInfos(const TypeSet<GatherInfo>& gi)
				{ sliceselfld_->setSelGatherInfos(gi); }

    Notifier<uiViewer2DPosDlg> okpushed_;

protected:

    uiGatherPosSliceSel*	sliceselfld_;
    bool			is2d_;
    bool			acceptOK(CallBacker*);
};


mExpClass(uiPreStackProcessing) uiViewer2DSelDataDlg : public uiDialog
{
public: 	
			    uiViewer2DSelDataDlg(uiParent*,
				    const BufferStringSet&,BufferStringSet&);
protected:

    uiListBox*			allgatherfld_;
    uiListBox*			selgatherfld_;
    uiToolButton*		toselect_;	
    uiToolButton*		fromselect_;

    BufferStringSet&		selgathers_;	

    void 			selButPush(CallBacker*);
    bool 			acceptOK(CallBacker*);
};

}; //namespace

#endif

