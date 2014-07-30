/* -------------------------------------------------------------------- */
/*  LOG3.C                        ACit                         91Aug27  */
/* -------------------------------------------------------------------- */
/*                     Overlayed newuser log code                       */
/*                  and configuration / userlog edit                    */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Includes                                                            */
/* -------------------------------------------------------------------- */
#include <string.h>
#include <time.h>
#include "ctdl.h"
#include "prot.h"
#include "glob.h"

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/*  forwardaddr()   .EF sets up forwarding address for private mail     */
/*  killuser()      .SK sysop special to kill a log entry               */
/*  newPW()         .EP is routine to change password & initials        */
/*  Readlog()       .RU handles read userlog                            */
/*  showuser()      .SS sysop fn: to display any user's config.         */
/*  userEdit()      .SU Edit a user via menu                            */
/*  configure()     .EC edits user configuration via menu               */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  forwardaddr()   sets up forwarding address for private mail         */
/* -------------------------------------------------------------------- */
void forwardaddr(void)
{
    label name;
    int logno;

    getNormStr("forwarding name", name, NAMESIZE, ECHO);

    if( !strlen(name) )
    {
        mPrintf(" Private mail now routed to you");
        logBuf.forward[0] = '\0';
    }
    else
    {
        logno = findPerson(name, &lBuf);

        if (logno == ERROR)
        {
            mPrintf("No '%s' known.", name);
            return;
        }

        mPrintf(" Private mail now routed to %s", lBuf.lbname);
        strcpy(logBuf.forward, lBuf.lbname);
    }
    /* save it */
    if (loggedIn) storeLog();
}

/* -------------------------------------------------------------------- */
/*  killuser()      sysop special to kill a log entry                   */
/* -------------------------------------------------------------------- */
void killuser(void)
{
    label who;
    int logno, tabslot;

    getNormStr("who", who, NAMESIZE, ECHO);

    logno   = findPerson(who, &lBuf);

    if (logno == ERROR || !strlen(who))  
    {
        mPrintf("No \'%s\' known. \n ", who);
        return;
    }

    if (strcmpi(logBuf.lbname, who) == SAMESTRING)
    {
        mPrintf("Can't kill your own account, log out first.\n");
        return;
    }

    if (!getYesNo(confirm, 0))  return;

    mPrintf( "\'%s\' terminated.\n ", who);

    /* trap it */
    sprintf(msgBuf->mbtext, "User %s terminated", who);
    trap(msgBuf->mbtext, T_SYSOP);

    /* get log tab slot for person */
    tabslot = personexists(who);

    logTab[tabslot].ltpwhash   = 0;
    logTab[tabslot].ltinhash   = 0;
    logTab[tabslot].ltnmhash   = 0;
    logTab[tabslot].permanent  = 0;

    lBuf.lbname[0] = '\0';
    lBuf.lbin[  0] = '\0';
    lBuf.lbpw[  0] = '\0';
    lBuf.lbflags.L_INUSE   = FALSE;
    lBuf.lbflags.PERMANENT = FALSE;

    putLog(&lBuf, logno);
}

/* -------------------------------------------------------------------- */
/*  newPW()         is menu-level routine to change password & initials */
/* -------------------------------------------------------------------- */
void newPW(void)
{
    char InitPw[42];
    char passWord[42];
    char Initials[42];
    char oldPw[42];
    char *semicolon;

    int  goodpw;

    if (!loggedIn)
    {
        mPrintf("\n --Must be logged in.\n ");
        return ;
    }

    /* display old pw & initials */
    displaypw(logBuf.lbname, logBuf.lbin, logBuf.lbpw);

    if (!getYesNo(confirm, 0))  return;

    strcpy(oldPw, logBuf.lbpw);

    getNormStr("your new initials", InitPw, 40, NO_ECHO);
    dospCR();

    semicolon = strchr(InitPw, ';');

    if(semicolon)
    {
        normalizepw(InitPw, Initials, passWord);
    }
    else  strcpy(Initials, InitPw);

    /* dont allow anything over 19 characters */
    Initials[19] = '\0';

    do                           
    {
        if (!semicolon) 
        {
            getNormStr("new password", passWord, NAMESIZE, NO_ECHO);
            dospCR();
        }
        goodpw = ( ((pwexists(passWord) == ERROR) && strlen(passWord) >= 2)
            || (strcmpi(passWord, oldPw) == SAMESTRING));

        if ( !goodpw) mPrintf("\n Poor password\n ");
        semicolon = FALSE;
    } 
    while ( !goodpw && (haveCarrier || whichIO==CONSOLE));

    strcpy(logBuf.lbin, Initials);
    strcpy(logBuf.lbpw, passWord);

    /* insure against loss of carrier */
    if (haveCarrier || whichIO == CONSOLE)
    {
        logTab[0].ltinhash      = hash(Initials);
        logTab[0].ltpwhash      = hash(passWord);

        storeLog();
    }

    /* display new pw & initials */
    displaypw(logBuf.lbname, logBuf.lbin, logBuf.lbpw);

    /* trap it */
    trap("Password changed", T_PASSWORD);
}

/* -------------------------------------------------------------------- */
/*  Readlog()       handles read userlog                                */
/* -------------------------------------------------------------------- */
void Readlog(char verbose, char reverse)
{
    int i, grpslot;
    char dtstr[80];
    char flags[11];
    char wild=FALSE;
    char buser=FALSE;
    int incr;

    grpslot = ERROR;

    if (mf.mfUser[0])
    {
        getNormStr("user", mf.mfUser, NAMESIZE, ECHO);                     
                                                                           
        if (personexists(mf.mfUser) == ERROR)                              
        {                                                                  
            if(   strpos('?',mf.mfUser)                                       
               || strpos('*',mf.mfUser)                                       
               || strpos('[',mf.mfUser))                                      
            {                                                                 
                wild = TRUE;                                                  
            }                                                                 
            else                                                              
            {                                                                 
                mPrintf(" \nNo such user!\n ");                               
                return;                                                       
            }                                                                 
        }                                                                  
        else                                                               
        {                                                                  
            buser = TRUE;                                                    
        }                                                                  
    }

    outFlag = OUTOK;

    if (mf.mfLim && (cfg.readluser || sysop || aide))
    {
        doCR();
        getgroup();
        if (!mf.mfLim)
            return;
        grpslot = partialgroup(mf.mfGroup);
    }
    else
    {
        mf.mfLim = FALSE;
    }

    if (!expert) mPrintf(" \n \n <J>ump <N>ext <P>ause <S>top");

#if 1
    if (reverse)
    {
        i = cfg.MAXLOGTAB-1;
        incr = -1;
    }
    else
    {
        i = 0;
        incr = 1;
    }

    for (; (i >= 0 && i < cfg.MAXLOGTAB) && (outFlag != OUTSKIP); i += incr)
#else
    for (i = 0; ( (i < cfg.MAXLOGTAB) && (outFlag != OUTSKIP) ); i++)
#endif
    {
        if(BBSCharReady())
            if(mAbort())
                return;

        if (logTab[i].ltpwhash != 0 &&
            logTab[i].ltnmhash != 0)
        {
            if (buser && (int)hash(mf.mfUser) != logTab[i].ltnmhash)
                continue;

            getLog(&lBuf,logTab[i].ltlogSlot);

            if (buser && strcmpi(mf.mfUser, lBuf.lbname) != SAMESTRING)
                continue;

            if(wild && !u_match(lBuf.lbname, mf.mfUser))
                continue;

            if (mf.mfLim
              && lBuf.groups[grpslot] != grpBuf.group[grpslot].groupgen)
              continue;
       
#if 1
            /* Do not show yourself if unlisted */
            if ( lBuf.lbflags.L_INUSE
                 && (!lBuf.lbflags.UNLISTED || i) )
#else
            /* Show yourself even if unlisted */
            if ( (!i && loggedIn) || 
                 (lBuf.lbflags.L_INUSE
                  && (aide || !lBuf.lbflags.UNLISTED) )  )
#endif
            {
                if (verbose)
                {
                    sstrftime(dtstr, 79, cfg.vdatestamp, lBuf.calltime);

                    if ((cfg.surnames || cfg.titles) && verbose >= 2)
                    {
                        doCR();
                        doCR();
                        if (*lBuf.title)   mPrintf(" [%s]", lBuf.title);
                                           mPrintf(" %s",   lBuf.lbname);
                        if (*lBuf.surname) mPrintf(" [%s]", lBuf.surname);
                        doCR();
                        mPrintf(" #%lu %s",  lBuf.callno, dtstr);
                    }
                    else
                    {
                        doCR();
                        mPrintf(" %-20s #%lu %s", lBuf.lbname, lBuf.callno, dtstr);
                    }
                }
                else
                {
                    doCR();
#if 1
                    if (aide || lBuf.lbflags.NODE)
#endif
                    mPrintf(" %-20s",lBuf.lbname);
#if 1
                    else
                        mPrintf(" %s", lBuf.lbname);
#endif
                }

                if (aide )    /*   A>ide T>wit P>erm U>nlist N>etuser S>ysop */
                {
                    if (cfg.accounting && verbose)
                    {
                        if (lBuf.lbflags.NOACCOUNT)
                             mPrintf( " %10s", "N/A");
                        else mPrintf( " %10.2f", lBuf.credits);
                    }
    
                    strcpy(flags, "         ");

                    if ( lBuf.lbflags.AIDE)      flags[0] = 'A';
                    if ( lBuf.lbflags.PROBLEM)   flags[1] = 'T';
                    if ( lBuf.lbflags.PERMANENT) flags[2] = 'P';
                    if ( lBuf.lbflags.NETUSER)   flags[3] = 'N';
                    if ( lBuf.lbflags.UNLISTED)  flags[4] = 'U';
                    if ( lBuf.lbflags.SYSOP)     flags[5] = 'S';
                    if ( lBuf.lbflags.NOMAIL)    flags[6] = 'M';
                    if ( lBuf.VERIFIED)          flags[7] = 'V';
                    if ( lBuf.DUNGEONED)         flags[8] = 'D';
                    if ( lBuf.MSGAIDE)           flags[9] = 'm';
    
                    mPrintf(" %s",flags);
                }

                if (lBuf.lbflags.NODE)
                {
                    mPrintf(" (Node) ");
                }

#if 0
                if (verbose) doCR();
#endif
            }
        }
    }
    doCR();
}

/* -------------------------------------------------------------------- */
/*  showuser()      aide fn: to display any user's config.              */
/* -------------------------------------------------------------------- */
void showuser(void)
{  
    label who;
    int logno, oldloggedIn, oldthisLog;

    oldloggedIn = loggedIn;
    oldthisLog  = thisLog;

    loggedIn = TRUE;

    getNormStr("who", who, NAMESIZE, ECHO);

    if( strcmpi(who, logBuf.lbname) == SAMESTRING)
    {
        showconfig(&logBuf);
    }
    else
    {
        logno   = findPerson(who, &lBuf);

        if ( !strlen(who) || logno == ERROR)
        {
            mPrintf("No \'%s\' known. \n ", who);
        }
        else
        {
            showconfig(&lBuf);
        }
    }
   
    loggedIn = (BOOL)oldloggedIn;
    thisLog  = oldthisLog;
}

/* -------------------------------------------------------------------- */
/*  userEdit()      Edit a user via menu                                */
/* -------------------------------------------------------------------- */
void userEdit(void)
{
    BOOL    prtMess = TRUE;
    BOOL    quit    = FALSE;
    int     c;
    char    string[200];
    char    oldEcho;
    label   who, temp;
    int     logNo, ltabSlot, tsys;
    BOOL    editSelf = FALSE;
    
    getNormStr("who", who, NAMESIZE, ECHO);
    logNo    = findPerson(who, &lBuf);
    ltabSlot = personexists(who);

    if ( !strlen(who) || logNo == ERROR)  
    {
        mPrintf("No \'%s\' known. \n ", who);
        return;
    }

    /* make sure we use curent info */
    if (strcmpi(who, logBuf.lbname) == SAMESTRING)
    {
        tsys = logBuf.lbflags.SYSOP;
        setlogconfig(); /* update curent user */
        logBuf.lbflags.SYSOP = tsys;
        lBuf = logBuf;  /* use their online logbuffer */
        editSelf = TRUE;
    }

    doCR();

    do 
    {
        if (prtMess)
        {
            doCR();
            outFlag = OUTOK;
            mPrintf("<3N0> 3N0ame............. %s", lBuf.lbname);  doCR();
            mPrintf("<310> Title............ %s", lBuf.title);   doCR();
            mPrintf("<320> Surname.......... %s", lBuf.surname); doCR();
            mPrintf("<3L0> 3L0ock T & Surname. %s", 
                                            lBuf.SURNAMLOK ? "Yes" : "No");            
                                            doCR();
            mPrintf("<3Y0> S3y0sop............ %s", 
                                            lBuf.lbflags.SYSOP ? "Yes" : "No");
                                            doCR();
            mPrintf("<3D0> Ai3d0e............. %s", 
                                            lBuf.lbflags.AIDE ? "Yes" : "No");
                                            doCR();
            mPrintf("<3O0> N3o0de............. %s", 
                                            lBuf.lbflags.NODE ? "Yes" : "No");
                                            doCR();
            mPrintf("<3P0> 3P0ermanent........ %s", 
                                            lBuf.lbflags.PERMANENT ?"Yes":"No");
                                            doCR();
            mPrintf("<3E0> N3e0tuser.......... %s", 
                                            lBuf.lbflags.NETUSER ? "Yes" :"No");
                                            doCR();
            mPrintf("<3T0> 3T0witted.......... %s", 
                                            lBuf.lbflags.PROBLEM ? "Yes" :"No");  
                                            doCR();
            mPrintf("<3M0> 3M0ail............. %s", 
                                            lBuf.lbflags.NOMAIL ? "Off" : "On");
                                            doCR();
            mPrintf("<3V0> 3V0erified......... %s",
                                            !lBuf.VERIFIED ? "Yes" : "No");
                                            doCR();
            
            if (cfg.accounting)
            {
                mPrintf("<3I0> T3i0me (minutes)... ");
                
                if (lBuf.lbflags.NOACCOUNT)
                    mPrintf("N/A");
                else
                    mPrintf("%.0f", lBuf.credits);
    
                doCR();
            }
            
            if ((whichIO == CONSOLE))
            {
                mPrintf("    Password.......... %s;%s", lBuf.lbin, lBuf.lbpw); 
                                                            doCR();
            }
            
            doCR();
            mPrintf("<3S0> to save, <3A0> to abort."); doCR();
            prtMess = (BOOL)(!expert);
        }

        outFlag = IMPERVIOUS;

        doCR();
        mPrintf("2Change:0 ");
        
        oldEcho = echo;
        echo    = NEITHER;
        c       = iChar();
        echo    = oldEcho;

        if (!((whichIO == CONSOLE) || gotCarrier()))
            return;

        switch(toupper(c))
        {
        case '1':
            mPrintf("Title"); doCR();
            if (lBuf.lbflags.SYSOP && lBuf.SURNAMLOK && !editSelf)
            {
                doCR();
                mPrintf("User has locked their title and surname!"); doCR();
            }
            else 
            {
                strcpy(temp, lBuf.title);
                getString("new title", lBuf.title, 20, FALSE, ECHO, temp);
                if (!strlen(lBuf.title))
                {
                    strcpy(lBuf.title, temp);
                }
                normalizeString(lBuf.title);
            }
            break;
        
        case '2':
            mPrintf("Surname"); doCR();
            if (lBuf.lbflags.SYSOP && lBuf.SURNAMLOK && !editSelf)
            {
                doCR();
                mPrintf("User has locked their title and surname!"); doCR();
            }
            else 
            {
                strcpy(temp, lBuf.surname);
                getString("New surname", lBuf.surname, 20, FALSE, ECHO, temp);
                if (!strlen(lBuf.surname))
                {
                    strcpy(lBuf.surname, temp);
                }
                normalizeString(lBuf.surname);
            }
            break;

        case 'A':
            mPrintf("Abort"); doCR();
            if (getYesNo("Abort changes", 1))
            {
                return;
            }
            break;

        case 'D':
            lBuf.lbflags.AIDE = (BOOL)(!lBuf.lbflags.AIDE);
            mPrintf("Aide %s", lBuf.lbflags.AIDE ? "Yes" : "No");  doCR();
            break;

        case 'E':
            lBuf.lbflags.NETUSER = (BOOL)(!lBuf.lbflags.NETUSER);
            mPrintf("Netuser %s", lBuf.lbflags.NETUSER ? "Yes" : "No");  
                doCR();
            break;

        case 'I':
            mPrintf("Minutes"); doCR();
            if (cfg.accounting)
            {
                lBuf.lbflags.NOACCOUNT = 
                    getYesNo("Disable user's accounting", 
                        (BOOL)lBuf.lbflags.NOACCOUNT);
    
                if (!lBuf.lbflags.NOACCOUNT)
                {
                    lBuf.credits = (float)
                        getNumber("minutes in account", (long)0,
                        (long)cfg.maxbalance, (long)lBuf.credits);
                }
            }
            else 
            {
                doCR();
                mPrintf("Accounting turned off for system.");
            }
            break;

        case 'L':
            if (lBuf.lbflags.SYSOP && lBuf.SURNAMLOK && !editSelf)
            {
                mPrintf("Lock Title and Surname.");  doCR();
                doCR();
                mPrintf("You can not change that!"); doCR();
            }
            else
            {
                lBuf.SURNAMLOK = (BOOL)(!lBuf.SURNAMLOK);
                mPrintf("Lock Title and Surname: %s", 
                                                lBuf.SURNAMLOK ? "On" : "Off");
                                                doCR();
            }
            break;

        case 'M':
            lBuf.lbflags.NOMAIL = (BOOL)(!lBuf.lbflags.NOMAIL);
            mPrintf("Mail %s", lBuf.lbflags.NOMAIL ? "Off" : "On");  
                doCR();
            break;

        case 'N':
            mPrintf("Name"); doCR();
            strcpy(temp, lBuf.lbname);
            getString("New name", lBuf.lbname, 20, FALSE, ECHO, temp);
            normalizeString(lBuf.lbname);
            if (!strlen(lBuf.lbname))
                strcpy(lBuf.lbname, temp);
            break;

        case 'O':
            lBuf.lbflags.NODE = (BOOL)(!lBuf.lbflags.NODE);
            mPrintf("Node %s", lBuf.lbflags.NODE ? "Yes" : "No");  doCR();
            break;

        case 'P':
            lBuf.lbflags.PERMANENT = (BOOL)(!lBuf.lbflags.PERMANENT);
            mPrintf("Permanent %s", lBuf.lbflags.PERMANENT ? "Yes" : "No");  
                doCR();
            break;
 
        case 'S':
            mPrintf("Save"); doCR();
            if (getYesNo("Save changes", 0))
            {
                quit = TRUE;
            }
            break;

        case 'T':
            lBuf.lbflags.PROBLEM = (BOOL)(!lBuf.lbflags.PROBLEM);
            mPrintf("Twit/Problem user %s", lBuf.lbflags.PROBLEM ? "Yes" : "No");  
                doCR();
            break;

        case 'V':
            lBuf.VERIFIED = (BOOL)(!lBuf.VERIFIED);
            mPrintf("Verified %s", !lBuf.VERIFIED ? "Yes" : "No");  
                doCR();
            break;
        
        case 'Y':
            lBuf.lbflags.SYSOP = (BOOL)(!lBuf.lbflags.SYSOP);
            mPrintf("Sysop %s", lBuf.lbflags.SYSOP ? "Yes" : "No");  doCR();
            break;

        case '\r':
        case '\n':
        case '?':
            mPrintf("Menu"); doCR();
            prtMess = TRUE;
            break;

        default:
            mPrintf("%c ? for help", c); doCR();
            break;
        }

    } while (!quit);

    /* trap it */
    sprintf(string, "%s has:", who);
    if (lBuf.lbflags.SYSOP)     strcat(string, " Sysop Priv:");
    if (lBuf.lbflags.AIDE)      strcat(string, " Aide Priv:");
    if (lBuf.lbflags.NODE)      strcat(string, " Node status:");
    if (cfg.accounting)
    {
        if (lBuf.lbflags.NOACCOUNT)
        {
            strcat(string, " No Accounting:");
        }
        else
        {
            sprintf(temp, " %.0f minutes:", lBuf.credits);
            strcat(string, temp);
        }
    }

    if (lBuf.lbflags.PERMANENT) strcat(string, " Permanent Log Entry:");
    if (lBuf.lbflags.NETUSER)   strcat(string, " Network User:");
    if (lBuf.lbflags.PROBLEM)   strcat(string, " Problem User:");
    if (lBuf.lbflags.NOMAIL)    strcat(string, " No Mail:");
    if (lBuf.VERIFIED)          strcat(string, " Un-Verified:");
    
    trap(string, T_SYSOP);

    /* see if it is us: */
    if (loggedIn  &&  editSelf)
    {
        /* move it back */
        logBuf = lBuf;

        /* make our environment match */
        setsysconfig();
    }
            
    putLog(&lBuf, logNo);
    logTab[ ltabSlot ].permanent = (BOOL)lBuf.lbflags.PERMANENT;
    logTab[ ltabSlot ].ltnmhash  = hash(lBuf.lbname);
}

/* -------------------------------------------------------------------- */
/*  configure()     sets user configuration via menu                    */
/* -------------------------------------------------------------------- */
void configure(BOOL new)
{
    BOOL    prtMess = TRUE;
    BOOL    quit    = FALSE;
    int     c;
    label   temp;
    char    oldEcho;

    doCR();

    setlogconfig();
    _fmemmove(&lBuf, &logBuf, sizeof(struct logBuffer));

    do 
    {
        if (prtMess)
        {
            doCR();
            outFlag = OUTOK;
            mPrintf("<3W0> Screen 3W0idth...... %d", termWidth); doCR();
            mPrintf("<3L0> 3L0ines per Screen.. %s", logBuf.linesScreen 
                                   ? itoa(logBuf.linesScreen, temp, 10) : 
                                   "Screen Pause Off");
                                   doCR();
            mPrintf("<3~0> Terminal Type..... %s", 
                                           ansiOn ? "ANSI-BBS" : "Off"); 
                                           doCR();
#if 1
            mPrintf("<3!0> IBM graphics chrs. %s",
                                           logBuf.IBMGRAPH ? "On" : "Off");
                                           doCR();
#endif
            mPrintf("<3H0> 3H0elpful Hints..... %s", 
                                           !expert ? "On" : "Off"); 
                                           doCR();
            mPrintf("<3U0> List in 3u0serlog... %s", 
                                           !unlisted ? "Yes" : "No"); 
                                           doCR();
            mPrintf("<3O0> Last 3O0ld on New... %s",
                                           oldToo ? "On" : "Off");  
                                           doCR();
            mPrintf("<3R0> 3R0oom descriptions. %s",
                                           roomtell ? "On" : "Off"); 
                                           doCR();
#if 1
           if (cfg.surnames || cfg.netsurname || cfg.titles || cfg.nettitles)
           {
                mPrintf("<3T0> 3T0itles/surnames... %s",
                                           logBuf.DISPLAYTS ? "On" : "Off");
                                           doCR();
            }
            mPrintf("<3J0> Sub3j0ects.......... %s",
                                           logBuf.SUBJECTS ? "On" : "Off");
                                           doCR();
            mPrintf("<3G0> Si3g0natures........ %s",
                                           logBuf.SIGNATURES ? "On" : "Off");
                                           doCR();
#endif
            mPrintf("<3X0> Auto-ne3x0t hall.... %s", 
                                           logBuf.NEXTHALL ? "On" : "Off");    
                                           doCR();
            mPrintf("<3C0> Upper3c0ase only.... %s",
                                           termUpper ? "On" : "Off");
                                           doCR();
            mPrintf("<3F0> Line3f0eeds......... %s", 
                                           termLF ? "On" : "Off");  
                                           doCR();
            mPrintf("<3B0> Ta3b0s.............. %s", 
                                           termTab ? "On" : "Off"); 
                                           doCR();
            mPrintf("<3N0> 3N0ulls............. %s", 
                                           termNulls ?
                                           itoa(termNulls, temp, 10) : 
                                           "Off"); doCR();
            
            if (!new)
            {
                doCR();
                mPrintf("<3S0> to save, <3A0> to abort."); doCR();
            }
            prtMess = (BOOL)(!expert);
        }

        if (new)
        {
            if (getYesNo("Is this OK", 1))
            {
                quit = TRUE;
                continue;
            }
            new = FALSE;
        }

        outFlag = IMPERVIOUS;

        doCR();
        mPrintf("2Change:0 ");
        
        oldEcho = echo;
        echo    = NEITHER;
        c       = iChar();
        echo    = oldEcho;

        if (!((whichIO == CONSOLE) || gotCarrier()))
            return;

        switch(toupper(c))
        {
        case '~':
            ansiOn = (BOOL)(!ansiOn);
            mPrintf("Terminal Emulation %s", ansiOn ? "On" : "Off"); doCR();
            break;

        case '!':
            logBuf.IBMGRAPH = (BOOL)(!logBuf.IBMGRAPH);
            mPrintf("IBM graphics chars %s", logBuf.IBMGRAPH ? "On" : "Off"); 
            doCR();
            break;

        case 'A':
            mPrintf("Abort"); doCR();
            if (getYesNo("Abort changes", 1))
            {
                _fmemmove(&logBuf, &lBuf, sizeof(struct logBuffer));
                setsysconfig();
                return;
            }
            break;

        case 'B':
            termTab = (BOOL)(!termTab);
            mPrintf("Tabs %s", termTab ? "On" : "Off"); doCR();
            break;

        case 'C':
            termUpper = (BOOL)(!termUpper);
            mPrintf("Uppercase only %s", termUpper ? "On" : "Off"); doCR();
            break;

        case 'F':
            termLF = (BOOL)(!termLF);
            mPrintf("Linefeeds %s", termLF ? "On" : "Off");  doCR();
            break;

        case 'G':
            logBuf.SIGNATURES = (BOOL)(!logBuf.SIGNATURES);
            mPrintf("D%sisplay signatures", logBuf.SIGNATURES
                                            ? "" : "o not d");
            doCR();
            break;

        case 'H':
            expert = (BOOL)(!expert);
            mPrintf("Helpful Hints %s", !expert ? "On" : "Off"); doCR();
            prtMess = (BOOL)(!expert);
            break;

        case 'J':
            logBuf.SUBJECTS = (BOOL)(!logBuf.SUBJECTS);
            mPrintf("D%sisplay subjects", logBuf.SUBJECTS ? "" : "o not d");
            doCR();
            break;

        case 'L':
            if (!logBuf.linesScreen)
            {
                mPrintf("Pause on full screen"); doCR();
                logBuf.linesScreen =
                    (uchar) getNumber("Lines per screen", 10L, 80L, 21L);
            }
            else
            {
                mPrintf("Pause on full screen off"); doCR();
                logBuf.linesScreen = 0;
            }
            break;
              
        case 'N':
            if (!termNulls)
            {
                mPrintf("Nulls"); doCR();
                termNulls = (uchar) getNumber("number of Nulls", 0L, 255L, 5L);
            }
            else
            {
                mPrintf("Nulls off"); doCR();
                termNulls = 0;
            }
            break;

        case 'O':
            oldToo = (BOOL)(!oldToo);
            mPrintf("Last Old on New %s", oldToo ? "On" : "Off");  doCR();
            break;

        case 'R':
            roomtell = (BOOL)(!roomtell);
            mPrintf("Room descriptions %s", roomtell ? "On" : "Off"); doCR();
            break;

        case 'S':
            mPrintf("Save changes"); doCR();
            if (getYesNo("Save changes", 1))
            {
                quit = TRUE;
            }
            break;

        case 'T':
            if (!(cfg.surnames || cfg.netsurname
                  || cfg.titles || cfg.nettitles))
                break;
            if (logBuf.DISPLAYTS && (logBuf.title[0] || logBuf.surname[0]))
            {
                if (!getYesNo("Nullify title and surname", 1))
                    break;
                logBuf.title[0] = '\0';
                logBuf.surname[0] = '\0';
                doCR();
            }
            logBuf.DISPLAYTS = (BOOL)(!logBuf.DISPLAYTS);
            mPrintf("D%sisplay titles/surnames", logBuf.DISPLAYTS
                     ? "" : "o not d");
            doCR();
            break;
       
        case 'U':
            unlisted = (BOOL)(!unlisted);
            mPrintf("List in userlog %s", !unlisted ? "Yes" : "No"); doCR();
            break;

        case 'W':
            mPrintf("Screen Width"); doCR();
            termWidth = 
                (uchar)getNumber("Screen width", 10l, 255l,(long)termWidth);
            /* kludge for carr-loss */
            if (termWidth < 10) termWidth = cfg.width;
            break;

        case 'X':
            logBuf.NEXTHALL = (BOOL)(!logBuf.NEXTHALL);
            mPrintf("Auto-next hall %s", logBuf.NEXTHALL ? "On" : "Off"); 
                doCR();
            break;

        case '\r':
        case '\n':
        case '?':
            mPrintf("Menu"); doCR();
            prtMess = TRUE;
            break;

        default:
            mPrintf("%c ? for help", c); doCR();
            break;
        }
    
    } while (!quit);
}
