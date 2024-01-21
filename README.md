# FEC_for_Lossy_Networks
Lightweight FEC Transmission Module for Enhanced Network Reliability

![Language](https://img.shields.io/badge/language-c-brightgreen)
![License](https://img.shields.io/badge/license-MIT-yellow)
![Documentation](https://img.shields.io/badge/documentation-yes-brightgreen)

# 一套用于网络传输的FEC Framework
**简介:**  
FEC(前向纠错)通过在传输过程中增加冗余数据来提高数据完整性，减少重传需求，从而优化了网络性能和效率。这些特性使得FEC成为提高网络通信可靠性的重要工具，特别是在不稳定或资源受限的网络环境中。  
本项目的主要目的是开发一款轻量级、高效、强健的FEC传输模块。该模块专为提高网络通信中的数据可靠性和完整性而设计，尤其适用于易于发生数据包丢失和传输错误的环境。通过实施基于Reed-Solomon编码的FEC方案，项目旨在显著减少数据包丢失的影响，从而提高在各种网络条件下的整体性能和可靠性。

## 技术特性

**😊具有一定的协议封装, 应用层在发送前, 可将数据交由FEC Framework. FEC Framework会完成协议头的填充.**

**😊接收方将解析FEC Framework报文信息, 并将有效数据递交给应用层.**

**😊每发送k包应用层数据, 将产生m包冗余数据. k, m的取值可由应用层按需调整, k >= 1 && k+m <= 32即可**

**😊k+m为一组fec数据块, 如果有<=m包数据丢失, 接收方可根据fec信息恢复丢失的数据**

**😊当一组fec数据块以乱序到达接收端时, 依然能够恢复出丢失的数据**

**😊FEC Scheme采用的是Reed-Solomon编码方案, 该编码是一种常见且功能强大的FEC算法，适用于各种应用。**

## FEC Framework参数说明
❓k: Source Packet数量, 原始数据包的数量。  
❓m: Parity Packet数量, 生成的校验数据包的数量  
❓n: 每个FEC Block包含的数据包数量，等于k + m（n）。

## 使用概览

### 发送端流程
![发送端流程](/doc/image/sender_op.jpeg)

### 接收端流程
![接收端流程](/doc/image/recver_op.jpg)

## API说明
### 初始化
```
fec_info_t *fec_framework_init(IUINT8 k, IUINT8 n)
```
#### 初始化fec framework  
k: Source Packet数量, 原始数据包的数量。  
n: 每个FEC Block包含的数据包数量，等于k + m（n）。  
一旦确定了k与n后, fec framework在接受k包应用层数据时, 会生成m包冗余数据
