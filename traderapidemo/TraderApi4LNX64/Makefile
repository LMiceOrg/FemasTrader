.SUFFIXES:  .c .cpp .o .obj .a .lib

CPP=g++ -fpic
LINK=g++
LIB32=ar -ru
PREP=g++ -E -P
PUMP=pump
COPY=cp
DEL=rm
MAKE=make
ECHO=echo

.cpp.o:
	$(CPP) $(CPPFLAGS) $(INCLUDEDIR) -c $< -o $@ 2>> output


ISLIB=N
DEFINES=-DLINUX -DGCC
target=TraderApi

DEBUG_DEFINE=-DDEBUG -DDEBUG_LOG

APPEND_CPPFLAGS=-O3 -pthread -m64

PROFILE_CPPFLAGS=

WARNING_CPPFLAGS=-Wall -Wno-sign-compare

CPPFLAGS=     $(APPEND_CPPFLAGS) $(PROFILE_CPPFLAGS) $(WARNING_CPPFLAGS) $(DEBUG_DEFINE) $(DEFINES)

LIBS= -lpthread -lrt -L. -lUSTPtraderapi   

DEBUG_LDFLAGS=-O3 -m64

MAP_LDFLAGS=

PROFILE_LDFLAGS=

WARNING_LDFLAGS=-Wall -Wno-sign-compare

LDFLAGS=     $(MAP_LDFLAGS) $(DEBUG_LDFLAGS) $(PROFILE_LDFLAGS) $(WARNING_LDFLAGS)

LIBARFLAGS=    -static $(MAP_LDFLAGS) $(DEBUG_LDFLAGS) $(PROFILE_LDFLAGS)

DLLARFLAGS=    -shared $(MAP_LDFLAGS) $(DEBUG_LDFLAGS) $(PROFILE_LDFLAGS)


all: code

code: clearoutput $(target)

clearoutput:
	@$(ECHO) Compiling... > output

TraderApi_obj= ./testapi.o  ./TraderSpi.o  ./PublicFuncs.o 
TraderApi_include=
TraderApi_includedir=-I./.

all_objs= $(TraderApi_obj) 
all_libs= $(TraderApi_lib) 
INCLUDEDIR= $(TraderApi_includedir) 

./testapi.o: ./testapi.cpp $(TraderApi_include)  

./TraderSpi.o: ./TraderSpi.cpp $(TraderApi_include)  

./PublicFuncs.o: ./PublicFuncs.cpp $(TraderApi_include)  



$(target): $(all_objs)
	$(LINK) $(LDFLAGS) -o $@ $(all_objs) $(all_libs) $(LIBS) >> output


clean:
	-$(DEL) $(TraderApi_obj)
	-$(DEL) $(target)

pump:  

