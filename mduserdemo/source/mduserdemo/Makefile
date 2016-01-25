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
target=.//mduserdemo

DEBUG_DEFINE=-DDEBUG -DDEBUG_LOG

APPEND_CPPFLAGS=-O3 -pthread -m64

PROFILE_CPPFLAGS=

WARNING_CPPFLAGS=-Wall -Wno-sign-compare

CPPFLAGS= -DDEBUG_NOT_LOG     $(APPEND_CPPFLAGS) $(PROFILE_CPPFLAGS) $(WARNING_CPPFLAGS) $(DEBUG_DEFINE) $(DEFINES)

LIBS= -lpthread -lrt     

DEBUG_LDFLAGS=-O3 -m64

MAP_LDFLAGS=

PROFILE_LDFLAGS=

WARNING_LDFLAGS=-Wall -Wno-sign-compare

LDFLAGS=      $(MAP_LDFLAGS) $(DEBUG_LDFLAGS) $(PROFILE_LDFLAGS) $(WARNING_LDFLAGS)

LIBARFLAGS=     -static $(MAP_LDFLAGS) $(DEBUG_LDFLAGS) $(PROFILE_LDFLAGS)

DLLARFLAGS=     -shared $(MAP_LDFLAGS) $(DEBUG_LDFLAGS) $(PROFILE_LDFLAGS)


all: code

code: clearoutput $(target)

clearoutput:
	@$(ECHO) Compiling... > output

femasmduserapi_obj=
femasmduserapi_lib=-L../../source/femasmduserapi/ -lUSTPmduserapi64
femasmduserapi_include= ../../source/femasmduserapi/USTPFtdcMduserApi.h  ../../source/femasmduserapi/USTPFtdcUserApiDataType.h  ../../source/femasmduserapi/USTPFtdcUserApiStruct.h 
femasmduserapi_includedir=-I../../source/femasmduserapi/.
mduserdemo_obj= ../../source/mduserdemo/MsClient.o  ../../source/mduserdemo/USTPMDHandler.o  ../../source/mduserdemo/USTPMDClient.o  ../../source/mduserdemo/profile.o 
mduserdemo_include= ../../source/mduserdemo/USTPMDHandler.h  ../../source/mduserdemo/USTPMDClient.h  ../../source/mduserdemo/profile.h 
mduserdemo_includedir=-I../../source/mduserdemo/.

all_objs= $(femasmduserapi_obj)  $(mduserdemo_obj) 
all_libs= $(femasmduserapi_lib)  $(mduserdemo_lib) 
INCLUDEDIR= $(femasmduserapi_includedir)  $(mduserdemo_includedir) 

../../source/mduserdemo/MsClient.o: ../../source/mduserdemo/MsClient.cpp $(mduserdemo_include)  
../../source/mduserdemo/USTPMDHandler.o: ../../source/mduserdemo/USTPMDHandler.cpp $(mduserdemo_include)  
../../source/mduserdemo/USTPMDClient.o: ../../source/mduserdemo/USTPMDClient.cpp $(mduserdemo_include)  
../../source/mduserdemo/profile.o: ../../source/mduserdemo/profile.cpp $(mduserdemo_include)  

copytargetall= 
pumptargetall= 

$(target): $(all_objs)
	$(LINK) $(LDFLAGS) -o $@ $(all_objs) $(all_libs) $(LIBS) >> output


clean:
	-$(DEL) $(femasmduserapi_obj)
	-$(DEL) $(mduserdemo_obj)
	-$(DEL) $(copytargetall)
	-$(DEL) $(pumptargetall)
	-$(DEL) $(target)

pump: $(pumptargetall)

