/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uidpsdemo.cc,v 1.5 2009-11-04 15:29:59 cvsbert Exp $";

#include "uidpsdemo.h"

#include "datapointset.h"
#include "binidvalset.h"
#include "posvecdataset.h"
#include "datacoldef.h"
#include "statrand.h"
#include "emsurfacetr.h"
#include "emmanager.h"
#include "emhorizon3d.h"
#include "binidsurface.h"
#include "seistrc.h"
#include "seisbuf.h"
#include "seisread.h"
#include "seistrctr.h"
#include "seistrcprop.h"
#include "seisselectionimpl.h"

#include "uiseissel.h"
#include "uigeninput.h"
#include "uitaskrunner.h"
#include "uidatapointset.h"
#include "uimsg.h"


uiDPSDemo::uiDPSDemo( uiParent* p )
	: uiDialog(p,Setup("DataPointSet demo","Data extraction parameters",
		    	   mNoHelpID))
{
    Stats::RandGen::init();

    horfld_ = new uiIOObjSel( this, mIOObjContext(EMHorizon3D) );

    IOObjContext ctxt( mIOObjContext(SeisTrc) );
    seisfld_ = new uiSeisSel( this, ctxt, uiSeisSel::Setup(false,false) );
    seisfld_->attach( alignedBelow, horfld_ );

    nrptsfld_ = new uiGenInput( this, "Number of points to extract",
	    			IntInpSpec(1000) );
    nrptsfld_->attach( alignedBelow, seisfld_ );
}


uiDPSDemo::~uiDPSDemo()
{
}


#define mErrRet(s) { uiMSG().error( s ); return false; }

bool uiDPSDemo::acceptOK( CallBacker* )
{
    const IOObj* horioobj = horfld_->ioobj(); // emits its own error message
    if ( !horioobj ) return false;
    const IOObj* seisioobj = seisfld_->ioobj(); // this one, too
    if ( !seisioobj ) return false;

    const int nrpts = nrptsfld_->getIntValue();
    if ( nrpts < 2 )
	mErrRet( "Please enter a valid number of points" )

    return doWork( *horioobj, *seisioobj, nrpts );
}


// Needed to delete the DataPointSet once uiDataPointSet is finished
class uiDPSDemoDPSDeleter : public CallBacker
{
public:

void doDel( CallBacker* cb )
{
    mDynamicCastGet(uiDataPointSet*,dlg,cb)
    if ( dlg )
	delete &dlg->pointSet();
}

};



bool uiDPSDemo::doWork( const IOObj& horioobj, const IOObj& seisioobj,
			int nrpts )
{
    uiTaskRunner* tr = new uiTaskRunner( this );
    EM::EMObject* emobj = EM::EMM().loadIfNotFullyLoaded( horioobj.key(), tr );
    mDynamicCastGet(EM::Horizon3D*,hor,emobj)
    if ( !hor ) return false;

    DataPointSet* dps = new DataPointSet( false );
    dps->dataSet().add( new DataColDef("Amplitude") );
    dps->dataSet().add( new DataColDef("Frequency") );

    hor->ref();
    const bool isok = getRandPositions(*hor,nrpts,*dps);
    hor->unRef();
    if ( !isok || !getSeisData(seisioobj,*dps,*tr) )
	return false;

    BufferString wintitl( horioobj.name(), " / ", seisioobj.name() );
    uiDataPointSet::Setup su( wintitl );
    uiDataPointSet* dlg = new uiDataPointSet( parent(), *dps, su );
    dlg->setDeleteOnClose( true );

    static uiDPSDemoDPSDeleter dpsdel;
    dlg->windowClosed.notify( mCB(&dpsdel,uiDPSDemoDPSDeleter,doDel) );

    dlg->show();
    return true;
}


#define mSectGeom(sect) (*hor.geometry().sectionGeometry(sect))

bool uiDPSDemo::getRandPositions( const EM::Horizon3D& hor, int nrpts,
       				 DataPointSet& dps )
{
    TypeSet<int> nrsectnodes;
    int totnrnodes = 0;
    for ( EM::SectionID isect=0; isect<hor.nrSections(); isect++ )
    {
	const int nrnodes = mSectGeom(isect).nrKnots();
	nrsectnodes += nrnodes;
	totnrnodes += nrnodes;
    }
    if ( totnrnodes < 1 )
	mErrRet( "Horizon is empty" )

    bool needrandsel = nrpts < totnrnodes;
    const int actualnrpts = needrandsel ? nrpts : totnrnodes;
    int nrunsuccessful = 0;
#   define mNextTry() { ipt--; nrunsuccessful++; continue; }
    for ( int ipt=0; ipt<actualnrpts; ipt++ )
    {
	if ( nrunsuccessful > 1000000 )
	    break;

	// Get a random position in horizon
	int selidx = needrandsel ? Stats::RandGen::getIndex( totnrnodes )
	    			       : ipt;
	BinID bid; EM::SectionID selsect = 0;
	for ( EM::SectionID isect=0; isect<nrsectnodes.size(); isect++ )
	{
	    if ( nrsectnodes[isect] < selidx )
		selidx -= nrsectnodes[isect];
	    else
		{ selsect = isect; break; }
	}
	bid = BinID( mSectGeom(selsect).getKnotRowCol(selidx) );

	// Checking whether position is already in set. Here, we have to use
	// the BinIDValueSet, because we don't want to call
	// DataPointSet::dataChanged() after every add().
	if ( needrandsel && dps.bivSet().valid(bid) )
	    mNextTry()

	const float z = mSectGeom(selsect).getKnot(bid,true).z;
	if ( mIsUdf(z) )
	    mNextTry()

	// Add the position to set.
	// We store section+1 because DataPointSet's groups start at 1
	DataPointSet::Pos dpspos( bid, mSectGeom(selsect).getKnot(bid,true).z );
	DataPointSet::DataRow dr( dpspos, selsect+1 );
	dps.addRow( dr );
    }

    // This builds an index table in the DataPointSet.
    dps.dataChanged();

    return true;
}


bool uiDPSDemo::getSeisData( const IOObj& ioobj, DataPointSet& dps,
			     TaskRunner& tr )
{
    SeisTrcReader rdr( &ioobj );
    Seis::TableSelData* tsd = new Seis::TableSelData( dps.bivSet() );
    rdr.setSelData( tsd );
    if ( !rdr.prepareWork() )
	mErrRet(rdr.errMsg())

    SeisTrcBuf tbuf(true);
    SeisBufReader br( rdr, tbuf );
    if ( !tr.execute(br) )
	return false;

    const int icomp = 0;
    for ( int idx=0; idx<tbuf.size(); idx++ )
    {
	const SeisTrc& trc = *tbuf.get( idx );
	DataPointSet::RowID rid = dps.findFirst( trc.info().binid );
	if ( rid < 0 )
	    { pErrMsg("Huh?"); continue; }

	const float z = dps.z( rid );
	if ( mIsUdf(z) ) continue; // Hmm. Node with undef Z ...

	float* vals = dps.getValues( rid );
	vals[0] = trc.getValue( z, icomp );
	vals[1] = SeisTrcPropCalc(trc,icomp).getFreq( trc.nearestSample(z) );
    }

    return true;
}
