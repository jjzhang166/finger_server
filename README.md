


##说明：

```
FINGER_SERVER
├─bin    # 程序发布位置
├─common # sqlite3的封装模块，数据操作模块，一些公共的处理函数
├─CppClient         # 测试客户端 
│  ├─RegistClient   # 注册客户端
│  └─ValidateClient  # 认证客户端
├─lib               # 生成的依赖库
├─prj_linux         # linux makefile的文件夹
├─RegistServer      # 注册服务器
├─thirdparty        # 依赖的第三方库
│  ├─poco           # 依赖的poco C++库，使用的1.7.4版本（https://pocoproject.org/）
│  │  ├─Foundation
│  │  └─Net
└─ValidateServer     # 认证客户端
```
=============================================


##安装和编译
首次安装运行这个脚本,建议使用root用户权限执行
```
chmod +x install_and_compile.sh
./install_and_compile.sh
```

下面为单独的编译脚本
```
chmod +x compile.sh.sh
./compile.sh.sh
```

###安装脚本解释
```
#/bin/bash

# 解压并安装poco库
tar xzvf poco-1.7.6.tar.gz
cd poco-1.7.6
./configure --no-tests --no-samples --minimal
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

# 编译服务器程序
cd prj_linux
make 
cd ..

# 给可执行程序增加可执行权限
cd bin
chmod +x *
cd ..

```

=============================================

##使用
###RegistServer
```
useage : app <dbPath> <port-(optinal)>
如：
./RegistServer /data/test.db 5000
```

###ValidateServer
```
useage : app <dbPath> <port-(optinal)>
如：
./ValidateServer /data/test.db 5001
```

###RegistClient
```
useage : app <dbPath> <port-(optinal)>
如：
./RegistClient 192.168.1.101 5000 1.info
```

###ValidateClient
```
useage : app <dbPath> <port-(optinal)>
如：
./ValidateClient 192.168.1.101 5001 1.info
```
===============================================

## 服务端设计思想
主要采用reactor网络模式，使用了POCO开源库中的网络模块。
基于std的vector<char>设计了消息缓存队列。
Cppsqlite3为对sqlite3接口的封装，而DBhelper是基于业务层的封装。

每一个链接都会新建新的ServiceHandler,如注册服务端的RegistServiceHandler,一个RegistServiceHandler对应一个task。
这样就保证了验证服务器在并发的客户端请求不会被阻塞，因为这是在不同的task中并行的验证指纹。

对于每个链接都会有产生如下的对象
```
struct Client
{
	Buffer			inputBuffer;       // 输入消息队列
	Buffer			outputBuffer;      // 输出消息队列
	StreamSocket	socket;            // socket实例
};
```
输入和输出队列的独立，确保了消息不会阻塞。设计每个链接都有两个消息队列，可以避免不同锁的存在，因为锁非常耗CPU.

###注册服务端
注册服务端考虑到只能单用户操作，使用到了锁机制。当正在进行数据库修改时，新用户的链接会被阻塞，只到锁的释放，这样保证了数据的一致性。
新用户链接的同时，会释放上一个链接。

###验证服务端
验证服务端支持多并发，相当于读取数据库的操作。
通过数据库迭代器轮询数据库数据，逐个比较指纹数据。
当找到后退出轮询，否则只到整个数据库结束。



