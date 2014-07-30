/* -------------------------------------------------------------------- */
/*  NETROUTE.C                    ACit                         91Sep30  */
/*      Networking libs for the Citadel bulletin board system           */
/* -------------------------------------------------------------------- */

#include <string.h>
#include "ctdl.h"
#include "prot.h"
#include "glob.h"

#ifdef NETWORK
/* -------------------------------------------------------------------- */
/*                              Contents                                */
/*  alias()         return the name of the BBS from the #ALIAS          */
/*  route()         return the routing of a BBS from the #ROUTE         */
/* $alias_route()   returns the route or alias specified                */
/*  save_mail()     save a message bound for another system             */
/* -------------------------------------------------------------------- */

static BOOL alias_route(char *str, const char *srch);

/* -------------------------------------------------------------------- */
/*  alias()         return the name of the BBS from the #ALIAS          */
/* -------------------------------------------------------------------- */
BOOL alias(char *str)
{
    return alias_route(str, "#ALIAS");
}

/* -------------------------------------------------------------------- */
/*  route()         return the routing of a BBS from the #ROUTE         */
/* -------------------------------------------------------------------- */
BOOL route(char *str)
{
    return alias_route(str, "#ROUTE");
}

/* -------------------------------------------------------------------- */
/*  alias_route()   returns the route or alias specified                */
/* -------------------------------------------------------------------- */
static BOOL alias_route(char *str, const char *srch)
{                          
    FILE *fBuf;
    char line[90];
    char *words[256];
    char path[80];

    sprintf(path, "%s\\route.cit", cfg.homepath);
    
    if ((fBuf = fopen(path, "r")) == NULL)  /* ASCII mode */
    {  
        crashout("Can't find route.cit!");
    }

    while (fgets(line, 90, fBuf) != NULL)
    {
        if (line[0] != '#') continue;
   
        if (strnicmp(line, srch, 5) != SAMESTRING) continue;
     
        parse_it( words, line);

        if (strcmpi(srch, words[0]) == SAMESTRING)
        {
            if (strcmpi(str, words[1]) == SAMESTRING)
            {
                fclose(fBuf);
                strcpy(str, words[2]);
                return TRUE;
            }
        }
    }
    fclose(fBuf);
    return FALSE;
}

/* -------------------------------------------------------------------- */
/*  save_mail()     save a message bound for another system             */
/* -------------------------------------------------------------------- */
BOOL save_mail(void)
{
    label tosystem;
    char  filename[100];
    FILE *fl;

    /* where are we sending it? */
    strcpy(tosystem, msgBuf->mbzip);

    /* send it vila... */
    route(tosystem);

    /* get the node entery */
    if (!getnode(tosystem))
        return FALSE;
  
    sprintf(filename, "%s\\%s", cfg.transpath, node.ndmailtmp);

    fl = fopen(filename, "ab");
    if (!fl) return FALSE;

    PutMessage(fl);

    fclose(fl);

    return TRUE;
}

#endif
