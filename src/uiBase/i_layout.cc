/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          18/08/1999
 RCS:           $Id: i_layout.cc,v 1.60 2003-02-26 13:02:26 arend Exp $
________________________________________________________________________

-*/

#include "uilayout.h"
#include "errh.h"

#include "i_layoutiter.h"
#include "i_layout.h"
#include "uiobjbody.h"
#include "uimainwin.h"

//#include <qmenubar.h>

#include <stdio.h>
//#include <iostream>
//#include <limits.h>

#ifdef __debug__
#define MAX_ITER	2000
#else
#define MAX_ITER	10000
#endif


#ifdef __extensive_debug__
static BufferString jaap;
bool arend_debug=false;
#endif


i_LayoutMngr::i_LayoutMngr( QWidget* parnt, 
			    const char *name, uiObjectBody& mngbdy )
    : QLayout(parnt,0,0,name), UserIDObject(name)
    , minimumDone(false), preferredDone(false), ismain(false)
    , prefposStorCnt( 2 )
    , managedBody(mngbdy), hspacing(-1), vspacing(8), borderspc(0)
{}


i_LayoutMngr::~i_LayoutMngr()
{
}


void i_LayoutMngr::addItem( i_LayoutItem* itm )
{
    if ( !itm ) return;

    itm->deleteNotify( mCB(this,i_LayoutMngr,itemDel) );

    childrenList.append( itm );
}


/*! \brief Adds a QlayoutItem to the manager's children

    Should normally not been called, since all ui items are added to the
    parent's manager using i_LayoutMngr::addItem( i_LayoutItem* itm )

*/
void i_LayoutMngr::addItem( QLayoutItem *qItem )
{
    if ( !qItem ) return;
    addItem( new i_LayoutItem( *this, *qItem) );
}


void i_LayoutMngr::itemDel( CallBacker* cb )
{
    if ( !cb ) return;

    i_LayoutItem* itm = dynamic_cast<i_LayoutItem*>( cb );

    if ( !childrenList.removeRef( itm ) )
	pErrMsg("Removal of layoutitem failed.");
}


QSize i_LayoutMngr::minimumSize() const
{
    if ( !minimumDone ) 
    { 
	doLayout( minimum, QRect() ); 
	const_cast<i_LayoutMngr*>(this)->minimumDone=true; 
    }

    uiRect mPos;

    if ( ismain )
    {
	if ( managedBody.shrinkAllowed() )	
	    return QSize(0, 0);

	QSize sh = sizeHint();

	int hsz = sh.width();
	int vsz = sh.height();

	if ( hsz <= 0 || hsz > 4096 || vsz <= 0 || vsz > 4096 )
	{
	    mPos = curpos(minimum);
	    hsz = mPos.hNrPics();
	    vsz = mPos.vNrPics();
	}

#ifdef __extensive_debug__
	if ( arend_debug )
	{
	    BufferString msg;
	    msg="Returning Minimum Size for ";
	    msg += UserIDObject::name();
	    msg += ". (h,v)=(";
	    msg += hsz;
	    msg +=" , ";
	    msg += vsz;
	    msg += ").";

	    cout << msg << endl;
	}
#endif
	return QSize( hsz, vsz );
    }

    mPos = curpos(minimum);
    return QSize( mPos.hNrPics(), mPos.vNrPics() );
}


QSize i_LayoutMngr::sizeHint() const
{
    if ( !preferredDone )
    { 
	doLayout( preferred, QRect() ); 
	const_cast<i_LayoutMngr*>(this)->preferredDone=true; 
    }
    uiRect mPos = curpos(preferred);

#ifdef __debug__

    if ( mPos.hNrPics() > 4096 || mPos.vNrPics() > 4096 )
    {
	BufferString msg;
	msg="Very large preferred size for ";
	msg += UserIDObject::name();
	msg += ". (h,v)=(";
	msg += mPos.hNrPics();
	msg +=" , ";
	msg += mPos.vNrPics();
	msg += ").";

	msg += " (t,l),(r,b)=(";
	msg += mPos.top();
	msg +=" , ";
	msg += mPos.left();
	msg += "),(";
	msg += mPos.right();
	msg +=" , ";
	msg += mPos.bottom();
	msg += ").";

	pErrMsg(msg);
    }

#endif

    return QSize( mPos.hNrPics(), mPos.vNrPics() );
}


QSizePolicy::ExpandData i_LayoutMngr::expanding() const
{
    return QSizePolicy::BothDirections;
}


const uiRect& i_LayoutMngr::curpos(layoutMode lom) const 
{ 
    i_LayoutItem* managedItem = 
	    const_cast<i_LayoutItem*>(managedBody.layoutItem());

    return managedItem ? managedItem->curpos(lom) : layoutpos[lom]; 
}


uiRect& i_LayoutMngr::curpos(layoutMode lom)
{ 
    i_LayoutItem* managedItem = 
	    const_cast<i_LayoutItem*>(managedBody.layoutItem());

    return managedItem ? managedItem->curpos(lom) : layoutpos[lom]; 
}


uiRect i_LayoutMngr::winpos( layoutMode lom ) const 
{
    i_LayoutItem* managedItem = 
	    const_cast<i_LayoutItem*>(managedBody.layoutItem());

    if ( ismain && !managedItem )
    {
	int hborder = layoutpos[lom].left();
	int vborder = layoutpos[lom].top();
	return uiRect( 0, 0, layoutpos[lom].right()+2*hborder,
			     layoutpos[lom].bottom()+2*vborder );
    }

    if ( ismain ) { return managedItem->curpos(lom); }

    return managedItem->mngr().winpos(lom);
}


QLayoutIterator i_LayoutMngr::iterator()
{
    return QLayoutIterator( new i_LayoutIterator( &childrenList ) ) ;
}


//! \internal class used when resizing a window
class resizeItem
{
#define NR_RES_IT	3
public:
			resizeItem( i_LayoutItem* it, int hStre, int vStre ) 
			    : item( it ), hStr( hStre ), vStr( vStre )
			    , hDelta( 0 ), vDelta( 0 ) 
			    , nhiter( hStre ? NR_RES_IT : 0 )
			    , nviter( vStre ? NR_RES_IT : 0 ) {}

    i_LayoutItem* 	item;
    const int 		hStr;
    const int 		vStr;
    int			nhiter;
    int			nviter;
    int			hDelta;
    int			vDelta;

};


void i_LayoutMngr::childrenClear( uiObject* cb )
{
    for ( QPtrListIterator<i_LayoutItem> childIter( childrenList );
		    i_LayoutItem* curChld = childIter.current(); ++childIter )
    {
	uiObject* cl = curChld->objLayouted();
	if ( cl && cl != cb ) cl->clear();
    }
}


bool i_LayoutMngr::isChild( uiObject* obj )
{
    for ( QPtrListIterator<i_LayoutItem> childIter( childrenList );
		    i_LayoutItem* curChld = childIter.current(); ++childIter )
    {
	uiObject* cl = curChld->objLayouted();
	if ( cl && cl == obj ) return true;
    }
    return false;
}

int i_LayoutMngr::childStretch( bool hor ) const
{

    int sum=0;
    for ( QPtrListIterator<i_LayoutItem> childIter( childrenList );
		    i_LayoutItem* curChld = childIter.current(); ++childIter )
    {
	uiObjectBody* ccbl = curChld->bodyLayouted();

	if ( ccbl )
	{
	    int cs = ccbl->stretch( hor );
	    if ( cs < 0 || cs > 2 ) { cs = 0; }
	    sum = mMAX( sum, cs );
	}
    }

    return sum;
}

int i_LayoutMngr::horSpacing() const
{
    return hspacing >= 0 ? hspacing :  managedBody.fontWdt();
}


void i_LayoutMngr::forceChildrenRedraw( uiObjectBody* cb, bool deep )
{
    for ( QPtrListIterator<i_LayoutItem> childIter( childrenList );
		    i_LayoutItem* curChld = childIter.current(); ++childIter )
    {
	uiObjectBody* cl = curChld->bodyLayouted();
	if ( cl && cl != cb ) cl->reDraw( deep );
    }

}

void i_LayoutMngr::fillResizeList( ObjectSet<resizeItem>& resizeList, 
				   bool isPrefSz )
{
    for ( QPtrListIterator<i_LayoutItem> childIter( childrenList );
		    i_LayoutItem* curChld = childIter.current(); ++childIter )
    {
	int hs = curChld->stretch(true);
	int vs = curChld->stretch(false);

	if ( hs || vs )
        {
	    bool add=false;

	    if ( (hs>1) || (hs==1 && !isPrefSz) )	add = true;
	    else					hs=0;

	    if ( (vs>1) || (vs==1 && !isPrefSz) )	add = true;
	    else					vs=0;

	    if ( add ) resizeList += new resizeItem( curChld, hs, vs );
        }
    } 
}


void i_LayoutMngr::moveChildrenTo(int rTop, int rLeft, layoutMode lom )
{
    for ( QPtrListIterator<i_LayoutItem> childIter( childrenList );
		    i_LayoutItem* curChld = childIter.current(); ++childIter )
    {
	uiRect& chldGeomtry = curChld->curpos(lom);
	chldGeomtry.topTo ( rTop );
	chldGeomtry.leftTo ( rLeft );
    }
}

bool i_LayoutMngr::tryToGrowItem( resizeItem& itm, 
				  const int maxhdelt, const int maxvdelt,
				  int hdir, int vdir,
				  const QRect& targetRect, int iternr )
{
    layoutChildren( setGeom );
    uiRect childrenBBox = childrenRect(setGeom);

    uiRect& itmGeometry   = itm.item->curpos( setGeom );
    const uiRect& refGeom = itm.item->curpos( preferred );

    int oldrgt = itmGeometry.right();
    int oldbtm = itmGeometry.bottom();

    if ( hdir<0 && itm.hStr>1 && oldrgt <= targetRect.right() )  hdir = 1;
    if ( vdir<0 && itm.vStr>1 && oldbtm <= targetRect.bottom() ) vdir = 1;

    bool hdone = false;
    bool vdone = false;

    if ( hdir && itm.nhiter>0
        && ( (itm.hStr>1) 
	    || ((itm.hStr==1) && (abs(itm.hDelta+hdir) < abs(maxhdelt)))
	   )
        &&!( (hdir>0) && 
	     ( itmGeometry.right() + hdir > targetRect.right() )
	   )
        &&!( (hdir<0) && 
             ( itmGeometry.right() <= targetRect.right() )
	   )
      ) 
    { 
        hdone = true;
        itm.hDelta += hdir;
        itmGeometry.setHNrPics ( refGeom.hNrPics() + itm.hDelta );
	if ( hdir>0 &&  itmGeometry.right() > targetRect.right() )
	    itmGeometry.leftTo( itmGeometry.left() - 1 );
    }   
    
    if ( vdir && itm.nviter>0 
        && ( (itm.vStr>1) 
            || ((itm.vStr==1) && (abs(itm.vDelta+vdir) < abs(maxvdelt)))
	   )
        &&!( (vdir>0) && 
	       ( itmGeometry.bottom() + vdir > targetRect.bottom() ) )
        &&!( (vdir<0) && 
              ( itmGeometry.bottom() <= targetRect.bottom()) )
      )
    {   
        vdone = true; 
        itm.vDelta += vdir;
        itmGeometry.setVNrPics( refGeom.vNrPics() + itm.vDelta );
    }


    if ( !hdone && !vdone )
	return false;

#ifdef __debug__
    if ( iternr == NR_RES_IT )
    {
	BufferString msg;
	if ( itm.item && itm.item->objLayouted() )
	{   
	    msg="Trying to grow item ";
	    msg+= itm.item->objLayouted() ? 
		    (const char*)itm.item->objLayouted()->name() : "UNKNOWN ";

	    if ( hdone ) msg+=" hdone. ";
	    if ( vdone ) msg+=" vdone. ";
	    if ( itm.nviter ) { msg+=" viter: "; msg += itm.nviter; }
	    if ( itm.nhiter ) { msg+=" hiter: "; msg += itm.nhiter; }

	}
	else msg = "not a uiLayout item..";
	pErrMsg( msg ); 
    }
#endif

    int oldcbbrgt = childrenBBox.right();
    int oldcbbbtm = childrenBBox.bottom();

    layoutChildren( setGeom );
    childrenBBox = childrenRect(setGeom);  

    bool do_layout = false;

    if ( hdone )
    {
	bool revert = false;

	if ( hdir > 0 )
	{
	    revert |= itmGeometry.right() > targetRect.right();

	    bool tmp = childrenBBox.right() > targetRect.right();
	    tmp &= childrenBBox.right() > oldcbbrgt;

	    revert |= tmp;
	}

	if( hdir < 0 )
	{
	    bool tmp =  childrenBBox.right() <= oldcbbrgt ;
/*
	    bool tmp2 = itmGeometry.right() > targetRect.right();
		tmp2 &= itmGeometry.right() < oldrgt;
*/
	    revert = !tmp;
	}

	if( revert )
	{ 
	    itm.nhiter--;
	    itm.hDelta -= hdir;

	    itmGeometry.setHNrPics( refGeom.hNrPics() + itm.hDelta );
	    do_layout = true;
	}
    }

    if ( vdone )
    {
	if ( ((vdir >0)&&
             ( 
		( itmGeometry.bottom() > targetRect.bottom() )
	      ||(  ( childrenBBox.bottom() > targetRect.bottom() ) 
		 &&( childrenBBox.bottom() > oldcbbbtm )
	        )
	     )
	    ) 
	    || ( (vdir <0) && 
                !(  ( childrenBBox.bottom() < oldcbbbtm ) 
		  ||(  ( itmGeometry.bottom() > targetRect.bottom())
		     &&( itmGeometry.bottom() < oldbtm)
		    )
                 )
               )
	  )
	{   
	    itm.nviter--;
	    itm.vDelta -= vdir;

	    itmGeometry.setVNrPics( refGeom.vNrPics() + itm.vDelta );
	    do_layout = true;
	}
    }

    if ( do_layout ) 
    {   // move all items to top-left corner first 
	moveChildrenTo( targetRect.top(), targetRect.left(),setGeom);
	layoutChildren( setGeom );
    } 

    return true;
}


void i_LayoutMngr::resizeTo( const QRect& targetRect )
{
    doLayout( setGeom, targetRect );//init to prefer'd size and initial layout

    const int hgrow = targetRect.width()  - curpos(preferred).hNrPics();
    const int vgrow = targetRect.height() - curpos(preferred).vNrPics();

    const int hdir = ( hgrow >= 0 ) ? 1 : -1;
    const int vdir = ( vgrow >= 0 ) ? 1 : -1;

    bool isprefsz = !hgrow && !vgrow;
    if ( managedBody.uiObjHandle().mainwin()  )
    {
	bool popped = managedBody.uiObjHandle().mainwin()->poppedUp();
	isprefsz |= !popped;
    }

#ifdef __extensive_debug__
    cout << "(Re)sizing:" << UserIDObject::name();
    if ( isprefsz ) cout << " yes"; else 
	{ cout << " no " << hgrow << " ," << vgrow; }
    cout << endl;
#endif

    ObjectSet<resizeItem> resizeList;
    fillResizeList( resizeList, isprefsz );

    int iternr = MAX_ITER;

    for( bool go_on = true; go_on && iternr; iternr--)
    {   
	go_on = false;
	for( int idx=0; idx<resizeList.size(); idx++ )
	{
	    resizeItem* cur = resizeList[idx];
	    if ( cur && (cur->nhiter || cur->nviter)) 
	    { 
		if ( tryToGrowItem( *cur, hgrow, vgrow, 
				    hdir, vdir, targetRect, iternr ))
		    go_on = true; 
	    }
	}
    }

    deepErase( resizeList );

    static int printsleft=10;
    if ( !iternr && (printsleft--)>0 )
	{ pErrMsg("Stopped resize. Too many iterations "); }
}

void i_LayoutMngr::setGeometry( const QRect &extRect )
{
#ifdef __extensive_debug__
    if( arend_debug )
    {
	cout << "setGeometry called on: ";
	cout << UserIDObject::name() << endl;

	cout << "l: " << extRect.left() << " t: " << extRect.top();
	cout << " hor: " << extRect.width();
	cout << " ver: " << extRect.height() << endl;

    }
#endif

    resizeTo( extRect );
    layoutChildren( setGeom, true ); // move stuff that's attached to border

    bool popped = managedBody.uiObjHandle().mainwin()  ?
		  managedBody.uiObjHandle().mainwin()->poppedUp() : true;

    bool store2prefpos = false;
    if( prefposStorCnt > 0 || ismain && !popped )
    {
	uiRect mPos = curpos( preferred );

//	store2prefpos = ( extRect.width() == mPos.hNrPics() 
//			   && extRect.height() == mPos.vNrPics() );

	int hdif = abs( extRect.width() - mPos.hNrPics() );
	int vdif = abs( extRect.height() - mPos.vNrPics() );

	if ( hdif < 0 || vdif < 0 ) { cout << "huh!" << endl; }

	store2prefpos = prefposStorCnt > 0 || hdif < 10 && vdif < 10;

#ifdef __debug__
	if( !store2prefpos )
	{
	    cout << "setGeometry called with wrong size on: ";
	    cout << UserIDObject::name() << endl;
	    cout << "Width should be " << mPos.hNrPics();
	    cout << ", is " << extRect.width();
	    cout << ". Height should be " << mPos.vNrPics();
	    cout << ", is " << extRect.height();
	    cout << endl;
	}
#endif
    }

    childrenCommitGeometrySet( store2prefpos );
    if( store2prefpos )
    {
	prefGeometry = extRect;
	if ( prefposStorCnt > 0 ) prefposStorCnt--;
    }

    QLayout::setGeometry( extRect );
}

void i_LayoutMngr::childrenCommitGeometrySet( bool store2prefpos )
{
    for ( QPtrListIterator<i_LayoutItem> childIter( childrenList );
		    i_LayoutItem* curChld = childIter.current(); ++childIter )
    {
	curChld->commitGeometrySet( store2prefpos );
    }
}

void i_LayoutMngr::doLayout( layoutMode lom, const QRect &externalRect )
{

    bool geomSetExt = ( externalRect.width() && externalRect.height() );
    if ( geomSetExt )
    {
	curpos(lom) = uiRect( externalRect.left(), externalRect.top(), 
			    externalRect.right(), externalRect.bottom() );
    }

    int mngrTop  = geomSetExt ? externalRect.top() + borderSpace() 
			      : borderSpace();
    int mngrLeft = geomSetExt ? externalRect.left() + borderSpace()
			      : borderSpace();

    for ( QPtrListIterator<i_LayoutItem> childIter( childrenList );
		    i_LayoutItem* curChld = childIter.current(); ++childIter )
    { 
	curChld->initLayout( lom, mngrTop, mngrLeft ); 
    }

    layoutChildren(lom);

    if ( !geomSetExt )
    {
	curpos(lom) = childrenRect(lom);
    }

}

void i_LayoutMngr::layoutChildren( layoutMode lom, bool finalLoop )
{
    if ( managedBody.uiObjHandle().mainwin()  )
	managedBody.uiObjHandle().mainwin()->touch();

    int iternr;

    for ( iternr = 0 ; iternr <= MAX_ITER ; iternr++ ) 
    {
        bool child_updated = false;

	for ( QPtrListIterator<i_LayoutItem> childIter( childrenList );
		    i_LayoutItem* curChld = childIter.current(); ++childIter )
	{ 
	    child_updated |= curChld->layout( lom, iternr, finalLoop ); 
	}

	if ( !child_updated )		break;
	if ( finalLoop && iternr > 1 )	break;
    }

    if ( iternr == MAX_ITER ) 
      { pErrMsg("Stopped layout. Too many iterations "); }
}

//! returns rectangle wrapping around all children.
uiRect i_LayoutMngr::childrenRect( layoutMode lom )
{
    uiRect chldRect(-1,-1,-1,-1);

    for ( QPtrListIterator<i_LayoutItem> childIter( childrenList );
		    i_LayoutItem* curChld = childIter.current(); ++childIter )
    {
	const uiRect* childPos = &curChld->curpos(lom);


	if ( (childPos->top() ) < chldRect.top() || chldRect.top() < 0 ) 
			chldRect.setTop( childPos->top() );
	if ( (childPos->left()) < chldRect.left() || chldRect.left() < 0 ) 
			chldRect.setLeft( childPos->left() );
	if ( childPos->right() > chldRect.right() || chldRect.right() < 0)
				    chldRect.setRight( childPos->right() );
	if ( childPos->bottom()> chldRect.bottom() || chldRect.bottom()< 0)
				    chldRect.setBottom( childPos->bottom());
    }

    if ( int bs = borderSpace() >= 0 )
    {
	int l = mMAX( 0,chldRect.left() - bs );
	int t = mMAX( 0,chldRect.top() - bs );
	int w = chldRect.hNrPics() + 2*bs;
	int h = chldRect.vNrPics() + 2*bs;

	uiRect ret(l,t,0,0);
	ret.setHNrPics(w);
	ret.setVNrPics(h);
	return ret;
    }

    return chldRect;
}


void i_LayoutMngr::invalidate() 
{ 
    for ( QPtrListIterator<i_LayoutItem> childIter( childrenList );
		    i_LayoutItem* curChld = childIter.current(); ++childIter )
    { 
	curChld->invalidate(); 
    }
}

void i_LayoutMngr::updatedAlignment(layoutMode lom)
{ 
    for ( QPtrListIterator<i_LayoutItem> childIter( childrenList );
		    i_LayoutItem* curChld = childIter.current(); ++childIter )
    { 
	curChld->updatedAlignment(lom);
    }
}

void i_LayoutMngr::initChildLayout(layoutMode lom)
{ 
    for ( QPtrListIterator<i_LayoutItem> childIter( childrenList );
		    i_LayoutItem* curChld = childIter.current(); ++childIter )
    { 
	curChld->initLayout( lom, -1, -1 );
    }
}



//! return true if successful
bool i_LayoutMngr::attach ( constraintType type, QWidget& current, 
			    QWidget* other, int margin, bool reciprocal) 
{
    if (&current == other)
	{ pErrMsg("Cannot attach an object to itself"); return false; }
    
    i_LayoutItem *cur=0, *oth=0;
    for ( QPtrListIterator<i_LayoutItem> childIter( childrenList );
			i_LayoutItem* loop = childIter.current(); ++childIter )
    {
	if ( loop->qwidget() == &current)	cur = loop;
        if ( loop->qwidget() == other)		oth = loop;
	if ( cur && oth ) break;
    }

    if (cur && ((!oth && !other) || (other && oth && (oth->qwidget()==other)) ))
    {
	cur->attach( type, oth, margin, reciprocal );
	return true;
    }

    const char* curnm =  current.name();
    const char* othnm =  other ? other->name() : "";
    
    BufferString msg( "Cannot attach " );
    msg += curnm;
    msg += " and ";
    msg += othnm;
    pErrMsg( msg );

    return false;
}

