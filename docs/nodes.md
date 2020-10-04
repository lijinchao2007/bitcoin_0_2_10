## 【比特币0.2.10源码分析】节点管理
### [目录](../README.md)
### [项目地址](https://github.com/lijinchao2007/bitcoin_0_2_10)

> ThreadSocketHandler是收发原始数据；接受连接的socket线程，创建CNode，标记入站请求节点，fInboundIn=true
```
loop循环
    循环节点vNodes，处理断线逻辑
        节点fDisconnect被标记，或者无引用并且无待收发的数据，即可执行清理逻辑

    循环断线节点vNodesDisconnectedCopy，等待所有的锁被释放。

    更新nPrevNodeCount，更新UI

    预处理socket数据，为执行select做准备
    执行select

    FD_ISSET(hListenSocket, &fdsetRecv) 处理8333端口，是否有新节点请求
        执行以下的创建逻辑，标记入站请求节点，fInboundIn=true
        CNode* pnode = new CNode(hSocket, addr, true);
        pnode->AddRef();
        CRITICAL_BLOCK(cs_vNodes)
            vNodes.push_back(pnode);
    
    遍历vNodes
        处理消息接收；主要是加锁，调用recv，根据接收的数据做相应的处理；正常的目的是将数据缓存至node的vRecv中。
        处理消息发送；主要是加锁，调用send。
        处理节点超时
```

>ThreadOpenConnections 根据获取的比特币节点信息，发起的socket连接，并创建CNode，标记非入站请求节点，fInboundIn=false
```
处理connect参数的地址，执行OpenNetworkConnection
处理addnode参数的地址，执行OpenNetworkConnection

loop
    如果vNodes数目大于等于nMaxConnections（15），就不再增加节点连接了。

    做各种处理，获取需要连接的节点地址，如果存在就调用OpenNetworkConnection
```


> OpenNetworkConnection 连接节点地址
```
OpenNetworkConnection
    ConnectNode创建CNode
        ip已经连接，就返回
        ConnectSocket 创建socket
        用socket创建new CNode(hSocket, addrConnect, false);
    
    设置fNetworkNode=true

    pnode->PushMessage("getaddr"); 节点连接成功后，获取更多网络节点。

    pnode->PushMessage("subscribe", nChannel, nHops); 需要探究
```

> ThreadMessageHandler 用于原始数据的上层逻辑分发。
```
while (!fShutdown)
    遍历vNodes，AddRef

    遍历vNodes，并随机出pnodeTrickle

        ProcessMessages 处理接受消息
            loop
                根据pchMessageStart，搜索出vRecv的开始点
                读出消息头 CMessageHeader hdr
                根据消息头，读出strCommand和消息体vMsg
                调用ProcessMessage，这段处理逻辑比较多，可看姊妹篇【数据同步】

        SendMessages 处理发送消息
            为获取version消息前，不做任何处理。version消息如果未处理通过，节点就是无效的
            节点发送ping消息
            每天将本服务的地址设置进入vAddrToSend，为广播地址做准备

            随机到的fSendTrickle节点，才有资格将本服务的地址广播出去。

            遍历vInventoryToSend，将CInv结构缓存的block或tx记录，以消息头inv的形式，发送到节点缓冲区，告诉对方有新的数据。

            遍历mapAskFor，将CInv结构缓存的block或tx记录，以消息头getdata的形式，发送到节点缓冲区，告诉对方，我需要这些数据。

```
