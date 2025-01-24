MAKE_MODE = debug

CC = g++
warnings = -pedantic -Wall -Wextra -Werror
flags := $(warnings) -std=c++20 -O3

SRC_DIR = ./src
OBJ_DIR =./obj
SRC_FILES := $(wildcard $(SRC_DIR)/**/*.cpp) $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC_FILES))

ifeq ($(MAKE_MODE), debug)
	flags := $(flags) -ggdb3
endif

main: $(OBJS)
ifeq ($(MAKE_MODE), debug)
else ifeq ($(MAKE_MODE), build)
else
	echo "Error: unknown make mode \"$(MAKE_MODE)\""
	exit 1
endif
	$(CC) -o main.exe $(OBJS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CC) -c $< -o $@ $(flags)

.PHONY: clean
clean:
	cd obj
	find . -name "*.o" -type f -delete
	cd ../

.PHONY: rebuild
rebuild:
	make clean
	make