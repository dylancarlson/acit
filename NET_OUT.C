/* -------------------------------------------------------------------- */
/*  NET_OUT.C                     ACit                         91Sep30  */
/*      Networking libs for the Citadel bulletin board system           */
/* -------------------------------------------------------------------- */

#include <conio.h>     /* getch() */
#include <dos.h>
#include <string.h>
#include <time.h>      /* time() */
#include "ctdl.h"
#include "prot.h"
#include "glob.h"

#ifdef NETWORK
/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/* $n_dial()        call the bbs in the node buffer                     */
/* $n_login()       Login to the bbs with the macro in the node file    */
/*  net_callout()   Entry point from Cron.C                             */
/*  net_master()    entry point to call a node                          */
/* $wait_for()      wait for a string to come from the modem            */
/* -------------------------------------------------------------------- */

static BOOL n_dial(void);
static BOOL n_login(void);
static BOOL wait_for(const char *str);

/* -------------------------------------------------------------------- */
/*  n_dial()        call the bbs in the node buffer                     */
/* -------------------------------------------------------------------- */
static BOOL n_dial(void)
{
    long ts;
    char str[40];
    char ch;

    cPrintf("\n \n Dialing...");

    if (debug) cPrintf("(%s%s)", cfg.dialpref, node.ndphone);
    
    baud(node.ndbaud);
    update25();

    outstring(cfg.dialsetup);
    outstring("\r\n");

    pause(100);
  
    strcpy(str, cfg.dialpref);
    strcat(str, node.ndphone);
    outstring(str);
    outstring("\r\n");

    time(&ts);
  
    for(;;) /* while(TRUE) */
    {
        if ((int)(time(NULL) - ts) > node.nddialto)  /* Timeout */
            break;

        if (KBReady())                             /* Aborted by user */
        {
            getch();
            getkey = 0;
            break;
        }

        if (gotCarrier())                          /* got carrier!  */ 
        {
            ansiattr = cfg.wattr;
            cPrintf("success");
            ansiattr = cfg.attr;
            return TRUE;
        }
#if 1

        if (MIReady())
        {
            ch = (char)getMod();
            if (debug) outCon(ch);
        }
#endif
    } 

    while (MIReady())
    {
        ch = (char)getMod();
        if (debug) outCon(ch);
    }

    ansiattr = cfg.wattr;
    cPrintf("failed");
    ansiattr = cfg.attr;

    return FALSE;
}

/* -------------------------------------------------------------------- */
/*  n_login()       Login to the bbs with the macro in the node file    */
/* -------------------------------------------------------------------- */
static BOOL n_login(void)
{
    union REGS in, out;
    register int ptime, j=0, k;
    char ch;
    int i, count;
    char *words[256];

    cPrintf("\n Logging in...");

    count = parse_it( words, node.ndlogin);

    i = 0;

    while(++i < count)
    {
        switch(tolower(*words[i++]))
        {     
            case 'p':
                if (debug)
                {
                    ansiattr = cfg.wattr;
                    cPrintf(" Pause For (%s)", words[i]);
                    ansiattr = cfg.attr;
                }
                ptime=atoi(words[i]);
                in.h.ah=0x2C;
                intdos(&in, &out);
                k = out.h.dl/10;
                while(j < ptime)
                {
                    in.h.ah=0x2C;
                    intdos(&in, &out);
                    if(out.h.dl/10 < k)
                        j += (10+(out.h.dl/10))-k;
                    else
                        j += (out.h.dl/10)-k;
                    k = out.h.dl/10;
                    if (MIReady())
                    {
                        ch = (char)getMod();
                        if (debug) outCon(ch);
                    }
                }
                break;
            case 's':
                if (debug)
                {
                    ansiattr = cfg.wattr;
                    cPrintf(" Send (%s)", words[i]);
                    ansiattr = cfg.attr;
                }
                outstring(words[i]);
                break;
            case 'w':
                if (debug) 
                {
                    ansiattr = cfg.wattr;
                    cPrintf(" Wait for (%s)", words[i]);
                    ansiattr = cfg.attr;
                }
                if (!wait_for(words[i]))
                {
                    ansiattr = cfg.wattr;
                    cPrintf(" Failed");
                    ansiattr = cfg.attr;
                    return FALSE;
                }
                break;
            case '!':
                apsystem(words[i]);
                break;
            default:
#if 1
                ansiattr = (uchar)(cfg.cattr | 128);
                cPrintf(" Invalid login word (%s)", words[i]);
                ansiattr = cfg.attr;
#endif
                break;
        }
    }
    ansiattr = cfg.wattr;
    cPrintf("success");
    ansiattr = cfg.attr;
    doccr();
    doccr();
    return TRUE;
}

/* -------------------------------------------------------------------- */
/*  net_callout()   Entry point from Cron.C                             */
/* -------------------------------------------------------------------- */
BOOL net_callout(const char *node)
{
    int slot;
    int tmp;

    /* login user */

    mread = 0; entered = 0;

    slot = personexists(node);

    if (slot == ERROR)
    {
        cPrintf("\n No such node in userlog!");
        return FALSE;
    }

    getLog(&logBuf, logTab[slot].ltlogSlot);

    thisSlot = slot;
    thisLog = logTab[slot].ltlogSlot;
 
    loggedIn    = TRUE;
    setsysconfig();
    setgroupgen();
    setroomgen();
    setlbvisit();

    update25();

    sprintf( msgBuf->mbtext, "NetCallout %s", logBuf.lbname);
    trap(msgBuf->mbtext, T_NETWORK);

    /* node logged in */
     
    tmp = net_master();

    /* terminate user */

    if (tmp == TRUE)
    { 
        logBuf.callno      = cfg.callno;
        time(&logtimestamp);
        logBuf.calltime    = logtimestamp;
        logBuf.lbvisit[0]  = cfg.newest;
        logTab[0].ltcallno = cfg.callno;

        slideLTab(thisSlot);
        cfg.callno++;

        storeLog();
        loggedIn = FALSE;

        /* trap it */
        sprintf(msgBuf->mbtext, "NetLogout %s (succeded)", logBuf.lbname);
        trap(msgBuf->mbtext, T_NETWORK);

        outFlag = IMPERVIOUS;
        cPrintf("Networked with \"%s\"\n ", logBuf.lbname);

        if (cfg.accounting)  unlogthisAccount();
        heldMessage = FALSE;
        cleargroupgen();
        initroomgen();

        *logBuf.lbname = '\0';

        setalloldrooms();

        sprintf(msgBuf->mbtext, "  ----- %4d messages entered", entered);
        trap(msgBuf->mbtext, T_NETWORK);

        sprintf(msgBuf->mbtext, "  ----- %4d messages read",  mread);
        trap(msgBuf->mbtext, T_NETWORK);

        sprintf(msgBuf->mbtext, "  ----- %4d messages expired",  xpd);
        trap(msgBuf->mbtext, T_NETWORK);

        sprintf(msgBuf->mbtext, "  ----- %4d messages duplicate",  duplic);
        trap(msgBuf->mbtext, T_NETWORK);
    }
    else
    {
        loggedIn = FALSE;

        sprintf(msgBuf->mbtext, "NetLogout %s (failed)", logBuf.lbname);
        trap(msgBuf->mbtext, T_NETWORK);
    }

    setdefaultconfig();

    /* user terminated */
    /* onConsole       = FALSE; */
    callout         = FALSE;

    pause(100);
   
    Initport();

    return (BOOL)(tmp);
}

/* -------------------------------------------------------------------- */
/*  net_master()    entry point to call a node                          */
/* -------------------------------------------------------------------- */
BOOL net_master(void)
{
    if (!readnode())
    {
        cPrintf("\n No nodes.cit entry!\n ");
        return FALSE;
    }

    if (debug)
    {
      cPrintf("\nNode:  \"%s\" \"%s\"\n", node.ndname, node.ndregion);  
      cPrintf("Phone: \"%s\"   Timeout: %d\n", node.ndphone, node.nddialto);     
      cPrintf("Login: \"%s\"\n", node.ndlogin);     
      cPrintf("Baud:  %-4d    Protocol: \"%s\"\n", bauds[node.ndbaud],
              node.ndprotocol);
      cPrintf("Expire:%d    Waitout:  %d", node.ndexpire, node.ndwaitto);
      doccr();
    }
  
    if (!n_dial()) return FALSE;
    if (!n_login()) return FALSE;
    netError = FALSE;

    /* cleanup */
    changedir(cfg.temppath);
    ambigUnlink("room.*",   FALSE);
    ambigUnlink("roomin.*", FALSE);

    if (master())
    {
        if (slave())
        {
            cleanup();
            return TRUE;
        }
    }
    return FALSE;
}

/* -------------------------------------------------------------------- */
/*  wait_for()      wait for a string to come from the modem            */
/* -------------------------------------------------------------------- */
static BOOL wait_for(const char *str)
{
    char line[80];
    long st;
    int stl;
   
    stl = strlen(str);

    memset(line, sizeof(line), '\0');

    time(&st);
   
    while( (time(NULL) - st) <= (long)node.ndwaitto)
    {
        if (MIReady())
        {
            memcpy(line, line+1, stl);
            line[stl-1]  = (char) getMod();
            line[stl] = '\0';
            if (debug) outCon(line[stl-1]);
            if (strcmpi(line, str) == SAMESTRING) 
                return TRUE;
        }else{
            if (KBReady())                             /* Aborted by user */
            {
                getch();
                getkey = 0;
                return FALSE;
            }
        }
    }   
    return FALSE;
}   

#endif
