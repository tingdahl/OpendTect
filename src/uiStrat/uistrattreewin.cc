/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene
 Date:          July 2007
 RCS:		$Id: uistrattreewin.cc,v 1.10 2007-08-31 14:48:08 cvshelene Exp $
________________________________________________________________________

-*/

#include "uistrattreewin.h"

#include "compoundkey.h"
#include "stratlevel.h"
#include "stratunitrepos.h"
#include "uicolor.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uigroup.h"
#include "uilistbox.h"
#include "uilistview.h"
#include "uimenu.h"
#include "uisplitter.h"
#include "uistratreftree.h"
#include "uistratutildlgs.h"

#define	mExpandTxt	"&Expand all"
#define	mCollapseTxt	"&Collapse all"
#define	mEditTxt	"&Edit"
#define	mLockTxt	"&Lock"


static const char* infolvltrs[] =
{
    "Survey level",
    "OpendTect data level",
    "User level",
    "Global level",
    0
};

using namespace Strat;

uiStratTreeWin::uiStratTreeWin( uiParent* p )
    : uiMainWin(p,"Manage Stratigraphy", 0, true, true)
    , tmptree_( 0 )
{
    createMenus();
    createGroups();
    setExpCB(0);
}


uiStratTreeWin::~uiStratTreeWin()
{
    delete uitree_;
    if ( tmptree_ ) delete tmptree_;
}

    
void uiStratTreeWin::createMenus()
{
    uiMenuBar* menubar =  menuBar();
    uiPopupMenu* viewmnu = new uiPopupMenu( this, "&View" );
    expandmnuitem_ = new uiMenuItem( mExpandTxt,
				     mCB(this, uiStratTreeWin, setExpCB ) );
    viewmnu->insertItem( expandmnuitem_ );
    menubar->insertItem( viewmnu );

    uiPopupMenu* actmnu = new uiPopupMenu( this, "&Action" );
    editmnuitem_ = new uiMenuItem( mEditTxt, mCB(this,uiStratTreeWin,editCB) );
    actmnu->insertItem( editmnuitem_ );
    savemnuitem_ = new uiMenuItem( "&Save", mCB(this,uiStratTreeWin,saveCB) );
    actmnu->insertItem( savemnuitem_ );
    resetmnuitem_ = new uiMenuItem( "&Reset", mCB(this,uiStratTreeWin,resetCB));
    actmnu->insertItem( resetmnuitem_ );
    actmnu->insertSeparator();
    openmnuitem_ = new uiMenuItem( "&Open...", mCB(this,uiStratTreeWin,openCB));
    actmnu->insertItem( openmnuitem_ );
    saveasmnuitem_ = new uiMenuItem( "Save&As...",
	    			     mCB(this,uiStratTreeWin,saveAsCB) );
    actmnu->insertItem( saveasmnuitem_ );
    menubar->insertItem( actmnu );	    
}


void uiStratTreeWin::createGroups()
{
    uiGroup* leftgrp = new uiGroup( this, "LeftGroup" );
    leftgrp->setStretch( 1, 1 );
    uiGroup* rightgrp = new uiGroup( this, "RightGroup" );
    rightgrp->setStretch( 1, 1 );
    uitree_ = new uiStratRefTree( leftgrp, &Strat::RT() );
    const_cast<uiStratRefTree*>(uitree_)->listView()->selectionChanged.notify( 
	    				mCB( this,uiStratTreeWin,unitSelCB ) );
    lvllistfld_ = new uiLabeledListBox( rightgrp, "Existing Levels", false,
	    				uiLabeledListBox::AboveMid );
    lvllistfld_->box()->setStretch( 2, 2 );
    lvllistfld_->box()->setFieldWidth( 12 );
    lvllistfld_->box()->selectionChanged.notify( mCB( this, uiStratTreeWin,
						      selLvlChgCB ) );
    lvllistfld_->box()->rightButtonClicked.notify( mCB( this, uiStratTreeWin,
						      rClickLvlCB ) );
    fillLvlList();
    
    uiSplitter* splitter = new uiSplitter( this, "Splitter", true );
    splitter->addGroup( leftgrp );
    splitter->addGroup( rightgrp );
}


void uiStratTreeWin::setExpCB( CallBacker* )
{
    bool expand = !strcmp( expandmnuitem_->text(), mExpandTxt );
    uitree_->expand( expand );
    expandmnuitem_->setText( expand ? mCollapseTxt : mExpandTxt );
}


void uiStratTreeWin::unitSelCB(CallBacker*)
{
    /*uiListViewItem* item = uitree_->listView()->selectedItem();
    BufferString bs = item->text();
    int itemdepth = item->depth();
    for ( int idx=itemdepth-1; idx>=0; idx-- )
    {
	item = item->parent();
	CompoundKey kc( item->text() );
	kc += bs.buf();
	bs = kc.buf();
    }
    const Strat::UnitRef* ur = uitree_->findUnit( bs.buf() ); 
*/}


void uiStratTreeWin::editCB( CallBacker* )
{
    bool doedit = !strcmp( editmnuitem_->text(), mEditTxt );
    uitree_->makeTreeEditable( doedit );
    editmnuitem_->setText( doedit ? mLockTxt : mEditTxt );
    if ( doedit && !tmptree_ )
    {
	const Strat::RefTree* rt = &Strat::RT();
	tmptree_ = new Strat::RefTree( rt->treeName(), rt->source() );
	for ( int idx=0; idx<rt->nrLevels(); idx++ )
	{
	    Level* lvl = new Level( *rt->level(idx) );
	    tmptree_->addLevel( lvl );
	}
    }
}


#define mSaveAtLoc( loc )\
{\
    const_cast<UnitRepository*>(&UnRepo())->copyCurTreeAtLoc( loc );\
    UnRepo().write( loc );\
}\
    

void uiStratTreeWin::saveCB( CallBacker* )
{
    if ( tmptree_ )
	prepTreeForSave();
    
    mSaveAtLoc( Repos::Survey );
    tmptree_ = 0;
    editCB(0);
}


void uiStratTreeWin::saveAsCB( CallBacker* )
{
    const char* dlgtit = "Save the stratigraphy at:";
    const char* helpid = 0;
    uiDialog savedlg( this, uiDialog::Setup( "Save Stratigraphy",
					     dlgtit, helpid ) );
    BufferStringSet bfset( infolvltrs );
    uiListBox saveloclist( &savedlg, bfset );
    savedlg.go();
    if ( savedlg.uiResult() == 1 )
    {
	if ( tmptree_ )
	    prepTreeForSave();
	const char* savetxt = saveloclist.getText();
	if ( !strcmp( savetxt, infolvltrs[1] ) )
	    mSaveAtLoc( Repos::Data )
	else if ( !strcmp( savetxt, infolvltrs[2] ) )
	    mSaveAtLoc( Repos::User )
	else if ( !strcmp( savetxt, infolvltrs[3] ) )
	    mSaveAtLoc( Repos::ApplSetup )
	else
	    mSaveAtLoc( Repos::Survey )
	tmptree_ = 0;
    }

    editCB(0);
}


void uiStratTreeWin::resetCB( CallBacker* )
{
    uitree_->setTree( &Strat::RT(), true );
    uitree_->expand( true );
}


void uiStratTreeWin::openCB( CallBacker* )
{
    pErrMsg("Not implemented yet: uiStratTreeWin::openCB");
}


void uiStratTreeWin::selLvlChgCB( CallBacker* )
{
    pErrMsg("Not implemented yet: uiStratTreeWin::selLvlChgCB");
}


void uiStratTreeWin::rClickLvlCB( CallBacker* )
{
    if ( !strcmp( editmnuitem_->text(), mLockTxt ) ) return;
    int curit = lvllistfld_->box()->currentItem();
    uiPopupMenu mnu( this, "Action" );
    mnu.insertItem( new uiMenuItem("Create New ..."), 0 );
    if ( curit>-1 )
    {
	mnu.insertItem( new uiMenuItem("Edit ..."), 1 );
	mnu.insertItem( new uiMenuItem("Remove"), 2 );
    }
    const int mnuid = mnu.exec();
    if ( mnuid<0 || mnuid>2 ) return;
    if ( mnuid == 2 )
    {
	const Level* curlvl = RT().getLevel( lvllistfld_->box()->getText() );
	tmptree_->remove( curlvl );
	lvllistfld_->box()->removeItem( lvllistfld_->box()->currentItem() );
	return;
    }

    editLevel( mnuid ? false : true );
}


void uiStratTreeWin::fillLvlList()
{
    lvllistfld_->box()->empty();
    int nrlevels = RT().nrLevels();
    for ( int idx=0; idx<nrlevels; idx++ )
    {
	const Level* lvl = RT().level( idx );
	if ( !lvl ) return;
	lvllistfld_->box()->addItem( lvl->name_, lvl->color() );
    }
}


void uiStratTreeWin::fillTmpTreeFromListview()
{
    const uiListView* lv = uitree_->listView();
    uiListViewItem* lvit = lv->firstChild();
    while ( lvit )
    {
	BufferString lithodesc;
	const char* lithotxt = lvit->text(2);
	if ( lithotxt )
	{
	    int lithid = UnRepo().getLithID( lithotxt );
	    if ( lithid >-1 )
	    {
		lithodesc = lithid;
		lithodesc += "`";
	    }
	}
	lithodesc += lvit->text(1);
	tmptree_->addUnit( getCodeFromLVIt( lvit ), lithodesc, true );
	lvit = lvit->itemBelow();
    }
}


BufferString uiStratTreeWin::getCodeFromLVIt( const uiListViewItem* item ) const
{
    BufferString bs = item->text();
    int itemdepth = item->depth();
    for ( int idx=itemdepth-1; idx>=0; idx-- )
    {
	item = item->parent();
	CompoundKey kc( item->text() );
	kc += bs.buf();
	bs = kc.buf();
    }

    return bs;
}


void uiStratTreeWin::prepTreeForSave()
{
    fillTmpTreeFromListview();
    eUnRepo().replaceTree( tmptree_ );
}


void uiStratTreeWin::editLevel( bool create )
{
    uiStratLevelDlg newlvldlg( this );
    Level* curlvl = 0;
    if ( !create && lvllistfld_->box()->currentItem()>-1 )
    {
	curlvl = const_cast<Level*> ( RT().getLevel( lvllistfld_->box()
		    					->getText() ) );
	if ( curlvl )
	{
	    newlvldlg.lvlnmfld_->setText( curlvl->name_ );
	    newlvldlg. lvltimefld_->setValue( curlvl->time() );
	    newlvldlg.lvlcolfld_->setColor( curlvl->color() );
	}
    }
    if ( newlvldlg.go() )
    {
	if ( !curlvl )
	{
	    curlvl = new Level( newlvldlg.lvlnmfld_->text(), 0, false );
	    tmptree_->addLevel( curlvl );

	}
	else
	    curlvl->name_ = newlvldlg.lvlnmfld_->text();

	curlvl->setColor( newlvldlg.lvlcolfld_->color() );
	curlvl->setTime( newlvldlg.lvltimefld_->getfValue() );
	if ( create )
	    lvllistfld_->box()->addItem( curlvl->name_, curlvl->color() );
	else
	{
	    int curit = lvllistfld_->box()->currentItem();
	    lvllistfld_->box()->setItemText( curit, curlvl->name_ );
	    lvllistfld_->box()->setPixmap( curit, curlvl->color() );
	}
    }
	
}
