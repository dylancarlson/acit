/************************************************************************/
/*                               KEY.H                         91Aug23  */
/*        all keywords used by Citadel configuration program            */
/************************************************************************/

/************************************************************************/
/*                         config.cit keywords                          */
/************************************************************************/
#ifdef CFG
char *keywords[] = 
{
    "#ACCOUNTING",    "#AIDEHALL",    "#ALLDONE",    "#APPLICPATH",
    "#ATTR",          "#BATTR",       "#BIOS",       "#CATTR",
    "#COST1200",      "#COST2400",    "#DATESTAMP",  "#DIAL_INIT",
    "#DIAL_PREF",     "#DUMB_MODEM",  "#ENTEROK",    "#F6PASSWORD",
    "#FORCELOGIN",    "#HELPPATH",    "#HOMEPATH",   "#IDLE_WAIT",
    "#INIT_BAUD",     "#KILL",        "#LINEFEEDS",  "#LOGINSTATS",
    "#MAXBALANCE",    "#MAXFILES",    "#MAXLOGTAB",  "#MAXTEXT",
    "#MAX_WARN",      "#MDATA",       "#MESSAGEK",   "#MESSAGE_ROOM",
    "#MODERATE",      "#MODSETUP",    "#MSGPATH",    "#NEWBAL",
    "#NEWUSERAPP",    "#NMESSAGES",   "#NODENAME",   "#NODEREGION",
    "#NOPWECHO",      "#NULLS",       "#OFFHOOK",    "#OLDCOUNT",
    "#PRINTER",       "#PRIVATE",     "#READLLOG",   "#READOK",
    "#ROOMOK",        "#ROOMPATH",    "#ROOMTELL",   "#SUBHUBS",
    "#TABS",          "#TEMPPATH",    "#TIMEOUT",    "#TRAP",
    "#TRAP_FILE",     "#TWIT_FEATURES",  "#UNLOGGEDBALANCE",
    "#UNLOGTIMEOUT",  "#UPPERCASE",   "#USER",       "#UTTR",
    "#VDATESTAMP",    "#WATTR",       "#WIDTH",
#if 1
    "#NODECOUNTRY",   "#DIAL_RING",   "#TWITREGION", "#TWITCOUNTRY",
    "#SIGNATURE",     "#ANONAUTHOR",  "#TRANSPATH",  "#DIRPATH",
    "#HOURS",
#endif           
    NULL
};
                       
enum cfgkey_enum
{
    K_ACCOUNTING,     K_AIDEHALL,     K_ALLDONE,     K_APPLICATIONS,
    K_ATTR,           K_BATTR,        K_BIOS,        K_CATTR,
    K_COST1200,       K_COST2400,     K_DATESTAMP,   K_DIAL_INIT,
    K_DIAL_PREF,      K_DUMB_MODEM,   K_ENTEROK,     K_F6PASSWORD,
    K_FORCELOGIN,     K_HELPPATH,     K_HOMEPATH,    K_IDLE_WAIT,
    K_INIT_BAUD,      K_KILL,         K_LINEFEEDS,   K_LOGINSTATS,
    K_MAXBALANCE,     K_MAXFILES,     K_MAXLOGTAB,   K_MAXTEXT,
    K_MAX_WARN,       K_MDATA,        K_MESSAGEK,    K_MESSAGE_ROOM,
    K_MODERATE,       K_MODSETUP,     K_MSGPATH,     K_NEWBAL,
    K_NEWUSERAPP,     K_NMESSAGES,    K_NODENAME,    K_NODEREGION,
    K_NOPWECHO,       K_NULLS,        K_OFFHOOK,     K_OLDCOUNT,
    K_PRINTER,        K_PRIVATE,      K_READLLOG,    K_READOK,
    K_ROOMOK,         K_ROOMPATH,     K_ROOMTELL,    K_SUBHUBS,
    K_TABS,           K_TEMPPATH,     K_TIMEOUT,     K_TRAP,
    K_TRAP_FILE,      K_TWIT_FEATURES,   K_UNLOGGEDBALANCE,
    K_UNLOGTIMEOUT,   K_UPPERCASE,    K_USER,        K_UTTR,
    K_VDATESTAMP,     K_WATTR,        K_WIDTH,
#if 1
    K_NODECOUNTRY,    K_DIAL_RING,    K_TWITREGION,  K_TWITCOUNTRY,
    K_SIGNATURE,      K_ANONAUTHOR,   K_TRANSPATH,   K_DIRPATH,
    K_HOURS
#endif
};

#define K_NWORDS (sizeof(keywords)/sizeof(char *) - 2)
/* better to use macro than count */

char *trapkeywords[] =
{
    "ALL",            "CARRIER",      "LOGIN",       "NEWROOM",
    "PROBLEM_USER",   "CHAT",         "PASSWORD",    "AIDE",
    "SYSOP",          "UPLOAD",       "DOWNLOAD",    "ERROR",
    "NETWORK",        "ACCOUNT",      "APPLIC",
    NULL
};

char *userkeywords[] = 
{   
    "PROBLEM_USER" ,  "PERMANENT",    "NO_ACCOUNTING",
    "NETWORK",        "NO_MAIL",      "AIDE",       "SYSOP",
    NULL
};

char *twitfeatures[] = 
{   
    "MSG_NYMS",       "BORDER_LINES", "TITLES",     "NET_TITLES",
    "SURNAMES",       "NET_SURNAMES", "ENTER_TITLES",
    NULL
};

#endif /* CFG */

/************************************************************************/
/*                        grpdata.cit keywords                          */
/************************************************************************/

#ifdef MAIN
char *grpkeywords[] =
{
    "#DAYS",          "#GROUP",       "#HOURS",     "#DAY_INC",
    "#SPECIAL",       "#PRIORITY",    "#MAX_BAL",   "#DL_MULT",
    "#UL_MULT",
    NULL
};
#else
extern char *grpkeywords[];
#endif /* MAIN */

enum grpkey_enum
{
    GRK_DAYS,         GRK_GROUP,      GRK_HOURS,    GRK_DAYINC,
    GRK_SPECIAL,      GRK_PRIORITY,   GRK_MAXBAL,   GRK_DLMULT,
    GRK_ULMULT
};

#ifdef MAIN
char *daykeywords[] = 
{
   "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Any", NULL 
};
#else
extern char *daykeywords[];
#endif


/************************************************************************/
/*                         nodes.cit keywords                           */
/************************************************************************/

#ifdef MAIN
char *nodekeywords[] = 
{
    "#BAUD",          "#DISCARD",     "#DISCARD_USER",
    "#EXPIRE",        "#GROUP",       "#DIAL_TIMEOUT",
    "#LOGIN",         "#WAIT_TIMEOUT",    "#PROTOCOL",
    "#NODE",          "#PHONE",       "#REDIAL",    "#MAIL_TMP",
    "#ROOM",          "#ROUTE",       "#ALIAS",     "#REGION",
    NULL
};
#else
extern char *nodekeywords[];
#endif /* MAIN */

enum nodekey_enum
{
    NOK_BAUD,         NOK_DISCARD,    NOK_DISCARD_USER,
    NOK_EXPIRE,       NOK_GROUP,      NOK_DIAL_TIMEOUT,
    NOK_LOGIN,        NOK_WAIT_TIMEOUT,   NOK_PROTOCOL,
    NOK_NODE,         NOK_PHONE,      NOK_REDIAL,   NOK_MAIL_TMP,
    NOK_ROOM,         NOK_ROUTE,      NOK_ALIAS,    NOK_REGION
};

/************************************************************************/
/*                         cron.cit keywords                            */
/************************************************************************/
#ifdef MAIN
char *cronkeywords[] = 
{
    "#DO",      "#HOURS",   "#DAYS",   "#REDO_TIME",
    "#RETRY_TIME",         "#PRIORITY",
    NULL
};
#else
extern char *cronkeywords[];
#endif /* MAIN */

enum cronkey_enum
{
    CR_DO,      CR_HOURS,   CR_DAYS,   CR_REDO,
    CR_RETRY,   CR_PRIORITY
};

#ifdef MAIN
char *crontypes[] = 
{
  "NETWORK",    "SHELL_1",    "COMMAND",
  NULL
};
#else
extern char *crontypes[];
#endif

enum crontype_enum
{
  CR_NET,       CR_SHELL_1,   CR_COMMAND
};
