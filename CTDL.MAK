PROJ = CTDL
PROJFILE = CTDL.MAK
DEBUG = 0

PWBRMAKE  = pwbrmake
NMAKEBSC1  = set
NMAKEBSC2  = nmake
CC  = cl
CFLAGS_G  = /AL /W4 /BATCH /Gt /FR$*.sbr
CFLAGS_D  = /qc /Od /Zi /Zr
CFLAGS_R  = /Ot /Ol /Og /Oe /Oi /Gs /Gm
ASM  = ml
AFLAGS_G  = /W3 /WX /Zm /FR$*.sbr
AFLAGS_D  = /Zi
AFLAGS_R  = /nologo
MAPFILE_D  = $(PROJ).map
MAPFILE_R  = $(PROJ).map
LFLAGS_G  =  /NOI /STACK:16288  /BATCH
LFLAGS_D  =  /CO /M /FAR /NOP
LFLAGS_R  =  /M /EXE /FAR /NOP
LINKER  = link
ILINK  = ilink
LRF  = echo > NUL
NMFLAGS  =                                              
BRFLAGS  =  /o $(PROJ).bsc /Es /Iu
BROWSE  = 1
CVFLAGS  =  /50 /X /F

OBJS  = ACCT.obj APLC.obj CFG.obj CRON.obj CTDL.obj EDIT.obj FILE.obj FMT.obj\
        GRPH.obj INFO.obj INFO2.obj INPUT.obj LIB.obj LOG.obj LOG2.obj\
        LOG3.obj MDM.obj MISC.obj MISC2.obj MSG.obj MSG2.obj MSG3.obj OUT.obj\
        PORT.obj ROOM.obj ROOM2.obj SYS.obj SER.obj SYSG.obj SYSH.obj TIME.obj\
        UPDNLD.obj VRSN.obj WIND.obj XLATE.obj ZIP.obj NETGMSG.obj NETNODE.obj\
        NETPMSG.obj NETROUTE.obj NET_IN.obj NET_OUT.obj
SBRS  = ACCT.sbr APLC.sbr CFG.sbr CRON.sbr CTDL.sbr EDIT.sbr FILE.sbr FMT.sbr\
        GRPH.sbr INFO.sbr INFO2.sbr INPUT.sbr LIB.sbr LOG.sbr LOG2.sbr\
        LOG3.sbr MDM.sbr MISC.sbr MISC2.sbr MSG.sbr MSG2.sbr MSG3.sbr OUT.sbr\
        PORT.sbr ROOM.sbr ROOM2.sbr SYS.sbr SER.sbr SYSG.sbr SYSH.sbr TIME.sbr\
        UPDNLD.sbr VRSN.sbr WIND.sbr XLATE.sbr ZIP.sbr NETGMSG.sbr NETNODE.sbr\
        NETPMSG.sbr NETROUTE.sbr NET_IN.sbr NET_OUT.sbr

all: $(PROJ).exe

.SUFFIXES:
.SUFFIXES: .sbr .obj .c .asm

ACCT.obj : ACCT.C ctdl.h prot.h glob.h

ACCT.sbr : ACCT.C ctdl.h prot.h glob.h

APLC.obj : APLC.C spawno.h ctdl.h prot.h glob.h aplc.h apst.h

APLC.sbr : APLC.C spawno.h ctdl.h prot.h glob.h aplc.h apst.h

CFG.obj : CFG.C ctdl.h prot.h key.h glob.h

CFG.sbr : CFG.C ctdl.h prot.h key.h glob.h

CRON.obj : CRON.C ctdl.h key.h prot.h glob.h

CRON.sbr : CRON.C ctdl.h key.h prot.h glob.h

CTDL.obj : CTDL.C ctdl.h key.h prot.h glob.h

CTDL.sbr : CTDL.C ctdl.h key.h prot.h glob.h

EDIT.obj : EDIT.C ctdl.h prot.h glob.h

EDIT.sbr : EDIT.C ctdl.h prot.h glob.h

FILE.obj : FILE.C ctdl.h prot.h glob.h

FILE.sbr : FILE.C ctdl.h prot.h glob.h

FMT.obj : FMT.C ctdl.h prot.h glob.h

FMT.sbr : FMT.C ctdl.h prot.h glob.h

GRPH.obj : GRPH.C ctdl.h prot.h glob.h

GRPH.sbr : GRPH.C ctdl.h prot.h glob.h

INFO.obj : INFO.C ctdl.h prot.h glob.h

INFO.sbr : INFO.C ctdl.h prot.h glob.h

INFO2.obj : INFO2.C ctdl.h prot.h glob.h

INFO2.sbr : INFO2.C ctdl.h prot.h glob.h

INPUT.obj : INPUT.C ctdl.h prot.h glob.h

INPUT.sbr : INPUT.C ctdl.h prot.h glob.h

LIB.obj : LIB.C ctdl.h prot.h glob.h

LIB.sbr : LIB.C ctdl.h prot.h glob.h

LOG.obj : LOG.C ctdl.h prot.h glob.h

LOG.sbr : LOG.C ctdl.h prot.h glob.h

LOG2.obj : LOG2.C ctdl.h prot.h glob.h

LOG2.sbr : LOG2.C ctdl.h prot.h glob.h

LOG3.obj : LOG3.C ctdl.h prot.h glob.h

LOG3.sbr : LOG3.C ctdl.h prot.h glob.h

MDM.obj : MDM.C ctdl.h prot.h glob.h

MDM.sbr : MDM.C ctdl.h prot.h glob.h

MISC.obj : MISC.C ctdl.h prot.h glob.h

MISC.sbr : MISC.C ctdl.h prot.h glob.h

MISC2.obj : MISC2.C ctdl.h prot.h glob.h

MISC2.sbr : MISC2.C ctdl.h prot.h glob.h

MSG.obj : MSG.C ctdl.h key.h prot.h glob.h

MSG.sbr : MSG.C ctdl.h key.h prot.h glob.h

MSG2.obj : MSG2.C ctdl.h prot.h glob.h

MSG2.sbr : MSG2.C ctdl.h prot.h glob.h

MSG3.obj : MSG3.C ctdl.h prot.h glob.h

MSG3.sbr : MSG3.C ctdl.h prot.h glob.h

OUT.obj : OUT.C ctdl.h prot.h glob.h

OUT.sbr : OUT.C ctdl.h prot.h glob.h

PORT.obj : PORT.C ctdl.h prot.h glob.h

PORT.sbr : PORT.C ctdl.h prot.h glob.h

ROOM.obj : ROOM.C ctdl.h prot.h glob.h

ROOM.sbr : ROOM.C ctdl.h prot.h glob.h

ROOM2.obj : ROOM2.C ctdl.h prot.h glob.h

ROOM2.sbr : ROOM2.C ctdl.h prot.h glob.h

SYS.obj : SYS.C ctdl.h prot.h glob.h

SYS.sbr : SYS.C ctdl.h prot.h glob.h

SER.obj : SER.ASM

SER.sbr : SER.ASM

SYSG.obj : SYSG.C ctdl.h prot.h glob.h

SYSG.sbr : SYSG.C ctdl.h prot.h glob.h

SYSH.obj : SYSH.C ctdl.h prot.h glob.h

SYSH.sbr : SYSH.C ctdl.h prot.h glob.h

TIME.obj : TIME.C ctdl.h prot.h glob.h

TIME.sbr : TIME.C ctdl.h prot.h glob.h

UPDNLD.obj : UPDNLD.C ctdl.h prot.h glob.h

UPDNLD.sbr : UPDNLD.C ctdl.h prot.h glob.h

VRSN.obj : VRSN.C

VRSN.sbr : VRSN.C

WIND.obj : WIND.C ctdl.h prot.h glob.h

WIND.sbr : WIND.C ctdl.h prot.h glob.h

XLATE.obj : XLATE.C

XLATE.sbr : XLATE.C

ZIP.obj : ZIP.C ctdl.h prot.h glob.h

ZIP.sbr : ZIP.C ctdl.h prot.h glob.h

NETGMSG.obj : NETGMSG.C ctdl.h key.h prot.h glob.h

NETGMSG.sbr : NETGMSG.C ctdl.h key.h prot.h glob.h

NETNODE.obj : NETNODE.C ctdl.h key.h prot.h glob.h

NETNODE.sbr : NETNODE.C ctdl.h key.h prot.h glob.h

NETPMSG.obj : NETPMSG.C ctdl.h prot.h glob.h

NETPMSG.sbr : NETPMSG.C ctdl.h prot.h glob.h

NETROUTE.obj : NETROUTE.C ctdl.h prot.h glob.h

NETROUTE.sbr : NETROUTE.C ctdl.h prot.h glob.h

NET_IN.obj : NET_IN.C ctdl.h prot.h glob.h

NET_IN.sbr : NET_IN.C ctdl.h prot.h glob.h

NET_OUT.obj : NET_OUT.C ctdl.h prot.h glob.h

NET_OUT.sbr : NET_OUT.C ctdl.h prot.h glob.h


$(PROJ).bsc : $(SBRS)
        $(PWBRMAKE) @<<
$(BRFLAGS) $(SBRS)
<<

$(PROJ).exe : $(OBJS)
!IF $(DEBUG)
        $(LRF) @<<$(PROJ).lrf
$(RT_OBJS: = +^
) $(OBJS: = +^
)
$@
$(MAPFILE_D)
$(LLIBS_G: = +^
) +
$(LLIBS_D: = +^
) +
$(LIBS: = +^
)
$(DEF_FILE) $(LFLAGS_G) $(LFLAGS_D);
<<
!ELSE
        $(LRF) @<<$(PROJ).lrf
$(RT_OBJS: = +^
) $(OBJS: = +^
)
$@
$(MAPFILE_R)
$(LLIBS_G: = +^
) +
$(LLIBS_R: = +^
) +
$(LIBS: = +^
)
$(DEF_FILE) $(LFLAGS_G) $(LFLAGS_R);
<<
!ENDIF
!IF $(DEBUG)
        $(LINKER) @$(PROJ).lrf
!ELSE
        $(LINKER) @response
!ENDIF
        $(NMAKEBSC1) MAKEFLAGS=
        $(NMAKEBSC2) $(NMFLAGS) -f $(PROJFILE) $(PROJ).bsc


.c.sbr :
!IF $(DEBUG)
        $(CC) /Zs $(CFLAGS_G) $(CFLAGS_D) /FR$@ $<
!ELSE
        $(CC) /Zs $(CFLAGS_G) $(CFLAGS_R) /FR$@ $<
!ENDIF

.asm.sbr :
!IF $(DEBUG)
        $(ASM) /Zs $(AFLAGS_G) $(AFLAGS_D) /FR$@ $<
!ELSE
        $(ASM) /Zs $(AFLAGS_G) $(AFLAGS_R) /FR$@ $<
!ENDIF

.c.obj :
!IF $(DEBUG)
        $(CC) /c $(CFLAGS_G) $(CFLAGS_D) /Fo$@ $<
!ELSE
        $(CC) /c $(CFLAGS_G) $(CFLAGS_R) /Fo$@ $<
!ENDIF

.asm.obj :
!IF $(DEBUG)
        $(ASM) /c $(AFLAGS_G) $(AFLAGS_D) /Fo$@ $<
!ELSE
        $(ASM) /c $(AFLAGS_G) $(AFLAGS_R) /Fo$@ $<
!ENDIF


run: $(PROJ).exe
        $(PROJ).exe $(RUNFLAGS)

debug: $(PROJ).exe
        CV $(CVFLAGS) $(PROJ).exe $(RUNFLAGS)
