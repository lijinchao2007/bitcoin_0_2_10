## 【比特币0.2.10源码分析】交易流程
### [目录](../README.md)
### [项目地址](https://github.com/lijinchao2007/bitcoin_0_2_10)

> UI的转账入口是SendMoneyToBitcoinAddress，调用SendMoney；RPC的转账入口是SendMoney
 
```
SendMoney 有4个参数。
    CScript scriptPubKey是转账目标地址生成的脚本；int64 nValue转账金额；CWalletTx& wtxNew 交易记录，数据都记录在此；bool fAskFee尚未使用。

    CreateTransaction 生成签名等
        多了两个新参，CKey key 作为转账剩余金额的拥有秘钥；int64 nFeeRequired 费用返回值。

        loop循环，起始的费用从全局变量nFee取，如果小于转账必须的费用，会修改nFee，再次循环执行所有下面的逻辑

            SelectCoins 根据转账金额，获取发送者所持有的最合适的set<CWalletTx*>& setCoinsRet；
            
                遍历发送者的mapWallet；刚好有金额相符的记录，直接返回；反之，小于转账金额的记录，累加到nTotalLower，缓存记录到vValue；
                大于的则找到最小的记录pcoinLowestLarger，以备使用。

                如果nTotalLower不足，则pcoinLowestLarger直接返回。

                如果nTotalLower充足，则要随机1000次，找到大于等于转账金额的最小组合记录。

            如果SelectCoins未找到足够的记录，则转账失败。

            nValueIn获取可用转账记录的总金额

            fChangeFirst随机决定是先记录接收者的收款条目，还是余额条目（如果没有余额则略过）。

            处理完out，再处理in；
            遍历可用的记录，再遍历所有的out；out不属于发送者的跳过；用CWalletTx的hash和out所在的位置生成in条目。
            遍历可用的记录，再遍历所有的out；out不属于发送者的跳过；调用SignSignature对wtxNew记录中的所有新生成的in条目签名。

                SignSignature 尚未详读，单列详解； 核心是生成in条目的签名

            AddSupportingTransactions 尚未详读，感觉的处理默克尔树。



    CommitTransaction 存储，广播等

        上一步如果有生成新的秘钥，存入钱包；如果没存入，余额就丢了。

        AddToWallet 缓存到mapWallet
        记录fInsertedNew和fUpdated，决定是否调用WriteToDisk，存储到db
        更新默认私钥
        插入vWalletUpdated，用于更新UI

        mapRequestCount设定交易记录被请求次数

        wtxNew.AcceptTransaction 
            逻辑很多，留待详解
            检验交易是否合法
            ConnectInputs 处理db等很多业务在此

        wtxNew.RelayWalletTransaction();
            遍历vtxPrev，
                所有前任存储成功的情况下，
                调用RelayMessage(CInv(MSG_TX, hash), (CTransaction)tx)，中继广播交易。
                    将tx转换为CDataStream，调用RelayMessage
                        mapRelay存储inv和tx，并在vRelayExpiration中记录时间对
                        调用RelayInventory
                            遍历所有节点，调用PushInventory，广播交易记录
                                setInventoryKnown中已经存在的过滤
                                vInventoryToSend中插入inv
                                在处理发送消息的时候，批量推送pto->PushMessage("inv", vInv)， vInv是vector<CInv>
                                之所以PushInventory不直接发送区块广播，是想按批次广播。
                                更详细的过程可参见【数据同步】

            对当期交易执行RelayMessage，参加上文逻辑
    
```

> 签名的过程
```
SignSignature 
```