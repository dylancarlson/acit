/************************************************************************/
/*  PROT.H                        ACit                         91Sep30  */
/*                        Function Prototypes                           */
/*                                                                      */
/* C> for %f in (*.c) do cl /DNETWORK /Zg %f >> prot                    */
/*                                                                      */
/************************************************************************/

/***** ser.asm *****/
void  INITRS(int Port, int Baud, int Stops, int Parity, int Length);
void  DEINITRS(void);
void  FLUSHRS(void);
int   STATRS(void);
int   GETRS(void);
void  PUTRS(char ch);
void  DTRRS(int dtr);
int   CARRSTATRS(void);
int   RINGSTATRS(void);
int   STATCON(void);
char  GETCON(void);
void  PUTCON(char ch);


#define chkptr(foo)\
        if ((foo) != NULL) \
            cPrintf("\a\nDangling " #foo " = %Fp (%s:%u)\n",\
                    (foo), __FILE__,__LINE__)


/***** ACCT.C *****/ 
void  logincrement(void);
int  logincheck(void);
void  negotiate(void);
void  newaccount(void);
void  unlogthisAccount(void);
void  updatebalance(void);
void  calc_trans(long  time1,long  time2,char  trans);
 
/***** APLC.C *****/ 
void  ExeAplic(void);
void  shellescape(char  super);
void  apsystem(const char  *stuff);
void  wxsnd(const char  *path,const char  *file,char  trans);
void  wxrcv(const char  *path,const char  *file,char  trans);
 
/***** CFG.C *****/ 
void  configcit(void);
void  readaccount(void);
void  readprotocols(void);
 
/***** CRON.C *****/ 
void  readcron(void);
int  do_cron(int  why_called);
void  cron_commands(void);
void  did_net(const char  *callnode);
void illegal(const char *fmt, ...);

/***** CTDL.C *****/ 
int  main(int  argc,char  * *argv);
char  commandchar(void);
 
/***** EDIT.C *****/ 
unsigned char  getText(void);
void fakeFullCase(char *text);
void  GetFileMessage(FILE *fl,char  *str,int  mlen);
 
/***** FILE.C *****/ 
int  ambig(const char  *filename);
int  ambigUnlink(char  *filename,char  change);
void  attributes(void);
long  bytesfree(void);
int  checkfilename(const char  *filename,char  xtype);
void  dir(char  *filename,char  verbose, char which, char reverse);
double  dltime(long  size);
int  dump(const char  *filename);
int  dumpf(const char  *filename);
void  filldirectory(char  *filename,char  verbose,
                    char which, char reverse);
unsigned char  getattr(const char  _far *filename);
void  hello(void);
void  goodbye(void);
void  readdirectory(char  verbose, char which, char reverse);
void  renamefile(void);
void  setattr(const char  _far *filename,unsigned char  attr);
void  tutorial(char  *filename);
void  unlinkfile(void);
 
/***** FMT.C *****/ 
void  sformat(char  *str,const char  *fmt,const char  *val,...);
void  normalizeString(char  *s);
int  parse_it(char  * *words,char  *input);
int  qtext(char  *buf,const char  *line,char  end);
int  strpos(char  ch,const char  *str);
int  substr(const char  *str1,char  *str2);
int  u_match(const char  *s,char  *p);
 
/***** GRPH.C *****/ 
void  cleargroupgen(void);
void  defaulthall(void);
void  enterhall(void);
void  gotodefaulthall(void);
int  groupseeshall(int  hallslot);
int  groupseesroom(int  groupslot);
int  groupexists(const char  *groupname);
int  hallexists(const char  *hallname);
int  ingroup(int  groupslot);
int  iswindow(int  roomslot);
void  knownhalls(void);
void  getgroup(void);
int  partialgroup(const char  *groupname);
int  partialhall(const char  *hallname);
void  readhalls(void);
int  roominhall(int  roomslot);
void  setgroupgen(void);
void  stephall(int  direction);
 
/***** INFO.C *****/ 
void  entercomment(const char  *filename,const char  *uploader,
                   const char  *comment);
void  setfileinfo(void);
 
/***** INFO2.C *****/ 
void  killinfo(const char  *filename);
void readinfofile(char verbose, char which, char reverse);
void  updateinfo(void);
int  batchinfo(int  askuser);
void  moveFile(void);
 
/***** INPUT.C *****/ 
void  getNormStr(const char  *prompt,char  *s,int  size,char  doEcho);
long  getNumber(const char  *prompt,long  bottom,long  top,long  dfaultnum);
void  getString(const char  *prompt,char  *buf,int  lim,char  Qmark,
                char  doEcho,const char  *dfault);
int  getYesNo(char  *prompt,char  dfault);
int  BBSCharReady(void);
int  iChar(void);
void  setio(char  whichio,char  echo,char  outflag);
 
/***** LIB.C *****/ 
void  getGroup(void);
void  putGroup(void);
void  getHall(void);
void  putHall(void);
void  getLog(struct  logBuffer *lBuf,int  n);
void  putLog(const struct  logBuffer *lBuf,int  n);
void  getRoom(int  rm);
void  putRoom(int  rm);
int  readTables(void);
void  writeTables(void);
void  allocateTables(void);
 
/***** LOG.C *****/ 
int  findPerson(const char  *name,struct  logBuffer *lbuf);
int  personexists(const char  *name);
void  setdefaultconfig(void);
void  setlogconfig(void);
void  setsysconfig(void);
void  showconfig(const struct  logBuffer *lbuf);
void  slideLTab(int  slot);
void  storeLog(void);
void  displaypw(const char  *name,const char  *in,const char  *pw);
void  normalizepw(char  *InitPw,char  *Initials,char  *passWord);
int  pwexists(const char  *pw);
 
/***** LOG2.C *****/ 
void  login(const char  *initials,const char  *password);
void  setalloldrooms(void);
void  setlbvisit(void);
void  setroomgen(void);
void  terminate(char  discon,char  verbose);
void  initroomgen(void);
 
/***** LOG3.C *****/ 
void  forwardaddr(void);
void  killuser(void);
void  newPW(void);
void  Readlog(char  verbose,char  reverse);
void  showuser(void);
void  userEdit(void);
void  configure(unsigned char  new);
 
/***** MDM.C *****/ 
int  carrier(void);
void  carrdetect(void);
void  doccr(void);
void  domcr(void);
unsigned char  KBReady(void);
void  offhook(void);
void  outstring(const char  *string);
int  getModStr(char  *str);
 
/***** MISC.C *****/ 
void  crashout(const char  *message);
void  exitcitadel(void);
unsigned char  filexists(const char  *filename);
unsigned int  hash(const char  *str);
void  ctrl_c(void);
void  initCitadel(void);
void  openFile(const char  *filename,const struct  _iobuf * *fd);
void  trap(const char  *string,int  what);
void  SaveAideMess(void);
void  amPrintf(const char  *fmt,...);
void  amZap(void);
int  changedir(const char  *path);
char  *ltoac(long  num);
void  editBorder(void);
void  doBorder(void);
 
/***** MISC2.C *****/ 
void  systat(char  verbose);
void  chat(void);
void  ringSysop(void);
void  ringSystemREQ(void);
void  dial_out(void);
void  logo(void);
 
/***** MSG.C *****/ 
void  aideMessage(void);
void  specialMessage(void);
void  clearmsgbuf(void);
void  getMessage(void);
unsigned char  mAbort(void);
unsigned char  putMessage(void);
void  noteMessage(void);
void  indexmessage(unsigned long  here);
 
/***** MSG2.C *****/ 
int  indexslot(unsigned long  msgno);
unsigned char  mayseemsg(void);
unsigned char  mayseeindexmsg(int  slot);
void  changeheader(unsigned long  id,unsigned char  roomno,
                   unsigned char  attr);
void  crunchmsgTab(int  howmany);
unsigned char  dGetWord(char  *dest,int  lim);
int  getMsgChar(void);
void  getMsgStr(char  *dest,int  lim);
void  putMsgChar(char  c);
unsigned int  sizetable(void);
void  copyindex(int  dest,int  source);
void  dPrintf(const char  *fmt,...);
void  overwrite(int  bytes);
void  putMsgStr(const char  *string);
 
/***** MSG3.C *****/ 
void  insert(void);
unsigned char  makeMessage(void);
void  printMessage(unsigned long  id,char  verbose);
void  showMessages(char  whichMess,char  revOrder,char  verbose);

/***** NET_IN.C *****/
unsigned char  net_slave(void);

/***** NET_OUT.C *****/
BOOL net_callout(const char *node);
unsigned char  net_master(void);

/***** NETGMSG.C *****/
void  cleanup(void);
void  GetStr(struct  _iobuf *fl,char  *str,int  mlen);
unsigned char  master(void);

/***** NETNODE.C *****/
unsigned char  getnode(const char  *nodename);
unsigned char  readnode(void);

/***** NETPMSG.C *****/
void  PutMessage(struct  _iobuf *fl);
void  PutStr(struct  _iobuf *fl,const char  *str);
unsigned char  slave(void);

/***** NETROUTE.C *****/
unsigned char  alias(char  *str);
unsigned char  route(char  *str);
unsigned char  save_mail(void);

/***** OUT.C *****/ 
void  mFormat(char  *string);
void  putWord(const char  *st);
void  termCap(char  c);
void  doBS(void);
void  doCR(void);
void  dospCR(void);
void  echocharacter(char  c);
void  oChar(char  c);
void  mPrintf(const char  *fmt,...);
void  cPrintf(const char  *fmt,...);
void  cCPrintf(const char  *fmt,...);
void  prtList(const char  *item);
 
/***** PORT.C *****/ 
int  ringdetect(void);
int  MIReady(void);
void  Initport(void);
void  Hangup(void);
int  gotCarrier(void);
int  getMod(void);
void  drop_dtr(void);
void  baud(int  baudrate);
void  outMod(unsigned char  ch);
void  portInit(void);
void  portExit(void);
 
/***** ROOM.C *****/ 
int  gotoRoom(char  *roomname);
void  listRooms(unsigned int  what,char  verbose,char  numMess);
void  RoomStatus(void);
void  printrm(int  room);
char  *makeRoomName(int  room);
int  roomExists(const char  *room);
void  roomtalley(void);
void  givePrompt(void);
void  indexRooms(void);
void  noteRoom(void);
void  stepRoom(int  direction);
void  unGotoRoom(void);
 
/***** ROOM2.C *****/ 
void  formatSummary(char  *buffer);
void  killempties(void);
void  killroom(void);
void  makeRoom(void);
void  massdelete(void);
void  readbymsgno(void);
void  renameRoom(void);
 
/***** SYS.C *****/ 
void  globalverify(void);
void  msgNym(void);
void  sysopunlink(void);
 
/***** SYSG.C *****/ 
void  do_SysopGroup(void);
void  groupfunc(void);
void  listgroup(void);
 
/***** SYSH.C *****/ 
void  do_SysopHall(void);
void  hallfunc(void);
void  windowfunc(void);
 
/***** TIME.C *****/ 
void  changeDate(void);
int  dayofweek(void);
void  diffstamp(long  oldtime);
void  getdstamp(char  *buffer,unsigned int  stamp);
void  gettstamp(char  *buffer,unsigned int  stamp);
int  hour(void);
void  sstrftime(char  *outstr,int  maxsize,const char *formatstr,long  tnow);
void  pause(int  ptime);
int  systimeout(long  timer);
 
/***** UPDNLD.C *****/ 
void  entertextfile(void);
void  readtextfile(char  verbose);
void  download(char  c);
void  upload(char  c);
 
/***** WIND.C *****/ 
void  cls(void);
void  connectcls(void);
void  cursoff(void);
void  curson(void);
void  doccr(void);
int  gmode(void);
void  help(void);
void  outCon(char  c);
void _fastcall  position(unsigned char  row,unsigned char  column);
void  update25(void);
void  directstring(unsigned int  row,const char  *str,unsigned char  attr);
void  directchar(char  ch,unsigned char  attr);
void  biosstring(unsigned int  row,const char  *str,unsigned char  attr);
void  bioschar(char  ch,unsigned char  attr);
void  setscreen(void);
void  save_screen(void);
void  restore_screen(void);
void  ScreenFree(void);
 
/***** ZIP.C *****/ 
void  readzip(char  verbose);
char  zipview(char  *filename,char  verbose);
 
