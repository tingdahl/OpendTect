/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          12/02/2003
 RCS:           $Id: uitable.cc,v 1.46 2006-09-19 19:00:46 cvskris Exp $
________________________________________________________________________

-*/


#include "uitable.h"

#include "uifont.h"
#include "uimenu.h"
#include "pixmap.h"
#include "uilabel.h"
#include "uiobjbody.h"
#include "uicombobox.h"
#include "convert.h"
#include "bufstringset.h"
#include "i_layoutitem.h"

#ifdef USEQT4
# define mQTable Q3Table
# define mQHeader Q3Header
# define mQMemArray Q3MemArray
#else
# define mQTable QTable
# define mQHeader QHeader
# define mQMemArray QMemArray
#endif

// leftMargin() / topMargin() are protected. 
// TODO : hack. Should be fixed by the Trolls.....
// Isssue: N49203 ; submitted 24 - 05 - 2004
/*

Arend wrote:

> The following methods in QScrollView are protected, while they are
> const
> functions:
>
>
> int leftMargin () const
> int topMargin () const
> int rightMargin () const
> int bottomMargin () const
>
> 

Yep, that's probably an API bug. We can't fix that before Qt 4.0, as the
access rights are part of the symbol generated by some compilers, but
we'll address it for the next Qt version.


*/
#define protected public

#include "i_qtable.h"
#include <qcolor.h>


class CellObject
{
    public:
			CellObject( QWidget* qw, uiObject* obj )
			    : qwidget_(qw)
			    , object_(obj)	{}

    uiObject*		object_;
    QWidget*		qwidget_;
};


class uiTableBody : public uiObjBodyImpl<uiTable,mQTable>
{
public:

                        uiTableBody( uiTable& handle, uiParent* parnt, 
				     const char* nm, int nrows, int ncols )
			    : uiObjBodyImpl<uiTable,mQTable>(handle,parnt,nm)
			    , messenger_ (*new i_tableMessenger(this,&handle))
			{
			    if ( nrows >= 0 ) setLines( nrows+1 );
			    if ( ncols >= 0 ) setNumCols( ncols );
			}

    virtual 		~uiTableBody()
			{
			    deepErase( cellobjects_ );
			    delete &messenger_;
			}

    void 		setLines( int prefNrLines )
			{ 
			    setNumRows( prefNrLines - 1 );
			    if ( prefNrLines > 1 )
			    {
				const int rowh = rowHeight( 0 );
				const int prefh = rowh * (prefNrLines-1) + 30;
				setPrefHeight( mMIN(prefh,200) );
			    }

			    int hs = stretch(true);
			    if ( stretch(false) != 1 )
				setStretch( hs, ( nrTxtLines()== 1) ? 0 : 2 );
			}

    virtual int 	nrTxtLines() const
			    { return numRows() >= 0 ? numRows()+1 : 7; }

    void		setRowLabels( const QStringList &labels )
			{
			    mQHeader* leftHeader = verticalHeader();

			    int i = 0;
			    for( QStringList::ConstIterator it = labels.begin();
				 it != labels.end() && i < numRows();
				 ++i,++it )
				leftHeader->setLabel( i, *it );
			}

    void		setCellObject( const RowCol& rc,
	    			       uiObject* obj )
			{
			    QWidget* qwidget = obj->body()->qwidget();
			    setCellWidget( rc.row, rc.col, qwidget );
			    cellobjects_ += new CellObject( qwidget, obj );
			}

    uiObject*		getCellObject( const RowCol& rc ) const
			{
			    QWidget* qw = cellWidget( rc.row, rc.col );
			    if ( !qw ) return 0;

			    uiObject* obj = 0;
			    for ( int idx=0; idx<cellobjects_.size(); idx++ )
			    {
				if ( cellobjects_[idx]->qwidget_ == qw )
				{
				    obj = cellobjects_[idx]->object_;
				    break;
				}
			    }

			    return obj;
			}

    void		clearCellObject( const RowCol& rc )
			{
			    QWidget* qw = cellWidget( rc.row, rc.col );
			    if ( !qw ) return;

			    CellObject* co = 0;
			    for ( int idx=0; idx<cellobjects_.size(); idx++ )
			    {
				if ( cellobjects_[idx]->qwidget_ == qw )
				{
				    co = cellobjects_[idx];
				    break;
				}
			    }

			    clearCellWidget( rc.row, rc.col );
			    if ( co )
			    {
				cellobjects_ -= co;
				delete co;
			    }
			}

protected:

    ObjectSet<CellObject> cellobjects_;

private:

    i_tableMessenger&	messenger_;

};


uiTable::uiTable( uiParent* p, const Setup& s, const char* nm )
    : uiObject(p,nm,mkbody(p,nm,s.size_.height(),s.size_.width()))
    , setup_(s)
    , valueChanged(this)
    , clicked(this)
    , rightClicked(this)
    , leftClicked(this)
    , doubleClicked(this)
    , rowInserted(this)
    , colInserted(this)
    , rowDeleted(this)
    , colDeleted(this)
{
    clicked.notify( mCB(this,uiTable,clicked_) );
    setGeometry.notify( mCB(this,uiTable,geometrySet_) );

    setHSzPol( uiObject::MedVar );
    setVSzPol( uiObject::SmallVar );

//    setStretch( s.colgrow_ ? 2 : 1, s.rowgrow_ ? 2 : 1 );

    setSelectionMode( s.selmode_ );
    if ( s.defrowlbl_ )
	setDefaultRowLabels();
    if ( s.defcollbl_ )
	setDefaultColLabels();
}


uiTableBody& uiTable::mkbody( uiParent* p, const char* nm, int nr, int nc )
{
    body_ = new uiTableBody( *this, p, nm, nr, nc );
    return *body_;
}


uiTable::~uiTable()
{}


void uiTable::showGrid( bool yn )
{
    body_->setShowGrid( yn );
}


bool uiTable::gridShown() const
{
    return body_->showGrid();
}


void uiTable::setDefaultRowLabels()
{
    const int nrrows = nrRows();
    for ( int idx=0; idx<nrrows; idx++ )
    {
	BufferString lbl( setup_.rowdesc_ ); lbl += " ";
	lbl += idx+1;
	setRowLabel( idx, lbl );
    }
}


void uiTable::setDefaultColLabels()
{
    const int nrcols = nrCols();
    for ( int idx=0; idx<nrcols; idx++ )
    {
	BufferString lbl( setup_.coldesc_ ); lbl += " ";
	lbl += idx+1;
	setColumnLabel( idx, lbl );
    }
}

#define updateRow(r) update( true, r )
#define updateCol(c) update( false, c )

void uiTable::update( bool row, int rc )
{
    if ( row && setup_.defrowlbl_ ) setDefaultRowLabels(); 
    else if ( !row && setup_.defcollbl_ ) setDefaultColLabels();

    int max = row ? nrRows() : nrCols();

    if ( rc > max ) rc = max;
    if ( rc < 0 ) rc = 0;

    int r = row ? rc : 0;
    int c = row ? 0 : rc;

    setCurrentCell( RowCol(r,c) );
}


int uiTable::columnWidth( int col ) const
{
    return col == -1 ? body_->topMargin() : body_->columnWidth(col);
}


int uiTable::rowHeight( int row ) const	
{
    return row == -1 ? body_->leftMargin() : body_->rowHeight( row );
}


void uiTable::setLeftMargin( int width )
{
    body_->setLeftMargin( width );
}


void uiTable::setColumnWidth( int col, int w )	
{
    if ( col >= 0 )
	body_->setColumnWidth( col, w );
    else if ( col == -1 )
    {
	for ( int idx=0; idx<nrCols(); idx++ )
	    body_->setColumnWidth( idx, w );
    }
}


void uiTable::setColumnWidthInChar( int col, float w )
{
    const float wdt = w * body_->fontWdt();
    setColumnWidth( col, mNINT(wdt) );
}


void uiTable::setTopMargin( int h )
{
    body_->setTopMargin( h );
}


void uiTable::setRowHeight( int row, int h )
{
    if ( row >= 0 )
	body_->setRowHeight( row, h );
    else if ( row == -1 )
    {
	for ( int idx=0; idx<nrRows(); idx++ )
	    body_->setRowHeight( idx, h );
    }
}


void uiTable::setRowHeightInChar( int row, float h )
{
    float hgt = h * body_->fontHgt();
    setRowHeight( row, mNINT(hgt) );
}


void uiTable::removeRCs( const TypeSet<int>& idxs, bool col )
{
    if ( idxs.size() < 1 ) return;
    const int first = idxs[0];
    if ( idxs.size() == 1 )
    {
	col ? removeColumn( first ) : removeRow( first );
	return;
    }

    mQMemArray<int> qidxs( idxs.size() );
    for ( int idx=0; idx<idxs.size(); idx++ )
	qidxs.at(idx) = idxs[idx];

    if ( col )
    {
	body_->removeColumns( qidxs );
	updateCol( first );
    }
    else
    {
	body_->removeRows( qidxs );
	updateRow( first );
    }
}


void uiTable::insertRows( int row, int cnt )
{ body_->insertRows( row, cnt ); updateRow(row); }

void uiTable::insertColumns( int col, int cnt )
{ body_->insertColumns(col,cnt); updateCol(col); }

void uiTable::removeRow( int row )
{ body_->removeRow( row ); updateRow(row); }

void uiTable::removeColumn( int col )
{ body_->removeColumn( col );  updateCol(col); }

void uiTable::removeRows( const TypeSet<int>& idxs )
{ removeRCs( idxs, false ); }

void uiTable::removeColumns( const TypeSet<int>& idxs )
{ removeRCs( idxs, true ); }

void uiTable::setNrRows( int nr )
{ body_->setLines( nr + 1 ); updateRow(0); }

void uiTable::setNrCols( int nr )
{ body_->setNumCols( nr );  updateCol(0); }

int uiTable::nrRows() const			{ return body_->numRows(); }
int uiTable::nrCols() const			{ return body_->numCols(); }

void uiTable::setText( const RowCol& rc, const char* txt )
{ body_->setText( rc.row, rc.col, txt ); }

void uiTable::clearCell( const RowCol& rc )
{ body_->clearCell( rc.row, rc.col ); }

void uiTable::setCurrentCell( const RowCol& rc )
{ body_->setCurrentCell( rc.row, rc.col ); }


const char* uiTable::text( const RowCol& rc ) const
{
// TODO: if cellobject on this rc, get from cellobject
    body_->endEdit( rc.row, rc.col, true, false );
    static BufferString rettxt;
    rettxt = (const char*) body_->text( rc.row, rc.col );
    return rettxt;
}


void uiTable::setTableReadOnly( bool yn )
{ body_->setReadOnly( yn ); }


bool uiTable::isTableReadOnly() const
{ return body_->isReadOnly(); }


#define mSetFunc(func) \
    void uiTable::func( int rc, bool yn ) \
    { body_->func( rc, yn ); }

#define mIsFunc(func) \
    bool uiTable::func( int rc ) const\
    { return body_->func( rc ); }

mSetFunc( setColumnReadOnly )
mIsFunc( isColumnReadOnly )
mSetFunc( setRowReadOnly )
mIsFunc( isRowReadOnly )


void uiTable::hideColumn( int col, bool yn )
{
    if ( yn ) body_->hideColumn( col );
    else body_->showColumn( col );
}

void uiTable::hideRow( int col, bool yn )
{
    if ( yn ) body_->hideRow( col );
    else body_->showRow( col );
}

bool uiTable::isColumnHidden( int col ) const
{
#if QT_VERSION < 0x030300
    pErrMsg("Function is not supported by QT");
    return false;
#else
    return body_->isColumnHidden(col);
#endif
}


bool uiTable::isRowHidden( int row ) const
{
#if QT_VERSION < 0x030300
    pErrMsg("Function is not supported by QT");
    return false;
#else
    return body_->isRowHidden(row);
#endif
}


mSetFunc( setColumnStretchable )
mIsFunc( isColumnStretchable )
mSetFunc( setRowStretchable )
mIsFunc( isRowStretchable )


void uiTable::setPixmap( const RowCol& rc, const ioPixmap& pm )
{ body_->setPixmap( rc.row, rc.col, *pm.Pixmap() ); }


void uiTable::setColor( const RowCol& rc, const Color& col )
{
    QRect rect = body_->cellRect( rc.row, rc.col );
    const int widthbuffer = 10;
    QPixmap pix( rect.width()*widthbuffer, rect.height() );
    QColor qcol( col.r(), col.g(), col.b() );
    pix.fill( qcol );
    body_->setPixmap( rc.row, rc.col, pix );
    body_->setFocus();
    body_->setText( rc.row, rc.col, qcol.name() );
}


const Color uiTable::getColor( const RowCol& rc ) const
{
    BufferString coltxt = text(rc);
    if ( !*coltxt ) coltxt = "white";
	
    QColor qcol( coltxt.buf() );
    return Color( qcol.red(), qcol.green(), qcol.blue() );
}


const char* uiTable::rowLabel( int nr ) const
{
    static BufferString ret;
    mQHeader* topHeader = body_->verticalHeader();
    ret = (const char*) topHeader->label( nr );
    return ret;
}


void uiTable::setRowLabel( int row, const char* label )
{
    mQHeader* topHeader = body_->verticalHeader();
    topHeader->setLabel( row, label );

    //setRowStretchable( row, true );
}


void uiTable::setRowLabels( const char** labels )
{
    int nlabels=0;
    while ( labels[nlabels] )
	nlabels++;

    body_->setLines( nlabels + 1 );
    for ( int idx=0; idx<nlabels; idx++ )
	setRowLabel( idx, labels[idx] );
}


void uiTable::setRowLabels( const BufferStringSet& labels )
{
    body_->setLines( labels.size() + 1 );

    for ( int i=0; i<labels.size(); i++ )
        setRowLabel( i, *labels[i] );
}


const char* uiTable::columnLabel( int nr ) const
{
    static BufferString ret;
    mQHeader* topHeader = body_->horizontalHeader();
    ret = (const char*) topHeader->label( nr );
    return ret;
}


void uiTable::setColumnLabel( int col, const char* label )
{
    mQHeader* topHeader = body_->horizontalHeader();
    topHeader->setLabel( col, label );

    //setColumnStretchable( col, true );
}


void uiTable::setColumnLabels( const char** labels )
{
    int nlabels=0;
    while ( labels[nlabels] )
	nlabels++;

    body_->setNumCols( nlabels );
    for ( int idx=0; idx<nlabels; idx++ )
	setColumnLabel( idx, labels[idx] );
}


void uiTable::setColumnLabels( const BufferStringSet& labels )
{
    body_->setNumCols( labels.size() );

    for ( int i=0; i<labels.size(); i++ )
        setColumnLabel( i, *labels[i] );
}


int uiTable::getIntValue( const RowCol& rc ) const
{
// TODO: if cellobject on this rc, get from cellobject
    const char* str = text( rc );
    if ( !str || !*str ) return mUdf(int);

    return Conv::to<int>( text(rc) );
}


double uiTable::getValue( const RowCol& rc ) const
{
// TODO: if cellobject on this rc, get from cellobject
    const char* str = text( rc );
    if ( !str || !*str ) return mUdf(double);

    return Conv::to<double>(str);
}


float uiTable::getfValue( const RowCol& rc ) const
{
// TODO: if cellobject on this rc, get from cellobject
    const char* str = text( rc );
    if ( !str || !*str ) return mUdf(float);

    return Conv::to<float>(str);
}


void uiTable::setValue( const RowCol& rc, int i )
{
// TODO: if cellobject on this rc, set to cellobject
    setText( rc, Conv::to<const char*>(i) );
}


void uiTable::setValue( const RowCol& rc, float f )
{
// TODO: if cellobject on this rc, set to cellobject
    setText( rc, mIsUdf(f) ? "" : Conv::to<const char*>(f) );
}


void uiTable::setValue( const RowCol& rc, double d )
{
// TODO: if cellobject on this rc, set to cellobject
    setText( rc, mIsUdf(d) ? "" : Conv::to<const char*>(d) );
}


void uiTable::setSelectionMode( SelectionMode m )
{
    switch ( m ) 
    {
	case Single : body_->setSelectionMode( mQTable::Single ); break;
	case Multi : body_->setSelectionMode( mQTable::Multi ); break;
	case SingleRow : body_->setSelectionMode( mQTable::SingleRow ); break;
	case MultiRow : body_->setSelectionMode( mQTable::MultiRow ); break;
	default :  body_->setSelectionMode( mQTable::NoSelection ); break;
    }

}


void uiTable::clicked_( CallBacker* cb )
{
    mCBCapsuleUnpack(const MouseEvent&,ev,cb);

    if ( ev.buttonState() & OD::RightButton )
	{ rightClk(); rightClicked.trigger(); return; }

    if ( ev.buttonState() & OD::LeftButton )
	leftClicked.trigger();

    if ( setup_.snglclkedit_ )
	editCell( notifcell_, false );
}


void uiTable::editCell( const RowCol& rc, bool replace )
{
    body_->editCell( rc.row, rc.col, replace );
}


void uiTable::rightClk()
{
    if ( !setup_.rowgrow_ && !setup_.colgrow_ )
	return;

    uiPopupMenu* mnu = new uiPopupMenu( parent(), "Action" );
    BufferString itmtxt;

    int inscolbef = 0;
    int delcol = 0;
    int inscolaft = 0;
    if ( setup_.colgrow_ )
    {
	itmtxt = "Insert "; itmtxt += setup_.coldesc_; itmtxt += " before";
	inscolbef = mnu->insertItem( new uiMenuItem( itmtxt ) );
	itmtxt = "Remove "; itmtxt += setup_.coldesc_;
	delcol = mnu->insertItem( new uiMenuItem( itmtxt ) );
	itmtxt = "Insert "; itmtxt += setup_.coldesc_; itmtxt += " after";
	inscolaft = mnu->insertItem( new uiMenuItem( itmtxt ) );
    }

    int insrowbef = 0;
    int delrow = 0;
    int insrowaft = 0;
    if ( setup_.rowgrow_ )
    {
	itmtxt = "Insert "; itmtxt += setup_.rowdesc_; itmtxt += " before";
	insrowbef = mnu->insertItem( new uiMenuItem( itmtxt ) );
	itmtxt = "Remove "; itmtxt += setup_.rowdesc_;
	delrow = mnu->insertItem( new uiMenuItem( itmtxt ) );
	itmtxt = "Insert "; itmtxt += setup_.rowdesc_; itmtxt += " after";
	insrowaft = mnu->insertItem( new uiMenuItem( itmtxt ) );
    }

    int ret = mnu->exec();
    if ( !ret ) return;

    RowCol cur = notifiedCell();

    if ( ret == inscolbef || ret == inscolaft )
    {
	const int offset = (ret == inscolbef) ? 0 : 1;
	newcell_ = RowCol( cur.row, cur.col + offset );
	insertColumns( newcell_, 1 );

	if ( !setup_.defcollbl_ )
	{
	    BufferString label( newcell_.col );
	    setColumnLabel( newcell_, label );
	}

	colInserted.trigger();
    }
    else if ( ret == delcol )
    {
	removeColumn( cur.col );
	colDeleted.trigger();
    }
    else if ( ret == insrowbef || ret == insrowaft  )
    {
	const int offset = (ret == insrowbef) ? 0 : 1;
	newcell_ = RowCol( cur.row + offset, cur.col );
	insertRows( newcell_, 1 );

	if ( !setup_.defrowlbl_ )
	{
	    BufferString label( newcell_.row );
	    setRowLabel( newcell_, label );
	}

	rowInserted.trigger();
    }
    else if ( ret == delrow )
    {
	removeRow( cur.row );
	rowDeleted.trigger();
    }

    setCurrentCell( newcell_ );
    updateCellSizes();
}


void uiTable::geometrySet_( CallBacker* cb )
{
//    if ( !mainwin() ||  mainwin()->poppedUp() ) return;
    mCBCapsuleUnpack(uiRect&,sz,cb);

    uiSize size = sz.size();
    updateCellSizes( &size );
}


void uiTable::updateCellSizes( const uiSize* size )
{
    if ( size ) lastsz = *size;
    else	size = &lastsz;

    if ( !setup_.manualresize_ && body_->layoutItem()->inited() )
    {
	if ( !setup_.fillcol_ )
	    for ( int idx=0; idx < nrCols(); idx++ )
		body_->setColumnStretchable ( idx, true );

        if ( !setup_.fillrow_ )
	    for ( int idx=0; idx < nrRows(); idx++ )
		body_->setRowStretchable ( idx, true );
    }

    int nc = nrCols();
    if ( nc && setup_.fillrow_ )
    {
	int width = size->hNrPics();
	int availwdt = width - body_->verticalHeader()->frameSize().width()
			 - 2*body_->frameWidth();

	int colwdt = availwdt / nc;

	const int minwdt = (int)(setup_.mincolwdt_ * (float)font()->avgWidth());
	const int maxwdt = (int)(setup_.maxcolwdt_ * (float)font()->avgWidth());

	if ( colwdt < minwdt ) colwdt = minwdt;
	if ( colwdt > maxwdt ) colwdt = maxwdt;

	for ( int idx=0; idx < nc; idx ++ )
	{
	    if ( idx < nc-1 )
		setColumnWidth( idx, colwdt );
	    else 
	    {
		int wdt = availwdt;
		if ( wdt < minwdt )	wdt = minwdt;
		if ( wdt > maxwdt )	wdt = maxwdt;

		setColumnWidth( idx, wdt );
	    }
	    availwdt -= colwdt;
	}
    }

    int nr = nrRows();
    if ( nr && setup_.fillcol_ )
    {
	int height = size->vNrPics();
	int availhgt = height - body_->horizontalHeader()->frameSize().height()
			 - 2*body_->frameWidth();

	int rowhgt =  availhgt / nr;
	const float fonthgt = (float)font()->height();

	const int minhgt = (int)(setup_.minrowhgt_ * fonthgt);
	const int maxhgt = (int)(setup_.maxrowhgt_ * fonthgt);

	if ( rowhgt < minhgt ) rowhgt = minhgt;
	if ( rowhgt > maxhgt ) rowhgt = maxhgt; 

	for ( int idx=0; idx < nr; idx ++ )
	{
	    if ( idx < nr-1 )
		setRowHeight( idx, rowhgt );
	    else
	    {
		int hgt = availhgt;
		if ( hgt < minhgt ) hgt = minhgt;
		if ( hgt > maxhgt ) hgt = maxhgt;

		setRowHeight( idx, hgt );
	    }
	    availhgt -= rowhgt;
	}
    }
}


void uiTable::clearTable()
{
    for ( int row = 0 ; row < nrRows() ; row++ )
	for ( int col = 0 ; col < nrCols() ; col++ )
	    clearCell( RowCol(row,col) );
}


void uiTable::removeAllSelections()
{
    for ( int idx = 0; idx < body_->numSelections(); ++idx )
	body_->removeSelection( idx );
}


bool uiTable::isRowSelected( int row ) const
{
    return body_->isRowSelected( row );
}


bool uiTable::isColumnSelected( int col ) const
{
    return body_->isColumnSelected( col );
}


int uiTable::currentRow() const
{
    return body_->currentRow();
}


int uiTable::currentCol() const
{
    return body_->currentColumn();
}


void  uiTable::selectRow( int row )
{
    return body_->selectRow( row );
}


void  uiTable::selectColumn( int col )
{
    return body_->selectColumn( col );
}


void uiTable::ensureCellVisible( const RowCol& rc )
{
    body_->ensureCellVisible( rc.row, rc.col );
}


void uiTable::setCellObject( const RowCol& rc, uiObject* obj )
{
    body_->setCellObject( rc, obj );
}


uiObject* uiTable::getCellObject( const RowCol& rc ) const
{
    return body_->getCellObject( rc );
}


void uiTable::clearCellObject( const RowCol& rc )
{
    body_->clearCellObject( rc );
}
