# 设置要编译的源文件列表：要编译的源文件太多，一个个添加到add_executable()列表中麻烦，使用下面函数将多个源文件放到源文件列表dir中
aux_source_directory(. SRC_LIST) # 把当前路径下的所有源文件名字放入变量SRC_LIST里面

# 将STR_LIST 变量所定义的源文件编译成可执行文件——ChatClient
add_executable(ChatClient ${SRC_LIST})

# 指定可执行文件链接时需要链接 thread 这个库文件
target_link_libraries(ChatClient pthread)