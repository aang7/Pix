# g++ `sdl2-config --cflags` -I ../.. main.cpp imgui_impl_sdl_gl2.cpp ../../imgui*.cpp `sdl2-config --libs` -lGL -o sdl2example

EXE = pixout
COMPILER = -std=c++11
OBJS = main.o imgui_impl_sdl_gl2.o
OBJS += ../../imgui/imgui.o ../../pixui.o ../../tinyfiledialogs.o ../../imgui/imgui_draw.o

UNAME_S := $(shell uname -s)


ifeq ($(UNAME_S), Linux) #LINUX
	ECHO_MESSAGE = "Linux"
	LIBS = -lGL `sdl2-config --libs` `pkg-config --cflags --libs opencv`  -lpthread
	CXXFLAGS = -I ../../imgui `sdl2-config --cflags` 
	CXXFLAGS += -Wall -Wformat
	CFLAGS = $(CXXFLAGS)
endif


.cpp.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(ECHO_MESSAGE)

$(EXE): $(OBJS)
	$(CXX) $(COMPILER) -o $(EXE) $(OBJS) $(CXXFLAGS) $(LIBS)

clean:
	rm $(EXE) $(OBJS)

