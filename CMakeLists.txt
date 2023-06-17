# 要求的cmake最低的版本号
cmake_minimum_required(VERSION 3.0)

# 定义当前工程名字
# project(chat)

# 配置编译选项：在CMAKE_CXX_FLAGS基础上添加 -g生成可调试文件
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)

# 设置可执行文件最终输出时候存储的路径：工程文件的根目录下的bin文件夹下
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 配置头文件搜索路径
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/thirdparty)
include_directories(${PROJECT_SOURCE_DIR}/include/server)
include_directories(${PROJECT_SOURCE_DIR}/include/server/db)
include_directories(${PROJECT_SOURCE_DIR}/include/server/model)
include_directories(${PROJECT_SOURCE_DIR}/include/server/redis)

# 指定搜索的子目录:可编译的源码都在src目录下
add_subdirectory(src)

# 项目编译构建时，先构建本文件，接着会到src下面的cmakelist下进行构建

