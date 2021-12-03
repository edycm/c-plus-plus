CXX = g++
CXXFLAGS = -g -std=c++11

TARGET1 = kafka_consumer_test
TARGET2 = kafka_producer_test
OBJ_PATH = ./obj/

PWD := $(shell pwd)

LIB_PATH := -L../lib
INC_PATH := -I../include

SRCS := $(wildcard ./*.cpp)

LIBS := -lrt -lz -lm -ldl -lpthread  -lrdkafka++ -lrdkafka
LIBS += -Wl,-rpath=../lib

TMP_FILE = $(notdir $(SRCS))
OBJS := $(addprefix $(OBJ_PATH), $(TMP_FILE:%.cpp=%.o))

PRODUCER_OBJS := $(filter-out ./obj/KafkaConsumerTest.o,$(OBJS))
CONSUMER_OBJS := $(filter-out ./obj/KafkaProducerTest.o,$(OBJS))

all:
	make $(TARGET1)
	make $(TARGET2)

$(TARGET1): $(CONSUMER_OBJS)
	$(CXX) $(CXXFLAGS) $(INC_PATH) $^ -o $@ $(LIB_PATH) $(LIBS)

$(TARGET2): $(PRODUCER_OBJS)
	$(CXX) $(CXXFLAGS) $(INC_PATH) $^ -o $@ $(LIB_PATH) $(LIBS)

$(OBJ_PATH)%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c $(INC_PATH) $< -o $@


clean: 
	@rm -f $(TARGET1)
	@rm -f $(TARGET2)
	@rm -f $(OBJS)

.PHONY : all exe clean
