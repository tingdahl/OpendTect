/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		July 2017
________________________________________________________________________

-*/

#include "fileformat.h"
#include "uistrings.h"


// cannot macro-ise these because of the text translations

static File::Format* allfilesfmt_ = 0;
const File::Format& File::Format::allFiles()
{
    if ( !allfilesfmt_ )
	allfilesfmt_ = new File::Format( tr("All files"), "" );
    return *allfilesfmt_;
}

static File::Format* parfilesfmt_ = 0;
const File::Format& File::Format::parFiles()
{
    if ( !parfilesfmt_ )
	parfilesfmt_ = new File::Format( tr("Parameter files"), "par" );
    return *parfilesfmt_;
}

static File::Format* zipfilesfmt_ = 0;
const File::Format& File::Format::zipFiles()
{
    if ( !zipfilesfmt_ )
	zipfilesfmt_ = new File::Format( tr("Zip files"), "zip" );
    return *zipfilesfmt_;
}

static File::Format* textfilesfmt_ = 0;
const File::Format& File::Format::textFiles()
{
    if ( !textfilesfmt_ )
	textfilesfmt_ = new File::Format( tr("Text files"), "txt", "dat" );
    return *textfilesfmt_;
}

static File::Format* shlibwinfilesfmt_ = 0;
static File::Format* shlibmacfilesfmt_ = 0;
static File::Format* shlibothfilesfmt_ = 0;
const File::Format& File::Format::shlibFiles()
{
    if ( !shlibwinfilesfmt_ )
	shlibwinfilesfmt_ = new File::Format( tr("DLL files"), "dll" );
    if ( !shlibmacfilesfmt_ )
	shlibmacfilesfmt_ = new File::Format( tr("Dynamic Libs"), "dylib" );
    if ( !shlibothfilesfmt_ )
	shlibothfilesfmt_ = new File::Format( tr("Shared Libraries"), "so" );
#ifdef __win__
    return *shlibwinfilesfmt_;
#else
# ifdef __mac__
    return *shlibmacfilesfmt_;
# else
    return *shlibothfilesfmt_;
# endif
#endif
}


// tradstr would be either a 'filter', or just an extension

File::Format::Format( const char* tradstr )
{
    BufferString bs( tradstr );
    if ( bs.isEmpty() )
	return;

    char* startptr = bs.getCStr();
    mTrimBlanks( startptr );
    char* ptr = firstOcc( startptr, '(' );
    if ( !ptr )
	ptr = startptr;
    else
    {
	*ptr++ = '\0';
	usrdesc_ = toUiString( startptr );
	if ( !ptr || !*ptr )
	    return;
    }

    startptr = ptr;
    ptr = firstOcc( startptr, ')' );
    if ( ptr )
	*ptr = '\0';
    while ( true )
    {
	mSkipBlanks( startptr );
	if ( !*startptr )
	    break;
	ptr = startptr;
	mSkipNonBlanks( ptr );
	if ( *ptr )
	    *ptr++ = '\0';

	// startptr could now be pointing to a "*.xx" of just an extension
	if ( *startptr == '*' )
	    startptr += 2;
	addExtension( startptr );
	if ( usrdesc_.isEmpty() )
	    usrdesc_ = toUiString( BufferString( startptr, " files" ) );
	startptr = ptr;
    }
}


File::Format::Format( const uiString& ud, const char* ext, const char* ext2,
		      const char* ext3 )
    : usrdesc_(ud)
{
    if ( ext )
	addExtension( ext );
    if ( ext2 )
	addExtension( ext2 );
    if ( ext3 )
	addExtension( ext3 );
}


#define mGetCleanExt() \
    if ( !ext ) \
	ext = ""; \
    while ( *ext && *ext == '.' ) \
	ext++


void File::Format::addExtension( const char* ext )
{
    mGetCleanExt();
    exts_.addIfNew( ext );
}


bool File::Format::hasExtension( const char* ext ) const
{
    mGetCleanExt();
    return exts_.isPresent( ext, CaseInsensitive );
}


const char* File::Format::extension( int idx ) const
{
    if ( !exts_.validIdx(idx) )
	{ pErrMsg("Invalid idx (ext)"); return ""; }
    return exts_.get( idx ).str();
}


BufferString File::Format::getFileFilter() const
{
    BufferString ret( usrdesc_.getFullString(), " (" );
    for ( int iext=0; iext<exts_.size(); iext++ )
    {
	if ( iext > 0 )
	    ret.addSpace();
	const BufferString& ext = exts_.get( iext );
	if ( !ext.isEmpty() )
	    ret.add( "*." ).add( ext );
	else
	{
	    ret.add( "*" );
#ifdef __win__
	    ret.add( " *.*" );
#endif
	}
    }
    ret.add( ')' );
    return ret;
}


File::FormatList::FormatList( const Format& fmt )
{
    addFormat( fmt );
}


File::FormatList::FormatList( const char* trad_str )
{
    BufferString workstr( trad_str );
    if ( workstr.isEmpty() )
	return;
    char* startptr = workstr.getCStr();
    mTrimBlanks( startptr );
    if ( !*startptr )
	return;

    while ( true )
    {
	mSkipBlanks( startptr );
	if ( !*startptr )
	    break;
	char* ptr = firstOcc( startptr, ';' );
	if ( ptr )
	{
	    *ptr = '\0';
	    if ( *ptr++ == ';' )
		*ptr++ = '\0';
	}

	addFormat( startptr );

	if ( !ptr )
	    break;
	startptr = ptr;
    }
}


File::FormatList::FormatList( const uiString& ud, const char* ext,
			      const char* ext2, const char* ext3 )
{
    addFormat( Format(ud,ext,ext2,ext3) );
}


File::FormatList::FormatList( const FormatList& oth )
{
    *this = oth;
}


File::FormatList& File::FormatList::operator =( const FormatList& oth )
{
    if ( this != &oth )
    {
	deepErase( fmts_ );
	deepCopy( fmts_, oth.fmts_ );
    }
    return *this;
}


int File::FormatList::indexOf( const char* ext ) const
{
    for ( int ifmt=0; ifmt<fmts_.size(); ifmt++ )
	if ( fmts_[ifmt]->exts_.isPresent(ext) )
	    return ifmt;
    for ( int ifmt=0; ifmt<fmts_.size(); ifmt++ )
	if ( fmts_[ifmt]->hasExtension(ext) )
	    return ifmt;
    return false;
}


File::Format File::FormatList::format( int ifmt ) const
{
    if ( !fmts_.validIdx(ifmt) )
	{ pErrMsg("Invalid list idx (fmt)"); return Format::allFiles(); }
    return *fmts_[ifmt];
}


uiString File::FormatList::userDesc( int ifmt ) const
{
    if ( !fmts_.validIdx(ifmt) )
	{ pErrMsg("Invalid list idx (udesc)"); return uiString::emptyString(); }
    return fmts_[ifmt]->userDesc();
}


void File::FormatList::addFormat( const uiString& ud, const char* ext )
{
    fmts_ += new Format( ud, ext );
}


void File::FormatList::addFormats( const FormatList& oth )
{
    for ( int ifmt=0; ifmt<oth.fmts_.size(); ifmt++ )
    {
	const Format* fmt = oth.fmts_[ifmt];
	for ( int iext=0; iext<oth.size(); iext++ )
	    if ( isPresent(fmt->extension(iext)) )
		{ fmt = 0; break; }
	if ( fmt )
	    fmts_ += new Format( *fmt );
    }
}


void File::FormatList::setEmpty()
{
    deepErase( fmts_ );
}


void File::FormatList::removeFormat( int idx )
{
    if ( fmts_.validIdx(idx) )
	delete fmts_.removeSingle( idx );
}


BufferString File::FormatList::getFileFilter( int idx ) const
{
    if ( !fmts_.validIdx(idx) )
	{ pErrMsg("Invalid list idx (filefilt)"); return BufferString(); }
    return fmts_[idx]->getFileFilter();
}


BufferString File::FormatList::getFileFilters() const
{
    BufferString ret;
    for ( int ifmt=0; ifmt<fmts_.size(); ifmt++ )
    {
	if ( ifmt > 0 )
	    ret.add( ";; " );
	ret.add( fmts_[ifmt]->getFileFilter() );
    }

    return ret;
}
