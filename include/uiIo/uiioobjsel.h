#ifndef uiioobjsel_h
#define uiioobjsel_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: uiioobjsel.h,v 1.13 2001-07-19 09:46:02 bert Exp $
________________________________________________________________________

-*/

#include <uiiosel.h>
#include <uidialog.h>
#include <multiid.h>
class IOObj;
class CtxtIOObj;
class IODirEntryList;
class uiListBox;
class uiGenInput;


/*! \brief Dialog for selection of IOObjs

This class may be subclassed to make selection more specific.

*/

class uiIOObjSelDlg : public uiDialog
{
public:
			uiIOObjSelDlg(uiParent*,const CtxtIOObj&,
				      const char* transl_glob_expr=0);
			~uiIOObjSelDlg();

    const IOObj*	ioObj() const		{ return ioobj; }

    virtual void	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

protected:

    const CtxtIOObj&	ctio;
    IODirEntryList*	entrylist;
    IOObj*		ioobj;

    uiListBox*		listfld;
    uiGenInput*		nmfld;
    uiGroup*		grp;

    bool		acceptOK(CallBacker*);
    void		selChg(CallBacker*);

    virtual bool	createEntry(const char*);
};


/*! \brief UI element for selection of IOObjs

This class may be subclassed to make selection more specific.

*/

class uiIOObjSel : public uiIOSelect
{
public:
			uiIOObjSel(uiParent*,CtxtIOObj&,const char* txt=0,
				   bool wthclear=false,
				   const char* transl_globexpr=0);
			~uiIOObjSel();

    bool		commitInput(bool mknew);

    void		updateInput();	//!< updates from CtxtIOObj
    void		processInput(); //!< Match user typing with existing
					//!< IOObjs, then set item accordingly
    bool		existingTyped() const;
					//!< returns false is typed input is
					//!< not an existing IOObj name
    CtxtIOObj&		ctxtIOObj()		{ return ctio; }

    virtual bool	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

protected:

    CtxtIOObj&		ctio;
    bool		forread;
    BufferString	trglobexpr;

    void		doObjSel(CallBacker*);

    virtual const char*	userNameFromKey(const char*) const;
    virtual void	objSel();

    virtual void	newSelection(uiIOObjSelDlg*)		{}
    virtual uiIOObjSelDlg* mkDlg();
    virtual IOObj*	createEntry(const char*);

};


#endif
