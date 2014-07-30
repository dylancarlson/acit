/* -------------------------------------------------------------------- */
/*  MDM.C                         ACit                         91Sep27  */
/*  High level modem code, should not need to be changed for porting(?) */
/* -------------------------------------------------------------------- */

#include <conio.h>
#include <string.h>
#include <time.h>
#include "ctdl.h"
#include "prot.h"
#include "glob.h"

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/*  carrier()       checks carrier                                      */
/*  carrdetect()    sets global flags for carrier detect                */
/* $carrloss()      sets global flags for carrier loss                  */
/* $checkCR()       Checks for CRs from the data port for half a second.*/
/*  domcr()         print cr on modem, nulls and lf's if needed         */
/* $findbaud()      Finds the baud from sysop and user supplied data.   */
/* $fkey()          Deals with function keys from console               */
/*  KBReady()       returns TRUE if a console char is ready             */
/*  offhook()       sysop fn: to take modem off hook                    */
/*  outstring()     push a string directly to the modem                 */
/* $verbosebaud()   sets baud rate according to verbodse codes          */
/*  getModStr()     get a string from the modem, waiting for upto 3 secs*/
/* -------------------------------------------------------------------- */

static void carrloss(void);
static BOOL checkCR(void);
static int findbaud(void);
static void fkey(void);
static void verbosebaud(void);

/* -------------------------------------------------------------------- */
/*  carrier()       checks carrier                                      */
/* -------------------------------------------------------------------- */
int carrier(void)
{
    unsigned char c;

    if ( 
           (c=(uchar)gotCarrier()) != (unsigned char)haveCarrier
        && (!detectflag)
       )
    {
        /* carrier changed   */
        if (c) /* carrier present   */
        {
            switch(cfg.dumbmodem)
            {
            case 0:     /* numeric */
                /* do not use this rutine for carrier detect */
                return (1);

            case 1:     /* returns */
                if (!findbaud())
                {
                    Initport();
                    return TRUE;
                }
                break;

            case 2:     /* HS on RI */
                baud(ringdetect());
                break;

            case 3:     /* verbose */
                verbosebaud();
                break;

            default:
            case 4:     /* forced */
                baud(cfg.initbaud);
                break;
            }

            if (!(whichIO == CONSOLE)) 
            {
                carrdetect();
                detectflag = FALSE;
                return(0);
            }
            else 
            {
                detectflag = TRUE;
                update25();
                return(1);
            }
        } 
        else
        {
            pause(200);                 /* confirm it's not a glitch */
            if (!gotCarrier())          /* check again */
            {    
                carrloss();

                return(0);
            }
        }
    }
    return(1);
}

/* -------------------------------------------------------------------- */
/*  carrdetect()    sets global flags for carrier detect                */
/* -------------------------------------------------------------------- */
void carrdetect(void)
{
    char temp[30];
    
    warned          = FALSE;

    haveCarrier     = TRUE;
    newCarrier      = TRUE;
    justLostCarrier = FALSE;

    time(&conntimestamp);

    connectcls();
    update25();

    sprintf(temp, "Carrier-Detect (%d)", bauds[speed]);
    trap(temp,  T_CARRIER);

    logBuf.credits = cfg.unlogbal;
}

/* -------------------------------------------------------------------- */
/*  carrloss()      sets global flags for carrier loss                  */
/* -------------------------------------------------------------------- */
static void carrloss(void)
{
    outFlag         = OUTSKIP;

    haveCarrier     = FALSE;
    newCarrier      = FALSE;
    justLostCarrier = TRUE;

    Initport();

    trap("Carrier-Loss", T_CARRIER);
}

/* -------------------------------------------------------------------- */
/*  checkCR()       Checks for CRs from the data port for half a second.*/
/* -------------------------------------------------------------------- */
static BOOL checkCR(void)
{
    int i;

    for (i = 0; i < 50; i++)
    {
        pause(1);
        if (MIReady()) if (getMod() == '\r') return FALSE;
    }
    return TRUE;
}

/* -------------------------------------------------------------------- */
/*  domcr()         print cr on modem, nulls and lf's if needed         */
/* -------------------------------------------------------------------- */
void domcr(void)
{
    int i;

    outMod('\r');
    for (i = termNulls;  i;  i--) outMod(0);
    if (termLF) outMod('\n');
}

/* -------------------------------------------------------------------- */
/*  findbaud()      Finds the baud from sysop and user supplied data.   */
/* -------------------------------------------------------------------- */
static int findbaud(void)
{
    char noGood = TRUE;
    int  Time = 0;
    int  baudRunner;                    /* Only try for 60 seconds      */

    while (MIReady())   getMod();               /* Clear garbage        */
    baudRunner = 0;
    while (gotCarrier() && noGood && Time < 120)
    {
        Time++;
        baud(baudRunner);
        noGood = checkCR();
        if (noGood) baudRunner = (baudRunner + 1) % (3 /* 2400 too */);
    }
    return !noGood;
}

/* -------------------------------------------------------------------- */
/*  fkey()          Deals with function keys from console               */
/* -------------------------------------------------------------------- */
static void fkey(void)
{            
    char key;
    int oldIO, i, oldDowhat; 
    label string;

    #define F1     59
    #define F2     60
    #define F3     61
    #define F4     62
    #define F5     63
    #define F6     64
    #define S_F6   89
    #define A_F6   109
    #define F7     65
    #define F8     66
    #define F9     67
    #define F10    68
    #define ALT_P  25
    #define ALT_D  32
    #define ALT_B  48
    #define ALT_L  38
    #define ALT_T  20
    #define ALT_X  45
    #define ALT_C  46
    #define ALT_E  18
    #define ALT_Z  44

    key = (char)getch();

    if (strcmpi(cfg.f6pass, "f6disabled") != SAMESTRING)    
    if (ConLock == TRUE && key == ALT_L &&
        strcmpi(cfg.f6pass, "disabled") != SAMESTRING)
    {
        ConLock = FALSE;

        oldIO = whichIO;
        whichIO = CONSOLE;
        /* onConsole = TRUE; */
        update25();
        string[0] = 0;
        getNormStr("System Password", string, NAMESIZE, NO_ECHO);
        if (strcmpi(string, cfg.f6pass) != SAMESTRING)
            ConLock = TRUE;
        whichIO = (BOOL)oldIO;
        /* onConsole = (BOOL)(whichIO == CONSOLE);  */
        update25();
        givePrompt();
        return;
    }

    if (ConLock && !sysop && strcmpi(cfg.f6pass, "f6disabled") != SAMESTRING)
        return;

    switch(key)
    {
    case F1:
        drop_dtr();
        detectflag = FALSE;
        break;

    case F2:
        Initport();
        detectflag = FALSE;
        break;

    case F3:
        sysReq = (BOOL)(!sysReq);
        break;

    case F4:
        ScreenFree();
        anyEcho = (BOOL)(!anyEcho);
        break;

    case F5: 
        if  (whichIO == CONSOLE) whichIO = MODEM;
        else                     whichIO = CONSOLE;

        /* onConsole = (BOOL)(whichIO == CONSOLE); */
        break;

    case S_F6:
        if (!ConLock)
            aide = (BOOL)(!aide);
        break;

    case A_F6:
        if (!ConLock)
            sysop = (BOOL)(!sysop);
        break;

    case F6:
        if (sysop || !ConLock)
            sysopkey = TRUE;
        break;

    case F7:
        cfg.noBells = !cfg.noBells;
        break;

    case ALT_C:
    case F8:
        chatkey = (BOOL)(!chatkey);   /* will go into chat from main() */
        break;

    case F9:
        cfg.noChat = !cfg.noChat;
        chatReq = FALSE;
        break;
    
    case F10:
        help();
        break;

    case ALT_B:
        backout = (BOOL)(!backout);
        break;

    case ALT_D:
        debug = (BOOL)(!debug);
        break;

    case ALT_E:
        eventkey = TRUE;
        break;

    case ALT_L:
        if (cfg.f6pass[0] && strcmpi(cfg.f6pass, "f6disabled") != SAMESTRING)
          ConLock = (BOOL)(!ConLock);
        break;

    case ALT_P:
        if (printing)
        {
            printing=FALSE;
            fclose(printfile);
        }else{
            printfile=fopen(cfg.printer, "a");
            if (printfile)
            {
                printing=TRUE;
            } else {
                printing=FALSE;
                fclose(printfile);
            }
        }
        break;

    case ALT_T:
        if (haveCarrier)
            twit = (BOOL)(!twit);
        break;

    case ALT_X:
        if (dowhat == MAINMENU || dowhat == SYSOPMENU)
        {
            oldDowhat = dowhat;
            
            if (loggedIn)
            {
                i = getYesNo("Exit to MS-DOS", 0);
            }
            else
            {
                doCR();
                doCR();
                mPrintf("Exit to MS-DOS"); doCR();
                i = TRUE;
            }

            dowhat = (char)oldDowhat;
            
            if (!i)
            {
                if (dowhat == MAINMENU)
                {
                    givePrompt();
                }else{
                    doCR();
                    mPrintf("2Privileged function:0 ");
                }
                break;
            }
            ExitToMsdos = TRUE;
        }
        break;

    case ALT_Z:
        sleepkey = TRUE;
        break;

    default:
        break;
    }

    update25();
}

/* -------------------------------------------------------------------- */
/*  KBReady()       returns TRUE if a console char is ready             */
/* -------------------------------------------------------------------- */
BOOL KBReady(void)
{
    int c;

    if (getkey) return(TRUE);
  
    if (kbhit())
    {
        c = getch();
 
        if (!c)
        {
            fkey();
            return(FALSE);
        }
        else ungetch(c);

        getkey = 1;
       
        return(TRUE);
    }
    else return(FALSE);
}

/* -------------------------------------------------------------------- */
/*  offhook()       sysop fn: to take modem off hook                    */
/* -------------------------------------------------------------------- */
void offhook(void)
{
    Initport();
    outstring("ATM0H1\r");
}

/* -------------------------------------------------------------------- */
/*  outstring()     push a string directly to the modem                 */
/* -------------------------------------------------------------------- */
void outstring(const char *string)
{
    int mtmp;

    mtmp = modem;
    modem = TRUE;

    while(*string)
    {
        outMod(*string++);  /* output string */
    }

    modem = (uchar)mtmp;
}

/* -------------------------------------------------------------------- */
/*  verbosebaud()   sets baud rate according to verbodse codes          */
/* -------------------------------------------------------------------- */
static void verbosebaud(void)
{
    char c, f=0;
    long t;

    if (debug)
        outCon('[');

    time(&t);
    
    while(gotCarrier() && time(NULL) < (t + 4) && !KBReady())
    {
        if(MIReady())
        {
            c = (char)getMod();
        } else {
            c = 0;
        }

        if (debug && c)
        {
            outCon(c);
        }

        if (f)
        {
            switch(c)
            {
            case '\n':
            case '\r':  /* CONNECT */
                baud(0);
                if (debug)
                    outCon(']');
                return;
            case '1':   /* CONNECT 1200 */
                baud(1);
                if (debug)
                    outCon(']');
                return;
            case '2':   /* CONNECT 2400 */
                baud(2);
                if (debug)
                    outCon(']');
                return;
            case '4':   /* CONNECT 4800 */
                baud(3);
                if (debug)
                    outCon(']');
                return;
            case '9':   /* CONNECT 9600 */
                baud(4);
                if (debug)
                    outCon(']');
                return;
            default:
                break;
            }
        }

        if (c == 'C')
        {
            if (debug)
            {
                outCon('@');
            }
            f = 1;
        }
    }
}

/* -------------------------------------------------------------------- */
/*  getModStr()     get a string from the modem, waiting for upto 3 secs*/
/*                  for it. Returns TRUE if it gets one.                */
/* -------------------------------------------------------------------- */
int getModStr(char *str)
{
    long tm;
    int  l = 0, c;

    time(&tm);

    if (debug) cPrintf("[");

    while (
             (time(NULL) - tm) < 4 
          && !KBReady() 
          && l < 40 
          )
    {
        if (MIReady())
        {
            c = getMod();

            if (c == 13 || c == 10) /* CR || LF */
            {
                str[l] = 0 /* NULL */;
                if (debug) cPrintf("]\n");
                return TRUE;
            }else{
                if (debug) cPrintf("%c", c);
                str[l] = (char)c;
                l++;
            }
        }
    }

    if (debug) cPrintf(":F]\n");

    str[0] = 0 /* NULL */;

    return FALSE;
}
