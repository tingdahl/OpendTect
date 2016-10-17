/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : March 1994
 * FUNCTION : general utilities
-*/


#include "genc.h"
#include "oddirs.h"
#include "envvars.h"
#include "winutils.h"
#include "debug.h"
#include "file.h"
#include "filepath.h"
#include "settings.h"
#include "survinfo.h"
#include "od_istream.h"
#include <iostream>

#ifdef __msvc__
# include <direct.h>
#endif
#ifndef __win__
# define sDirSep	"/"
static const char* lostinspace = "/tmp";
#else
# define sDirSep	"\\"
static const char* lostinspace = "C:\\";

#endif

#ifdef __mac__
# include <CoreServices/CoreServices.h>
#endif

#define mPrDebug(fn,val) od_debug_message( BufferString(fn,": ",val) );



const char* GetLastSurveyFileName()
{
    mDeclStaticString( fnm );

    if ( fnm.isEmpty() )
    {
	File::Path fp( GetSettingsDir(), "survey" );
	const char* ptr = GetSoftwareUser();
	if ( ptr )
	    fp.setExtension( ptr );


	fnm = fp.fullPath();

	if ( od_debug_isOn(DBG_SETTINGS) )
	    mPrDebug( "LastSurveyFileName", fnm.buf() );
    }

    return fnm.str();
}


/*-> implementing oddirs.h */

/* 'survey data' scope */

extern "C" { mGlobal(Basic) void SetBaseDataDir(const char*); }
mExternC(Basic) void SetBaseDataDir( const char* dirnm )
{
#ifdef __win__
    const BufferString windirnm( File::Path(dirnm).fullPath(File::Path::Windows) );
    SetEnvVar( "DTECT_WINDATA", windirnm );
    if ( GetOSEnvVar( "DTECT_DATA" ) )
	SetEnvVar( "DTECT_DATA", windirnm );
#else
    SetEnvVar( "DTECT_DATA", dirnm );
#endif
}

mExternC(Basic) const char* GetBaseDataDir()
{
    BufferString dir;
#ifdef __win__

    dir = GetEnvVar( "DTECT_WINDATA" );
    if ( dir.isEmpty() )
    {
	dir = getCleanWinPath( GetEnvVar("DTECT_DATA") );
	if ( dir.isEmpty() )
	    dir = getCleanWinPath( GetSettingsDataDir() );

	if ( !dir.isEmpty() )
	    SetEnvVar( "DTECT_WINDATA", dir );
    }

#else

    dir = GetEnvVar( "DTECT_DATA" );
    if ( dir.isEmpty() )
    {
	dir = GetSettingsDataDir();
	if ( !dir.isEmpty() )
	    SetEnvVar( "DTECT_DATA", dir );
    }

#endif

    if ( dir.isEmpty() )
	return 0;

    mDeclStaticString( ret );
    ret = dir;
    return ret.buf();
}


mExternC(Basic) const char* GetDataDir()
{
    const char* basedir = GetBaseDataDir();
    if ( !basedir || !*basedir )
	return 0;

    const char* survnm = SI().getDirName();
    if ( !survnm || !*survnm )
	survnm = "_no_current_survey_";

    mDeclStaticString( ret );
    ret = File::Path( basedir, survnm ).fullPath();
    if ( od_debug_isOn(DBG_SETTINGS) )
	mPrDebug( "GetDataDir", ret );
    return ret.buf();
}


mExternC(Basic) const char* GetProcFileName( const char* fname )
{
    mDeclStaticString( ret );
    ret = File::Path( GetDataDir(), "Proc", fname ).fullPath();
    return ret.buf();
}


mExternC(Basic) const char* GetScriptsDir( const char* subdir )
{
    mDeclStaticString( ret );
    const char* envval = GetEnvVar( "DTECT_SCRIPTS_DIR" );
    ret = envval && *envval ? envval : GetProcFileName( subdir );
    return ret.buf();
}


mExternC(Basic) const char* GetSoftwareDir( bool acceptnone )
{
    mDeclStaticString( res );

    if ( res.isEmpty() )
    {
	//Find the relinfo directory, and set sw dir to its parent
	const File::Path filepath = GetFullExecutablePath();
	for ( int idx=filepath.nrLevels()-1; idx>=0; idx-- )
	{
	    const char* relinfostr = "relinfo";
#ifdef __mac__
	    const File::Path datapath( filepath.dirUpTo(idx).buf(), "Resources",
				     relinfostr );
#else
	    const File::Path datapath( filepath.dirUpTo(idx).buf(), relinfostr );
#endif
	    if ( File::isDirectory( datapath.fullPath()) )
	    {
		res = filepath.dirUpTo(idx);
		break;
	    }
	}

	if ( res.isEmpty() )
	{
	    if ( acceptnone )
		return 0;

	    std::cerr << "Cannot determine OpendTect location ..." << std::endl;
	    ExitProgram( 1 );
	}

	if ( od_debug_isOn(DBG_SETTINGS) )
	    mPrDebug( "GetSoftwareDir", res );
    }

    return res.buf();
}


mExternC(Basic) const char* GetApplSetupDir()
{
    mDeclStaticString( bs );
    if ( bs.isEmpty() )
    {

#ifdef __win__
	bs = GetEnvVar( "DTECT_WINAPPL_SETUP" );
#endif
	if ( bs.isEmpty() )
	    bs = GetEnvVar( "DTECT_APPL_SETUP" );

    }
    return bs.str();
}

static const char* sData = "data";

static const char* GetSoftwareDataDir( bool acceptnone )
{
    File::Path basedir = GetSoftwareDir( acceptnone );
    if ( basedir.isEmpty() )
	return 0;

#ifdef __mac__
    basedir.add( "Resources" );
#endif

    basedir.add( sData );

    mDeclStaticString( dirnm );
    dirnm = basedir.fullPath();
    return dirnm.str();
}


mExternC(Basic) const char* GetSetupDataFileDir( ODSetupLocType lt,
						 bool acceptnone )
{

    if ( lt>ODSetupLoc_ApplSetupPref )
    {
	const char* res = GetSoftwareDataDir( acceptnone ||
					lt==ODSetupLoc_ApplSetupPref );
	if ( res || lt==ODSetupLoc_SWDirOnly )
	    return res;
    }

    File::Path basedir = GetApplSetupDir();

    if ( basedir.isEmpty() )
    {
	if ( lt==ODSetupLoc_ApplSetupOnly )
	    return 0;

	return GetSoftwareDataDir( acceptnone );
    }

    basedir.add( sData );
    mDeclStaticString( dirnm );
    dirnm = basedir.fullPath();
    return dirnm.buf();
}


mExternC(Basic) const char* GetSetupDataFileName( ODSetupLocType lt,
				const char* fnm, bool acceptnone )
{
    mDeclStaticString( filenm );

    if ( lt == ODSetupLoc_SWDirOnly )
    {
	filenm = File::Path(GetSetupDataFileDir(lt,acceptnone),fnm).fullPath();
	return filenm.buf();
    }

    const char* appldir =
		GetSetupDataFileDir(ODSetupLoc_ApplSetupOnly,acceptnone);
    if ( !appldir )
	return lt == ODSetupLoc_ApplSetupOnly ? 0
	     : GetSetupDataFileName(ODSetupLoc_SWDirOnly,fnm,acceptnone);

    filenm = File::Path(GetSetupDataFileDir(lt,acceptnone),fnm).fullPath();

    if ( (lt == ODSetupLoc_ApplSetupPref || lt == ODSetupLoc_SWDirPref)
	&& !File::exists(filenm) )
    {
	/* try 'other' file */
	GetSetupDataFileName( lt == ODSetupLoc_ApplSetupPref
		? ODSetupLoc_SWDirOnly : ODSetupLoc_ApplSetupOnly, fnm,
				acceptnone );
	if ( File::exists(filenm) )
	    return filenm.buf();

	/* 'other' file also doesn't exist: revert */
	GetSetupDataFileName( lt == ODSetupLoc_ApplSetupPref
		? ODSetupLoc_ApplSetupOnly : ODSetupLoc_SWDirOnly, fnm,
				acceptnone );
    }

    return filenm.buf();
}


mExternC(Basic) const char* GetDocFileDir( const char* filedir )
{
    mDeclStaticString( dirnm );
    if ( dirnm.isEmpty() )
    {
#ifdef __mac__
	dirnm = File::Path(GetSoftwareDir(0),"Resources","doc",
			 filedir).fullPath();
#else
	dirnm = File::Path(GetSoftwareDir(0),"doc",filedir).fullPath();
#endif
    }

    return dirnm;
}


mExternC(Basic) const char* GetPlfSubDir()
{
    return __plfsubdir__;
}


mExternC(Basic) const char* GetExecPlfDir()
{
    mDeclStaticString( res );
    if ( res.isEmpty() )
	res = File::Path( GetFullExecutablePath() ).pathOnly();
    return res.buf();
}


mExternC(Basic) const char* GetLibPlfDir()
{
    mDeclStaticString( res );
    if ( res.isEmpty() )
#ifdef __mac__
        res = File::Path(GetSoftwareDir(0),"Frameworks").fullPath();
#else
        res = File::Path( GetFullExecutablePath() ).pathOnly();
#endif
    return res.buf();
}


mExternC(Basic) const char* GetScriptDir()
{
    mDeclStaticString( res );
    if ( res.isEmpty() )
#ifdef __mac__
    res = File::Path( GetFullExecutablePath() ).pathOnly();
#else
    res = File::Path( GetSoftwareDir(0),"bin" ).fullPath();
#endif
    return res.buf();
}


static const char* gtExecScript( const char* basedir, int remote )
{
    mDeclStaticString( scriptnm );
    scriptnm = File::Path(basedir,"bin","od_exec").fullPath();
    if ( remote ) scriptnm.add( "_rmt" );
    return scriptnm;
}


mExternC(Basic) const char* GetExecScript( int remote )
{

#ifdef __msvc__
    return "";
#else

    const char* basedir = GetApplSetupDir();
    const char* fnm = 0;
    if ( basedir )
	fnm = gtExecScript( basedir, remote );

    if ( !fnm || !File::exists(fnm) )
	fnm = gtExecScript( GetSoftwareDir(0), remote );

    mDeclStaticString( progname );
    progname.set( "'" ).add( fnm ).add( "' " );
    return progname.buf();
#endif
}


mExternC(Basic) const char* GetSoftwareUser()
{
    mDeclStaticString( bs );
    if ( bs.isEmpty() )
    {
	const char* envval = 0;
#ifdef __win__
	envval = GetEnvVar( "DTECT_WINUSER" );
#endif
	if ( !envval || !*envval )
	    envval = GetEnvVar( "DTECT_USER" );
	if ( od_debug_isOn(DBG_SETTINGS) )
	    mPrDebug( "GetSoftwareUser", envval ? envval : "<None>" );

	bs = envval;
    }

    return bs.str();
}


mExternC(Basic) const char* GetUserNm()
{
#ifdef __win__
    mDefineStaticLocalObject( char, usernm, [256] );
    DWORD len = 256;
    GetUserName( usernm, &len );
    return usernm;
#else
    mDeclStaticString( ret );
    ret = GetEnvVar( "USER" );
    return ret.isEmpty() ? 0 : ret.buf();
#endif
}


static void getHomeDir( BufferString& homedir )
{
    const char* dir;

#ifndef __win__

    dir = GetEnvVar( "DTECT_HOME" );
    if ( !dir ) dir = GetEnvVar( "HOME" );

#else

    dir = GetEnvVar( "DTECT_WINHOME" );
    if ( !dir ) dir = getCleanWinPath( GetEnvVar("DTECT_HOME") );
				// should always at least be set
    if ( !dir ) dir = getCleanWinPath( GetEnvVar("HOME") );

    if ( !dir && GetEnvVar("HOMEDRIVE") && GetEnvVar("HOMEPATH") )
    {
	/* This may be not be trustworthy */
	homedir.set( GetEnvVar("HOMEDRIVE") ).add( GetEnvVar("HOMEPATH") );
	if ( !homedir.isEmpty() && !caseInsensitiveEqual(homedir.buf(),"c:\\",0)
	  && File::isDirectory(homedir) )
	    dir = homedir.buf();
    }

    if ( !dir && od_debug_isOn(DBG_SETTINGS) )
	od_debug_message( "Problem: No DTECT_WINHOME, DTECT_HOME "
			  "or HOMEDRIVE/HOMEPATH. Result may be bad." );

    if ( !dir ) dir = getCleanWinPath( GetEnvVar("HOME") );
    if ( !dir ) dir = GetEnvVar( "USERPROFILE" ); // set by OS
    if ( !dir ) dir = GetEnvVar( "APPDATA" );     // set by OS -- but is hidden
    if ( !dir ) dir = GetEnvVar( "DTECT_USERPROFILE_DIR" );// set by init script
    if ( !dir ) // Last resort. Is known to cause problems when used
		// during initialisation of statics. (0xc0000005)
	dir = GetSpecialFolderLocation( CSIDL_PROFILE ); // "User profile"

#endif

    if ( !dir )
    {
	dir = lostinspace;
	od_debug_message( "Serious problem: Cannot find any good value for "
			  "'Personal directory'. Using:\n" );
	od_debug_message( dir );
    }

    if ( dir != homedir.buf() )
	homedir = dir;

#ifdef __win__
    if ( !GetEnvVar("DTECT_WINHOME") )
	SetEnvVar( "DTECT_WINHOME", homedir.buf() );
    homedir.replace( '\r', '\0' );
#endif
}


mExternC(Basic) const char* GetBinSubDir()
{
#ifndef __debug__
# ifdef __hassymbols__
    return "RelWithDebInfo";
# else
    return "Release";
# endif
#else
    return "Debug";
#endif
}

mExternC(Basic) const char* GetPersonalDir()
{
    mDeclStaticString( dirnm );

    if ( dirnm.isEmpty() )
    {
	const char* ptr = GetEnvVar( "DTECT_PERSONAL_DIR" );
	if ( ptr )
	    dirnm = ptr;
	else
	    getHomeDir( dirnm );

	if ( od_debug_isOn(DBG_SETTINGS) )
	    mPrDebug( "GetPersonalDir", dirnm );
    }

    return dirnm.buf();
}


mExternC(Basic) const char* GetSettingsDir()
{
    mDeclStaticString( dirnm );

    if ( dirnm.isEmpty() )
    {
	const char* ptr = 0;
#ifdef __win__
	ptr = GetEnvVar( "DTECT_WINSETTINGS" );
	if( !ptr ) ptr = getCleanWinPath( GetEnvVar("DTECT_SETTINGS") );
#else
	ptr = GetEnvVar( "DTECT_SETTINGS" );
#endif

	if ( ptr )
	    dirnm = ptr;
	else
	{
	    getHomeDir( dirnm );
	    dirnm = File::Path( dirnm, ".od" ).fullPath();
	}

	if ( !File::isDirectory(dirnm) )
	{
	    if ( File::exists(dirnm) )
		File::remove( dirnm );
	    if ( !File::createDir(dirnm) )
	    {
		std::cerr << "Fatal: Cannot create '.od' directory in home "
		    "directory:\n" << dirnm.buf() << std::endl;
		ExitProgram( 1 );
	    }
	    if ( od_debug_isOn(DBG_SETTINGS) )
		mPrDebug( "Had to create SettingsDir", dirnm );
	}

	if ( od_debug_isOn(DBG_SETTINGS) )
	    mPrDebug( "GetSettingsDir", dirnm );
    }

    return dirnm.buf();
}


mExternC(Basic) const char* GetSettingsFileName( const char* fnm )
{
    mDeclStaticString( ret );
    ret = File::Path( GetSettingsDir(), fnm ).fullPath();
    return ret;
}
