#/bin/bash


# 编译服务器程序
cd prj_linux
make 
cd ..

# 给可执行程序增加可执行权限
cd bin
chmod +x *
cd ..

