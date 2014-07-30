/* -------------------------------------------------------------------- */
/*  LOG2.C                        ACit                         91Sep27  */
/*                       Overlayed login log code                       */
/* -------------------------------------------------------------------- */

#include <string.h>
#include <time.h>
#include "ctdl.h"
#include "prot.h"
#include "glob.h"

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/*  login()         is the menu-level routine to log someone in         */
/* $minibin()       minibin log-in stats                                */
/* $pwslot()        returns logtab slot password is in, else ERROR      */
/*  setalloldrooms()    set all rooms to be old.                        */
/*  setlbvisit()    sets lbvisit at log-in                              */
/*  setroomgen()    sets room gen# with log gen                         */
/*  terminate()     is menu-level routine to exit system                */
/*  initroomgen()   initializes room gen# with log gen                  */
/* $newlog()        sets up a new log entry for new users if available  */
/* $newslot()       attempts to find a slot for new user to reside in   */
/* $newUser()       prompts for name and password                       */
/* $newUserFile()   writes new user info out to a file                  */
/* -------------------------------------------------------------------- */

static void minibin(void);
static int pwslot(const char *in, const char *pw);
static int  newlog(const char  *fullnm,const char  *in,const char  *pw);
static int  newslot(void);
static void  newUser(const char  *initials,const char  *password);
static void  newUserFile(void);

/* -------------------------------------------------------------------- */
/*  login()         is the menu-level routine to log someone in         */
/* -------------------------------------------------------------------- */
void login(const char *initials, const char *password)
{
    int  foundIt;
    int loop;

    if (justLostCarrier)  return;

    /* reset transmitted & received */
    transmitted = 0l;
    received    = 0l;

    /* reset read & entered */
    mread   = 0;
    entered = 0;

    /* Clear message per room array */
    for (loop=0;loop < MAXROOMS; loop++)
    {
        MessageRoom[loop]=0;
    }

    foundIt = ((pwslot(initials, password)) != ERROR);

    if (foundIt && *password)
    {
        /* update userlog entries: */

        loggedIn    = TRUE;
        heldMessage = FALSE;
        markedMId = 0UL;

        setsysconfig();
        setgroupgen();
        setroomgen();
        setlbvisit();

        slideLTab(thisSlot);

        update25();

        /* trap it */
        if (!logBuf.lbflags.NODE) 
        {
          sprintf( msgBuf->mbtext, "Login %s", logBuf.lbname);
          trap(msgBuf->mbtext, T_LOGIN);
        }else{
          sprintf( msgBuf->mbtext, "NetLogin %s", logBuf.lbname);
          trap(msgBuf->mbtext, T_NETWORK);
        }
  
        /* cant log in now. */
        if (cfg.accounting && !logBuf.lbflags.NOACCOUNT)
        {
            negotiate();
            logincrement();
            if (!logincheck()) 
            {
                Initport();
                justLostCarrier = TRUE;
                return;
            }
        }

        /* cant log in now. */
        if (logBuf.VERIFIED && !(whichIO == CONSOLE))
        {
            tutorial("verified.blb");
            Initport();
            justLostCarrier = TRUE;
            return;
        }

        /* reverse engineering Minibin?!?! */
        if (cfg.loginstats && !logBuf.lbflags.NODE)  minibin();

        if (!logBuf.lbflags.NODE)
        {
            changedir(cfg.helppath); 
   
            if ( filexists("bulletin.blb") )
                tutorial("bulletin.blb");
        }
        
        gotodefaulthall();

        if (logBuf.lbflags.NODE)
        {
          time(&logtimestamp);
          return;
        }

        roomtalley();

        mf.mfLim = 0;   /* just to make sure. */
        mf.mfMai = 0;
        mf.mfPub = 0;
        mf.mfUser[0]=0;

        showMessages(NEWoNLY, FALSE, FALSE);

        if (expert) listRooms(NEWRMS, FALSE, FALSE);
        else        listRooms(OLDNEW, FALSE, FALSE);

        outFlag = OUTOK;

    }
    else
    {
        if ( (cfg.private != 0) && whichIO == MODEM && !sysopNew)
        {
            if (getYesNo(" No record: Request access", 1))
            {
                if (justLostCarrier) return;
                if (cfg.private < 5) tutorial("userinfo.blb");
                switch (cfg.private)
                {
                  case 10:
                  case 9:
                    tutorial("closesys.blb");
                    break;
                  case 8:
                  case 7:
                  case 6:
                  case 5:
                  case 4:
                  case 3:
                    if (cfg.private == 3 || cfg.private == 4
                      ||cfg.private == 7 || cfg.private == 8)
                    {
                        newUser(initials, password);
                        if (!loggedIn)  break;
                        logBuf.VERIFIED = TRUE;
                        newaccount();
                        update25();
                    }

                    if (cfg.private == 3 || cfg.private == 4)
                    {
                        newUserFile();
                    }

                    if (cfg.private == 5 || cfg.private == 6
                      ||cfg.private == 7 || cfg.private == 8)
                    {
                        if (changedir(cfg.aplpath) == ERROR)
                        {
                            mPrintf("  -- Can't find application directory.\n\n");
                            changedir(cfg.homepath);
                            break;
                        }
                        apsystem(cfg.newuserapp);
                        changedir(cfg.homepath);
                        break;
                    }

                    if (cfg.private == 3 || cfg.private == 4
                      ||cfg.private == 7 || cfg.private == 8)
                    {
                        time(&logtimestamp);
                        cfg.callno++;
                        storeLog();
                        terminate(FALSE, FALSE);
                    }

                    break;
                  case 2:
                  case 1:
                  default:
                    mailFlag  = TRUE;
                    oldFlag   = FALSE;
                    limitFlag = FALSE;
                    linkMess  = FALSE;
                    makeMessage();
                    break;
                }
                if ( (cfg.private == 2)
                   ||(cfg.private == 4)
                   ||(cfg.private == 6)
                   ||(cfg.private == 8)
                   ||(cfg.private == 10))
                {
                    mPrintf("\n Thank you, Good Bye.\n");
                    Initport();
                    justLostCarrier = TRUE;
                }
            }
            return;
        }
        else
        if (getYesNo(" No record: Enter as new user", 1))
        {
            newUser(initials, password);
            if (!loggedIn)  return;
            newaccount();
            update25();
            if (cfg.accounting && !logBuf.lbflags.NOACCOUNT)
            {
                negotiate();
                if (!logincheck()) 
                {
                  Initport();
                  justLostCarrier = TRUE;
                  return;
                }
            }
#if 1
            changedir(cfg.helppath); 
            if ( filexists("bulletin.blb") )
                tutorial("bulletin.blb");
            changedir(cfg.homepath);
#endif
            roomtalley();
            listRooms(OLDNEW, FALSE, FALSE);
        }
        else
        {
            if (whichIO == CONSOLE)
            {
                whichIO = MODEM;
                /* onConsole = (BOOL)( whichIO == CONSOLE); */
                Initport();
            }
        }
    }

    if (!loggedIn)  return;

    /* record login time, date */
    time(&logtimestamp);

    cfg.callno++;

    storeLog();
}

/* -------------------------------------------------------------------- */
/*  minibin()       minibin log-in stats                                */
/* -------------------------------------------------------------------- */
static void minibin(void)
{
    int calls, messages;
    char dtstr[80];

    messages = (int)(cfg.newest - logBuf.lbvisit[1]);
    calls    = (int)(cfg.callno - logBuf.callno);

    /* special hack to kill mangled surnames beacuse of the
       3.10.05 to 3.11.00 conversion program... */
#if 0
    if (!tfilter[logBuf.surname[0]])
    {
        logBuf.surname[0] = 0 /* NULL */;
    }
#endif
    
    if (!expert) mPrintf(" \n \n <J>ump <N>ext <P>ause <S>top");
    
    doCR();
    mPrintf("Welcome back ");
    if (cfg.titles && logBuf.title[0])
    {
        mPrintf("[%s] ", logBuf.title);
    }
    mPrintf("%s",logBuf.lbname);
    if (cfg.surnames && logBuf.surname[0])
    {
        mPrintf(" [%s]", logBuf.surname);
    }    
    mPrintf("!");
    doCR();
    mPrintf("You are position # %d in the userlog.", thisLog );
    doCR();
    if(calls == 0)
    {
        mPrintf("You were just here.");
        doCR();
    }
    else
    {
        sstrftime(dtstr, 79, cfg.vdatestamp, logBuf.calltime);
        mPrintf("You last called on: %s", dtstr);
        doCR();
        mPrintf("You are caller %s", ltoac(cfg.callno + 1l));
        doCR();
        mPrintf("%d %s made", people,
            (people == 1)?"person has":"people have");
        doCR();
        mPrintf("%d %s and left",calls, (calls == 1)?"call":"calls");
        doCR();
        mPrintf("%s new %s since you were last here.",ltoac((long)messages),
            (messages==1)? cfg.msg_nym: cfg.msgs_nym);
        doCR();
    }

    if (cfg.accounting && !logBuf.lbflags.NOACCOUNT)
    {
        if (!specialTime)
        {
            mPrintf("You have %.0f %s left today.", logBuf.credits,
                ((int)logBuf.credits == 1)?"minute":"minutes");
        } else {
            mPrintf("You have unlimited time.");
        }

        doCR();
    }

    outFlag = OUTOK;
}

/* -------------------------------------------------------------------- */
/*  pwslot()        returns logtab slot password is in, else ERROR      */
/* -------------------------------------------------------------------- */
static int pwslot(const char *in, const char *pw)
{
    int slot;

    if (strlen(pw) < 2)  return ERROR;  /* Don't search for these pwds */

    slot = pwexists(pw);

    if (slot == ERROR) return ERROR;

    /* initials must match too */
    if ( (logTab[slot].ltinhash) != (int)hash(in) ) return ERROR;

    getLog(&lBuf, logTab[slot].ltlogSlot);

    if (  (strcmpi(pw, lBuf.lbpw) == SAMESTRING)
    &&    (strcmpi(in, lBuf.lbin) == SAMESTRING) )
    {
        _fmemmove(&logBuf, &lBuf, sizeof logBuf);
        thisSlot = slot;
        thisLog  = logTab[slot].ltlogSlot;
        return(slot);
    }
    else  return ERROR;
}

/* -------------------------------------------------------------------- */
/*  setalloldrooms()    set all rooms to be old.                        */
/* -------------------------------------------------------------------- */
void setalloldrooms(void)
{
    int i;

    for (i = 1; i < MAXVISIT; i++)
        logBuf.lbvisit[i] = cfg.newest;

    logBuf.lbvisit[0] = cfg.newest;
}

/* -------------------------------------------------------------------- */
/*  setlbvisit()    sets lbvisit at log-in                              */
/* -------------------------------------------------------------------- */
void setlbvisit(void)
{
    int i;

    /* see if the message base was cleared since last call */
    for (i = 1; i < MAXVISIT; i++)
    {
        if (logBuf.lbvisit[i] > cfg.newest)
        {
            for (i = 1; i < MAXVISIT; i++)
                logBuf.lbvisit[i] = cfg.oldest;
            logBuf.lbvisit[ 0            ]= cfg.newest;
            logBuf.lbvisit[ (MAXVISIT-1) ]= cfg.oldest;
            doCR();
            mPrintf("%s base destroyed since last call!", cfg.msg_nym); doCR();
            mPrintf("All %s pointers reset.", cfg.msg_nym); doCR();
            return;
        }
    }
    
    /* slide lbvisit array down and change lbgen entries to match: */
    for (i = (MAXVISIT - 2);  i;  i--)
    {
        logBuf.lbvisit[i] = logBuf.lbvisit[i-1];
    }
    logBuf.lbvisit[(MAXVISIT - 1)] = cfg.oldest;
    logBuf.lbvisit[0             ] = cfg.newest;

    for (i = 0;  i < MAXROOMS;  i++)
    {
        if ((logBuf.lbroom[i].lvisit)  <  (MAXVISIT-2))
        {
            logBuf.lbroom[i].lvisit++;
        }
    } 
}

/* -------------------------------------------------------------------- */
/*  setroomgen()    sets room gen# with log gen                         */
/* -------------------------------------------------------------------- */
void setroomgen(void)
{
    int i;

    /* set gen on all unknown rooms  --  INUSE or no: */
    for (i = 0;  i < MAXROOMS;  i++)
    {

        /* Clear mail and xclude flags in logbuff for any  */
        /* rooms created since last call                   */
    
        if (logBuf.lbroom[i].lbgen != roomTab[i].rtgen)
        {
            /* logBuf.lbroom[i].mail    = FALSE; */
            logBuf.lbroom[i].xclude  = FALSE;
        }

        /* if not a public room */
        if (roomTab[i].rtflags.PUBLIC == 0)
        {
            /* if you don't know about the room */
            if (((logBuf.lbroom[i].lbgen) != roomTab[i].rtgen) ||
               (!aide && i == AIDEROOM))
            {
                /* mismatch gen #'s properly */
                logBuf.lbroom[i].lbgen 
                   = (unsigned char)((roomTab[i].rtgen + (MAXGEN-1)) % MAXGEN);

                logBuf.lbroom[i].lvisit =  MAXVISIT - 1;

            }
        }

        else if ((logBuf.lbroom[i].lbgen) != roomTab[i].rtgen) 
        {
            /* newly created public room -- remember to visit it; */
            logBuf.lbroom[i].lbgen  = roomTab[i].rtgen;
            logBuf.lbroom[i].lvisit = 1;
        }
    }
}

/* -------------------------------------------------------------------- */
/*  terminate()     is menu-level routine to exit system                */
/* -------------------------------------------------------------------- */
void terminate(char discon, char verbose)
{
    float balance;
    char  doStore;
    int   traptype;

    chatReq = FALSE;
    
    doStore = (BOOL)(haveCarrier || (whichIO == CONSOLE));

    if (discon || !doStore)
    {
        sysopNew = FALSE;
    }
      
    balance = logBuf.credits;

    outFlag = OUTOK;

    if (doStore && verbose == 2)
    {
        doCR();
        mPrintf(" You were caller %s", ltoac(cfg.callno));
        doCR();
        mPrintf(" You were logged in for: "); diffstamp(logtimestamp);
        doCR();
        mPrintf(" You entered %d %s", entered, cfg.msgs_nym);
        doCR();
        mPrintf(" and read %d.", mread);
        doCR();
        if (cfg.accounting && !logBuf.lbflags.NOACCOUNT)
        {
            mPrintf(" %.0f %s used this is call",startbalance - logBuf.credits,
              ( (int)(startbalance - logBuf.credits) == 1)?"minute":"minutes" );
            doCR();
            mPrintf(" Your balance is %.0f %s", logBuf.credits,
                 ( (int)logBuf.credits == 1 ) ? "minute" : "minutes" );
            doCR();
        }
    }

    if (doStore && verbose) goodbye();

    outFlag = IMPERVIOUS;

    if (loggedIn) mPrintf(" %s logged out\n ", logBuf.lbname);

    thisHall = 0;    /* go to ROOT hallway */

    if (discon) 
    {
        switch (whichIO)
        {
        case MODEM:
            Hangup();
            iChar();                    /* And now detect carrier loss  */
            break;
        case CONSOLE:
            whichIO =  MODEM;
            if (!gotCarrier())  Initport();
            break;
        }
    }

    if  ( !doStore)  /* if carrier dropped */
    {
        /* trap it */
        sprintf(msgBuf->mbtext, "Carrier dropped");
        trap(msgBuf->mbtext, T_CARRIER);
    }   
    else    /* update JL properly at status line */
    { 
        justLostCarrier = FALSE;
    }

    /* update new pointer only if carrier not dropped */
    if (loggedIn && doStore)
    {
        logBuf.lbroom[thisRoom].lbgen    = roomBuf.rbgen;
        logBuf.lbroom[thisRoom].lvisit   = 0;
     /* logBuf.lbroom[thisRoom].mail     = 0;  */
    }

    if (loggedIn)
    {
        logBuf.callno      = cfg.callno;
        logBuf.calltime    = logtimestamp;
        logBuf.lbvisit[0]  = cfg.newest;
        logTab[0].ltcallno = cfg.callno;

        storeLog();
        loggedIn = FALSE;

        /* trap it */
        if (!logBuf.lbflags.NODE) 
        {
            sprintf(msgBuf->mbtext, "Logout %s", logBuf.lbname);
            trap(msgBuf->mbtext, T_LOGIN);
        }else{
            sprintf(msgBuf->mbtext, "NetLogout %s", logBuf.lbname);
            trap(msgBuf->mbtext, T_NETWORK);
        }

        if (cfg.accounting)  unlogthisAccount();
        heldMessage = FALSE;
        cleargroupgen();
        initroomgen();

        strcpy(logBuf.lbname, "Not_logged_in");

        setalloldrooms();
    }

    update25();

    setdefaultconfig();
    ansiOn = TRUE;
    roomtalley();
    getRoom(LOBBY);

#if 0
    if (!logBuf.lbflags.NODE)
        traptype = T_ACCOUNT;
    else
#endif
        traptype = T_NETWORK;


    sprintf(msgBuf->mbtext, "  ----- %4d messages entered", entered);
    trap(msgBuf->mbtext, traptype);

    sprintf(msgBuf->mbtext, "  ----- %4d messages read",  mread);
    trap(msgBuf->mbtext, traptype);

    if (logBuf.lbflags.NODE)
    {
       sprintf(msgBuf->mbtext, "  ----- %4d messages expired",  xpd);
       trap(msgBuf->mbtext, T_NETWORK);

       sprintf(msgBuf->mbtext, "  ----- %4d messages duplicate",  duplic);
       trap(msgBuf->mbtext, T_NETWORK);
    }    

    sprintf(msgBuf->mbtext, "Cost was %ld", (long)startbalance - (long)balance);
    trap(msgBuf->mbtext, T_ACCOUNT);
#if 1
    fclose(trapfl);
    trapfl = fopen(cfg.trapfile, "a+");
#endif

}

/* -------------------------------------------------------------------- */
/*  initroomgen()   initializes room gen# with log gen                  */
/* -------------------------------------------------------------------- */
void initroomgen(void)
{
    int i;

    for (i = 0; i < MAXROOMS;  i++)
    {
        /* Clear mail and xclude flags in logbuff for every room */

        /* logBuf.lbroom[i].mail    = FALSE; */
        logBuf.lbroom[i].xclude  = FALSE;

        if (roomTab[i].rtflags.PUBLIC == 1)
        {
            /* make public rooms known: */
            logBuf.lbroom[i].lbgen  = roomTab[i].rtgen;
            logBuf.lbroom[i].lvisit = MAXVISIT - 1;

        } else
        {
            /* make private rooms unknown: */
            logBuf.lbroom[i].lbgen = (unsigned char)
                ((roomTab[i].rtgen + (MAXGEN-1)) % MAXGEN);

            logBuf.lbroom[i].lvisit = MAXVISIT - 1;
        }
    }
}

/* -------------------------------------------------------------------- */
/*  newlog()        sets up a new log entry for new users returns ERROR */
/*                  if cannot find a usable slot                        */
/*                  #OLDCOUNT implemented semantically (newpointer)     */
/* -------------------------------------------------------------------- */
int newlog(const char *fullnm, const char *in, const char *pw)
{
    int  ourSlot, i;
    ulong newpointer;

    /* get a new slot for this user */
    thisSlot = newslot();

    if(thisSlot == ERROR)
    {
        thisSlot = 0;
        return(ERROR);
    }

    ourSlot = logTab[thisSlot].ltlogSlot;

    getLog(&logBuf, ourSlot);

    /* copy info into record: */
    setlogconfig();
    strcpy(logBuf.lbname, fullnm);
    strcpy(logBuf.lbin, in);
    strcpy(logBuf.lbpw, pw);
    logBuf.surname[0] = '\0';     /* no starting surname */
    logBuf.title  [0] = '\0';     /* no starting title   */
    logBuf.forward[0] = '\0';     /* no starting forwarding */

    logBuf.lbflags.L_INUSE   = TRUE;
    logBuf.lbflags.PROBLEM   = cfg.user[D_PROBLEM];
    logBuf.lbflags.PERMANENT = cfg.user[D_PERMANENT];
    logBuf.lbflags.NOACCOUNT = cfg.user[D_NOACCOUNT];
    logBuf.lbflags.NETUSER   = cfg.user[D_NETWORK];
    logBuf.lbflags.NOMAIL    = cfg.user[D_NOMAIL];
    logBuf.lbflags.AIDE      = cfg.user[D_AIDE];
    logBuf.lbflags.NODE      = FALSE;
    logBuf.lbflags.SYSOP     = cfg.user[D_SYSOP];

    logBuf.DUNGEONED         = FALSE;
    logBuf.MSGAIDE           = FALSE;
    logBuf.FORtOnODE         = FALSE;
/*  logBuf.NEXTHALL          = FALSE; */
    logBuf.VERIFIED          = FALSE;
#if 1
    logBuf.IBMGRAPH          = FALSE;
    logBuf.DISPLAYTS         = TRUE;
    logBuf.SUBJECTS          = TRUE;
    logBuf.SIGNATURES        = TRUE;
#endif

#if 1
    if (!cfg.oldcount)
    {
#endif
        for (i = 1; i < MAXVISIT; i++)
        {
            logBuf.lbvisit[i] = cfg.oldest;
        }

        logBuf.lbvisit[ 0            ]= cfg.newest;
        logBuf.lbvisit[ (MAXVISIT-1) ]= cfg.oldest;
#if 1
    }
    else
    {
        newpointer = (cfg.newest - cfg.oldcount);
        if (newpointer < cfg.oldest)  newpointer = cfg.oldest;

        for (i = 1; i < MAXVISIT; i++)
            logBuf.lbvisit[i] = newpointer;

        logBuf.lbvisit[ 0            ]= cfg.newest;
        logBuf.lbvisit[ (MAXVISIT-1) ]= newpointer;
    }
#endif
    
    initroomgen();

    cleargroupgen();

    /* put user into group NULL */
    logBuf.groups[0] = grpBuf.group[0].groupgen;

    /* accurate read-userlog for first time call */
    logBuf.callno   = cfg.callno + 1;
    logBuf.credits  = (float)0;
    time(&logBuf.calltime);

    setsysconfig();

    /* trap it */
    sprintf( msgBuf->mbtext, "New user %s", logBuf.lbname);
    trap(msgBuf->mbtext, T_LOGIN);

    loggedIn = TRUE;
    slideLTab(thisSlot);
    storeLog();

    return(TRUE);
}

/* -------------------------------------------------------------------- */
/*  newslot()       attempts to find a slot for a new user to reside in */
/*                  puts slot in global var  thisSlot                   */
/* -------------------------------------------------------------------- */
int newslot(void)
{
    int i;
    int foundit = ERROR;

    for ( i = cfg.MAXLOGTAB - 1; ((i > -1) && (foundit == ERROR)) ; --i)
    {
        if (!logTab[i].permanent) foundit = i;
    }
    if (foundit == ERROR)
    {
        mPrintf("\n All log slots taken.\n");
    }
    return foundit;
}

/* -------------------------------------------------------------------- */
/*  newUser()       prompts for name and password                       */
/* -------------------------------------------------------------------- */
void newUser(const char *initials, const char *password)
{
    label fullnm;
    char InitPw[80];
    char Initials[80];
    char passWord[80];
    char *semicolon = FALSE;

    int abort, good = 0;
    char  firstime = 1;

    if (justLostCarrier)  return;

    unlisted = FALSE;  /* default to [Y] for list in userlog for new users */
    roomtell = TRUE;   /* default to [Y] for display of room descriptions  */

    configure(TRUE);      /* make sure new users configure reasonably     */

    tutorial("password.blb");
    
    do
    {
        do
        {
            getNormStr("full name", fullnm, NAMESIZE, ECHO);

            if ( (personexists(fullnm) != ERROR )
            ||   (strcmpi(fullnm, "Sysop") == SAMESTRING)
            ||   !strlen(fullnm) )
            {
                mPrintf("We already have a %s\n", fullnm);
                good = FALSE;
            }        
            else (good = TRUE);
        }
        while(!good && !justLostCarrier);

        if (justLostCarrier)  return;

        if (firstime)  strcpy(Initials, initials);
        else
        {
            getNormStr("your initials", InitPw, 40, NO_ECHO);
            dospCR();

            semicolon = strchr(InitPw, ';');

            if( semicolon )
            {
               normalizepw(InitPw, Initials, passWord);
            }
            else  strcpy(Initials, InitPw);

            /* dont allow anything over 19 characters */
            Initials[19] = '\0';
        }

        do
        {
            if (firstime)  strcpy(passWord, password);
            else
            if (!semicolon)
            {
                getNormStr("password",  passWord, NAMESIZE, NO_ECHO);
                dospCR();
            }
            firstime  = FALSE;  /* keeps from going in infinite loop */
            semicolon = FALSE;

            if ( pwexists(passWord) != ERROR || strlen(passWord) < 2)
            {
                good = FALSE;
                mPrintf("\n Poor password\n ");
            }
            else good = TRUE;
        }
        while( !good  && !justLostCarrier 
        /*(haveCarrier || whichIO==CONSOLE)*/);

        displaypw(fullnm, Initials, passWord);

        abort = getYesNo("OK",2);

        if (abort == 2) return;  /* check for Abort at (Y/N/A)[A]: */
    }
    while ( (!abort) && (haveCarrier || whichIO==CONSOLE));

    if (haveCarrier || whichIO == CONSOLE)
    {
        if (newlog(fullnm, Initials, passWord) == ERROR) return;
    }
}

/* -------------------------------------------------------------------- */
/*  newUserFile()   Writes new user info out to a file                  */
/* -------------------------------------------------------------------- */
void newUserFile(void)
{
    FILE           *fl;
    char      name[40];
    char     phone[30];
    label      surname;
    char      temp[60];
    char     dtstr[80];
    int    tempmaxtext;
    int          clm=0;
    int            l=0;

    *name     ='\0';
    *phone    ='\0';
    *surname  ='\0';

    if (cfg.surnames)
        getNormStr("the title you desire",   surname,  20, ECHO);

    getNormStr("your REAL name",           name,     40, ECHO);

    if (name[0])
        getNormStr("your phone number [(xxx)xxx-xxxx]", phone, 30, ECHO);

    strcpy(msgBuf->mbto, "Sysop");
    strcpy(msgBuf->mbauth, logBuf.lbname);
    msgBuf->mbtext[0] = 0;
    msgBuf->mbsur[0] = 0 /* NULL */;
    tempmaxtext = cfg.maxtext;
    cfg.maxtext = 1024;

    getText();

    cfg.maxtext = tempmaxtext;

    if (changedir(cfg.homepath) == ERROR)  return;

    fl = fopen("newuser.log", "at");
    sstrftime(dtstr, 79, cfg.vdatestamp, 0l);

    sprintf(temp, "\n %s\n", dtstr);
    fwrite(temp, strlen(temp), 1, fl);

    if (surname[0])
    {
        sprintf(temp, " Nym:       [%s] %s\n",surname, logBuf.lbname );
    }
    else
    {
        sprintf(temp, " Nym:       %s\n",    logBuf.lbname );
    }
    fwrite(temp, strlen(temp), 1, fl);

    sprintf(temp, " Real name: %s\n",             name );
    fwrite(temp, strlen(temp), 1, fl);

    sprintf(temp, " Phone:     %s\n",            phone );
    fwrite(temp, strlen(temp), 1, fl);

    sprintf(temp, " Baud:      %d\n",     bauds[speed] );
    fwrite(temp, strlen(temp), 1, fl);

    sprintf(temp, "\n");

    if(msgBuf->mbtext[0])   /* xPutStr(fl, msgBuf->mbtext); */
    {
        do
        {
            if((msgBuf->mbtext[l] == 32 || msgBuf->mbtext[l] == 9) && clm > 73)
            {
                fwrite(temp, strlen(temp), 1, fl);
                clm = 0;
                l++;
            }
            else
            {
                fputc(msgBuf->mbtext[l], fl);
                clm++;
                if(msgBuf->mbtext[l] == 10)
                    clm = 0;
                if(msgBuf->mbtext[l] == 9)
                    clm = clm + 7;
                l++;
            }
        } while(msgBuf->mbtext[l]);
    }

    fclose(fl);
    doCR();
}
