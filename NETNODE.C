/* -------------------------------------------------------------------- */
/*  NETNODE.C                     ACit                         91Sep27  */
/*      Networking libs for the Citadel bulletin board system           */
/* -------------------------------------------------------------------- */

#include <dos.h>
#include <string.h>
#include "ctdl.h"
#include "key.h"
#include "prot.h"
#include "glob.h"

#ifdef NETWORK
/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/*  getnode()       read the nodes.cit to get the nodes info            */
/*  readnode()      read the nodes.cit to get the nodes info for logbuf */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  getnode()       read the nodes.cit to get the nodes info            */
/* -------------------------------------------------------------------- */
BOOL getnode(const char *nodename)
{
    FILE *fBuf;
    char line[90], ltmp[90];
    char *words[256];
    int  i, j, k, found = FALSE;
    long pos;
    char path[80];
    char *toomany = "%s over %d characters (%s)\n";

    memset(&node, 0, sizeof(struct nodest));
    
    sprintf(path, "%s\\nodes.cit", cfg.homepath);

    if ((fBuf = fopen(path, "r")) == NULL)  /* ASCII mode */
    {  
        cPrintf("Can't find nodes.cit!"); doccr();
        return FALSE;
    }

    pos = ftell(fBuf);
    while (fgets(line, 90, fBuf) != NULL)
    {
        if (line[0] != '#')
        {
            pos = ftell(fBuf);
            continue;
        }

        if (!found && strnicmp(line, "#NODE", 5) != SAMESTRING)
        {
            pos = ftell(fBuf);
            continue;
        }
        
        strcpy(ltmp, line);
        parse_it( words, line);

        for (i = 0; nodekeywords[i] != NULL; i++)
        {
            if (strcmpi(words[0], nodekeywords[i]) == SAMESTRING)
            {
                break;
            }
        }

        if (i == NOK_NODE)
        {
            if (found)
            {
                fclose(fBuf);
                return TRUE;
            }

            if (strcmpi(nodename,  words[1]) == SAMESTRING)
            {
                found = TRUE;  
            }
            else
            {
                continue;
            }
        }     

        switch(i)
        {
        case NOK_BAUD:
            j = atoi(words[1]);
            switch(j) /* icky hack */
            {
            case 300:
                node.ndbaud = 0;
                break;
            case 1200:
                node.ndbaud = 1;
                break;
            case 2400:
                node.ndbaud = 2;
                break;
            case 4800:
                node.ndbaud = 3;
                break;
            case 9600:
                node.ndbaud = 4;
                break;
            default:
                node.ndbaud = 1;
                break;
            }
            break;

        case NOK_PHONE:
            if (strlen(words[1]) <= LABELSIZE)
                strcpy(node.ndphone, words[1]);
            else
                cPrintf(toomany, nodekeywords[i], LABELSIZE, words[1]);
            break;

        case NOK_PROTOCOL:
            if (strlen(words[1]) <= LABELSIZE)
                strcpy(node.ndprotocol, words[1]);
            else
                cPrintf(toomany, nodekeywords[i], LABELSIZE, words[1]);
            break;

        case NOK_MAIL_TMP:
            if (strlen(words[1]) <= LABELSIZE)
                strcpy(node.ndmailtmp, words[1]);
            else
                cPrintf(toomany, nodekeywords[i], LABELSIZE, words[1]);
            break;

        case NOK_LOGIN:
            strcpy(node.ndlogin, ltmp);
            break;

        case NOK_NODE:
            if (strlen(words[1]) <= LABELSIZE)
                strcpy(node.ndname, words[1]);
            else
                cPrintf(toomany, nodekeywords[i], LABELSIZE, words[1]);
            if (strlen(words[2]) <= LABELSIZE)
                strcpy(node.ndregion, words[2]);
            else
                cPrintf(toomany, nodekeywords[i], LABELSIZE, words[2]);
            for (j=0; j<MAXGROUPS; j++)
                node.ndgroups[j].here[0] = '\0';
            node.roomoff = 0L;
            break;

        case NOK_REDIAL:
            node.ndredial = atoi(words[1]);
            break;

        case NOK_DIAL_TIMEOUT:
            node.nddialto = atoi(words[1]);
            break;

        case NOK_WAIT_TIMEOUT:
            node.ndwaitto = atoi(words[1]);
            break;

        case NOK_EXPIRE:
            node.ndexpire = atoi(words[1]);
            break;

        case NOK_ROOM:
            node.roomoff = pos;
            fclose(fBuf);
            return TRUE;

        case NOK_GROUP:
            for (j = 0, k = ERROR; j < MAXGROUPS; j++)
            {
                if (node.ndgroups[j].here[0] == '\0') 
                {
                    k = j;
                    j = MAXGROUPS;
                }
            } 

            if (k == ERROR) 
            {
                cPrintf("Too many groups!!\n ");
               break;
            }
            
            if (strlen(words[1]) <= LABELSIZE)
                strcpy(node.ndgroups[k].here,  words[1]);
            else
                cPrintf(toomany, "local group name", LABELSIZE, words[1]);

            if (strlen(words[2]) <= LABELSIZE)
                strcpy(node.ndgroups[k].there, words[2]);
            else
                cPrintf(toomany, "remote group name", LABELSIZE, words[2]);
            if (!strlen(words[2]))
                strcpy(node.ndgroups[k].there, words[1]);
            break;

        default:
            cPrintf("Nodes.cit - Warning: Unknown variable %s", words[0]);
            doccr();
            break;
        }
        pos = ftell(fBuf);
    }
    fclose(fBuf);
    return (BOOL)(found);
}

/* -------------------------------------------------------------------- */
/*  readnode()      read the nodes.cit to get the nodes info for logbuf */
/* -------------------------------------------------------------------- */
BOOL readnode(void)
{
    return getnode(logBuf.lbname);
}

#endif
