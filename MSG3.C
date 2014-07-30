/* -------------------------------------------------------------------- */
/*  MSG3.C                        ACit                         91Sep27  */
/*                       Overlayed message code                         */
/* -------------------------------------------------------------------- */

#include <string.h>
#include <stdarg.h>
#include "ctdl.h"
#include "prot.h"
#include "glob.h"

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/* $copymessage()   copies specified message # into specified room      */
/* $deleteMessage() deletes message for pullIt()                        */
/*  insert()        aide fn: to insert a message                        */
/*  makeMessage()   is menu-level routine to enter a message            */
/* $markIt()        is a sysop special to mark current message          */
/* $markmsg()       marks a message for insertion and/or visibility     */
/*  printMessage()  prints message on modem and console                 */
/* $pullIt()        is a sysop special to remove a message from a room  */
/* $stepMessage()   find the next message in DIR                        */
/*  showMessages()  is routine to print roomful of msgs                 */
/* $printheader()   prints current message header                       */
/* -------------------------------------------------------------------- */

static void  copymessage(ulong id, uchar roomno);
static void  deleteMessage(void);
static BOOL  markIt(void);
static void  markmsg(void);
static unsigned char  pullIt(void);
static unsigned char  stepMessage(unsigned long  *at,int  dir);
static void printheader(ulong id, char verbose, int slot);

/* -------------------------------------------------------------------- */
/*  copymessage()   copies specified message # into specified room      */
/* -------------------------------------------------------------------- */
static void copymessage(ulong id, uchar roomno)
{
    unsigned char attr;
    char copy[20];
    int slot;

#if 1
    if (id == 0UL)
        return;
#endif
    slot = indexslot(id);

    /* load in message to be inserted */
    fseek(msgfl, msgTab2[slot].mtmsgLoc, 0);
    getMessage();

    /* retain vital information */
    attr    = msgBuf->mbattr;
    strcpy(copy, msgBuf->mbId);
    
    clearmsgbuf();
    msgBuf->mbtext[0] = '\0';

    strcpy(msgBuf->mbcopy, copy);
    msgBuf->mbattr   = attr;
    msgBuf->mbroomno = roomno;    
    
    putMessage();
    noteMessage();
}

/* -------------------------------------------------------------------- */
/*  deleteMessage() deletes message for pullIt()                        */
/* -------------------------------------------------------------------- */
static void deleteMessage(void)
{
    ulong id;

    if (!copyflag) sscanf(msgBuf->mbId, "%lu", &id);
    else           id = originalId;

    if (!(*msgBuf->mbx))
        markmsg();    /* Mark it for possible insertion elsewhere */

    changeheader(id, DUMP, 255);

#if 1    
    if (thisRoom != AIDEROOM && thisRoom != DUMP && aide)
#else
    if (thisRoom != AIDEROOM && thisRoom != DUMP)
#endif
    {
        /* note in Aide): */
        sprintf(msgBuf->mbtext, "Following %s deleted by %s:",
                cfg.msg_nym, logBuf.lbname);

        aideMessage();

        copymessage(id, AIDEROOM); 
        if (!logBuf.lbroom[AIDEROOM].lvisit)
            talleyBuf.room[AIDEROOM].new--;

        trap(msgBuf->mbtext, T_AIDE);
    }
}

/* -------------------------------------------------------------------- */
/*  insert()        aide fn: to insert a message  (.AI)                 */
/* -------------------------------------------------------------------- */
void insert(void)
{
    if ( thisRoom   == AIDEROOM  ||  markedMId == 0l )
    {
        mPrintf("Not here.");
        return;
    }
    copymessage(markedMId, (uchar)thisRoom); 
    
    sprintf(msgBuf->mbtext, "Following %s inserted in %s> by %s",
        cfg.msg_nym, roomBuf.rbname, logBuf.lbname );

    trap(msgBuf->mbtext, T_AIDE);

    aideMessage();

    copymessage(markedMId, AIDEROOM); 
    if (!logBuf.lbroom[AIDEROOM].lvisit)
        talleyBuf.room[AIDEROOM].new--;
}

/* -------------------------------------------------------------------- */
/*  makeMessage()   is menu-level routine to enter a message            */
/* -------------------------------------------------------------------- */
BOOL makeMessage(void)
{
#ifdef NETWORK
    int              i;
    int              logNo2;
#endif
    char             *pc, allUpper;
    int              logNo;
    char             recipient[NAMESIZE + NAMESIZE + 3];
    char             rnode[NAMESIZE + NAMESIZE + 3];
#if 1
    char             rreg[NAMESIZE+1];
    char             rcont[NAMESIZE+1];
#endif
    label            forward;
    label            groupname;
    int              groupslot;
    label            replyId;
    char             filelink[64];

    if (oldFlag && heldMessage)
        _fmemmove( msgBuf, msgBuf2, sizeof(struct msgB) );

    *recipient = '\0';
    *forward   = '\0';
    *filelink  = '\0';
    *rnode     = '\0';
#if 1
    *rreg      = '\0';
    *rcont     = '\0';
#endif

    /* limited-access message, ask for group name */
    if (limitFlag)
    {
        getString("group", groupname, NAMESIZE, FALSE, ECHO, "");

        groupslot = partialgroup(groupname);

        if ( groupslot == ERROR || !ingroup(groupslot) )
        {
            mPrintf("\n No such group.");
            return FALSE;
        }
        /* makes it look prettier */
        strcpy(groupname, grpBuf.group[groupslot].groupname);
    }

    /* if replying, author becomes recipient */
    /* also record message which is being replied to */
    if (replyFlag)
    {
        strcpy(recipient, msgBuf->mbauth);
        strcpy(replyId,   *msgBuf->mbsrcId ? msgBuf->mbsrcId : msgBuf->mbId);
        strcpy(rnode,     msgBuf->mboname);
#if 1
        strcpy(rreg,      msgBuf->mboreg);
        strcpy(rcont,     msgBuf->mbocont);
#endif
    }

    /* clear message buffer 'cept when entring old message */
    if (!oldFlag)   setmem(msgBuf, sizeof(struct msgB), 0);

    /* user not logged in, sending exclusive to sysop */
    if (mailFlag && !replyFlag && !loggedIn)
    {
        doCR();
        mPrintf(" Private mail to 'Sysop'");
        strcpy(recipient, "Sysop");
    }

    /* sending exclusive mail which is not a reply */
    if (mailFlag && !replyFlag && loggedIn)
    {
        getNormStr("recipient", recipient, NAMESIZE + NAMESIZE + 1, ECHO);
        if (!strlen(recipient))
        {
            return FALSE;
        }

#ifdef NETWORK
        if (strchr(recipient, '@'))
        {
            strcpy(rnode, strchr(recipient, '@'));
        }

        if (*rnode)
        {
            rnode[0] = ' ';

            normalizeString(rnode);

            if (strlen(rnode) > NAMESIZE)
            {
                mPrintf("\n No node %s.\n", rnode);
                return FALSE;
            }

            for (i=0; i < NAMESIZE + NAMESIZE; i++)
                if (recipient[i] == '@')
                    recipient[i] = '\0';

            normalizeString(recipient);

            if (strlen(recipient) > NAMESIZE)
            {
                mPrintf("\n User's name is too long!\n");
                return FALSE;
            }
        }
#endif

    }

    if (mailFlag)
    {
#ifdef NETWORK
        if (*rnode) alias(rnode);

        logNo = findPerson(*rnode ? rnode : recipient, &lBuf);

        if ( (logNo != ERROR) && *rnode)
        {
            if (!lBuf.lbflags.NODE)
                logNo = ERROR;
        }

        if ( (logNo != ERROR) && lBuf.lbflags.NODE && !rnode[0])
        {
            mPrintf(" %s forwarded to Sysop on %s\n", cfg.msg_nym, recipient);
            strcpy(rnode, recipient);
            strcpy(recipient, "SysOp");
        }

        if ( (logNo != ERROR) && lBuf.forward[0])
        {
            mPrintf(" %s forwarded to ", cfg.msg_nym);

            logNo2 = findPerson(lBuf.forward, &lBuf2);

            if (logNo2 != ERROR)
            {
                mPrintf("%s", lBuf2.lbname);
                strcpy(forward, lBuf2.lbname);
            }
            doCR();
        }

        if ( (logNo == ERROR) && ( hash(recipient) != hash("Sysop"))
           && ( hash(recipient) != hash("Aide")) )
        {
            if (*rnode)
            {
                label temp;
                strcpy(temp, rnode);
                route(temp);
                if (!getnode(temp))
                {
                    mPrintf("Don't know how to reach '%s'", rnode);
                    return FALSE;
                }
            }else{
                 mPrintf("No '%s' known", recipient);
                 return FALSE;
            }
        }
#else
        logNo = findPerson(recipient, &lBuf);

        if ( (logNo == ERROR) && ( hash(recipient) != hash("Sysop"))
           && ( hash(recipient) != hash("Aide")) )
        {
             mPrintf("No '%s' known", recipient);
             return FALSE;
        }
#endif
    }

    if (linkMess)
    {
        getNormStr("file", filelink, 64, ECHO);
        if ( !strlen(filelink)) return FALSE;
    }

    /* copy groupname into the message buffer */
    if (limitFlag)
    {
        strcpy(msgBuf->mbgroup, groupname);
    }

    if (*rnode)
    {
        strcpy(msgBuf->mbzip, rnode);
#if 1
        strcpy(msgBuf->mbrzip, rreg);
        strcpy(msgBuf->mbczip, rcont);
#endif
    }

    /* moderated messages */
    if (
         (
           roomBuf.rbflags.MODERATED 
           || (roomTab[thisRoom].rtflags.SHARED && !logBuf.lbflags.NETUSER)
         )
         && !mailFlag
       )
    {
        strcpy(msgBuf->mbx, "M");
    }

    /* problem user message */
    if (twit && !mailFlag)
    {
        strcpy(msgBuf->mbx, "Y");
    }
 
    /* copy message Id of message being replied to */
    if (replyFlag)
    {
        strcpy(msgBuf->mbreply, replyId);
    }        

    /* finally it's time to copy recipient to message buffer */
    if (*recipient)
    {
        strcpy(msgBuf->mbto, recipient);
    }else{
        msgBuf->mbto[0] = '\0';
    }

    /* finally it's time to copy forward addressee to message buffer */
    if (*forward)
    {
        strcpy(msgBuf->mbfwd, forward);
    }else{
        msgBuf->mbfwd[0] = '\0';
    }

    if (*filelink)
    {
        strcpy(msgBuf->mblink, filelink);
    }else{
        msgBuf->mblink[0] = '\0';
    }

    /* lets handle .Enter old-message */
    if (oldFlag)
    {
        if (!heldMessage)
        {
            mPrintf("\n No aborted %s\n ", cfg.msg_nym);
            return FALSE;
        } else {
            if (!getYesNo("Use aborted message", 1))
                /* clear only the text portion of message buffer */
                setmem( msgBuf->mbtext , sizeof msgBuf->mbtext, 0);
        }
    }

    /* clear our flags */
    heldMessage = FALSE;

    /* copy author name into message buffer */
    if (roomBuf.rbflags.ANON)
    {
        strcpy(msgBuf->mbauth, cfg.anonauthor);
    }
    else if (loggedIn)
    {
        strcpy(msgBuf->mbauth,  logBuf.lbname);
        strcpy(msgBuf->mbsur,   logBuf.surname);
        strcpy(msgBuf->mbtitle, logBuf.title);
    }

    /* set room# and attribute byte for message */
    msgBuf->mbroomno = (uchar)thisRoom;
    msgBuf->mbattr   = 0;

#if 1
    *msgBuf->mbsig = '\0';
    *msgBuf->mbsubj = '\0';
    *msgBuf->mbsw = '\0';
    *msgBuf->mbusig = '\0';
#endif

    if (!linkMess)
    {
        if (getText() == TRUE)
        {
            for (pc=msgBuf->mbtext, allUpper=TRUE; *pc && allUpper;  pc++)
            {
                if (toupper(*pc) != *pc)  allUpper = FALSE;
            }

            if (allUpper)   fakeFullCase(msgBuf->mbtext);
    
            sprintf(msgBuf->mbId, "%lu", (unsigned long)(cfg.newest + 1) );
    
#ifdef NETWORK
            if (*msgBuf->mbzip)  /* save it for netting... */
                save_mail();
#endif
    
            *msgBuf->mboname = '\0';
            *msgBuf->mboreg = '\0';
#if 1
            *msgBuf->mbcreg = '\0';
            *msgBuf->mbccont = '\0';
#endif
    
            if (sysop && msgBuf->mbx[0] == 'M')
            {
                if (getYesNo("Release message", 1))
                {
                    msgBuf->mbx[0] = 0 /* NULL */;
                }
            }
            
            putMessage();
    
            if (!replyFlag)
                MessageRoom[msgBuf->mbroomno]++;
    
            noteMessage();
            limitFlag = 0;  /* keeps Aide) messages from being grouponly */
            /* 
             * if its mail, note it in recipients log entry 
             */
        /*  if (mailFlag) notelogmessage(msgBuf->mbto); */
            /* note in forwarding addressee's log entry */
        /*  if (*forward)  notelogmessage(msgBuf->mbfwd); */
    
            msgBuf->mbto[   0] = '\0';
            msgBuf->mbgroup[0] = '\0';
            msgBuf->mbfwd[  0] = '\0';
    
            oldFlag     = FALSE;
    
            return TRUE;
        }
    }else{
        msgBuf->mbtext[0] = '\0';
     
        sprintf(msgBuf->mbId, "%lu", (unsigned long)(cfg.newest + 1) );
    
        putMessage();

        if (!replyFlag)
            MessageRoom[msgBuf->mbroomno]++;

        noteMessage();
        limitFlag = 0;  /* keeps Aide) messages from being grouponly */
        /* if its mail, note it in recipients log entry */
     /* if (mailFlag)  notelogmessage(msgBuf->mbto);  */
        /* note in forwarding addressee's log entry */
     /* if (*forward)  notelogmessage(msgBuf->mbfwd); */

        msgBuf->mbto[   0] = '\0';
        msgBuf->mbgroup[0] = '\0';
        msgBuf->mbfwd[  0] = '\0';
        msgBuf->mblink[ 0] = '\0';

        return TRUE;
    }  
    oldFlag     = FALSE;
    return FALSE;
}

/* -------------------------------------------------------------------- */
/*  markIt()        is an aide special to mark current message          */
/* -------------------------------------------------------------------- */
static BOOL markIt(void)
{
    ulong id;

    sscanf(msgBuf->mbId, "%lu", &id);

    /* confirm that we're marking the right one: */
    outFlag = OUTOK;
    printMessage( id, (char)0 );

    outFlag = OUTOK;

    doCR();

    if (getYesNo("Mark",1)) 
    {
        markmsg();
        return TRUE;
    }
    else return FALSE;
}

/* -------------------------------------------------------------------- */
/*  markmsg()       marks a message for insertion and/or visibility     */
/* -------------------------------------------------------------------- */
static void markmsg(void)
{
    ulong id;
    uchar attr;

    sscanf(msgBuf->mbId, "%lu", &markedMId);

    if (!copyflag) id = markedMId;
    else           id = originalId;

    if (msgBuf->mbx[0])
    {
        if (!copyflag)  attr = msgBuf->mbattr;
        else            attr = originalattr;

        attr = (unsigned char)(attr ^ ATTR_MADEVIS);

        if (!copyflag)  msgBuf->mbattr = attr;
        else            originalattr  = attr;

        changeheader(id, 255, attr);

        if ((attr & ATTR_MADEVIS) == ATTR_MADEVIS)
            copymessage( id, (uchar)thisRoom);
    }
}

/* -------------------------------------------------------------------- */
/*  printMessage()  prints message on modem and console                 */
/* -------------------------------------------------------------------- */
#define msgstuff  msgTab1[slot].mtmsgflags  

void printMessage(ulong id, char verbose)
{
    char  moreFollows;
    ulong here;
    long  loc;
    int strip;
    int slot;
    static level = 0;

    slot = indexslot(id);
    
    if (slot == ERROR) return;

    if (msgTab1[slot].mtmsgflags.COPY)
    {
        copyflag     = TRUE;
        originalId   = id;
        originalattr = 0;

        originalattr = (uchar)
       (originalattr | (msgstuff.RECEIVED)?ATTR_RECEIVED :0 );

        originalattr = (uchar)
       (originalattr | (msgstuff.REPLY   )?ATTR_REPLY : 0 );

        originalattr = (uchar)
       (originalattr | (msgstuff.MADEVIS )?ATTR_MADEVIS : 0 );

        level ++;

        if (level > 20)
        {
            level = 0;
            return;
        }
        
        if (msgTab3[slot].mtoffset <= (unsigned)slot)
            printMessage( (ulong)(id - (ulong)msgTab3[slot].mtoffset), verbose);

        level --;

        return;
    }

    /* in case it returns without clearing buffer */
    msgBuf->mbfwd[  0]  = '\0';
    msgBuf->mbto[   0]  = '\0';

    loc = msgTab2[slot].mtmsgLoc;
    if (loc == ERROR) return;

    if (copyflag)  slot = indexslot(originalId);

    if (!mayseeindexmsg(slot) ) return;

    fseek(msgfl, loc, 0);

    getMessage();

    dotoMessage = NO_SPECIAL;

    sscanf(msgBuf->mbId, "%lu", &here);

    /* cludge to return on dummy msg #1 */
    if ((int)here == 1) return;

    if (!mayseemsg()) return;

    mread++; /* Increment # messages read */

    if (here != id )
    {
        mPrintf("Can't find message. Looking for %lu at byte %ld!\n ",
                 id, loc);
        return;
    }

    printheader( id, verbose, slot);

    seen = TRUE;

    if (msgBuf->mblink[0])
    {
        dumpf(msgBuf->mblink);
    }
    else
    {
        for(;;) /* while (TRUE) */
        {
            moreFollows     = dGetWord(msgBuf->mbtext, 150);
    
            /* strip control Ls out of the output                   */
            for (strip = 0; msgBuf->mbtext[strip] != 0; strip++)
            {
                if (msgBuf->mbtext[strip] == 0x0c ||
                    msgBuf->mbtext[strip] == 27 /* SPECIAL */)
                    msgBuf->mbtext[strip] = 0x00; /* Null NOT space.. */
            }

            putWord(msgBuf->mbtext);

            if (!(moreFollows  &&  !mAbort()))
            {
                if (outFlag == OUTNEXT)  doCR();   /* If <N>ext, extra line */
                break;
            }
        }
#if 1
        if (logBuf.SIGNATURES && (*msgBuf->mbsig || *msgBuf->mbusig))
#else
        if (*msgBuf->mbsig || *msgBuf->mbusig)
#endif
        {
            termCap(TERM_NORMAL);
            mPrintf(" \n ");
            termCap(TERM_BOLD);
            mPrintf("----");
            termCap(TERM_NORMAL);
            if (*msgBuf->mbusig)
                mPrintf("\n %s", msgBuf->mbusig);
            if (*msgBuf->mbsig)
                mPrintf("\n %s", msgBuf->mbsig);

        }
    }
    termCap(TERM_NORMAL);
    doCR();
    echo = BOTH;
}

/* -------------------------------------------------------------------- */
/*  pullIt()        is a sysop special to remove a message from a room  */
/* -------------------------------------------------------------------- */
static BOOL pullIt(void)
{
    ulong id;
    sscanf(msgBuf->mbId, "%lu", &id);

    /* confirm that we're removing the right one: */
    outFlag = OUTOK;

    printMessage( id,  (char)0 );

    outFlag = OUTOK;

    doCR();

    if (getYesNo("Pull",0)) 
    {
        deleteMessage();
        return TRUE;
    }
    else return FALSE;
}

/* -------------------------------------------------------------------- */
/*  stepMessage()   find the next message in DIR                        */
/* -------------------------------------------------------------------- */
static BOOL stepMessage(ulong *at, int dir)
{
    int i;

    for (i = indexslot(*at), i += dir; i > -1 && i < (int)sizetable(); i += dir)
    {
        /* skip messages not in this room */
        if (msgTab4[i].mtroomno != (uchar)thisRoom) continue;

        /* skip by special flag */
        if (mf.mfMai && !msgTab1[i].mtmsgflags.MAIL) continue;
        if (mf.mfLim && !msgTab1[i].mtmsgflags.LIMITED) continue;
        if (mf.mfPub && 
           (msgTab1[i].mtmsgflags.LIMITED || msgTab1[i].mtmsgflags.MAIL ))
           continue;

        if (mayseeindexmsg(i))
        {
            *at = (ulong)(cfg.mtoldest + i);
            return TRUE;
        }
    }
    return FALSE;
}

/* -------------------------------------------------------------------- */
/*  showMessages()  is routine to print roomful of msgs                 */
/* -------------------------------------------------------------------- */
void showMessages(char whichMess, char revOrder, char verbose)
{
    int   increment; /* i for message save to file */
    ulong lowLim, highLim, msgNo, start;
    unsigned char attr;
    BOOL  done;

    if (mf.mfLim)
    {
        getgroup();
        if (!mf.mfLim)
            return;
    }
    else 
    {
      doCR();
    }

    if (mf.mfUser[0])
    {
        if (roomBuf.rbflags.ANON)
        {
            mPrintf("\n\n  --Read by user not allowed in anonymous rooms\n");
            return;
        }
        getNormStr("user", mf.mfUser, NAMESIZE, ECHO);
    }

    outFlag = OUTOK;

    if (!expert )  mPrintf("\n <J>ump <N>ext <P>ause <R>everse <S>top");

    switch (whichMess)  
    {
    case NEWoNLY:
        if (loggedIn)
           lowLim  = logBuf.lbvisit[ logBuf.lbroom[thisRoom].lvisit ] + 1;
        else
           lowLim  = cfg.oldest;
        highLim = cfg.newest;

        /* print out last new message */
        if (!revOrder && oldToo && (highLim >= lowLim))
            stepMessage(&lowLim, -1);
        break;

    case OLDaNDnEW:
        lowLim  = cfg.oldest;
        highLim = cfg.newest;
        break;

    case OLDoNLY:
        lowLim  = cfg.oldest;
        if (loggedIn)
            highLim = logBuf.lbvisit[ logBuf.lbroom[thisRoom].lvisit  ];
        else
            highLim=cfg.newest;
        break;
    }

    /* stuff may have scrolled off system unseen, so: */
    if (cfg.oldest  > lowLim)  lowLim = cfg.oldest;

    /* Allow for reverse retrieval: */
    if (!revOrder)
    {
        start       = lowLim;
        increment   = 1;
    }else{
        start       = highLim;
        increment   = -1;
    }

    start -= (long)increment;
    done = (BOOL)(!stepMessage(&start, increment));

    for (msgNo = start;
         !done 
         && msgNo >= lowLim 
         && msgNo <= highLim 
         && (haveCarrier || (whichIO == CONSOLE));
         done = (BOOL)(!stepMessage(&msgNo, increment)) )
    {

        /*i = indexslot(msgNo);   for save message to file??? */

        if (BBSCharReady()) mAbort();

        if (outFlag != OUTOK)
        {
            if (outFlag == OUTNEXT || outFlag == OUTPARAGRAPH)
            {
                outFlag = OUTOK;
            }
            else if (outFlag == OUTSKIP)  
            {
                echo = BOTH;
                mf.mfPub   = FALSE;
                mf.mfMai   = FALSE;
                mf.mfLim   = FALSE;
                mf.mfUser[0] = FALSE;
                mf.mfGroup[0] = FALSE;
                return;
            }
        }

        seen = FALSE;

        printMessage( msgNo, verbose );

        if (outFlag != OUTNEXT && outFlag != OUTSKIP)
        {
            switch(dotoMessage)
            {
                case PULL_IT:
                    /* Pull current message from room if flag set */
                    pullIt();
                    outFlag = OUTOK;
                    break;
    
                case MARK_IT:
                    /* Mark current message from room if flag set */
                    markIt();
                    outFlag = OUTOK;
                    break;
    
                case REVERSE_READ:
                    increment = -increment;
                    doCR();
                    mPrintf("<Reversed %c>", (increment == 1) ? '+' : '-');
                    lowLim = cfg.oldest;
                    highLim= cfg.newest;     /* reevaluate for Livia */
                    doCR();
                    break;
    
                case NO_SPECIAL:
                    /* Release (Y/N)[N] */
                    if ( *msgBuf->mbx && aide && seen
                      && ( msgBuf->mbattr & ATTR_MADEVIS) != ATTR_MADEVIS
                      && outFlag == OUTOK )
                    if (getYesNo("Release", 0))
                    {
                        markmsg();
                        outFlag = OUTOK;
                    }
    
                    /* reply to mail */
                    if ( whichMess == NEWoNLY 
                      && ( strcmpi(msgBuf->mbto,  logBuf.lbname) == SAMESTRING
                      ||   strcmpi(msgBuf->mbfwd, logBuf.lbname) == SAMESTRING )
                      && loggedIn )
                    {
                       outFlag = OUTOK;
                       doCR();
                       if (getYesNo("Respond", 1)) 
                       {
                           replyFlag = 1;
                           mailFlag  = 1;
                           linkMess  = FALSE;
    
                           if (!copyflag)  attr = msgBuf->mbattr;
                           else            attr = originalattr;
    
                           if (whichIO != CONSOLE)
                           {
                               cPrintf(" to %s\n", msgBuf->mbauth);
                               echo = CALLER;
                           }
    
                           if  (makeMessage()) 
                           {
                               attr = (unsigned char)(attr | ATTR_REPLY);
    
                               if (!copyflag)  msgBuf->mbattr = attr;
                               else            originalattr  = attr;
    
                               if (!copyflag)  changeheader(msgNo,      255, attr);
                               else            changeheader(originalId, 255, attr);
                           }
    
                           replyFlag = 0;
                           mailFlag  = 0;
    
                           /* Restore privacy zapped by make... */
                           if (whichIO != CONSOLE)  echo = BOTH;
    
                           outFlag = OUTOK;
    
                           if (cfg.oldest  > lowLim)
                           {
                               lowLim = cfg.oldest;
                               if (msgNo < lowLim) msgNo = lowLim;
                           }
                       }
                    }
                    break;
                    
                default:
                    mPrintf("Showmess(), dotoMessage == BAD_VALUE\n");
                    break;
            }
        }

        copyflag     = FALSE;
        originalId   = 0;
        originalattr = 0;
    }
    echo = BOTH;
    mf.mfPub   = FALSE;
    mf.mfMai   = FALSE;
    mf.mfLim   = FALSE;
    mf.mfUser[0] = FALSE;
    mf.mfGroup[0] = FALSE;
}

/* -------------------------------------------------------------------- */
/*  printheader()   prints current message header                       */
/* -------------------------------------------------------------------- */
static void printheader(ulong id, char verbose, int slot)
{
    char dtstr[80];
    uchar attr;
    long timestamp;

    if (outFlag == OUTNEXT) outFlag = OUTOK;

    if (*msgBuf->mbto && whichIO != CONSOLE) echo = CALLER;

    termCap(TERM_BOLD);

    if (verbose && !roomBuf.rbflags.ANON)
    {
        doCR();
        mPrintf("    # %s of %lu",msgBuf->mbId, cfg.newest);
        if (copyflag && aide)
            mPrintf(" (Duplicate id # %lu)", originalId);
        if (*msgBuf->mbsrcId) 
        {
            doCR();
            mPrintf("    Source id #%s", msgBuf->mbsrcId);
        }              
        if (*msgBuf->mblink && sysop) 
        {
            doCR();
            mPrintf("    Linked file is %s", msgBuf->mblink);
        }
        if (*msgBuf->mbfpath)
        {
            mPrintf("\n    Path followed: %s!%s", msgBuf->mbfpath, cfg.nodeTitle);
        }
        if (*msgBuf->mbtpath)
        {
            mPrintf("\n    Forced path: %s", msgBuf->mbtpath);
        }
#if 1
        if (*msgBuf->mbsw)
        {
            mPrintf("\n    Software: %s", msgBuf->mbsw);
        }
#endif

    }
    
    doCR();
    mPrintf("    ");

    if (*msgBuf->mbtime && !roomBuf.rbflags.ANON)
    {
        sscanf(msgBuf->mbtime, "%ld", &timestamp);
        sstrftime(dtstr, 79, 
                 verbose ? cfg.vdatestamp : cfg.datestamp, timestamp);
        mPrintf("%s", dtstr);
    }

    if (roomBuf.rbflags.ANON)
        mPrintf(" From 043%s03", cfg.anonauthor);
    else if (msgBuf->mbauth[ 0])
    {
        mPrintf(" From ");
        
       if (msgBuf->mbtitle[0]
#if 1
           && (logBuf.DISPLAYTS || logBuf.title[0]) && (
#else
           && (
#endif
                (cfg.titles && !(msgBuf->mboname[0])) 
                || cfg.nettitles
              )
           )
        {
             mPrintf( "[%s] ", msgBuf->mbtitle);
        }
        
        mPrintf("043%s03", msgBuf->mbauth);
        
        if (msgBuf->mbsur[0]
#if 1
           && (logBuf.DISPLAYTS || logBuf.surname[0]) && (
#else
           && (
#endif
                (cfg.surnames && !(msgBuf->mboname[0])) 
                || cfg.netsurname
              )
           )
        {
             mPrintf( " [%s]", msgBuf->mbsur);
        }
    }
#if 1    
    termCap(TERM_BOLD);
#endif

    if (msgBuf->mboname[0]
        && (strcmpi(msgBuf->mboname, cfg.nodeTitle) != SAMESTRING
          || strcmpi(msgBuf->mboreg, cfg.nodeRegion) != SAMESTRING)
            && strcmpi(msgBuf->mbauth, msgBuf->mboname) != SAMESTRING)
             mPrintf(" @ %s", msgBuf->mboname);

    if (msgBuf->mboreg[0] &&
        strcmpi(msgBuf->mboreg, cfg.nodeRegion) != SAMESTRING)
        {
           mPrintf(", %s", msgBuf->mboreg);
#if 1
           if (verbose && *msgBuf->mbcreg)
               mPrintf(" {%s}", msgBuf->mbcreg);
#endif
        }
#if 1
    if (verbose && *msgBuf->mbocont &&
        strcmpi(msgBuf->mbocont, cfg.nodeCountry) != SAMESTRING)
        {     
           mPrintf(", %s", msgBuf->mbocont);
           if (verbose && *msgBuf->mbccont)
               mPrintf(" {%s}", msgBuf->mbccont);
        }
#endif

    if (msgBuf->mbto[0])
    {
        mPrintf(" To %s", msgBuf->mbto);

        if (msgBuf->mbzip[0]
              && strcmpi(msgBuf->mbzip, cfg.nodeTitle) != SAMESTRING)
                 mPrintf(" @ %s", msgBuf->mbzip);

        if (msgBuf->mbrzip[0] &&
            strcmpi(msgBuf->mbrzip, cfg.nodeRegion))
              mPrintf(", %s", msgBuf->mbrzip);

#if 1
        if (verbose && msgBuf->mbczip[0] &&
            strcmpi(msgBuf->mbczip, cfg.nodeCountry))
              mPrintf(", %s", msgBuf->mbczip);
#endif

        if (msgBuf->mbfwd[0])
            mPrintf(" Forwarded to %s", msgBuf->mbfwd );

        if (msgBuf->mbreply[0])
        {
            if (verbose)
                mPrintf(" [reply to %s]", msgBuf->mbreply);
            else
                mPrintf(" [reply]");
        }
        if ( msgstuff.RECEIVED)  mPrintf(" [received]");
        if ( msgstuff.REPLY)     mPrintf(" [reply sent]");

        if ( (msgBuf->mbto[0])
           && !(strcmpi(msgBuf->mbauth, logBuf.lbname) == SAMESTRING ))
        {

            if (!copyflag)  attr = msgBuf->mbattr;
            else            attr = originalattr;

            if (!(attr & ATTR_RECEIVED))
            {
                attr = (unsigned char)(attr | ATTR_RECEIVED);

                if (!copyflag)  msgBuf->mbattr = attr;
                else            originalattr  = attr;

                if (!copyflag)  changeheader(id,         255, attr);
                else            changeheader(originalId, 255, attr);

            }
        }
    }

    if (verbose && (strcmpi(msgBuf->mbroom, roomBuf.rbname) != SAMESTRING))
    {
        mPrintf(" In %s>",  msgBuf->mbroom );
    }

    if (msgBuf->mbgroup[0])
    {
        mPrintf(" (%s only)", msgBuf->mbgroup);
    }

    if ((aide || sysop) && msgBuf->mbx[0])
    {
        if (!msgstuff.MADEVIS)
        {
          if (msgBuf->mbx[0] == 'Y')
              mPrintf(" [1problem user03]");
          else
              mPrintf(" [moderated]");
        }
        else  mPrintf(" [viewable %s]", msgBuf->mbx[0] == 'Y' ?
              "problem user" : "moderated" );
    }

    if ((aide || sysop) && msgBuf->mblink[0])
        mPrintf(" [file-linked]");

#if 1    
    if (*msgBuf->mbsubj && logBuf.SUBJECTS)
    {
        doCR();
        mPrintf("    Subject: %s", msgBuf->mbsubj);
    }
#endif

    termCap(TERM_NORMAL);
    doCR();
}
