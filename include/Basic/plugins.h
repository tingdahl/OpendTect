#ifndef plugins_h
#define plugins_h
/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Aug 2003
 Contents:	Plugins
 RCS:		$Id: plugins.h,v 1.14 2006-02-13 15:34:24 cvsbert Exp $
________________________________________________________________________

-*/


#include "bufstringset.h"

#ifdef __cpp__
extern "C" {
#endif

#define PI_AUTO_INIT_NONE	0
#define PI_AUTO_INIT_EARLY	1
#define PI_AUTO_INIT_LATE	2

/*!\brief Information about plugin for outside world */

typedef struct {

    const char*	dispname;
    const char*	creator;
    const char*	version;
    const char*	text;

} PluginInfo;

/* C Access. C++ should use PluginManager! */

/*! To be called from program (once for EARLY, once for LATE) */
void LoadAutoPlugins(int argc,char** argv,int inittype);
/*! To be called from program if needed */
int LoadPlugin(const char* libnm);


#ifdef __cpp__
}

/*!\brief Plugin manager - loads plugins: shared libs or DLLs.
 
 For shared libs things to be in any way useful, an init function
 must be called. The name of that function should predictable.
 It is constructed as follows:
 InitxxxPlugin
 where xxx is the name of the plugin file, where:
 libxxx.so -> xxx 
 xxx.dll -> xxx 
 etc.

 The signature is:

 extern "C" {
 const char* InitxxxPlugin(int,char**);
 }

 Optional extras.

 1) If you want the plugin to be loaded automatically at
 startup define:

 extern "C" int GetxxxPluginType(void);

 if not defined, PI_AUTO_INIT_NONE is assumed, which means it will not be loaded
 if not explicitly done so.

 Loading from startup is done from $HOME/.od/plugins/$PLFSUBDIR or $dGB_APPL/...
 Plugins can always be put in these directory (for all) or in a subdirectory
 with the name of the program (e.g. dtectmain).
 From OpendTect V1.0.3 we also support the '.alo' files with the names of
 the plugins (e.g. DipSteer), and the plugins themeselves in the 'libs'
 subdirectory.
 The non-alo plugins will be loaded in alphabetical order, for .alo files the
 order specified in the file. The alo files themselves are handled in
 alphabetical order.

 2) It may be a good idea to define a function:

 extern "C" PluginInfo* GetxxxPluginInfo(void);

 Make sure it returns an object of type PluginManager::Info*. Make sure it
 points to an existing object (static or made with new/malloc);

3) The user of PIM() can decide not to load all of the .alo load libs. After
   setArgs, the getData() list is filled. You can remove entries from this list
   before calling loadAuto().

 */

class PluginManager
{
public:

    void			setArgs(int argc,char** argv);
    void			loadAuto(bool late=true);
    bool			load(const char* libnm);

    struct Data
    {
	enum AutoSource		{ None, UserDir, AppDir, Both };
	static bool		isUserDir( AutoSource src )
	    			{ return src != AppDir && src != None; }

				Data( const char* nm )
				    : name_(nm)
				    , info_(0)
				    , autosource_(None)		{}

	BufferString		name_;
	PluginInfo*		info_;
	AutoSource		autosource_;
    };

    ObjectSet<Data>&		getData()		{ return data_; }
    Data*			findData( const char* nm )
    				{ return fndData( nm ); }
    const Data*			findData( const char* nm ) const
    				{ return fndData( nm ); }
    const Data*			findDataWithDispName(const char*) const;

    bool			isLoaded(const char*) const;
    bool			isPresent(const char*) const;
    const char*			userName(const char*) const;
    				//!< returns without path, 'lib' and extension
    const char*			getFileName(const Data&) const;

    const char*			getAutoDir( bool usr ) const
				{ return usr ? userlibdir_ : applibdir_; }

private:

    				PluginManager();
    friend PluginManager&	PIM();

    int				argc_;
    char**			argv_;
    ObjectSet<Data>		data_;
    static PluginManager*	theinst_;

    BufferString		userdir_;
    BufferString		appdir_;
    BufferString		userlibdir_;
    BufferString		applibdir_;

    bool			isPresent(const char*);
    Data*			fndData(const char*) const;
    void			getDefDirs();
    void			getALOEntries(const char*,bool);
    void			mkALOList();
    bool			loadAutoLib(Data&,bool,bool);

};

inline PluginManager& PIM()
{
    if ( !PluginManager::theinst_ )
	PluginManager::theinst_ = new PluginManager;
    return *PluginManager::theinst_;
}

#endif

#endif
