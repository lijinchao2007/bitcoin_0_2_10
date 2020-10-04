## 【比特币0.2.10源码分析】数据同步
### [目录](../README.md)
### [项目地址](https://github.com/lijinchao2007/bitcoin_0_2_10)

### 连接到新节点触发的同步过程
> 节点A收到节点B的version信息，会触发后续一系列的消息。
核心的是A回复B消息getblocks，询问B需要同步哪些区块；
B回复A消息inv，告知A需要同步的区块列表；
A回复B消息getdata，向B拉去最新的区块。
下面是详细的过程。

```
回复verack消息
    节点B：处理verack
        设置vRecv的版本

回复getblocks消息，fAskedForBlocks标志决定节点只在第一次收到version消息是回复
    节点B：处理getblocks
        CBlockIndex* pindex = locator.GetBlockIndex(); 
        如果节点A的区块落后，则返回节点A最新的区块。只有这种情况，节点B才需要广播区块。
        如果节点A的区块先进，则返回节点B最新的区块。

        pindex如果存在就更新成下一个区块
        节点A区块落后，则更新成节点A下一个区块
        节点A区快先进，则更新为空

        int nLimit = 500 + locator.GetDistanceBack(); 限定节点B通知节点A拉去区块的个数
        GetDistanceBack是获取节点A落后节点B的区块个数。不落后则返回0.

        遍历pindex，及其之后的区块，
            遇到终止区块或者空区块，就停止

            调用PushInventory，将CInv区块广播消息插入vInventoryToSend队列
            在处理发送消息的时候，批量推送pto->PushMessage("inv", vInv)， vInv是vector<CInv>
            之所以PushInventory不直接发送区块广播，是想按批次广播。

            如果超出nLimit，则记录当前区块到hashContinue，停止遍历。


    节点A：处理inv（当节点A落后的情况）（事实上inv消息不止广播区块，交易记录亦可以被广播）
        遍历收到的vector<CInv>
            CNode调用AddInventoryKnown，将inv插入setInventoryKnown，表示这个区块节点A已经知晓
            猜测避免的一种情况是：如果后续有一些其他节点广播的消息发给A，恰好A准备发给B，则可以过滤掉。

            bool fAlreadyHave = AlreadyHave(txdb, inv); 判断A是不是已经收到区块数据。
                没有收到：
                则调用AskFor，将inv插入到CNode的mapAskFor中，以及全局变量mapAlreadyAskedFor中。
                在处理发送消息的时候，批量推送pto->PushMessage("getdata", vGetData);
                遇到重复插入的记录，需要将于上次插入时间间隔两分钟，避免短时间连续请求。
                说不定请求的区块在路上，到达了就会将mapAskFor清除。
                或者赶上批次，就能去掉重复请求了。

                有收到，并且是孤儿区块：
                调用pfrom->PushGetBlocks(pindexBest, GetOrphanRoot(mapOrphanBlocks[inv.hash]));
                获取最新区块与孤儿区块祖块之间的区块。触发的逻辑，可回到【回复getblocks消息】去执行。

                mapRequestCount更新区块被请求次数；针对A尚未存在这个区块的情况，无法更新。
                搜索mapRequestCount可看到，只在内存中存在，而且只在创建区块或交易的时候填充。
                所以几乎没意义。

    节点B：处理getdata消息。
        遍历收到的vector<CInv>
            如果是拉取区块
                在mapBlockIndex中查找，找到索引后，从db中加载完整的区块。
                并调用pfrom->PushMessage("block", block)，返回区块数据给A
                区块数据较大，所以直接进入发送缓冲区，没有做批次处理

                如果当前拉取的区块，刚好等于hashContinue
                则广播A最新的CInv(MSG_BLOCK, hashBestChain)消息
                之所以没有广播hashContinue到hashBestChain之间的所有CInv消息，是对孤儿块有做处理。
            
            如果是拉取tx
                在mapRelay中查找并返回tx消息

    节点A：处理block消息
        调用AddInventoryKnown，将inv插入setInventoryKnown，表示这个区块节点A已经知晓

        ProcessBlock 保存区块，广播区块；这个方法也会在挖到矿的时候调用，可参考【挖矿过程】
        其中一块逻辑需要点出，刚好对应了前文hashContinue的处理方法：
            mapBlockIndex检验是否存在前一个block。
            不存在则存入孤儿块中，同时调用pfrom->PushGetBlocks(pindexBest, GetOrphanRoot(pblock));
            获取最新区块与孤儿区块祖块之间的区块。触发的逻辑，可跳到【回复getblocks消息】去执行。
            从而实现了区块同步数据过多，可以递归的不断从pindexBest向前推进。
            设计回环之巧妙，叹为观止。

        清理mapAlreadyAskedFor

```

### 发起交易的同步过程
[【比特币0.2.10源码分析】交易流程:RelayWalletTransaction](./transaction.md)

### 挖到新block的同步过程
[【比特币0.2.10源码分析】挖矿过程:AcceptBlock](./minner.md)

> 中继传播的逻辑未能在区块同步中体现，可参考发起交易中的记录同步过程。


