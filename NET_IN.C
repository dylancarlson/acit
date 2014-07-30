/* -------------------------------------------------------------------- */
/*  NET_IN.C                      ACit                         91Sep27  */
/*      Networking libs for the Citadel bulletin board system           */
/* -------------------------------------------------------------------- */

#include "ctdl.h"
#include "prot.h"
#include "glob.h"

#ifdef NETWORK
/* -------------------------------------------------------------------- */
/*                              Contents                                */
/*  net_slave()     network entry point from LOGIN                      */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  net_slave()     network entry point from LOGIN                      */
/* -------------------------------------------------------------------- */
BOOL net_slave(void)
{
    if (!readnode())
    {
        cPrintf("\n No nodes.cit entry!\n ");
        return FALSE;
    }

    if (whichIO == CONSOLE) 
    {
        if (debug)
        {
          cPrintf("\nNode:  \"%s\" \"%s\"\n", node.ndname, node.ndregion);  
          cPrintf("Phone: \"%s\"   Timeout: %d\n", node.ndphone, node.nddialto);     
          cPrintf("Login: \"%s\"\n", node.ndlogin);     
          cPrintf("Baud:  %-4d    Protocol: \"%s\"\n", bauds[node.ndbaud],
                                                   node.ndprotocol);
          cPrintf("Expire:%d    Waitout:  %d", node.ndexpire, node.ndwaitto);
          doccr();
        }
    }
    else
    {
        netError = FALSE;
        
        /* cleanup */
        changedir(cfg.temppath);
        ambigUnlink("room.*",   FALSE);
        ambigUnlink("roomin.*", FALSE);
        
        if (slave())
        {
            if (master())
            {
                cleanup();
                did_net(node.ndname);
                return TRUE;
            }
        }
    }
    return FALSE;
}

#endif
