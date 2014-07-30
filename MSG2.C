/* -------------------------------------------------------------------- */
/*  MSG2.C                        ACit                         91Sep30  */
/*                low level code for accessing messages                 */
/* -------------------------------------------------------------------- */

#include <string.h>
#include <stdarg.h>
#include <malloc.h>
#include "ctdl.h"
#include "prot.h"
#include "glob.h"

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/*  indexslot()     give it a message # and it returns a slot#          */
/*  mayseemsg()     returns TRUE if person can see message. 100%        */
/*  mayseeindexmsg() Can see message by slot #. 99%                     */
/*  changeheader()  Alters room# or attr byte in message base & index   */
/*  crunchmsgTab()  obliterates slots at the beginning of table         */
/*  dGetWord()      fetches one word from current message, off disk     */
/*  getMsgChar()    reads a character from msg file, curent position    */
/*  getMsgStr()     reads a NULL terminated string from msg file        */
/*  putMsgChar()    writes character to message file                    */
/*  sizetable()     returns # messages in table                         */
/*  copyindex()     copies msg index source to message index dest w/o   */
/*  dPrintf()       sends formatted output to message file              */
/*  overwrite()     checks for any overwriting of old messages          */
/*  putMsgStr()     writes a string to the message file                 */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  indexslot()     give it a message # and it returns a slot#          */
/* -------------------------------------------------------------------- */
int indexslot(ulong msgno)
{ 
    if (msgno < cfg.mtoldest)
    {
        if (debug)
        {
            doCR();
            mPrintf("Can't find attribute");
            doCR();
        }
        return(ERROR);
    }

    return((int)(msgno - cfg.mtoldest));
}

/* -------------------------------------------------------------------- */
/*  mayseemsg()     returns TRUE if person can see message. 100%        */
/* -------------------------------------------------------------------- */
BOOL mayseemsg(void)
{
    int i;
    uchar attr;

    if (!copyflag) attr = msgBuf->mbattr;
    else           attr = originalattr;

    /* mfUser */
    if ( mf.mfUser[0] )
    {
        if (   !u_match(msgBuf->mbto, mf.mfUser)
            && !u_match(msgBuf->mbauth, mf.mfUser)
#if 1
            && !u_match(msgBuf->mbfwd,  mf.mfUser)
            && !u_match(msgBuf->mboname,mf.mfUser)
            && !u_match(msgBuf->mboreg, mf.mfUser)
            && !u_match(msgBuf->mbocont,mf.mfUser)
            && !u_match(msgBuf->mbzip,  mf.mfUser)
            && !u_match(msgBuf->mbrzip, mf.mfUser)
            && !u_match(msgBuf->mbczip, mf.mfUser)
#endif
               ) return (FALSE);
    }

    /* check for PUBLIC non problem user messages first */
    if ( !msgBuf->mbto[0] && !msgBuf->mbx[0] )
        return(TRUE);

    if (!loggedIn && dowhat != NETWORKING) return(FALSE);

    /* problem users cant see copys of their own messages */
    if (strcmpi(msgBuf->mbauth,  logBuf.lbname) == SAMESTRING && msgBuf->mbx[0]
       && copyflag) return(FALSE);

    /* but everyone else cant see the orignal if it has been released */
    if (strcmpi(msgBuf->mbauth, logBuf.lbname) != SAMESTRING && msgBuf->mbx[0]
       && !copyflag && ((attr & ATTR_MADEVIS) == ATTR_MADEVIS)
        ) return(FALSE);

    /* author can see his own private messages */
    if (strcmpi(msgBuf->mbauth,  logBuf.lbname) == SAMESTRING)
       return(TRUE);

    if (msgBuf->mbx[0] )
    {
        if (!aide && msgBuf->mbx[0] == 'Y' && 
            !((attr & ATTR_MADEVIS) == ATTR_MADEVIS))
            return(FALSE);
        if (msgBuf->mbx[0] == 'M' && !((attr & ATTR_MADEVIS) == ATTR_MADEVIS))
           if ( !(sysop || (aide && !cfg.moderate)) )
             return(FALSE);
    }   

    if ( msgBuf->mbto[0] )
    {
        /* recipient can see private messages      */
        if (strcmpi(msgBuf->mbto, logBuf.lbname) == SAMESTRING)
        return(TRUE);          

        /* forwardee can see private messages      */
        if (strcmpi(msgBuf->mbfwd, logBuf.lbname) == SAMESTRING)
        return(TRUE);
            
        /* sysops see messages to 'Sysop'           */
        if ( sysop && ( strcmpi(msgBuf->mbto, "Sysop") == SAMESTRING) )
        return(TRUE);

        /* aides see messages to 'Aide'           */
        if ( aide && ( strcmpi(msgBuf->mbto, "Aide") == SAMESTRING) )
        return(TRUE);

        /* none of those so cannot see message     */
        return(FALSE);
    }

    if ( msgBuf->mbgroup[0] )
    {
        if (mf.mfGroup[0])
        {
          if (strcmpi(mf.mfGroup, msgBuf->mbgroup) != SAMESTRING)
            return(FALSE);
        }

        for (i = 0 ; i < MAXGROUPS; ++i)
        {
            /* check to see which group message is to */
            if (strcmpi(grpBuf.group[i].groupname, msgBuf->mbto) == SAMESTRING)
            {
#if 1
                if (i == 0 && logBuf.lbflags.NODE)    /* Nodes can never */
                    return(FALSE);                    /* read Null group */
#endif
                /* if in that group */
                if (logBuf.groups[i] == grpBuf.group[i].groupgen )
                return(TRUE);
            }
        } /* group can't see message, return false */
        return(FALSE);
    }

    return(TRUE);
}


/* -------------------------------------------------------------------- */
/*  mayseeindexmsg() Can see message by slot #. 99%                     */
/* -------------------------------------------------------------------- */
BOOL mayseeindexmsg(int slot)
{
    int i;

    if (msgTab3[slot].mtoffset > (unsigned short)slot)  return(FALSE);

    /* check for PUBLIC non problem user messages first */
    if ( !msgTab5[slot].mttohash && !msgTab1[slot].mtmsgflags.PROBLEM)
    {
        return(TRUE);
    }

    if (!loggedIn && dowhat != NETWORKING) return(FALSE);

    if (msgTab1[slot].mtmsgflags.PROBLEM)
    {
        if (msgTab1[slot].mtmsgflags.COPY)
        {
            /* problem users can not see copys of their messages */
            if (msgTab6[slot].mtauthhash == (int)hash(logBuf.lbname))
                return FALSE;
        }
        else
        {
            if (
                   (
                        /* if you are a aide/sop and it is not MADEVIS */
                        (!aide && !sysop) 
                     || msgTab1[slot].mtmsgflags.MADEVIS
                   )
                && msgTab6[slot].mtauthhash != (int)hash(logBuf.lbname)
               ) return FALSE;
        }
    }   

    if (msgTab1[slot].mtmsgflags.MAIL)
    {
        /* author can see his own private messages */
        if (msgTab6[slot].mtauthhash == (int)hash(logBuf.lbname)
            && msgTab9[slot].mtorigin == 0 /* NULL */)  return(TRUE);

        /* recipient can see private messages      */
        if (msgTab5[slot].mttohash == (int)hash(logBuf.lbname)
            && !msgTab1[slot].mtmsgflags.NET)   return(TRUE);

        /* forwardee can see private messages      */
        if (msgTab7[slot].mtfwdhash == (int)hash(logBuf.lbname))  return(TRUE);
            
        /* sysops see messages to 'Sysop'           */
        if ( sysop && (msgTab5[slot].mttohash == (int)hash("Sysop")) )
        return(TRUE);

        /* aides see messages to 'Aide'           */
        if ( aide && (msgTab5[slot].mttohash == (int)hash("Aide")) )
        return(TRUE);

        /* none of those so cannot see message     */
        return(FALSE);
    }

    if (msgTab1[slot].mtmsgflags.LIMITED)
    {
        if (*(mf.mfGroup))
        {
          if ((int)hash(mf.mfGroup) != msgTab5[slot].mttohash)
            return(FALSE);
        }

        for (i = 0 ; i < MAXGROUPS; ++i)
        {
            /* check to see which group message is to */
            if ((int)hash(grpBuf.group[i].groupname) == msgTab5[slot].mttohash)
            {
                /* if in that group */
                if (logBuf.groups[i] == grpBuf.group[i].groupgen )
                return(TRUE);
            }
        } /* group can't see message, return false */
        return(FALSE);
    }
    return(TRUE);
}

/* -------------------------------------------------------------------- */
/*  changeheader()  Alters room# or attr byte in message base & index   */
/* -------------------------------------------------------------------- */
void changeheader(ulong id, uchar roomno, uchar attr)
{
    long loc;
    int  slot;
    int  c;
    long pos;
    int  room;

    pos = ftell(msgfl);
    slot = indexslot(id);

    /*
     * Change the room # for the message
     */
    if (roomno != 255)
    {
        /* determine room # of message to be changed */
        room = msgTab4[slot].mtroomno;

        /* fix the message tallys from */
        talleyBuf.room[room].total--;
        if (mayseeindexmsg(slot))
        {
            talleyBuf.room[room].messages--;
            if  ((ulong)(cfg.mtoldest + slot) >
                logBuf.lbvisit[ logBuf.lbroom[room].lvisit ])
                talleyBuf.room[room].new--;
        }

        /* fix room tallys to */
        talleyBuf.room[roomno].total++;
        if (mayseeindexmsg(slot))
        {
            talleyBuf.room[roomno].messages++;
            if  ((ulong)(cfg.mtoldest + slot) >
                logBuf.lbvisit[ logBuf.lbroom[roomno].lvisit ])
                talleyBuf.room[room].new++;
        }
    }

    loc  = msgTab2[slot].mtmsgLoc;
    if (loc == ERROR) return;

    fseek(msgfl, loc, SEEK_SET);

    /* find start of message */
    do c = getMsgChar(); while (c != 0xFF);

    if (roomno != 255)
    {
        overwrite(1);
        /* write room #    */
        putMsgChar(roomno);

        msgTab4[slot].mtroomno = roomno;
    }
    else
    {
        getMsgChar();
    }

    if (attr != 255)
    {
        overwrite(1);
        /* write attribute */
        putMsgChar(attr);  

        msgTab1[slot].mtmsgflags.RECEIVED
            = ((attr & ATTR_RECEIVED) == ATTR_RECEIVED);

        msgTab1[slot].mtmsgflags.REPLY
            = ((attr & ATTR_REPLY)    == ATTR_REPLY   );

        msgTab1[slot].mtmsgflags.MADEVIS
            = ((attr & ATTR_MADEVIS)  == ATTR_MADEVIS );
    }

    fseek(msgfl, pos, SEEK_SET);
}

/* -------------------------------------------------------------------- */
/*  crunchmsgTab()  obliterates slots at the beginning of table         */
/* -------------------------------------------------------------------- */
void crunchmsgTab(int howmany)
{
    int i;
    int room;
    uint total = (uint)(cfg.nmessages - howmany);

    for (i = 0; i < howmany; ++i)
    {
        room = msgTab4[i].mtroomno;

        talleyBuf.room[room].total--;

        if (mayseeindexmsg(i))
        {
            talleyBuf.room[room].messages--;

            if  ((ulong)(cfg.mtoldest + i) >
                logBuf.lbvisit[ logBuf.lbroom[room].lvisit ])
                talleyBuf.room[room].new--;
        }
    }

    _fmemmove(msgTab1, &(msgTab1[howmany]),
            ( total * (unsigned)sizeof(*msgTab1)) );
    _fmemmove(msgTab2, &(msgTab2[howmany]),
            ( total * (unsigned)sizeof(*msgTab2)) );
    _fmemmove(msgTab3, &(msgTab3[howmany]),
            ( total * (unsigned)sizeof(*msgTab3)) );
    _fmemmove(msgTab4, &(msgTab4[howmany]),
            ( total * (unsigned)sizeof(*msgTab4)) );
    _fmemmove(msgTab5, &(msgTab5[howmany]),
            ( total * (unsigned)sizeof(*msgTab5)) );
    _fmemmove(msgTab6, &(msgTab6[howmany]),
            ( total * (unsigned)sizeof(*msgTab6)) );
    _fmemmove(msgTab7, &(msgTab7[howmany]),
            ( total * (unsigned)sizeof(*msgTab7)) );
    _fmemmove(msgTab8, &(msgTab8[howmany]),
            ( total * (unsigned)sizeof(*msgTab8)) );
    _fmemmove(msgTab9, &(msgTab9[howmany]),
            ( total * (unsigned)sizeof(*msgTab9)) );

    cfg.mtoldest += howmany;
}

/************************************************************************/
/*      dGetWord() fetches one word from current message, off disk      */
/*      returns TRUE if more words follow, else FALSE                   */
/************************************************************************/
BOOL dGetWord(char *dest, int lim)
{
    char c;

    --lim;      /* play it safe */


    /* pick up any leading blanks & tabs: */
    for (c = (char)getMsgChar(); 
    ((c == ' ') || (c == '\t') || (c == '\n'))
        &&  c && lim;   c = (char)getMsgChar())
    {
        if (lim) { *dest++ = c;  lim--; }
    }

    /* step through word: */
    for ( ; 
    ((c != ' ') && (c != '\t') && (c != '\n'))
    && c && lim;  c = (char)getMsgChar())
    {
        if (lim) { *dest++ = c;  lim--; }
    }

    /* took one too many */
#if 1
    if (c)
    {
        if (dest[-1] == '\1')      /* ending with ^A */
            dest++;                /* so stick with following char */
        else
            ungetc(c, msgfl);      /* else save it for later */
    }
#else
    if (c) ungetc(c, msgfl);
#endif

    *dest = '\0';  /* tie off string */

    return  (BOOL)c;
}


/* -------------------------------------------------------------------- */
/*  getMsgChar()    reads a character from msg file, curent position    */
/* -------------------------------------------------------------------- */
int getMsgChar(void)
{
    int c;

    c = fgetc(msgfl);

    if (c == ERROR)
    {
        /* check for EOF */
        if (feof(msgfl))
        {
            clearerr(msgfl);
            fseek(msgfl, 0l, SEEK_SET);
            c = fgetc(msgfl);
        }
    }
    return c;
}

/* -------------------------------------------------------------------- */
/*  getMsgStr()     reads a NULL terminated string from msg file        */
/* -------------------------------------------------------------------- */
void getMsgStr(char *dest, int lim)
{
    char c;

    while ((c = (char)getMsgChar()) != 0)    /* read the complete string     */
    {
        if (lim)                        /* if we have room then         */
        {
            lim--;
            *dest++ = c;                /* copy char to buffer          */
        }
    }
    *dest = '\0';                       /* tie string off with null     */
}


/* -------------------------------------------------------------------- */
/*  putMsgChar()    writes character to message file                    */
/* -------------------------------------------------------------------- */
void putMsgChar(char c)
{
    if (ftell(msgfl) >= (long)((long)cfg.messagek * 1024l))
    {
        /* scroll to the beginning */
        fseek(msgfl, 0l, 0);
    }

    /* write character out */
    fputc(c, msgfl);
}

/* -------------------------------------------------------------------- */
/*  sizetable()     returns # messages in table                         */
/* -------------------------------------------------------------------- */
uint sizetable(void)
{
    return (int)((cfg.newest - cfg.mtoldest) + 1);
}

/* -------------------------------------------------------------------- */
/*  copyindex()     copies msg index source to message index dest w/o   */
/*                  certain fields (attr, room#)                        */
/* -------------------------------------------------------------------- */
void copyindex(int dest, int source)
{
    msgTab5[dest].mttohash             =     msgTab5[source].mttohash;
    msgTab8[dest].mtomesg              =     msgTab8[source].mtomesg;
    msgTab9[dest].mtorigin             =     msgTab9[source].mtorigin;
    msgTab6[dest].mtauthhash           =     msgTab6[source].mtauthhash;
    msgTab7[dest].mtfwdhash            =     msgTab7[source].mtfwdhash;
    msgTab1[dest].mtmsgflags.MAIL      =     msgTab1[source].mtmsgflags.MAIL;
    msgTab1[dest].mtmsgflags.LIMITED   =     msgTab1[source].mtmsgflags.LIMITED;
    msgTab1[dest].mtmsgflags.PROBLEM   =     msgTab1[source].mtmsgflags.PROBLEM;

    msgTab1[dest].mtmsgflags.COPY    = TRUE;
}

/* -------------------------------------------------------------------- */
/*  dPrintf()       sends formatted output to message file              */
/* -------------------------------------------------------------------- */
void dPrintf(const char *fmt, ... )
{
    char buff[256];
    va_list ap;

    va_start(ap, fmt);
    vsprintf(buff, fmt, ap);
    va_end(ap);

    putMsgStr(buff);
}

/* -------------------------------------------------------------------- */
/*  overwrite()     checks for any overwriting of old messages          */
/* -------------------------------------------------------------------- */
void overwrite(int bytes)
{
    long pos;
    int i;
#if 1
    int c;
#endif

    pos = ftell(msgfl);

    fseek(msgfl, 0l, SEEK_CUR);

    for ( i = 0; i < bytes; ++i)
    {
#if 1
        c = fgetc(msgfl);                  /* this is an implementation */
        if (c == ERROR)                    /* of getMsgChar with the */
        {                                  /* following line changed */
            if (ftell(msgfl) == cfg.messagek * 1024L)   /* msg.dat full? */
            {
                clearerr(msgfl);
                fseek(msgfl, 0L, SEEK_SET);
                c = fgetc(msgfl);
            }
            else if (feof(msgfl))          /* resized MSG.DAT not full */
            {
                clearerr(msgfl);
                return;                    /* no more checking */
            }
        }
        if (c == 0xFF)
#else
        if (getMsgChar() == 0xFF /* -1 */) /* obliterating a message */
#endif
        {
            logBuf.lbvisit[(MAXVISIT-1)]    = ++cfg.oldest;
        }
    }

    fseek(msgfl, pos, SEEK_SET);
}

/* -------------------------------------------------------------------- */
/*  putMsgStr()     writes a string to the message file                 */
/* -------------------------------------------------------------------- */
void putMsgStr(const char *string)
{
    /* check for obliterated messages */
    overwrite(strlen(string) + 1); /* the '+1' is for the null */

    for (;  *string;  string++) putMsgChar(*string);

    /* null to tie off string */
    putMsgChar(0);
}
