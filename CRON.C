/* -------------------------------------------------------------------- */
/*  CRON.C                        ACit                         91Sep19  */
/* -------------------------------------------------------------------- */
/*  This file contains all the code to deal with the cron events        */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Includes                                                            */
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
/*                                                                      */
/*  readcron()      reads cron.cit values into events structure         */
/*  do_cron()       called when the system is ready to do an event      */
/* $cando_event()   Can we do this event?                               */
/* $do_event()      Actualy do this event                               */
/* $list_event()    List all events                                     */
/*  cron_commands() Sysop Fn: Cron commands                             */
/* $zap_event()     Zap an event out of the cron list                   */
/* $reset_event()   Reset an even so that it has not been done          */
/* $done_event()    Set event so it seems to have been done             */
/*  did_net()       Set all events for a node to done                   */
/* $force_event()   Force an event                                      */
/* $do_cmd_event()  Do a "COMMAND" event                                */
/* $next_event_set() Set the next event pointer.                        */
/*                                                                      */
/* -------------------------------------------------------------------- */

static int  cando_event(int evnt);
static void  do_event(int  evnt);
static void  list_event(void);
static void  zap_event(void);
static void  reset_event(void);
static void  done_event(void);
static void  force_event(void);
static void do_cmd_event(char *str);
static void  next_event_set(void);

/* -------------------------------------------------------------------- */
/*  Static Data                                                         */
/* -------------------------------------------------------------------- */
static struct event events[MAXCRON];
static int on_event = 0;
static int numevents= 0;

/* -------------------------------------------------------------------- */
/*      readcron()     reads cron.cit values into events structure      */
/* -------------------------------------------------------------------- */
void readcron(void)
{                          
    FILE *fBuf;
    char line[90];
    char *words[256];
    int  i, j, k, l, count;
    int cronslot = ERROR;
    int hour;

   
    /* move to home-path */
    changedir(cfg.homepath);

    if ((fBuf = fopen("cron.cit", "r")) == NULL)  /* ASCII mode */
    {  
        cPrintf("Can't find Cron.cit!"); doccr();
        exit(1);
    }

    while (fgets(line, 90, fBuf) != NULL)
    {
        if (line[0] != '#')  continue;

        count = parse_it( words, line);

        for (i = 0; cronkeywords[i] != NULL; i++)
        {
            if (strcmpi(words[0], cronkeywords[i]) == SAMESTRING)
            {
                break;
            }
        }

        switch(i)
        {
        case CR_DAYS:              
            if (cronslot == ERROR)  break;

            /* init days */
            for ( j = 0; j < 7; j++ )
               events[cronslot].e_days[j] = 0;

            for (j = 1; j < count; j++)
            {
                for (k = 0; daykeywords[k] != NULL; k++)
                {
                    if (strcmpi(words[j], daykeywords[k]) == SAMESTRING)
                    {
                        break;
                    }
                }
                if (k < 7)
                    events[cronslot].e_days[k] = TRUE;
                else if (k == 7)  /* any */
                {
                    for ( l = 0; l < MAXGROUPS; ++l)
                        events[cronslot].e_days[l] = TRUE;
                }
                else
                {
                    doccr();
                    cPrintf("Cron.cit - Warning: Unknown day %s ", words[j]);
                    doccr();
                }
            }
            break;

        case CR_DO:
            cronslot = (cronslot == ERROR) ? 0 : (cronslot + 1);

            if (cronslot > MAXCRON)
            {
                doccr();
                illegal("Cron.Cit - too many entries");
            } 

            for (k = 0; crontypes[k] != NULL; k++)
            {
              if (strcmpi(words[1], crontypes[k]) == SAMESTRING)
                events[cronslot].e_type = (uchar)k;
            }
               
            strcpy(events[cronslot].e_str, words[2]);
            events[cronslot].l_sucess  = (long)0;
            events[cronslot].l_try     = (long)0;
            break;
            
        case CR_HOURS:             
            if (cronslot == ERROR)  break;

            /* init hours */
            for ( j = 0; j < 24; j++ )
                events[cronslot].e_hours[j]   = 0;

            for (j = 1; j < count; j++)
            {
                if (strcmpi(words[j], "Any") == SAMESTRING)
                {
                    for (l = 0; l < 24; ++l)
                        events[cronslot].e_hours[l] = TRUE;
                }
                else
                {
                    hour = atoi(words[j]);

                    if ( hour > 23 ) 
                    {
                        doccr();
                        cPrintf("Cron.Cit - Warning: Invalid hour %d ",
                        hour);
                        doccr();
                    }
                    else
                   events[cronslot].e_hours[hour] = TRUE;
                }
            }
            break;

        case CR_REDO:
            if (cronslot == ERROR)  break;
            
            events[cronslot].e_redo = atoi(words[1]);
            break;

        case CR_RETRY:
            if (cronslot == ERROR)  break;
            
            events[cronslot].e_retry = atoi(words[1]);
            break;

        default:
            cPrintf("Cron.cit - Warning: Unknown variable %s", words[0]);
            doccr();
            break;
        }
    }
    fclose(fBuf);

    numevents = cronslot;

    for (i=0; i<MAXCRON; i++)
    {
        events[i].l_sucess  = (long)0;
        events[i].l_try     = (long)0;
    }
}

/* -------------------------------------------------------------------- */
/*  do_cron()       called when the system is ready to do an event      */
/* -------------------------------------------------------------------- */
int do_cron(int why_called)
{
    int was_event, done;

    why_called = why_called; /* to prevent a -W3 warning, the varible will
                              be used latter */
    was_event = on_event;
    done = FALSE;

    do{
        if (cando_event(on_event))
        {
            do_event(on_event);
            done = TRUE;  
        }

        on_event = on_event > numevents ? 0 : on_event + 1;
    }while(!done && on_event != was_event);  

    if (!done)
    {
        if (debug) mPrintf("No Job> ");
        Initport();
        return FALSE;
    }

    doCR();
    return TRUE;  
}

/* -------------------------------------------------------------------- */
/*  cando_event()   Can we do this event?                               */
/* -------------------------------------------------------------------- */
int cando_event(int evnt)
{
    long l, time();

    /* not a valid (posible zaped) event */
    if (events[evnt].e_type == ERROR)
        return FALSE;

    /* not right time || day */
    if (!events[evnt].e_hours[hour()] || !events[evnt].e_days[dayofweek()])
        return FALSE;

    /* already done, wait a little longer */
    if ((time(&l) - events[evnt].l_sucess)/(long)60 < (long)events[evnt].e_redo)
        return FALSE;  
    
    /* didnt work, give it time to get un-busy */
    if ((time(&l) - events[evnt].l_try)/(long)60 < (long)events[evnt].e_retry)
        return FALSE;  

    return TRUE;
}

/* -------------------------------------------------------------------- */
/*  do_event()      Actualy do this event                               */
/* -------------------------------------------------------------------- */
static void do_event(int evnt)
{
    long time(), l;

    switch(events[evnt].e_type)
    {
    case CR_SHELL_1:
        mPrintf("SHELL: \"%s\"", events[evnt].e_str);
        if (changedir(cfg.aplpath) == ERROR)
        {
            mPrintf("  -- Can't find application directory.\n\n");
            changedir(cfg.homepath);
            return;
        }
        apsystem(events[evnt].e_str);
        changedir(cfg.homepath);
        events[evnt].l_sucess = time(&l);
        events[evnt].l_try    = time(&l);
        Initport();
        break;

#ifdef NETWORK
    case CR_NET:
        if (!loggedIn)
        {
            mPrintf("NETWORK: with \"%s\"", events[evnt].e_str);
            if (net_callout(events[evnt].e_str))
                did_net(events[evnt].e_str);
            events[evnt].l_try       = time(&l);
        }
        else 
        {
            mPrintf("Can't network with a user logged in\n ");
        }
        break;
#endif

#if 1
     case CR_COMMAND:
        mPrintf("COMMAND: %s", events[evnt].e_str);
        do_cmd_event(events[evnt].e_str);
        events[evnt].l_sucess = time(&l);
        events[evnt].l_try    = time(&l);
        Initport();
        break;
#endif

    default:
        mPrintf(" Unknown event type %d, slot %d\n ", events[evnt].e_type, evnt);
        break;  
    }
}

/* -------------------------------------------------------------------- */
/*  list_event()    List all events                                     */
/* -------------------------------------------------------------------- */
void list_event(void)
{
    int i;
    char dtstr[20];
  
    termCap(TERM_UNDERLINE);
    mPrintf(" ##   " "   Type   " "           String     "
            "    Redo  Retry   Succeeded    Attempted");
    termCap(TERM_NORMAL);
    doCR();
  
    for (i=0; i<=numevents; i++)
    {
        if (events[i].e_type != ERROR) 
        {
            mPrintf(" %02d%c%c %10s%22s%8d%7d", i,
                on_event == i  ? '<' : ' ',
                cando_event(i) ? '+' : 
                    (events[i].l_sucess != events[i].l_try) ? '-' : ' ',
                crontypes[events[i].e_type], events[i].e_str,
                events[i].e_redo, events[i].e_retry);
            if (events[i].l_sucess)
            {
                sstrftime(dtstr, 19, "  %b%D %H:%M", events[i].l_sucess);
                mPrintf("%s", dtstr);
            }
            else
            {
                mPrintf("      N/A    ");
            }
            if (events[i].l_try)
            {
                sstrftime(dtstr, 19, "  %b%D %H:%M", events[i].l_try);
                mPrintf("%s", dtstr);
            }
            else
            {
                mPrintf("      N/A");
            }
            doCR();
        }
    }
}

/* -------------------------------------------------------------------- */
/*  cron_commands() Sysop Fn: Cron commands                             */
/* -------------------------------------------------------------------- */
void cron_commands(void)
{
    int i;
    char ich;
    
    switch (toupper( (ich = commandchar()) ))
    {
    case 'A':
        mPrintf("All Done\n ");
        doCR();
        mPrintf("Seting all events to done...");
        for (i=0; i<MAXCRON; i++)
        {
            events[i].l_sucess  = time(NULL);
            events[i].l_try     = time(NULL);
        }
        doCR();
        break;
    case 'D':
        mPrintf("Done event\n ");
        if (numevents == ERROR)
        {
            mPrintf("\n NO CRON EVENTS!\n ");
            return;
        }
        done_event();
        break;
    case 'E':
        mPrintf("Enter Cron file\n ");
        readcron();
        break;
    case 'F':
        mPrintf("Force event\n ");
        force_event();
        break;
    case 'L':
        mPrintf("List events\n\n"); 
        list_event();
        break;
    case 'N':
        mPrintf("Next event set\n\n");
        next_event_set();
        break;
    case 'R':
        mPrintf("Reset event\n ");
        if (numevents == ERROR)
        {
            mPrintf("\n NO CRON EVENTS!\n ");
            return;
        }
        reset_event();
        break;
    case 'Z':
        mPrintf("Zap event\n ");
        if (numevents == ERROR)
        {
            mPrintf("\n NO CRON EVENTS!\n ");
            return;
        }
        zap_event();
        break;
    case '?':
        oChar('?');
        doCR();
        doCR();
        mPrintf(" A>ll done\n"
                " D>one event\n"
                " E>nter Cron file\n"
                " F>orce event\n");
        mPrintf(" L>ist event\n"
                " N>ext event set\n"
                " R>eset event\n"
                " Z>ap event\n"
                " ?> -- this menu\n");
        break;
    default:
        oChar(ich);
        if (!expert)  mPrintf("\n '?' for menu.\n "  );
        else          mPrintf(" ?\n "                );
        break;
    }
}  

/* -------------------------------------------------------------------- */
/*  zap_event()     Zap an event out of the cron list                   */
/* -------------------------------------------------------------------- */
static void zap_event(void)
{
    int i;

    i = (int)getNumber("event", 0L, (long)numevents, (long)ERROR);
    if (i == ERROR) return;
    events[i].e_type = ERROR;
}

/* -------------------------------------------------------------------- */
/*  reset_event()   Reset an even so that it has not been done          */
/* -------------------------------------------------------------------- */
static void reset_event(void)
{
    int i;

    i = (int)getNumber("event", 0L, (long)numevents, (long)ERROR);
    if (i == ERROR) return;
    events[i].l_sucess = 0L;
    events[i].l_try    = 0L;
}

/* -------------------------------------------------------------------- */
/*  done_event()    Set event so it seems to have been done             */
/* -------------------------------------------------------------------- */
static void done_event(void)
{
    int i;
    long l, time();

    i = (int)getNumber("event", 0L, (long)numevents, (long)ERROR);
    if (i == ERROR) return;
    events[i].l_sucess = time(&l);
    events[i].l_try    = time(&l);
}

/* -------------------------------------------------------------------- */
/*  did_net()       Set all events for a node to done                   */
/* -------------------------------------------------------------------- */
void did_net(const char *callnode)
{
    int i;
    long l, time();
  
    for (i=0; i<=numevents; i++)
    {
        if( strcmpi(events[i].e_str, callnode) == SAMESTRING
                && events[i].e_type == CR_NET)
        {
            events[i].l_sucess = time(&l);
            events[i].l_try    = time(&l);
        }
    }
}

/* -------------------------------------------------------------------- */
/*  force_event()   Force an event to occur                             */
/* -------------------------------------------------------------------- */
static void force_event(void)
{
    int i;

    i = (int) getNumber("event", 0L, (long)numevents, (long)ERROR);
    if (i != ERROR)
        do_event(i);
}

/* -------------------------------------------------------------------- */
/*  do_cmd_event()  Perform the event indicated in the string           */
/* -------------------------------------------------------------------- */
void do_cmd_event(char *str)
{
    char *words[256];
/*  int i; */

    parse_it(words, str);
    if (strcmpi(words[0], "chat") == SAMESTRING)
    {
        if (strcmpi(words[1], "on") == SAMESTRING)
            cfg.noChat = FALSE;
        else if (strcmpi(words[1], "off") == SAMESTRING)
            cfg.noChat = TRUE;
        else
            cPrintf("\nInvalid CRON command: %s %s\n", words[0], words[1]);
    }
}
 
/* -------------------------------------------------------------------- */
/*  next_event_set() Set the next event pointer.                        */
/* -------------------------------------------------------------------- */
void next_event_set(void)
{
    int i;

    i = (int) getNumber("event", 0L, (long)numevents, (long)ERROR);
    if (i != ERROR)
        on_event = i;
}
      
