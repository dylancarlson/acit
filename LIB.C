/************************************************************************/
/*  LIB.C                         ACit                         91Sep30  */
/*                                                                      */
/*                  Routines used by Ctdl & Confg                       */
/************************************************************************/

#include <direct.h>
#include <malloc.h>
#include "ctdl.h"
#include "prot.h"
#include "glob.h"

/************************************************************************/
/*                              contents                                */
/*                                                                      */
/*  getGroup()              loads groupBuffer                           */
/*  putGroup()              stores groupBuffer to disk                  */
/*  getHall()               loads hallBuffer                            */
/*  putHall()               stores hallBuffer to disk                   */
/*  getLog()                loads requested CTDLLOG record              */
/*  putLog()                stores a logBuffer into citadel.log         */
/*  getRoom()               load given room into RAM                    */
/*  putRoom()               store room to given disk slot               */
/*  writeTables()           writes all system tables to disk            */
/*  readTables()            loads all tables into ram                   */
/*  allocateTables()        allocate table space with halloc            */
/* $readMsgTab()            Avoid the 64K limit. (stupid segments)      */
/* $writeMsgTab()           Avoid the 64K limit. (stupid segments)      */
/*                                                                      */
/************************************************************************/

static int readMsgTab(void);
static void writeMsgTab(void);

/************************************************************************/
/*      getGrooup() loads group data into RAM buffer                    */
/************************************************************************/
void getGroup(void)
{
    fseek(grpfl, 0L, 0);

    if (fread(&grpBuf, sizeof grpBuf, 1, grpfl) != 1)
    {
        crashout("getGroup-read fail EOF detected!");
    }
}

/************************************************************************/
/*      putGroup() stores group data into grp.cit                       */
/************************************************************************/
void putGroup(void)
{
    fseek(grpfl, 0L, 0);

    if (fwrite(&grpBuf, sizeof grpBuf, 1, grpfl) != 1)
    {
        crashout("putGroup-write fail");
    }
    fflush(grpfl);
}

/************************************************************************/
/*      getHall() loads hall data into RAM buffer                       */
/************************************************************************/
void getHall(void)
{
    fseek(hallfl, 0L, 0);

    if (fread(hallBuf, sizeof (struct hallBuffer), 1, hallfl) != 1)
    {
        crashout("getHall-read fail EOF detected!");
    }
}

/************************************************************************/
/*      putHall() stores group data into hall.cit                       */
/************************************************************************/
void putHall(void)
{
    fseek(hallfl, 0L, 0);

    if (fwrite(hallBuf, sizeof (struct hallBuffer), 1, hallfl) != 1)
    {
        crashout("putHall-write fail");
    }
    fflush(hallfl);
}

/************************************************************************/
/*      getLog() loads requested log record into RAM buffer             */
/************************************************************************/
void getLog(struct logBuffer *lBuf, int n)
{
    long int s;

    if (lBuf == &logBuf)  thisLog = n;

    s = (long)n * (long)sizeof logBuf;

    fseek(logfl, s, 0);

    if (fread(lBuf, sizeof logBuf, 1, logfl) != 1)
    {
        crashout("getLog-read fail EOF detected!");
    }
}

/************************************************************************/
/*      putLog() stores given log record into log.cit                   */
/************************************************************************/
void putLog(const struct logBuffer *lBuf, int n)
{
    long int s;

    s = (long)n * (long)(sizeof(struct logBuffer));

    fseek(logfl, s, 0);  

    if (fwrite(lBuf, sizeof logBuf, 1, logfl) != 1)
    {
        crashout("putLog-write fail");
    }
    fflush(logfl);
}

/************************************************************************/
/*      getRoom()                                                       */
/************************************************************************/
void getRoom(int rm)
{
    long int s;

    /* load room #rm into memory starting at buf */
    thisRoom    = rm;
    s = (long)rm * (long)sizeof roomBuf;

    fseek(roomfl, s, 0);
    if (fread(&roomBuf, sizeof roomBuf, 1, roomfl) != 1)  
    {
        crashout("getRoom(): read failed error or EOF!");
    }
}

/************************************************************************/
/*      putRoom() stores room in buf into slot rm in room.buf           */
/************************************************************************/
void putRoom(int rm)
{
    long int s;

    s = (long)rm * (long)sizeof roomBuf;

    fseek(roomfl, s, 0);
    if (fwrite(&roomBuf, sizeof roomBuf, 1, roomfl) != 1)
    {
        crashout("putRoom() crash! 0 returned.");
    }
    fflush(roomfl);
}

/************************************************************************/
/*      readTables()  loads all tables into ram                         */
/************************************************************************/
int readTables(void)
{
    FILE  *fd;

    getcwd(etcpath, 64);

    /*
     * ETC.DAT
     */
    if ((fd  = fopen("etc.dat" , "rb")) == NULL)
        return(FALSE);
    if (!fread(&cfg, sizeof cfg, 1, fd))
    {
        fclose(fd);
        return FALSE;
    }
    fclose(fd);
    unlink("etc.dat");

    changedir(cfg.homepath);

    allocateTables();
    if (logTab == NULL)
        crashout("Error allocating logTab \n");
    if (msgTab1 == NULL || msgTab2 == NULL || msgTab3 == NULL ||
        msgTab4 == NULL || msgTab5 == NULL || msgTab6 == NULL ||
        msgTab7 == NULL || msgTab8 == NULL || msgTab9 == NULL)
        crashout("Error allocating msgTab \n");

    /*
     * LOG.TAB
     */
    if ((fd  = fopen("log.tab" , "rb")) == NULL)
        return(FALSE);
    if (!fread(logTab, sizeof(struct lTable), cfg.MAXLOGTAB, fd))
    {
        fclose(fd);
        return FALSE;
    }
    fclose(fd);
    unlink("log.tab" );

    /*
     * MSG.TAB
     */
    if (readMsgTab() == FALSE)  return FALSE;

    /*
     * ROOM.TAB
     */
    if ((fd = fopen("room.tab", "rb")) == NULL)
        return(FALSE);
    if (!fread(roomTab, sizeof(struct rTable), MAXROOMS, fd))
    {
        fclose(fd);
        return FALSE;
    }
    fclose(fd);
    unlink("room.tab");

    return(TRUE);
}

/************************************************************************/
/*      writeTables()  stores all tables to disk                        */
/************************************************************************/
void writeTables(void)
{
    FILE  *fd;

    changedir(etcpath);

    if ((fd     = fopen("etc.dat" , "wb")) == NULL)
    {
        crashout("Can't make Etc.dat");
    }
    /* write out Etc.dat */
    fwrite(&cfg, sizeof cfg, 1, fd);
    fclose(fd);

    changedir(cfg.homepath);

    if ((fd  = fopen("log.tab" , "wb")) == NULL)
    {
        crashout("Can't make Log.tab");
    }
    /* write out Log.tab */
    fwrite(logTab, sizeof(struct lTable), cfg.MAXLOGTAB, fd);
    fclose(fd);
 
    writeMsgTab();

    if ((fd = fopen("room.tab", "wb")) == NULL)
    {
        crashout("Can't make Room.tab");
    }
    /* write out Room.tab */
    fwrite(roomTab, sizeof(struct rTable), MAXROOMS, fd);
    fclose(fd);

    changedir(etcpath);
}

/************************************************************************/
/*    allocateTables()   allocate msgTab and logTab                     */
/************************************************************************/
void allocateTables(void)
{
    chkptr(logTab);
    logTab =  _fcalloc(cfg.MAXLOGTAB+1, sizeof(struct lTable));
    chkptr(msgTab1);
    msgTab1 = _fcalloc(cfg.nmessages+1, sizeof(struct messagetable1));
    chkptr(msgTab2);
    msgTab2 = _fcalloc(cfg.nmessages+1, sizeof(struct messagetable2));
    chkptr(msgTab3);
    msgTab3 = _fcalloc(cfg.nmessages+1, sizeof(struct messagetable3));
    chkptr(msgTab4);
    msgTab4 = _fcalloc(cfg.nmessages+1, sizeof(struct messagetable4));
    chkptr(msgTab5);
    msgTab5 = _fcalloc(cfg.nmessages+1, sizeof(struct messagetable5));
    chkptr(msgTab6);
    msgTab6 = _fcalloc(cfg.nmessages+1, sizeof(struct messagetable6));
    chkptr(msgTab7);
    msgTab7 = _fcalloc(cfg.nmessages+1, sizeof(struct messagetable7));
    chkptr(msgTab8);
    msgTab8 = _fcalloc(cfg.nmessages+1, sizeof(struct messagetable8));
    chkptr(msgTab9);
    msgTab9 = _fcalloc(cfg.nmessages+1, sizeof(struct messagetable9));
}

/* -------------------------------------------------------------------- */
/*  readMsgTab()     Avoid the 64K limit. (stupid segments)             */
/* -------------------------------------------------------------------- */
static int readMsgTab(void)
{
    FILE *fd;
    char temp[80];

    sprintf(temp, "%s\\%s", cfg.homepath, "msg.tab");

    if ((fd  = fopen(temp , "rb")) == NULL)
        return(FALSE);

    if (!fread(msgTab1, sizeof(*msgTab1), cfg.nmessages, fd)) return(FALSE);
    if (!fread(msgTab2, sizeof(*msgTab2), cfg.nmessages, fd)) return(FALSE);
    if (!fread(msgTab3, sizeof(*msgTab3), cfg.nmessages, fd)) return(FALSE);
    if (!fread(msgTab4, sizeof(*msgTab4), cfg.nmessages, fd)) return(FALSE);
    if (!fread(msgTab5, sizeof(*msgTab5), cfg.nmessages, fd)) return(FALSE);
    if (!fread(msgTab6, sizeof(*msgTab6), cfg.nmessages, fd)) return(FALSE);
    if (!fread(msgTab7, sizeof(*msgTab7), cfg.nmessages, fd)) return(FALSE);
    if (!fread(msgTab8, sizeof(*msgTab8), cfg.nmessages, fd)) return(FALSE);
    if (!fread(msgTab9, sizeof(*msgTab9), cfg.nmessages, fd)) return(FALSE);
    
    fclose(fd);
    unlink(temp);

    return TRUE;
}

/* -------------------------------------------------------------------- */
/*  writeMsgTab()     Avoid the 64K limit. (stupid segments)            */
/* -------------------------------------------------------------------- */
static void writeMsgTab(void)
{
    FILE *fd;
    char temp[80];

    sprintf(temp, "%s\\%s", cfg.homepath, "msg.tab");

    if ((fd  = fopen(temp , "wb")) == NULL)
        return;

    fwrite(msgTab1, sizeof(*msgTab1), cfg.nmessages , fd);
    fwrite(msgTab2, sizeof(*msgTab2), cfg.nmessages , fd);
    fwrite(msgTab3, sizeof(*msgTab3), cfg.nmessages , fd);
    fwrite(msgTab4, sizeof(*msgTab4), cfg.nmessages , fd);
    fwrite(msgTab5, sizeof(*msgTab5), cfg.nmessages , fd);
    fwrite(msgTab6, sizeof(*msgTab6), cfg.nmessages , fd);
    fwrite(msgTab7, sizeof(*msgTab7), cfg.nmessages , fd);
    fwrite(msgTab8, sizeof(*msgTab8), cfg.nmessages , fd);
    fwrite(msgTab9, sizeof(*msgTab9), cfg.nmessages , fd);

    fclose(fd);
}
