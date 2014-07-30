/************************************************************************/
/* CTDL.C                         ACit                          91Sep30 */
/*              Command-interpreter code for Citadel                    */
/************************************************************************/

#define MAIN

#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dos.h>
#include "ctdl.h"
#include "key.h"
#include "prot.h"
#include "glob.h"

   int _far _nullcheck(void);  /* Undocumented function for MSC 6.0 */
static void doComment(void);
/* unsigned _stklen = 1024*12; */         /* set up our stackspace */

/************************************************************************/
/*                              Contents                                */
/*                                                                      */
/* $doAide()                handles Aide-only       commands            */
/* $doChat()                handles C(hat)          command             */
/* $doDownload()            handles D(ownload)      command             */
/* $doUpload()              handles U(pload)        command             */
/* $doEnter()               handles E(nter)         command             */
/* $exclude()               handles X(clude)        command             */
/* $doGoto()                handles G(oto)          command             */
/* $doHelp()                handles H(elp)          command             */
/* $doIntro()               handles I(ntro)         command             */
/* $doKnown()               handles K(nown rooms)   command             */
/* $doLogin()               handles L(ogin)         command             */
/* $doLogout()              handles T(erminate)     command             */
/* $doRead()                handles R(ead)          command             */
/* $doXpert()               handles .X(pert)        command             */
/* $doRegular()             fanout for above commands                   */
/* $doSysop()               handles sysop-only      commands            */
/* $doNext()                handles '+' next room                       */
/* $doPrevious()            handles '-' previous room                   */
/* $doNextHall()            handles '>' next room                       */
/* $doPreviousHall()        handles '<' previous room                   */
/* $doComment()             handles ';' comment                         */
/* $getCommand()            prints prompt and gets command char         */
/* $greeting()              System-entry blurb etc                      */
/*  main()                  has the central menu code                   */
/*  commandchar()           gets a character for menu level items       */
/************************************************************************/

static void doAide(char moreYet, char first);
static void doChat(char moreYet, char first);
static void doDownload(char ex);
static void doUpload(char ex);
static void doEnter(char moreYet, char first);
static void exclude(void);
static void doGoto(char expand, int skip);
static void doHelp(char expand);
static void doIntro(void);
static void doKnown(char moreYet, char first);
static void doLogin(char moreYet);
static void doLogout(char expand, char first);
static void doRead(char moreYet, char first);
static void doXpert(void);
static char doRegular(char x, char c);
static char doSysop(void);
static void doNext(void);
static void doPrevious(void);
static void doNextHall(void);
static void doPreviousHall(void);
static void doComment(void);
static char getCommand(char *c);
static void greeting(void);

/************************************************************************/
/*      doAide() handles the aide-only menu                             */
/*          return FALSE to fall invisibly into default error msg       */
/************************************************************************/
static void doAide(char moreYet, char first)
{
    int  roomExists();
    char oldchat;
    char ich = '\0';

    if (moreYet)   first = '\0';

    mPrintf("Aide special fn: ");

    /* if (first)     oChar(first);  */

    switch (toupper( first ? (char)first : (char)(ich = commandchar()) ))
    {
    case 'A':
        mPrintf("Attributes ");
        if (roomBuf.rbflags.MSDOSDIR != 1)
        {
            if (expert) mPrintf("? ");
            else        mPrintf("\n Not a directory room.");
        }
        else attributes();
        break;

    case 'C':
        chatReq = TRUE;
        oldchat = (char)cfg.noChat;
        cfg.noChat = FALSE;
        mPrintf("Chat\n ");
        if (whichIO == MODEM)   ringSysop();
        else                    chat() ;
        cfg.noChat = oldchat;
        break;

    case 'E':
        mPrintf("Edit room\n  \n");
        renameRoom();
        break;

    case 'F':
        mPrintf("File set\n  \n");
        batchinfo(TRUE);
        break;
    
    case 'G':
        mPrintf("Group membership\n  \n");
        groupfunc();
        break;

    case 'H':
        mPrintf("Hallway changes\n  \n");
        if (!cfg.aidehall && !sysop)
        {
            mPrintf(" Must be a Sysop!\n");
        }
        else
        {
            hallfunc();
        }
        break;

    case 'I':
        mPrintf("Insert %s\n ", cfg.msg_nym);
        insert();
        break;

    case 'K':
        mPrintf("Kill room\n ");
        killroom();
        break;

    case 'L':
        mPrintf("List group ");
        listgroup();
        break;

    case 'M':
        mPrintf("Move file ");
        moveFile();
        break;

    case 'N':
        mPrintf("Name Messages");
        msgNym();
        break;

    case 'R':
        mPrintf("Rename file ");
        if (roomBuf.rbflags.MSDOSDIR != 1)
        {
            if (expert) mPrintf("? ");
            else        mPrintf("\n Not a directory room.");
        }
        else
        {
            renamefile();
        }
        break;

    case 'S':
        mPrintf("Set file info\n ");
        if (roomBuf.rbflags.MSDOSDIR != 1)
        {
            if (expert) mPrintf("? ");
            else        mPrintf("\n Not a directory room.");
        }
        else
        {
            setfileinfo();
        }
        break;

    case 'U':
        mPrintf("Unlink file\n ");
        if (roomBuf.rbflags.MSDOSDIR != 1)
        {
            if (expert) mPrintf("? ");
            else        mPrintf("\n Not a directory room.");
        }
        else
        {
            unlinkfile();
        }
        break;

    case 'V':
        mPrintf("View Help Text File\n ");
        tutorial("aide.hlp");
        break;

    case 'W':
        mPrintf("Window into hallway\n ");
        if (!cfg.aidehall && !sysop)
        {
            mPrintf(" Must be a Sysop!\n");
        }
        else
        {
            windowfunc();
        }
        break;

    case '?':
        oChar('?');
        tutorial("aide.mnu");
        break;

    default:
        oChar(ich);
        if (!expert)   mPrintf("\n '?' for menu.\n " );
        else           mPrintf(" ?\n "               );
        break;
    }
}


/************************************************************************/
/*      doChat()                                                        */
/************************************************************************/
static void doChat(char moreYet, char first)
{
    static uchar whichNochat = 0;
    char noChatName[15];

    moreYet = moreYet;  /* -W3 */
    first   = first;    /* -W3 */
    
    chatReq = TRUE;
    
    mPrintf("Chat ");

    trap("Chat request", T_CHAT);

    if (cfg.noChat)
    {
        /* for filexists() to work properly */
        if (changedir(cfg.helppath) == -1 ) return;

        ++whichNochat;
        sprintf(noChatName, "nochat%d.blb", whichNochat);
        if (!filexists(noChatName))
        {
            sprintf(noChatName, "nochat.blb");
            whichNochat = 1;
        }
        tutorial(noChatName);
        changedir(cfg.homepath);
        return;
    }

    if (whichIO == MODEM)  ringSysop();
    else                   chat() ;
}


/************************************************************************/
/*      doDownload()                                                    */
/************************************************************************/
static void doDownload(char ex)
{
    ex = ex;

    mPrintf("Download ");

    if  (!loggedIn && !cfg.unlogReadOk) 
    {
        mPrintf("\n  --Must log in to download.\n ");
        return;
    }

    /* handle uponly flag! */
    if ( roomTab[thisRoom].rtflags.UPONLY && !groupseesroom(thisRoom) )
    {
        mPrintf("\n  --Room is upload-only.\n ");
        return;
    }

    if ( !roomBuf.rbflags.MSDOSDIR )
    {
        if (expert) mPrintf("? ");
        else        mPrintf("\n Not a directory room.");
        return;
    }
    download('\0');
}

/************************************************************************/
/*      doUpload()                                                      */
/************************************************************************/
static void doUpload(char ex)
{
    ex = ex;

    mPrintf("Upload ");

    /* handle downonly flag! */
    if ( roomTab[thisRoom].rtflags.DOWNONLY && !groupseesroom(thisRoom))
    {
        mPrintf("\n\n  --Room is download-only.\n ");
        return;
    }

    if ( !loggedIn && !cfg.unlogEnterOk )
    {
        mPrintf("\n\n  --Must log in to upload.\n ");
        return;
    }

    if ( !roomBuf.rbflags.MSDOSDIR )
    {
        if (expert) mPrintf("? ");
        else        mPrintf("\n Not a directory room.");
        return;
    }
    upload('\0');
    return;
}
/***********************************************************************/
/*     doEnter() handles E(nter) command                               */
/***********************************************************************/
static void doEnter(char moreYet, char first)
{
    char done;
    char letter;
    char ich = '\0';

    if (moreYet)  first = '\0';

    done      = TRUE ;
    mailFlag  = FALSE;
    oldFlag   = FALSE;
    limitFlag = FALSE;
    linkMess  = FALSE;

    mPrintf("Enter ");

    /* if (first)  oChar(first); */

    do  
    {
        outFlag = IMPERVIOUS;
        done    = TRUE;

  letter = (char)(toupper( first ? (char)first : (char)(ich = commandchar())));

        /* allow exclusive mail entry only */
        if ( !loggedIn && !cfg.unlogEnterOk && (letter != 'E') )
        {
            mPrintf("\n  --Must log in to enter.\n ");
            break;
        }

        /* handle readonly flag! */
        if ( roomTab[thisRoom].rtflags.READONLY && !groupseesroom(thisRoom)
        && ( (letter == '\r') || (letter == '\n') || (letter == 'M') 
        ||   (letter == 'E')  || (letter == 'O') || (letter == 'G')  ) )
        {
            mPrintf("\n\n  --Room is read-only.\n ");
            break;
        }

        /* handle steeve */
        if ( (unsigned char)MessageRoom[thisRoom] == cfg.MessageRoom && !(sysop || aide)
        && ( (letter == '\r') || (letter == '\n') || (letter == 'M') 
        ||   (letter == 'E')  || (letter == 'O') || (letter == 'G') ) )
        {
            mPrintf("\n\n  --Only %d %s per room.\n ",cfg.MessageRoom, 
                    cfg.msgs_nym);
            break;
        }

        /* handle nomail flag! */
        if ( logBuf.lbflags.NOMAIL && (letter == 'E'))
        {
            mPrintf("\n\n  --You can't enter mail.\n ");
            break;
        }

        /* handle downonly flag! */
        if ( roomTab[thisRoom].rtflags.DOWNONLY && !groupseesroom(thisRoom)
        && (  (letter == 'T') || (letter == 'W') ) ) 
        {
            mPrintf("\n\n  --Room is download-only.\n ");
            break;
        }

        if ( !sysop && (letter == '*'))
        {
            mPrintf("\n\n  --You can't enter a file-linked %s.\n ", 
                    cfg.msg_nym);
            break;
        }

        if ( !sysop && (letter == 'S') && !cfg.entersur )
        {
            mPrintf("\n\n  --Users can't enter their title and surname.\n ");
            break;
        }

        if ( !sysop && (letter == 'S') && logBuf.SURNAMLOK )
        {
            mPrintf("\n\n  --Your title and surname have been locked!\n ");
            break;
        }

        switch (letter)
        {
        case '\r':
        case '\n':
#if 1
            doCR();
#endif
            moreYet   =  FALSE;
            makeMessage();
            break;
        case 'B':
            mPrintf("Border Line");
            editBorder();
            break;
        case 'C':
            mPrintf("Configuration ");
            configure(FALSE);
            break;
        case 'D':
            mPrintf("Default-hallway ");
            doCR();
            defaulthall();
            break;
        case 'E':
            mPrintf("Exclusive %s ", cfg.msg_nym);
            doCR();
            if (whichIO != CONSOLE) echo = CALLER;
            limitFlag = FALSE; 
            mailFlag = TRUE;
            makeMessage();
            echo = BOTH;
            break;
        case 'F':
            mPrintf("Forwarding-address ");
            doCR();
            forwardaddr();
            break;
        case 'H':
            mPrintf("Hallway ");
            doCR();
            enterhall();
            break;
        case 'L':
            mPrintf("Limited-access ");
            done      = FALSE;
            limitFlag = TRUE;
            break;
        case '*':
            mPrintf("File-linked ");
            done      = FALSE;
            linkMess  = TRUE;
            break;
        case 'M':
            mPrintf("%s ", cfg.msg_nym);
            doCR();
            makeMessage();
            break;
        case 'G':
            mPrintf("Group-only %s ", cfg.msg_nym);
            doCR();
            limitFlag = TRUE;
            makeMessage();
            break;
        case 'O':
            mPrintf("Old %s ", cfg.msg_nym);
            done    = FALSE;
            oldFlag = TRUE;
            break;
        case 'P':
            mPrintf("Password ");
            doCR();
            newPW();
            break;
        case 'R':
            mPrintf("Room ");
            if (!cfg.nonAideRoomOk && !aide)
            {
                mPrintf("\n  --Must be aide to create room.\n ");
                 break;
            }
            if (!loggedIn)
            {
                 mPrintf("\n  --Must log in to create new room.\n ");
                 break;
            }
            doCR();
            makeRoom();
            break;
        case 'T':
            mPrintf("Textfile ");
            if (roomBuf.rbflags.MSDOSDIR != 1)
            {
                if (expert) mPrintf("? ");
                else        mPrintf("\n Not a directory room.");
                return;
            }
            if (!loggedIn)
            {
                mPrintf("\n  --Must be logged in.\n ");
                break;
            }
            entertextfile();
            break;
        case 'A':
            mPrintf("Application");
            if (!loggedIn)
            {
                mPrintf("\n  --Must be logged in.\n ");
            }
            else ExeAplic();
            break;
        case 'X':
            mPrintf("Exclude Room ");
            exclude();
            break;
        case 'S':
            if (cfg.surnames || cfg.titles)
            {
                label tempsur;

                mPrintf("Surname / Title"); doCR();
                
                if (cfg.titles)
                {
                    getString("title", tempsur, NAMESIZE, 0, ECHO, 
                              logBuf.title);
                    if (*tempsur)
                    {
                        strcpy(logBuf.title, tempsur);
                        normalizeString(logBuf.title);
                    }
                }
                
                if (cfg.surnames)
                {
                    getString("surname", tempsur, NAMESIZE, 0, ECHO, 
                              logBuf.surname);
                    if (*tempsur)
                    {
                        strcpy(logBuf.surname, tempsur);
                        normalizeString(logBuf.surname);
                    }
                }
                break;
            }
        default:
            oChar(ich);
            mPrintf("? ");
            if (expert)  break;
        case '?':
            if (letter == '?') oChar('?');
            tutorial("entopt.mnu");
            break;
        }
    }
    while (!done && moreYet);

    oldFlag   = FALSE;
    mailFlag  = FALSE;
    limitFlag = FALSE;

}

/************************************************************************/
/*      exclude() handles X>clude room,  toggles the bit                */
/************************************************************************/
static void exclude(void)
{
    if  (!logBuf.lbroom[thisRoom].xclude)
    {
         mPrintf("\n \n Room now excluded from G)oto loop.\n ");
         logBuf.lbroom[thisRoom].xclude = TRUE;  
    }else{
         mPrintf("\n \n Room now in G)oto loop.\n ");
         logBuf.lbroom[thisRoom].xclude = FALSE;
    }
}

/************************************************************************/
/*      doGoto() handles G(oto) command                                 */
/************************************************************************/
static void doGoto(char expand, int skip)
{
    label roomName;

    if (!skip)
    {
        mPrintf("Goto ");
        skiproom = FALSE;
    } 
    else 
    {
        mPrintf("Bypass to ");
        skiproom = TRUE;
    }

    if (!expand)
    {
        gotoRoom("");
        return;
    }

    getString("", roomName, NAMESIZE, 1, ECHO, "");
    normalizeString(roomName);

    if (roomName[0] == '?')
    {
        listRooms(OLDNEW, FALSE, FALSE);
    }
    else 
    {
        gotoRoom(roomName);
    }
}

/************************************************************************/
/*      doHelp() handles H(elp) command                                 */
/************************************************************************/
static void doHelp(char expand)
{
    label fileName;

    mPrintf("Help ");
    if (!expand)
    {
        mPrintf("\n\n");
        tutorial("dohelp.hlp");
        return;
    }

    getString("", fileName, 9, 1, ECHO, "");
    normalizeString(fileName);

    if (strlen(fileName) == 0)  strcpy(fileName, "dohelp");

    if (fileName[0] == '?')
    {
        tutorial("helpopt.hlp");
    } else {
        /* adding the extention makes things look simpler for           */
        /* the user... and restricts the files which can be read        */
        strcat(fileName, ".hlp");

        tutorial(fileName);
    }
}

/************************************************************************/
/*      doIntro() handles Intro to ....  command.                       */
/************************************************************************/
static void doIntro(void)
{
    mPrintf("Intro to %s\n ", cfg.nodeTitle);
    tutorial("intro.hlp");
}


/***********************************************************************/
/*      doKnown() handles K(nown rooms) command.                       */
/***********************************************************************/
static void doKnown(char moreYet, char first)
{
    char letter;
    char verbose = FALSE;
    char numMess = FALSE;
    char done;
    char ich = '\0';

    if (moreYet)  first = '\0';

    mPrintf("Known ");

    /* if (first)  oChar(first); */

    do  
    {
        outFlag = IMPERVIOUS;
        done    = TRUE;

  letter = (char)(toupper( first ? (char)first : (char)(ich = commandchar())));
        switch (letter)
        {
            case 'A':
                mPrintf("Application Rooms ");
                mPrintf("\n ");
                listRooms(APLRMS, verbose, numMess);
                break;
            case 'D':
                mPrintf("Directory Rooms ");
                mPrintf("\n ");
                listRooms(DIRRMS, verbose, numMess);
                break;
            case 'H':
                mPrintf("Hallways ");
                knownhalls();
                break;
            case 'L':
                mPrintf("Limited Access Rooms ");
                mPrintf("\n ");
                listRooms(LIMRMS, verbose, numMess);
                break;
            case 'N':
                mPrintf("New Rooms ");
                mPrintf("\n ");
                listRooms(NEWRMS, verbose, numMess);
                break;
            case 'O':
                mPrintf("Old Rooms ");
                mPrintf("\n ");
                listRooms(OLDRMS, verbose, numMess);
                break;
            case 'M':
                mPrintf("Mail Rooms ");
                mPrintf("\n ");
                listRooms(MAILRM, verbose, numMess);
                break;
            case 'S':
                mPrintf("Shared Rooms ");
                mPrintf("\n ");
                listRooms(SHRDRM, verbose, numMess);
                break;
            case 'I':
                mPrintf("Room Info");
                mPrintf("\n ");
                RoomStatus();
                break;
            case '\r':
            case '\n':
                listRooms(OLDNEW, verbose, numMess);
                break;
            case 'R':
                mPrintf("Rooms ");
                mPrintf("\n ");
                listRooms(OLDNEW, verbose, numMess);
                break;
            case 'V':
                mPrintf("Verbose ");
                done    = FALSE;
                verbose = TRUE;
                break;
            case 'W':
                mPrintf("Windows ");
                mPrintf("\n ");
                listRooms(WINDWS, verbose, numMess);
                break;
            case 'X':
                mPrintf("Xcluded Rooms ");
                mPrintf("\n ");
                listRooms(XCLRMS, verbose, numMess);
                break;
            case '#':
                mPrintf("# of %s ", cfg.msgs_nym);
                done    = FALSE;
                numMess = TRUE;
                break;
            default:
                oChar(ich);
                mPrintf("? ");
                if (expert)  break;
            case '?':
                if (letter == '?') oChar('?');
                tutorial("known.mnu");
                break;
        }
    }
    while (!done && moreYet);
}

/************************************************************************/
/*      doLogin() handles L(ogin) command                               */
/************************************************************************/
static void doLogin(char moreYet)
{
    char InitPw[80];
    char passWord[80];
    char Initials[80];
    char *semicolon;

    if (justLostCarrier)  return;

    if (moreYet == 2)
        moreYet = FALSE;
    else
        mPrintf("Login ");

    /* we want to be in console mode when we log in from local */
    if (!gotCarrier() && !loggedIn) 
    {
        whichIO = CONSOLE;
        /* onConsole = (char)(whichIO == CONSOLE); */
        update25();
        if (cfg.offhook)  offhook();
    }

    if (loggedIn)  
    {
        mPrintf("\n Already logged in!\n ");
        return;
    }

    getNormStr( (moreYet) ? "" : "your initials", InitPw, 40, NO_ECHO);
    dospCR();

    semicolon = strchr(InitPw, ';');

    if (!semicolon)
    {
        strcpy(Initials, InitPw);
        getNormStr( "password",  passWord, NAMESIZE, NO_ECHO);
        dospCR();
    }     
    else  normalizepw(InitPw, Initials, passWord);

    /* dont allow anything over 19 characters */
    Initials[19] = '\0';

    login(Initials, passWord);
}

/************************************************************************/
/*      doLogout() handles T(erminate) command                          */
/************************************************************************/
static void doLogout(char expand, char first)
{
    char done = FALSE, verbose = FALSE;
    char ich = '\0';

    if (expand)   first = '\0';

    mPrintf("Terminate ");

    /* if (first)   oChar(first); */

    if (first == 'q')
        verbose = 1;
    
    while(!done)
    {
        done = TRUE;

       switch (toupper( first ? (int)first : (int)(ich = commandchar())))
        {
        case '?':
            oChar('?');
            mPrintf("\n Logout options:\n \n ");
    
            mPrintf("Q>uit-also\n " );
            mPrintf("S>tay\n "      );
            mPrintf("V>erbose\n "   );
            mPrintf("? -- this list\n "  );
            break;
        case 'Y':
        case 'Q':
            mPrintf("Quit-also\n ");
            if (!expand)  
            {
                if (!getYesNo(confirm, 0))   break;
            }
            if (!(haveCarrier || (whichIO == CONSOLE))) break;
            terminate( /* hangUp == */ TRUE, verbose);
            break;
        case 'S':
            mPrintf("Stay\n ");
            terminate( /* hangUp == */ FALSE, verbose);
            break;
        case 'V':
            mPrintf("Verbose ");
            verbose = 2;
            done = FALSE;
            break;
        default:
            oChar(ich);
            if (expert)
                mPrintf("? ");
            else
                mPrintf("? for help");
            break;
        }
        first = '\0';
    }
}

/************************************************************************/
/*      doRead() handles R(ead) command                                 */
/************************************************************************/
static void doRead(char moreYet, char first)
{
    char abort, done, letter;
    char whichMess, revOrder, verbose;
    char ich = '\0';

    if (moreYet)   first = '\0';

    mPrintf("Read ");

    abort      = FALSE;
    revOrder   = FALSE;
    verbose    = FALSE;
    whichMess  = NEWoNLY;
    mf.mfPub   = FALSE;
    mf.mfMai   = FALSE;
    mf.mfLim   = FALSE;
    mf.mfUser[0] = FALSE;
    mf.mfGroup[0] = FALSE;

    if  (!loggedIn && !cfg.unlogReadOk) 
    {
        mPrintf("\n  --Must log in to read.\n ");
        return;
    }

    /* if (first)  oChar(first); */

    do
    {
        outFlag = IMPERVIOUS;
        done    = TRUE;

     letter = (char)(toupper(first ? (int)first : (int)(ich = commandchar())));

        /* handle uponly flag! */
        if ( roomTab[thisRoom].rtflags.UPONLY && !groupseesroom(thisRoom)
        && (  (letter == 'T') || (letter == 'W') ) ) 
        {
            mPrintf("\n\n  --Room is upload-only.\n ");
            break;
        }

        switch (letter)
        {
        case '\n':
        case '\r':
            moreYet = FALSE;
            break;
        case 'B':
            mPrintf("By-User ");
            mf.mfUser[0] = TRUE;
            done         = FALSE;
            break;
        case 'C':
            mPrintf("Configuration ");
            showconfig(&logBuf);
            abort     = TRUE;
            break;
        case 'D':
            mPrintf("Directory ");
            if ( !roomBuf.rbflags.MSDOSDIR )
            {
                if (expert) mPrintf("? ");
                else        mPrintf("\n Not a directory room.");
            }
            else readdirectory(verbose, whichMess, revOrder);
            abort      = TRUE;
            break;
        case 'E':
            mPrintf("Exclusive ");
            mf.mfMai     = TRUE;
            done         = FALSE;
            break;
        case 'F':
            mPrintf("Forward ");
            revOrder     = FALSE;
            whichMess    = OLDaNDnEW;
            done         = FALSE;
            break;
        case 'H':
            mPrintf("Hallways ");
            readhalls();
            abort     = TRUE;
            break;
        case 'I':
            mPrintf("Info file ");
            if ( !roomBuf.rbflags.MSDOSDIR )
            {
                if (expert) mPrintf("? ");
                else        mPrintf("\n Not a directory room.");
            }
            else readinfofile(verbose, whichMess, revOrder);
            abort      = TRUE;
            break;
        case 'L':
            mPrintf("Limited-access ");
            mf.mfLim     = TRUE;
            done         = FALSE;
            break;
        case 'N':
            mPrintf("New ");
            whichMess  = NEWoNLY;
            done       = FALSE;
            break;
        case 'O':
            mPrintf("Old ");
            revOrder   = TRUE;
            whichMess  = OLDoNLY;
            done       = FALSE;
            break;
        case 'P':
            mPrintf("Public ");
            mf.mfPub     = TRUE;
            done         = FALSE;
            break;
        case 'R':
            mPrintf("Reverse ");
            revOrder   = TRUE;
            whichMess  = OLDaNDnEW;
            done       = FALSE;
            break;
        case 'S':
            mPrintf("Status\n ");
            systat(verbose);
            abort         = TRUE;
            break;
        case 'T':
            mPrintf("Textfile ");
            if ( !roomBuf.rbflags.MSDOSDIR )
            {
                if (expert) mPrintf("? ");
                else        mPrintf("\n Not a directory room.");
            }
            else readtextfile(verbose);
            abort         = TRUE;
            break;
        case 'U':
            mPrintf("Userlog ");
            Readlog(verbose, revOrder);
            abort         = TRUE;
            break;
        case 'V':
            mPrintf("Verbose ");
#if 1
            ++verbose;
#else
            verbose       = TRUE;
#endif
            done          = FALSE;
            break;
        case 'Z':
            mPrintf("ZIP-file ");
            if ( !roomBuf.rbflags.MSDOSDIR )
            {
                if (expert) mPrintf("? ");
                else        mPrintf("\n Not a directory room.");
            }else readzip(verbose);
            abort     = TRUE;
            break;
        default:
            oChar(ich);
            mPrintf("? ");
            abort    = TRUE;
            if(expert) break;
        case '?':
            if (letter == '?') oChar('?');
            tutorial("readopt.mnu");
            abort    = TRUE;
            break;
        }
        first = '\0';

    }
    while (!done && moreYet && !abort);

    if (abort) return;

    showMessages(whichMess, revOrder, verbose);
}


/************************************************************************/
/*      doXpert                                                         */
/************************************************************************/
static void doXpert(void)
{
    mPrintf("Xpert %s", (expert) ? "Off" : "On");
    doCR();
    expert = (char)(!expert);
}

/************************************************************************/
/*      doRegular()                                                     */
/************************************************************************/
static char doRegular(char x, char c)
{
    char toReturn;
    int i;

    toReturn = FALSE;

    switch (toupper(c))
    {

    case 'S':
        if (sysop && x)
        {
            mPrintf("\bSysop Menu");
            doCR();
            doSysop();
        } 
        else 
        {
            toReturn=TRUE;
        }
        break;

    case 'A':
        if (aide)
        {
            doAide(x, 'E');
        } else {
            oChar(c);
            toReturn=TRUE;
        }
        break;

    case 'C': doChat(  x, '\0');                    break;
    case 'D': doDownload( x )  ;                    break;
    case 'E': doEnter( x, 'm' );                    break;
    case 'F': doRead(  x, 'f' );                    break;
    case 'G': doGoto(  x, FALSE);                   break;
    case 'H': doHelp(  x      );                    break;
    case 'I': doIntro()        ;                    break;
    case 'J': mPrintf("Jump back to "); unGotoRoom(); break;
    case 'K': doKnown( x, 'r');                     break;
    case 'L': doLogin( x      );                    break;    
    case 'N': doRead(  x, 'n' );                    break;
    case 'O': doRead(  x, 'o' );                    break;
    case 'R': doRead(  x, 'r' );                    break;
    case 'B': doGoto(  x, TRUE);                    break;
    case 'T': doLogout(x, 'q' );                    break;
    case 'U': doUpload( x )    ;                    break;
    case 'X':
        if (!x)
        {
            doEnter( x, 'x' );
        }else{
            doXpert();
        }
        break;

    case '=':
    case '+': doNext()         ;                    break;
    case '-': doPrevious()     ;                    break;

    case ']':
    case '>': doNextHall()     ;                    break;
    case '[':
    case '<': doPreviousHall() ;                    break;
    case '~': 
        mPrintf("Ansi %s\n ", ansiOn ? "Off" : "On");
        ansiOn = (char)(!ansiOn);
        break;

#if 1
    case ';':
              doComment()      ;                   break;
#endif

    case '?': oChar('?'); tutorial("mainopt.mnu");              break;

    case 0:
        if (newCarrier)  
        {
            greeting();

            if (cfg.forcelogin)
            {
                doCR();
                /* doCR(); */
                i = 0;
                while (!loggedIn && gotCarrier())
                {
                  doLogin( 2      );
                  if (++i > 3) Initport();
                }
            } 
            newCarrier  = FALSE;
        }

#ifdef NETWORK
        if (logBuf.lbflags.NODE && loggedIn)
        {
#if 1            
            i = disabled;
            disabled = FALSE;
#endif
            dowhat = NETWORKING;
            net_slave();
            dowhat = DUNO;

            warned = FALSE;
            haveCarrier = FALSE;
            newCarrier = FALSE;
            justLostCarrier = FALSE;
            /* onConsole = FALSE; */
#if 1
            disabled  = (char) i;
#else
            disabled  = FALSE;
#endif
            
            callout    = FALSE;

            pause(200);
          
            Initport();

            cfg.callno++;
            terminate(FALSE, FALSE);

        }
#endif

        if (justLostCarrier)
        {
            justLostCarrier = FALSE;
            if (loggedIn) terminate(FALSE, FALSE);
        }
        break;  /* irrelevant value */

    default:
        oChar(c);
        toReturn=TRUE;
        break;
    }
    /* if they get unverified online */
    if (logBuf.VERIFIED)  terminate(FALSE, FALSE);
    
    update25();
    return toReturn;
}

/************************************************************************/
/*      doSysop() handles the sysop-only menu                           */
/*          return FALSE to fall invisibly into default error msg       */
/************************************************************************/
static char doSysop(void)
{
    char  oldIO;
    int   c;

    oldIO = whichIO;
    
    /* we want to be in console mode when we go into sysop menu */
    if (!gotCarrier() || !sysop)
    {
        whichIO = CONSOLE;
        /* onConsole = (char)(whichIO == CONSOLE); */
    }

    sysop = TRUE;

    update25();
 
    while (!ExitToMsdos)
    {
        amZap();
        
        outFlag = IMPERVIOUS;
        doCR();
        mPrintf("2Privileged function:0 ");
       
        dowhat = SYSOPMENU;
        c = commandchar();
        dowhat = DUNO;
        
        switch (toupper( c ))
        {
        case 'A':
           /* mPrintf("\b"); */
        case 0:
            mPrintf("Abort");
            doCR();
            /* restore old mode */
            whichIO = oldIO;
            sysop = (char)(loggedIn ? logBuf.lbflags.SYSOP : 0);
            /* onConsole = (char)(whichIO == CONSOLE); */
            update25();
            return FALSE;

        case 'C':
            mPrintf("Cron special: ");
            cron_commands();
            break;

        case 'D':
            mPrintf("Date change\n ");
            changeDate();
            break;

        case 'E':
            mPrintf("Enter EXTERNAL.CIT and GRPDATA.CIT files.\n ");
            readaccount();
            readprotocols();
            break;

        case 'F':
            doAide( 1, 'E');
            break;

        case 'G':
            mPrintf("Group special: ");
            do_SysopGroup();
            break;

        case 'H':
            mPrintf("Hallway special: ");
            do_SysopHall();
            break;

        case 'K':
            mPrintf("Kill account\n ");
            killuser();
            break;

        case 'L':
            mPrintf("Login enabled\n ");
            sysopNew = TRUE;
            break;

        case 'M':
            mPrintf("Mass delete\n ");
            massdelete();
            break;

        case 'N':
            mPrintf("NewUser Verification\n ");
            globalverify();
            break;

        case 'O':
            mPrintf("Off hook\n ");
            if (!(whichIO == CONSOLE)) break;
            offhook();
            break;    
        
        case 'R':
            mPrintf("Reset file info\n ");
            if (roomBuf.rbflags.MSDOSDIR != 1)
            {
                if (expert) mPrintf("? ");
                else        mPrintf("\n Not a directory room.");
            }
            else updateinfo();
            break;

        case 'S':
            mPrintf("Show user\n ");
            showuser();
            break;

        case 'U':
            mPrintf("Userlog edit\n ");
            userEdit();
            break;

        case 'V':
            mPrintf("View Help Text File\n ");
            tutorial("sysop.hlp");
            break;

        case 'X':
            mPrintf("Exit to MS-DOS\n ");
            if (!(whichIO == CONSOLE)) break;
            if (!getYesNo(confirm, 0))   break;
            ExitToMsdos = TRUE;
            return FALSE;

        case 'Z': 
            mPrintf("Zap empty rooms\n ");
            killempties(); 
            break; 

        case '!':        
            mPrintf("Shell escape");
            doCR();
            if (!(whichIO == CONSOLE)) break;
            shellescape(0);
            break;

        case '@':        
            mPrintf("Super shell escape");
            doCR();
            if (!(whichIO == CONSOLE)) break;
            shellescape(1);
            break;

        case '#':
            mPrintf("Read by %s number\n ", cfg.msg_nym);
            readbymsgno();
            break;

        case '*':
            mPrintf("Unlink file(s)\n ");
            if (roomBuf.rbflags.MSDOSDIR != 1)
            {
                if (expert) mPrintf("? ");
                else        mPrintf("\n Not a directory room.");
            }
            else sysopunlink();
            break;

        case '?':
            oChar('?');
            tutorial("sysop.mnu");
            break;

        default:
            oChar((char)c);
            if (!expert)  mPrintf("\n '?' for menu.\n "  );
            else          mPrintf(" ?\n "                );
            break;
        }
    }
    return FALSE;
}

/************************************************************************/
/*     doNext() handles the '+' for next room                           */
/************************************************************************/
static void doNext(void)
{
    mPrintf("Next Room: ");
    stepRoom(1);
}

/************************************************************************/
/*     doPrevious() handles the '-' for previous room                   */
/************************************************************************/
static void doPrevious(void)
{
    mPrintf("Previous Room: ");
    stepRoom(0);
}

/************************************************************************/
/*     doNextHall() handles the '>' for next hall                       */
/************************************************************************/
static void doNextHall(void)
{
    mPrintf("Next Hall: ");
    stephall(1);
}

/************************************************************************/
/*     doPreviousHall() handles the '<' for previous hall               */
/************************************************************************/
static void doPreviousHall(void)
{
    mPrintf("Previous Hall: ");
    stephall(0);
}

#if 1
/************************************************************************/
/*     doComment() handles the ';' for a comment                        */
/************************************************************************/
static void doComment(void)
{
    mPrintf(";");
    getString("", msgBuf->mbtext, MAXTEXT-2, FALSE, ECHO, "");
}   
#endif

/************************************************************************/
/*      getCommand() prints menu prompt and gets command char           */
/*      Returns: char via parameter and expand flag as value  --        */
/*               i.e., TRUE if parameters follow else FALSE.            */
/************************************************************************/
static char getCommand(char *c)
{
    char expand;

    outFlag = IMPERVIOUS;

    /* update user's balance */
    if( cfg.accounting && !logBuf.lbflags.NOACCOUNT )
        updatebalance();

#ifdef DRAGON
    dragonAct();    /* user abuse routine :-) */
#endif

    if (cfg.borders)
    {
        doBorder();
    }

    givePrompt();

    do{
         dowhat = MAINMENU;
         *c = commandchar();
         dowhat = DUNO;
    }while(toupper(*c) == 'P'); 

    expand  = (char)
              ( (*c == ' ') || (*c == '.') || (*c == ',') || (*c == '/') );

    if (expand)
    {
        oChar(*c);
        *c = commandchar();
    }

    if (justLostCarrier)
    {
        justLostCarrier = FALSE;
        if (loggedIn) terminate(FALSE, FALSE);
    }
    return expand;
}

/************************************************************************/
/*      greeting() gives system-entry blurb etc                         */
/************************************************************************/
static void greeting(void)
{
    int messages;
    char dtstr[80];

    if (loggedIn) terminate(FALSE, FALSE);
    echo  =  BOTH;

    setdefaultconfig();
    initroomgen();
    cleargroupgen();
    if (cfg.accounting) unlogthisAccount();

    pause(10);

    if (newCarrier)  hello();

    mPrintf("\n Welcome to %s, %s", cfg.nodeTitle, cfg.nodeRegion
                                        );
    mPrintf("\n Running %s v%s (Microsoft C)", softname, version);
    if (strlen(testsite))
    {
        mPrintf("\n %s", testsite);
    }
    doCR();
    doCR();

    sstrftime(dtstr, 79, cfg.vdatestamp, 0l);
    mPrintf(" %s", dtstr);

    if (!cfg.forcelogin)
    {
        mPrintf("\n H for Help");
        mPrintf("\n ? for Menu");
        mPrintf("\n L to Login");
    }

    getRoom(LOBBY);

    messages = talleyBuf.room[thisRoom].messages;

    doCR();

    mPrintf("  %d %s ", messages, 
        (messages == 1) ? cfg.msg_nym: cfg.msgs_nym);

    doCR();

    while (MIReady()) getMod();

}

/************************************************************************/
/*      main() contains the central menu code                           */
/************************************************************************/
int main(int argc, char *argv[])
{
    int i;
    char c, x = FALSE, floppy = FALSE;

    cfg.bios = 1;

    for(i = 1; i < argc; i++)
    {
        if (   argv[i][0] == '/'
            || argv[i][0] == '-')
        {
            switch(toupper((int)argv[i][1]))
            {
            case 'D':
                cfg.bios = 0;
                break;

            case 'C':
                x = TRUE;
                break;

            case 'F':
                floppy = TRUE;
                break;

            case 'N':
                if (toupper(argv[i][2]) == 'B')
                {
                    cfg.noBells = TRUE;
                }
                if (toupper(argv[i][2]) == 'C')
                {
                    cfg.noChat = TRUE;
                }
                break;

            default:
                printf("\nUnknown command-line switch '%s'.\n", argv[i]);
                exit(1);
            }
        }
    }

    cfg.attr = 7;   /* logo gets white letters */

    setscreen();

    if (floppy)
    {
        cPrintf("\nInsert data disk(s) now and press any key.\n\n");
        getch();
    }

    if (x == TRUE) unlink("etc.dat");

    logo();

    if (time(NULL) < 607415813L)
    {
        cPrintf("\n\nPlease set your time and date!\n");
        exit(1);
    }

    initCitadel();

    greeting();

    sysReq = FALSE;

    if (cfg.f6pass[0])
        ConLock = TRUE;
    update25();

    while (!ExitToMsdos) 
    {
        if (_nullcheck())
            cPrintf("\n");

        if (sysReq == TRUE && !loggedIn && !haveCarrier)
        {
            sysReq=FALSE;
            if (cfg.offhook)
            {
                offhook();
            }else{
                drop_dtr();
            }
            ringSystemREQ();
        }

        x       = getCommand(&c);

        outFlag = IMPERVIOUS;

        if (chatkey) chat();
        if (eventkey && !haveCarrier)
        { 
            do_cron(CRON_TIMEOUT);
            eventkey = FALSE;
        }

        if ((sysopkey)  ?  doSysop()  :  doRegular(x, c)) 
        {
            if (!expert)  mPrintf("\n '?' for menu, 'H' for help.\n \n" );
            else          mPrintf(" ?\n \n" );
        }
    }
    exitcitadel();

    return 0; /* never realy gets here */
}

/************************************************************************/
/*      commandchar() gets a character for menu level commands          */
/************************************************************************/
char commandchar(void)
{
    char ich;
    char oldecho;

    oldecho = echo;

    outFlag = OUTOK;
    echo    = NEITHER;
    setio(whichIO, echo, outFlag);

    ich = (char)iChar();

    outFlag = OUTOK;
    echo    = oldecho;
    setio(whichIO, echo, outFlag);

    return(ich);
}
