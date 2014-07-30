/* -------------------------------------------------------------------- */
/*  LOG.C                         ACit                         91Sep30  */
/*                           Local log code                             */
/* -------------------------------------------------------------------- */

#define LOG1

#include <string.h>
#include <time.h>
#include "ctdl.h"
#include "prot.h"
#include "glob.h"

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/*  findPerson()    loads log record for named person.                  */
/*                  RETURNS: ERROR if not found, else log record #      */
/*  personexists()  returns slot# of named person else ERROR            */
/*  setdefaultconfig()  this sets the global configuration variables    */
/*  setlogconfig()  this sets the configuration in current logBuf equal */
/*                  to the global configuration variables               */
/*  setsysconfig()  this sets the global configuration variables equal  */
/*                  to the the ones in logBuf                           */
/*  showconfig()    displays user configuration                         */
/*  slideLTab()     crunches up slot, then frees slot at beginning,     */
/*                  it then copies information to first slot            */
/*  storeLog()      stores the current log record.                      */
/*  displaypw()     displays callers name, initials & pw                */
/*  normalizepw()   This breaks down inits;pw into separate strings     */
/*  pwexists()      returns TRUE if password exists in logtable         */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  findPerson()    loads log record for named person.                  */
/*                  RETURNS: ERROR if not found, else log record #      */
/* -------------------------------------------------------------------- */
int findPerson(const char *name, struct logBuffer *lbuf)
{
    int slot, logno;

    slot = personexists(name);

    if (slot == ERROR) return(ERROR);

    getLog(lbuf, logno = logTab[slot].ltlogSlot);

    return(logno);
}

/* -------------------------------------------------------------------- */
/*  personexists()  returns slot# of named person else ERROR            */
/* -------------------------------------------------------------------- */
int personexists(const char *name)
{
    int i, namehash;
    struct logBuffer logRead;

    namehash = hash(name);

    /* check to see if name is in log table */

    for ( i = 0;  i < cfg.MAXLOGTAB;  i++)
    {
        if (namehash == logTab[i].ltnmhash)
        {
            getLog(&logRead, logTab[i].ltlogSlot);

            if (strcmpi(name, logRead.lbname) == SAMESTRING)
                return(i);
        }
    }

    return(ERROR);
}

/* -------------------------------------------------------------------- */
/*  setdefaultconfig()  this sets the global configuration variables    */
/* -------------------------------------------------------------------- */
void setdefaultconfig(void)
{
    prevChar    = ' ';
    termWidth   = cfg.width;
    termLF      = (BOOL)cfg.linefeeds;
    termUpper   = (BOOL)cfg.uppercase;
    termNulls   = cfg.nulls;
    expert      = FALSE;
    aide        = FALSE;
    sysop       = FALSE;
    twit        = cfg.user[D_PROBLEM];
    unlisted    = FALSE;
    termTab     = (BOOL)cfg.tabs;
    oldToo      = TRUE;   /* later a cfg.lastold */
    roomtell    = FALSE;
    logBuf.NEXTHALL    = FALSE;
    logBuf.DUNGEONED   = FALSE;
    logBuf.MSGAIDE     = FALSE;
    logBuf.FORtOnODE   = FALSE;
#if 1
    logBuf.IBMGRAPH    = FALSE;
    logBuf.DISPLAYTS   = TRUE;
    logBuf.SUBJECTS    = TRUE;
    logBuf.SIGNATURES  = TRUE;
#endif
    logBuf.linesScreen = 0;
    strcpy(logBuf.tty, "TTY");
    ansiOn=FALSE;
}

/* -------------------------------------------------------------------- */
/*  setlogconfig()  this sets the configuration in current logBuf equal */
/*                  to the global configuration variables               */
/* -------------------------------------------------------------------- */
void setlogconfig(void)
{
    logBuf.lbwidth           = termWidth;
    logBuf.lbnulls           = termNulls;
    logBuf.lbflags.EXPERT    = expert;
    logBuf.lbflags.UCMASK    = termUpper;
    logBuf.lbflags.LFMASK    = termLF;
    logBuf.lbflags.AIDE      = aide;
    logBuf.lbflags.SYSOP     = sysop;
    logBuf.lbflags.TABS      = termTab;
    logBuf.lbflags.PROBLEM   = twit;
    logBuf.lbflags.UNLISTED  = unlisted;
    logBuf.lbflags.OLDTOO    = oldToo;
    logBuf.lbflags.ROOMTELL  = roomtell;
    if (ansiOn)
        strcpy(logBuf.tty, "ANSI-BBS");
    else
        strcpy(logBuf.tty, "TTY");
}

/* -------------------------------------------------------------------- */
/*  setsysconfig()  this sets the global configuration variables equal  */
/*                  to the the ones in logBuf                           */
/* -------------------------------------------------------------------- */
void setsysconfig(void)
{
    termWidth   = logBuf.lbwidth;
    termNulls   = logBuf.lbnulls;
    termLF      = (BOOL)logBuf.lbflags.LFMASK ;
    termUpper   = (BOOL)logBuf.lbflags.UCMASK ;
    expert      = (BOOL)logBuf.lbflags.EXPERT ;
    aide        = (BOOL)logBuf.lbflags.AIDE   ;
    sysop       = (BOOL)logBuf.lbflags.SYSOP  ;
    termTab     = (BOOL)logBuf.lbflags.TABS   ;
    oldToo      = (BOOL)logBuf.lbflags.OLDTOO ;
    twit        = (BOOL)logBuf.lbflags.PROBLEM;
    unlisted    = (BOOL)logBuf.lbflags.UNLISTED;
    roomtell    = (BOOL)logBuf.lbflags.ROOMTELL;
    ansiOn      = (BOOL)(strcmpi(logBuf.tty, "ANSI-BBS") == SAMESTRING);
}

/* -------------------------------------------------------------------- */
/*  showconfig()    displays user configuration                         */
/* -------------------------------------------------------------------- */
void showconfig(const struct logBuffer *lbuf)
{
    int i;
    char *dodisplay = "";
    char *dont = "o not d";

    outFlag = OUTOK;
    
    if(loggedIn || lbuf != &logBuf)
    {
        setlogconfig();

        doCR();
        mPrintf(" User ");
        
        if (cfg.titles   && *lbuf->title  )
        {
            mPrintf("[%s] ", lbuf->title);
        }
        
        mPrintf("%s", lbuf->lbname);
        
        if (cfg.surnames && *lbuf->surname)
        {
            mPrintf(" [%s]", lbuf->surname);
        }

#if 0
        if ((whichIO == CONSOLE) || lbuf == &logBuf)
        {
            doCR();
            mPrintf(" Password: %s;%s", lbuf->lbin, lbuf->lbpw);
        }
#endif
        
        if (lbuf->lbflags.UNLISTED ||
            lbuf->lbflags.SYSOP    ||
            lbuf->lbflags.AIDE     ||
            lbuf->lbflags.NETUSER  ||
            lbuf->lbflags.NODE     ||
            lbuf->DUNGEONED        ||
            lbuf->MSGAIDE)
        {
            doCR();         
            if (lbuf->lbflags.UNLISTED) mPrintf(" Unlisted");
            if (lbuf->lbflags.SYSOP)    mPrintf(" Sysop");
            if (lbuf->lbflags.AIDE)     mPrintf(" Aide");
            if (lbuf->lbflags.NETUSER)  mPrintf(" Netuser");
            if (lbuf->lbflags.NODE)     mPrintf(" (Node)");
            if (lbuf->DUNGEONED)        mPrintf(" Dungeoned");
            if (lbuf->MSGAIDE)          mPrintf(" Moderator");
        }

        if (lbuf->forward[0])
        {
            mPrintf("\n Private mail forwarded to ");

            if ( personexists(lbuf->forward) != ERROR )
                mPrintf("%s", lbuf->forward);
        }

        if (lbuf->hallhash)
        {
            mPrintf("\n Default hallway: ");

            for (i = 1; i < MAXHALLS; ++i)
            {
                if ( (int)hash( hallBuf->hall[i].hallname )  == lbuf->hallhash )
                {
                    if (groupseeshall(i))
                        mPrintf("%s", hallBuf->hall[i].hallname);
                }
            }
        }

        doCR();
        mPrintf(" Groups: ");

        prtList(LIST_START);
        for (i = 0; i < MAXGROUPS; ++i)
        {
            if (   grpBuf.group[i].g_inuse
                && (lbuf->groups[i] == grpBuf.group[i].groupgen)
               )
            {
                prtList(grpBuf.group[i].groupname);
                /*mPrintf("%s ", grpBuf.group[i].groupname);*/
            }
        }
        prtList(LIST_END);
    }

    if (cfg.accounting && !lbuf->lbflags.NOACCOUNT && loggedIn)
    {
        doCR();
        mPrintf(" Time in account %.0f", lbuf->credits);
    }
 
    mPrintf("\n Width %d, ", lbuf->lbwidth);
 
    if (lbuf->lbflags.UCMASK ) mPrintf("UPPERCASE ONLY, ");
 
    if (!lbuf->lbflags.LFMASK) mPrintf("No ");

    mPrintf("Linefeeds, ");
 
    mPrintf("%d nulls, ", lbuf->lbnulls);

    if (!lbuf->lbflags.TABS) mPrintf("No ");

    mPrintf("Tabs");

    mPrintf("\n D%sisplay last Old %s on N>ew %s request.",
            lbuf->lbflags.OLDTOO ? dodisplay : dont,
            cfg.msg_nym, cfg.msg_nym);

    if (loggedIn)
        mPrintf("\n Terminal type: %s", lbuf->tty);

    mPrintf("\n IBM Graphics characters %s" "abled",
            lbuf->IBMGRAPH ? "en" : "dis");

    if (lbuf->NEXTHALL)
        mPrintf("\n Auto-next hall on.");
    
    if (cfg.roomtell && loggedIn)
        mPrintf("\n D%sisplay room descriptions.",
            lbuf->lbflags.ROOMTELL ? dodisplay : dont);
    
#if 1
    if (cfg.surnames || cfg.netsurname || cfg.titles || cfg.nettitles)
        mPrintf("\n D%sisplay surnames and titles",
                lbuf->DISPLAYTS ? dodisplay : dont);

    mPrintf("\n D%sisplay subjects.",
            lbuf->SUBJECTS ? dodisplay : dont);

    mPrintf("\n D%sisplay signatures.",
            lbuf->SIGNATURES ? dodisplay : dont);
#endif

    doCR();
}

/* -------------------------------------------------------------------- */
/*  slideLTab()     crunches up slot, then frees slot at beginning,     */
/*                  it then copies information to first slot            */
/* -------------------------------------------------------------------- */
void slideLTab(int slot)    /* slot is current tabslot being moved */
{
    int ourSlot, i;

    people = slot; /* number of people since last call */

    if (!slot) return;

    ourSlot = logTab[slot].ltlogSlot;

    /* Gee, this works.. */
    for (i=slot; i>0; i--)
        logTab[i] = logTab[i-1];

    thisSlot = 0;

    /* copy info to beginning of table */
    logTab[0].ltpwhash      = hash(logBuf.lbpw);
    logTab[0].ltinhash      = hash(logBuf.lbin);
    logTab[0].ltnmhash      = hash(logBuf.lbname);
    logTab[0].ltlogSlot     = ourSlot;
    logTab[0].ltcallno      = logBuf.callno;
    logTab[0].permanent     = (BOOL)logBuf.lbflags.PERMANENT;
}
 

/* -------------------------------------------------------------------- */
/*  storeLog()      stores the current log record.                      */
/* -------------------------------------------------------------------- */
void storeLog(void)
{
    /* make log configuration equal to our environment */
    setlogconfig();

    putLog(&logBuf, thisLog);
}

/* -------------------------------------------------------------------- */
/*  displaypw()     displays callers name, initials & pw                */
/* -------------------------------------------------------------------- */
void displaypw(const char *name, const char *in, const char *pw)
{
    mPrintf("\n nm: %s",name);
    mPrintf("\n in: ");
    echo = CALLER;
    mPrintf("%s", in);
    echo = BOTH;
    mPrintf("\n pw: ");
    echo = CALLER;
    mPrintf("%s",pw);
    echo = BOTH;
    doCR();
}


/* -------------------------------------------------------------------- */
/*  normalizepw()   This breaks down inits;pw into separate strings     */
/* -------------------------------------------------------------------- */
void normalizepw(char *InitPw, char *Initials, char *passWord)
{
    char *pwptr;
    char *inptr;
    char *inpwptr;

    inpwptr = InitPw;
    pwptr   = passWord;
    inptr   = Initials;

    while (*inpwptr != ';')
    {
        *inptr++ = *inpwptr;
        inpwptr++;
    }
    *inptr++ = '\0';  /* tie off with a null */

    inpwptr++;   /* step over semicolon */

    while (*inpwptr != '\0')
    {
        *pwptr++ = *inpwptr;
        inpwptr++;
    }
    *pwptr++ = '\0';  /* tie off with a null */

    normalizeString(Initials);
    normalizeString(passWord);

    /* dont allow anything over 19 characters */
    Initials[19] = '\0';
    passWord[19] = '\0';
}

/* -------------------------------------------------------------------- */
/*  pwexists()      returns TRUE if password exists in logtable         */
/* -------------------------------------------------------------------- */
int pwexists(const char *pw)
{
    int i, pwhash;
    
    pwhash = hash(pw);

    for ( i = 0;  i < cfg.MAXLOGTAB;  i++)
    {
        if (pwhash == logTab[i].ltpwhash)
        return(i);
    }
    return(ERROR);
}
