/************************************************************************/
/*  SYSG.C                        ACit                         91Sep29  */
/*                          Sysop group code                            */
/************************************************************************/

#include <string.h>
#include "ctdl.h"
#include "prot.h"
#include "glob.h"

/************************************************************************/
/*                              Contents                                */
/*                                                                      */
/*  do_SysopGroup()   .SG  handles doSysop() Group functions            */
/*  groupfunc()       .AG  add/remove group members                     */
/* $globalgroup()     .SGG add/remove group members  (global)           */
/* $globaluser()      .SGU add/remove user from groups (global)         */
/* $killgroup()       .SGK sysop special to kill a group                */
/*  listgroup()       .AL  aide fn: to list groups                      */
/* $newgroup()        .SGN sysop special to add a group                 */
/* $renamegroup()     .SGR sysop special to rename a group              */
/*                                                                      */
/************************************************************************/

static void globalgroup(void);
static void globaluser(void);
static void killgroup(void);
static void newgroup(void);
static void renamegroup(void);

/************************************************************************/
/*  do_SysopGroup()   handles doSysop() Group functions                 */
/************************************************************************/
void do_SysopGroup(void)
{
    char ich;

    switch(toupper((ich = commandchar())))
    {
    case 'G':
        mPrintf("Group-specific user membership\n  \n");
        globalgroup();
        break;
    case 'K':
        mPrintf("Kill group");
        killgroup();
        break;
    case 'N':
        mPrintf("New group");
        newgroup();
        break;
    case 'U':
        mPrintf("User-specific group membership\n  \n");
        globaluser();
        break;
    case 'R':
        mPrintf("Rename group");
        renamegroup();
        break;
    case '?':
        oChar('?');
        mPrintf("\n N>ew group");
        mPrintf("\n R>ename group");
        mPrintf("\n K>ill group");
        mPrintf("\n G>roup-specific membership (all users)");
        mPrintf("\n U>ser-specific membership (all groups)");
        break;
    default:
        if (!expert)  mPrintf("\n '?' for menu.\n "  );
        else          mPrintf(" ?\n "                );
        break;
    }
}

/************************************************************************/
/*      groupfunc()  aide fn: to add/remove group members               */
/************************************************************************/
void groupfunc(void)
{
    label               who;
    label groupname;
    int groupslot,      logNo;

    getString("group", groupname, NAMESIZE, FALSE, ECHO, "");

    groupslot = partialgroup(groupname);

    if ( grpBuf.group[groupslot].lockout && !sysop )
        groupslot = ERROR;

    if ( grpBuf.group[groupslot].hidden && !ingroup(groupslot) )
        groupslot = ERROR;

    if ( groupslot == ERROR || !strlen(groupname) )
    {
        mPrintf("\n No such group.");
        return;
    }

    getNormStr("who", who, NAMESIZE, ECHO);
    logNo   = findPerson(who, &lBuf);
    if (logNo == ERROR || !strlen(who) )  
    {
        mPrintf("No \'%s\' known. \n ", who);
        return;
    }

    if (lBuf.groups[groupslot] == grpBuf.group[groupslot].groupgen)
    {
        if (getYesNo("Remove from group", 0))
        {
            lBuf.groups[groupslot] = (unsigned char)
                ((grpBuf.group[groupslot].groupgen
              + (MAXGROUPGEN - 1)) % MAXGROUPGEN);

            sprintf(msgBuf->mbtext,
                    "%s kicked out of group %s by %s",
                    lBuf.lbname,
                    grpBuf.group[groupslot].groupname,
                    logBuf.lbname );

            trap(msgBuf->mbtext, T_AIDE);

            aideMessage();
        }
    }
    else

    if (getYesNo("Add to group", 0))
    {
        lBuf.groups[groupslot] = grpBuf.group[groupslot].groupgen;

        sprintf(msgBuf->mbtext,
        "%s added to group %s by %s",
        lBuf.lbname,
        grpBuf.group[groupslot].groupname,
        logBuf.lbname );

        trap(msgBuf->mbtext, T_AIDE);

        aideMessage();

    }

    putLog(&lBuf, logNo);

    /* see if it is us: */
    if (loggedIn  &&  strcmpi(logBuf.lbname, who) == SAMESTRING) 
    {
        logBuf.groups[groupslot] = lBuf.groups[groupslot];
    }

}

/************************************************************************/
/*      globalgroup() aide fn: to add/remove group members  (global)    */
/************************************************************************/
static void globalgroup(void)
{
    label groupname;
    int groupslot, i, yn, add, logNo;

    mPrintf(" Add/Remove/Both: ");

    switch (toupper( commandchar() ))
    {
    case 'A':
        mPrintf("Add\n ");
        add = 1;
        break;
    case 'R':
        mPrintf("Remove\n ");
        add = 2;
        break;
    case 'B':
        mPrintf("Both\n ");
        add = 0;
        break;
    default:
        mPrintf("Both\n ");
        add = 0;
        break;
    }

    getString("group", groupname, NAMESIZE, FALSE, ECHO, "");

    groupslot = partialgroup(groupname);

    if ( grpBuf.group[groupslot].hidden && !ingroup(groupslot) )
        groupslot = ERROR;

    if ( groupslot == ERROR || !strlen(groupname) )
    {
        mPrintf("\n No such group.");
        return;
    }

    if ( grpBuf.group[groupslot].lockout && !sysop )
    {
        mPrintf("\n Group is locked.");
        return;
    }

    for (i = 0; i < cfg.MAXLOGTAB; i++)
    {
        if (logTab[i].ltpwhash != 0 && logTab[i].ltnmhash !=0)
        {
            logNo=logTab[i].ltlogSlot;
            getLog(&lBuf, logNo);
            if (lBuf.groups[groupslot] == grpBuf.group[groupslot].groupgen)
            {
                if(add == 2 || add == 0)
                {
                    mPrintf(" %s", lBuf.lbname);
                    yn=getYesNo("Remove from group", 0+3);
                    if (yn == 2)
                    {
                        SaveAideMess();
                        return;
                    }
      
                    if (yn)
                    {
                        lBuf.groups[groupslot] = (unsigned char)
                            ((grpBuf.group[groupslot].groupgen
                          + (MAXGROUPGEN - 1)) % MAXGROUPGEN);
          
                        sprintf(msgBuf->mbtext,
                            "%s kicked out of group %s by %s",
                            lBuf.lbname,
                            grpBuf.group[groupslot].groupname,
                            logBuf.lbname );
           
                        trap(msgBuf->mbtext, T_SYSOP);
          
                        amPrintf(" %s\n", msgBuf->mbtext);
                    }
                }
            }else{
                if (add == 0 || add == 1)
                {
                    mPrintf(" %s", lBuf.lbname);
                    yn=getYesNo("Add to group", 0+3);
                    if (yn == 2)
                    {
                        SaveAideMess();
                        return;
                    }
                    if (yn)
                    {
                        lBuf.groups[groupslot] =
                            grpBuf.group[groupslot].groupgen;
           
                        sprintf(msgBuf->mbtext,
                            "%s added to group %s by %s",
                            lBuf.lbname,
                            grpBuf.group[groupslot].groupname,
                            logBuf.lbname );
         
                        trap(msgBuf->mbtext, T_AIDE);
           
                        amPrintf(" %s\n",msgBuf->mbtext);
                    }
                }
            }
  
            putLog(&lBuf, logNo);
     
            /* see if it is us: */
            if (loggedIn  &&  strcmpi(logBuf.lbname, lBuf.lbname) == SAMESTRING)
            {
                logBuf.groups[groupslot] = lBuf.groups[groupslot];
            }
        }
    }

    SaveAideMess();
}

/************************************************************************/
/*      globaluser() aide fn: to add/remove user from groups (global)   */
/************************************************************************/
static void globaluser(void)
{
    label               who;
    int groupslot, yn, logNo;

    getNormStr("who", who, NAMESIZE, ECHO);
    logNo   = findPerson(who, &lBuf);
    if (logNo == ERROR || !strlen(who) )  
    {
        mPrintf("No \'%s\' known. \n ", who);
        return;
    }

    for(groupslot=0; groupslot < MAXGROUPS; groupslot++)
    {
      if (grpBuf.group[groupslot].g_inuse 
          && ( !grpBuf.group[groupslot].lockout || sysop )
          && ( !grpBuf.group[groupslot].hidden  || ingroup(groupslot) )
         )
      {
        mPrintf(" %s", grpBuf.group[groupslot].groupname);
        if (lBuf.groups[groupslot] == grpBuf.group[groupslot].groupgen)
        {
            if ((yn = getYesNo("Remove from group", 3)) != 0 /* NULL */)
            {
                if (yn == 2)
                {
                    SaveAideMess();
                    return;
                }

                lBuf.groups[groupslot] = (unsigned char)
                  ((grpBuf.group[groupslot].groupgen
                  + (MAXGROUPGEN - 1)) % MAXGROUPGEN);
                sprintf(msgBuf->mbtext,
                  "%s kicked out of group %s by %s",
                  lBuf.lbname,
                grpBuf.group[groupslot].groupname,
                  logBuf.lbname );

                trap(msgBuf->mbtext, T_AIDE);

              amPrintf(" %s\n", msgBuf->mbtext);
            }
        }else{
          if ((yn = getYesNo("Add to group", 3)) != 0 /* NULL */)
          {
            if (yn == 2)
                {
                    SaveAideMess();
                    return;
                }

            lBuf.groups[groupslot] = grpBuf.group[groupslot].groupgen;

            sprintf(msgBuf->mbtext,
              "%s added to group %s by %s",
              lBuf.lbname,
              grpBuf.group[groupslot].groupname,
              logBuf.lbname );
            trap(msgBuf->mbtext, T_SYSOP);

              amPrintf(" %s\n",msgBuf->mbtext);
          }
        }
        putLog(&lBuf, logNo);

        /* see if it is us: */
        if (loggedIn  &&  strcmpi(logBuf.lbname, who) == SAMESTRING) 
        {
            logBuf.groups[groupslot] = lBuf.groups[groupslot];
        }
      }
    }

    SaveAideMess();
}

/************************************************************************/
/*      killgroup()  sysop special to kill a group                      */
/************************************************************************/
static void killgroup(void)
{
    label groupname;
    int groupslot, i;

    getString("group", groupname, NAMESIZE, FALSE, ECHO, "");

    groupslot = partialgroup(groupname);

    if ( groupslot == 0 || groupslot == 1)
    {
        mPrintf("\n Cannot delete Null or Reserved_2 groups.");
        return;
    }

    if ( groupslot == ERROR || !strlen(groupname) )
    {
        mPrintf("\n No such group.");
        return;
    }

    for (i = 0; i < MAXROOMS; i++)
    {
         if  (   roomTab[i].rtflags.INUSE
             &&  roomTab[i].rtflags.GROUPONLY
             && (roomTab[i].grpno  == (unsigned char)groupslot)
          /* && (roomTab[i].grpgen == grpBuf.group[groupslot].groupgen) */ )
         {
             mPrintf("\n Group still has rooms.");
             return;
         }
    }

    for (i = 0; i < MAXHALLS; i++)
    {
        if ( hallBuf->hall[i].h_inuse
        &&   hallBuf->hall[i].owned 
        &&   hallBuf->hall[i].grpno == (unsigned char)groupslot)
        {
            mPrintf("\n Group still has hallways.");
            return;
        }
    }

    if (!getYesNo(confirm, 0)) return;

    grpBuf.group[groupslot].g_inuse = 0;

    putGroup();

    sprintf(msgBuf->mbtext,
    "Group %s deleted", groupname );

    trap(msgBuf->mbtext, T_SYSOP);
}

/************************************************************************/
/*      listgroup()  aide fn: to list groups                            */
/************************************************************************/
void listgroup(void)
{
    label groupname;
    int i, groupslot;

    getString("", groupname, NAMESIZE, FALSE, ECHO, "");

    outFlag = OUTOK;

    if ( !strlen(groupname))
    {
        for (i = 0; i < MAXGROUPS; i++)
        {
            /* can they see the group */
            if (  grpBuf.group[i].g_inuse
              && (!grpBuf.group[i].lockout || sysop)
              && (!grpBuf.group[i].hidden  || (whichIO == CONSOLE) || ingroup(i))
              )
            {
                mPrintf(" %-20s ", grpBuf.group[i].groupname);
                if (grpBuf.group[i].lockout) mPrintf("(Locked) ");
                if (grpBuf.group[i].hidden)  mPrintf("(Hidden) ");
                doCR();
            }
        }  
        return;
    }

    groupslot = partialgroup(groupname);

    if ( grpBuf.group[groupslot].hidden && !ingroup(groupslot) )
        groupslot = ERROR;

    if (groupslot == ERROR)
    {
        mPrintf("\n No such group.");
        return;
    }

    if ( grpBuf.group[groupslot].lockout
    && (!(logBuf.lbflags.SYSOP || (whichIO == CONSOLE))) )
    {
        mPrintf("\n Group is locked.");
        return;
    }

    doCR();

    outFlag = OUTOK;

    mPrintf(" Users:");

    for (i = 0; ( ( i < cfg.MAXLOGTAB) && (outFlag != OUTSKIP)
        && (outFlag != OUTNEXT) ); i++)
    {
        if (logTab[i].ltpwhash != 0 && logTab[i].ltnmhash != 0)
        {
            getLog(&lBuf, logTab[i].ltlogSlot);

            if (lBuf.groups[groupslot] == grpBuf.group[groupslot].groupgen)
            {
                doCR();
                mPrintf(" %s", lBuf.lbname);
            }
        }
    }
    
    if (outFlag == OUTSKIP)
    {
        doCR();
        return;
    }  

    outFlag = OUTOK;

    doCR(); 
    doCR(); 

    mPrintf(" Rooms:"); 

    for (i = 0; ( ( i < MAXROOMS) && (outFlag != OUTSKIP)
        && (outFlag != OUTNEXT) ); i++)
    {
         if  (   roomTab[i].rtflags.INUSE 
             &&  roomTab[i].rtflags.GROUPONLY
             && (roomTab[i].grpno  == (unsigned char)groupslot)
         /*  && (roomTab[i].grpgen == grpBuf.group[groupslot].groupgen) */ )
         {
             doCR();
             mPrintf(" %s" , roomTab[i].rtname);
         }
    }

    if (outFlag == OUTSKIP)
    {
        doCR();
        return;
    }  

    outFlag = OUTOK;

    doCR(); 
    doCR(); 

    mPrintf(" Hallways:"); 

    for (i = 0; ( ( i < MAXHALLS) && (outFlag != OUTSKIP)
        && (outFlag != OUTNEXT) ); i++)
    {
        if ( hallBuf->hall[i].h_inuse &&
             hallBuf->hall[i].owned   &&
            (hallBuf->hall[i].grpno == (unsigned char)groupslot) )
        {
            doCR();
            mPrintf(" %s", hallBuf->hall[i].hallname);
        }
    }
}

/************************************************************************/
/*      newgroup()  sysop special to add a new group                    */
/************************************************************************/
static void newgroup(void)
{
    label groupname;
    int slot, i;

    getString("group", groupname, NAMESIZE, FALSE, ECHO, "");

    if ( (groupexists(groupname) != ERROR) || !strlen(groupname) )
    {
        mPrintf("\n We already have a \'%s\' group.", groupname);
        return;
    }

    /* search for a free group slot */

    for (i = 0, slot = 0; i < MAXGROUPS && !slot ; i++)
    {
        if (!grpBuf.group[i].g_inuse) slot = i;
    }

    if (!slot)
    {
        mPrintf("\n Group table full.");
        return;
    }
    
    /*  locked group? */
    grpBuf.group[slot].lockout = (getYesNo("Lock group from aides", 0 ));

    grpBuf.group[slot].hidden  = (getYesNo("Hide group", 0 ));

    strcpy(grpBuf.group[slot].groupname, groupname);
    grpBuf.group[slot].g_inuse = 1;

    /* increment group generation # */
    grpBuf.group[slot].groupgen = (unsigned char)
   ((grpBuf.group[slot].groupgen + 1) % MAXGROUPGEN);


    if (getYesNo(confirm, 0))
    {
        putGroup();

        sprintf(msgBuf->mbtext,
        "Group %s created", grpBuf.group[slot].groupname );

        trap(msgBuf->mbtext, T_SYSOP);
#if 1
        if (loggedIn)
        {
            i = findPerson(logBuf.lbname, &lBuf);
            lBuf.groups[slot] = 
            logBuf.groups[slot] = 
                                  grpBuf.group[slot].groupgen;
            putLog(&lBuf, i);

        }
#endif
    }
    else getGroup();
}

/************************************************************************/
/*      renamegroup()  sysop special to rename a group                  */
/************************************************************************/
static void renamegroup(void)
{
    label groupname, newname;
    int groupslot, locked, hidden;

    getString("group", groupname, NAMESIZE, FALSE, ECHO, "");

    groupslot = partialgroup(groupname);

    if ( grpBuf.group[groupslot].hidden && !ingroup(groupslot) && !(whichIO == CONSOLE))
        groupslot = ERROR;

    if ( groupslot == ERROR || !strlen(groupname) )
    {
        mPrintf("\n No such group.");
        return;
    }

    getString("new group name", newname, NAMESIZE, FALSE, ECHO, "");

    if (groupexists(newname) != ERROR)
    {
        mPrintf("\n A \'%s\' group already exists.", newname);
        return;
    }

    /*  locked group? */
    locked =
        (getYesNo("Lock group from aides", (char)grpBuf.group[groupslot].lockout ));

    hidden =
        (getYesNo("Make group hidden", (char)grpBuf.group[groupslot].hidden ));

    if (!getYesNo(confirm, 0)) return;

    grpBuf.group[groupslot].lockout = locked;
    grpBuf.group[groupslot].hidden  = hidden;

    if (strlen(newname))
        strcpy(grpBuf.group[groupslot].groupname, newname);

    sprintf(msgBuf->mbtext,
    "Group %s renamed %s", groupname, newname);

    trap(msgBuf->mbtext, T_SYSOP);

    putGroup();
}
