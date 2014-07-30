/* -------------------------------------------------------------------- */
/*  MISC.C                        ACit                         91Sep30  */
/*  Citadel garbage dump, if it aint elsewhere, its here.               */
/* -------------------------------------------------------------------- */

#define MISC

#include <bios.h>
#include <conio.h>
#include <dos.h>
#include <direct.h>
#include <io.h>
#include <malloc.h>
#include <signal.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include "ctdl.h"
#include "prot.h"
#include "glob.h"

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/*  crashout()      Fatal system error                                  */
/*  exitcitadel()   Done with cit, time to leave                        */
/*  filexists()     Does the file exist?                                */
/*  hash()          Make an int out of their name                       */
/*  ctrl_c()        Used to catch CTRL-Cs                               */
/*  initCitadel()   Load up data files and open everything.             */
/*  openFile()      Special to open a .cit file                         */
/*  trap()          Record a line to the trap file                      */
/*  SaveAideMess()  Save aide message from AIDEMSG.TMP                  */
/*  amPrintf()      aide message printf                                 */
/*  amZap()         Zap aide message being made                         */
/*  changedir()     changes curent drive and directory                  */
/*  ltoac()         change a long into a number with ','s in it         */
/*  editBorder()    edit a boarder line                                 */
/*  doBorder()      print a boarder line                                */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  crashout()      Fatal system error                                  */
/* -------------------------------------------------------------------- */
void crashout(const char *message)
{
    FILE *fd;           /* Record some crash data */

    Hangup();

    fcloseall();

    fd = fopen("crash.cit", "w");
    fprintf(fd, message);
    fclose(fd);

    writeTables();

    cfg.attr = 7;   /* exit with white letters */

    position(0,0);
    cPrintf("F\na\nt\na\nl\n \nS\ny\ns\nt\ne\nm\n \nC\nr\na\ns\nh\n");
    cPrintf(" %s\n", message);

    drop_dtr();

    portExit();

    _ffree((void far *)msgTab1);    msgTab1 = NULL;
    _ffree((void far *)msgTab2);    msgTab2 = NULL;
    _ffree((void far *)msgTab3);    msgTab3 = NULL;
    _ffree((void far *)msgTab4);    msgTab4 = NULL;
    _ffree((void far *)msgTab5);    msgTab5 = NULL;
    _ffree((void far *)msgTab6);    msgTab6 = NULL;
    _ffree((void far *)msgTab7);    msgTab7 = NULL;
    _ffree((void far *)msgTab8);    msgTab8 = NULL;
    _ffree((void far *)msgTab9);    msgTab9 = NULL;
    _ffree((void far *)logTab);     logTab = NULL;
    _ffree((void far *)extrn);      extrn = NULL;
    _ffree((void far *)othCmd);     othCmd = NULL;
    _ffree((void far *)roomTab);    roomTab = NULL;

    exit(1);
}

/* -------------------------------------------------------------------- */
/*  exitcitadel()   Done with cit, time to leave                        */
/* -------------------------------------------------------------------- */
void exitcitadel(void)
{
    if (loggedIn) terminate( /* hangUp == */ TRUE, FALSE);

    drop_dtr();        /* turn DTR off */

    putGroup();       /* save group table */
    putHall();        /* save hall table  */

    writeTables(); 

    trap("Citadel Terminated", T_SYSOP);

    /* close all files */
    fcloseall();

    cfg.attr = 7;   /* exit with white letters */
    cls();

    drop_dtr();

    portExit();

    _ffree((void far *)msgTab1);    msgTab1 = NULL;
    _ffree((void far *)msgTab2);    msgTab2 = NULL;
    _ffree((void far *)msgTab3);    msgTab3 = NULL;
    _ffree((void far *)msgTab4);    msgTab4 = NULL;
    _ffree((void far *)msgTab5);    msgTab5 = NULL;
    _ffree((void far *)msgTab6);    msgTab6 = NULL;
    _ffree((void far *)msgTab7);    msgTab7 = NULL;
    _ffree((void far *)msgTab8);    msgTab8 = NULL;
    _ffree((void far *)msgTab9);    msgTab9 = NULL;
    _ffree((void far *)logTab);     logTab = NULL;
    _ffree((void far *)extrn);      extrn = NULL;
    _ffree((void far *)othCmd);     othCmd = NULL;
    _ffree((void far *)roomTab);    roomTab = NULL;

    if (gmode() != 7)
    {
        outp(0x3d9,0);
    }

    exit(0);
}

/* -------------------------------------------------------------------- */
/*  filexists()     Does the file exist?                                */
/* -------------------------------------------------------------------- */
BOOL filexists(const char *filename)
{
    return (BOOL)((access(filename, 4) == 0) ? TRUE : FALSE);
}

/* -------------------------------------------------------------------- */
/*  hash()          Make an int out of their name                       */
/* -------------------------------------------------------------------- */
uint hash(const char *str)
{
    int  h, shift;

    for (h=shift=0;  *str;  shift=(shift+1)&7, str++)
    {
        h ^= (toupper(*str)) << shift;
    }
    return h;
}

/* -------------------------------------------------------------------- */
/*  ctrl_c()        Used to catch CTRL-Cs                               */
/* -------------------------------------------------------------------- */
void ctrl_c(void)
{
    extern volatile uchar far * const Row;

    signal(SIGINT, ctrl_c);
    position((uchar)((*Row) - 1), 19);
    ungetch('\r');
    getkey = TRUE;
}
 
/* -------------------------------------------------------------------- */
/*  initCitadel()   Load up data files and open everything.             */
/* -------------------------------------------------------------------- */
void initCitadel(void)
{
    /*FILE *fd, *fp;*/

    char *grpFile, *hallFile, *logFile, *msgFile, *roomFile;
    char scratch[80];

    /* lets ignore ^C's  */
    signal(SIGINT, ctrl_c);

    /* This sillyness opens two files and reads 1 byte from each so that */
    /* the 8K block will be allocated in front of our halloc'ed blocks.  */
    /* I tested opening the same file twice, it seems ok, as long as they*/
    /* are 2 different pointers. Also they are read only. Can't be sure  */
    /* that you can find any other files. They might be else where.      */

    chkptr(edit);
    if ((edit = _fcalloc(MAXEXTERN, sizeof(struct ext_editor))) == NULL)
    {
        crashout("Can not allocate external editors");
    }
    chkptr(hallBuf);
    if ((hallBuf = _fcalloc(1, sizeof(struct hallBuffer))) == NULL)
    {
        crashout("Can not allocate space for halls");
    }
    chkptr(msgBuf);
    if ((msgBuf = _fcalloc(1, sizeof(struct msgB))) == NULL)
    {
        crashout("Can not allocate space for message buffer 1");
    }
    chkptr(msgBuf2);
    if ((msgBuf2 = _fcalloc(1, sizeof(struct msgB))) == NULL)
    {
        crashout("Can not allocate space for message buffer 2");
    }
    chkptr(extrn);
    if ((extrn = _fcalloc(MAXEXTERN, sizeof(struct ext_prot))) == NULL)
    {
        crashout("Can not allocate space for external protocol");
    }
    chkptr(othCmd);
    if ((othCmd = _fcalloc(MAXEXTERN, sizeof(struct ext_other))) == NULL)
    {
        crashout("Can not allocate space for other extern commands");
    }
    chkptr(roomTab);
    if ((roomTab = _fcalloc(MAXROOMS, sizeof(struct rTable))) == NULL)
    {
        crashout("Can not allocate space for room table");
    }
    
    if (!readTables())
    {
        cPrintf("Etc.dat not found"); doccr();
        pause(300);
        cls();
        configcit();
    }

    if (changedir(cfg.temppath) == ERROR)
        crashout("Can't change to temppath");
    if (changedir(cfg.msgpath) == ERROR)
        crashout("Can't change to msgpath");
    if (changedir(cfg.helppath) == ERROR)
        crashout("Can't change to helppath");
    if (changedir(cfg.roompath) == ERROR)
        crashout("Can't change to roompath");
    if (changedir(cfg.aplpath) == ERROR)
        crashout("Can't change to aplpath");
    if (changedir(cfg.transpath) == ERROR)
        crashout("Can't change to transpath");
    if (changedir(cfg.dirpath) == ERROR)
        crashout("Can't change to dirpath");
    changedir(cfg.homepath);

    speed = 0;
    portInit();

    setscreen();

    update25();

    if (cfg.msgpath[ (strlen(cfg.msgpath) - 1) ]  == '\\')
        cfg.msgpath[ (strlen(cfg.msgpath) - 1) ]  =  '\0';

    sprintf(scratch, "%s\\%s", cfg.msgpath, "msg.dat");

    /* open message files: */
    grpFile     = "grp.dat" ;
    hallFile    = "hall.dat";
    logFile     = "log.dat" ;
    msgFile     =  scratch  ;
    roomFile    = "room.dat";

    openFile(grpFile,  &grpfl );
    openFile(hallFile, &hallfl);
    openFile(logFile,  &logfl );
    openFile(msgFile,  &msgfl );
    openFile(roomFile, &roomfl);

    /* open Trap file */
    trapfl = fopen(cfg.trapfile, "a+");

    trap("Citadel Started", T_SYSOP);

    getGroup();
    getHall();

    if (cfg.accounting)
    {
        readaccount();    /* read in accounting data */
    }
    readprotocols();
    readcron();

    getRoom(LOBBY);     /* load Lobby>  */
    Initport();
    Initport();
    whichIO = MODEM;

    /* record when we put system up */
    time(&uptimestamp);

    cls();
    setdefaultconfig();
    update25();
    setalloldrooms();
    roomtalley();
}

/* -------------------------------------------------------------------- */
/*  openFile()      Special to open a .cit file                         */
/* -------------------------------------------------------------------- */
void openFile(const char *filename, const FILE **fd)
{
    /* open message file */
    if ((*fd = fopen(filename, "r+b")) == NULL)
    {
        crashout(".DAT file missing!");
    }
}

/* -------------------------------------------------------------------- */
/*  trap()          Record a line to the trap file                      */
/* -------------------------------------------------------------------- */
void trap(const char *string, int what)
{
    char dtstr[20];

    /* check to see if we are supposed to log this event */
    if (!cfg.trapit[what])  return;

    sstrftime(dtstr, 19, "%y%b%D %X", 0l);

    fprintf(trapfl, "%s:  %s\n", dtstr, string);

    fflush(trapfl);
}

/* -------------------------------------------------------------------- */
/*  SaveAideMess()  Save aide message from AIDEMSG.TMP                  */
/* -------------------------------------------------------------------- */
void SaveAideMess(void)
{
    char temp[90];
    FILE *fd;

    /*
     * Close curent aide message (if any)
     */
    if (aideFl == NULL)
    {
        return;
    }
    fclose(aideFl);
    aideFl = NULL;

    clearmsgbuf();

    /*
     * Read the aide message
     */
    sprintf(temp, "%s\\%s", cfg.temppath, "aidemsg.tmp");
    if ((fd  = fopen(temp, "rb")) == NULL)
    {
        crashout("AIDEMSG.TMP file not found during aide message save!");
    }
    GetFileMessage(fd, msgBuf->mbtext, cfg.maxtext);

    fclose(fd);
    unlink(temp);

    if (strlen(msgBuf->mbtext) < 10)
        return;

    strcpy(msgBuf->mbauth, cfg.nodeTitle);  

    msgBuf->mbroomno = AIDEROOM;

    putMessage();
    noteMessage();
}

/* -------------------------------------------------------------------- */
/*  amPrintf()      aide message printf                                 */
/* -------------------------------------------------------------------- */
void amPrintf(const char *fmt, ... )
{
    va_list ap;
    char temp[90];

    /* no message in progress? */
    if (aideFl == NULL)
    {
        sprintf(temp, "%s\\%s", cfg.temppath, "aidemsg.tmp");

        unlink(temp);
 
        if ((aideFl = fopen(temp, "w")) == NULL)
        {
            crashout("Can not open AIDEMSG.TMP!");
        }
    }

    va_start(ap, fmt);
    vfprintf(aideFl, fmt, ap);
    va_end(ap);

    fflush(aideFl);
}

/* -------------------------------------------------------------------- */
/*  amZap()         Zap aide message being made                         */
/* -------------------------------------------------------------------- */
void amZap(void)
{
    char temp[90];

    if (aideFl != NULL)
    {
        fclose(aideFl);
    }

    sprintf(temp, "%s\\%s", cfg.temppath, "aidemsg.tmp");

    unlink(temp);

    aideFl = NULL;
}

/* -------------------------------------------------------------------- */
/*  changedir()     changes curent drive and directory                  */
/* -------------------------------------------------------------------- */
int changedir(const char *path)
{
    union REGS REG;

#if 0
    /* uppercase   */ 
    path[0] = (char)toupper(path[0]);
#endif

    /* change disk */
    REG.h.ah = 0x0E;     /* select drive */
    REG.h.dl = (uchar)(toupper(path[0]) - 'A');
    intdos(&REG, &REG);

    /* change path */
    if (chdir(path)  == -1) return -1;

    return TRUE;
}

/* -------------------------------------------------------------------- */
/*  ltoac()         change a long into a number with ','s in it         */
/* -------------------------------------------------------------------- */
char *ltoac(long num)
{
    char s1[30];
    static char s2[40];
    int i, i2, i3, l;

    sprintf(s1, "%lu", num);

    l = strlen(s1);

    for (i = l, i2 = 0, i3 = 0; s1[i2]; i2++, i--)
    {
        if (!(i % 3) && i != l)
        {
            s2[i3++] = ',';
        }
        s2[i3++] = s1[i2];
    }

    s2[i3] = 0 /* NULL */;

    return s2;
}

/* -------------------------------------------------------------------- */
/*  editBorder()    edit a boarder line.                                */
/* -------------------------------------------------------------------- */
void editBorder(void)
{
    int i;

    doCR();
    doCR();
        
    if (!cfg.borders)
    {
        mPrintf(" Border lines not enabled!");
        doCR();
        return;
    }

    outFlag = OUTOK;
    
    for (i = 0; i < MAXBORDERS; i++)
    {
        mPrintf("Border %d:", i+1);
        if (*cfg.border[i])
        {
            doCR();
            mPrintf("%s", cfg.border[i]);
        }
        else
        {
            mPrintf(" Empty!"); 
        }
        doCR();
        doCR();
    }

    i = (int)getNumber("border line to change", 0L, (long)MAXBORDERS, 0L);

    if (i)
    {
        doCR();
        getString("border line", cfg.border[i-1], 80, FALSE, ECHO, "");
    }
}

/* -------------------------------------------------------------------- */
/*  doBorder()      print a boarder line.                               */
/* -------------------------------------------------------------------- */
void doBorder(void)
{
    static count = 0;
    static line  = 0;
    int    tries;

    if (count++ == 10)
    {
        count = 0;

        for (line == MAXBORDERS-1 ? line = 0 : line++, tries = 0; 
             tries < MAXBORDERS + 1;
             line == MAXBORDERS-1 ? line = 0 : line++, tries++)
        {
            if (*cfg.border[line])
            {
                doCR();
                mPrintf("%s", cfg.border[line]);
                break;
            }
        }
    }
}
