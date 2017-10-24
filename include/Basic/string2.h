#pragma once

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		11-4-1994
 Contents:	Extra string functions
________________________________________________________________________
-*/

/* Functions making sure you don't need <string.h>

   (1) Usually OD::String or its subclass BufferString can already do the job
   (2) toString(XX) for all simple types
   (3) toXX from const char* for most used simple types
   (4) Replacements for strlen, strchr and strrstr
   (5) Various construction-, manipulation- and matching functions

 */

#include "basicmod.h"
#include "plftypes.h"
#include "commondefs.h"
#include <ctype.h>
#include <wctype.h>
namespace OD { class String; }

mGlobal(Basic) const char*	toString(od_int32);
mGlobal(Basic) const char*	toString(od_uint32);
mGlobal(Basic) const char*	toString(od_int64);
mGlobal(Basic) const char*	toString(od_uint64);
mGlobal(Basic) const char*	toString(float);
mGlobal(Basic) const char*	toStringLim(float,int maxtxtwdth);
mGlobal(Basic) const char*	toStringPrecise(float);
mGlobal(Basic) const char*	toString(double);
mGlobal(Basic) const char*	toStringLim(double,int maxtxtwdth);
mGlobal(Basic) const char*	toStringPrecise(double);
mGlobal(Basic) const char*	toString(short);
mGlobal(Basic) const char*	toString(unsigned short);
mGlobal(Basic) const char*	toString(const char*);
mGlobal(Basic) const char*	toString(unsigned char);
mGlobal(Basic) const char*	toString(signed char);
mGlobal(Basic) const char*	toString(bool);
mGlobal(Basic) const char*	toString(const OD::String&);

mGlobal(Basic) bool		yesNoFromString(const char*);

mGlobal(Basic) int		toInt(const char*,int defval=0);
mGlobal(Basic) od_int64		toInt64(const char* s,od_int64 defval=0);
mGlobal(Basic) float		toFloat(const char*,float defval=0.f);
mGlobal(Basic) double		toDouble(const char*,double defval=0.);
inline bool			toBool( const char* s, bool defval=true )
				{ return s && *s ? yesNoFromString(s) : defval;}


/*!\brief Advances given pointer to first non-whitespace */
#define mSkipBlanks(ptr) \
    { if ( ptr ) { while ( *(ptr) && iswspace(*(ptr)) ) (ptr)++; } }

/*!\brief Advances given pointer to first whitespace  */
#define mSkipNonBlanks(ptr) \
    { if ( ptr ) { while ( *(ptr) && !iswspace(*(ptr)) ) (ptr)++; } }

/*!\brief Advances to first whitespace and removes trailing whitespace */
#define mTrimBlanks(ptr) \
    { mSkipBlanks(ptr); removeTrailingBlanks(ptr); }


mGlobal(Basic) bool caseInsensitiveEqual(const char*,const char*,
					 int match_nrchars=-1);
	/*!< stricmp with option to compare part, default is all */
mGlobal(Basic) bool stringStartsWith(const char* startstring,
				const char* maybebigger);
	/*!< checks whether a string is the start of another string. */
mGlobal(Basic) bool stringStartsWithCI(const char*,const char*);
	/*!< is a case insensitive version of matchString */
mGlobal(Basic) bool stringEndsWith(const char* endstring,
				   const char* maybebigger);
	/*!< checks whether a string is the end of another string. */
mGlobal(Basic) bool stringEndsWithCI(const char*,const char*);
	/*!< is a case insensitive version of stringEndsWith */

mGlobal(Basic) void removeTrailingBlanks(char*);
	/*!< bluntly puts a '\0' on trailing white space. */
mGlobal(Basic) void cleanupString(char*,bool,bool,bool);
	/*!< replaces whitespace, file separators, dots -> underscores. */
mGlobal(Basic) bool isNumberString(const char*,bool int_only=false);
	/*!< tells whether a string holds a parseable number */
mGlobal(Basic) bool isAlphaNumString(const char*,bool allowspace = false);
	/*!< tells whether has printable characters only. */

mGlobal(Basic) const char* getNextWord(const char*,char*);
	/*!< fills a buffer with the next word (delimited by whitespace).
	     It returns a ptr just after the word. */

	/*!> Replacement for strlen */
mGlobal(Basic) int strLength(const char*);
	/*!> Replacement for strchr: const version */
mGlobal(Basic) const char* firstOcc(const char*,char);
mGlobal(Basic) const char* firstOcc(const char*,int);
	/*!> Replacement for strchr: non-const version */
mGlobal(Basic) char* firstOcc(char*,char);
mGlobal(Basic) char* firstOcc(char*,int);
	/*!> Replacement for strrchr: const version */
mGlobal(Basic) const char* lastOcc(const char*,char);
mGlobal(Basic) const char* lastOcc(const char*,int);
	/*!> Replacement for strrchr: non-const version */
mGlobal(Basic) char* lastOcc(char*,char);
mGlobal(Basic) char* lastOcc(char*,int);
	/*!> Replacement for strstr: const version */
mGlobal(Basic) const char* firstOcc(const char*,const char*);
	/*!> Replacement for strstr: non-const version */
mGlobal(Basic) char* firstOcc(char*,const char*);
	/*!> Replacement for (imaginary) strrstr: const version */
mGlobal(Basic) const char* lastOcc(const char*,const char*);
	/*!> Replacement for (imaginary) strrstr: non-const version */
mGlobal(Basic) char* lastOcc(char*,const char*);

mGlobal(Basic) const char* getYesNoString(bool);
	/*!< returns ptr to static buffer with "yes" or "No". */
mGlobal(Basic) const char* getDistUnitString(bool isfeet,bool withparentheses);
	/*!< returns ptr to static buffer with "m" or "ft" */
mGlobal(Basic) const char* getVelUnitString(bool isfeet,bool withparentheses);
	/*!< returns ptr to static buffer with "m/s" or "ft/s" */

mGlobal(Basic) const char* getRankPostFix(int);
	/*!< returns "th" or "st" or "nd" or "rd"; like in 1st, 2nd, 3rd etc. */
mGlobal(Basic) const char* getBytesString(od_uint64);
	/*!< returns a nicely, readable size, in bytes, KB, MB, GB, or TB */
mGlobal(Basic) const char* getLimitedDisplayString(const char*,int nrchars,
					    bool trimright);
	/*!< returns a string for display, never larger than nrchars */

mGlobal(Basic) int getIndexInStringArrCI(const char*,const char* const* arr,
				  int startnr=0,int nr_chars_to_match=0,
				  int notfoundidx=-1);
	/*!< Finds a string in string array, case insensitive */

mGlobal(Basic) const char* getAreaString(float m2,bool parensonunit,
					 char* str=0);
	/*!<Returns a string with an area and its unit, depending on survey and
	    area size, unit is ft^2, m^2, km^2 or mile^2. */

mGlobal(Basic) char* truncateString(char* str, int maxlen );
	/*!<If str is longer than maxlen-4, string will be truncated and a
	 * " ..." will be appended". */
