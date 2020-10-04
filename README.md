# bitcoin_0_2_10

## bitoin 0.2.10版本，做了一些修改

* mac下make -f makefile.osx可运行，其他环境未尝试
* 修改初始的nBits，大概一百万次找到一个nNonce
* 注释irc
* 修改增加-port，可以本地组成网络
* 增加一些日志查看
* 修改wxdgets的一些页面，便于查看
* TODO:tx详情展示完整信息
* TODO:增加本地钱包的秘钥查看
* TODO:tx交易的鉴权过程


# 比特币0.2.10代码解析

## [前言](./docs/start.md)
## [项目运行](./docs/running.md)
## [项目框架](./docs/arc.md)
## [比特币的一些疑惑](./docs/questions.md)
***
* - [ ]  [节点管理](./docs/nodes.md)
* - [ ]  [数据同步](./docs/data_sync.md)
* - [ ]  [挖矿过程](./docs/miner.md)
* - [ ]  [存储](./docs/db.md)
* - [x]  [交易流程](./docs/transaction.md)

