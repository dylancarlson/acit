/* -------------------------------------------------------------------- */
/*  MISC2.C                       ACit                         91Aug23  */
/* -------------------------------------------------------------------- */
/*                        Overlayed misc stuff                          */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Includes                                                            */
/* -------------------------------------------------------------------- */
#define MISC2
#include <malloc.h>
#include <bios.h>
#include <conio.h>
#include <string.h>
#include <time.h>
#include <dos.h>
#include "ctdl.h"
#include "prot.h"
#include "glob.h"


/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/*  systat()        System status                                       */
/*  chat()          This is the chat mode                               */
/*  ringSysop()     ring the sysop                                      */
/*  ringSystemREQ() signals a system request for 2 minutes.             */
/*  dial_out()      dial out to other boards                            */
/*  logo()          prints out logo screen and quote at start-up        */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  systat()        System status                                       */
/* -------------------------------------------------------------------- */
void systat(char verbose)
{
    union REGS r;
    int i;
    int tablesize;
    long average, work;
    char summary[250];
    char dtstr[80];
    int  public    = 0,
         private   = 0,
         group     = 0,
         problem   = 0,
         moderated = 0;

    outFlag = OUTOK;

    mPrintf("%s %s, %s ", softname, cfg.nodeTitle, cfg.nodeRegion
                                   );
    doCR();
    mPrintf(" Running Version %s Compiled on %s at %s",
              version, cmpDate, cmpTime);
    doCR();

    sstrftime(dtstr, 79, cfg.vdatestamp, 0l);
    mPrintf(" %s", dtstr);
    doCR();
    mPrintf(" Up time: ");  diffstamp(uptimestamp);

    if (gotCarrier())
    {
        doCR();
        mPrintf(" Connect time: ");  diffstamp(conntimestamp);
    }

    doCR();
    mPrintf(" Maximum of %d log entries,", cfg.MAXLOGTAB);

    mPrintf(" Call number %s", ltoac(cfg.callno));

    doCR();
    strcpy(dtstr, ltoac(cfg.newest - cfg.oldest + 1));
    mPrintf(" %s %s, numbered %lu to %lu",
        dtstr, cfg.msgs_nym, cfg.oldest, cfg.newest);

    tablesize = (int)sizetable();
    for( i = 0; i < tablesize; ++i)
    {
             if (msgTab1[i].mtmsgflags.PROBLEM  ) problem++;
             if (msgTab1[i].mtmsgflags.MODERATED) moderated++;
             if (msgTab1[i].mtmsgflags.LIMITED  ) group++  ;
       else  if (msgTab1[i].mtmsgflags.MAIL     ) private++;
       else                                      public++ ;
    }

    doCR();

    mPrintf(" There are a total of ");
    
    if ( (cfg.mtoldest - cfg.oldest) > 0 )
       mPrintf("%d %s missing, ", (int)(cfg.mtoldest - cfg.oldest), 
               cfg.msgs_nym);

    mPrintf
      ("%d public %s, %d private %s, %d moderated %s, ",
       public, cfg.msgs_nym, private, cfg.msgs_nym, moderated, cfg.msgs_nym);

    if (!aide) mPrintf("and ");

    mPrintf("%d group only %s", group, cfg.msgs_nym);

    if (aide)  mPrintf(", and %d problem user %s", problem, cfg.msgs_nym);

    doCR();
    mPrintf(" %dK %s space, ", cfg.messagek, cfg.msg_nym);

    if (cfg.oldest > 1)  work = (long)((long)cfg.messagek * 1024l);
    else                 work = cfg.catLoc;

    if (cfg.oldest > 1)  average = (work) / (cfg.newest - cfg.oldest + 1);
    else                 average = (work) / (cfg.newest);

    mPrintf("%s bytes average %s length", ltoac(average), cfg.msg_nym);

#if 1
    _fheapmin();
#endif

    if (sysop || verbose)
    {
        r.h.ah = 0x48;   /* allocate memory              */
        r.h.al = 0;
        r.x.bx = 0xffff; /* ask for an impossible amount */

        intdos(&r, &r);

        doCR();
        mPrintf(" DOS version %d.%d,", _osmajor, _osminor);
        mPrintf(" %uK system memory,", _bios_memsize());
        mPrintf(" %ld bytes free system memory.", (long)r.x.bx*16L);
    }

    doCR();
    mPrintf(" Hallway %s",hallBuf->hall[thisHall].hallname);

    if (hallBuf->hall[thisHall].owned) 
    {
        mPrintf(", owned by group %s",
            grpBuf.group[ hallBuf->hall[thisHall].grpno ].groupname);
    }

    doCR();
    formatSummary(summary);

    mPrintf("%s", summary);
#if 1
    doCR();
    mPrintf(" %d %s read, %d %s %s", mread, cfg.msgs_nym,
            entered, cfg.msgs_nym, cfg.msg_done);
#endif
    doCR();
    strcpy(dtstr, ltoac(received));
    mPrintf(" %s characters transmitted, %s characters received",
            ltoac(transmitted), dtstr );

    if (cfg.accounting && !logBuf.lbflags.NOACCOUNT)
    {
        doCR();

        if (!specialTime)
        {
            mPrintf(" You have %.0f %s left today.", logBuf.credits,
                ((int)logBuf.credits == 1) ? "minute":"minutes");
        } else {
            mPrintf(" You have unlimited time.");
        }
    }
    doCR();
    mPrintf(" %s", copyright[0]);
    doCR();
    mPrintf(" %s", copyright[1]);
    doCR();
}

/* -------------------------------------------------------------------- */
/*  chat()          This is the chat mode                               */
/* -------------------------------------------------------------------- */
void chat(void)
{
    int c, from = 0, lastfrom, wsize = 0, i;
    char word[50];

    chatkey = FALSE;
    chatReq = FALSE;

    if (!gotCarrier())
    {
        dial_out();
        return;
    }

    lastfrom = 2;

    outFlag = IMPERVIOUS;
    mPrintf("  Now in chat mode, Control-Z to exit.\n\n ");
    outFlag = OUTOK;

    do
    {
        c = 0;

        if (KBReady())
        {
            c = getch();
            getkey = 0;
            from = 0;
        }

        if (MIReady())
        {
            if (!(whichIO == CONSOLE))
            {
                c = getMod();
                from = 1;
            } else {
                getMod();
            }
        }
#if 0
        c = ( c & 0x7F );
        c = tfilter[c];
#else
        if (c < 128)
            c = filt_in[c];
#endif

        if (c && c != 26 /* CNTRLZ */)
        {
            if (from != lastfrom)
            {
                if (from)
                {
                    termCap(TERM_NORMAL);
                    ansiattr = cfg.attr;
                }
                else
                {
                    termCap(TERM_BOLD);
                    ansiattr = cfg.cattr;
                }
                lastfrom = from;
            }
            
            if (c == '\r' || c == '\n' || c == ' ' || c == '\t' || wsize == 50)
            {
                wsize = 0;
            } else {
                if (crtColumn >= (unsigned char)(termWidth-1))
                {
                    if (wsize)
                    {
                        for (i = 0; i < wsize; i++)
                            doBS();
                        doCR();
                        for (i = 0; i < wsize; i++)
                            echocharacter(word[i]);
                    } else {
                        doCR();
                    }

                    wsize = 0;
                } else {
                    word[wsize] = (char)c;
                    wsize ++;
                }
            }

            echocharacter((char)c);
        }
    } while ( (c != 26 /* CNTRLZ */) && gotCarrier() );

    time(&lasttime);
    termCap(TERM_NORMAL);
    ansiattr = cfg.attr;

    doCR();
}

/* -------------------------------------------------------------------- */
/*  ringSysop()     ring the sysop                                      */
/* -------------------------------------------------------------------- */
void ringSysop(void)
{
    char i;
    char answered = FALSE;
    int  oldBells;
    static char shave[] = { 10, 5, 5, 10, 20, 10, 75 };
    char ringlimit = 30;
    int j = 0;

    /* turn the ringer on */
    oldBells = cfg.noBells;
    cfg.noBells = FALSE;
    
    mPrintf("\n Ringing sysop.");

    answered = FALSE;

    for (i = 0; (i < ringlimit) && !answered && gotCarrier(); i++)
    {
        oChar(7 /* BELL */); 

        pause(shave[j]);

        j++;
        if (j ==7)
        {
            j = 0;
        }

        if (debug)
        {
            mPrintf("%d", j);
        }


        if (BBSCharReady() || KBReady())
        {
            answered = TRUE;
        }
    }

    cfg.noBells = oldBells;

    if (KBReady())  
    {
        chat();
    }
    else if (i >= ringlimit)
    {
        mPrintf("  Sorry, Sysop not around.\n ");
    }
    else iChar();
}

/* -------------------------------------------------------------------- */
/*  ringSystemREQ() signals a system request for 2 minutes.             */
/* -------------------------------------------------------------------- */
void ringSystemREQ(void)
{
    extern volatile char far * const Row;

    char i;
    char answered = FALSE;
    char ringlimit = 120;

    doccr();
    doccr();
    (*stringattr)((unsigned int)*Row," * System Available * ",(unsigned char)
        (cfg.wattr | 128));
    update25();
    doccr();

    answered = FALSE;
    for (i = 0; (i < ringlimit) && !answered; i++)
    {
        putch(7 /* BELL */);
        pause(80);
        if (KBReady()) answered = TRUE;
    }

    if (!KBReady() && i >= ringlimit)  Initport();

    update25();
}

/* -------------------------------------------------------------------- */
/*  dial_out()      dial out to other boards                            */
/* -------------------------------------------------------------------- */
void dial_out(void)
{
    char con, mod;
    char fname[61];

    fname[0] = 60;

    outFlag = IMPERVIOUS;
    cPrintf("  Now in dial out mode, Ctrl+Q to exit, Ctrl+N to net,\n"
            " Ctrl+B for baud rate change, Ctrl+E for shell escape.\n \n ");
    outFlag = OUTOK;

    Hangup();

    disabled = FALSE;
 
    baud(cfg.initbaud);

    update25();

    outstring(cfg.dialsetup);
    outstring("\r");

    pause(100);

    callout = TRUE;

    con = mod = 0;

    do 
    {
        if (KBReady())
        {
            con = (char)getch();
            getkey = 0;
            if (debug) oChar((char)con);
            if (con != 17 && con != 2 && con != 5
                          && con != 14)
                outMod(con);
            else
            switch(con)
            {
            case  2:
                    mPrintf("New baud rate [0-4]: ");
                    con = (char)getch();
                    doccr();
                    if (con > ('0' - 1) && con < '5')
                        baud(con-48);
                    update25();
                    break;

            case  5:
                    if (sysop || !ConLock)
                        shellescape(FALSE);
                    break;

#ifdef NETWORK
            case 14:
                    readnode();
                    master();
                    slave();
                    cleanup();
                    break;
#endif
#if 1
            case 17:  /* ^Q */
                    continue;
#endif

            default:
                    break;
            }
            con = 0;
        }

        if (MIReady())
        {
            mod = (char)getMod();

            if (debug)
            {
              mod = (char)( mod & 0x7F );
              mod = filt_in[mod];
            }

            if (mod == '\n')
              doccr();
            else
              if (mod != '\r')
                oChar(mod);
            mod = 0;
        }
    } while (con != 17 /* ctrl-q */);
    
    callout = FALSE;

    Initport();
#if 1
    if (cfg.offhook)
        offhook();
#endif

    doCR();
}

/*----------------------------------------------------------------------*/
/*      logo()  prints out system logo at startup                       */
/*----------------------------------------------------------------------*/
void logo()
{
    int i;
    cls();

#ifdef GOODBYE
    for (i=0; welcome[i]; i++)
        if(welcomecrcs[i] != stringcrc(welcome[i]))
            crashout("Some ASSHOLE tampered with the welcome message!");

    for (i=0; copyright[i]; i++)
        if(copycrcs[i] != stringcrc(copyright[i]))
            crashout("Some ASSHOLE tampered with the Copyright message!");
#endif

    position(5, 0);

    cCPrintf("%s v%s", softname, version);
    doccr();
    if (strlen(testsite))
    {
        cCPrintf(testsite);
    }
    doccr();
    cCPrintf("Host is IBM, (Microsoft C)");

    position(9, 0);

    for (i=0; welcome[i]; i++)
    {
        cCPrintf(welcome[i]);
        doccr();
    }

    position(19, 0);

    for (i=0; copyright[i]; i++)
    {
        cCPrintf(copyright[i]);
        doccr();
    }
    
    doccr();
}
