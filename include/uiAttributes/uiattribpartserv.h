#ifndef uiattribpartserv_h
#define uiattribpartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Feb 2002
 RCS:           $Id: uiattribpartserv.h,v 1.17 2006-08-24 14:50:58 cvskris Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"
#include "attribdescid.h"
#include "position.h"
#include "multiid.h"
#include "menuhandler.h"
#include "timer.h"

namespace Attrib
{
    class DataCubes;
    class Data2DHolder;
    class Desc;
    class DescSet;
    class DescSetMan;
    class EngineMan;
    class SelSpec;
};

class BinID;
class BinIDValueSet;
class BufferStringSet;
class CubeSampling;
class Executor;
class IOPar;
class NLACreationDesc;
class NLAModel;
class PosVecDataSet;
class SeisTrcBuf;
class SeisTrcInfo;
class uiAttribDescSetEd;
namespace Pick { class Set; }
template <class T> class Interval;
template <class T> class Array3D;


/*! \brief Service provider for application level - Attributes */

class uiAttribPartServer : public uiApplPartServer
{
public:
			uiAttribPartServer(uiApplService&);
			~uiAttribPartServer();

    const char*		name() const			{ return "Attributes"; }

    static const int	evDirectShowAttr;
    			//!< User requested direct redisplay of curAttrDesc()
    static const int	evNewAttrSet;
    			//!< FYI
    static const int	evAttrSetDlgClosed;
    			//!< AttributeSet window closes
    static const int	evEvalAttrInit;
    			//!< Initialization of evaluation dialog
    static const int	evEvalCalcAttr;
    			//!< User wants to evaluate current attribute
    static const int	evEvalShowSlice;
    			//!< Display slice
    static const int	evEvalStoreSlices;
    			//!< Store slices
    static const int	objNLAModel;
    			//!< Request current NLAModel* via getObject()

    void		manageAttribSets();
    const Attrib::DescSet* curDescSet() const;
    void		getDirectShowAttrSpec(Attrib::SelSpec&) const;
    bool		setSaved() const;
    void		saveSet();
    bool		editSet();
    			//!< returns whether new AttribDescSet has been created
    bool		attrSetEditorActive() const	{ return attrsetdlg; }
    void		updateSelSpec(Attrib::SelSpec&) const;

    bool		selectAttrib(Attrib::SelSpec&,const char* depthdomkey);
    bool		setPickSetDirs(Pick::Set&,const NLAModel*);
    void		outputVol(MultiID&);
    bool		replaceSet(const IOPar&);
    bool		addToDescSet(const char*);
    int			getSliceIdx() const		{ return sliceidx; }
    void		getPossibleOutputs(BufferStringSet&) const;

    void		setTargetSelSpec(const Attrib::SelSpec&);
    const Attrib::DataCubes* createOutput(const CubeSampling&,
				          const Attrib::DataCubes* prevslcs=0);
    bool		createOutput(ObjectSet<BinIDValueSet>&);
    bool		createOutput( const BinIDValueSet& bidvalset,
	    			      SeisTrcBuf&);

    bool		isDataAngles() const;
    			/*!<\returns true if the target data is an
				     angle, i.e. -PI==PI. */

    bool		isDataClassified(const Array3D<float>&) const;

    Attrib::DescID	createStored2DAttrib(const MultiID& lineset,
	    				     const char* attribname);
    
    bool		create2DOutput(const CubeSampling&, const char* linekey,
	    			       Attrib::Data2DHolder& );

    bool		extractData(const NLACreationDesc&,
				    const ObjectSet<BinIDValueSet>&,
				    ObjectSet<PosVecDataSet>&);
    bool		createAttributeSet(const BufferStringSet&,
	    				   Attrib::DescSet*);

    const NLAModel*	getNLAModel() const;
    void		setNLAName( const char* nm )	{ nlaname = nm; }

    MenuItem*         	storedAttribMenuItem(const Attrib::SelSpec&);
    MenuItem*         	calcAttribMenuItem(const Attrib::SelSpec&);
    MenuItem*         	nlaAttribMenuItem(const Attrib::SelSpec&);
    MenuItem*         	depthdomainAttribMenuItem(const Attrib::SelSpec&,
	    					  const char* key);

    bool		handleAttribSubMenu(int mnuid,Attrib::SelSpec&) const;

    void		setEvaluateInfo(bool ae,bool as)
			{ alloweval=ae; allowevalstor=as; }

    void		fillPar( IOPar& ) const;
    void		usePar( const IOPar& );

protected:

    MenuItem            storedmnuitem;
    MenuItem            calcmnuitem;
    MenuItem            nlamnuitem;
    MenuItem            depthdomainmnuitem;

    Attrib::DescSetMan*	adsman;
    const Attrib::Desc*	dirshwattrdesc;
    uiAttribDescSetEd*	attrsetdlg;
    Timer		attrsetclosetim;

    Attrib::EngineMan*	createEngMan(const CubeSampling* cs=0,const char* lk=0);

    void		directShowAttr(CallBacker*);

    void		showEvalDlg(CallBacker*);
    void		calcEvalAttrs(CallBacker*);
    void		showSliceCB(CallBacker*);
    void		evalDlgClosed(CallBacker*);

    void		attrsetDlgClosed(CallBacker*);
    void		attrsetDlgCloseTimTick(CallBacker*);

    static const char*	attridstr;
    BufferString	nlaname;

    bool		alloweval;
    bool		allowevalstor;
    int			sliceidx;
    Attrib::DescSet*	evalset;
    TypeSet<Attrib::SelSpec> targetspecs;
};


/*!\mainpage Attribute User Interface

 This part server's main task is handling the attribute set editor. Other
 services are selection and volume output.

 The Attribute set editor is a pretty complex piece of user interface. The left
 and top part of the window are fixed. They handle the 'common' things in
 attribute set editing. The right part is defined via the uiAttribFactory .

 The proble that was facing us was that we needed a user interface that could
 be dynamically extended. Further more,much of the needed functionality is
 common to all attributes. Thus, we defined:

 - A base class for all attribute editors (uiAttrDescEd)
 - A factory of uiAttrDescEdCreater, creating uiAttrDescEd instances from
   the name of the attribute

 The uiAttrDescEd itself already has a lot of implemented functionality,
 leaving only things specific for that particular attribute to be defined.
 Once such a subclass is fully defined, a uiAttrDescEdCreater instance must be
 added to the factory to make it active.

 To see how such a new attribute can be created aswell as a user interface for
 it, take a look at the Coherency example in the programmer documentation on
 plugins.

*/


#endif
