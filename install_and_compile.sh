#/bin/bash

# 解压并安装poco库
tar xzvf poco-1.7.6.tar.gz
cd poco-1.7.6
./configure --no-tests --no-samples --minimal --static
make 
sudo make install
cd ..

# 安装sqlite3
tar xzvf sqlite-autoconf-3150200.tar.gz
cd sqlite-autoconf-3150200
./configure 
make 
sudo make install
cd ..

# 拷贝认证模块库
sudo cp -f thirdparty/libGBFPIA.so /usr/local/lib

# 编译服务器程序
cd prj_linux
make 
cd ..

# 给可执行程序增加可执行权限
cd bin
chmod +x *
cd ..

# 删除多余的文件
rm -rf poco-1.7.6*
rm -rf sqlite-autoconf-3150200*


