#ifndef segyhdr_h
#define segyhdr_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		10-5-1995
 RCS:		$Id: segyhdr.h,v 1.1 2001-02-13 17:16:09 bert Exp $
________________________________________________________________________

-*/
 
#include <seisinfo.h>
#include <segythdef.h>
class ostream;

#define SegyTxtHeaderLength		3200
#define SegyBinHeaderLength		400
#define SegyBinHeaderUnassShorts	170
#define SegyTracHeaderLength		240


class SegyTxtHeader
{
public:
    		SegyTxtHeader();
 
    const char* getClient() const;
    const char* getCompany() const;
    void	setClient(const char*);
    void	setCompany(const char*);
    void	setAuxInfo(const char*);
    void	setPosInfo(int,int,int,int);

    void	putAt(int,int,int,const char*);
    void	getFrom(int,int,int,char*) const;
    void	print(ostream&) const;
 
    void        setAscii();
    void        setEbcdic();
 
    unsigned char txt[SegyTxtHeaderLength];

};


class SegyBinHeader
{
public:

		SegyBinHeader();
    void	getFrom(const void*);
    void	putTo(void*) const;
    void	print(ostream&) const;
    int		bytesPerSample() const
		{ return formatBytes( format ); }
    static int	formatBytes( int frmt )
		{
		    if ( frmt == 3 ) return 2;
		    return frmt == 5 ? 1 : 4;
		}

    bool	needswap;

    int jobid;      /* job identification number */
    int lino;       /* line number (only one line per reel) */
    int reno;       /* reel number */
    short ntrpr;    /* number of data traces per record */
    short nart;     /* number of auxiliary traces per record */
    short hdt;      /* sample interval in micro secs for this reel */
    short dto;      /* same for original field recording */
    short hns;      /* number of samples per trace for this reel */
    short nso;      /* same for original field recording */
    short format;   /* data sample format code:
                            1 = floating point (4 bytes)
                            2 = fixed point (4 bytes)
                            3 = fixed point (2 bytes)
                            4 = fixed point w/gain code (4 bytes)
		       + common extensions (not strict SEG-Y):
                            5 = signed char (1 byte)
                            6 = IEEE float (4 bytes)
		    */
    short fold, tsort, vscode, hsfs, hsfe, hslen, hstyp, schn, hstas,
		hstae, htatyp, hcorr, bgrcv, rcvm;

    short mfeet;    /* measurement system code: 1 = meters 2 = feet */

    short polyt, vpol;
    short hunass[SegyBinHeaderUnassShorts];
};


class SegyTraceheader
{
public:

			SegyTraceheader( unsigned char* b,
					 SegyTraceheaderDef& hd )
			: buf(b)
			, hdef(hd)
			, needswap(false), seqnr(1)		{}

    void		print(ostream&) const;

    unsigned short	nrSamples() const;
    void		putNrSamples(unsigned short);

    void		use(const SeisTrcInfo&);
    void		fill(SeisTrcInfo&) const;

    float		postScale(int numbfmt) const;

    unsigned char*	buf;
    bool		needswap;
    SegyTraceheaderDef&	hdef;
    int			seqnr;

};


#endif
