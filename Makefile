CC = g++
CXXFLAGS = -std=c++11 -g

LIBS =
INC_PATH = -I./


SRCS = $(wildcard ./*.cpp)
OBJS = $(SRCS:%.cpp=%.o)

TARGET = bitmap_test


all:
	@make exe

depend:
	$(CXX) $(CXXFLAGS) $(SRCS) -M > MAKEFILE.DEPEND

exe: $(TARGET)

$(TARGET) : $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIB_PATH)

clean:
	@rm -f $(OBJS)
	@rm -f $(TARGET)


.PHONY: all exe clean
