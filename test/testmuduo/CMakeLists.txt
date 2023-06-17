# 配置头文件搜索路径
#include_directories()
# 配置库文件搜索路径
#link_directories()

# 设置要编译的源文件列表：要编译的源文件太多，一个个添加到add_executable()列表中麻烦，使用下面函数将多个源文件放到源文件列表dir中
# aux_source_directory(. SRC_LIST) 把当前路径下的所有源文件名字放入变量SRC_LIST里面
set(SRC_LIST ./muduo_server.cpp)

# 设置可执行文件最终输出时候存储的路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 将STR_LIST变量所定义的源文件编译成server可执行文件
add_executable(server ${SRC_LIST})
# server目标程序需要连接muduo_net muduo_base pthread这三个库文件
target_link_libraries(server muduo_net muduo_base pthread)