# 定义变量
CXX = g++
CXXFLAGS = -std=c++17 -I./include -g -w
LDFLAGS = -lglog -lssl -lcrypto
SRC_DIR = ./src
OBJ_DIR = ./obj
BIN_DIR = ./bin
LOG_DIR = ./logs

# 源文件
CLIENT_SRC = $(SRC_DIR)/client/client.cpp
SERVER_SRC = $(SRC_DIR)/server/server.cpp
PACKET_SRC = $(SRC_DIR)/packet/packet.cpp

# 目标文件
CLIENT_OBJ = $(OBJ_DIR)/client.o $(OBJ_DIR)/packet.o
SERVER_OBJ = $(OBJ_DIR)/server.o $(OBJ_DIR)/packet.o

# 可执行文件
CLIENT_BIN = $(BIN_DIR)/client
SERVER_BIN = $(BIN_DIR)/server

# 默认目标
all: setup $(CLIENT_BIN) $(SERVER_BIN)

# 设置环境（创建目录）
setup:
	mkdir -p $(OBJ_DIR) $(BIN_DIR) $(LOG_DIR)

# 编译 client 程序
$(CLIENT_BIN): $(CLIENT_OBJ)
	$(CXX) $^ $(LDFLAGS) -o $@

# 编译 server 程序
$(SERVER_BIN): $(SERVER_OBJ)
	$(CXX) $^ $(LDFLAGS) -o $@

# 编译目标文件
$(OBJ_DIR)/%.o: $(SRC_DIR)/*/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理生成的文件
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(LOG_DIR)

# 伪目标
.PHONY: all setup clean
