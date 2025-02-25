# debug, prof, build
MAKE_MODE := build

CC = g++
warnings = -pedantic -Wall -Wextra -Werror
flags := $(warnings) -std=c++20 -O3

# flags used in compilation and linking
common_flags := 

SRC_DIR = ./src
OBJ_DIR =./obj
SRC_FILES := $(wildcard $(SRC_DIR)/**/*.cpp) $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC_FILES))
EXECUTABLE = main.exe

ifeq ($(MAKE_MODE), debug)
	flags := $(flags) -ggdb3
endif
ifeq ($(MAKE_MODE), prof)
	common_flags := $(common_flags) -pg
endif

main: $(OBJS)
ifeq ($(MAKE_MODE), debug)
else ifeq ($(MAKE_MODE), prof)
else ifeq ($(MAKE_MODE), build)
else
	echo "Error: unknown make mode \"$(MAKE_MODE)\""
	exit 1
endif
	$(CC) -o $(EXECUTABLE) $(OBJS) $(common_flags)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CC) -c $< -o $@ $(flags) $(common_flags)

.PHONY: clean
clean:
	cd obj
	find . -name "*.o" -type f -delete
	cd ../

.PHONY: rebuild
rebuild:
	make clean
	make

.PHONY: profile
profile:
ifneq ($(MAKE_MODE), prof)
	@echo "Cannot run profiler if build mode is not set to \"prof\""
	exit 1
endif
	@echo "Running profiler"
	./main.exe run example.sg
	gprof ./main.exe gmon.out > perf/analysis.txt
	rm gmon.out