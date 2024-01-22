#ifndef __FEC_FRAMEWORK_H__
#define __FEC_FRAMEWORK_H__

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>

//#include "fec.h"
#include "fec_buf.h"
#include "fec_platform.h"

#define UKCP_FEC_TYPE_SOURCE_PACKET    0x01
#define UKCP_FEC_TYPE_PARITY_PACKET    0x02

typedef struct {
    IUINT16 group_id;
    IUINT16 seq_id;
#define FEC_SYMBOL_ID_MASK  0x1F
    IUINT8 symbol_id;
    IUINT8 buf_type;
    IUINT16 buf_size;
    IUINT8 fdu[0];
} fec_header_t;

typedef struct fec_adu_info {
    IUINT16 len;
    IUINT16 rfu;
    IUINT8 adu[0];
} fec_adu_info_t;

typedef struct {
    fec_header_t fec_header;
    IUINT8 processed;
    fec_adu_info_t *adu_info;
} fec_symbol_t;

typedef struct {
    IUINT16 seq_id;
    IUINT16 seq_block_start;
    IUINT32 symbol_set;
    fec_symbol_t *symbol;
    IUINT16 max_len;
    IUINT16 group_id;
    IUINT8  count;
    IUINT8  parity;
} fec_block_t;

typedef struct {
    IUINT16 k;
    IUINT16 n;
    void *code;
    IUINT32 expect_symbol_set;
    fec_block_t fec_w_block;
    fec_block_t fec_r_block;
} fec_info_t;

fec_info_t * fec_framework_init(IUINT8 k, IUINT8 n);

void fec_framework_deinit(fec_info_t *fec_info);

IINT32
fec_framework_encode(fec_info_t *fec_info, struct fec_buf *ubuf, struct fec_buf **out_ubuf, IINT32 *out_ubuf_count);

IINT32
fec_framework_decode(fec_info_t *fec_info, struct fec_buf *ubuf, struct fec_buf **out_ubuf, IINT32 *out_ubuf_count);

#endif
