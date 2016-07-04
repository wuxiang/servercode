
# C Compiler include & library path setting
CC          = ${CC_COMPILER} ${CC_CFLAGS} ${CC_IPATH}
LN          = ${CC_COMPILER}
PC          = ${PC_COMPILER} ${PC_FLAGS}  ${PC_IPATH} ${PC_LPATH}
LB          = ar -r
CP          = cp
RM          = rm -f
MV          = mv


#-------------------------------------------------

.SUFFIXES:
.SUFFIXES: .o .c
.SUFFIXES: .o .cpp
.SUFFIXES: .o .pc
.SUFFIXES: .obj .c
.SUFFIXES: .obj .cpp
.SUFFIXES: .obj .pc
.SUFFIXES: .c .pc
.c.o:
	$(CC) -o $(<:.c=.o) -c $<
.cpp.o:
	$(CC) -o $(<:.cpp=.o) -c $<
.pc.o:
	$(PC) $<
	$(CC) -c $(<:.pc=.c)
	$(RM) $(<:.pc=.c) *.lis
.c.obj:
	$(CC) -c $<
.cpp.obj:
	$(CC) -c $<
.pc.obj:
	$(PC) $<
	$(CC) -c $(<:.pc=.c)
.pc.c:
	$(PC) $<


#-------------------------------------------------

TARGET      = _convert

AIM_PATH    = $(DIST_AIM_PATH)

SHARE_OBJS  =

NEW_OBJS    = Convert.o 


#-------------------------------------------------

all: $(TARGET)

$(TARGET): build
#	$(LN) $(NEW_OBJS) $(SHARE_OBJS) ${CC_LPATH} -o $@

build: $(NEW_OBJS)

clear:
	@$(RM) $(TARGET) $(NEW_OBJS) *.o *.obj *.lis *.pdb *.ilk

setup: $(TARGET)
	@$(CP) $(SHARE_OBJS) $(NEW_OBJS) $(AIM_PATH)

clean:
	@$(RM) $(TARGET) $(NEW_OBJS)


#-------------------------------------------------
