CC = g++
flags = -Wall -ggdb3 -std=c++17 -O3

SRC_DIR = ./src
OBJ_DIR =./obj
SRC_FILES := $(wildcard $(SRC_DIR)/**/*.cpp) $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC_FILES))


main: $(OBJS)
	echo $(SRC_FILES)
	$(CC) -o main.exe $(OBJS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CC) -c $< -o $@ $(flags)