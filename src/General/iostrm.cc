/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 25-10-1994
-*/

static const char* rcsID = "$Id: iostrm.cc,v 1.6 2001-03-30 08:52:58 bert Exp $";

#include "iostrm.h"
#include "iolink.h"
#include "ascstream.h"
#include "strmprov.h"
#include "separstr.h"
#include "string2.h"

DefineConcreteClassDef(IOStream,"Stream");


IOStream::IOStream( const char* nm, const char* uid, bool mkdef )
	: IOObject(nm,uid)
	, readcmd(&fname)
	, writecmd(0)
	, blocksize(0)
	, skipfiles(0)
	, rew(NO)
	, padzeros(0)
	, fnrs(0,0,1)
	, type_(StreamConn::File)
	, curfnr(0)
	, nrretries(0)
	, retrydelay(0)
{
    connclassdef_ = &StreamConn::classdef;
    if ( mkdef ) genFileName();
}


IOStream::~IOStream()
{
    delete writecmd;
}


const ClassDef& IOStream::connType() const
{
    return StreamConn::classdef;
}


bool IOStream::bad() const
{
    return type_ == StreamConn::Command || *(const char*)fname ? NO : YES;
}


void IOStream::copyFrom( const IOObj* obj )
{
    if ( !obj ) return;
    if ( obj->isLink() ) obj = ((IOLink*)obj)->link();

    IOObj::copyFrom(obj);
    if ( obj->hasClass(IOStream::classid) )
    {
	const IOStream* iosobj = (const IOStream*)obj;
	hostname = iosobj->hostname;
	padzeros = iosobj->padzeros;
	fname = iosobj->fname;
	writecmd = iosobj->writecmd ? new FileNameString(*iosobj->writecmd) : 0;
	type_ = iosobj->type_;
	blocksize = iosobj->blocksize;
	skipfiles = iosobj->skipfiles;
	rew = iosobj->rew;
	fnrs = iosobj->fnrs;
	curfnr = iosobj->curfnr;
	nrretries = iosobj->nrretries;
	retrydelay = iosobj->retrydelay;
    }
}


const char* IOStream::fullUserExpr( bool forread ) const
{
    StreamProvider* sp = streamProvider( forread );
    if ( !sp ) return "<bad>";

    static char* ret = 0;
    delete [] ret;

    const char* s = sp->fullName();
    ret = new char [strlen(s)+1];
    strcpy( ret, s );

    delete sp;
    return ret;
}


bool IOStream::implExists( bool fr ) const
{
    StreamProvider* sp = streamProvider( fr );
    bool ret = sp && sp->exists( fr );
    delete sp;
    return ret;
}


bool IOStream::implRemovable() const
{
    if ( type_ != StreamConn::File ) return NO;
    return implExists( NO );
}


bool IOStream::implRemove() const
{
    if ( type_ != StreamConn::File ) return NO;

    StreamProvider* sp = streamProvider( YES );
    bool ret = sp && sp->remove();
    delete sp;
    return ret;
}


Conn* IOStream::getConn( Conn::State rw ) const
{
    bool fr = rw == Conn::Read;
    if ( !validNr() ) return 0;

    StreamProvider* sp = streamProvider( fr );
    if ( !sp ) return 0;

    if ( type_ == StreamConn::Device )
    {
	sp->setBlockSize( blocksize );
	if ( rew ) sp->rewind();
	if ( skipfiles ) sp->skipFiles( skipfiles );
    }
    else if ( type_ == StreamConn::File && fname == "?" )
    {
	((IOStream*)this)->genFileName();
	delete sp;
	sp = streamProvider( fr );
    }

    StreamConn*	s = 0;
    if ( fr )	s = new StreamConn( sp->makeIStream() );
    else	s = new StreamConn( sp->makeOStream() );
    if ( s )
    {
	s->ioobj = (IOObj*)this;
	s->setNrRetries( nrretries );
	s->setRetryDelay( retrydelay );
    }

    delete sp;
    return s;
}


void IOStream::genFileName()
{
    fname = name();
    cleanupString( fname.buf(), NO, YES, YES );
    if ( *(const char*)extension )
    {
        fname += ".";
        fname += extension;
    }
}


void IOStream::setReader( const char* str )
{
    if ( !str ) return;
    type_ = StreamConn::Command;
    fname = str;
}


void IOStream::setWriter( const char* str )
{
    if ( !str ) return;
    type_ = StreamConn::Command;
    if ( !writecmd ) writecmd = new FileNameString( str );
    else if ( *writecmd == str ) return;
    *writecmd = str;
}


void IOStream::setFileName( const char* str )
{
    if ( !str ) return;
    type_ = StreamConn::File;
    fname = str;
}


const char* IOStream::devName() const
{
    const char* retptr = fname;
    return matchString( "/dev/", retptr ) ? retptr + 5 : retptr;
}


void IOStream::setDevName( const char* str )
{
    if ( !str ) return;
    type_ = StreamConn::Device;
    fname = "/dev/";
    if ( matchString("/dev/",str) ) fname = str;
    else			    fname += str;
}


int IOStream::getFrom( ascistream& stream )
{
    const char* kw = stream.keyWord() + 1;
    if ( !strcmp(kw,"Hostname") )
    {
	hostname = stream.value();
	stream.next();
    }
    if ( !strcmp(kw,"Extension") )
    {
	extension = stream.value();
	stream.next();
    }
    if ( !strcmp(kw,"Retry") )
    {
	FileMultiString fms( stream.value() );
	nrretries = atoi(fms[0]);
	retrydelay = atoi(fms[1]);
	stream.next();
    }
    if ( !strcmp(kw,"Multi") )
    {
	FileMultiString fms( stream.value() );
	fnrs.start = atoi(fms[0]);
	fnrs.stop = atoi(fms[1]);
	fnrs.step = atoi(fms[2]); if ( fnrs.step == 0 ) fnrs.step = 1;
	if ( ( fnrs.start < fnrs.stop && fnrs.step < 0 )
	  || ( fnrs.stop < fnrs.start && fnrs.step > 0 ) )
	    Swap( fnrs.start, fnrs.stop );
	padzeros = atoi(fms[3]);
	curfnr = fnrs.start;
	stream.next();
    }

    fname = stream.value();
    if ( !strcmp(kw,"Name") )
	type_ = StreamConn::File;
    else if ( !strcmp(kw,"Reader") )
    {
	type_ = StreamConn::Command;
	stream.next();
	if ( !writecmd ) writecmd = new FileNameString( stream.value() );
	else *writecmd = stream.value();
    }
    else if ( !strcmp("Device", kw) )
	return getDev(stream);
    else
	return NO;

    stream.next();
    return YES;
}


bool IOStream::directNumberMultiConn() const
{
    if ( !isMulti() ) return NO;
    return strchr(fname,'*') || (writecmd && strchr(*writecmd,'*'));
}


bool IOStream::getDev( ascistream& stream )
{
    const char* kw = stream.keyWord() + 1;
    type_ = StreamConn::Device;
    while ( !atEndOfSection(stream.next()) )
    {
	if ( *stream.keyWord() == '#' ) break;
	if ( !strcmp(kw,"Blocksize") )
	    blocksize = atoi(stream.value());
	else if ( !strcmp(kw,"Fileskips") )
	    skipfiles = atoi(stream.value());
	else if ( !strcmp(kw,"Rewind") )
	    rew = yesNoFromString(stream.value());
    }
    return YES;
}


int IOStream::putTo( ascostream& stream ) const
{
    if ( hostname[0] )
	stream.put( "$Hostname", hostname );
    if ( extension[0] )
	stream.put( "$Extension", extension );
    if ( nrretries )
    {
	FileMultiString fms;
	fms += nrretries;
	fms += retrydelay;
	stream.put( "$Retry", fms );
    }
    if ( isMulti() )
    {
	FileMultiString fms;
	fms += fnrs.start;
	fms += fnrs.stop;
	fms += fnrs.step;
	fms += padzeros;
	stream.put( "$Multi", fms );
    }

    switch( type_ )
    {
    case StreamConn::File:
	stream.put( "$Name", fname );
	return YES;
    case StreamConn::Command:
	stream.put( "$Reader", fname );
	stream.put( "$Writer", writecmd ? *writecmd : fname );
	return YES;
    case StreamConn::Device:
	stream.put( "$Device", fname );
	if ( blocksize )
	    stream.put( "$Blocksize", blocksize );
	if ( skipfiles )
	    stream.put( "$Fileskips", skipfiles );
	if ( rew )
	    stream.put( "$Rewind", getYesNoString(rew) );
	return YES;
    }

    return NO;
}


StreamProvider* IOStream::streamProvider( bool fr ) const
{
    FileNameString nm( type_ == StreamConn::Command && !fr
			? writer() : (const char*)fname );

    bool doins = isMulti() && (strchr(nm,'*') || strchr(nm,'%'));
    if ( doins )
    {
	char numb[80], numbstr[80]; numbstr[0] = '\0';
	sprintf( numb, "%d", curfnr );
	if ( padzeros )
	{
	    int len = strlen( numb );
	    for ( int idx=len; idx<padzeros; idx++ )
		strcat( numbstr, "0" );
	}
	strcat( numbstr, numb );
	
	FileNameString inp = nm;
	char* ptr = inp.buf();
	nm = "";
	char wc = directNumberMultiConn() ? '*' : '%';
	while ( 1 )
	{
	    char* wcptr = strchr( ptr, wc );
	    if ( !wcptr ) break;
	    *wcptr = '\0';
	    nm += ptr;
	    nm += numbstr;
	    ptr = wcptr + 1;
	}
	nm += ptr;
    }

    StreamProvider* sp = new StreamProvider( hostname, nm, type_ );
    if ( sp->bad() )
    {
	delete sp;
	return 0;
    }
    if ( !hostname[0] && type_ == StreamConn::File )
	sp->addPathIfNecessary( dirName() );
    if ( blocksize ) sp->setBlockSize( blocksize );

    return sp;
}
