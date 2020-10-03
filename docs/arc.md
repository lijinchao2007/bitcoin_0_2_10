## 【比特币0.2.10源码分析】项目框架
### [目录](../README.md)
### [项目地址](https://github.com/lijinchao2007/bitcoin_0_2_10)
### 入口模块init
    继承wxApp，实现了几个生命周期方法
        Initialize
        OnInit 解析命令参数,开启了多个模块任务的执行，单列详解

    OnInit详解
        BindListenPort 监听默认8333端口，等待其他node节点的连接
        LoadAddresses 加载node节点地址
        LoadBlockIndex 加载区块
        LoadWallet 加载钱包秘钥，交易信息等
        ReacceptWalletTransactions 未详细了解
        CreateMainWindow 创建wxwidgets的主界面
        StartNode 在net模块，node节点的处理都在此；单列详解
        ThreadRPCServer 监听8334端口，用于命令行对服务的rpc调用执行

### UI文件
    uibase.cpp是wxwidgets生成的
    ui.cpp是

    命令行版本
    通过wxUSE_GUI宏定义的关闭，开启这个版本的编译
### 主逻辑文件main
    定义了核心的类
        CTxIn 交易入
        CTxOut 交易出
        CTransaction，CMerkleTx，CWalletTx 层层继承的交易记录
        CBlock 区块，包含交易记录
        CBlockIndex 区块索引，只有交易记录的hashMerkleRoot记录
    
    定义了核心业务方法：节点间的消息收发ProcessMessages,SendMessages，挖矿方法BitcoinMiner都在此。
### 网络处理模块net
    定义了核心的网络类
        CMessageHeader 消息头
        CAddress 地址
        CInv 节点间block和tx广播使用的结构体
        CNode 网络节点类，封装了消息收发的逻辑
        CDataStream 虽然定义在serilize，但这个模块用的最多，用于消息网络传输时候的序列化
    
    网络处理的总入口StartNode
        ThreadIRCSeed 用于irc聊天室的注册和定义，主要为了获取比特币网络上存在节点的信息。在这个项目中，被我关闭了。
        ThreadSocketHandler 收发原始数据；接受连接的socket线程，创建CNode，标记入站请求节点，fInboundIn=true
        ThreadOpenConnections 根据获取的比特币节点信息，发起的socket连接，并创建CNode，标记非入站请求节点，fInboundIn=false
        ThreadMessageHandler 用于原始数据的上层逻辑分发。
        GenerateBitcoins 用于开关挖矿线程
        loop循环，用于监视错误，重启ThreadSocketHandler线程
### 其他
    其他模块不再详解，由前面的业务引出讲解
        base58模块 公钥的处理方法。比特币早一些的版本使用的是椭圆加密算法的公钥，这个版本已经使用公钥hash值（160位）的base58了。

        bignum模块 大数处理。 想了解比特币出块难度的实现，需要了解这个模块。

        db模块，包含了CTxDB区块索引存储，CAddrDB节点地址存储，CWalletDB秘钥，用户交易记录存储。这三着都继承在CDB，一个封装了berkeley-db的基础类

        irc模块，用于注册和获取节点地址

        key模块，对openssl的椭圆加密算法做了封装

        rpc模块，用于客户端命令调用rpc方法的处理

        script模块。定义了CScript类，是区块链签名的核心。定义的多种指令，是智能合约的基础。

        sha模块，挖矿方法调用的hash方法就在此处





