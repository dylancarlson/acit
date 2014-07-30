/************************************************************************/
/*  SYS.C                                                      91Jul02  */
/*        Sysop function code for Citadel bulletin board system         */
/************************************************************************/

#include <string.h>
#include "ctdl.h"
#include "prot.h"
#include "glob.h"

/************************************************************************/
/*                              Contents                                */
/*                                                                      */
/*  globalverify()      does global sweep to verify any un-verified     */
/*  msgNym()            Aide message nym setting function               */
/*  sysopunlink()       unlinks ambiguous filenames sysop only          */
/*                                                                      */
/************************************************************************/

/*----------------------------------------------------------------------*/
/*      globalverify()  does global sweep to verify any un-verified     */
/*----------------------------------------------------------------------*/
void globalverify(void)
{
    int    logNo, i, yn, tabslot;

    outFlag = OUTOK;

    for (i=0;( (i < cfg.MAXLOGTAB) && (outFlag != OUTSKIP) && !mAbort() );i++)
        if (logTab[i].ltpwhash != 0 && logTab[i].ltnmhash !=0)
        {
            logNo=logTab[i].ltlogSlot;
            getLog(&lBuf, logNo);
            if (lBuf.VERIFIED == TRUE)
            {
                mPrintf("\n %s", lBuf.lbname);
                doCR();
                yn=getYesNo("Verify", 0+3);
                if (yn == 2)
                {
                    SaveAideMess();
                    return;
                }
                if (yn)
                {
                    lBuf.VERIFIED = FALSE;
                    if (strcmpi(logBuf.lbname, lBuf.lbname) == SAMESTRING)
                        logBuf.VERIFIED = FALSE;
                    sprintf(msgBuf->mbtext, "%s verified by %s",
                                           lBuf.lbname, logBuf.lbname );
                    trap(msgBuf->mbtext, T_SYSOP);
                    amPrintf(" %s\n", msgBuf->mbtext);
                }
                else
                if (strcmpi(logBuf.lbname, lBuf.lbname) != SAMESTRING)
                    if (getYesNo("Kill account", 0))
                    {
                        mPrintf( "\n \'%s\' terminated.\n ", lBuf.lbname);
                            /* trap it */
                        sprintf(msgBuf->mbtext,
                            "Un-verified user %s terminated", lBuf.lbname);
                        trap(msgBuf->mbtext, T_SYSOP);
                            /* get log tab slot for person */
                        tabslot = personexists(lBuf.lbname);
                        logTab[tabslot].ltpwhash   = 0;
                        logTab[tabslot].ltinhash   = 0;
                        logTab[tabslot].ltnmhash   = 0;
                        logTab[tabslot].permanent  = 0;
                        lBuf.lbname[0] = '\0';
                        lBuf.lbin[  0] = '\0';
                        lBuf.lbpw[  0] = '\0';
                        lBuf.lbflags.L_INUSE   = FALSE;
                        lBuf.lbflags.PERMANENT = FALSE;
                    }
                putLog(&lBuf, logNo);
            }
        }
    SaveAideMess();
}

/* -------------------------------------------------------------------- */
/*  msgNym()        Aide message nym setting function                   */
/* -------------------------------------------------------------------- */
void msgNym(void)
{
    doCR();
    if (!cfg.msgNym)
    {
        doCR();
        printf(" Message nyms not enabled!");
        doCR();
        return;
    }

    getString("name (SINGLE)", cfg.msg_nym,  LABELSIZE, FALSE, ECHO, "");
    getString("name (PLURAL)", cfg.msgs_nym, LABELSIZE, FALSE, ECHO, "");
    getString("what to do to message", 
               cfg.msg_done, LABELSIZE, FALSE, ECHO, "");

    sprintf(msgBuf->mbtext, "\n Message nym changed by %s to\n "
                            "Single:   %s\n "
                            "Plural:   %s\n "
                            "Verb  :   %s\n ",
                            logBuf.lbname, 
                            cfg.msg_nym, cfg.msgs_nym, cfg.msg_done );
    aideMessage();
    doCR();
}

/************************************************************************/
/*     sysopunlink()   unlinks ambiguous filenames sysop only           */
/************************************************************************/
void sysopunlink(void)
{
    label files;
    int i;

    getString("file(s) to unlink", files, NAMESIZE, FALSE, ECHO, "");

    if(files[0])
    {
        i = ambigUnlink(files, TRUE);
        if(i)
            updateinfo();
        doCR();
        mPrintf("(%d) file(s) unlinked", i);
        doCR();

        sprintf(msgBuf->mbtext, "File(s) %s unlinked in room %s]",
                    files, roomBuf.rbname);

        trap(msgBuf->mbtext, T_SYSOP);
    }
}

