#ifndef strmdata_H
#define strmdata_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		3-4-1996
 Contents:	Data on any stream
 RCS:		$Id: strmdata.h,v 1.8 2003-03-19 16:21:59 bert Exp $
________________________________________________________________________

-*/
 
#include "gendefs.h"
#include <stdio.h>
#include <iosfwd>


/*!\brief holds data to use and close an iostream.

Usualyy created by StreamProvider.
Need to find out what to do with the pipe in windows.

*/

class StreamData
{
public:

		StreamData() : fnm(0)		{ initStrms(); }
		~StreamData()			{ delete [] fnm; }
		StreamData( const StreamData& sd )
		: fnm(0)			{ copyFrom( sd ); }
    StreamData&	operator =(const StreamData&);
    void	transferTo(StreamData&);	//!< retains fileName()

    void	close();
    bool	usable() const;

    void	setFileName(const char*);
    const char*	fileName() const		{ return fnm; }
    						//!< Beware: may be NULL

    istream*	istrm;
    ostream*	ostrm;
    FILE*	fp;
    bool	ispipe;

protected:

    char*	fnm;
    void	copyFrom(const StreamData&);

private:

    inline void	initStrms() { fp = 0; istrm = 0; ostrm = 0; ispipe = false; }

};


#endif
