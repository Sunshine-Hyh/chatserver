# 设置要编译的源文件列表：要编译的源文件太多，一个个添加到add_executable()列表中麻烦，使用下面函数将多个源文件放到源文件列表dir中
aux_source_directory(. SRC_LIST) # 把当前路径下的所有源文件名字放入变量SRC_LIST里面
aux_source_directory(./db DB_LIST) # 把当前路径db文件夹下的所有源文件名字放入变量DB_LIST里面
aux_source_directory(./model MODEL_LIST) # 把当前路径model文件夹下的所有源文件名字放入变量MODEL_LIST里面
aux_source_directory(./redis REDIS_LIST) # 把当前路径redis文件夹下的所有源文件名字放入变量REDIS_LIST里面

# 将STR_LIST  DB_LIST变量所定义的源文件编译成可执行文件——ChatServer
add_executable(ChatServer ${SRC_LIST} ${DB_LIST} ${MODEL_LIST} ${REDIS_LIST})

# 指定可执行文件链接时需要链接muduo_net muduo_base pthread这三个库文件
target_link_libraries(ChatServer muduo_net muduo_base mysqlclient hiredis pthread)