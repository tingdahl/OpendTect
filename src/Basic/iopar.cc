/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-12-1995
-*/

static const char* rcsID = "$Id: iopar.cc,v 1.31 2003-09-03 10:57:38 bert Exp $";

#include "iopar.h"
#include "ascstream.h"
#include "aobset.h"
#include "position.h"
#include "separstr.h"
#include "multiid.h"
#include "globexpr.h"
#include "bufstring.h"
#include "strmdata.h"
#include "strmprov.h"
#include "keystrs.h"
#include <ctype.h>

IOPar::IOPar( const char* nm )
	: UserIDObject(nm)
	, pars_(*new AliasObjectSet(this))
{
}


IOPar::IOPar( UserIDObject* u )
	: UserIDObject(u)
	, pars_(*new AliasObjectSet(this))
{
}


IOPar::IOPar( const IOPar& iop )
	: UserIDObject(iop.name())
	, pars_(*new AliasObjectSet(this))
{
    for ( int idx=0; idx<iop.pars_.size(); idx++ )
	add( iop.pars_[idx]->name(), iop.pars_[idx]->obj->name() );
}


IOPar& IOPar::operator =( const IOPar& iop )
{
    if ( this != &iop )
    {
	clear();
	setName( iop.name() );
	for ( int idx=0; idx<iop.pars_.size(); idx++ )
	    add( iop.pars_[idx]->name(), iop.pars_[idx]->obj->name() );
    }
    return *this;
}


bool IOPar::isEqual( const IOPar& iop, bool worder ) const
{
    if ( &iop == this ) return true;
    const int sz = size();
    if ( iop.size() != sz ) return false;

    for ( int idx=0; idx<sz; idx++ )
    {
	if ( worder )
	{
	    if ( iop.pars_[idx]->name() != pars_[idx]->name()
		|| iop.pars_[idx]->obj->name() != pars_[idx]->obj->name() )
		return false;
	}
	else
	{
	    const char* res = iop.find( getKey(idx) );
	    if ( !res || strcmp(res,getValue(idx)) )
		return false;
	}
    }

    return true;
}


IOPar::~IOPar()
{
    clear();
    delete &pars_;
}


int IOPar::size() const
{
    return pars_.size();
}


const char* IOPar::getKey( int nr ) const
{
    if ( nr >= size() ) return "";
    return pars_[nr]->name();
}


const char* IOPar::getValue( int nr ) const
{
    if ( nr >= size() ) return "";
    return pars_[nr]->obj->name();
}


bool IOPar::setKey( int nr, const char* s )
{
    if ( nr >= size() || !s || !*s || pars_.indexOf(s) >= 0 )
	return false;

    pars_[nr]->setName( s );
    return true;
}


void IOPar::setValue( int nr, const char* s )
{
    if ( nr < size() ) pars_[nr]->obj->setName( s );
}


void IOPar::clear()
{
    pars_.deepEraseWithObjs();
}


void IOPar::remove( int idx )
{
    if ( idx >= size() ) return;
    
    AliasObject* aob = pars_[idx];
    pars_ -= aob;
    delete aob->obj; delete aob;
}


void IOPar::merge( const IOPar& iopar )
{
    for ( int idx=0; idx<iopar.pars_.size(); idx++ )
	set( iopar.pars_[idx]->name(), iopar.pars_[idx]->obj->name() );
}


const char* IOPar::compKey( const char* key1, int k2 )
{
    static BufferString intstr;
    intstr = ""; intstr += k2;
    return compKey( key1, (const char*)intstr );
}


const char* IOPar::compKey( const char* key1, const char* key2 )
{
    static BufferString ret;
    ret = key1;
    if ( key1 && key2 && *key1 && *key2 ) ret += ".";
    ret += key2;
    return ret;
}


IOPar* IOPar::subselect( int nr ) const
{
    BufferString s; s+= nr;
    return subselect( s );
}


IOPar* IOPar::subselect( const char* key ) const
{
    if ( !key ) return 0;

    IOPar* iopar = new IOPar( name() );
    for ( int idx=0; idx<pars_.size(); idx++ )
    {
	const char* nm = pars_[idx]->name();
	if ( !matchString(key,nm) ) continue;
	nm += strlen(key);
	if ( *nm == '.' && *(nm+1) )
	    iopar->add( nm+1, pars_[idx]->obj->name() );
    }

    if ( iopar->pars_.size() == 0 )
	{ delete iopar; iopar = 0; }
    return iopar;
}


void IOPar::mergeComp( const IOPar& iopar, const char* key )
{
    static BufferString buf;

    for ( int idx=0; idx<iopar.pars_.size(); idx++ )
    {
	buf = key;
	buf += ".";
	buf += iopar.pars_[idx]->name();
	set( buf, iopar.pars_[idx]->obj->name() );
    }
}


const char* IOPar::findKeyFor( const char* s, int nr ) const
{
    if ( !s ) return 0;

    for ( int idx=0; idx<pars_.size(); idx++ )
    {
	if ( pars_[idx]->obj->name() == s )
	{
	    if ( nr )	nr--;
	    else	return (const char*)pars_[idx]->name();
	}
    }

    return 0;
}


void IOPar::removeWithKey( const char* key )
{
    GlobExpr ge( key );
    for ( int idx=0; idx<pars_.size(); idx++ )
    {
	AliasObject* aob = pars_[idx];
	if ( ge.matches( aob->name() ) )
	{
	    pars_.remove( idx );
	    delete aob->obj; delete aob;
	    idx--;
	}
    }
}


const char* IOPar::operator[]( const char* keyw ) const
{
    const char* res = find( keyw );
    return res ? res : "";
}


const char* IOPar::find( const char* keyw ) const
{
    AliasObject* aob = pars_[keyw];
    return aob && aob->obj ? (const char*)aob->obj->name() : 0;
}


void IOPar::add( const char* nm, const char* val )
{
    UserIDObject* valstr = new UserIDObject( val );
    pars_ += new AliasObject( valstr, nm );
}


#define get1Val( type, convfunc ) \
bool IOPar::get( const char* s, type& res ) const \
{ \
    const char* ptr = (*this)[s]; \
    if ( !ptr || !*ptr ) return false; \
\
    char* endptr; \
    type tmpval = convfunc; \
    if ( ptr==endptr) return false; \
    res = tmpval; \
    return true; \
}

get1Val(int,strtol(ptr, &endptr, 0));
get1Val(long long,strtoll(ptr, &endptr, 0));
get1Val(unsigned long long,strtoull(ptr, &endptr, 0));

#define mGetMulti( type, function ) \
bool IOPar::get( const char* s, TypeSet<type>& res ) const\
{ \
    const char* ptr = (*this)[s]; \
    if ( !ptr || !*ptr ) return false;\
\
    FileMultiString fms(ptr);\
    if ( fms.size()<res.size() ) return false;\
\
    TypeSet<type> tmpres(res);\
    for ( int idx=0; idx<res.size(); idx++ )\
    {\
	ptr = fms[idx];\
	if ( !ptr || !*ptr ) return false;\
\
    	char* endptr;	\
	tmpres += function; \
	if ( ptr==endptr ) return false;\
    }\
\
    res = tmpres;\
\
    return true; \
}

mGetMulti( int, strtol(ptr, &endptr, 0) );
mGetMulti( long long, strtoll(ptr, &endptr, 0) );
mGetMulti( unsigned long long, strtoull(ptr, &endptr, 0) );
mGetMulti( double, strtod(ptr, &endptr ) );
mGetMulti( float, strtod(ptr, &endptr ) );

bool IOPar::get( const char* s, int& i1, int& i2 ) const
{
    const char* ptr = (*this)[s];
    bool havedata = false;
    if ( ptr && *ptr )
    {
	FileMultiString fms = ptr;
	ptr = fms[0];
	if ( *ptr ) { i1 = atoi( ptr ); havedata = true; }
	ptr = fms[1];
	if ( *ptr ) { i2 = atoi( ptr ); havedata = true; }
    }
    return havedata;
}


bool IOPar::getSc( const char* s, float& f, float sc, bool udf ) const
{
    const char* ptr = (*this)[s];
    if ( ptr && *ptr )
    {
	f = atof( ptr );
	if ( !mIsUndefined(f) ) f *= sc;
	return true;
    }
    else if ( udf )
	f = mUndefValue;

    return false;
}


bool IOPar::getSc( const char* s, double& d, double sc, bool udf ) const
{
    const char* ptr = (*this)[s];
    if ( ptr && *ptr )
    {
	d = atof( ptr );
	if ( !mIsUndefined(d) ) d *= sc;
	return true;
    }
    else if ( udf )
	d = mUndefValue;

    return false;
}


bool IOPar::getSc( const char* s, float& f1, float& f2, float sc,
		   bool udf ) const
{
    double d1, d2;
    if ( getSc( s, d1, d2, sc, udf ) )
	{ f1 = (float)d1; f2 = (float)d2; return true; }
    return false;
}


bool IOPar::getSc( const char* s, float& f1, float& f2, float& f3, float sc,
		   bool udf ) const
{
    double d1, d2, d3;
    if ( getSc( s, d1, d2, d3, sc, udf ) )
	{ f1 = (float)d1; f2 = (float)d2; f3 = (float)d3; return true; }
    return false;
}


bool IOPar::getSc( const char* s, float& f1, float& f2, float& f3, float& f4,
		   float sc, bool udf ) const
{
    double d1, d2, d3, d4;
    if ( getSc( s, d1, d2, d3, d4, sc, udf ) )
    {
	f1 = (float)d1; f2 = (float)d2; f3 = (float)d3; f4 = (float)d4;
	return true;
    }
    return false;

}


bool IOPar::getSc( const char* s, double& d1, double& d2, double sc,
		 bool udf ) const
{
    const char* ptr = (*this)[s];
    bool havedata = false;
    if ( udf || *ptr )
    {
	FileMultiString fms = ptr;
	ptr = fms[0];
	if ( *ptr )
	{
	    havedata = true;
	    d1 = atof( ptr );
	    if ( !mIsUndefined(d1) ) d1 *= sc;
	}
	else if ( udf )
	    d1 = mUndefValue;

	ptr = fms[1];
	if ( *ptr )
	{
	    havedata = true;
	    d2 = atof( ptr );
	    if ( !mIsUndefined(d2) ) d2 *= sc;
	}
	else if ( udf )
	    d2 = mUndefValue;
    }
    return havedata;
}


bool IOPar::getSc( const char* s, double& d1, double& d2, double& d3, double sc,
		 bool udf ) const
{
    const char* ptr = (*this)[s];
    bool havedata = false;
    if ( udf || *ptr )
    {
	FileMultiString fms = ptr;
	ptr = fms[0];
	if ( *ptr )
	{
	    d1 = atof( ptr );
	    if ( !mIsUndefined(d1) ) d1 *= sc;
	    havedata = true;
	}
	else if ( udf )
	    d1 = mUndefValue;

	ptr = fms[1];
	if ( *ptr )
	{
	    d2 = atof( ptr );
	    if ( !mIsUndefined(d2) ) d2 *= sc;
	    havedata = true;
	}
	else if ( udf )
	    d2 = mUndefValue;

	ptr = fms[2];
	if ( *ptr )
	{
	    d3 = atof( ptr );
	    if ( !mIsUndefined(d3) ) d3 *= sc;
	    havedata = true;
	}
	else if ( udf )
	    d3 = mUndefValue;
    }
    return havedata;
}


bool IOPar::getSc( const char* s, double& d1, double& d2, double& d3,
		   double& d4, double sc, bool udf ) const
{
    const char* ptr = (*this)[s];
    bool havedata = false;
    if ( udf || *ptr )
    {
	FileMultiString fms = ptr;
	ptr = fms[0];
	if ( *ptr )
	{
	    havedata = true;
	    d1 = atof( ptr );
	    if ( !mIsUndefined(d1) ) d1 *= sc;
	}
	else if ( udf )
	    d1 = mUndefValue;

	ptr = fms[1];
	if ( *ptr )
	{
	    havedata = true;
	    d2 = atof( ptr );
	    if ( !mIsUndefined(d2) ) d2 *= sc;
	}
	else if ( udf )
	    d2 = mUndefValue;

	ptr = fms[2];
	if ( *ptr )
	{
	    havedata = true;
	    d3 = atof( ptr );
	    if ( !mIsUndefined(d3) ) d3 *= sc;
	}
	else if ( udf )
	    d3 = mUndefValue;

	ptr = fms[3];
	if ( *ptr )
	{
	    havedata = true;
	    d4 = atof( ptr );
	    if ( !mIsUndefined(d4) ) d4 *= sc;
	}
	else if ( udf )
	    d4 = mUndefValue;
    }
    return havedata;
}


bool IOPar::get( const char* s, int& i1, int& i2, int& i3 ) const
{
    const char* ptr = (*this)[s];
    bool havedata = false;
    if ( ptr && *ptr )
    {
	FileMultiString fms = ptr;
	ptr = fms[0];
	if ( *ptr ) { i1 = atoi( ptr ); havedata = true; }
	ptr = fms[1];
	if ( *ptr ) { i2 = atoi( ptr ); havedata = true; }
	ptr = fms[2];
	if ( *ptr ) { i3 = atoi( ptr ); havedata = true; }
    }
    return havedata;
}


bool IOPar::getYN( const char* s, bool& i, char c ) const
{
    const char* ptr = (*this)[s];
    if ( !ptr || !*ptr ) return false;

    if ( !c )	i = yesNoFromString(ptr);
    else	i = toupper(*ptr) == toupper(c);
    return true;
}


void IOPar::set( const char* keyw, const char* vals )
{
    AliasObject* par = pars_[keyw];
    if ( !par )
	add( keyw, vals );
    else
	par->obj->setName( vals );
}


void IOPar::set( const char* keyw, const char* vals1, const char* vals2 )
{
    FileMultiString fms( vals1 ); fms += vals2;
    AliasObject* par = pars_[keyw];
    if ( !par )
	add( keyw, fms );
    else
	par->obj->setName( fms );
}


#define mSetMulti(type, tostringfunc ) \
void IOPar::set( const char* keyw, const TypeSet<type>& vals ) \
{\
    if ( !vals.size() ) return;\
\
    type val = vals[0];\
    FileMultiString fms( tostringfunc );\
\
    const int nrvals = vals.size();\
\
    for ( int idx=1; idx<nrvals; idx++ )\
    {\
	val = vals[idx];\
	fms += tostringfunc;\
    }\
\
    set( keyw, fms );\
}


mSetMulti( int, getStringFromInt(0,val) );
mSetMulti( float, getStringFromFloat(0,val) );
mSetMulti( double, getStringFromDouble(0,val) );
mSetMulti( long long, getStringFromLongLong(0,val) );
mSetMulti( unsigned long long, getStringFromUnsignedLongLong(0,val) );


#define mSet1Val( type, tostringfunc ) \
void IOPar::set( const char* keyw, type val ) \
{\
    set( keyw, tostringfunc(0,val) );\
}

mSet1Val( int, getStringFromInt );
mSet1Val( long long, getStringFromLongLong );
mSet1Val( unsigned long long, getStringFromUnsignedLongLong );
mSet1Val( float, mIsUndefined(val) ? sUndefValue : getStringFromFloat );
mSet1Val( double, mIsUndefined(val) ? sUndefValue : getStringFromDouble);


void IOPar::set( const char* s, int i1, int i2 )
{
    FileMultiString fms = getStringFromInt(0,i1);
    fms.add( getStringFromInt(0,i2) );
    set( s, fms );
}


void IOPar::set( const char* s, float f1, float f2 )
{
    FileMultiString fms =
	     mIsUndefined(f1) ? sUndefValue : getStringFromFloat(0,f1);
    fms.add( mIsUndefined(f2) ? sUndefValue : getStringFromFloat(0,f2) );
    set( s, fms );
}


void IOPar::set( const char* s, float f1, float f2, float f3 )
{
    FileMultiString fms =
	     mIsUndefined(f1) ? sUndefValue : getStringFromFloat(0,f1);
    fms.add( mIsUndefined(f2) ? sUndefValue : getStringFromFloat(0,f2) );
    fms.add( mIsUndefined(f3) ? sUndefValue : getStringFromFloat(0,f3) );
    set( s, fms );
}


void IOPar::set( const char* s, float f1, float f2, float f3, float f4 )
{
    FileMultiString fms =
	     mIsUndefined(f1) ? sUndefValue : getStringFromFloat(0,f1);
    fms.add( mIsUndefined(f2) ? sUndefValue : getStringFromFloat(0,f2) );
    fms.add( mIsUndefined(f3) ? sUndefValue : getStringFromFloat(0,f3) );
    fms.add( mIsUndefined(f4) ? sUndefValue : getStringFromFloat(0,f4) );
    set( s, fms );
}


void IOPar::set( const char* s, double d1, double d2 )
{
    FileMultiString fms =
	     mIsUndefined(d1) ? sUndefValue : getStringFromDouble(0,d1);
    fms.add( mIsUndefined(d2) ? sUndefValue : getStringFromDouble(0,d2) );
    set( s, fms );
}


void IOPar::set( const char* s, double d1, double d2, double d3 )
{
    FileMultiString fms =
	     mIsUndefined(d1) ? sUndefValue : getStringFromDouble(0,d1);
    fms.add( mIsUndefined(d2) ? sUndefValue : getStringFromDouble(0,d2) );
    fms.add( mIsUndefined(d3) ? sUndefValue : getStringFromDouble(0,d3) );
    set( s, fms );
}


void IOPar::set( const char* s, double d1, double d2, double d3, double d4 )
{
    FileMultiString fms =
	     mIsUndefined(d1) ? sUndefValue : getStringFromDouble(0,d1);
    fms.add( mIsUndefined(d2) ? sUndefValue : getStringFromDouble(0,d2) );
    fms.add( mIsUndefined(d3) ? sUndefValue : getStringFromDouble(0,d3) );
    fms.add( mIsUndefined(d4) ? sUndefValue : getStringFromDouble(0,d4) );
    set( s, fms );
}


void IOPar::set( const char* s, int i1, int i2, int i3 )
{
    FileMultiString fms = getStringFromInt(0,i1);
    fms.add( getStringFromInt(0,i2) );
    fms.add( getStringFromInt(0,i3) );
    set( s, fms );
}


void IOPar::setYN( const char* keyw, bool i )
{
    set( keyw, getYesNoString(i) );
}


bool IOPar::get( const char* s, Coord& coord ) const
{ return get( s, coord.x, coord.y ); }
void IOPar::set( const char* s, const Coord& coord )
{ set( s, coord.x, coord.y ); }
bool IOPar::get( const char* s, BinID& binid ) const
{ return get( s, binid.inl, binid.crl ); }
void IOPar::set( const char* s, const BinID& binid )
{ set( s, binid.inl, binid.crl ); }


bool IOPar::get( const char* s, BufferString& bs ) const
{
    const char* res = find( s );
    if ( !res ) return false;
    bs = res;
    return true;
}


bool IOPar::get( const char* s, BufferString& bs1, BufferString& bs2 ) const
{
    const char* res = find( s );
    if ( !res ) return false;
    FileMultiString fms( res );
    bs1 = fms[0]; bs2 = fms[1];
    return true;
}


void IOPar::set( const char* s, const BufferString& bs )
{
    set( s, (const char*)bs );
}


void IOPar::set( const char* s, const BufferString& bs1,
				const BufferString& bs2 )
{
    set( s, (const char*)bs1, (const char*)bs2 );
}


bool IOPar::get( const char* s, MultiID& mid ) const
{
    const char* res = (*this)[s];
    if ( !res || !*res ) return false;
    mid = res;
    return true;
}


void IOPar::set( const char* s, const MultiID& mid )
{ 
    set( s, (const char*)mid );
}


IOPar::IOPar( ascistream& astream, bool withname )
	: UserIDObject("")
	, pars_(*new AliasObjectSet(this))
{
    if ( withname )
    {
	if ( atEndOfSection(astream) )
	    astream.next();
	setName( astream.keyWord() );
	astream.next();
    }
    pars_.getFrom( astream );
}


void IOPar::putTo( ascostream& astream, bool withname ) const
{
    astream.tabsOff();
    if ( withname ) astream.put( name() );
    pars_.putTo( astream );
    astream.newParagraph();
}


static const char* sersep = "#-#";

void IOPar::putTo( BufferString& str ) const
{
    str = name();
    BufferString buf;
    for ( int idx=0; idx<pars_.size(); idx++ )
    {
	buf = sersep;
	buf += pars_[idx]->name();
	buf += sersep;
	buf += pars_[idx]->obj->name();
	str += buf;
    }
}


void IOPar::getFrom( const char* str )
{
    clear();

    BufferString buf = str;
    char* ptrstart = buf.buf();
    char* ptr = ptrstart;

    bool name_done = false;
    AliasObject* aob = 0;
    while ( *ptr )
    {
	// advance to next separator or end of string
	while ( *ptr && *ptr != *sersep )
	    ptr++;
	if ( *ptr && (*(ptr+1) != *(sersep+1) || *(ptr+2) != *(sersep+2)) )
	    { ptr++; continue; }

	// skip separator
	if ( *ptr )
	    { *ptr++ = '\0'; if ( *ptr ) ptr++; if ( *ptr ) ptr++; }

	if ( !name_done )
	    { setName( ptrstart ); name_done = true; }
	else if ( !aob )
	{
	    aob = new AliasObject( new UserIDObject, ptrstart );
	    pars_ += aob;
	}
	else
	{
	    aob->obj->setName( ptrstart );
	    aob = 0;
	}

	ptrstart = ptr;
    }
}


bool IOPar::read( const char* fnm )
{
    StreamData sd = StreamProvider(fnm).makeIStream();
    if ( !sd.usable() ) return false;

    ascistream astream( *sd.istrm, YES );
    astream.next();
    setName( astream.keyWord() );

    pars_.getFrom( astream );
    sd.close();

    return true;
}


bool IOPar::dump( const char* fnm, const char* typ ) const
{
    if ( !typ ) typ = sKey::Pars;
    StreamData sd = StreamProvider(fnm).makeOStream();
    if ( !sd.usable() ) return false;

    ascostream astream( *sd.ostrm );
    BufferString ky( name() );
    if ( ky == "" ) ky = sKey::Pars;
    if ( !astream.putHeader( ky ) ) return false;

    pars_.putTo( astream );
    sd.close();
    return true;
}
