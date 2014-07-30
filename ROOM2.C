/************************************************************************/
/*  ROOM2.C                       ACit                         91Sep29  */
/*              room code for Citadel bulletin board system             */
/************************************************************************/
#include <direct.h>
#include <string.h>
#include "ctdl.h"
#include "prot.h"
#include "glob.h"

/************************************************************************/
/*                              Contents                                */
/*                                                                      */
/*  findRoom()              find a free room                            */
/*  formatSummary()         summarizes current room                     */
/*  killempties()       .SZ deletes empty rooms                         */
/*  killroom()          .AK aide fn: to kill current room               */
/*  makeRoom()          .ER make new room via user dialogue             */
/*  massdelete()            sysop fn: to kill all msgs from person      */
/*  readbymsgno()       .S# sysop fn: to read by message #              */
/*  directory_l()           returns whether directory is locked         */
/*  renameRoom()        .AE sysop special to rename rooms               */
/************************************************************************/

static int findRoom(void);
static int  directory_l(const char  *str);

/************************************************************************/
/*      findRoom() returns # of free room if possible, else ERROR       */
/************************************************************************/
static int findRoom(void)
{
    int roomRover;

    for (roomRover = 0;  roomRover < MAXROOMS;  roomRover++)
    {
        if (roomTab[roomRover].rtflags.INUSE == 0) return roomRover;
    }
    return ERROR;
}

/************************************************************************/
/*      formatSummary() formats a summary of the current room           */
/************************************************************************/
void formatSummary(char *buffer)  /* 515 maximum, currently */
{
    char line[150];

    sprintf(line, " Room %s", roomBuf.rbname);        /* 37 */

    strcpy(buffer, line);

    if (roomBuf.rbflags.GROUPONLY)
    {
        sprintf(line, ", owned by group %s",          /* 48 */
            grpBuf.group[ roomBuf.rbgrpno ].groupname);

        strcat(buffer, line);
    }

    if (!roomBuf.rbflags.PUBLIC)
        strcat(buffer, ", hidden");                   /* 8 */
    
    if (roomBuf.rbflags.ANON)
        strcat(buffer, ", Anonymous");                /* 11 */
    
    if (roomBuf.rbflags.BIO)
        strcat(buffer, ", BIO");                      /* 5 */

    if (roomBuf.rbflags.MODERATED)
        strcat(buffer, ", moderated");                /* 11 */

    if (roomBuf.rbflags.READONLY)
        strcat(buffer, ", read-only");                /* 11 */

    if (roomBuf.rbflags.DOWNONLY)
        strcat(buffer, ", download-only");            /* 15 */

    if (roomBuf.rbflags.SHARED)
        strcat(buffer, ", networked/shared");         /* 18 */

    if (roomBuf.rbflags.APLIC)
        strcat(buffer, ", application");              /* 13 */

    if (roomBuf.rbflags.PERMROOM)
        strcat(buffer, ", permanent room");           /* 16 */

    if (aide)
    {
        if (roomBuf.rbflags.MSDOSDIR)
        {
            sprintf(line, "\n Directory room:  path is %s",   /* 31+64 */
                    roomBuf.rbdirname);
            strcat(buffer, line);
        }
    }

    if (sysop && roomBuf.rbflags.APLIC)
    {                                                         /* 17+64 */
        sprintf(line, "\n Application is %s", roomBuf.rbaplic);
        strcat(buffer, line);
    }

    if (roomBuf.rbroomtell[0] && cfg.roomtell && sysop)
    {                                                         /* 13+29 */
        sprintf(line, "\n Room description file is %s", roomBuf.rbroomtell);
        strcat(buffer, line);
    }

    if (roomBuf.descript[0])
    {                                                         /* 22+80 */
        sprintf(line, "\n Room Info-line is: %s", roomBuf.descript);
        strcat(buffer, line);
    }
}

/************************************************************************/
/*      killempties() aide fn: to kill empty rooms                      */
/************************************************************************/
void killempties(void)
{
    label oldName;
    int  rm, roomExists();

    if (!getYesNo(confirm, 0))  return;

    sprintf(msgBuf->mbtext, "The following empty rooms deleted by %s: ",
                                                logBuf.lbname);
    trap(msgBuf->mbtext, T_AIDE);

    strcpy(oldName, roomBuf.rbname);
    indexRooms();

    if ((rm=roomExists(oldName)) != ERROR)  getRoom(rm);
    else                                    getRoom(LOBBY);

    aideMessage();
}


/************************************************************************/
/*      killroom() aide fn: to kill current room                        */
/************************************************************************/
void killroom(void)
{
    int i;

    if (thisRoom == LOBBY   || thisRoom == MAILROOM
    || thisRoom == AIDEROOM || thisRoom == DUMP    )
    {
        doCR();
        mPrintf(" Cannot kill %s>, %s>, %s), or %s>",
            roomTab[LOBBY   ].rtname,
            roomTab[MAILROOM].rtname,
            roomTab[AIDEROOM].rtname,
            roomTab[DUMP    ].rtname );
        return;
    }
    if (!getYesNo(confirm, 0))  return;

    for (i = 0;  i < (int)sizetable();  i++)
    {
        if (msgTab4[i].mtroomno == (uchar)thisRoom)
        {
            changeheader((ulong)(cfg.mtoldest + i),
                3 /* Dump */, 255 /* no change */);
        }
    }

    /* kill current room from every hall */
    for (i = 0; i < MAXHALLS; i++)
    {
        hallBuf->hall[i].hroomflags[thisRoom].inhall = FALSE;
        hallBuf->hall[i].hroomflags[thisRoom].window = FALSE;
    }
    putHall();  /* update hall buffer */

    sprintf( msgBuf->mbtext, "%s> killed by %s",
        roomBuf.rbname,  logBuf.lbname );

    trap(msgBuf->mbtext, T_AIDE);

    aideMessage();

    roomBuf.rbflags.INUSE = FALSE;
    putRoom(thisRoom);
    noteRoom();
    getRoom(LOBBY);
}


/************************************************************************/
/*      makeRoom() constructs a new room via dialogue with user.        */
/************************************************************************/
void makeRoom(void)
{
    label roomname;
    label oldName;
    label groupname;
    int groupslot, testslot;
    char line[80];
    int  i;

    logBuf.lbroom[thisRoom].lbgen  = roomBuf.rbgen;
    logBuf.lbroom[thisRoom].lvisit = 0; 

    memset(&roomBuf, 0, sizeof(roomBuf));

    /* zero new count in talleybuffer */
    talleyBuf.room[thisRoom].new  = 0;

    strcpy(oldName, roomBuf.rbname);

    thisRoom = findRoom();

    if ( (thisRoom) == ERROR )
    {
        indexRooms();   /* try to reclaim empty room */
        
        thisRoom = findRoom();
   
        if (thisRoom == ERROR)
        {
            mPrintf(" Room table full.");

            thisRoom = roomExists(oldName);

            /* room is missing, go back to Lobby>    */
            if ((thisRoom) == ERROR)  thisRoom = LOBBY;

            getRoom(thisRoom);   /* room is gone, go back to Lobby> */

            return;
        }
    }

    getNormStr("name for new room", roomname, NAMESIZE, ECHO);

    if (!strlen(roomname))
    {
        thisRoom = roomExists(oldName);

        /* room is missing, go back to Lobby>    */
        if ((thisRoom) == ERROR)  thisRoom = LOBBY;

        getRoom(thisRoom);   /* room is gone, go back to Lobby> */

        return;
    }

    testslot = roomExists(roomname);

    if (testslot != ERROR)
    {
        mPrintf(" A \"%s\" room already exists.\n", roomname);
 
        thisRoom = roomExists(oldName);

        /* room is missing, go back to Lobby>    */
        if ((thisRoom) == ERROR)  thisRoom = LOBBY;

        getRoom(thisRoom);   /* room is gone, go back to Lobby> */

        return;
    }

    if (limitFlag)
    {
        getString("group for new room", groupname, NAMESIZE, FALSE, ECHO, "");

        groupslot = partialgroup(groupname);
        
        if (!strlen(groupname) || (groupslot == ERROR)
        || !ingroup(groupslot) )
        {
            mPrintf("No such group.");

            thisRoom = roomExists(oldName);

            /* room is missing, go back to Lobby>    */
            if ((thisRoom) == ERROR)  thisRoom = LOBBY;

            getRoom(thisRoom);   /* room is gone, go back to Lobby> */

            return;
        }
        roomBuf.rbgrpno  = (unsigned char)groupslot;
     /* roomBuf.rbgrpgen = grpBuf.group[groupslot].groupgen; */

        roomBuf.rbflags.READONLY = getYesNo("Read-only", 0);
    }
    if (!expert)   tutorial("newroom.blb");


    roomBuf.rbflags.INUSE     = TRUE;
    roomBuf.rbflags.GROUPONLY = limitFlag;

    getNormStr("Description for new room", roomBuf.descript, 80, ECHO);

    roomBuf.rbflags.PUBLIC = getYesNo("Make room public", 1);

    sprintf(line, "Install \"%s\" as a %s room",
    roomname , (roomBuf.rbflags.PUBLIC) ? "public" : "private" , 0);

    if (!getYesNo( line , 0) )
    {
        thisRoom = roomExists(oldName);

        /* room is missing, go back to Lobby>    */
        if ((thisRoom) == ERROR)  thisRoom = LOBBY;

        getRoom(thisRoom);   /* room is gone, go back to Lobby> */

        return;
    }

    strcpy(roomBuf.rbname, roomname);

    for (i = 0;  i < (int)sizetable();  i++)
    {
        if (msgTab4[i].mtroomno == (uchar)thisRoom)
            changeheader(cfg.mtoldest + i,
            3   /* Dump      */,
            255 /* No change */ );
    }
    roomBuf.rbgen = (unsigned char)((roomTab[thisRoom].rtgen + 1) % MAXGEN);

    noteRoom();                         /* index new room       */
    if (strcmp(roomTab[thisRoom].rtname, roomBuf.rbname) != SAMESTRING)
    {
        cPrintf("Room names changed roomTab = \"%s\", roomBuf = \"%s\"\n",
                roomTab[thisRoom].rtname, roomBuf.rbname);
    }
    
    putRoom(thisRoom);

    /* remove room from all halls */
    for (i = 0; i < MAXHALLS; i++)
    {
        /* remove from halls */
        hallBuf->hall[i].hroomflags[thisRoom].inhall = FALSE;

        /* unwindow */
        hallBuf->hall[i].hroomflags[thisRoom].window = FALSE;
    }

    /* put room in current hall */
    hallBuf->hall[thisHall].hroomflags[thisRoom].inhall = TRUE;

    /* put room in maintenance hall */
    hallBuf->hall[1].hroomflags[thisRoom].inhall = TRUE;

    putHall();  /* save it */


    sprintf(msgBuf->mbtext, "%s%c created by %s", roomname, 
    (roomBuf.rbflags.PUBLIC) ? '>' : ')', logBuf.lbname);

 /*  sprintf(msgBuf->mbtext, "%s> created by %s", roomname, logBuf.lbname); */

    trap(msgBuf->mbtext, T_NEWROOM);

    aideMessage();

    logBuf.lbroom[thisRoom].lbgen  = roomBuf.rbgen;
    logBuf.lbroom[thisRoom].lvisit = 0; 
}


/************************************************************************/
/*     massdelete()  sysop fn: to kill all msgs from person             */
/************************************************************************/
void massdelete(void)
{
    label who;
    char string[80];
    int i, namehash, killed = 0;

    getNormStr("who", who, NAMESIZE, ECHO);

    if (!strlen(who)) return;

    sprintf(string, "Delete all %s from %s", cfg.msgs_nym, who);

    namehash = hash(who);
    
    if (getYesNo(string, 0))
    {
        for (i = 0; i < (int)sizetable(); ++i)
        {
            if (msgTab6[i].mtauthhash == namehash)
            {
                if (msgTab4[i].mtroomno != 3 /* DUMP */ )
                {
                    ++killed;

                    changeheader((ulong)(cfg.mtoldest + i),
                        3 /* Dump */, 255 /* no change */);
                }
            }
        }

        mPrintf("%d %s killed", killed, cfg.msgs_nym);

        doCR();

        if (killed)
        {
            sprintf(msgBuf->mbtext, "All messages from %s deleted", who);
            trap(msgBuf->mbtext, T_SYSOP);
        }
    }
}

/************************************************************************/
/*      readbymsgno()  sysop fn: to read by message #                   */
/************************************************************************/
void readbymsgno(void)
{
    ulong msgno;
    int slot;

    doCR();
    
    msgno = (ulong) getNumber("Message number to read",
        (long)cfg.mtoldest, (long)cfg.newest, -1l);

    slot = indexslot(msgno);

    if (!mayseeindexmsg(slot)) 
    {
        mPrintf("%s not viewable.", cfg.msg_nym);
    }
    else
    {
        printMessage(msgno, (char)1);
    }

    doCR();
}

/* ------------------------------------------------------------------------ */
/*  directory_l()   returns wether a directory is locked                    */
/* ------------------------------------------------------------------------ */
static int directory_l(const char *str)
{                          
    FILE *fBuf;
    char line[90];
    char *words[256];
    char path[80];

    sprintf(path, "%s\\external.cit", cfg.homepath);
    
    if ((fBuf = fopen(path, "r")) == NULL)  /* ASCII mode */
    {  
        crashout("Can't find EXTERNAL.CIT!");
    }

    while (fgets(line, 90, fBuf) != NULL)
    {
        if (line[0] != '#') continue;
   
        if (strnicmp(line, "#DIRE", 5) != SAMESTRING) continue;
     
        parse_it( words, line);

        if (strcmpi(words[0], "#DIRECTORY") == SAMESTRING)
        {
            if (u_match(str, words[1]))
            {
                fclose(fBuf);
                return TRUE;
            }
        }
    }
    fclose(fBuf);
    return FALSE;
}

/************************************************************************/
/*      renameRoom() is aide special fn to edit room                    */
/************************************************************************/
void renameRoom(void)
{ 
    char    pathname[64];
    char    summary[515];
    label   roomname;
    label   oldname;
    label   groupname;
    char    line[80];
    char    waspublic;
    int     groupslot;
    char    description[13];
    int     roomslot;
    BOOL    prtMess = TRUE;
    BOOL    quit    = FALSE;
    int     c;
    char    oldEcho;
   
    strcpy(oldname,roomBuf.rbname);
    if (!roomBuf.rbflags.MSDOSDIR)
    {
        roomBuf.rbdirname[0] = '\0';
    }

    doCR();

    do 
    {
        if (prtMess)
        {
            doCR();
            outFlag = OUTOK;
            mPrintf("<3N0> Name.............. %s", roomBuf.rbname);   doCR();
            mPrintf("<3I0> Infoline.......... %s", roomBuf.descript); doCR();
            mPrintf("<3D0> Directory......... %s",
                             roomBuf.rbflags.MSDOSDIR
                             ? roomBuf.rbdirname : "None");             doCR();
            if (roomBuf.rbflags.GROUPONLY && roomBuf.rbflags.MSDOSDIR)
            {
                mPrintf("    Download-only..... %s", 
                             roomBuf.rbflags.DOWNONLY ? "Yes" : "No" ); doCR();
                mPrintf("    Upload-only....... %s", 
                             roomBuf.rbflags.UPONLY ? "Yes" : "No" );   doCR();
            }
            mPrintf("<3L0> Application....... %s",
                             roomBuf.rbflags.APLIC
                             ? roomBuf.rbaplic   : "None");             doCR();
            mPrintf("<3F0> Description File.. %s", 
                             roomBuf.rbroomtell[0]
                             ? roomBuf.rbroomtell : "None");            doCR();
            
            mPrintf("<3G0> Group............. %s", 
                             roomBuf.rbflags.GROUPONLY
                             ? grpBuf.group[roomBuf.rbgrpno].groupname
                             : "None");                                 doCR();
            
            mPrintf("<3H0> Hidden............ %s", 
                             roomBuf.rbflags.PUBLIC ? "No" : "Yes" );   doCR();
            
            mPrintf("<3Y0> Anonymous......... %s", 
                             roomBuf.rbflags.ANON ? "Yes" : "No" );     doCR();
            
            mPrintf("<3O0> BIO............... %s", 
                             roomBuf.rbflags.BIO ? "Yes" : "No" );      doCR();
                                                
            mPrintf("<3M0> Moderated......... %s", 
                             roomBuf.rbflags.MODERATED ? "Yes" : "No" );doCR();
            
            mPrintf("<3E0> Networked/Shared.. %s", 
                             roomBuf.rbflags.SHARED ? "Yes" : "No" );   doCR();
            
            mPrintf("<3P0> Permanent......... %s", 
                             roomBuf.rbflags.PERMROOM ? "Yes" : "No" ); doCR();
            
            if (roomBuf.rbflags.GROUPONLY)
            {
                mPrintf("<3R0> Read Only......... %s", 
                             roomBuf.rbflags.READONLY ? "Yes" : "No" ); doCR();
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
        case 'A':
            mPrintf("Abort");  doCR();
            if (getYesNo("Abort", TRUE))
            {
                getRoom(thisRoom);
                return;
            }
            break;
    
        case 'D':
            mPrintf("Directory"); doCR();

            if (sysop)
            {
                if (getYesNo("Directory room", (uchar)roomBuf.rbflags.MSDOSDIR))
                {
                    roomBuf.rbflags.MSDOSDIR = TRUE;

                    if (!roomBuf.rbdirname[0])
                        mPrintf(" No drive and path");
                    else
                        mPrintf(" Now space %s",roomBuf.rbdirname);

                    doCR();
                    strcpy(pathname, cfg.dirpath);
                    getString("Path", pathname, 63, FALSE, ECHO,
                        (roomBuf.rbdirname[0]) ? roomBuf.rbdirname
                                               : pathname);
                    pathname[0] = (char)toupper(pathname[0]);

                    doCR();
                    mPrintf("Checking pathname \"%s\"", pathname);
                    doCR();
                    
                    if (directory_l(pathname) && !(whichIO == CONSOLE))
                    {
                        logBuf.VERIFIED = TRUE;

                        sprintf(msgBuf->mbtext, 
                                "Security violation on directory %s by %s\n "
                                "User unverified.", pathname, logBuf.lbname);
                        aideMessage();

                        doCR();
                        mPrintf("Security violation, your account is being "
                                "held for sysop's review"); 
                        doCR();
                        Initport();

                        getRoom(thisRoom);
                        return;
                    }

                    if (changedir(pathname) != -1)
                    {
                        mPrintf(" Now space %s", pathname);
                        doCR();
                        strcpy(roomBuf.rbdirname, pathname);
                    }
                    else
                    {
                        mPrintf("%s does not exist! ", pathname);
                        if (getYesNo("Create", 0))
                        {
                            if (mkdir(pathname) == -1)
                            {
                                mPrintf("mkdir() ERROR!");
                                strcpy(roomBuf.rbdirname, cfg.temppath);
                            }
                            else
                            {
                                strcpy(roomBuf.rbdirname, pathname);
                                mPrintf(" Now space %s",roomBuf.rbdirname);
                                doCR();
                            }
                        }
                        else
                        {
                            strcpy(roomBuf.rbdirname, cfg.temppath);
                        }
                    }

                    if (roomBuf.rbflags.GROUPONLY && roomBuf.rbflags.MSDOSDIR)
                    {
                        roomBuf.rbflags.DOWNONLY =
                            getYesNo("Download-only", 
                                    (uchar)roomBuf.rbflags.DOWNONLY);

                        if (!roomBuf.rbflags.DOWNONLY)
                        {
                            roomBuf.rbflags.UPONLY   =  getYesNo("Upload-only", 
                                                 (uchar)roomBuf.rbflags.UPONLY);
                        }
                    }
                }
                else
                {
                    roomBuf.rbflags.MSDOSDIR = 0;
                    roomBuf.rbflags.DOWNONLY = 0;
                }
                changedir(cfg.homepath);
            }
            else
            {
                doCR();
                mPrintf("Must be Sysop to make directories.");
                doCR();
            }
            break;
    
        case 'E':
            mPrintf("Networked/Shared"); doCR();
            
            if (sysop)
            {
                roomBuf.rbflags.SHARED = getYesNo("Networked/Shared room",
                                         (uchar)roomBuf.rbflags.SHARED);
            }
            else
            {
                doCR();
                mPrintf("Must be Sysop to make Shared rooms.");
                doCR();
            }
            break;
    
        case 'F':
            mPrintf("Description File"); doCR();

            if (cfg.roomtell && sysop)
            {
                if ( getYesNo("Display room description File",
                        (uchar)(roomBuf.rbroomtell[0] != '\0') ) )
                {
                    getString("Description Filename", description, 13, FALSE,
                    ECHO, (roomBuf.rbroomtell[0]) ? roomBuf.rbroomtell : "");
                    strcpy(roomBuf.rbroomtell, description);
                }
                else roomBuf.rbroomtell[0] = '\0';
            }
            else
            {
                doCR();
                mPrintf("Must be Sysop and have Room descriptions configured.");
                doCR();
            }
            break;
    
        case 'G':
            mPrintf("Group"); doCR();
            
            if ((thisRoom > 2) || (thisRoom > 0 && sysop))
            {
                if (getYesNo("Change Group", 0))
                {
                    getString("Group for room <CR> for no group",
                                    groupname, NAMESIZE, FALSE, ECHO, "");

                    roomBuf.rbflags.GROUPONLY = 1;

                    groupslot = partialgroup(groupname);

                    if (!strlen(groupname) || (groupslot == ERROR) )
                    {
                        roomBuf.rbflags.GROUPONLY = 0;
                        roomBuf.rbflags.READONLY  = 0;
                        roomBuf.rbflags.DOWNONLY  = 0;

                        if (groupslot == ERROR && strlen(groupname))
                            mPrintf("No such group.");
                    }

                    if (roomBuf.rbflags.GROUPONLY )
                    {
                        roomBuf.rbgrpno  = (unsigned char)groupslot;
                     /* roomBuf.rbgrpgen = grpBuf.group[groupslot].groupgen; */
                    }
                }
            }
            else
            {
                if(thisRoom > 0)
                {
                    doCR();
                    mPrintf("Must be Sysop to change group for Mail> or Aide)");
                    doCR();
                }
                else
                {
                    doCR();
                    mPrintf("Lobby> can never be group-only");
                    doCR();
                }
            }
            break;
    
        case 'H':
            mPrintf("Hidden Room"); doCR();
            
            if ((thisRoom > 2) || (thisRoom>0 && sysop))
            {
                waspublic = (uchar)roomBuf.rbflags.PUBLIC;

                roomBuf.rbflags.PUBLIC =
                    !getYesNo("Hidden room", (uchar)(!roomBuf.rbflags.PUBLIC));

                if (waspublic && (!roomBuf.rbflags.PUBLIC))
                {
                  roomBuf.rbgen = (unsigned char)((roomBuf.rbgen +1) % MAXGEN);
                    logBuf.lbroom[thisRoom].lbgen = roomBuf.rbgen;
                }
            }
            else
            {
                doCR();
                mPrintf("Must be Sysop to make Lobby>, Mail> or Aide) hidden.");
                doCR();
            }
            break;
    
        case 'I':
            mPrintf("Info-line \n ");
            getNormStr("New room Info-line", roomBuf.descript, 79, ECHO);
            break;
    
        case 'L':
            mPrintf("Application"); doCR();
            
            if (sysop && (whichIO == CONSOLE))
            {
                if ( getYesNo("Application", (uchar)(roomBuf.rbflags.APLIC) ) )
                {
                    getString("Application filename", description, 13, FALSE,
                            ECHO, (roomBuf.rbaplic[0]) ? roomBuf.rbaplic : "");

                    strcpy(roomBuf.rbaplic, description);

                    roomBuf.rbflags.APLIC = TRUE;
                }
                else
                {
                    roomBuf.rbaplic[0] = '\0';
                    roomBuf.rbflags.APLIC = FALSE;
                }
            }
            else
            {
                mPrintf("Must be Sysop at console to enter application.");
                doCR();
            }
            break;
   
        case 'M':
            mPrintf("Moderated"); doCR();
            
            if (sysop)
            {
                if (getYesNo("Moderated", (uchar)(roomBuf.rbflags.MODERATED) ))
                    roomBuf.rbflags.MODERATED = TRUE;
                else
                    roomBuf.rbflags.MODERATED = FALSE;
            }
            else
            {
                doCR();
                mPrintf("Must be Sysop to make Moderated rooms.");
                doCR();
            }
            break;
    
        case 'N':
            mPrintf("Name"); doCR();
            
            getNormStr("New room name", roomname, NAMESIZE, ECHO);
            roomslot = roomExists(roomname);
            if (roomslot >= 0  &&  roomslot != thisRoom)
            {
                mPrintf("A \"%s\" room already exists.\n", roomname);
            } else {
                strcpy(roomBuf.rbname, roomname); /* also in room itself */
            }
            break;
    
        case 'O':
            mPrintf("BIO Room"); doCR();
            
            if ((thisRoom > 2) || (thisRoom>0 && sysop))
            {
                roomBuf.rbflags.BIO =
                    getYesNo("BIO room", (uchar)(roomBuf.rbflags.BIO));
            }
            else
            {
                doCR();
                mPrintf("Must be Sysop to make Lobby>, Mail> or Aide) BIO.");
                doCR();
            }
            break;
            
        case 'P':
            mPrintf("Permanent");
            doCR();
            if (thisRoom > DUMP)
            {
                if (!roomBuf.rbflags.MSDOSDIR)
                {
                    roomBuf.rbflags.PERMROOM =
                        getYesNo("Permanent", (uchar)roomBuf.rbflags.PERMROOM);
                }
                else
                {
                    roomBuf.rbflags.PERMROOM = 1;
                    doCR();
                    mPrintf("Directory rooms are automatically Permanent.");
                    doCR();
                }
            }
            else
            {
                doCR();
                mPrintf("Lobby> Mail> Aide) and Dump> always Permanent.");
                doCR();
            }
            break;
    
        case 'R':
            mPrintf("Read Only");
            doCR();
            if (roomBuf.rbflags.GROUPONLY )
            {
                roomBuf.rbflags.READONLY =
                    getYesNo("Read-only", (uchar)roomBuf.rbflags.READONLY);
            }
            else
            {
                doCR();
                mPrintf("Must be a group-only room to be Read-only.");
                doCR();
            }
            break;
   
        case 'S':
            mPrintf("Save");  doCR();
            if (getYesNo("Save changes", FALSE))
            {
                noteRoom();
                putRoom(thisRoom);

                /* trap file line */
                sprintf(line, "Room \'%s\' changed to \'%s\' by %s",
                                oldname, roomBuf.rbname, logBuf.lbname);
                trap(line, T_AIDE);

                /* Aide room */
                formatSummary(summary);
                sprintf(msgBuf->mbtext, "%s \n%s", line, summary);
                aideMessage();

                return;
            }
            break;
        
        case 'Y':
            mPrintf("Anonymous Room"); doCR();
            
            if ((thisRoom > 2) || (thisRoom>0 && sysop))
            {
                roomBuf.rbflags.ANON =
                     getYesNo("Anonymous room", (uchar)(roomBuf.rbflags.ANON));
            }
            else
            {
                doCR();
                mPrintf("Must be Sysop to make Lobby>, Mail> or Aide) Anonymous.");
                doCR();
            }
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
