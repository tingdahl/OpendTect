#ifndef uimain_H
#define uimain_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          03/12/1999
 RCS:           $Id: uimain.h,v 1.3 2002-01-18 12:38:12 arend Exp $
________________________________________________________________________

-*/

#include "uigeom.h"
class uiMainWin;
class QApplication;
class uiFont;


class uiMain 
{
public:
			uiMain(int argc,char** argv);
    virtual		~uiMain();

    int			exec();	
    void 		exit(int retcode=0);

    void		setTopLevel(uiMainWin*);
    uiMainWin*		topLevel()			{ return mainobj; }
    void		setFont(const uiFont& font,bool passtochildren);    

    const uiFont*	font(); 

    static uiMain&	theMain();
    static void		setTopLevelCaption( const char* );

    static void		flushX();
    static void		processEvents(int msec=3000);

    static uiSize	desktopSize();

protected:

    static uiMain*	themain;
    uiMainWin*		mainobj;

    static QApplication*  app;
    static const uiFont*  font_;

private:
			uiMain( QApplication* );
};


#endif
