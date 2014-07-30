/************************************************************************/
/*  GRPH.C                        ACit                         91Sep26  */
/*        hall and group code for Citadel bulletin board system         */
/************************************************************************/

#include <string.h>
#include "ctdl.h"
#include "prot.h"
#include "glob.h"

/************************************************************************/
/*                              Contents                                */
/*                                                                      */
/* $accesshall()            returns TRUE if person can access hall      */
/*  cleargroupgen()         removes logBuf from all groups              */
/*  defaulthall()           handles .ed  (enter Default-hallway)        */
/*  enterhall()             handles .eh                                 */
/*  gotodefaulthall()       goes to user's default hallway              */
/*  groupexists()           returns # of named group, else ERROR        */
/*  groupseeshall()         indicates if group can see hall #           */
/*  groupseesroom()         indicates if group can see room #           */
/*  hallexists()            returns # of named hall,  else ERROR        */
/*  ingroup()               returns TRUE if log is in named group       */
/*  iswindow()              for .kw .kvw is # room a window             */
/*  knownhalls()            handles .kh, .kvh                           */
/*  getgroup()              handles .RL                                 */
/*  partialgroup()          returns slot of partially named group       */
/*  partialhall()           returns slot of partially named hall        */
/*  readhalls()             handles .rh, .rvh                           */
/*  roominhall()            indicates if room# is in hall               */
/*  setgroupgen()           sets unmatching group generation #'s        */
/*  stephall()              handles previous, next hall                 */
/*                                                                      */
/************************************************************************/

static int accesshall(int slot);

/************************************************************************/
/*    accesshall() returns true if hall can be accessed                 */
/*    from current room                                                 */
/************************************************************************/
static int accesshall(int slot)
{
    int accessible = 0;

    if  ( (slot == (int)thisHall) || ( hallBuf->hall[slot].h_inuse
    && hallBuf->hall[slot].hroomflags[thisRoom].window
    && groupseeshall(slot)))
    {
        accessible = TRUE;
    }
    return accessible;
}


/************************************************************************/
/*    cleargroupgen()  removes logBuf from all groups                   */
/************************************************************************/
void cleargroupgen(void)
{
    int groupslot;

    for (groupslot = 0; groupslot < MAXGROUPS; groupslot++)
    {
        logBuf.groups[groupslot] =
            (unsigned char)((grpBuf.group[groupslot].groupgen
          + (MAXGROUPGEN - 1)) % MAXGROUPGEN);
    }
}


/************************************************************************/
/*      defaulthall() handles enter default hallway   .ed               */
/************************************************************************/
void defaulthall(void)
{
    label hallname;
    int slot, accessible = 1;

    getString("hallway", hallname, NAMESIZE, FALSE, ECHO, "");

    slot = hallexists(hallname);
    if (slot == ERROR)  slot = partialhall(hallname);

    if (slot != ERROR) accessible = accesshall(slot);

    if ( (slot == ERROR) || !strlen(hallname) || !accessible )
    {
        mPrintf("\n No such hall.");
        return;
    }

    strcpy(hallname, hallBuf->hall[slot].hallname);

    doCR();
    mPrintf(" Default hallway: %s", hallname);
    
    logBuf.hallhash = hash(hallname);

    /* 0 for root hallway */
    if (slot == 0) logBuf.hallhash = 0;

    if (loggedIn) storeLog();
}


/************************************************************************/
/*      enterhall()  handles .eh                                        */
/************************************************************************/
void enterhall(void)
{
    label hallname;
    int slot, accessible=1;

    getString("hall", hallname, NAMESIZE, FALSE, ECHO, "");

    slot = hallexists(hallname);
    if (slot == ERROR)  slot = partialhall(hallname);

    if (slot != ERROR)  accessible = accesshall(slot);

    if ( (slot == ERROR) || !strlen(hallname) || !accessible )
    {
        mPrintf(" No such hall.");
        return;
    }
    else 
    {
        thisHall = (unsigned char)slot;
    }
}

/************************************************************************/
/*      gotodefaulthall()  goes to user's default hallway               */
/************************************************************************/
void gotodefaulthall(void)
{
    int i;

    if (logBuf.hallhash)
    {
        for (i = 1; i < MAXHALLS; ++i)
        {
            if ( hash( hallBuf->hall[i].hallname )  == (unsigned int)logBuf.hallhash 
               && hallBuf->hall[i].h_inuse)
            {
                if (groupseeshall(i))  thisHall = (unsigned char)i;
            }
        }
    }
}

/************************************************************************/
/*      groupexists()  return # of named group, else ERROR              */
/************************************************************************/
int groupexists(const char *groupname)
{
    int i;

    for (i = 0; i < MAXGROUPS; i++)
    {
        if (grpBuf.group[i].g_inuse &&
            strcmpi(groupname, grpBuf.group[i].groupname) == SAMESTRING )
        return(i);
    }
    return(ERROR);
}


/************************************************************************/
/*      groupseeshall()  returns true if group can see hallway          */
/************************************************************************/
int groupseeshall(int hallslot)
{
    if ( (!hallBuf->hall[hallslot].owned) ||

    /* generation in logBuf for this hall's owning group */
    ( logBuf.groups[ hallBuf->hall[hallslot].grpno ]  ==

    /* generation in groupbuffer for this hall's owning group */
    grpBuf.group[ hallBuf->hall[hallslot].grpno ].groupgen  ))
    
    return(TRUE);

    return(FALSE);
}

/************************************************************************/
/*      groupseesroom()  returns true if group can see room             */
/************************************************************************/
int groupseesroom(int groupslot)
{
    if ( 
            (!roomTab[groupslot].rtflags.GROUPONLY) ||

    /* generation in logBuf for this room's owning group == */
    /* generation in groupbuffer for this room's owning group */
            ( logBuf.groups[ roomTab[groupslot].grpno ]  ==
              grpBuf.group[  roomTab[groupslot].grpno ].groupgen  )
       )
    {
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}


/************************************************************************/
/*      hallexists()  return # of named hall, else ERROR                */
/************************************************************************/
int hallexists(const char *hallname)
{
    int i;

    for (i = 0; i < MAXHALLS; i++)
    {
        if (hallBuf->hall[i].h_inuse &&
            strcmpi(hallname, hallBuf->hall[i].hallname) == SAMESTRING )
        return(i);
    }
    return(ERROR);
}

/************************************************************************/
/*      ingroup()  returns TRUE if person is in named group             */
/************************************************************************/
int ingroup(int groupslot)
{
    if ( (logBuf.groups[groupslot] == grpBuf.group[groupslot].groupgen)
        &&  grpBuf.group[groupslot].g_inuse)
        return(TRUE);
    return(FALSE);
}

/************************************************************************/
/*      iswindow()  is room a window into accessible halls?             */
/************************************************************************/
int iswindow(int roomslot)
{
    int i, window = 0;

    if (!roomTab[roomslot].rtflags.INUSE)  return(FALSE);

    for (i = 0; i < MAXHALLS && !window ; i++)
    {
        if (hallBuf->hall[i].h_inuse &&
            hallBuf->hall[i].hroomflags[roomslot].window )
        window = TRUE;
    }
    return(window);
}


/************************************************************************/
/*      knownhalls()  handles .kh .kvh                                  */
/************************************************************************/
void knownhalls(void)
{
    int i;

    doCR();

    mPrintf(" Hallways accessible.");

    doCR();

    prtList(LIST_START);
    
    for (i = 0; i < MAXHALLS; i++)
    {
        if (accesshall(i))
        {
            prtList(hallBuf->hall[i].hallname);
        }
    }
    
    prtList(LIST_END);
}


/************************************************************************/
/*      getgroup()  Read fn: to read by group (.RL)                     */
/************************************************************************/
void getgroup(void)
{
    label groupname;
    int groupslot;

    mf.mfGroup[0] = 0 /* NULL */;

    getString("group", groupname, NAMESIZE, FALSE, ECHO, "");

    if (!(*groupname))
        return;

    groupslot = partialgroup(groupname);

    if (groupslot == ERROR)
    {
        mPrintf("\n No such group!");
        doCR();
        mf.mfLim = FALSE;
        return;
    }

    if ( !(ingroup(groupslot) || sysop || aide))
    {
        mf.mfLim = FALSE;
        return;
    }

    if ( grpBuf.group[groupslot].lockout && (!logBuf.lbflags.SYSOP) )
    {
        mf.mfLim = FALSE;
        return;
    }

    mPrintf("\n Reading group %s only.\n ", grpBuf.group[groupslot].groupname);

    strcpy(mf.mfGroup, grpBuf.group[groupslot].groupname);
}

/************************************************************************/
/*      partialgroup()  returns slot # of partial group name, else error*/
/*      used for .EL Message, .EL Room and .AG .AL                      */
/************************************************************************/
int partialgroup(const char *groupname)
{
    label compare;
    int i, length;

    i = groupexists(groupname);
    if (i != ERROR)
        return i;

    length = strlen(groupname);

    for (i = 0; i < MAXGROUPS; i++)
    {
        if (grpBuf.group[i].g_inuse)
        {
            strcpy(compare, grpBuf.group[i].groupname);

            compare[ length ] = '\0';

            if ((strcmpi(compare, groupname) == SAMESTRING)
                && (ingroup(i) || aide))
            return(i);
        }
    }         
    return(ERROR);
}

/************************************************************************/
/*      partialhall()  returns slot # of partial hall name, else error  */
/*      used for .Enter Hallway and .Enter Default-hallway  only!       */
/************************************************************************/
int partialhall(const char *hallname)
{
    label compare;
    int i, length;

    i = hallexists(hallname);
    if (i != ERROR)
        return i;

    length = strlen(hallname);

    for (i = 0; i < MAXHALLS; i++)
    {
        if (hallBuf->hall[i].h_inuse)
        {
            strcpy(compare, hallBuf->hall[i].hallname);

            compare[ length ] = '\0';

            if ((strcmpi(compare, hallname) == SAMESTRING)
                && groupseeshall(i))
            return(i);
        }
    }         
    return(ERROR);
}

/************************************************************************/
/*      readhalls()  handles .rh .rvh                                   */
/************************************************************************/
void readhalls(void)
{
    int i;

    doCR();
    doCR();

    mPrintf("Room %s is contained in:", makeRoomName(thisRoom));
    doCR();

    prtList(LIST_START);

    for (i = 0; i < MAXHALLS; i++)
    {
        if  ( hallBuf->hall[i].h_inuse
        && hallBuf->hall[i].hroomflags[thisRoom].inhall
        && groupseeshall(i))

        prtList(hallBuf->hall[i].hallname);
    }

    prtList(LIST_END);
}

/************************************************************************/
/*    roominhall()  returns TRUE if room# is in current hall            */
/************************************************************************/
int roominhall(int roomslot)
{
    if ( hallBuf->hall[thisHall].hroomflags[roomslot].inhall )
    return(TRUE);

    return(FALSE);
}


/************************************************************************/
/*    setgroupgen()  sets unmatching group generation #'s               */
/************************************************************************/
void setgroupgen(void)
{
    int groupslot;

    for (groupslot = 0; groupslot < MAXGROUPS; groupslot++)
    {
        if (logBuf.groups[groupslot] != grpBuf.group[groupslot].groupgen)
        {
            logBuf.groups[groupslot] =
                (unsigned char)((grpBuf.group[groupslot].groupgen
                + (MAXGROUPGEN - 1)) % MAXGROUPGEN);
        }
    }
}

/************************************************************************/
/*    stephall()  handles previous, next hall                           */
/************************************************************************/
void stephall(int direction)
{
    int i;
    char done = FALSE;

    i = thisHall;

    do
    {
        /* step */
        if (direction == 1)
        {
            ++i;
            if (i == MAXHALLS ) 
                i = 0; 
        } else {
            --i;
            if ( i == -1 )
                i = MAXHALLS - 1;
        }

        /* keep from looping endlessly */
        if (i == (int)thisHall)
        {
#if 1
            mPrintf("\n\n  --Not a windowed room.");
#else
            mPrintf("%s ", hallBuf->hall[i].hallname);
#endif

            return;
        }

        if (hallBuf->hall[i].h_inuse)
        {
            /* is this room a window in hall we're checking */
            if  (hallBuf->hall[i].hroomflags[thisRoom].window)
            {       
                /* can group see this hall */
                if (groupseeshall(i))
                {
                    mPrintf("%s ", hallBuf->hall[i].hallname);
                    thisHall = (unsigned char)i;
                    done = TRUE;
                }
            }
        }

    } while ( !done );

    if (hallBuf->hall[thisHall].described && roomtell)
    {
        if (changedir(cfg.roompath) == -1 ) return;

        if (checkfilename(hallBuf->hall[thisHall].htell, 0) == ERROR)
        {
            changedir(cfg.homepath);
            return;
        }

        doCR();
    
        if (!filexists(hallBuf->hall[thisHall].htell))
        {
            doCR();
            mPrintf("No hallway description %s", hallBuf->hall[thisHall].htell);
            changedir(cfg.homepath);
            doCR();
            return;
        }
    
        if (!expert)
        {
            doCR();
            mPrintf("<J>ump <P>ause <S>top");
            doCR();
        }
    
        /* print it out */
        dumpf(hallBuf->hall[thisHall].htell);
    
        /* go to our home-path */
        changedir(cfg.homepath);
    
        outFlag = OUTOK;
    }
}
