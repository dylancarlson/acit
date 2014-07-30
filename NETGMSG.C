/* -------------------------------------------------------------------- */
/*  NETGMSG.C                     ACit                         91Sep30  */
/*      Networking libs for the Citadel bulletin board system           */
/* -------------------------------------------------------------------- */

#include <dos.h>
#include <string.h>
#include <time.h>
#include "ctdl.h"
#include "key.h"
#include "prot.h"
#include "glob.h"

#ifdef NETWORK
/* -------------------------------------------------------------------- */
/*                              Contents                                */
/*  cleanup()       Done with other system, save mail and messages      */
/* $get_first_room() get the first room in the room list                */
/* $get_next_room() gets the next room in the list                      */
/* $GetMessage()    Gets a message from a file, returns sucess          */
/*  GetStr()        gets a null-terminated string from a file           */
/*  master()        During network master code                          */
/* $NfindRoom()     find the room for main (unimplmented, ret: MAILROOM)*/
/* $ReadMsgFl()     Reads a message file into thisRoom                  */
/* -------------------------------------------------------------------- */

static BOOL get_first_room(char *here, char *there);
static BOOL get_next_room(char *here, char *there);
static BOOL GetMessage(FILE *fl);
static int NfindRoom(const char *str);
static int ReadMsgFl(int room, const char *filename,
                     const char *here, const char *there);


/* -------------------------------------------------------------------- */
/*  cleanup()       Done with other system, save mail and messages      */
/* -------------------------------------------------------------------- */
void cleanup(void)
{
    int t, i, rm, err;
    int new=0, exp=0, dup=0, rms=0;
    label fn, here, there;
    char line[100];
  
    sprintf(line, "%s\\%s", cfg.transpath, node.ndmailtmp);
    unlink(line);

    drop_dtr();

    outFlag = IMPERVIOUS;

    doccr();
    cPrintf(" Incorporating:");
    doccr();
    cPrintf("                 Room:  New: Expired: Duplicate:");
    doccr();/* XXXXXXXXXXXXXXXXXXXX    XX     XX     XX*/
    cPrintf("อออออออออออออออออออออออออออออออออออออออออออออออออ");
    doccr();
    for (t=get_first_room(here, there), i=0;
         t;
         t=get_next_room(here, there), i++)
    {
        sprintf(fn, "roomin.%d", i);

        rm = roomExists(here);
        if (rm != ERROR)
        {
            cPrintf(" %20.20s   ", here);
            err = ReadMsgFl(rm, fn, here, there);
            if (err != ERROR)
            {
                if (err)          ansiattr = cfg.cattr;
                cPrintf("%3d", err);
                                  ansiattr = cfg.attr;
                cPrintf("    ");
                if (expired)      ansiattr = cfg.cattr;
                cPrintf("%3d", expired);
                                  ansiattr = cfg.attr;
                cPrintf("    ");
                if (duplicate)    ansiattr = cfg.cattr;
                cPrintf("%3d", duplicate);
                                  ansiattr = cfg.attr;
                new += err;
                exp += expired;
                dup += duplicate;
                rms ++;
            }else{
                amPrintf(" No '%s' room on remote\n", there);
                netError = TRUE;
                cPrintf(" Room not found on other system.");
            }
            doccr();
        }else{
            cPrintf(" %20.20s   Room not found on local system.", here);
            amPrintf(" No '%s' room local.\n", here);
            netError = TRUE;
            doccr();
        }

        unlink(fn);
    }
    cPrintf("อออออออออออออออออออออออออออออออออออออออออออออออออ");
    doccr();
    cPrintf("Totals:           ");
    if (rms)     ansiattr = cfg.cattr;
    cPrintf("%3d", rms);
                 ansiattr = cfg.attr;
    cPrintf("   ");
    if (new)     ansiattr = cfg.cattr;
    cPrintf("%3d", new);
                 ansiattr = cfg.attr;
    cPrintf("    ");
    if (exp)     ansiattr = cfg.cattr;
    cPrintf("%3d", exp);
                 ansiattr = cfg.attr;
    cPrintf("    ");
    if (dup)     ansiattr = cfg.cattr;
    cPrintf("%3d", dup);
                 ansiattr = cfg.attr;
           /* XXXXXXXXXXXXXXXXXXXX    XX     XX     XX*/
    doccr();
    cPrintf("                (Rooms)(New)  (Exp)  (Dup)");
    doccr();
    doccr();
    cPrintf("Incorporating MAIL");
    i =  ReadMsgFl(MAILROOM, "mesg.tmp", "", "");
    cPrintf(" %d new message(s)", i == ERROR ? 0 : i);
    doccr();
  
    changedir(cfg.temppath);
    ambigUnlink("room.*",   FALSE);
    ambigUnlink("roomin.*", FALSE);
    unlink("mesg.tmp");
    changedir(cfg.homepath);
  
    xpd = exp;
    duplic = dup;
    entered = new;
  
    if (netError)
    {
        amPrintf(" \n Netting with '%s'\n", logBuf.lbname );
        SaveAideMess();
    }
}

/* -------------------------------------------------------------------- */
/*  the following two routines are used for scanning through the rooms  */
/*  requested from a node                                               */
/* -------------------------------------------------------------------- */
FILE *nodefile;

/* -------------------------------------------------------------------- */
/*  get_first_room()    get the first room in the room list             */
/* -------------------------------------------------------------------- */
static BOOL get_first_room(char *here, char *there)
{
    if (!node.roomoff) return FALSE;

    /* move to home-path */
    changedir(cfg.homepath);

    if ((nodefile = fopen("nodes.cit", "r")) == NULL)  /* ASCII mode */
    {  
        crashout("Can't find nodes.cit!");
    }

    changedir(cfg.temppath);

    fseek(nodefile, node.roomoff, SEEK_SET);

    return get_next_room(here, there);
}

/* -------------------------------------------------------------------- */
/*  get_next_room() gets the next room in the list                      */
/* -------------------------------------------------------------------- */
static BOOL get_next_room(char *here, char *there)
{
    char  line[90];
    char *words[256];
   
    while (fgets(line, 90, nodefile) != NULL)
    {
        if (line[0] != '#')  continue;

        parse_it( words, line);

        if (strcmpi(words[0], "#NODE") == SAMESTRING)
        {
            fclose(nodefile);
            return FALSE;
        }

        if (strcmpi(words[0], "#ROOM") == SAMESTRING)
        {
            strncpy(here,  words[1], LABELSIZE);
            strncpy(there, words[2], LABELSIZE);
            here[LABELSIZE]  = '\0';
            there[LABELSIZE] = '\0';
            if (!strlen(there))
                strcpy(there, here);
            return TRUE;
        }
    }
    fclose(nodefile);
    return FALSE;
}  

/* -------------------------------------------------------------------- */
/*  GetMessage()    Gets a message from a file, returns sucess          */
/* -------------------------------------------------------------------- */
static BOOL GetMessage(FILE *fl)
{
    char c;

    /* clear message buffer out */
    clearmsgbuf();

    /* find start of message */
    do
    {
        c = (uchar)fgetc(fl);
    } while (c != -1 && !feof(fl));

    if (feof(fl))
        return FALSE;

    /* get message's attribute byte */
    msgBuf->mbattr = (uchar)fgetc(fl);

    GetStr(fl, msgBuf->mbId, LABELSIZE);

    do 
    {
        c = (uchar)fgetc(fl);
        switch (c)
        {
        case 'A':     GetStr(fl, msgBuf->mbauth,  LABELSIZE);    break;
        case 'D':     GetStr(fl, msgBuf->mbtime,  LABELSIZE);    break;
        case 'F':     GetStr(fl, msgBuf->mbfwd,   LABELSIZE);    break;
        case 'G':     GetStr(fl, msgBuf->mbgroup, LABELSIZE);    break;
        case 'I':     GetStr(fl, msgBuf->mbreply, LABELSIZE);    break;
        case 'M':     /* will be read off disk later */         break;
        case 'N':     GetStr(fl, msgBuf->mbtitle, LABELSIZE);    break;
        case 'n':     GetStr(fl, msgBuf->mbsur,   LABELSIZE);    break;
        case 'O':     GetStr(fl, msgBuf->mboname, LABELSIZE);    break;
        case 'o':     GetStr(fl, msgBuf->mboreg,  LABELSIZE);    break;
        case 'P':     GetStr(fl, msgBuf->mbfpath, 256     );    break;
        case 'p':     GetStr(fl, msgBuf->mbtpath, 128     );    break;
        case 'Q':     GetStr(fl, msgBuf->mbocont, LABELSIZE);    break;
        case 'q':     GetStr(fl, msgBuf->mbczip,  LABELSIZE);    break;
        case 'R':     GetStr(fl, msgBuf->mbroom,  LABELSIZE);    break;
        case 'S':     GetStr(fl, msgBuf->mbsrcId, LABELSIZE);    break;
        case 'T':     GetStr(fl, msgBuf->mbto,    LABELSIZE);    break;
/*      case 'X':     GetStr(fl, msgBuf->mbx,     LABELSIZE);    break; */
        case 'Z':     GetStr(fl, msgBuf->mbzip,   LABELSIZE);    break;
        case 'z':     GetStr(fl, msgBuf->mbrzip,  LABELSIZE);    break;
#if 1
        case 'J':     GetStr(fl, msgBuf->mbcreg,  LABELSIZE);    break;
        case 'j':     GetStr(fl, msgBuf->mbccont, LABELSIZE);    break;
        case '.':     GetStr(fl, msgBuf->mbsig,   80       );    break;
        case 'B':     GetStr(fl, msgBuf->mbsubj,  72       );    break;
        case 's':     GetStr(fl, msgBuf->mbsw,    LABELSIZE);    break;
        case '_':     GetStr(fl, msgBuf->mbusig,  80       );    break;
#endif

        default:
            GetStr(fl, msgBuf->mbtext, cfg.maxtext);  /* discard unknown field  */
            msgBuf->mbtext[0]    = '\0';
            break;
        }
    } while (c != 'M' && !feof(fl));

    if (!*msgBuf->mboname)
    {
        strcpy(msgBuf->mboname, node.ndname);
    }

    if (!*msgBuf->mboreg)
    {
        strcpy(msgBuf->mboreg, node.ndregion);
    }

#if 1
    if ((strstr(msgBuf->mbocont, "@TLC") != NULL))
        *msgBuf->mbocont = '\0';
#endif

    if (!*msgBuf->mbsrcId)
    {
        strcpy(msgBuf->mbsrcId, msgBuf->mbId);
    }

    /*
     * If the other node did not set up a from path, do it.
     */
    if (!*msgBuf->mbfpath)
    {
        if (strcmpi(msgBuf->mboname, node.ndname) == 0)
        {
            strcpy(msgBuf->mbfpath, msgBuf->mboname);
        }
        else
        {
            /* last node did not originate, make due with what we got... */
            strcpy(msgBuf->mbfpath, msgBuf->mboname);
            strcat(msgBuf->mbfpath, "!..!");
            strcat(msgBuf->mbfpath, node.ndname);
        }
    }

    if (feof(fl))
    {
        return FALSE;
    }

    GetStr(fl, msgBuf->mbtext, cfg.maxtext);  /* get the message field  */

    return TRUE;
}

/* -------------------------------------------------------------------- */
/*  GetStr()        gets a null-terminated string from a file           */
/* -------------------------------------------------------------------- */
void GetStr(FILE *fl, char *str, int mlen)
{
    int l;
    char ch;
  
    l=0; ch=1;
    while(!feof(fl) && ch)
    {
        ch=(uchar)fgetc(fl);
        if (ch != '\r' && ch != '\xFF' && l < mlen)
        {
            str[l]=ch;
            l++;
        }
    }
    str[l]='\0';
}

/* -------------------------------------------------------------------- */
/*  master()        During network master code                          */
/* -------------------------------------------------------------------- */
BOOL master(void)
{
    char line[75], line2[75];
    label here, there;
    FILE *file, *fopen();
    int i, rms;
    time_t t;

    if (!gotCarrier()) return FALSE;

    sprintf(line, "%s\\mesg.tmp", cfg.temppath);
    unlink(line);

    cPrintf(" Receiving mail file.");
    doccr();

    wxrcv(cfg.temppath, "mesg.tmp", 
          (char)strpos((char)tolower(node.ndprotocol[0]), extrncmd));

    if (!gotCarrier()) return FALSE;

    sprintf(line, "%s\\roomreq.tmp", cfg.temppath);
    unlink(line);

    if((file = fopen(line, "ab")) == NULL)
    {
        perror("Error opening roomreq.tmp");
        return FALSE;
    }

    for (i=get_first_room(here, there), rms=0;
         i;
         i=get_next_room(here, there), rms++)
    {
        PutStr(file, there);
    }

    PutStr(file, "");
    fclose(file);

    cPrintf(" Sending room request file.");
    doccr();

    wxsnd(cfg.temppath, "roomreq.tmp", 
         (char)strpos((char)tolower(node.ndprotocol[0]), extrncmd));
    unlink(line);

    if (!gotCarrier()) return FALSE;

    /* clear the buffer */
    while (gotCarrier() && MIReady())
    {
        getMod();
    }

    cPrintf(" Waiting for transfer.");
    /* wait for them to get their shit together */
    t = time(NULL);
    while (gotCarrier() && !MIReady())
    {
        KBReady();
        if (time(NULL) > (t + (15 * 60))) /* only wait 15 minutes */
        {
            drop_dtr();
        }
    }
    doccr();

    if (!gotCarrier()) return FALSE;

    cPrintf(" Receiving message files.");
    doccr();

    wxrcv(cfg.temppath, "", 
         (char)strpos((char)tolower(node.ndprotocol[0]), extrncmd));

#if 1
    changedir(cfg.temppath);
#endif
    for (i=0; i<rms; i++)
    {
        sprintf(line,  "room.%d",   i);
        sprintf(line2, "roomin.%d", i);
        rename(line, line2);
    }

    return TRUE;
}

/* -------------------------------------------------------------------- */
/*  NfindRoom()     find the room for main (unimplmented, ret: MAILROOM)*/
/* -------------------------------------------------------------------- */
static int NfindRoom(const char *str)
{
    int i = MAILROOM; 

    i = roomExists(str);

    if (i == ERROR)
        i = MAILROOM;

    return(i);
}

/* -------------------------------------------------------------------- */
/*  ReadMsgFl()   Reads a message file into thisRoom                    */
/* -------------------------------------------------------------------- */
static int ReadMsgFl(int room, const char *filename,
                     const char *here, const char *there)
{
    FILE *file;
    char str[100];
    ulong oid, loc;
    long l;
    int i, bad, oname, temproom, lp, goodmsg = 0;

    expired = 0;   duplicate = 0;

    sprintf(str, "%s\\%s", cfg.temppath, filename);

    file = fopen(str, "rb");

    if (!file)
        return -1;

    while(GetMessage(file) == TRUE)
    {
        msgBuf->mbroomno = (uchar)room;

        sscanf(msgBuf->mbsrcId, "%ld", &oid);
        oname = hash(msgBuf->mboname);

        memcpy( msgBuf2, msgBuf, sizeof(struct msgB) );

        bad = FALSE;

        if (strcmpi(cfg.nodeTitle, msgBuf->mboname) == SAMESTRING)
        { 
            bad = TRUE; 
            duplicate++; 
        }

        if (*msgBuf->mbzip) /* is mail */
        {
            /* not for this system */
            if (strcmpi(msgBuf->mbzip, cfg.nodeTitle) != SAMESTRING)
            {
                if (!save_mail())
                {
                    clearmsgbuf();
                    strcpy(msgBuf->mbauth, cfg.nodeTitle);
                    strcpy(msgBuf->mbto,   msgBuf2->mbauth);
                    strcpy(msgBuf->mbzip,  msgBuf2->mboname);
                    strcpy(msgBuf->mbrzip, msgBuf2->mboreg);
                    strcpy(msgBuf->mbroom, msgBuf2->mbroom);
                    sprintf(msgBuf->mbtext, 
                           " \n Can not find route to '%s'.", msgBuf2->mbzip);
                    amPrintf(" Can not find route to '%s'"
                             " in message from '%s @ %s'.\n",
                        msgBuf2->mbzip, msgBuf2->mbauth, msgBuf2->mboname);
                    netError = TRUE;
                
                    save_mail();
                }
                bad = TRUE;
            } else {
            /* for this system */
                if (*msgBuf->mbto && personexists(msgBuf->mbto) == ERROR
                    && strcmpi(msgBuf->mbto, "Sysop") != SAMESTRING)
                {
                    clearmsgbuf();
                    strcpy(msgBuf->mbauth, cfg.nodeTitle);
                    strcpy(msgBuf->mbto,   msgBuf2->mbauth);
                    strcpy(msgBuf->mbzip,  msgBuf2->mboname);
                    strcpy(msgBuf->mbrzip, msgBuf2->mboreg);
                    strcpy(msgBuf->mbroom, msgBuf2->mbroom);
                    sprintf(msgBuf->mbtext, 
                        " \n No '%s' user found on %s.", msgBuf2->mbto,
                        cfg.nodeTitle);
                    save_mail();
                    bad = TRUE;
                }
            }
        } else {
            /* is public */
            if (!bad)
            {
                for (i = sizetable(); i != -1 && !bad; i--)
                {
                    if (msgTab9[i].mtorigin == oname
                       && oid == (unsigned long)msgTab8[i].mtomesg)
                    {
                        loc = msgTab2[i].mtmsgLoc;
                        fseek(msgfl, loc, 0);
                        getMessage();
                        if (strcmpi(msgBuf->mbauth, msgBuf2->mbauth)   
                                                                == SAMESTRING
                         && strcmpi(msgBuf->mboname, msgBuf2->mboname)
                                                                == SAMESTRING
                         && strcmpi(msgBuf->mbtime, msgBuf2->mbtime)  
                                                                == SAMESTRING
                       /*&& strcmpi(msgBuf->mboreg, msgBuf2->mboreg)  
                                                                == SAMESTRING*/
                       /* Changed beacuse of region name changes */
                           )
                        {
                            bad = TRUE; 
                            duplicate++; 
                        }
                    }
                }
            }

            memcpy( msgBuf, msgBuf2, sizeof(struct msgB) );
    
            /* fix group only messages, or discard them! */
            if (*msgBuf->mbgroup && !bad)
            {
                bad = TRUE;
                for (i=0; node.ndgroups[i].here[0]; i++)
                {
                    if (strcmpi(node.ndgroups[i].there, msgBuf->mbgroup) 
                        == SAMESTRING)
                    {
                        strcpy(msgBuf->mbgroup, node.ndgroups[i].here);
                        bad = FALSE;
                    }
                }
                /* put it in RESERVED_2 */
                if (bad)
                {
                    bad = FALSE;
                    strcpy(msgBuf->mbgroup, grpBuf.group[1].groupname);
                }
            }
    
            /* Expired? */
            if (( atol(msgBuf2->mbtime) 
                < (time(&l) - ((long)node.ndexpire *60*60*24)) ) 
                  || ( atol(msgBuf2->mbtime) 
                > (time(&l) + ((long)node.ndexpire *60*60*24)) ))
            {
                bad = TRUE;
                expired++;
            }
        }

        if (!bad)
        { /* its good, save it */
            temproom = room;

            if (strcmpi(msgBuf->mbroom, there) == SAMESTRING)
                strcpy(msgBuf->mbroom, here);

            if (*msgBuf->mbto)
                temproom = NfindRoom(msgBuf->mbroom);

            msgBuf->mbroomno = (uchar)temproom;

            putMessage();
            noteMessage();
            goodmsg++;

            if (*msgBuf->mbto)
            {
                lp = thisRoom;
                thisRoom = temproom;
                /* notelogmessage(msgBuf->mbto); */
                thisRoom = lp;
            }
        }
    }
    fclose(file);

    return goodmsg;
}

#endif
