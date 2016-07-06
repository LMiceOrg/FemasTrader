
CXX=g++
OUT_NAME=guava_demo

OUT_PATH=./bin/linux/
SRC_PATH=./
OBJ_PATH=./obj/

PUBLISH_PATH=../../bin/linux/debug/

EES_LIB_INCLUDE_PATH=./
EES_LIB_LIB_PATH=../bin/linux/


EES_QUOTE_DEMO_FILE=-lpthread \
		-ldl \


SRC_FILE=$(wildcard $(SRC_PATH) *.cpp)
ALL_FILE=$(notdir $(SRC_FILE))
OBJ_FILE=$(patsubst %.cpp,%.o,$(ALL_FILE))

DEBUG_CXXFLAGS=-g -Wall -DDEBUG   


OUT_DEBUG_FILE=$(OUT_PATH)$(OUT_NAME)
OBJ_DEBUG_FILE=$(addprefix  $(OBJ_PATH),$(notdir $(OBJ_FILE)))


.PHONY: clean

debug : pre_debug $(OUT_DEBUG_FILE)

publish : pre_publish


pre_debug :
	-$(shell mkdir $(OBJ_PATH) -p)
	-$(shell mkdir $(OUT_PATH) -p)
	
$(OUT_DEBUG_FILE) : $(OBJ_DEBUG_FILE)
	$(CXX) $(DEBUG_CXXFLAGS) $(addprefix  $(OBJ_PATH),$(notdir $^)) -o $@ $(EES_QUOTE_DEMO_FILE) 
	
$(OBJ_PATH)%.o : %.cpp
	$(CXX) -c $(DEBUG_CXXFLAGS)  -I $(EES_LIB_INCLUDE_PATH)  $(SRC_PATH)$< -o $@
	
pre_publish:
	$(shell cp $(OUT_PATH)$(OUT_NAME) $(PUBLISH_PATH) -f)
	@echo "make publish"	
clean :
	@echo  "make clean"
	$(shell rm $(OBJ_PATH) -rf)
	$(shell rm $(OUT_PATH)$(OUT_NAME) -f)

