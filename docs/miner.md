## 【比特币0.2.10源码分析】挖矿过程
### [目录](../README.md)
### [项目地址](https://github.com/lijinchao2007/bitcoin_0_2_10)

> GenerateBitcoins是开启挖矿的入口。会开启指定数目的线程，执行BitcoinMiner
```
BitcoinMiner 

    key.MakeNewKey();生成新的秘钥

    while (fGenerateBitcoins)

        unsigned int nBits = GetNextWorkRequired(pindexPrev);
        根据当前的block，获取挖矿难度。
        难度的表示是bignum的compact，0x1f0002ff为例，共有四字节，第一个字节是后三位数值所在的位置，像是浮点数的表示方法。
        网上无资料，这是我理解的结果，有些细节没深究，不完全准确。
        转换为大数，为000002ff00000000000000000000000000000000000000000000000000000000。

            没有最新的block，则直接返回默认的难度

            未达需要从新计算难度的区块高度，则直接返回最新区块的难度

            到达14*24*6个区块数，重新计算难度。
            找到14*24*6之前的区块pindexLast，计算出两块之间的时间nActualTimespan

            新的难度系数计算过程是先设置原有的nBits，得到表示难度的大数bnNew。
            bnNew乘以nActualTimespan/nTargetTimespan,(nTargetTimespan=14 * 24 * 60 * 60,两周时间)

            返回bnNew.GetCompact()

        CTransaction txNew; 创建coinbase tx
        auto_ptr<CBlock> pblock(new CBlock()); 创建block，将txNew作为区块的第一条记录。
        遍历已有交易，查找需要打包的交易，插入到block的vtx中；限定size小于MAX_SIZE/2;
        所以不会打包所有交易，这段循环查找逻辑还可再挖细节。

        设定pblock的nBites，以及设定第一条记录的第一条in条目的金额，为GetBlockValue()的结果。
            GetBlockValue为(50 * COIN) >> (nBestHeight / 210000)，210000次大概是4年，所以4年减半，最终为0.
        
        构造tmp结构体

        uint256 hashTarget = CBigNum().SetCompact(pblock->nBits).getuint256(); 将nBits转化为目标hash值。
        loop
            两次调用BlockSHA256，计算tmp结构体的hash值；
            hash算法中,计算tmp.block+tmp.pchPadding0的hash值；
            hash值受128位数据的影响，block为80位，pchPadding0为64位，不会超算。
            而tmp.block中，nVersion，hashPrevBlock，hashMerkleRoot，nTime，nBits都是构建后就固定的
            唯有nNonce会在循环后改变；

            hash值小于hashTarget则挖矿成功，否则继续。

            如果tmp.block.nNonce加到最大也是不行，则break出循环，从新执行while循环。
            下次变化的参数主要是nTime，循环运行到挖矿成功。
            尤其需要注意的是，hash计算有很强的随机性，运气好很快，不好的时候会很久。
            10分钟出块的限定，只是按概率统计的平均值，虽然极端情况很少，但也会有。
            历史中，就有多次比特币几个小时才出块的情况发生，交易会临时积压，
            这也是为何:出块时间为何不是准确的10分钟，而是平均10分钟？

            通过这些参数的限定和hash算法的计算，可以有效的调节出块的难度；
            nActualTimespan/nTargetTimespan系数的调整，刚好能使得nActualTimespan不断逼近nTargetTimespan；
            nTargetTimespan是两周，两周出14*24*6块，一个块平均是十分钟。
            这就是为何10分钟出一块？

            而出块的时候，限定了可包含交易记录的大小不能超出MAX_SIZE/2，大概是是1M（这个版本貌似是32/2=16M?）
            大概是两千笔交易记录。所以一秒钟平均大概三四笔交易的速度。
            交易速度的快慢，由出块速度和出块大小限制。
            而区块的同步到全网的时间，又跟区块的大小有关。
            记得有篇论文研究，1M大小同步全网大概是12s，中位数6m，越大越慢。
            而同步慢的后果，会导致节点的不可信，降低算力攻击的难度。
            理论上是51%才能颠覆共识，会因为网络同步的缓慢，而降到更低的阈值。
            新的一些币种，都在降低出块时间，加大块的上限；以实现加速交易的目的
            所以，这就是为何一块要限制大小？为何比特币交易速度限制在一秒几笔？

            AddKey 保存新的秘钥，否则，矿就成无主的财富了。

            mapRequestCount设置为0，后续做区块广播的时候，会统计这个区块被其他节点请求的次数。

            ProcessBlock 保存区块，广播区块；这个方法也会在收到其他节点发来的区块后调用。

                pblock->CheckBlock()检查区块是否合法；
                很多确认条件，例如检验时间；确认交易是否合法；nNonce是否合法；难度系数是否有伪造；默克尔树是否合法。

                mapBlockIndex检验是否存在前一个block。
                不存在则存入孤儿块中，同时调用pfrom->PushGetBlocks(pindexBest, GetOrphanRoot(pblock));
                获取最新区块与孤儿区块祖块之间的区块。触发的逻辑，可跳到【数据同步：回复getblocks消息】去执行。
                这种情况主要发生在区块是其他节点广播过来的时候

                pblock->AcceptBlock
                    WriteToDisk 写入区块数据
                    AddToBlockIndex 写入区块索引数据
                    循环vNodes节点，pnode->PushInventory(CInv(MSG_BLOCK, hash)),广播新区块
                    区块分发可了解[【比特币0.2.10源码分析】数据同步](./data_sync.md)

                到此基本完成挖矿了。
                vWorkQueue这块的逻辑是针对接受到其他节点分发区块的时候处理的。
                如果收到的当前块B比较老，恰好曾经收到的新快A是B快的下一块。
                A块收到的时候，由于前块B还不存在，被当做孤儿块存入到mapOrphanBlocksByPrev中。
                此时，A块因为父块B已经存在，所以就可以执行孤儿块的AcceptBlock逻辑了，将孤儿块加入链中。
                同时孤儿块A的hash加入vWorkQueue中，继续找寻依赖于A块的孤儿块，直到vWorkQueue空。
```
