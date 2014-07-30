/************************************************************************/
/* ROOM.C                         ACit                         91Sep30  */
/*              room code for Citadel bulletin board system             */
/************************************************************************/

#include <string.h>
#include "ctdl.h"
#include "prot.h"
#include "glob.h"

/************************************************************************/
/*                              Contents                                */
/*                                                                      */
/* $canseeroom()            returns TRUE if user can see a room         */
/* $dumpRoom()              tells us # new messages etc                 */
/*  gotoRoom()              handles "g(oto)" command for menu           */
/*  listRooms()             lists known rooms                           */
/*  RoomStatus()            shows the status of a room...               */
/* $partialExist()          returns slot# of partially named room       */
/*  printrm()               displays roomname, prompt style             */
/* $printroomVer()          displays roomname, description, msg counts  */
/*  makeRoomName()          returns pointer to "roomname:])" string     */
/* $roomdescription()       prints out room description                 */
/*  roomExists()            returns slot# of named room else ERROR      */
/*  roomtalley()            talleys up total,messages & new             */
/*  givePrompt()            prints the usual "CURRENTROOM>" prompt.     */
/*  indexRooms()           build index to ROOM.CIT, for del empty rooms */
/*  noteRoom()              enter room into RAM index array.            */
/*  stepRoom()              1 for next 0, 0 for previous room           */
/*  unGotoRoom()            jump to previous room                       */
/************************************************************************/

static int canseeroom(int roomslot);
static void dumpRoom(void);
static int partialExist(char *roomname);
static void printroomVer(int room, int verbose, char numMess);
static void roomdescription(void);

/************************************************************************/
/*      canseeroom() returns TRUE if user has access to room            */
/************************************************************************/
static int canseeroom(int roomslot)
{ 
    /* is room in use              */
    if ( roomTab[roomslot].rtflags.INUSE

    /* and room's in this hall     */
    &&  roominhall(roomslot)

    /* and group can see this room */
    &&  (groupseesroom(roomslot)
    || roomTab[roomslot].rtflags.READONLY
    || roomTab[roomslot].rtflags.DOWNONLY 
    || roomTab[roomslot].rtflags.UPONLY )

    /* only aides go to aide room  */ 
    &&   ( roomslot != AIDEROOM || aide) )

    return(TRUE);

    return(FALSE);

}

/************************************************************************/
/*      dumpRoom() tells us # new messages etc                          */
/************************************************************************/
static void dumpRoom(void)
{
    int   total, messages, new;
    
    total    = talleyBuf.room[thisRoom].total;
    messages = talleyBuf.room[thisRoom].messages;
    new      = talleyBuf.room[thisRoom].new;

    if (    cfg.roomtell 
         && roomBuf.rbroomtell[0] 
         && logBuf.lbroom[thisRoom].lvisit
       )
    {
        roomdescription();
    }
    else if (*roomBuf.descript)
    {
        mPrintf(" %s", roomBuf.descript);
        doCR();
    }

    if (loggedIn && aide)
    {
        mPrintf(" %d total",  total);
        doCR();
    }

    mPrintf(" %d %s", messages, (messages == 1)? cfg.msg_nym: cfg.msgs_nym);

    if (loggedIn)
    {
        if (new)
        {
            doCR();
            mPrintf(" %d new ", new );
        }

       /* if (logBuf.lbroom[thisRoom].mail) */
        if (talleyBuf.room[thisRoom].hasmail)
        {
            doCR();
            mPrintf(" You have mail here. ");
        }
    }
}

/************************************************************************/
/*      gotoRoom() is the menu fn to travel to a new room               */
/*      returns TRUE if room is Lobby>, else FALSE                      */
/************************************************************************/
int gotoRoom(char *roomname)    /* partialexist */
{
    int  i, j, foundit, roomNo = ERROR;
    int check, foundflag = 0;
                   
    oldroom = thisRoom;

    logBuf.lbroom[thisRoom].lbgen    = roomBuf.rbgen; 
    if (!skiproom)
    {
        ug_lvisit  = logBuf.lbroom[thisRoom].lvisit;
        ug_new     = talleyBuf.room[thisRoom].new;
        ug_hasmail = talleyBuf.room[thisRoom].hasmail;

        logBuf.lbroom[thisRoom].lvisit   = 0; 
        /* logBuf.lbroom[thisRoom].mail     = 0;  */
        talleyBuf.room[thisRoom].hasmail = 0;

        /* zero new count in talleybuffer */
        talleyBuf.room[thisRoom].new     = 0;
    }
    skiproom = FALSE;

    if (!strlen(roomname))
    {
        foundit = FALSE;      /* leaves us in Lobby> if nothing found */

        j = oldroom + 1;

        for (i = 0; i < MAXROOMS  &&  !foundit; i++, ++j)
        {
            if (j == MAXROOMS) j = 0;
            /* can user access this room?         */
            if ( canseeroom(j)

            /* does it have new messages,         */
            && (  talleyBuf.room[j].new ||

                /* or is it a window?              */
                (iswindow(j) && j>oldroom && thisHall != 1
                 && cfg.subhubs==2))     /* DragCit-style, empties visited */

            /* is it NOT excluded                 */
            &&  (!logBuf.lbroom[j].xclude ||  /* logBuf.lbroom[j].mail */
                                              talleyBuf.room[j].hasmail  )


            /* we dont come back to current room  */
            &&   j != thisRoom

            /* and is it K>nown                   */
            && (roomTab[j].rtgen == logBuf.lbroom[j].lbgen) )
            {
                foundit = j;
            }
        }

/*  special subhub section  */

        if ( !foundit && cfg.subhubs)
        {
            j = oldroom + 1;

            for (i = 0; i < MAXROOMS && !foundflag; ++i, ++j)
            { 
                if (j == MAXROOMS) j = 0;

                if (iswindow(j) 

                /* can user access this room?     */
                && canseeroom(j)

                /* is it NOT excluded             */
                &&  !logBuf.lbroom[j].xclude 

                /* and is it K>nown               */
                && (roomTab[j].rtgen == logBuf.lbroom[j].lbgen) )
                {
                    foundit   = j;
                    foundflag = TRUE;
                }
            }

            if (!foundflag && !roominhall(LOBBY) )
            {
#if 0
                for (i = thisRoom+1; i != thisRoom && !foundit; ++i)
                {
                    if (i == MAXROOMS)  i = 0;
#else
                for (i = 0; i < MAXROOMS && !foundit; ++i)
                { 
#endif
                    /* can user access this room? */
                    if ( canseeroom(i)

                    /* is it NOT excluded         */
                    &&  !logBuf.lbroom[i].xclude 

                    /* and is it K>nown           */
                    && (roomTab[i].rtgen == logBuf.lbroom[i].lbgen) )
                    {
                        foundit = i;
                    }
                } 
            }
        }

/* ^ special subhub section ^ */

        getRoom(foundit);

        mPrintf("%s ", roomBuf.rbname);

        if (iswindow(foundit) && logBuf.NEXTHALL) 
        {
            mPrintf("AutoSkip to "); 
            stephall(1);
            doCR();
        }
        doCR(); 
    }
    else
    {
        foundit = FALSE;

        check = roomExists(roomname);

        if (!canseeroom(check)) check = ERROR;

        if (check == ERROR) check = partialExist(roomname);

        if (check != ERROR) roomNo = check;

        if (roomNo != ERROR && canseeroom(roomNo) )
        {

            foundit = roomNo;
            getRoom(roomNo);

            /* if may have been unknown... if so, note it:      */
            if ((logBuf.lbroom[thisRoom].lbgen ) != roomBuf.rbgen)
            {
                logBuf.lbroom[thisRoom].lbgen    = roomBuf.rbgen;
                logBuf.lbroom[thisRoom].lvisit = (MAXVISIT - 1);
            }
        }
        else
        {
            mPrintf(" No '%s' room\n ", roomname);
        }
    }

    dumpRoom();  

    return foundit;
}

/***********************************************************************/
/*     listRooms() lists known rooms                                   */
/***********************************************************************/
void listRooms(unsigned int what, char verbose, char numMess)
{
    int i, j; 
    char firstime;

    outFlag = OUTOK;

    showdir    = 0;
    showhidden = 0;
    showgroup  = 0;

    /* criteria for NEW rooms */
 
    if (what == NEWRMS || what == OLDNEW)
    {
        doCR();
        termCap(TERM_BOLD);
        mPrintf(" Rooms with unread %s along %s:", cfg.msgs_nym,
                hallBuf->hall[thisHall].hallname);
        termCap(TERM_NORMAL);
        doCR();
 
        prtList(LIST_START);
        for ( i = 0; i < MAXROOMS && (outFlag != OUTSKIP); i++)
        {
            if ( canseeroom(i)
            &&  (roomTab[i].rtgen == logBuf.lbroom[i].lbgen)
            &&   talleyBuf.room[i].new
            &&  !logBuf.lbroom[i].xclude )
            {
                printroomVer(i, verbose, numMess);
            }
        }
        prtList(LIST_END);
    }
    if (outFlag == OUTSKIP) return;

#if 1
    if (outFlag == OUTPARAGRAPH)     outFlag = OUTOK;
#endif   
    /* for dir rooms */

    if (what == DIRRMS || what == APLRMS || what == LIMRMS || what == SHRDRM)
    {
        firstime = TRUE;

        for ( i = 0; i < MAXROOMS && (outFlag != OUTSKIP); i++)
        {
            if ( canseeroom(i)
            &&  (roomTab[i].rtgen == logBuf.lbroom[i].lbgen)
            &&  ( (roomTab[i].rtflags.MSDOSDIR  && what == DIRRMS)
               || (roomTab[i].rtflags.APLIC     && what == APLRMS)
               || (roomTab[i].rtflags.SHARED    && what == SHRDRM)
               || (roomTab[i].rtflags.GROUPONLY && what == LIMRMS) ) )
            {
                if (firstime)
                {
                    doCR();
                    termCap(TERM_BOLD);
                    mPrintf(" %s room:", 
                       what == DIRRMS ? "Directory"      :
                       what == LIMRMS ? "Limited Access" :
                       what == SHRDRM ? "Shared"         : "Application");
                    termCap(TERM_NORMAL);
                    doCR();

                    firstime = FALSE;
                    prtList(LIST_START);
                }
                printroomVer(i, verbose, numMess);
            }
        }
        prtList(LIST_END);
    }
    if (outFlag == OUTSKIP) return;
#if 1
    if (outFlag == OUTPARAGRAPH)     outFlag = OUTOK;
#endif   

 
    /* criteria for MAIL rooms */

    if (what == NEWRMS || what == OLDNEW || what == MAILRM) 
    {
        firstime = TRUE;
 
        for ( i = 0; i < MAXROOMS && (outFlag != OUTSKIP); i++)
        {
            if ( canseeroom(i)
            &&   (roomTab[i].rtgen == logBuf.lbroom[i].lbgen)
            &&   talleyBuf.room[i].hasmail /* logBuf.lbroom[i].mail */ )
            {
                if (firstime)
                {
                    doCR();
                    termCap(TERM_BOLD);
                    mPrintf(" You have private mail in:");
                    termCap(TERM_NORMAL);
                    doCR();

                    firstime = FALSE;
                    prtList(LIST_START);
                }
                printroomVer(i, verbose, numMess);
            }
        }
        prtList(LIST_END);
    }
    if (outFlag == OUTSKIP) return;
#if 1
    if (outFlag == OUTPARAGRAPH)     outFlag = OUTOK;
#endif   
 
    /* criteria for OLD rooms */
 
    if (what == OLDNEW || what == OLDRMS)
    {
        doCR();
        termCap(TERM_BOLD);
        mPrintf(" No unseen %s in:", cfg.msgs_nym);
        termCap(TERM_NORMAL);
        doCR();
 
        prtList(LIST_START);
        for ( i = 0; i < MAXROOMS && (outFlag != OUTSKIP); i++)
        {
            if ( canseeroom(i)
            &&  (roomTab[i].rtgen == logBuf.lbroom[i].lbgen)
            &&  !talleyBuf.room[i].new )
            {
                printroomVer(i, verbose, numMess);
            }
        }
        prtList(LIST_END);
    }
    if (outFlag == OUTSKIP) return;
#if 1
    if (outFlag == OUTPARAGRAPH)     outFlag = OUTOK;
#endif   
 
    /* criteria for EXCLUDED rooms */
 
    if (what == OLDNEW || what == XCLRMS)
    {
        firstime = TRUE;
 
        prtList(LIST_START);
        for ( i = 0; i < MAXROOMS && (outFlag != OUTSKIP); i++)
        {
            if ( canseeroom(i)
            &&   (roomTab[i].rtgen == logBuf.lbroom[i].lbgen) 
            &&   logBuf.lbroom[i].xclude
            &&   (talleyBuf.room[i].new || what == XCLRMS) )
            {
                if (firstime) 
                {
                    doCR();
                    termCap(TERM_BOLD);
                    mPrintf(" Excluded rooms:");
                    termCap(TERM_NORMAL);
                    doCR();

                    firstime = FALSE;
                }
                printroomVer(i, verbose, numMess);
            }
        }
        prtList(LIST_END);
    }
    if (outFlag == OUTSKIP) return;
#if 1
    if (outFlag == OUTPARAGRAPH)     outFlag = OUTOK;
#endif   

    /* criteria for WINDOWS */
 
    if ( what == WINDWS )
    {
        doCR();
        termCap(TERM_BOLD);
        mPrintf(" Rooms exiting to other halls:");
        termCap(TERM_NORMAL);
        doCR();
        
        if (!verbose) prtList(LIST_START);
        for ( i = 0; i < MAXROOMS && (outFlag != OUTSKIP); i++)
        {
            if ( canseeroom(i)
            &&   (roomTab[i].rtgen == logBuf.lbroom[i].lbgen) 
            &&   iswindow(i) )
            {
                if (verbose)
                {
                    mPrintf("  %s : ", makeRoomName(i));
                
                }
                else
                {
                    printroomVer(i, FALSE, FALSE);
                }

                if (verbose)
                {
                    prtList(LIST_START);
                    
                    for (j = 0 ; j < MAXHALLS; j++)
                    {
                        if ( hallBuf->hall[j].hroomflags[i].window 
                        &&   hallBuf->hall[j].h_inuse
                        &&   groupseeshall(j) )
                        {
                            prtList(hallBuf->hall[j].hallname);
                        }
                    }
                    prtList(LIST_END);
                }
            }
        }
        if (!verbose) prtList(LIST_END);
    }
    if (outFlag == OUTSKIP) return;
#if 1
    if (outFlag == OUTPARAGRAPH)     outFlag = OUTOK;
#endif   

    if (!expert)
    {
        doCR();
        termCap(TERM_BOLD);
        if (showhidden)                   mPrintf(" ) => hidden room"    );
        if (showgroup)                    mPrintf(" : => group only room");
        if (showdir && what != DIRRMS)    mPrintf(" ] => directory room" );
        termCap(TERM_NORMAL);
    }
}

/* -------------------------------------------------------------------- */
/*  RoomStatus() Shows the status of a room...                          */
/* -------------------------------------------------------------------- */
void RoomStatus(void)
{
    char buff[500];
    int j;
    
    doCR();
    formatSummary(buff);
    mPrintf(buff);
    doCR();
    doCR();
   
    mPrintf("Windowed in Halls:");
    doCR();
  
    prtList(LIST_START);
    for (j = 0; j < MAXHALLS; j++)
    {
        if ( hallBuf->hall[j].hroomflags[thisRoom].window 
          && hallBuf->hall[j].h_inuse
          && groupseeshall(j) )
        {
            prtList(hallBuf->hall[j].hallname);
        }
    }
    prtList(LIST_END);
    readhalls();
    doCR();
} 

/************************************************************************/
/*      partialExist() the list looking for a partial match             */
/************************************************************************/
static int partialExist(char *roomname)   /* substr(roomname) */
{ 
    label compare;
    int i, j, length;

    length = strlen(roomname);

    j = thisRoom + 1;

    for (i = 0; i < MAXROOMS; ++i, ++j)
    {
        if ( j == MAXROOMS ) j = 0;

        if (roomTab[j].rtflags.INUSE &&
        (roomTab[j].rtgen == logBuf.lbroom[j].lbgen) )
        {
            /* copy roomname into scratch buffer */
            strcpy(compare, roomTab[j].rtname);

            /* make both strings the same length */
            compare[ length ] = '\0'; 
          
            if ((strcmpi(compare, roomname) == SAMESTRING) && roominhall(j)
                && canseeroom(j) )
                return(j);
        } 
    }

    for (i = 0, j = thisRoom + 1; i < MAXROOMS; ++i, ++j)
    {
        if ( j == MAXROOMS ) j = 0;

        if (roomTab[j].rtflags.INUSE &&
        (roomTab[j].rtgen == logBuf.lbroom[j].lbgen) )
        {
            if (substr(roomTab[j].rtname, roomname)
                && roominhall(j)
                && canseeroom(j) )
                return(j);
        } 
    }
    return(ERROR);
}

/************************************************************************/
/*  printrm()               displays roomname, prompt style             */
/************************************************************************/
void printrm(int room)
{  
    char string[NAMESIZE + 10];

    strcpy(string, roomTab[room].rtname);

    if  (roomTab[room].rtflags.MSDOSDIR)
        strcat(string, "]");
    if  (roomTab[room].rtflags.GROUPONLY)
        strcat(string, ":");
    if (!roomTab[room].rtflags.PUBLIC)
        strcat(string, ")");
    else if (!roomTab[room].rtflags.GROUPONLY && 
             !roomTab[room].rtflags.MSDOSDIR)
        strcat(string, ">");
    if (iswindow(room)) strcat(string, ">");

    if ( roomTab[room].rtflags.GROUPONLY)showgroup   = TRUE;
    if ( roomTab[room].rtflags.MSDOSDIR) showdir     = TRUE;
    if (!roomTab[room].rtflags.PUBLIC)   showhidden  = TRUE;

    mPrintf("%s",string);
}

/************************************************************************/
/*  printroomVer()          displays roomname, description, msg counts  */
/************************************************************************/
static void printroomVer(int room, int verbose, char numMess)
{  
    char *string;
    int oldRoom;

    string = makeRoomName(room);

    if ( roomTab[room].rtflags.GROUPONLY)showgroup   = TRUE;
    if ( roomTab[room].rtflags.MSDOSDIR) showdir     = TRUE;
    if (!roomTab[room].rtflags.PUBLIC)   showhidden  = TRUE;

    if (!verbose && !numMess)
    {
        /*
        mPrintf("%s ",string);
        */
        prtList(string);
    }
    else
    {
        oldRoom = thisRoom;
        getRoom(room);
        mPrintf("%-23s ", string);

        if (numMess)
        {
            if (aide)
            {
                mPrintf("%3d total, ", talleyBuf.room[room].total);
            }
            mPrintf("%3d %s, %3d new", talleyBuf.room[room].messages,
                                       cfg.msgs_nym,
                                       talleyBuf.room[room].new );

            if (talleyBuf.room[room].new && talleyBuf.room[room].hasmail 
/* logBuf.lbroom[room].mail */ )
            {
                mPrintf(", (Mail)");
            }
            
            doCR();

            if (*roomBuf.descript && verbose)
            {
                mPrintf("    ");
            }
        }

        if (verbose)
        {
            if (*roomBuf.descript)
                mPrintf("%s", roomBuf.descript);
            if (!numMess || *roomBuf.descript)
                doCR();
        }

        if (verbose && numMess)  doCR();

        getRoom(oldRoom);
    }
}

/* -------------------------------------------------------------------- */
/*  makeRoomName()          returns pointer to "roomname:])" string     */
/* -------------------------------------------------------------------- */
char *makeRoomName(int room)
{
    static char string[NAMESIZE+NAMESIZE];

    strcpy(string, roomTab[room].rtname);

    if  (roomTab[room].rtflags.MSDOSDIR)
        strcat(string, "]");
    if  (roomTab[room].rtflags.GROUPONLY)
        strcat(string, ":");
    if (!roomTab[room].rtflags.PUBLIC)
        strcat(string, ")");
    else if (!roomTab[room].rtflags.GROUPONLY && 
             !roomTab[room].rtflags.MSDOSDIR)
        strcat(string, ">");
    if (iswindow(room)) strcat(string, ">");

    return string;
}

/************************************************************************/
/*      roomdescription()  prints out room description                  */
/************************************************************************/
static void roomdescription(void)
{
    outFlag     = OUTOK;

    if (!roomtell) return;

    if (!roomBuf.rbroomtell[0]) return;

    /* only do room description upon first visit this call to a room    */
    if (!logBuf.lbroom[thisRoom].lvisit) return;

    if (changedir(cfg.roompath) == -1 ) return;

    /* no bad files */
    if (checkfilename(roomBuf.rbroomtell, 0) == ERROR)
    {
        changedir(cfg.homepath);
        return;
    }

    if (!filexists(roomBuf.rbroomtell))
    {
        mPrintf("No room description %s", roomBuf.rbroomtell);
        doCR();
        doCR();
        changedir(cfg.homepath);
        return;
    }

    if (!expert) 
    {
        doCR();
        mPrintf(" <J>ump <N>ext <P>ause <S>top");
        doCR();
    }

    /* print it out */
    dumpf(roomBuf.rbroomtell);

    /* go to our home-path */
    changedir(cfg.homepath);

    outFlag = OUTOK;

    doCR();
}

/************************************************************************/
/*      roomExists() returns slot# of named room else ERROR             */
/************************************************************************/
int roomExists(const char *room)
{
    int i;

    for (i = 0;  i < MAXROOMS;  i++)
    {
        if (roomTab[i].rtflags.INUSE == 1   &&
            strcmpi(room, roomTab[i].rtname) == SAMESTRING )
        {
            return(i);
        }
    }
    return(ERROR);
}

/************************************************************************/
/*   roomtalley()  talleys up total,messages & new for every room       */
/************************************************************************/
void roomtalley(void)
{
    register int i;
    int          num;
    register int room;
    int          slot;

    for (room = 0; room < MAXROOMS; room++)
    {
        talleyBuf.room[room].total    = 0;
        talleyBuf.room[room].messages = 0;
        talleyBuf.room[room].new      = 0;
        talleyBuf.room[room].hasmail  = 0;
    }

    num = sizetable();

    for (i = 0, slot=0; i < num; ++i, slot++)
    {
        room = msgTab4[slot].mtroomno;

        if (msgTab3[slot].mtoffset <= (unsigned short)i)
            talleyBuf.room[room].total++;

        if (mayseeindexmsg(i))
        {
            talleyBuf.room[room].messages++;

            if  ((ulong)(cfg.mtoldest + i) >
                logBuf.lbvisit[ logBuf.lbroom[room].lvisit ])
            {
                talleyBuf.room[room].new++;

                /* check to see if its private mail and set flag if so */
                if (msgTab1[i].mtmsgflags.MAIL)
                    talleyBuf.room[room].hasmail = TRUE;

            }
        }
    }
}

/************************************************************************/
/*      givePrompt() prints the usual "CURRENTROOM>" prompt.            */
/************************************************************************/
void givePrompt(void)
{
    while (MIReady()) getMod();

    outFlag   = IMPERVIOUS;
    echo      = BOTH;
    /* onConsole = (char)(whichIO == CONSOLE);  */

    ansiattr = cfg.attr;

    doCR();

    termCap(TERM_REVERSE);
    printrm(thisRoom);
    termCap(TERM_NORMAL);
    mPrintf(" ");

    if (strcmp(roomBuf.rbname, roomTab[thisRoom].rtname) != SAMESTRING)
    {
        crashout("Dependent variables mismatch!");
    }
    outFlag = OUTOK;
}

/************************************************************************/
/*  indexRooms()      build RAM index to ROOM.CIT, for del empty rooms  */
/************************************************************************/
void indexRooms(void)
{
    int  goodRoom, slot, i;

    for (slot = 0;  slot < MAXROOMS;  slot++)
    {
        if (roomTab[slot].rtflags.INUSE)
        {
            goodRoom = FALSE;

            if (roomTab[slot].rtflags.PERMROOM ||
                roomTab[slot].rtflags.WINDOW   ||
                roomTab[slot].rtflags.SHARED   ||
                roomTab[slot].rtflags.APLIC)
            {
                goodRoom = TRUE;
            } else {
                for (i = 0; i < (int)sizetable(); ++i)
                {
                    if (msgTab4[i].mtroomno == (uchar)slot)
                         goodRoom = TRUE;
                }
            }

            if (!goodRoom)
            {
                getRoom(slot);
                roomBuf.rbflags.INUSE     = FALSE;
                roomBuf.rbflags.PUBLIC    = FALSE;
                roomBuf.rbflags.MSDOSDIR  = FALSE;
                roomBuf.rbflags.PERMROOM  = FALSE;
                roomBuf.rbflags.GROUPONLY = FALSE;
                roomBuf.rbflags.READONLY  = FALSE;
                roomBuf.rbflags.SHARED    = FALSE;
                roomBuf.rbflags.MODERATED = FALSE;
                roomBuf.rbflags.DOWNONLY  = FALSE;
                roomBuf.rbflags.UPONLY    = FALSE;
                putRoom(slot);
                strcat(msgBuf->mbtext, roomBuf.rbname);
                strcat(msgBuf->mbtext, "> ");
                noteRoom();
            }
        }
    }
}

/************************************************************************/
/*  noteRoom()              enter room into RAM index array.            */
/************************************************************************/
void noteRoom(void)
{
    strcpy(roomTab[thisRoom].rtname     , roomBuf.rbname) ;
    roomTab[thisRoom].rtgen             = roomBuf.rbgen   ;
    roomTab[thisRoom].grpno             = roomBuf.rbgrpno ;
/*  roomTab[thisRoom].grpgen            = roomBuf.rbgrpgen;  */

    /* dont YOU like ansi C? */
    roomTab[thisRoom].rtflags           = roomBuf.rbflags;
}

/************************************************************************/
/*  stepRoom()              1 for next 0, 0 for previous room           */
/************************************************************************/
void stepRoom(int direction)
{
    int i;
    char done = 0;

    i = thisRoom;
    oldroom = thisRoom;

    if (loggedIn)
    {
        ug_lvisit  = logBuf.lbroom[thisRoom].lvisit;
        ug_new     = talleyBuf.room[thisRoom].new;
        ug_hasmail = talleyBuf.room[thisRoom].hasmail;

        logBuf.lbroom[thisRoom].lbgen    = roomBuf.rbgen;
        logBuf.lbroom[thisRoom].lvisit   = 0;
        /* logBuf.lbroom[thisRoom].mail     = 0; */

        /* zero new count in talleybuffer */
        talleyBuf.room[thisRoom].new  = 0;
        talleyBuf.room[thisRoom].hasmail = 0;
    }

    do
    {
        if (direction == 1) ++i;
             else           --i;

              if ( (direction == 1) && (i == MAXROOMS) ) i = 0; 
        else  if ( (direction == 0) && (i == -1      ) ) i = MAXROOMS - 1;

        if ( (canseeroom(i) || i == thisRoom)
        /* and is it K>nown                             */
        /* sysops can plus their way into hidden rooms! */
        && ( (roomTab[i].rtgen == logBuf.lbroom[i].lbgen)
        ||   logBuf.lbflags.SYSOP ))
        {
            mPrintf("%s", roomTab[i].rtname); 
            doCR();

            getRoom(i);
 
            dumpRoom();

            done = 1;
        }
    }                                              
    while ( done != 1 );
}

/************************************************************************/
/*  unGotoRoom()            Jump to previous room                       */
/************************************************************************/
void unGotoRoom(void)
{
    int i;

    i = oldroom;

    if ( canseeroom(i) && i != thisRoom) 
    {
        mPrintf("%s", roomTab[i].rtname); doCR();

        getRoom(i);

        logBuf.lbroom[thisRoom].lvisit   = ug_lvisit;
        talleyBuf.room[thisRoom].new     = ug_new;
        talleyBuf.room[thisRoom].hasmail = (char)ug_hasmail;

        dumpRoom();
    }                                              
}
