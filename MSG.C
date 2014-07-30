/* -------------------------------------------------------------------- */
/*  MSG.C                         ACit                         91Sep27  */
/*               This is the high level message code.                   */
/* -------------------------------------------------------------------- */

#include <string.h>
#include <time.h>
#include "ctdl.h"
#include "key.h"
#include "prot.h"
#include "glob.h"

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/*  aideMessage()   save auto message in Aide>                          */
/*  specialMessage()    saves a special message in curent room          */
/* $buildroom()     builds a new room according to msg-buf              */
/*  clearmsgbuf()   this clears the message buffer out                  */
/*  getMessage()    reads a message off disk into RAM.                  */
/*  mAbort()        returns TRUE if the user has aborted typeout        */
/*  putMessage()    stores a message to disk                            */
/*  noteMessage()   puts message in mesgBuf into message index          */
/*  indexmessage()  builds one message index from msgBuf                */
/* -------------------------------------------------------------------- */

static void buildroom(void);

/* -------------------------------------------------------------------- */
/*  aideMessage()   save auto message in Aide>                          */
/* -------------------------------------------------------------------- */
void aideMessage(void)
{
    /* clear out message buffer */
    clearmsgbuf();

    msgBuf->mbroomno = AIDEROOM;

    strcpy(msgBuf->mbauth,  cfg.nodeTitle);

    putMessage();

    noteMessage();

    if (!logBuf.lbroom[AIDEROOM].lvisit)
        talleyBuf.room[AIDEROOM].new--;
}

/* -------------------------------------------------------------------- */
/*  specialMessage()    saves a special message in curent room          */
/* -------------------------------------------------------------------- */
void specialMessage(void)
{
    /* clear out message buffer */
    clearmsgbuf();

    msgBuf->mbroomno = (uchar)thisRoom;
    strcpy(msgBuf->mbauth,  cfg.nodeTitle);

    putMessage();

    noteMessage();

    if (!logBuf.lbroom[thisRoom].lvisit)
        talleyBuf.room[thisRoom].new--;
}

/* -------------------------------------------------------------------- */
/*      buildroom()  builds a new room according to msg-buf             */
/* -------------------------------------------------------------------- */
static void buildroom(void)
{
    int roomslot;

    if (*msgBuf->mbcopy) return;
    roomslot = msgBuf->mbroomno;

    if (msgBuf->mbroomno < MAXROOMS)
    {
        getRoom(roomslot);

        if ((strcmp(roomBuf.rbname, msgBuf->mbroom) != SAMESTRING)
        || (!roomBuf.rbflags.INUSE))
        {
            if (msgBuf->mbroomno > 3)
            {
                roomBuf.rbflags.INUSE     = TRUE;
                roomBuf.rbflags.PERMROOM  = FALSE;
                roomBuf.rbflags.MSDOSDIR  = FALSE;
                roomBuf.rbflags.GROUPONLY = FALSE;
                roomBuf.rbroomtell[0]     = '\0';
                roomBuf.rbflags.PUBLIC    = TRUE;
            }
            strcpy(roomBuf.rbname, msgBuf->mbroom);

            putRoom(thisRoom);
        }
    }
}

/* -------------------------------------------------------------------- */
/*  clearmsgbuf()   this clears the message buffer out                  */
/* -------------------------------------------------------------------- */
void clearmsgbuf(void)
{
    /* clear msgBuf out */
    msgBuf->mbroomno    =   0 ;
    msgBuf->mbattr      =   0 ;
    msgBuf->mbauth[ 0]  = '\0';
    msgBuf->mbtitle[0]  = '\0';
    msgBuf->mbocont[0]  = '\0';
    msgBuf->mbfpath[0]  = '\0';
    msgBuf->mbtpath[0]  = '\0';
    msgBuf->mbczip[ 0]  = '\0';
    msgBuf->mbcopy[ 0]  = '\0';
    msgBuf->mbfwd[  0]  = '\0';
    msgBuf->mbgroup[0]  = '\0';
    msgBuf->mbtime[ 0]  = '\0';
    msgBuf->mbId[   0]  = '\0';
    msgBuf->mbsrcId[0]  = '\0';
    msgBuf->mboname[0]  = '\0';
    msgBuf->mboreg[ 0]  = '\0';
    msgBuf->mbreply[0]  = '\0';
    msgBuf->mbroom[ 0]  = '\0';
    msgBuf->mbto[   0]  = '\0';
    msgBuf->mbsur[  0]  = '\0';
    msgBuf->mblink[ 0]  = '\0';
    msgBuf->mbx[    0]  = '\0';
    msgBuf->mbzip[  0]  = '\0';
    msgBuf->mbrzip[ 0]  = '\0';
#if 1
    msgBuf->mbcreg[ 0]  = '\0';
    msgBuf->mbccont[0]  = '\0';
    msgBuf->mbsig[  0]  = '\0';
    msgBuf->mbsubj[ 0]  = '\0';
    msgBuf->mbsw[   0]  = '\0';
    msgBuf->mbusig[ 0]  = '\0';
#endif
}

/* -------------------------------------------------------------------- */
/*  getMessage()    reads a message off disk into RAM.                  */
/* -------------------------------------------------------------------- */
void getMessage(void)
{
    char c;

    /* clear message buffer out */
    clearmsgbuf();

    /* find start of message */
    do
    {
        c = (uchar)getMsgChar();
    } while (c != -1);

    /* record exact position of start of message */
    msgBuf->mbheadLoc  = (long)(ftell(msgfl) - (long)1);

    /* get message's room #         */
    msgBuf->mbroomno   = (uchar)getMsgChar();

    /* get message's attribute byte */
    msgBuf->mbattr     = (uchar)getMsgChar();

    getMsgStr(msgBuf->mbId, LABELSIZE);

    do 
    {
        c = (char)getMsgChar();
        switch (c)
        {
        case 'A':     getMsgStr(msgBuf->mbauth,  LABELSIZE);    break;
        case 'C':     getMsgStr(msgBuf->mbcopy,  LABELSIZE);    break;
        case 'D':     getMsgStr(msgBuf->mbtime,  LABELSIZE);    break;
        case 'F':     getMsgStr(msgBuf->mbfwd,   LABELSIZE);    break;
        case 'G':     getMsgStr(msgBuf->mbgroup, LABELSIZE);    break;
        case 'I':     getMsgStr(msgBuf->mbreply, LABELSIZE);    break;
        case 'L':     getMsgStr(msgBuf->mblink,  64);           break;
        case 'M':     /* will be read off disk later */         break;
        case 'N':     getMsgStr(msgBuf->mbtitle, LABELSIZE);    break;
        case 'n':     getMsgStr(msgBuf->mbsur,   LABELSIZE);    break;
        case 'O':     getMsgStr(msgBuf->mboname, LABELSIZE);    break;
        case 'o':     getMsgStr(msgBuf->mboreg,  LABELSIZE);    break;
        case 'P':     getMsgStr(msgBuf->mbfpath, 256     );     break;
        case 'p':     getMsgStr(msgBuf->mbtpath, 128     );     break;
        case 'Q':     getMsgStr(msgBuf->mbocont, LABELSIZE);    break;
        case 'q':     getMsgStr(msgBuf->mbczip,  LABELSIZE);    break;
        case 'R':     getMsgStr(msgBuf->mbroom,  LABELSIZE);    break;
        case 'S':     getMsgStr(msgBuf->mbsrcId, LABELSIZE);    break;
        case 'T':     getMsgStr(msgBuf->mbto,    LABELSIZE);    break;
        case 'X':     getMsgStr(msgBuf->mbx,     LABELSIZE);    break;
        case 'Z':     getMsgStr(msgBuf->mbzip,   LABELSIZE);    break;
        case 'z':     getMsgStr(msgBuf->mbrzip,  LABELSIZE);    break;
#if 1
        case 'J':     getMsgStr(msgBuf->mbcreg,  LABELSIZE);    break;
        case 'j':     getMsgStr(msgBuf->mbccont, LABELSIZE);    break;
        case '.':     getMsgStr(msgBuf->mbsig,   80       );    break;
        case 'B':     getMsgStr(msgBuf->mbsubj,  72       );    break;
        case 's':     getMsgStr(msgBuf->mbsw  ,  LABELSIZE);    break;
        case '_':     getMsgStr(msgBuf->mbusig,  80       );    break;
#endif

        default:
            getMsgStr(msgBuf->mbtext, cfg.maxtext);  /* discard unknown field  */
            msgBuf->mbtext[0]    = '\0';
            break;
        }
    } while (c != 'M' && c != 'L'); /* && isalpha(c)); */
}

/* -------------------------------------------------------------------- */
/*  mAbort()        returns TRUE if the user has aborted typeout        */
/* -------------------------------------------------------------------- */
BOOL mAbort(void)
{
    char c, toReturn = FALSE, oldEcho;
    static char *jumpstr = "0<3Jump0>";
    static char *nextstr = "0<3Next0>";
    static char *stopstr = "0<3Stop0>";
    static char *killstr = "0<3Kill0>";
    static char *markstr = "0<3Mark0>";

    /* Check for abort/pause from user */
    if (outFlag == IMPERVIOUS)  return FALSE;

    if (!BBSCharReady() && outFlag != OUTPAUSE)
    {
        /* Let modIn() report the problem */
        if (haveCarrier && !gotCarrier())  iChar(); 
        toReturn        = FALSE;
    } 
    else 
    {
        oldEcho  = echo;
        echo     = NEITHER;
        echoChar = 0;

        if (outFlag == OUTPAUSE)
        {
            outFlag = OUTOK;
            c = 'P';
        }
        else
        {
            c = (char)toupper(iChar());
        }

        switch (c)
        {
        case 19: /* XOFF */
        case 'P':                            /* pause:         */
            c = (char)toupper(iChar());             /* wait to resume */
            switch(c)
            {
               case 'J':                            /* jump paragraph:*/
                   mPrintf(jumpstr);
                   outFlag     = OUTPARAGRAPH;
                   toReturn    = FALSE;
                   break;
               case 'K':                            /* kill:          */
                   if ( aide ||
                      (cfg.kill && 
                      (strcmpi(logBuf.lbname, msgBuf->mbauth) == SAMESTRING)
                      && loggedIn))
                   {
                      dotoMessage = PULL_IT;
                      mPrintf(killstr);
                   }
                   toReturn               = FALSE;
                   break;
               case 'M':                            /* mark:          */
                   if (aide)
                   {
                       dotoMessage = MARK_IT;
                       mPrintf(markstr);
                   }
                   toReturn    = FALSE;
                   break;
               case 'N':                            /* next:          */
                   mPrintf(nextstr);
                   outFlag     = OUTNEXT;
                   toReturn    = TRUE;
                   break;
               case 'S':                            /* skip:          */
                   mPrintf(stopstr);
                   outFlag     = OUTSKIP;
                   toReturn    = TRUE;
                   break;
               case 'R':
                   dotoMessage = REVERSE_READ;
                   toReturn    = FALSE;
                   break;
               case '~':
                   termCap(TERM_NORMAL);
                   ansiOn = (BOOL)(!ansiOn);
                   break;
               default:
                   toReturn    = FALSE;
                   break;
            }
            break;
        case 'J':                            /* jump paragraph:*/
            mPrintf(jumpstr);
            outFlag     = OUTPARAGRAPH;
            toReturn    = FALSE;
            break;
        case 'K':                            /* kill:          */
            if ( aide ||
               (cfg.kill && (strcmpi(logBuf.lbname, msgBuf->mbauth) == SAMESTRING)
               && loggedIn))
            {
               dotoMessage = PULL_IT;
               mPrintf(killstr);
            }
            toReturn    = FALSE;
            break;
        case 'M':                            /* mark:          */
            if (aide)
            {
                dotoMessage = MARK_IT;
                mPrintf(markstr);
            }
            toReturn    = FALSE;
            break;
        case 'N':                            /* next:          */
            mPrintf(nextstr);
            outFlag     = OUTNEXT;
            toReturn    = TRUE;
            break;
        case 'S':                            /* skip:          */
            mPrintf(stopstr);
            outFlag     = OUTSKIP;
            toReturn    = TRUE;
            break;
        case 'R':
            dotoMessage = REVERSE_READ;
            toReturn    = FALSE;
            break;
        case '~':
            termCap(TERM_NORMAL);
            ansiOn = (BOOL)(!ansiOn);
            break;
        default:
            toReturn    = FALSE;
            break;
        }
        echo = oldEcho;
    }
    return toReturn;
}

/* -------------------------------------------------------------------- */
/*  putMessage()    stores a message to disk                            */
/* -------------------------------------------------------------------- */
BOOL putMessage(void)
{
    long timestamp;
    char stamp[20];

    time(&timestamp);

    sprintf(msgBuf->mbId, "%lu", (unsigned long)(cfg.newest + 1) );

    sprintf(stamp, "%ld", timestamp);

    /* record start of message to be noted */
    msgBuf->mbheadLoc = (long)cfg.catLoc;

    /* tell putMsgChar where to write   */
    fseek(msgfl, cfg.catLoc, 0);
 
    /* start-of-message              */
    overwrite(1);
    putMsgChar((char)0xFF);

    /* write room #                  */
    overwrite(1);
    putMsgChar(msgBuf->mbroomno);

    /* write attribute byte  */
    overwrite(1);
    putMsgChar(msgBuf->mbattr);  

    /* write message ID */
    dPrintf("%s", msgBuf->mbId);         

    /* Don't write out the author sometimes.. */
    if (!(loggedIn || strcmpi(msgBuf->mbauth, cfg.nodeTitle) == SAMESTRING))
    {
        *msgBuf->mbauth = 0 /* NULL */;
    }

    /* setup time/datestamp: */
    if (!msgBuf->mbcopy[0])
    {
        if(!*msgBuf->mbtime)
        {
            strcpy(msgBuf->mbtime, stamp);
        }
    }
    else
    {
        *msgBuf->mbtime = 0 /* NULL */;
    }


    /* write room name out:     */
    if (!*msgBuf->mboname)
    {
        if (!msgBuf->mbcopy[0]) 
        { 
            strcpy(msgBuf->mbroom, roomTab[msgBuf->mbroomno].rtname);
        }
        else
        {
            *msgBuf->mbroom = 0 /* NULL */;
        }
    }

    if (!msgBuf->mbcopy[0])  { dPrintf("A%s", msgBuf->mbauth);      }
    if (msgBuf->mbcopy[0])   { dPrintf("C%s", msgBuf->mbcopy);      }
    if (msgBuf->mbtime[0])   { dPrintf("D%s", msgBuf->mbtime);      }
    if (msgBuf->mbfwd[0])    { dPrintf("F%s", msgBuf->mbfwd);       }
    if (msgBuf->mbgroup[0])  { dPrintf("G%s", msgBuf->mbgroup);     }
    if (msgBuf->mbreply[0])  { dPrintf("I%s", msgBuf->mbreply);     }
    if (msgBuf->mblink[0])   { dPrintf("L%s", msgBuf->mblink);      }
    if (msgBuf->mbtitle[0])  { dPrintf("N%s", msgBuf->mbtitle);     }
    if (msgBuf->mbsur[0])    { dPrintf("n%s", msgBuf->mbsur);       }
    if (msgBuf->mboname[0])  { dPrintf("O%s", msgBuf->mboname);     }
    if (msgBuf->mboreg[0])   { dPrintf("o%s", msgBuf->mboreg);      }
    if (msgBuf->mbfpath[0])  { dPrintf("P%s", msgBuf->mbfpath);     }
    if (msgBuf->mbtpath[0])  { dPrintf("p%s", msgBuf->mbtpath);     }
    if (msgBuf->mbocont[0])  { dPrintf("Q%s", msgBuf->mbocont);     }
    if (msgBuf->mbczip[0])   { dPrintf("q%s", msgBuf->mbczip);      }
    if (msgBuf->mbroom[0])   { dPrintf("R%s", msgBuf->mbroom);      }
    if (msgBuf->mbsrcId[0])  { dPrintf("S%s", msgBuf->mbsrcId);     }
    if (msgBuf->mbto[0])     { dPrintf("T%s", msgBuf->mbto);        }
    if (msgBuf->mbx[0])      { dPrintf("X%s", msgBuf->mbx);         }
    if (msgBuf->mbzip[0])    { dPrintf("Z%s", msgBuf->mbzip);       }
    if (msgBuf->mbrzip[0])   { dPrintf("z%s", msgBuf->mbrzip);      }
#if 1
    if (msgBuf->mbcreg[0])   { dPrintf("J%s", msgBuf->mbcreg);      }
    if (msgBuf->mbccont[0])  { dPrintf("j%s", msgBuf->mbccont);     }
    if (msgBuf->mbsig[0])    { dPrintf(".%s", msgBuf->mbsig);       }
    if (msgBuf->mbsubj[0])   { dPrintf("B%s", msgBuf->mbsubj);      }
    if (msgBuf->mbsw[0])     { dPrintf("s%s", msgBuf->mbsw);        }
    if (msgBuf->mbusig[0])   { dPrintf("_%s", msgBuf->mbusig);      }
#endif

    /* M-for-message. */
    overwrite(1);
    putMsgChar('M'); putMsgStr(msgBuf->mbtext);

    /* now finish writing */
    fflush(msgfl);

    /* record where to begin writing next message */
    cfg.catLoc = ftell(msgfl);

    talleyBuf.room[msgBuf->mbroomno].total++;

    if (mayseemsg()) 
    {
        talleyBuf.room[msgBuf->mbroomno].messages++;
        talleyBuf.room[msgBuf->mbroomno].new++;
    }

    return  TRUE;
}

/* -------------------------------------------------------------------- */
/*  noteMessage()   puts message in mesgBuf into message index          */
/* -------------------------------------------------------------------- */
void noteMessage(void)
{
    ulong id,copy;
    int crunch = 0;
    int slot, copyslot;

    logBuf.lbvisit[0]   = ++cfg.newest;

    sscanf(msgBuf->mbId, "%lu", &id);

    /* mush up any obliterated messages */
    if (cfg.mtoldest < cfg.oldest)
    {
        crunch = ((ushort)(cfg.oldest - cfg.mtoldest));
    }

    /* scroll index at #nmessages mark */
    if ( (int)(id - cfg.mtoldest) >= cfg.nmessages)
    {
        crunch++;
    }

    if (crunch)
    {
        crunchmsgTab(crunch);
    }

    /* now record message info in index */
    indexmessage(id);

    /* special for duplicated messages */
    /* This is special. */
    if  (*msgBuf->mbcopy)
    {
        /* get the ID# */
        sscanf(msgBuf->mbcopy, "%ld", &copy);

        copyslot = indexslot(copy);
        slot     = indexslot(id);

        if (copyslot != ERROR)
        {
            copyindex(slot, copyslot);
        }
    }
}

/* -------------------------------------------------------------------- */
/*  indexmessage()  builds one message index from msgBuf                */
/* -------------------------------------------------------------------- */
void indexmessage(ulong here)
{
    ushort slot;
    ulong copy;
    ulong oid;
    
    slot = indexslot(here);

    msgTab2[slot].mtmsgLoc            = (long)0;

    msgTab1[slot].mtmsgflags.MAIL     = 0;
    msgTab1[slot].mtmsgflags.RECEIVED = 0;
    msgTab1[slot].mtmsgflags.REPLY    = 0;
    msgTab1[slot].mtmsgflags.PROBLEM  = 0;
    msgTab1[slot].mtmsgflags.MADEVIS  = 0;
    msgTab1[slot].mtmsgflags.LIMITED  = 0;
    msgTab1[slot].mtmsgflags.MODERATED= 0;
    msgTab1[slot].mtmsgflags.RELEASED = 0;
    msgTab1[slot].mtmsgflags.COPY     = 0;
    msgTab1[slot].mtmsgflags.NET      = 0;

    msgTab6[slot].mtauthhash  = 0;
    msgTab5[slot].mttohash    = 0;
    msgTab7[slot].mtfwdhash   = 0;
    msgTab3[slot].mtoffset    = 0;
    msgTab9[slot].mtorigin    = 0;
    msgTab8[slot].mtomesg     = (long)0;

    msgTab4[slot].mtroomno    = DUMP;

    /* --- */
    
    msgTab2[slot].mtmsgLoc    = msgBuf->mbheadLoc;

    if (*msgBuf->mbsrcId)
    {
        sscanf(msgBuf->mbsrcId, "%ld", &oid);
        msgTab8[slot].mtomesg = oid;
    }

    if (*msgBuf->mbauth)  msgTab6[slot].mtauthhash =  hash(msgBuf->mbauth);

    if (*msgBuf->mbto)
    {
        msgTab5[slot].mttohash   =  hash(msgBuf->mbto);    

        msgTab1[slot].mtmsgflags.MAIL = 1;

        if (*msgBuf->mbfwd)  msgTab7[slot].mtfwdhash = hash(msgBuf->mbfwd);
    }
    else if (*msgBuf->mbgroup)
    {
        msgTab5[slot].mttohash   =  hash(msgBuf->mbgroup);
        msgTab1[slot].mtmsgflags.LIMITED = 1;
    }

    if (*msgBuf->mboname)
      msgTab9[slot].mtorigin = hash(msgBuf->mboname);

    if (strcmpi(msgBuf->mbzip, cfg.nodeTitle) != SAMESTRING && *msgBuf->mbzip)
    {
        msgTab1[slot].mtmsgflags.NET = 1;
        msgTab5[slot].mttohash       = hash(msgBuf->mbzip);
    }

    if (*msgBuf->mbx)  msgTab1[slot].mtmsgflags.PROBLEM = 1;

    msgTab1[slot].mtmsgflags.RECEIVED = 
        ((msgBuf->mbattr & ATTR_RECEIVED) == ATTR_RECEIVED);

    msgTab1[slot].mtmsgflags.REPLY    = 
        ((msgBuf->mbattr & ATTR_REPLY   ) == ATTR_REPLY   );

    msgTab1[slot].mtmsgflags.MADEVIS  = 
        ((msgBuf->mbattr & ATTR_MADEVIS ) == ATTR_MADEVIS );

    msgTab4[slot].mtroomno = msgBuf->mbroomno;

    /* This is special. */
    if  (*msgBuf->mbcopy)
    {
        msgTab1[slot].mtmsgflags.COPY = 1;

        /* get the ID# */
        sscanf(msgBuf->mbcopy, "%ld", &copy);

        msgTab3[slot].mtoffset = (ushort)(here - copy);
    }

    if (roomBuild) buildroom();
}
