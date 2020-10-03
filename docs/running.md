## 【比特币0.2.10源码分析】项目运行
### [目录](../README.md)
### [项目地址](https://github.com/lijinchao2007/bitcoin_0_2_10)

### makefile
    0.2.10比起早起的已经支持多个系统，这个项目是基于0.2.10的代码，做了一定的修改。

    makefile.osx中主要实现了ui和无ui程序的编译。

    make -f makefile.osx bitcoin
    make -f makefile.osx bitcoind


### 项目相关库
* boost@1.55
* openssl使用了1.0.2t，brew支持的最低版本跑步起来。所以找了这个更低版本才把程序跑起来
* berkeley-db@4
* wxwidgets用了3.0版本

### 项目相关库升级
>以上依赖基本都跟0.2.10的依赖版本不同了，毕竟过了10年，变化太大。好在兜兜转转，把程序运行了起来

>在后来对界面做些修改的时候，碰到不少麻烦。wxwidgets这个可与Qt相比的跨平台UI库，对我来说是第一次接触，不知道怎么开发。网上搜到用code::blocks，结果官网上mac版本是几年前发布的了，还是32位的程序，不支持64位的mac。好不容易找到一个64位版本，却一直崩溃。一度想放弃，后来找到wxFormbuider，可以对ui配置fbp文件修改，生成UI代码，才继续了下去。但也要花些时间，学习UI的开发。wxwidgets跟Qt，MFC之类的有些相似，所以了解起来也是很快。

>最终，比特币0.2.10版本的代码，主要的cpp文件才10个，一万多行代码，修改起来并不太困难。
