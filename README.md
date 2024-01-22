# Fec Framework
Lightweight FEC Transmission Module for Enhanced Network Reliability

![Language](https://img.shields.io/badge/language-c-brightgreen)
![License](https://img.shields.io/badge/license-MIT-yellow)
![Documentation](https://img.shields.io/badge/documentation-yes-brightgreen)

# 一套用于网络传输的FEC Framework
## 简介:  
FEC(前向纠错)通过在传输过程中增加冗余数据来提高数据完整性，减少重传需求，从而优化了网络性能和效率。这些特性使得FEC成为提高网络通信可靠性的重要工具，特别是在不稳定或资源受限的网络环境中。 

项目的主要目的是开发一款轻量级、高效、强健的FEC传输模块。该模块专为提高网络通信中的数据可靠性而设计，适用于易于发生数据包丢失的环境。

## 技术特性

**😊协议封装与解析功能：**  
  - 在发送或接收数据时，应用层可以将数据交给FEC Framework处理。FEC Framework将负责填充和解析协议头，确保数据的正确封装和解析。

**😊动态冗余数据生成：**  
  - FEC Framework每打包**k**个应用层数据，将自动生成m包冗余数据。应用层可以根据需要调整k和m的值，只要满足**k >= 1**且**k+m <= 32**的条件，就能灵活适应不同的数据传输需求。

**😊数据丢失后的恢复能力：**  
  - FEC Framework将**k+m**个数据包组织成一组FEC数据块(block)。即使在这组数据中有多达**m**包的数据丢失，接收方也能依靠FEC Framework提供的信息来恢复丢失的数据。

**😊对乱序数据的处理能力：**  
  - 即便一组FEC数据块以乱序形式到达接收端，FEC Framework依然有能力恢复出丢失的数据，增强了数据传输过程中的鲁棒性。

**😊采用Reed-Solomon编码方案：**  
  - FEC Framework采用的是Reed-Solomon编码，一种广泛应用且功能强大的FEC算法, 增强数据的可靠性。

> [!TIP]
> FEC Framework参数说明  
> ❓**k**: Source Packet数量, 经由FEC Framework封装后的原始数据包的数量。  
> ❓**m**: Parity Packet数量, 经由FEC Framework生成的校验数据包的数量。  
> ❓**n**: 每个FEC Block包含的数据包数量，等于k + m。

## 使用概览

### 发送端流程
![发送端流程](/doc/image/sender_op.jpeg)

### 接收端流程
![接收端流程](/doc/image/recver_op.jpg)

# API说明
### 初始化
```C
fec_info_t *fec_framework_init(IUINT8 k, IUINT8 n)
```
- **功能描述**  
初始化fec framework

- **参数说明**  
    - k: Source Packet数量, 原始数据包的数量。  
    - n: 每个FEC Block包含的数据包数量，等于k + m。  
一旦确定了k与n后, fec framework在接受k包应用层数据时, 会生成m包冗余数据

- **返回值**  
成功返回fec_framework句柄, 失败返回NULL

### 反初始化
```C
void fec_framework_deinit(fec_info_t *fec_info)
```
- **功能描述**  
反初始化fec framework

- **参数说明**  
  - fec_info: 由fec_framework_init创建的句柄

- **返回值**  
无

### FEC encode

```C
IINT32
fec_framework_encode(fec_info_t *fec_info, struct fec_buf *ubuf, struct fec_buf **out_ubuf, IINT32 *out_ubuf_count)
```
- **功能描述**  
  - 用于将应用层数据（Application Data Unit, ADU）封装进FEC报文头，以生成FEC数据包（Forward Error Correction Data Unit, FDU）。
  - 该函数在被应用层调用k次之后，除了返回原始数据外，还会额外生成m个冗余数据包。这些冗余数据包有助于在数据传输过程中恢复丢失或损坏的数据包。

- **参数说明**
  - fec_info
    - 描述: 这是一个由fec_framework_init函数创建的句柄。它包含了FEC编码所需的所有配置和状态信息。
    - 用途: 指导FEC编码过程，包括如何生成冗余数据等。
  - ubuf
    - 描述: 指向应用层数据（ADU）的指针。这些数据是需要被FEC算法处理的原始数据。
    - 用途: 作为FEC编码的输入，即将被加入FEC头部的数据。
  - out_ubuf
    - 描述: 输出参数. 为ADU添加了FEC头部的FEC packet(FDU)
    - 用途：存放编码后的数据，包括原始数据和生成的冗余数据。
  - out_ubuf_count
    - 描述: 输出参数. 表示输出的FDU数量。如果fec_framework_encode函数没有产生额外的冗余数据，则out_ubuf_count的值为1。
    - 用途：提供了输出FDU数组的长度，帮助调用者了解有多少个FDU需要进一步处理。

- **返回值**
  - 成功返回0, 失败返回-1

### FEC decode
```C
IINT32
fec_framework_decode(fec_info_t *fec_info, struct fec_buf *ubuf, struct fec_buf **out_ubuf, IINT32 *out_ubuf_count)
```
- **功能描述**  
  FEC Framework的解码部分，主要用于FEC接收端。该函数的目的是处理接收到的FEC数据包（FDU），并尝试从中恢复原始应用层数据（ADU）。
  这个过程包括:  
  - 解析FEC头部;
  - 以及根据接收到的原始数据和冗余数据重建丢失的数据包。

- **参数说明**
  - fec_info
    - 描述: 这是一个由fec_framework_init函数创建的句柄。它包含了FEC编码所需的所有配置和状态信息。
    - 用途: 为FEC解码过程提供必要的指导，包括如何处理接收到的数据包。
  - ubuf
    - 描述: 指向接收到的FEC数据包（FDU）的指针。这些数据包可能是原始数据或由发送端生成的冗余数据。
    - 用途: 作为FEC解码的输入，提供需要解码和重建的数据。
  - out_ubuf
    - 描述: 输出参数. 为ADU添加了FEC头部的FEC packet(FDU)
    - 用途：存放编码后的数据，包括原始数据和生成的冗余数据。
  - out_ubuf_count
    - 描述: 输出参数. 表示输出的ADU数量。这个数值指示了成功解码和重建的数据包数量。
    - 用途：提供了输出ADU数组的长度，帮助调用者了解有多少个数据包, 这也包含被成功恢复的数据包。

- **返回值**
  - 成功返回0, 失败返回-1

# 调用流程
1. 创建fec framework对象

    ```C
    //按照k=3,n=4的码率初始化fec.
    //也就是说:每调用3次fec_framework_encode函数后, fec_framework_encode会生成1包冗余数据(4 - 3).　
    fec_info = fec_framework_init(3, 4);
    ```

2. 应用层调用fec_framework_encode进行编码

    ```C
    //fec_framework_encode仅负责编码工作.真正的发送还需应用层处理.
    ret = fec_framework_encode(fec_info, &ubuf, &out_ubuf, &out_ubuf_count);
    ```
3. 应用层根据out_ubuf_count的值发送数据

    ```C
    for (int i = 0; i < out_ubuf_count; i++) {
      sendto(udp_socket, fec_buf_data(&out_ubuf[i]), fec_buf_length(&out_ubuf[i]), flags, dest_addr, addrlen);
    }
    ```
4. 应用层调用recvfrom接收数据

5. 应用层可根据情况决定是否交由FEC Framework处理

    ```C
    if (adu->need_fec_decode) {
      fec_framework_decode(fec_info, ubuf, &out_ubuf, &out_ubuf_count);
    }
    ```
6. 通过out_ubuf_count的值进行循环处理

    ```C
    for (int i = 0; i < out_ubuf_count; i++) {
      app_proc_func(out_ubuf[i]);
      ...
    }
    ```
