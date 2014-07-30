/************************************************************************/
/*  CFG.C                         ACit                         91Sep30  */
/*      configuration program for Citadel bulletin board system.        */
/************************************************************************/

#define CFG

#include <direct.h>
#include <string.h>
#include <stdarg.h>
#include "ctdl.h"
#include "prot.h"
#include "key.h"
#include "glob.h"

/************************************************************************/
/*                              Contents                                */
/* $buildcopies()           copies appropriate msg index members        */
/* $buildhalls()            builds hall-table (all rooms in Maint.)     */
/* $clearaccount()          sets all group accounting data to zero      */
/*  configcit()             the main configuration for citadel          */
/*  illegal()               abort config.exe program                    */
/* $initfiles()             opens & initalizes any missing files        */
/* $logInit()               indexes log.dat                             */
/* $logSort()               Sorts 2 entries in logTab                   */
/* $msgInit()               builds message table from msg.dat           */
/*  readaccount()           reads grpdata.cit values into grp struct    */
/* $readconfig()            reads config.cit values                     */
/*  readprotocols()         reads external.cit values                   */
/* $RoomTabBld()            builds room.tab, index's room.dat           */
/* $showtypemsg()           displays what kind of message was read      */
/* $slidemsgTab()           frees slots at beginning of msg table       */
/* $zapGrpFile()            initializes grp.dat                         */
/* $zapHallFile()           initializes hall.dat                        */
/* $zapLogFile()            initializes log.dat                         */
/* $zapMsgFile()            initializes msg.dat                         */
/* $zapRoomFile()           initializes room.dat                        */
/************************************************************************/
          
#if 1
static  char *toolong = "%s parameter (%s) too long,"
                        " must be less than %d characters";
#endif

static void buildcopies(void);
static void buildhalls(void);
static void clearaccount(void);
static void initfiles(void);
static void logInit(void);
static int logSort(const struct lTable *s1, const struct lTable *s2);
static void msgInit(void);
static void readconfig(void);
static void RoomTabBld(void);
static void showtypemsg(ulong here);
static void slidemsgTab(int howmany);
static void zapGrpFile(void);
static void zapHallFile(void);
static void zapLogFile(void);
static void zapMsgFile(void);
static void zapRoomFile(void);

/************************************************************************/
/*      buildcopies()  copies appropriate msg index members             */
/************************************************************************/
static void buildcopies(void)
{
    int i;

    for( i = 0; i < (int)sizetable(); ++i)
    {
        if (msgTab1[i].mtmsgflags.COPY)
        {
            if (msgTab3[i].mtoffset <= (unsigned short)i)
            {
                copyindex( i, (i - msgTab3[i].mtoffset));
            }
        }
    }
}

/************************************************************************/
/*      buildhalls()  builds hall-table (all rooms in Maint.)           */
/************************************************************************/
static void buildhalls(void)
{
    int i;

    doccr(); cPrintf("Building hall file "); doccr();

    for (i = 4; i < MAXROOMS; ++i)
    {
        if (roomTab[i].rtflags.INUSE)
        {
            hallBuf->hall[1].hroomflags[i].inhall = 1;  /* In Maintenance */
            hallBuf->hall[1].hroomflags[i].window = 0;  /* Not a Window   */
        }
    }
    putHall();
}

/************************************************************************/
/*      clearaccount()  initializes all group data                      */
/************************************************************************/
static void clearaccount(void)
{
    int i;
    int groupslot;

    for (groupslot = 0; groupslot < MAXGROUPS; groupslot++)
    {
        /* init days */
        for ( i = 0; i < 7; i++ )
            accountBuf.group[groupslot].account.days[i] = 1;

        /* init hours & special hours */
        for ( i = 0; i < 24; i++ )
        {
            accountBuf.group[groupslot].account.hours[i]   = 1;
            accountBuf.group[groupslot].account.special[i] = 0;
        }

        accountBuf.group[groupslot].account.have_acc      = FALSE;
        accountBuf.group[groupslot].account.dayinc        = 0.0F;
        accountBuf.group[groupslot].account.sp_dayinc     = 0.0F;
        accountBuf.group[groupslot].account.maxbal        = 0.0F;
        accountBuf.group[groupslot].account.priority      = 0.0F;
        accountBuf.group[groupslot].account.dlmult        = -1;
        accountBuf.group[groupslot].account.ulmult        =  1;

    }
}

/************************************************************************/
/*      configcit() the <main> for configuration                        */
/************************************************************************/
void configcit(void)
{
    fcloseall();

    cursoff();

    /* read config.cit */
    readconfig();

    /* move to home-path */
    changedir(cfg.homepath);

    /* initialize & open any files */
    initfiles();

    if (msgZap )  zapMsgFile();
    if (roomZap)  zapRoomFile();
    if (logZap )  zapLogFile();
    if (grpZap )  zapGrpFile();
    if (hallZap)  zapHallFile();

    if (roomZap && !msgZap)  roomBuild = TRUE;
    if (hallZap && !msgZap)  hallBuild = TRUE;

    logInit();
    msgInit();
    RoomTabBld();

    if (hallBuild)  buildhalls();

    curson();

    fclose(grpfl);
    fclose(hallfl);
    fclose(roomfl);
    fclose(msgfl);
    fclose(logfl);

    doccr();
    cPrintf("Config Complete");
    doccr();
}

/***********************************************************************/
/*    illegal() Prints out configur error message and aborts           */
/***********************************************************************/
void illegal(const char *fmt, ...)
{
    char buff[256];
    va_list ap;
    
    va_start(ap, fmt);
    vsprintf(buff, fmt, ap);
    va_end(ap);
    
    doccr();
    cPrintf("%s", buff);
    doccr();
    cPrintf("Fatal error in configuration. Aborting."); doccr();
    curson();
    exit(7);
}

/************************************************************************/
/*      initfiles() -- initializes files, opens them                    */
/************************************************************************/
static void initfiles(void)
{
    char  *grpFile, *hallFile, *logFile, *msgFile, *roomFile;
    char scratch[64];

    chdir(cfg.homepath);

    if (cfg.msgpath[ (strlen(cfg.msgpath) - 1) ]  == '\\')
        cfg.msgpath[ (strlen(cfg.msgpath) - 1) ]  =  '\0';

    sprintf(scratch, "%s\\%s", cfg.msgpath, "msg.dat");

    grpFile     = "grp.dat" ;
    hallFile    = "hall.dat";
    logFile     = "log.dat" ;
    msgFile     =  scratch  ;
    roomFile    = "room.dat";

    /* open group file */
    if ((grpfl = fopen(grpFile, "r+b")) == NULL)
    {
        cPrintf(" %s not found, creating new file. ", grpFile);  doccr();
        if ((grpfl = fopen(grpFile, "w+b")) == NULL)
            illegal("Can't create the group file!");
        {
            cPrintf(" It will be initialized. "); doccr();
            grpZap = TRUE;
        }
    }

    /* open hall file */
    if ((hallfl = fopen(hallFile, "r+b")) == NULL)
    {
        cPrintf(" %s not found, creating new file. ", hallFile); doccr();
        if ((hallfl = fopen(hallFile, "w+b")) == NULL)
            illegal("Can't create the hall file!");
        {
            cPrintf(" It will be initialized. ");  doccr();
            hallZap = TRUE;
        }
    }

    /* open log file */
    if ((logfl = fopen(logFile, "r+b")) == NULL)
    {
        cPrintf(" %s not found, creating new file. ", logFile); doccr();
        if ((logfl = fopen(logFile, "w+b")) == NULL)
            illegal("Can't create log file!");
        {
            cPrintf(" It will be initialized. ");  doccr();
            logZap = TRUE;
        }
    }

    /* open message file */
    if ((msgfl = fopen(msgFile, "r+b")) == NULL)
    {
        cPrintf(" msg.dat not found, creating new file. ");  doccr();
        if ((msgfl = fopen(msgFile, "w+b")) == NULL)
            illegal("Can't create the message file!");
        {
            cPrintf(" It will be initialized. ");  doccr();
            msgZap = TRUE;
        }
    }

    /* open room file */
    if ((roomfl = fopen(roomFile, "r+b")) == NULL)
    {
        cPrintf(" %s not found, creating new file. ", roomFile);  doccr();
        if ((roomfl = fopen(roomFile, "w+b")) == NULL)
            illegal("Can't create room file!");
        {
            cPrintf(" It will be initialized. "); doccr();
            roomZap = TRUE;
        }
    }
}

/************************************************************************/
/*      logInit() indexes log.dat                                       */
/************************************************************************/
static void logInit(void)
{
    int i;
    int count = 0;

    doccr(); doccr();
    cPrintf("Building log table "); doccr();

    cfg.callno = 0l;

    rewind(logfl);
    /* clear logTab */
    for (i = 0; i < cfg.MAXLOGTAB; i++) logTab[i].ltcallno = 0l;

    /* load logTab: */
    for (thisLog = 0;  thisLog < cfg.MAXLOGTAB;  thisLog++)
    {
  
        cPrintf("log#%3d\r",thisLog);

        getLog(&logBuf, thisLog);

        if (logBuf.callno > cfg.callno)  cfg.callno = logBuf.callno;

        /* count valid entries:             */

        if (logBuf.lbflags.L_INUSE == 1)  count++;

      
        /* copy relevant info into index:   */
        logTab[thisLog].ltcallno = logBuf.callno;
        logTab[thisLog].ltlogSlot= thisLog;
        logTab[thisLog].permanent = (char)logBuf.lbflags.PERMANENT;

        if (logBuf.lbflags.L_INUSE == 1)
        {
            logTab[thisLog].ltnmhash = hash(logBuf.lbname);
            logTab[thisLog].ltinhash = hash(logBuf.lbin  );
            logTab[thisLog].ltpwhash = hash(logBuf.lbpw  );
        }
        else
        {
            logTab[thisLog].ltnmhash = 0;
            logTab[thisLog].ltinhash = 0;
            logTab[thisLog].ltpwhash = 0;
        }
    }
    doccr();
    cPrintf("%lu calls.", cfg.callno);
    doccr();
    cPrintf("%d valid log entries.", count);  doccr();

    qsort(logTab, (unsigned)cfg.MAXLOGTAB, (unsigned)sizeof(*logTab), logSort);
}

/************************************************************************/
/*      logSort() Sorts 2 entries in logTab                             */
/************************************************************************/
static int logSort(const struct lTable *s1, const struct lTable *s2)
{
    if (s1->ltnmhash == 0 && s2->ltnmhash == 0)
        return 0;
    if (s1->ltnmhash == 0 && s2->ltnmhash != 0)
        return 1;
    if (s1->ltnmhash != 0 && s2->ltnmhash == 0)
        return -1;
    if (s1->ltcallno < s2->ltcallno)
        return 1;
    if (s1->ltcallno > s2->ltcallno)
        return -1;
    return 0;
}

/************************************************************************/
/*      msgInit() sets up lowId, highId, cfg.catSector and cfg.catChar, */
/*      by scanning over message.buf                                    */
/************************************************************************/
static void msgInit(void)
{
    ulong first, here;
    int makeroom;
    int skipcounter = 0;   /* maybe need to skip a few . Dont put them in
                              message index */
    int slot;

    doccr(); doccr();
    cPrintf("Building message table"); doccr();

    /* start at the beginning */
    fseek(msgfl, 0l, 0);

    getMessage();

    /* get the ID# */
    sscanf(msgBuf->mbId, "%ld", &first);

    showtypemsg(first);

    /* put the index in its place */
    /* mark first entry of index as a reference point */

    cfg.mtoldest = first;
    
    indexmessage(first);

    cfg.newest = cfg.oldest = first;

    cfg.catLoc = ftell(msgfl);

    for(;;) /* while (TRUE) */
    {
        getMessage();

        sscanf(msgBuf->mbId, "%ld", &here);

        if (here == first) break;

        showtypemsg(here);

        /* find highest and lowest message IDs: */
        /* >here< is the dip pholks             */
        if (here < cfg.oldest)
        {
            slot = ( indexslot(cfg.newest) + 1 );

            makeroom = (int)(cfg.mtoldest - here);

            /* check to see if our table can hold  remaining msgs */
            if (cfg.nmessages < (makeroom + slot))
            {
                skipcounter = (makeroom + slot) - cfg.nmessages;

                slidemsgTab(makeroom - skipcounter);

                cfg.mtoldest = (here + (ulong)skipcounter);
 
            }
            /* nmessages can handle it.. Just make room */
            else
            {
                slidemsgTab(makeroom);
                cfg.mtoldest = here;
            }
            cfg.oldest = here;
        }

        if (here > cfg.newest)
        {
            cfg.newest = here;

            /* read rest of message in and remember where it ends,      */
            /* in case it turns out to be the last message              */
            /* in which case, that's where to start writing next message*/
            while (dGetWord(msgBuf->mbtext, MAXTEXT));

            cfg.catLoc = ftell(msgfl);
        }

        /* check to see if our table is big enough to handle it */
        if ( (int)(here - cfg.mtoldest) >= cfg.nmessages)
        {
            crunchmsgTab(1);
        }

        if (skipcounter) 
        {
            skipcounter--;
        }
        else
        {
            indexmessage(here);
        }
    }    
    buildcopies();
}             

/************************************************************************/
/*      readaccount()  reads grpdata.cit values into group structure    */
/************************************************************************/
void readaccount(void)
{                          
    FILE *fBuf;
    char line[90];
    char *words[256];
    int  i, j, k, l, count;
    int groupslot = ERROR;
    int hour;
    static const char grpDataFile[] = "GRPDATA.CIT";
   
    clearaccount();   /* initialize all accounting data */

    /* move to home-path */
    changedir(cfg.homepath);

    if ((fBuf = fopen(grpDataFile, "r")) == NULL)  /* ASCII mode */
    {  
        cPrintf("Can't find %s!", grpDataFile); doccr();
        exit(1);
    }

    while (fgets(line, 90, fBuf) != NULL)
    {
        if (line[0] != '#')  continue;

        count = parse_it( words, line);

        for (i = 0; grpkeywords[i] != NULL; i++)
        {
            if (strcmpi(words[0], grpkeywords[i]) == SAMESTRING)
            {
                break;
            }
        }

        switch(i)
        {
            case GRK_DAYS:              
                if (groupslot == ERROR)  break;

                /* init days */
                for ( j = 0; j < 7; j++ )
                    accountBuf.group[groupslot].account.days[j] = 0;

                for (j = 1; j < count; j++)
                {
                    for (k = 0; daykeywords[k] != NULL; k++)
                    {
                        if (strcmpi(words[j], daykeywords[k]) == SAMESTRING)
                        {
                            break;
                        }
                    }
                    if (k < 7)
                        accountBuf.group[groupslot].account.days[k] = TRUE;
                    else if (k == 7)  /* any */
                    {
                        for ( l = 0; l < MAXGROUPS; ++l)
                            accountBuf.group[groupslot].account.days[l] = TRUE;
                    }
                    else
                    {
                        doccr();
                   cPrintf("%s - Warning: Unknown day %s ", 
                           grpDataFile, words[j]);
                        doccr();
                    }
                }
                break;

            case GRK_GROUP:             
                groupslot = groupexists(words[1]);
                if (groupslot == ERROR)
                {
                    doccr();
                    cPrintf("%s - Warning: Unknown group %s", 
                            grpDataFile, words[1]);
                    doccr();
                }
                accountBuf.group[groupslot].account.have_acc = TRUE;
                break;

            case GRK_HOURS:             
                if (groupslot == ERROR)  break;

                /* init hours */
                for ( j = 0; j < 24; j++ )
                    accountBuf.group[groupslot].account.hours[j]   = 0;

                for (j = 1; j < count; j++)
                {
                    if (strcmpi(words[j], "Any") == SAMESTRING)
                    {
                        for (l = 0; l < 24; l++)
                            accountBuf.group[groupslot].account.hours[l] = TRUE;
                    }
                    else
                    {
                        hour = atoi(words[j]);

                        if ( hour > 23 ) 
                        {
                            doccr();
                            cPrintf("%s - Warning: Invalid hour %d ",
                                    grpDataFile, hour);
                            doccr();
                        }
                        else
                       accountBuf.group[groupslot].account.hours[hour] = TRUE;
                    }
                }
                break;

            case GRK_DAYINC:
                if (groupslot == ERROR)  break;

                if (count > 1)
                {
                    sscanf(words[1], "%f ",
                    &accountBuf.group[groupslot].account.dayinc);   /* float */
                }
                break;

            case GRK_DLMULT:
                if (groupslot == ERROR)  break;

                if (count > 1)
                {
                    sscanf(words[1], "%f ",
                    &accountBuf.group[groupslot].account.dlmult);   /* float */
                }
                break;

            case GRK_ULMULT:
                if (groupslot == ERROR)  break;

                if (count > 1)
                {
                    sscanf(words[1], "%f ",
                    &accountBuf.group[groupslot].account.ulmult);   /* float */
                }
                break;

            case GRK_PRIORITY:
                if (groupslot == ERROR)  break;

                if (count > 1)
                {
                    sscanf(words[1], "%f ",
                    &accountBuf.group[groupslot].account.priority);  /* float */
                }

                break;

            case GRK_MAXBAL:
                if (groupslot == ERROR)  break;

                if (count > 1)
                {
                    sscanf(words[1], "%f ",
                    &accountBuf.group[groupslot].account.maxbal);   /* float */
                }

                break;



            case GRK_SPECIAL:           
                if (groupslot == ERROR)  break;

                /* init hours */
                for ( j = 0; j < 24; j++ )
                    accountBuf.group[groupslot].account.special[j]   = 0;

                for (j = 1; j < count; j++)
                {
                    if (strcmpi(words[j], "Any") == SAMESTRING)
                    {
                        for (l = 0; l < 24; l++)
                            accountBuf.group[groupslot].account.special[l] = TRUE;
                    }
                    else
                    {
                        hour = atoi(words[j]);

                        if ( hour > 23 )
                        {
                            doccr();
                            cPrintf("%s - Warning: Invalid special hour %d ",
                                    grpDataFile, hour);
                            doccr();
                        }
                        else
                       accountBuf.group[groupslot].account.special[hour] = TRUE;
                    }

                }
                break;
        }

    }
    fclose(fBuf);
}

/************************************************************************/
/*      readconfig() reads config.cit values                            */
/************************************************************************/
static void readconfig(void)
{
    FILE *fBuf;
    char line[256];
    char *words[256];
    int  i, j, k, l, count, att;
    char notkeyword[20];
    char valid = FALSE;
    char found[K_NWORDS+2];
    int  lineNo = 0;

    static char nochange[] = "%s invalid: can't change to '%s' directory";

/**** not needed, if not found it is complained about */
/*  cfg.MAXLOGTAB = 0;             * Initialize, just in case */
/*  cfg.maxfiles = 255;            * Initialize, just in case */
/*  strcpy(cfg.tdformat, "%x %X"); * Initialize, just in case */
    strcpy(cfg.msg_nym,  "message");
    strcpy(cfg.msgs_nym, "messages");
    strcpy(cfg.msg_done, "saved");


    cfg.version = 3110023L;
    
    for (i=0; i <= K_NWORDS; i++)
        found[i] = FALSE;

    if ((fBuf = fopen("config.cit", "r")) == NULL)  /* ASCII mode */
    {  
        cPrintf("Can't find Config.cit!"); doccr();
        exit(3);
    }

    while (fgets(line, 255, fBuf) != NULL)
    {
        lineNo++;

        if (line[0] != '#')  continue;

        count = parse_it( words, line);

        for (i = 0; keywords[i] != NULL; i++)
        {
            if (strcmpi(words[0], keywords[i]) == SAMESTRING)
            {
                break;
            }
        }

        if (keywords[i] == NULL)
        {
            cPrintf("CONFIG.CIT (%d) Warning: Unknown variable %s ", lineNo, 
                words[0]);
            doccr();
            continue;
        }else{
            if (found[i] == TRUE)
            {
                cPrintf("CONFIG.CIT (%d) Warning: %s mutiply defined!", lineNo, 
                    words[0]);
                doccr();
            }else{
                found[i] = TRUE;
            }
        }

        switch(i)
        {
            case K_ACCOUNTING:
                cfg.accounting = atoi(words[1]);
                break;

            case K_IDLE_WAIT:
                cfg.idle = atoi(words[1]);
                break;

            case K_ALLDONE: 
                break;

#if 1
            case K_ANONAUTHOR:
                if (strlen(words[1]) > LABELSIZE)
                    illegal(toolong, words[0], words[1], LABELSIZE+1);
                strcpy(cfg.anonauthor, words[1]);
                break;
#endif

            case K_ATTR:
                sscanf(words[1], "%x ", &att); /* hex! */
                cfg.attr = (uchar)att;
                break;

            case K_WATTR:
                sscanf(words[1], "%x ", &att); /* hex! */
                cfg.wattr = (uchar)att;
                break;

            case K_CATTR:
                sscanf(words[1], "%x ", &att); /* hex! */
                cfg.cattr = (uchar)att;
                break;

            case K_BATTR:
                sscanf(words[1], "%x ", &att);    /* hex! */
                cfg.battr = (unsigned char)att;
                break;

            case K_UTTR:
                sscanf(words[1], "%x ", &att);     /* hex! */
                cfg.uttr = (unsigned char)att;
                break;


            case K_INIT_BAUD:
                cfg.initbaud = (char)atoi(words[1]);
                break;

            case K_BIOS:
                cfg.bios = atoi(words[1]);
                break;

            case K_COST1200:
                sscanf(words[1], "%f ", &cfg.cost1200); /* float */
                break;

            case K_COST2400:
                sscanf(words[1], "%f ", &cfg.cost2400); /* float */
                break;

#if 1
            case K_DIRPATH:
                if (strlen(words[1]) > 63)
                    illegal(toolong, words[0], words[1], 64);
                strcpy(cfg.dirpath, words[1]);
                if (changedir(words[1]) == ERROR)
                    illegal(nochange, words[0], words[1]);
                break;
#endif

            case K_DUMB_MODEM:
                cfg.dumbmodem    = (char)atoi(words[1]);
                break;

            case K_READLLOG:
                cfg.readluser    = atoi(words[1]);
                break;

            case K_DATESTAMP:
                if (strlen( words[1] ) > 63 )
                illegal(toolong, words[0], words[1], 64);

                strcpy( cfg.datestamp, words[1] );
                break;

            case K_VDATESTAMP:
                if (strlen( words[1] ) > 63 )
                illegal(toolong, words[0], words[1], 64);

                strcpy( cfg.vdatestamp, words[1] );
                break;

            case K_ENTEROK:
                cfg.unlogEnterOk = atoi(words[1]);
                break;

            case K_FORCELOGIN: 
                cfg.forcelogin   = atoi(words[1]);
                break;

            case K_MODERATE: 
                cfg.moderate     = atoi(words[1]);
                break;

            case K_HELPPATH:  
                if (strlen( words[1] ) > 63 )
                    illegal(toolong, words[0], words[1], 64);
                if (changedir(words[1]) == ERROR)
                    illegal(nochange, words[0], words[1]);

                strcpy( cfg.helppath, words[1] );  
                break;

            case K_TEMPPATH:
                if (strlen( words[1] ) > 63 )
                    illegal(toolong, words[0], words[1], 64);
                if (changedir(words[1]) == ERROR)
                    illegal(nochange, words[0], words[1]);

                strcpy( cfg.temppath, words[1] );
                break;


            case K_HOMEPATH:
                if (strlen( words[1] ) > 63 )
                    illegal(toolong, words[0], words[1], 64);
                if (changedir(words[1]) == ERROR)
                    illegal(nochange, words[0], words[1]);

                strcpy(cfg.homepath, words[1] );  
                break;

            case K_HOURS:
                /* init hours */
                for ( j = 0; j < 24; j++ )
                    cfg.hours[j] = FALSE;

                for (j = 1; j < count; j++)
                {
                    if (strcmpi(words[j], "Any") == SAMESTRING)
                    {
                        for (l = 0; l < 24; l++)
                            cfg.hours[l] = TRUE;
                    }
                    else
                    {
                        l = atoi(words[j]);

                        if ( l > 23 ) 
                        {
                            doccr();
                            cPrintf("Invalid CONFIG.CIT %s: %s",
                                    words[0], words[j]);
                            doccr();
                        }
                        else
                            cfg.hours[l] = TRUE;
                    }
                }
                break;

            case K_KILL:
                cfg.kill = atoi(words[1]);
                break;

            case K_LINEFEEDS:
                cfg.linefeeds = atoi(words[1]);
                break;
            
            case K_LOGINSTATS:
                cfg.loginstats = atoi(words[1]);
                break;

            case K_MAXBALANCE:
                sscanf(words[1], "%f ", &cfg.maxbalance); /* float */
                break;

            case K_MAXLOGTAB:
                cfg.MAXLOGTAB    = atoi(words[1]);

                break;

            case K_MESSAGE_ROOM:
                cfg.MessageRoom = (char)atoi(words[1]);
                break;

            case K_NEWUSERAPP:
                if (strlen( words[1] ) > 12 )
                illegal(toolong, words[0], words[1], 13);

                strcpy( cfg.newuserapp, words[1] );
                break;

            case K_PRIVATE:
                cfg.private = atoi(words[1]);
                break;

            case K_MAXTEXT:
                cfg.maxtext = atoi(words[1]);
                break;

            case K_MAX_WARN:
                cfg.maxwarn = (char)atoi(words[1]);
                break;

            case K_MDATA:
                cfg.mdata   = (char)atoi(words[1]);

                if ( (cfg.mdata < 1) || (cfg.mdata > 4) )
                {
                    illegal("MDATA port can only currently be 1, 2, 3 or 4");
                }
                break;

            case K_MAXFILES:
                cfg.maxfiles = atoi(words[1]);
                break;

            case K_MSGPATH:
                if (strlen(words[1]) > 63)
                    illegal(toolong, words[0], words[1], 64);
                if (changedir(words[1]) == ERROR)
                    illegal(nochange, words[0], words[1]);

                strcpy(cfg.msgpath, words[1]);  
                break;

            case K_F6PASSWORD:
                if (strlen(words[1]) > 19)
                illegal(toolong, words[0], words[1], 20);

                strcpy(cfg.f6pass, words[1]);  
                break;

            case K_APPLICATIONS:
                if (strlen(words[1]) > 63)
                    illegal(toolong, words[0], words[1], 64);
                if (changedir(words[1]) == ERROR)
                    illegal(nochange, words[0], words[1]);

                strcpy(cfg.aplpath, words[1]);  
                break;

            case K_MESSAGEK:
                cfg.messagek = atoi(words[1]);
                break;

            case K_MODSETUP:
                if (strlen(words[1]) > 63)
                illegal(toolong, words[0], words[1], 64);

                strcpy(cfg.modsetup, words[1]);  
                break;
                
            case K_DIAL_INIT:
                if (strlen(words[1]) > 63)
                illegal(toolong, words[0], words[1], 64);

                strcpy(cfg.dialsetup, words[1]);  
                break;
                
            case K_DIAL_PREF:
                if (strlen(words[1]) > 30)
                illegal(toolong, words[0], words[1], 31);

                strcpy(cfg.dialpref, words[1]);  
                break;

#if 1
            case K_DIAL_RING:
                if (strlen(words[1]) > 30)
                illegal(toolong, words[0], words[1], 31);

                strcpy(cfg.dialring, words[1]);  
                break;
#endif

            case K_NEWBAL:
                sscanf(words[1], "%f ", &cfg.newbal);  /* float */
                break;

            case K_AIDEHALL:
                cfg.aidehall = atoi(words[1]);
                break;

            case K_NMESSAGES:
                cfg.nmessages  = atoi(words[1]);

                break;

            case K_NODENAME:
                if (strlen(words[1]) > 19)
                illegal(toolong, words[0], words[1], 20);

                strcpy(cfg.nodeTitle, words[1]);  
                break;

            case K_NODEREGION:
                if (strlen(words[1]) > 19)
                illegal(toolong, words[0], words[1], 20);

                strcpy(cfg.nodeRegion, words[1]);
                break;

#if 1
            case K_NODECOUNTRY:
                if (strlen(words[1]) > 19)
                illegal(toolong, words[0], words[1], 20);

                strcpy(cfg.nodeCountry, words[1]);
                break;

            case K_TWITREGION:
                if (strlen(words[1]) > 19)
                illegal(toolong, words[0], words[1], 20);

                strcpy(cfg.twitRegion, words[1]);
                break;

            case K_TWITCOUNTRY:
                if (strlen(words[1]) > 19)
                illegal(toolong, words[0], words[1], 20);

                strcpy(cfg.twitCountry, words[1]);
                break;

            case K_SIGNATURE:
                if (strlen(words[1]) > 79)
                illegal(toolong, words[0], words[1], 80);

                strcpy(cfg.signature, words[1]);
                break;

#endif

            case K_NOPWECHO:
                cfg.nopwecho = (unsigned char)atoi(words[1]);
                break;

            case K_NULLS:
                cfg.nulls = (unsigned char)atoi(words[1]);
                break;

            case K_OFFHOOK:
                cfg.offhook = atoi(words[1]);
                break;

            case K_OLDCOUNT:
                cfg.oldcount = atoi(words[1]);
                break;

            case K_PRINTER:
                if (strlen(words[1]) > 63)
                illegal(toolong, words[0], words[1], 64);

                strcpy(cfg.printer, words[1]);  
                break;

            case K_READOK:
                cfg.unlogReadOk = atoi(words[1]);
                break;

            case K_ROOMOK:
                cfg.nonAideRoomOk = atoi(words[1]);
                break;

            case K_ROOMTELL:
                cfg.roomtell = atoi(words[1]);
                break;

            case K_ROOMPATH:
                if (strlen(words[1]) > 63)
                illegal(toolong, words[0], words[1], 64);
                if (changedir(words[1]) == ERROR)
                    illegal(nochange, words[0], words[1]);

                strcpy(cfg.roompath, words[1]);  
                break;

            case K_SUBHUBS:
                cfg.subhubs = atoi(words[1]);
                break;

            case K_TABS:
                cfg.tabs = atoi(words[1]);
                break;
            
            case K_TIMEOUT:
                cfg.timeout = (char)atoi(words[1]);
                break;

#if 1
            case K_TRANSPATH:
                if (strlen(words[1]) > 63)
                    illegal(toolong, words[0], words[1], 64);
                if (changedir(words[1]) == ERROR)
                    illegal(nochange, words[0], words[1]);
                strcpy(cfg.transpath, words[1]);
                break;
#endif

            case K_TRAP:
                for (j = 1; j < count; j++)
                {
                    valid = FALSE;

                    for (k = 0; trapkeywords[k] != NULL; k++)
                    {
                        sprintf(notkeyword, "!%s", trapkeywords[k]);

                        if (strcmpi(words[j], trapkeywords[k]) == SAMESTRING)
                        {
                            valid = TRUE;

                            if ( k == 0)  /* ALL */
                            {
                                for (l = 0; l < 16; ++l) cfg.trapit[l] = TRUE;
                            }
                            else cfg.trapit[k] = TRUE;
                        }
                        else if (strcmpi(words[j], notkeyword) == SAMESTRING)
                        {
                            valid = TRUE;

                            if ( k == 0)  /* ALL */
                            {
                                for (l = 0; l < 16; ++l) cfg.trapit[l] = FALSE;
                            }
                            else cfg.trapit[k] = FALSE; 
                        }
                    }

                    if ( !valid )
                    {
                        doccr();
                        cPrintf("Config.Cit - Warning:"
                                " Unknown #TRAP parameter %s ", words[j]);
                        doccr();
                    }
                }
                break;

            case K_TRAP_FILE:
                if (strlen(words[1]) > 63)
                illegal(toolong, words[0], words[1], 64);
  
                strcpy(cfg.trapfile, words[1]);  

                break;

            case K_UNLOGGEDBALANCE:
                sscanf(words[1], "%f ", &cfg.unlogbal);  /* float */
                break;

            case K_UNLOGTIMEOUT:
                cfg.unlogtimeout = (char)atoi(words[1]);
                break;

            case K_UPPERCASE:
                cfg.uppercase = atoi(words[1]);
                break;

            case K_USER:
                for ( j = 0; j < 5; ++j)  cfg.user[j] = 0;

                for (j = 1; j < count; j++)
                {
                    valid = FALSE;

                    for (k = 0; userkeywords[k] != NULL; k++)
                    {
                        if (strcmpi(words[j], userkeywords[k]) == SAMESTRING)
                        {
                           valid = TRUE;

                           cfg.user[k] = TRUE;
                        }
                    }

                    if (!valid)
                    {
                        doccr();
                   cPrintf("Config.Cit - Warning: Unknown #USER parameter %s ",
                        words[j]);
                        doccr();
                    }
                }
                break;

            case K_WIDTH:
                cfg.width = (unsigned char)atoi(words[1]);
                break;

            case K_TWIT_FEATURES:
                for (j = 1; j < count; j++)
                {
                    valid = FALSE;

                    for (k = 0; twitfeatures[k] != NULL; k++)
                    {
                        if (strcmpi(words[j], twitfeatures[k]) == SAMESTRING)
                        {
                            valid = TRUE;

                            switch (k)
                            {
                            case 0:     /* MESSAGE NYMS */
                                cfg.msgNym = TRUE;
                                break;

                            case 1:     /* BORDERS */
                                cfg.borders = TRUE;
                                break;
                            
                            case 2:     /* TITLES */
                                cfg.titles = TRUE;
                                break;
                            
                            case 3:     /* NET_TITLES */
                                cfg.nettitles = TRUE;
                                break;
                            
                            case 4:     /* SURNAMES */
                                cfg.surnames = TRUE;
                                break;
                            
                            case 5:     /* NET_SURNAMES */
                                cfg.netsurname = TRUE;
                                break;
                            
                            case 6:     /* ENTER_TITLES */
                                cfg.entersur = TRUE;
                                break;

                            default:
                                break;
                            }
                        }
                    }

                    if ( !valid )
                    {
                        doccr();
                        cPrintf("Config.Cit - Warning:"
                                " Unknown #TWIT_FEATURES parameter %s ",
                                words[j]);
                        doccr();
                    }
                }
                break;

            default:
                cPrintf("Config.Cit - Warning: Unknown variable %s", words[0]);
                doccr();
                break;
        }
    }
    fclose(fBuf);
    changedir(cfg.homepath);

    for (i = 0, valid = TRUE; i <= K_NWORDS; i++)
    {
        if (!found[i])
        {
            cPrintf("CONFIG.CIT : ERROR: can not find %s keyword!\n",
                keywords[i]);
            valid = FALSE;
        }
    }

    if (!valid)
        illegal("");

    allocateTables();

    if (logTab == NULL)
        illegal("Can not allocate log table");

    if (msgTab1 == NULL || msgTab2 == NULL || msgTab3 == NULL ||
        msgTab4 == NULL || msgTab5 == NULL || msgTab6 == NULL ||
        msgTab7 == NULL || msgTab8 == NULL || msgTab9 == NULL)
        illegal("Can not allocate message table");
}

/************************************************************************/
/*      readprotocols() reads external.cit values into ext   structure  */
/************************************************************************/
void readprotocols(void)
{                          
    FILE *fBuf;
    char line[120];
    char *words[256];
    int  j, count;
    char *exttoolong="%s (%s) too long; must be less than %d characters";

    numOthCmds   = 0;
    extrncmd[0] = 0 /* NULL */;
    editcmd[0]  = 0 /* NULL */;
   
    /* move to home-path */
    changedir(cfg.homepath);

    if ((fBuf = fopen("external.cit", "r")) == NULL)  /* ASCII mode */
    {  
        cPrintf("Can't find external.cit!"); doccr();
        exit(1);
    }

    while (fgets(line, 120, fBuf) != NULL)
    {
        if (line[0] != '#')  continue;

        count = parse_it( words, line);

        if (strcmpi("#PROTOCOL", words[0]) == SAMESTRING)
        {
            j = strlen(extrncmd);

            if (strlen( words[1] ) > 19 )
              illegal(exttoolong, "Protocol name", words[1], 20);
            if (strlen( words[4] ) > 39 )
              illegal(exttoolong, "Receive command", words[1], 40);
            if (strlen( words[5] ) > 39 )
              illegal(exttoolong, "Send command", words[1], 40);
            if (atoi(words[2]) < 0 || atoi(words[2]) > 1)
              illegal("Batch field bad; must be 0 or 1");
            if (atoi(words[3]) < 0 || atoi(words[3]) > 10 * 1024)
              illegal("Block field bad; must be 0 to 10K");
            if (j >= MAXEXTERN)
              illegal("Too many external protocols, maximum=%d", MAXEXTERN);
    
            strcpy(extrn[j].ex_name, words[1]);
            extrn[j].ex_batch = (char)atoi(words[2]);
            extrn[j].ex_block = atoi(words[3]);
            strcpy(extrn[j].ex_rcv,  words[4]);
            strcpy(extrn[j].ex_snd,  words[5]);
            extrncmd[j]   = (char)tolower(*words[1]);
            extrncmd[j+1] = '\0';
        }
        if (strcmpi("#EDITOR", words[0]) == SAMESTRING)
        {
            j = strlen(editcmd);

            if (strlen( words[1] ) > 19 )
              illegal(exttoolong, "Protocol name", words[1], 20);
            if (strlen( words[3] ) > 29 )
              illegal(exttoolong, "Command line", words[1], 30);
            if (atoi(words[2]) < 0 || atoi(words[2]) > 1)
              illegal("Local field bad; must be 0 or 1");
            if (j > 19)
              illegal("Only 20 external editors");
    
            strcpy(edit[j].ed_name,  words[1]);
            edit[j].ed_local  = (char)atoi(words[2]);
            strcpy(edit[j].ed_cmd,   words[3]);
            editcmd[j]    = (char)tolower(*words[1]);
            editcmd[j+1]                = '\0';
        }
        if (strcmpi("#OTHER", words[0]) == SAMESTRING)
        {
            if (count < 6)
              illegal("Too few arguments for #OTHER command");
            if (strlen( words[1] ) > 39 )
              illegal(exttoolong, "Command line", words[1], 40);
            if (strlen( words[3] ) > 19 )
              illegal(exttoolong, "Protocol name (#1)", words[1], 20);
            if (strlen( words[5] ) > 19 )
              illegal(exttoolong, "Protocol name (#2)", words[1], 20);
            if (numOthCmds >= (MAXEXTERN) )
              illegal("Too many #OTHER external commands,"
                      " maximum=%d", MAXEXTERN);
    
            strcpy(othCmd[numOthCmds].eo_cmd,   words[1]);
            othCmd[numOthCmds].eo_cmd1 =        (char)tolower(*words[2]);
            strcpy(othCmd[numOthCmds].eo_name1, words[3]);
            othCmd[numOthCmds].eo_cmd1 =        (char)tolower(*words[4]);
            strcpy(othCmd[numOthCmds].eo_name2, words[5]);
            numOthCmds++;
        }
    }
    fclose(fBuf);
}

/************************************************************************/
/*      RoomTabBld() -- build RAM index to ROOM.DAT, displays stats.    */
/************************************************************************/
static void RoomTabBld(void)
{
    int  slot;
    int  roomCount = 0;

    doccr(); doccr();
    cPrintf("Building room table"); doccr();

    for (slot = 0;  slot < MAXROOMS;  slot++)
    {
        getRoom(slot);

        cPrintf("Room No: %3d\r", slot);

        if (roomBuf.rbflags.INUSE)  ++roomCount;
   
        noteRoom();
        putRoom(slot);
    }
    doccr();
    cPrintf(" %d of %d rooms in use", roomCount, MAXROOMS); doccr();

}

/************************************************************************/
/*      showtypemsg() prints out what kind of message is being read     */
/************************************************************************/
static void showtypemsg(ulong here)
{
#   ifdef DEBUG
    cPrintf("(%7lu)", msgBuf->mbheadLoc);
#   endif

    if  (*msgBuf->mbcopy)              cPrintf("Dup Mess# %6lu\r", here);
    else
    {
        if       (*msgBuf->mbto)       cPrintf("Pri Mess# %6lu\r", here);
        else if  (*msgBuf->mbx == 'Y')
                                       cPrintf("Prb Mess# %6lu\r", here);
        else if  (*msgBuf->mbx == 'M')
                                       cPrintf("Mod Mess# %6lu\r", here);
        else if  (*msgBuf->mbgroup)    cPrintf("Grp Mess# %6lu\r", here);
        else                           cPrintf("Pub Mess# %6lu\r", here);
    }
}

/************************************************************************/
/*      slidemsgTab() frees >howmany< slots at the beginning of the     */
/*      message table.                                                  */
/************************************************************************/
static void slidemsgTab(int howmany)
{
    uint numnuked;
   
    numnuked = cfg.nmessages - howmany;
   
    _fmemmove(&msgTab1[howmany], msgTab1,
            (uint)(numnuked * (sizeof(*msgTab1)) ));
    _fmemmove(&msgTab2[howmany], msgTab2,
            (uint)(numnuked * (sizeof(*msgTab2)) ));
    _fmemmove(&msgTab3[howmany], msgTab3,
            (uint)(numnuked * (sizeof(*msgTab3)) ));
    _fmemmove(&msgTab4[howmany], msgTab4,
            (uint)(numnuked * (sizeof(*msgTab4)) ));
    _fmemmove(&msgTab5[howmany], msgTab5,
            (uint)(numnuked * (sizeof(*msgTab5)) ));
    _fmemmove(&msgTab6[howmany], msgTab6,
            (uint)(numnuked * (sizeof(*msgTab6)) ));
    _fmemmove(&msgTab7[howmany], msgTab7,
            (uint)(numnuked * (sizeof(*msgTab7)) ));
    _fmemmove(&msgTab8[howmany], msgTab8,
            (uint)(numnuked * (sizeof(*msgTab8)) ));
    _fmemmove(&msgTab9[howmany], msgTab9,
            (uint)(numnuked * (sizeof(*msgTab9)) ));
}

/************************************************************************/
/*      zapGrpFile(), erase & reinitialize group file                   */
/************************************************************************/
static void zapGrpFile(void)
{
    doccr();
    cPrintf("Writing group table."); doccr();

    setmem(&grpBuf, sizeof grpBuf, 0);

    strcpy( grpBuf.group[0].groupname, "Null");
    grpBuf.group[0].g_inuse  = 1;
    grpBuf.group[0].groupgen = 1;      /* Group Null's gen# is one      */
                                       /* everyone's a member at log-in */

    strcpy( grpBuf.group[1].groupname, "Reserved_2");
    grpBuf.group[1].g_inuse   = 1;
    grpBuf.group[1].groupgen  = 1;

    putGroup();
}

/************************************************************************/
/*      zapHallFile(), erase & reinitialize hall file                   */
/************************************************************************/
static void zapHallFile(void)
{
    doccr();
    cPrintf("Writing hall table.");  doccr();

    strcpy( hallBuf->hall[0].hallname, "Root");
    hallBuf->hall[0].owned = 0;                 /* Hall is not owned     */

    hallBuf->hall[0].h_inuse = 1;
    hallBuf->hall[0].hroomflags[0].inhall = 1;  /* Lobby> in Root        */
    hallBuf->hall[0].hroomflags[1].inhall = 1;  /* Mail>  in Root        */
    hallBuf->hall[0].hroomflags[2].inhall = 1;  /* Aide)  in Root        */

    strcpy( hallBuf->hall[1].hallname, "Maintenance");
    hallBuf->hall[1].owned = 0;                 /* Hall is not owned     */

    hallBuf->hall[1].h_inuse = 1;
    hallBuf->hall[1].hroomflags[0].inhall = 1;  /* Lobby> in Maintenance */
    hallBuf->hall[1].hroomflags[1].inhall = 1;  /* Mail>  in Maintenance */
    hallBuf->hall[1].hroomflags[2].inhall = 1;  /* Aide)  in Maintenance */


    hallBuf->hall[0].hroomflags[2].window = 1;  /* Aide) is the window   */
    hallBuf->hall[1].hroomflags[2].window = 1;  /* Aide) is the window   */

    putHall();
}

/************************************************************************/
/*      zapLogFile() erases & re-initializes userlog.buf                */
/************************************************************************/
static void zapLogFile(void)
{
    int  i;

    /* clear RAM buffer out */
    logBuf.lbflags.L_INUSE   = FALSE;
    logBuf.lbflags.NOACCOUNT = FALSE;
    logBuf.lbflags.AIDE      = FALSE;
    logBuf.lbflags.NETUSER   = FALSE;
    logBuf.lbflags.NODE      = FALSE;
    logBuf.lbflags.PERMANENT = FALSE;
    logBuf.lbflags.PROBLEM   = FALSE;
    logBuf.lbflags.SYSOP     = FALSE;
    logBuf.lbflags.ROOMTELL  = FALSE;
    logBuf.lbflags.NOMAIL    = FALSE;
    logBuf.lbflags.UNLISTED  = FALSE;

    logBuf.callno = 0l;
 
    for (i = 0;  i < NAMESIZE;  i++)
    {
        logBuf.lbname[i] = 0;
        logBuf.lbin[i]   = 0;
        logBuf.lbpw[i]   = 0;
    }
    doccr();  doccr();
    cPrintf("MAXLOGTAB=%d",cfg.MAXLOGTAB);  doccr();

    /* write empty buffer all over file;        */
    for (i = 0; i < cfg.MAXLOGTAB;  i++)
    {
        cPrintf("Clearing log entry %3d\r", i);
        putLog(&logBuf, i);
        logTab[i].ltcallno = logBuf.callno;
        logTab[i].ltlogSlot= i;
        logTab[i].ltnmhash = hash(logBuf.lbname);
        logTab[i].ltinhash = hash(logBuf.lbin  );
        logTab[i].ltpwhash = hash(logBuf.lbpw  );
    }
    doccr();
}

/************************************************************************/
/*      zapMsgFl() initializes message.buf                              */
/************************************************************************/
static void zapMsgFile(void)
{
    int i;
    unsigned sect;
    unsigned char  sectBuf[1025];

    for (i = 0;  i < 1024;  i++) sectBuf[i] = 0;

    /* put null message in first sector... */
    sectBuf[0]  = 0xFF; /*                                   */
    sectBuf[1]  = DUMP; /*  \  To the dump                   */
    sectBuf[2]  = '\0'; /*  /  Attribute                     */
    sectBuf[3]  =  '1'; /*  >                                */
    sectBuf[4]  = '\0'; /*  \  Message ID "1" MS-DOS style   */
    sectBuf[5]  =  'M'; /*  /         Null messsage          */
    sectBuf[6]  = '\0'; /*                                   */
                                                  
    cfg.newest = cfg.oldest = 1l;

    cfg.catLoc = 7l;

    if (fwrite(sectBuf, 1024, 1, msgfl) != 1)
    {
        cPrintf("zapMsgFil: write failed"); doccr();
    }

    for (i = 0;  i < 1024;  i++) sectBuf[i] = 0;

    doccr();  doccr();
    cPrintf("MESSAGEK=%d", cfg.messagek);  doccr();
    for (sect = 1;  sect < cfg.messagek;  sect++)
    {
        cPrintf("Clearing block %4u\r", sect);
        if (fwrite(sectBuf, 1024, 1, msgfl) != 1)
        {
            cPrintf("zapMsgFil: write failed");  doccr();
        }
    }
}


/************************************************************************/
/*      zapRoomFile() erases and re-initailizes ROOM.DAT                */
/************************************************************************/
static void zapRoomFile(void)
{
    int i;

    roomBuf.rbflags.INUSE     = FALSE;
    roomBuf.rbflags.PUBLIC    = FALSE;
    roomBuf.rbflags.MSDOSDIR  = FALSE;
    roomBuf.rbflags.PERMROOM  = FALSE;
    roomBuf.rbflags.GROUPONLY = FALSE;
    roomBuf.rbflags.READONLY  = FALSE;
    roomBuf.rbflags.SHARED    = FALSE;
    roomBuf.rbflags.MODERATED = FALSE;
    roomBuf.rbflags.DOWNONLY  = FALSE;

    roomBuf.rbgen            = 0;
    roomBuf.rbdirname[0]     = 0;
    roomBuf.rbname[0]        = 0;   
    roomBuf.rbroomtell[0]    = 0;   
/*  roomBuf.rbgrpgen         = 0;  */
    roomBuf.rbgrpno          = 0;

    doccr();  doccr();
    cPrintf("MAXROOMS=%d", MAXROOMS); doccr();

    for (i = 0;  i < MAXROOMS;  i++)
    {
        cPrintf("Clearing room %3d\r", i);
        putRoom(i);
        noteRoom();
    }

    /* Lobby> always exists -- guarantees us a place to stand! */
    thisRoom            = 0          ;
    strcpy(roomBuf.rbname, "Lobby")  ;
    roomBuf.rbflags.PERMROOM = TRUE;
    roomBuf.rbflags.PUBLIC   = TRUE;
    roomBuf.rbflags.INUSE    = TRUE;

    putRoom(LOBBY);
    noteRoom();

    /* Mail> is also permanent...       */
    thisRoom            = MAILROOM      ;
    strcpy(roomBuf.rbname, "Mail");
    roomBuf.rbflags.PERMROOM = TRUE;
    roomBuf.rbflags.PUBLIC   = TRUE;
    roomBuf.rbflags.INUSE    = TRUE;

    putRoom(MAILROOM);
    noteRoom();

    /* Aide) also...                    */
    thisRoom            = AIDEROOM;
    strcpy(roomBuf.rbname, "Aide");
    roomBuf.rbflags.PERMROOM = TRUE;
    roomBuf.rbflags.PUBLIC   = FALSE;
    roomBuf.rbflags.INUSE    = TRUE;

    putRoom(AIDEROOM);
    noteRoom();

    /* Dump> also...                    */
    thisRoom            = DUMP;
    strcpy(roomBuf.rbname, "Dump");
    roomBuf.rbflags.PERMROOM = TRUE;
    roomBuf.rbflags.PUBLIC   = TRUE;
    roomBuf.rbflags.INUSE    = TRUE;

    putRoom(DUMP);
    noteRoom();
}
