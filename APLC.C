/* -------------------------------------------------------------------- */
/*  APLC.C                        ACit                         91Sep19  */
/*                    Application code for Citadel                      */
/* -------------------------------------------------------------------- */
#include <dos.h>
#include <malloc.h>
#include <string.h>
#include <process.h>

#include "spawno.h"
#include "ctdl.h"
#include "prot.h"
#include "glob.h"
#include "aplc.h"
#include "apst.h"


/* -------------------------------------------------------------------- */
/*                              Contents                                */
/*                                                                      */
/*  ExeAplic()      gets name of application and executes it            */
/*  shellescape()   handles the sysop '!' shell command                 */
/*  apsystem()      turns off interupts and makes a system call         */
/* $readAplFile()   read *.apl files from aplpath                       */
/* $writeAplFile()  write *.apl files to aplpath                        */
/*  wxsnd()         external protocol send file                         */
/*  wxrcv()         external protocol receive file                      */
/* -------------------------------------------------------------------- */

static char *swhacks = "%s\\%s";
static char *input_apl = "input.apl";
static char *output_apl = "output.apl";
static char *readme_apl = "readme.apl";
static char *message_apl = "message.apl";

static void readAplFile(void);
static void writeAplFile(void);

/* -------------------------------------------------------------------- */
/*      ExeAplic() gets the name of an aplication and executes it.      */
/* -------------------------------------------------------------------- */
void ExeAplic(void)
{
    char stuff[100];
    char comm[5];

    doCR();
    doCR();

    if (!roomBuf.rbflags.APLIC) 
    {
      mPrintf("  -- Room has no application.\n\n");
      changedir(cfg.homepath);
      return;
    }
    if (changedir(cfg.aplpath) == ERROR)
    {
      mPrintf("  -- Can't find application directory.\n\n");
      changedir(cfg.homepath);
      return;
    }

    sprintf(comm, "COM%d", cfg.mdata);
    sprintf(stuff,"%s %s %d %d %s",
           roomBuf.rbaplic,
           (whichIO == CONSOLE) ? "LOCAL" : comm,
           (whichIO == CONSOLE) ? 2400    : bauds[speed],
           sysop,
           logBuf.lbname); 

    apsystem(stuff);
    changedir(cfg.homepath);
}

/* -------------------------------------------------------------------- */
/*      shellescape()  handles the sysop '!' shell command              */
/* -------------------------------------------------------------------- */
void shellescape(char super)
{
    char prompt[92];
    static char oldprompt[92];
    char *envprompt;
    char command[80];

    envprompt = getenv("PROMPT");

    sprintf(prompt,   "PROMPT=_%s", envprompt);
    sprintf(oldprompt,"PROMPT=%s",  envprompt);

    putenv(prompt);

    changedir(roomBuf.rbflags.MSDOSDIR ? roomBuf.rbdirname : cfg.homepath);

    sprintf(command, "%s%s", super ? "!" : "", getenv("COMSPEC"));

    apsystem(command);

    putenv(oldprompt);

    update25();

    changedir(cfg.homepath);
}

#ifdef GOODBYE2
/* -------------------------------------------------------------------- */
/* tableIn   allocates RAM and reads log and msg tab file into RAM      */
/* -------------------------------------------------------------------- */
static void tableIn(void)
{
    FILE *fd;
    char scratch[64];

    if (dowhat != NETWORKING)
    {
        doCR();
        mPrintf("Restoring system variables, please wait.");
        doCR();
    }

    sprintf(scratch, swhacks, cfg.temppath, "tables.tmp");

    if ((fd  = fopen(scratch, "rb")) == NULL)
    {
        mPrintf("\n Fatal System Crash!\n System tables destroyed!");
        crashout("Log table lost in application");
    }

    allocateTables();

    if (logTab == NULL)
    {
        mPrintf("\n Fatal System Crash!\n Memory FUBAR!");
        crashout("Can not allocate log table after application");
    }

    if (msgTab1 == NULL || msgTab2 == NULL || msgTab3 == NULL ||
        msgTab4 == NULL || msgTab5 == NULL || msgTab6 == NULL ||
        msgTab7 == NULL || msgTab8 == NULL || msgTab9 == NULL)
    {
        mPrintf("\n Fatal System Crash!\n Memory FUBAR!");
        crashout("Can not allocate message table after application");
    }

    if (!readMsgTab())
    {
        mPrintf("\n Fatal System Crash!\n Message table destroyed!");
        crashout("Message table lost in application");
    }

    if (!fread(logTab, (sizeof(*logTab) * cfg.MAXLOGTAB) , 1, fd))
    {
        mPrintf("\n Fatal System Crash!\n Log table destroyed!");
        crashout("Log table lost in application");
    }

    fclose(fd);

    unlink(scratch);
}

/* -------------------------------------------------------------------- */
/* tableOut   writes msg and log tab files to disk and frees RAM        */
/* -------------------------------------------------------------------- */
static int tableOut(void)
{
    FILE *fd;
    char scratch[64];

    if (dowhat != NETWORKING)
    {
        mPrintf("Saving system variables, please wait."); doCR();
    }

    if (cfg.homepath[ (strlen(cfg.homepath) - 1) ]  == '\\')
        cfg.homepath[ (strlen(cfg.homepath) - 1) ]  =  '\0';

    sprintf(scratch, swhacks, cfg.temppath, "tables.tmp");

    if ((fd  = fopen(scratch , "wb")) == NULL)
    {
        mPrintf("Can not save system tables!\n "); 
        return ERROR;
    }

    /* write out Msg.tab */
    writeMsgTab();

    _ffree((void *)msgTab1);       msgTab1 = NULL;
    _ffree((void *)msgTab2);       msgTab2 = NULL;
    _ffree((void *)msgTab3);       msgTab3 = NULL;
    _ffree((void *)msgTab4);       msgTab4 = NULL;
    _ffree((void *)msgTab5);       msgTab5 = NULL;
    _ffree((void *)msgTab6);       msgTab6 = NULL;
    _ffree((void *)msgTab7);       msgTab7 = NULL;
    _ffree((void *)msgTab8);       msgTab8 = NULL;
    _ffree((void *)msgTab9);       msgTab9 = NULL;

    /* write out Log.tab */
    fwrite(logTab, (sizeof(*logTab) * cfg.MAXLOGTAB), 1, fd);

    _ffree((void *)logTab);        logTab = NULL;

    fclose(fd);

    return TRUE;
}
#endif /* GOODBYE2 */

/* -------------------------------------------------------------------- */
/*      apsystem() turns off interupts and makes a system call          */
/* -------------------------------------------------------------------- */
void apsystem(const char *stuff)
{
    BOOL clearit = TRUE,
        superit = FALSE,
        batch   = FALSE,
        door    = TRUE;
    char scratch[256];
    static char *words[256];
    int  count; /* , i, i2; */


    while (*stuff == '!' || *stuff == '@' || *stuff == '$' || *stuff == '?')
    {
        if (*stuff == '!')            superit = TRUE;
        if (*stuff == '@')            clearit = FALSE;
        if (*stuff == '$')            batch   = TRUE;
        if (*stuff == '?')            door    = FALSE;
        stuff++;
    }
#ifdef GOODBYE2
    if (superit)
        if (tableOut() == ERROR)
            return;
#endif

    if (disabled)
      drop_dtr();

    portExit();

    fcloseall();

#if 1
    if (_fheapmin())
        cPrintf("Error minimizing heap\n");
#endif
    if(clearit)
    {
        save_screen();
        cls();
    }

    if (debug)
        cPrintf("(%s)\n", stuff);

    if (stricmp(stuff, getenv("COMSPEC")) == SAMESTRING)
        cPrintf("Use the EXIT command to return to %s \n", softname);
    else if (door)
        writeAplFile();
#if 1
    if (batch)
        sprintf(scratch, "%s /c %s", getenv("COMSPEC"), stuff);
    else
        strcpy(scratch, stuff);
    count = parse_it(words, scratch);
    words[count] = NULL;
#else
    if (!batch)
    {
        count = parse_it(words, stuff);
        words[count] = NULL;
    }
#endif

    if (_fheapmin())
        cPrintf("Error minimizing heap\n");

#if 1
    if (superit)
    {
        mPrintf("\n Now saving system state with Ralf Brown's SPAWNO.\n"
                " Please wait.\n");
        spawnvpo(cfg.temppath, words[0], words);
    }
    else
        spawnvp(P_WAIT, words[0], words);
#else
    if (batch)
    {
        system( stuff );
    }
    else
    {
        spawnv(P_WAIT, words[0], words);
    }
#endif
    setscreen();

    if(clearit)
        restore_screen();

    portInit();
    baud((int)speed);

#ifdef GOODBYE2
    if (superit) tableIn();
#endif

    if (aideFl != NULL)
    {
        sprintf(scratch, swhacks, cfg.temppath, "aidemsg.tmp");
        if ((aideFl = fopen(scratch, "a")) == NULL)
        {
            crashout("Can not open AIDEMSG.TMP!");
        }
    }
    trapfl = fopen(cfg.trapfile, "a+");
    sprintf(scratch, swhacks, cfg.msgpath, "msg.dat");
    openFile(scratch,    &msgfl );
    sprintf(scratch, swhacks, cfg.homepath, "grp.dat");
    openFile(scratch,  &grpfl );
    sprintf(scratch, swhacks, cfg.homepath, "hall.dat");
    openFile(scratch,  &hallfl );
    sprintf(scratch, swhacks, cfg.homepath, "log.dat");
    openFile(scratch,  &logfl );
    sprintf(scratch, swhacks, cfg.homepath, "room.dat");
    openFile(scratch,  &roomfl );

    if (disabled)
    {
        drop_dtr();
    }

    readAplFile();

    readMessage = TRUE;
}

/* -------------------------------------------------------------------- */
/*      readAplFile()  reads INPUT.APL, MESSAGE,APL, README.APL         */
/* -------------------------------------------------------------------- */
static void readAplFile(void)
{
    FILE *fd;
    int i;
    char buff[200];
    int item;
    int roomno;
    int found;
    int slot;

    if (readMessage)
    {
        clearmsgbuf();
        strcpy(msgBuf->mbauth, cfg.nodeTitle);
        msgBuf->mbroomno = (unsigned char)thisRoom;
    }

    sprintf(buff, swhacks, cfg.aplpath, input_apl);
    if ((fd = fopen(buff, "rt")) != NULL)
    {
        do
        {
            item = fgetc(fd);
            if (feof(fd)) 
            {
                break;
            }

            fgets(buff, 198, fd);
            buff[strlen(buff)-1] = 0;
    
            found = FALSE;

            for(i = 0; AplTab[i].item != APL_END; i++)
            {
                if (AplTab[i].item == item && AplTab[i].keep)
                {
                    found = TRUE;

                    switch(AplTab[i].type)
                    {
                    case TYP_STR:
                        strncpy((char *)AplTab[i].variable, buff, AplTab[i].length);
                        ((char *)AplTab[i].variable)[ AplTab[i].length - 1 ] = 0;
                        break;
    
                    case TYP_BOOL:
                    case TYP_CHAR:
                        *((char *)AplTab[i].variable) = (char)atoi(buff);
                        break;
    
                    case TYP_INT:
                        *((int *)AplTab[i].variable) = atoi(buff);
                        break;
    
                    case TYP_FLOAT:
                        *((float *)AplTab[i].variable) = (float)atof(buff);
                        break;
    
                    case TYP_LONG:
                        *((long *)AplTab[i].variable) = atol(buff);
                        break;
    
                    case TYP_OTHER:
                        switch (AplTab[i].item)
                        {
                        case APL_HALL:
                            if (stricmp(buff, hallBuf->hall[thisHall].hallname)
                                != SAMESTRING)
                            {
                                slot = hallexists(buff);
                                if (slot != ERROR)
                                {
                                    mPrintf("Hall change to: %s", buff);
                                    doCR();
                                    thisHall = (unsigned char)slot;
                                }
                                else
                                {
                                    cPrintf("No such hall %s!\n", buff);
                                }
                            }
                            break;
            
                        case APL_ROOM:
                            if ( (roomno = roomExists(buff)) != ERROR)
                            {
                                if (roomno != thisRoom)
                                {
                                    mPrintf("Room change to: %s", buff);
                                    doCR();
                                    logBuf.lbroom[thisRoom].lbgen   
                                            = roomBuf.rbgen; 
                                   ug_lvisit = logBuf.lbroom[thisRoom].lvisit;
                                   ug_new    = talleyBuf.room[thisRoom].new;
                                  ug_hasmail = talleyBuf.room[thisRoom].hasmail;
                                    logBuf.lbroom[thisRoom].lvisit   = 0; 
                                 /* logBuf.lbroom[thisRoom].mail     = 0; */
                                    /* zero new count in talleybuffer */
                                    talleyBuf.room[thisRoom].new      = 0;
                                    talleyBuf.room[thisRoom].hasmail  = 0;

                                    getRoom(roomno);
 
                                    if ((logBuf.lbroom[thisRoom].lbgen ) 
                                        != roomBuf.rbgen)
                                    {
                                        logBuf.lbroom[thisRoom].lbgen
                                                = roomBuf.rbgen;
                                        logBuf.lbroom[thisRoom].lvisit
                                                = (MAXVISIT - 1);
                                    }
                                }
                            }
                            else
                            {
                                cPrintf("No such room: %s!\n", buff);
                            }
                            break;
                        
                        case APL_PERMANENT:
                            lBuf.lbflags.PERMANENT = atoi(buff);
                            break;
                        
                        case APL_VERIFIED:
                            lBuf.VERIFIED = !atoi(buff);
                            break;
            
                        case APL_NETUSER:
                            lBuf.lbflags.NETUSER = atoi(buff);
                            break;
            
                        case APL_NOMAIL:
                            lBuf.lbflags.NOMAIL = atoi(buff);
                            break;
            
                        case APL_CHAT:
                            cfg.noChat = atoi(buff);
                            break;
            
                        case APL_BELLS:
                            cfg.noBells = atoi(buff);
                            break;
            
                        default:
                            mPrintf("Bad value %d \"%s\"", item, buff); doCR();
                            break;
                        }
                        break;
    
                    default:
                        break;
                    }
                }
            }

            if (!found && readMessage)
            {
                found = TRUE;

                switch (item)
                {
                case MSG_NAME:
                    strcpy(msgBuf->mbauth, buff);
                    break;
    
                case MSG_TO:
                    strcpy(msgBuf->mbto, buff);
                    break;
    
                case MSG_GROUP:
                    strcpy(msgBuf->mbgroup, buff);
                    break;
    
                case MSG_ROOM:
                    if ( (roomno = roomExists(buff)) == ERROR)
                    {
                        cPrintf(" AP: No room \"%s\"!\n", buff);
                    }
                    else
                    {
                        msgBuf->mbroomno = (unsigned char)roomno;
                    }
                    break;

                default:
                    doCR();
                    found = FALSE;
                    break;
                }
            }

            if (!found && AplTab[i].item != APL_END)
            {
                mPrintf("Bad value %d \"%s\"", item, buff); doCR();
            }
        }
        while (item != APL_END && !feof(fd));

#ifdef BAD
        sprintf(buff, swhacks, cfg.aplpath, input_apl);
        unlink(buff);
#endif        

        fclose(fd);
    }

    update25();

    sprintf(buff, swhacks, cfg.aplpath, message_apl);
    if (readMessage)  /* && */
    {
        if ((fd = fopen(buff, "rb")) != NULL)
        {
            GetFileMessage(fd, msgBuf->mbtext, cfg.maxtext);
            fclose(fd);
            unlink(buff);
      
#if 1
            if (strlen(msgBuf->mbtext) > 2)
            {
#endif
                putMessage();
                noteMessage();
#if 1
            }
#endif
        }
    }

    sprintf(buff, swhacks, cfg.aplpath, readme_apl);
    if (filexists(buff))
    {
        dumpf(buff);
        unlink(buff);
        doCR();
    }
    sprintf(buff, swhacks, cfg.aplpath, output_apl);
    unlink(buff);
}

/* -------------------------------------------------------------------- */
/* writeAplFile()                                                       */
/* -------------------------------------------------------------------- */
static void writeAplFile(void)
{
    FILE *fd;
    char buff[80];
    int i;

    sprintf(buff, swhacks, cfg.aplpath, input_apl);
    unlink(buff);
    if (readMessage)
    {
        sprintf(buff, swhacks, cfg.aplpath, message_apl);
        unlink(buff);
    }
    sprintf(buff, swhacks, cfg.aplpath, readme_apl);
    unlink(buff);
    sprintf(buff, swhacks, cfg.aplpath, output_apl);
    unlink(buff);

    if ((fd = fopen(buff, "wb")) == NULL)
    {
        mPrintf("Can't make OUTPUT.APL\n");
        return;
    }

    for (i = 0; AplTab[i].item != APL_END; i++)
    {
        switch(AplTab[i].type)
        {
        case TYP_STR:
            sprintf(buff, "%c%s\n", AplTab[i].item, AplTab[i].variable);
            break;

        case TYP_BOOL:
        case TYP_CHAR:
            sprintf(buff, "%c%d\n", AplTab[i].item,
                *((char *)AplTab[i].variable));
            break;

        case TYP_INT:
            sprintf(buff, "%c%d\n", AplTab[i].item,
                *((int *)AplTab[i].variable));
            break;

        case TYP_FLOAT:
            sprintf(buff, "%c%f\n", AplTab[i].item,
                    *((float *)AplTab[i].variable));
            break;

        case TYP_LONG:
            sprintf(buff, "%c%ld\n", AplTab[i].item,
                    *((long *)AplTab[i].variable));
            break;           

        case TYP_OTHER:
            switch (AplTab[i].item)
            {
            case APL_MDATA:
                if ((whichIO == CONSOLE))  
                {
                    sprintf(buff, "%c0 (LOCAL)\n", AplTab[i].item);
                }
                else
                {
                    sprintf(buff, "%c%d\n", AplTab[i].item, cfg.mdata);
                }
                break;

            case APL_HALL:
                sprintf(buff, "%c%s\n", AplTab[i].item, 
                        hallBuf->hall[thisHall].hallname);
                break;

            case APL_ROOM:
                sprintf(buff, "%c%s\n", AplTab[i].item, roomBuf.rbname);
                break;

            case APL_ACCOUNTING:
                if(!logBuf.lbflags.NOACCOUNT && cfg.accounting)
                {
                    sprintf(buff, "%c1\n", AplTab[i].item);
                }
                else
                {
                    sprintf(buff, "%c0\n", AplTab[i].item);
                }
                break;
            
            case APL_PERMANENT:
                sprintf(buff, "%c%d\n", AplTab[i].item, lBuf.lbflags.PERMANENT);
                break;
            
            case APL_VERIFIED:
                sprintf(buff, "%c%d\n", AplTab[i].item,
                        lBuf.VERIFIED ? 0 : 1);
                break;

            case APL_NETUSER:
                sprintf(buff, "%c%d\n", AplTab[i].item, lBuf.lbflags.NETUSER);
                break;

            case APL_NOMAIL:
                sprintf(buff, "%c%d\n", AplTab[i].item, lBuf.lbflags.NOMAIL);
                break;

            case APL_CHAT:
                sprintf(buff, "%c%d\n", AplTab[i].item, cfg.noChat);
                break;

            case APL_BELLS:
                sprintf(buff, "%c%d\n", AplTab[i].item, cfg.noBells);
                break;

            default:
                buff[0] = 0;
                break;
            }
            break;

        default:
            buff[0] = 0;
            break;
        }

        if (strlen(buff) > 1)
        {
            fputs(buff, fd);
        }
    }

    fprintf(fd, "%c\n", APL_END);
    
    fclose(fd);
}



/************************************************************************/
/*  wxsnd    Extended Download                                          */
/************************************************************************/ 
void wxsnd(const char *path, const char *file, char trans)
{
    char  stuff[100];
    label tmp1, tmp2;

    if (changedir(path) == -1 )  return;

    sprintf(tmp1, "%d", cfg.mdata);
    sprintf(tmp2, "%d", bauds[speed]);
    sformat(stuff, extrn[trans-1].ex_snd, "fpsa", file,
            tmp1, tmp2, cfg.aplpath);
    apsystem(stuff);
}

/************************************************************************/
/*  wxrcv    Extended Upload                                            */
/************************************************************************/ 
void wxrcv(const char *path, const char *file, char trans)
{
    char stuff[100];
    label tmp1, tmp2;

    if (changedir(path) == -1 )  return;

    sprintf(tmp1, "%d", cfg.mdata);
    sprintf(tmp2, "%d", bauds[speed]);
    sformat(stuff, extrn[trans-1].ex_rcv, "fpsa", file,
            tmp1, tmp2, cfg.aplpath);
    apsystem(stuff);
}
