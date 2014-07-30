/* -------------------------------------------------------------------- */
/*  NETPMSG.C                     ACit                         91Sep30  */
/*      Networking libs for the Citadel bulletin board system           */
/* -------------------------------------------------------------------- */

#include <time.h>
#include <string.h>
#include "ctdl.h"
#include "prot.h"
#include "glob.h"

#ifdef NETWORK
/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/* $netcanseeroom() Can the node see this room?                         */
/* $NewRoom()       Puts all new messages in a room to a file           */
/*  PutMessage()    Puts a message to a file                            */
/*  PutStr()        puts a null-terminated string to a file             */
/* $saveMessage()   saves a message to file if it is netable            */
/*  slave()         Actual networking slave                             */
/* -------------------------------------------------------------------- */

static BOOL netcanseeroom(int roomslot);
static void NewRoom(int room, const char *filename);
static void saveMessage(ulong id, FILE *fl);


/* -------------------------------------------------------------------- */
/*  netcanseeroom() Can the node see this room?                         */
/* -------------------------------------------------------------------- */
static BOOL netcanseeroom(int roomslot)
{ 
        /* is room in use              */
    if ( roomTab[roomslot].rtflags.INUSE

        /* and it is shared            */
        && roomTab[roomslot].rtflags.SHARED 

        /* and group can see this room */
        && (groupseesroom(roomslot)
        || roomTab[roomslot].rtflags.READONLY
        || roomTab[roomslot].rtflags.DOWNONLY )       

        /* only aides go to aide room  */ 
        &&   ( roomslot != AIDEROOM || aide) )
    {
        return TRUE;
    }

    return FALSE;
}

/* -------------------------------------------------------------------- */
/*  NewRoom()       Puts all new messages in a room to a file           */
/* -------------------------------------------------------------------- */
static void NewRoom(int room, const char *filename)
{
    int   i, h;
    char str[100];
    ulong lowLim, highLim, msgNo;
    FILE *file;
    int tsize;

    lowLim  = logBuf.lbvisit[ logBuf.lbroom[room].lvisit ] + 1;
    highLim = cfg.newest;

    logBuf.lbroom[room].lvisit = 0;

    /* stuff may have scrolled off system unseen, so: */
    if (cfg.oldest  > lowLim)  lowLim = cfg.oldest;

    sprintf(str, "%s\\%s", cfg.temppath, filename);

    file = fopen(str, "ab");
    if (!file)
    {
        return;
    }

    h = hash(cfg.nodeTitle);
    tsize = (int)sizetable();
    
    for (i = 0; i != tsize; i++)
    {
        msgNo = (ulong)(cfg.mtoldest + i);
        
        if ( msgNo >= lowLim && highLim >= msgNo )
        {
            /* skip messages not in this room */
            if (msgTab4[i].mtroomno != (uchar)room) continue;
    
            /* no open messages from the system */
            if (msgTab6[i].mtauthhash == h) continue;
    
            /* skip mail */
            if (msgTab1[i].mtmsgflags.MAIL) continue;
    
            /* No problem user shit */
            if (
                (msgTab1[i].mtmsgflags.PROBLEM || msgTab1[i].mtmsgflags.MODERATED) 
            && !(msgTab1[i].mtmsgflags.MADEVIS || msgTab1[i].mtmsgflags.MADEVIS)
            && !sysop)
            { 
#ifdef DEBUGEM                
                if (debug) 
                {
                    cPrintf("\nPROBLEM USER MESSAGE NOT SAVED! "
                            "(Problem = %d, Moderated = %d, Released = %d)\n",
                        msgTab1[i].mtmsgflags.PROBLEM,
                        msgTab1[i].mtmsgflags.MODERATED, 
                        msgTab1[i].mtmsgflags.RELEASED);
                }
#endif                
                continue;
            }

            saveMessage( msgNo, file );
            mread ++;
        }
    }
    fclose(file);
}

/* -------------------------------------------------------------------- */
/*  PutMessage()    Puts a message to a file                            */
/* -------------------------------------------------------------------- */
void PutMessage(FILE *fl)
{
    /* write start of message */
    fputc(0xFF, fl);

    /* put message's attribute byte */
    msgBuf->mbattr = (uchar)(msgBuf->mbattr & (ATTR_RECEIVED|ATTR_REPLY));
    fputc(msgBuf->mbattr, fl);

    /* put local ID # out */
    PutStr(fl, msgBuf->mbId);

    if (!*msgBuf->mboname)
    {   
        strcpy(msgBuf->mboname, cfg.nodeTitle);
        strcpy(msgBuf->mboreg,  cfg.nodeRegion);
        strcpy(msgBuf->mbocont, cfg.nodeCountry); 
#if 1
        sprintf(msgBuf->mbsw, "%s %s", softname, version);
        strcpy(msgBuf->mbsig,   cfg.signature);
        strcpy(msgBuf->mbcreg,  cfg.twitRegion);
        strcpy(msgBuf->mbccont, cfg.twitCountry);
#endif

    }
    
    if (!*msgBuf->mbsrcId)
    {
        strcpy(msgBuf->mbsrcId, msgBuf->mbId);
    }
    
    if (*msgBuf->mbfpath)
    {
        strcat(msgBuf->mbfpath, "!");
    }
    strcat(msgBuf->mbfpath, cfg.nodeTitle);

    if (!*msgBuf->mbtime)
    {
        sprintf(msgBuf->mbtime, "%ld", time(NULL));
    }
    
    fputc('A', fl); PutStr(fl, msgBuf->mbauth);
    fputc('D', fl); PutStr(fl, msgBuf->mbtime);
    fputc('O', fl); PutStr(fl, msgBuf->mboname);
    fputc('o', fl); PutStr(fl, msgBuf->mboreg);
    fputc('S', fl); PutStr(fl, msgBuf->mbsrcId);
    fputc('P', fl); PutStr(fl, msgBuf->mbfpath);
    
    if (*msgBuf->mbfwd)   { fputc('F', fl); PutStr(fl, msgBuf->mbfwd);   }
    if (*msgBuf->mbgroup) { fputc('G', fl); PutStr(fl, msgBuf->mbgroup); }
    if (*msgBuf->mbreply) { fputc('I', fl); PutStr(fl, msgBuf->mbreply); }
    if (*msgBuf->mbtitle) { fputc('N', fl); PutStr(fl, msgBuf->mbtitle); }
    if (*msgBuf->mbsur)   { fputc('n', fl); PutStr(fl, msgBuf->mbsur);   }
    if (*msgBuf->mbtpath) { fputc('p', fl); PutStr(fl, msgBuf->mbtpath); }
    if (*msgBuf->mbocont) { fputc('Q', fl); PutStr(fl, msgBuf->mbocont); }
    if (*msgBuf->mbczip)  { fputc('q', fl); PutStr(fl, msgBuf->mbczip);  }
    if (*msgBuf->mbroom)  { fputc('R', fl); PutStr(fl, msgBuf->mbroom);  }
    if (*msgBuf->mbto)    { fputc('T', fl); PutStr(fl, msgBuf->mbto);    }
    if (*msgBuf->mbzip)   { fputc('Z', fl); PutStr(fl, msgBuf->mbzip);   }
    if (*msgBuf->mbrzip)  { fputc('z', fl); PutStr(fl, msgBuf->mbrzip);  }
#if 1
    if (*msgBuf->mbcreg)  { fputc('J', fl); PutStr(fl, msgBuf->mbcreg);  }
    if (*msgBuf->mbccont) { fputc('j', fl); PutStr(fl, msgBuf->mbccont); }
    if (*msgBuf->mbsig)   { fputc('.', fl); PutStr(fl, msgBuf->mbsig);   }
    if (*msgBuf->mbsubj)  { fputc('B', fl); PutStr(fl, msgBuf->mbsubj);  }
    if (*msgBuf->mbsw)    { fputc('s', fl); PutStr(fl, msgBuf->mbsw);    }
    if (*msgBuf->mbusig)  { fputc('_', fl); PutStr(fl, msgBuf->mbusig);  }
#endif

    /* put the message field  */
    fputc('M', fl); PutStr(fl, msgBuf->mbtext);
}

/* -------------------------------------------------------------------- */
/*  PutStr()        puts a null-terminated string to a file             */
/* -------------------------------------------------------------------- */
void PutStr(FILE *fl, const char *str)
{
    fwrite(str, sizeof(char), (strlen(str) + 1), fl);
}

/* -------------------------------------------------------------------- */
/*  saveMessage()   saves a message to file if it is netable            */
/* -------------------------------------------------------------------- */
#define msgstuff  msgTab1[slot].mtmsgflags  
static void saveMessage(ulong id, FILE *fl)
{
    ulong here;
    ulong loc;
    int   slot;

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
                       
        if (msgTab3[slot].mtoffset <= (ushort)slot)
            saveMessage( (ulong)(id - (ulong)msgTab3[slot].mtoffset), fl);

        return;
    }

    /* in case it returns without clearing buffer */
    *msgBuf->mbfwd  = '\0';
    *msgBuf->mbto   = '\0';

    loc = msgTab2[slot].mtmsgLoc;
    if (loc == ERROR) return;

    if (copyflag)  slot = indexslot(originalId);

    if (!mayseeindexmsg(slot) && !msgTab1[slot].mtmsgflags.NET) return;

    fseek(msgfl, loc, 0);

    getMessage();
    getMsgStr(msgBuf->mbtext, cfg.maxtext);

    sscanf(msgBuf->mbId, "%lu", &here);

    /* cludge to return on dummy msg #1 */
    if (here == 1L) return;

    if (!mayseemsg() && !msgTab1[slot].mtmsgflags.NET) return;

    if (here != id )
    {
        cPrintf("Can't find message. Looking for %lu at byte %ld!\n ",
                 id, loc);
        return;
    }

    if (*msgBuf->mblink)   return;

    PutMessage(fl);
}

/* -------------------------------------------------------------------- */
/*  slave()         Actual networking slave                             */
/* -------------------------------------------------------------------- */
BOOL slave(void)
{
    char line[75];
    label troo, fn;
    FILE *file, *fopen();
    int i = 0, rm;
    
    cPrintf(" Sending mail file.");
    doccr();

    sprintf(line, "%s\\%s", cfg.transpath, node.ndmailtmp);
#if 1
    fclose(fopen(line, "ab"));
#endif
    wxsnd(cfg.temppath, line, 
         (char)strpos((char)tolower(node.ndprotocol[0]), extrncmd));
  
    if (!gotCarrier()) return FALSE;

    cPrintf(" Receiving room request file.");
    doccr();
    
    wxrcv(cfg.temppath, "roomreq.tmp", 
            (char)strpos((char)tolower(node.ndprotocol[0]), extrncmd));
    if (!gotCarrier()) return FALSE;
    sprintf(line, "%s\\roomreq.tmp", cfg.temppath);
    file = fopen(line, "rb");
    if (!file)
    {
        perror("Error opening roomreq.tmp");
        return FALSE;
    }

    doccr();
    cPrintf(" Fetching:");
    doccr();

    GetStr(file, troo, 30);
    while(strlen(troo) && !feof(file))
    {
        KBReady();
        if ((rm = roomExists(troo)) != ERROR)
        {
            if (netcanseeroom(rm))
            {
                sprintf(fn, "room.%d", i);
                cPrintf(" %-20s  ", troo);
                if( !((i+1) % 3) ) doccr();
                NewRoom(rm, fn);
            }
            else
            {
                doccr();
                cPrintf(" No access to %s room.", troo);
                doccr();
                amPrintf(" '%s' room not available to remote.\n", troo);
                netError = TRUE;
            }
        }else{
            doccr();
            cPrintf(" No %s room on system.", troo);
            doccr();
            amPrintf(" '%s' room not found for remote.\n", troo);
            netError = TRUE;
        }

        i++;
        GetStr(file, troo, 30);
    }
    doccr();
    fclose(file);
    unlink(line);

    cPrintf(" Sending message files.");
    doccr();
  
    if (!gotCarrier()) return FALSE;
    wxsnd(cfg.temppath, "room.*",
          (char)strpos((char)tolower(node.ndprotocol[0]), extrncmd));

#if 1
    changedir(cfg.temppath);
#endif
    ambigUnlink("room.*",   FALSE);

    return TRUE;
}

#endif
