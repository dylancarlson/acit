/* -------------------------------------------------------------------- */
/*  GLOB.H                        ACit                         91Sep30  */
/*                                                                      */
/*  All the global variables in all of Citadel. Put here to make        */
/*  moving functions around a hell of a lot easier                      */
/* -------------------------------------------------------------------- */

#ifdef MISC

/* FILES */
FILE                    *msgfl;             /* msg file descriptor      */
FILE                    *printfile;         /* printer.out              */
FILE                    *trapfl;            /* trap file descriptor     */
FILE                    *upfd;
FILE                    *aideFl = NULL;     /* temp file for aide msgs  */
FILE                    *hallfl;            /* Hall file descriptor     */
FILE                    *grpfl;             /* Group file descriptor    */
FILE                    *roomfl;            /* Room file descriptor     */
FILE                    *logfl;             /* log file descriptor      */

/* STRUCTURES */
struct accounting       thisAccount;    /* Groupdata used for current user  */
struct accountBuffer    accountBuf;         /* Accounting buffer            */
struct mfilter          mf;                 /* Message filter               */
struct nodest           node;               /* node buffer                  */
label                   editcmd;            /* External editor commands     */
struct ext_editor       *edit;              /* External editor commands     */
struct fInfo            *fileinfo;          /* pointer for file-info struct */
struct groupBuffer      grpBuf;             /* Group data goes here         */
struct hallBuffer       *hallBuf;           /* Hall stuff right here        */ 
struct logBuffer        logBuf;             /* Log buffer of a person       */
struct logBuffer        lBuf;               /* Temp buffer...........       */
struct logBuffer        lBuf2;              /* Temp buffer...........       */
struct config           cfg;                /* A buncha variables           */ 
struct lTable           *logTab;            /* RAM index of pippuls         */ 
struct messagetable1  *msgTab1;         /* THE message table            */
struct messagetable2  *msgTab2;         /* THE message table            */
struct messagetable3  *msgTab3;         /* THE message table            */
struct messagetable4  *msgTab4;         /* THE message table            */
struct messagetable5  *msgTab5;         /* THE message table            */
struct messagetable6  *msgTab6;         /* THE message table            */
struct messagetable7  *msgTab7;         /* THE message table            */
struct messagetable8  *msgTab8;         /* THE message table            */
struct messagetable9  *msgTab9;         /* THE message table            */
struct aRoom            roomBuf;            /* Room buffer                  */
struct msgB             *msgBuf;            /* The -sole- message buffer    */   
struct msgB             *msgBuf2;           /* The Old-message buffer       */   
struct talleyBuffer     talleyBuf;          /* message talley for rooms     */
struct fDir             *filedir;            /* pointer for filedir struct  */

#endif

#ifdef LOG1
char                    extrncmd[MAXEXTERN];/* External command letters     */
struct ext_prot         *extrn;   /* External protocols buffer    */
struct ext_other        *othCmd;  /* External commands buffer     */
struct rTable           *roomTab;  /* RAM index of rooms           */ 

/* Function pointers */
void                    (*charattr)(char, uchar);
void                    (*stringattr)(uint, const char *, uchar);

/* OTHER */
char                    specialTime;    /* Is it a "special" hour           */
float                   startbalance;   /* Number of credits at log-in      */
long                    lasttime;       /* timestamp of last update         */
char                    readMessage = TRUE;
char                    ExitToMsdos = FALSE;  /*  bring system down         */
char                    sysReq = FALSE;       /*  SysOp wants system        */
char                    *confirm = "Confirm";
char                    mailFlag;           /* Sending private mail?        */
char                    oldFlag;            /* Entering old message?        */
char                    limitFlag;          /* Entering Limited-Access      */
char                    chatReq = FALSE;    /* Did they want to chat?       */
char                    dowhat = DUNO;      /* What is calling iChar        */
int                     xpd, duplic;
char                    duplicate, expired; /* count duplicates and expired */
char                    netError = FALSE;   /* save net error message?      */
unsigned char           thisHall;           /* hall we're in                */
int                     infolength;         /* size of info-file            */
int                     thisLog;            /* entry currently in logBuf    */
int                     thisRoom = LOBBY;   /* Current room                 */
char                    etcpath[64];        /* Path etc.dat and ctdl.exe in */     
int                     thisSlot;           /* logTab slot logBuf found via */     
char                    loggedIn = FALSE;   /* Global have-caller flag      */     
char                    prevChar;           /* for EOLN/EOParagraph stuff   */     
char                    expert;             /* true to suppress hints etc.  */     
char                    termUpper;          /* uppercase only flag          */     
char                    termLF;             /* LF-after-CR flag             */     
char                    aide;               /* aide privileges flag         */     
char                    sysop;              /* sysop privileges flag        */     
char                    twit;               /* Problem user?                */     
char                    unlisted;           /* List in userlog?             */     
char                    termTab;            /* Tabs                         */     
char                    oldToo;             /* Send last old on new request?*/     
char                    roomtell;           /* Print room descriptions?     */     
unsigned char           termWidth;          /* width to format output to    */     
unsigned char           termNulls;          /* # nulls to send at EOLN      */     
int                     people;             /* this is the old slot         */
char                    sysopNew = FALSE;                                         
long                    uptimestamp;
long                    logtimestamp;
long                    conntimestamp;
ulong                   transmitted;     /* how many characters transmitted */
ulong                   received;        /* how many characters transmitted */
char                    backout = FALSE; /* Words out backward?       */
char                    justLostCarrier = FALSE;   /* Just lost it        */
char                    newCarrier = FALSE;   /* Just got carrier         */
char                    whichIO = CONSOLE; /* CONSOLE or MODEM            */
char                    haveCarrier;     /* set if DCD == TRUE            */
char                    echo;          /* Either NEITHER, CALLER, or BOTH */
char                    echoChar; /* What to echo with if echo == NEITHER */
char                    anyEcho = TRUE;
char                    warned;
char                    sysopkey = FALSE;                                                      
char                    chatkey  = FALSE;                                                      
char                    eventkey = FALSE;
char                    sleepkey = FALSE;
char                    disabled;                 /* is modem disabled?   */
int                     getkey;
char                    speed;
char                    detectflag = FALSE; /* got carrier while on console*/
char                    modem = TRUE;
char                    printing = FALSE;
char                    console = TRUE;        /* output flag for console */
char                    ConLock = FALSE;
char                    debug = FALSE;
char                    callout = FALSE;
unsigned char           crtColumn;          /* current position on screen  */   
char                    outFlag = OUTOK;    /* will be one of the above    */   
char                    dotoMessage = NO_SPECIAL;                               
ulong                   markedMId   = 0l;   /* id of message to be marked  */   
char                    heldMessage;                                            
char                    replyFlag;          /* Sending a reply?            */   
char                    linkMess;           /* Sending a file-linked message? */
int                     mread, entered;     /* #messages read and entered  */   
                                                                                
ulong                   originalId = 0l;    /* this is the ID of origin      */ 
unsigned char           originalattr;       /* Attribute byte of origin      */ 
char                    copyflag = FALSE;   /* true when viewing a duplicate */ 
char                    MessageRoom[MAXROOMS];  /* number of message/room this  
                                                  call */                       
char                    seen;                                                   
int                     bauds[] =  { 300, 1200, 2400, 4800, 9600, 19200 };
char                    showdir, showhidden, showgroup;
char                    skiproom = TRUE;
int                     oldroom, ug_lvisit, ug_new, ug_hasmail;
uchar                   scrollpos = 23;    /* adjusts for help menu         */
uchar                   ansiattr;
char                    ansiOn = FALSE;
char                    heldMessage;
char                    replyFlag;          /* Sending a reply?            */
char                    linkMess;           /* Sending a file-linked message? */
int                     mread, entered;     /* #messages read and entered  */
unsigned char           originalattr;       /* Attribute byte of origin      */
char                    MessageRoom[MAXROOMS];  /* number of message/room this                                              
                                                    call */
char                    seen;

char   grpZap  = FALSE,
       hallZap = FALSE,
       msgZap  = FALSE,
       logZap  = FALSE,
       roomZap = FALSE;

char   roomBuild = FALSE,               /* Rebuild room-file?           */
       hallBuild = FALSE;               /* Rebuild hall-file?           */

char   gprompt[80];

uchar               numOthCmds;         /* number of other commands */

#endif

/************************************************************************/

#ifndef MISC
/* FILES */
extern FILE                    *msgfl;             /* msg file descriptor          */   
extern FILE                    *printfile;         /* printer.out                  */
extern FILE                    *trapfl;            /* trap file descriptor         */
extern FILE                    *upfd;
extern FILE                    *aideFl;            /* a temp file for aide messages */
extern FILE                    *hallfl;            /* Hall file descriptor         */     
extern FILE                    *grpfl;             /* Group file descriptor        */     
extern FILE                    *roomfl;            /* Room file descriptor         */
extern FILE                    *logfl;             /* log file descriptor          */
       
/* STRUCTURES */
extern struct accounting       thisAccount;    /* Groupdata used for current user  */
extern struct accountBuffer    accountBuf;         /* Accounting buffer            */
extern struct mfilter          mf;                 /* Message filter               */
extern struct nodest           node;               /* node buffer                  */
extern label                   editcmd;            /* External editor commands     */
extern struct ext_editor       *edit;              /* External editor commands     */
extern struct fInfo            *fileinfo;          /* pointer for file-info struct */
extern struct groupBuffer      grpBuf;             /* Group data goes here         */
extern struct hallBuffer       *hallBuf;           /* Hall stuff right here        */ 
extern struct logBuffer        logBuf;             /* Log buffer of a person       */
extern struct logBuffer        lBuf;               /* Temp buffer...........       */
extern struct logBuffer        lBuf2;              /* Temp buffer...........       */
extern struct config           cfg;                /* A buncha variables           */ 
extern struct lTable           *logTab;            /* RAM index of pippuls         */ 
extern struct messagetable1  *msgTab1;         /* THE message table            */
extern struct messagetable2  *msgTab2;         /* THE message table            */
extern struct messagetable3  *msgTab3;         /* THE message table            */
extern struct messagetable4  *msgTab4;         /* THE message table            */
extern struct messagetable5  *msgTab5;         /* THE message table            */
extern struct messagetable6  *msgTab6;         /* THE message table            */
extern struct messagetable7  *msgTab7;         /* THE message table            */
extern struct messagetable8  *msgTab8;         /* THE message table            */
extern struct messagetable9  *msgTab9;         /* THE message table            */
extern struct aRoom            roomBuf;            /* Room buffer                  */
extern struct msgB             *msgBuf;             /* The -sole- message buffer    */   
extern struct msgB             *msgBuf2;            /* The Old-message buffer       */   
extern struct talleyBuffer     talleyBuf;          /* message talley for rooms     */
extern struct fDir             *filedir;            /* pointer for filedir struct  */

#endif

#ifndef LOG1
extern char                    extrncmd[MAXEXTERN];/* External command letters     */
extern struct ext_prot         *extrn;   /* External protocols buffer    */
extern struct ext_other        *othCmd;  /* External commands buffer     */
extern struct rTable           *roomTab;  /* RAM index of rooms           */ 
       
/* Function pointers */
extern void                    (*charattr)(char, uchar);
extern void                    (*stringattr)(uint, const char *, uchar);
       
/* OTHER */
extern char                    specialTime;    /* Is it a "special" hour           */
extern float                   startbalance;   /* Number of credits at log-in      */
extern long                    lasttime;       /* timestamp of last update         */
extern char                    readMessage ;
extern char                    ExitToMsdos;  /*  bring system down         */
extern char                    sysReq;       /*  SysOp wants system        */
extern char                    *confirm;
extern char                    mailFlag;           /* Sending private mail?        */
extern char                    oldFlag;            /* Entering old message?        */
extern char                    limitFlag;          /* Entering Limited-Access      */
extern char                    chatReq;    /* Did they want to chat?       */
extern char                    dowhat;      /* What is calling iChar        */
extern int                     xpd, duplic;
extern char                    duplicate, expired; /* count duplicates and expired */
extern char                    netError;   /* save net error message?      */
extern unsigned char           thisHall;           /* hall we're in                */
extern int                     infolength;         /* size of info-file            */
extern int                     thisLog;            /* entry currently in logBuf    */
extern int                     thisRoom;   /* Current room                 */
extern char                    etcpath[64];        /* Path etc.dat and ctdl.exe in */     
extern int                     thisSlot;           /* logTab slot logBuf found via */     
extern char                    loggedIn;   /* Global have-caller flag      */     
extern char                    prevChar;           /* for EOLN/EOParagraph stuff   */     
extern char                    expert;             /* true to suppress hints etc.  */     
extern char                    termUpper;          /* uppercase only flag          */     
extern char                    termLF;             /* LF-after-CR flag             */     
extern char                    aide;               /* aide privileges flag         */     
extern char                    sysop;              /* sysop privileges flag        */     
extern char                    twit;               /* Problem user?                */     
extern char                    unlisted;           /* List in userlog?             */     
extern char                    termTab;            /* Tabs                         */     
extern char                    oldToo;             /* Send last old on new request?*/     
extern char                    roomtell;           /* Print room descriptions?     */     
extern unsigned char           termWidth;          /* width to format output to    */     
extern unsigned char           termNulls;          /* # nulls to send at EOLN      */     
extern int                     people;             /* this is the old slot         */
extern char                    sysopNew ;                                         
extern long                    uptimestamp;
extern long                    logtimestamp;
extern long                    conntimestamp;
extern ulong                   transmitted;     /* how many characters transmitted */
extern ulong                   received;        /* how many characters transmitted */
extern char                    backout; /* Words out backward?       */
extern char                    justLostCarrier;   /* Just lost it        */
extern char                    newCarrier ;   /* Just got carrier         */
extern char                    whichIO ; /* CONSOLE or MODEM            */
extern char                    haveCarrier;     /* set if DCD == TRUE            */
extern char                    echo;          /* Either NEITHER, CALLER, or BOTH */
extern char                    echoChar; /* What to echo with if echo == NEITHER */
extern char                    anyEcho ;
extern char                    warned;
extern char                    sysopkey ;                                                      
extern char                    chatkey  ;                                                      
extern char                    eventkey ;                                                      
extern char                    sleepkey ;                                                      
extern char                    disabled;                 /* is modem disabled?   */
extern int                     getkey;
extern char                    speed;
extern char                    detectflag ; /* got carrier while on console*/
#if 0
extern char                    tfilter[128];
#else
extern const uchar             filt_in[128];
extern const uchar             filt_out[256];
#endif
extern char                    modem ;
extern char                    printing ;
extern char                    console ;        /* output flag for console */
extern char                    ConLock ;
extern char                    debug ;
extern char                    callout;
extern unsigned char           crtColumn;          /* current position on screen  */   
extern char                    outFlag ;    /* will be one of the above    */   
extern char                    dotoMessage ;
extern ulong                   markedMId ;   /* id of message to be marked  */   
extern char                    heldMessage;                                            
extern char                    replyFlag;          /* Sending a reply?            */   
extern char                    linkMess;           /* Sending a file-linked message? */
extern int                     mread, entered;     /* #messages read and entered  */   
                                                                                       
extern ulong                   originalId ;    /* this is the ID of origin      */ 
extern unsigned char           originalattr;       /* Attribute byte of origin      */ 
extern char                    copyflag ;   /* true when viewing a duplicate */ 
extern char                    MessageRoom[MAXROOMS];  /* number of message/room this  
                                                         call */                       
extern char                    seen;                                                   
extern int                     bauds[];
extern char                    showdir, showhidden, showgroup;
extern char                    skiproom;
extern int                     oldroom, ug_lvisit, ug_new, ug_hasmail;
extern uchar                   scrollpos;    /* adjusts for help menu         */
extern uchar                   ansiattr;
extern char                    ansiOn;
extern char                    heldMessage;
extern char                    replyFlag;          /* Sending a reply?            */
extern char                    linkMess;           /* Sending a file-linked message? */
extern int                     mread, entered;     /* #messages read and entered  */
extern unsigned char           originalattr;       /* Attribute byte of origin      */
extern char                    MessageRoom[MAXROOMS];  /* number of message/room this                                              
                                                           call */
extern char                    seen;

extern char   grpZap, hallZap, msgZap, logZap, roomZap;

extern char   roomBuild,               /* Rebuild room-file?           */
              hallBuild;               /* Rebuild hall-file?           */

extern char   gprompt[80];

extern uchar               numOthCmds;         /* number of other commands */

#endif /* MAIN */

extern char version[];
#if 1
extern char softname[];
#endif
extern char testsite[];
extern char cmpDate[];
extern char cmpTime[];
extern char *welcome[];
extern char *copyright[];
