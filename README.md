# FEC_for_Lossy_Networks
Lightweight FEC Transmission Module for Enhanced Network Reliability

![Language](https://img.shields.io/badge/language-c-brightgreen)
![License](https://img.shields.io/badge/license-MIT-yellow)
![Documentation](https://img.shields.io/badge/documentation-yes-brightgreen)

# 一套用于网络传输的FEC Framework
本项目的主要目的是开发一款轻量级、高效、强健的前向纠错（FEC）传输模块。该模块专为提高网络通信中的数据可靠性和完整性而设计，尤其适用于易于发生数据包丢失和传输错误的环境。通过实施基于Reed-Solomon编码的FEC方案，项目旨在显著减少数据包丢失的影响，从而提高在各种网络条件下的整体性能和可靠性。

## 特点

**😊具有一定的协议封装, 应用层在发送前, 可将数据交由FEC Framework. FEC Framework会完成协议头的填充.**

**😊接收方将解析FEC Framework报文信息, 并将有效数据递交给应用层.**

**😊每发送k包应用层数据, 将产生m包校验数据. k, m可随意调整, k >= 1 && k+m <= 32即可**

**😊k+m为一组fec数据块, 如果有<=m包数据丢失, 接收方可根据fec信息恢复丢失的数据**

**😊当一组fec数据块以乱序到达接收端时, 依然能够恢复出丢失的数据**

**😊FEC Scheme采用的是Reed-Solomon编码方案**

## FEC Framework参数说明
❓k: Source Packet数量, 原始数据包的数量。  
❓m: Parity Packet数量, 生成的校验数据包的数量  
❓n: 每个FEC Block包含的数据包数量，等于k + m（n）。

## 
  
