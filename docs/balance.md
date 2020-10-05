## 【比特币0.2.10源码分析】余额
### [目录](../README.md)
### [项目地址](https://github.com/lijinchao2007/bitcoin_0_2_10)

> 余额优化
```
要做到瞬时查询，我们知道，使用关系数据库的主键进行查询，由于用了索引，速度极快。
因此，对区块链进行查询之前，首先要扫描整个区块链，重建一个类似关系数据库的地址-余额映射表
有了这样的表，就不用查询某个地址余额的时候，还要扫描全链了。

如果无索引，就要扫描全链条。
在钱包程序中，钱包管理的是一组私钥，对应的是一组公钥和地址。钱包程序必须从创世区块开始扫描每一笔交易，如果：
遇到某笔交易的某个Output是钱包管理的地址之一，则钱包余额增加；
遇到某笔交易的某个Input是钱包管理的地址之一，则钱包余额减少。
```

> 验证条目是否属于自己
```
CTxIn::IsMine
    在mapWallet中根据prevout找到自己的输入CTxOut，判断调用CTxOut的IsMine

CTxOut::IsMine
    调用script模块的::IsMine(scriptPubKey)，验证CTxOut的公钥
        Solver(scriptPubKey, 0, 0, scriptSig) 执行脚本

            Solver(scriptPubKey, vSolution) 解码scriptPubKey包含的脚本
                填充默认的解码模板vTemplates
                遍历解码模板
                    读取操作码，scriptPubKey的操作码要与模板匹配，否则就验证下一个模板
                    碰到操作码OP_PUBKEY，读取65字节，将pair插入PAIRTYPE(opcodetype, valtype)& vSolutionRet
                    碰到操作码OP_PUBKEYHASH,读取32字节，将pair插入PAIRTYPE(opcodetype, valtype)& vSolutionRet

            遍历PAIRTYPE(opcodetype, valtype)& vSolutionRet
                当first为OP_PUBKEY，只需判断钱包里mapKeys是否包含公钥。
                当first为OP_PUBKEYHASH，只需判断钱包里mapPubKeys是否包含160位的公钥hash。
            
            Solver里面还有两个参数uint256 hash, int nHashType，是通过SignSignature，调用Solver是使用的。可参见【交易流程】。


CTransaction:IsMine
    遍历vout，有一个IsMine是true，就返回true。
    这是因为只要有一笔交易属于调用者，就说明有一部分金额转入了调用者的账户里了。
```

> 获取余额
```
CTxIn::GetDebit 获取支出
    在mapWallet中根据prevout找到自己的输入CTxOut，调用CTxOut的nValue

CTxOut::GetCredit 获取收入
    判断IsMine，通过就返回nValue

CTransaction::GetDebit 获取支出
    遍历vin，累计GetDebit的结果

CTransaction::GetCredit 获取收入
    遍历vout，累计nValue

CTransaction::GetValueOut 获取总的累计nValue，不过滤是不是自己的。 


```

> transaction的in，out所实现的UTXO(Unspent TX Output)模型，下面的结构示例可以比较直观的说明
```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│Block #1     │     │Block #2     │     │Block #3     │     │Block #4     │
│┌──┬────┬───┐│     │┌──┬────┬───┐│     │┌──┬────┬───┐│     │┌──┬────┬───┐│
││CB│50.0│OUT├┼──┐  ││CB│50.0│OUT├┼──┐  ││CB│50.0│OUT├┼──┐  ││CB│50.0│OUT││
│└──┴────┴───┘│  │  │└──┴────┴───┘│  │  │└──┴────┴───┘│  │  │└──┴────┴───┘│
│             │  │  │┌──┬────┬───┐│  │  │┌──┬────┬───┐│  │  │┌──┬────┬───┐│
│             │  │  ││  │8.70│OUT├┼──┼──>│IN│    │   ││  └──>│IN│25.0│OUT││
│             │  └──>│IN├────┼───┤│  │  │├──┤58.7│OUT││     │├──┼────┼───┤│
│             │     ││  │41.3│OUT├┼─┐└──>│IN│    │   ││  ┌──>│IN│66.3│OUT││
│             │     │└──┴────┴───┘│ │   │└──┴────┴───┘│  │  │└──┴────┴───┘│
└─────────────┘     └─────────────┘ │   └─────────────┘  │  └─────────────┘
                                    └────────────────────┘

```