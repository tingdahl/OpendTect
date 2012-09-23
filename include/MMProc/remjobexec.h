#ifndef remjobexec_h
#define remjobexec_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          August 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include "mmprocmod.h"
#include "callback.h"
#include "gendefs.h"

class BufferString;
class IOPar;
class TcpSocket;

mClass(MMProc) RemoteJobExec : public CallBacker
{
public:
			RemoteJobExec(const char*,const int);
			~RemoteJobExec();
    bool		launchProc() const;
    void		addPar(const IOPar&);

protected:
    void		ckeckConnection();
    void		connectedCB(CallBacker*);
    void		uiErrorMsg(const char*);

    TcpSocket&		socket_;
    IOPar&		par_;
    bool		isconnected_;
    const char*		host_;
};

#endif

