/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 1995
 * FUNCTION : Translator functions
-*/

static const char* rcsID = "$Id: iox.cc,v 1.5 2001-02-13 17:21:02 bert Exp $";

#include "iox.h"
#include "iolink.h"
#include "ioman.h"
#include "ascstream.h"
#include "conn.h"

DefineConcreteClassDef(IOX,"X-Group");


IOX::IOX( const char* nm, const char* ky, bool )
	: IOObject(nm,ky)
	, ownkey_("")
{
    connclassdef_ = &StreamConn::classdef;
}


IOX::~IOX()
{
}


void IOX::setOwnKey( const MultiID& ky )
{
    ownkey_ = ky;
}


const ClassDef& IOX::connType() const
{
    IOObj* ioobj = IOM().get( ownkey_ );
    const ClassDef& rv = ioobj ? ioobj->connType() : StreamConn::classdef;
    delete ioobj;
    return rv;
}


bool IOX::bad() const
{
    return ownkey_ == "";
}


void IOX::copyFrom( const IOObj* obj )
{
    if ( !obj ) return;
    if ( obj->isLink() ) obj = ((IOLink*)obj)->link();
    if ( !obj ) return;

    IOObj::copyFrom(obj);
    if ( obj->hasClass(IOX::classid) )
    {
	const IOX* trobj = (const IOX*)obj;
	ownkey_ = trobj->ownkey_;
    }
}


const char* IOX::fullUserExpr( bool i ) const
{
    IOObj* ioobj = IOM().get( ownkey_ );
    if ( !ioobj ) return "<invalid>";
    const char* s = ioobj->fullUserExpr(i);
    delete ioobj;
    return s;
}


bool IOX::slowOpen() const
{
    IOObj* ioobj = IOM().get( ownkey_ );
    if ( !ioobj ) return false;
    bool ret = ioobj->slowOpen();
    delete ioobj;
    return ret;
}


bool IOX::implExists( bool i ) const
{
    IOObj* ioobj = IOM().get( ownkey_ );
    if ( !ioobj ) return false;
    bool yn = ioobj->implExists(i);
    delete ioobj;
    return yn;
}


bool IOX::implRemovable() const
{
    return false;
}


bool IOX::implRemove() const
{
    return false;
}


Conn* IOX::getConn( Conn::State rw ) const
{
    IOObj* ioobj = getIOObj();
    if ( !ioobj ) return 0;

    Conn* conn = ioobj->getConn( rw );
    delete ioobj;

    conn->ioobj = (IOObj*)this;
    return conn;
}


IOObj* IOX::getIOObj() const
{
    return ownkey_ == "" ? 0 : IOM().get( ownkey_ );
}


int IOX::getFrom( ascistream& stream )
{
    ownkey_ = stream.value();
    stream.next();
    return YES;
}


int IOX::putTo( ascostream& stream ) const
{
    stream.stream() << '$';
    stream.put( "ID", ownkey_ );
    return YES;
}
