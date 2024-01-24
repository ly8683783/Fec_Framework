#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>

#include "fec_framework.h"
#include "fec_buf.h"
#include "fec_platform.h"
#include "fec_scheme.h"

#define FEC_FRAMEWORK_N_LIMIT 32

static inline IUINT32 _fec_max_(IUINT32 a, IUINT32 b)
{
    return a >= b ? a : b;
}

static inline IINT16 fec_diff(IUINT16 later, IUINT16 earlier)
{
    return ((IINT16)(later - earlier));
}

void rs_encode(void *code, void *data[],IINT32 size, IUINT16 k, IUINT16 n)
{
	for(IINT32 i = k; i < n; i++)
		fec_encode(code, (void **)data, data[i],i, size);
}

IINT32 rs_decode(void *code, void *data[], IINT32 size, IUINT16 k, IUINT16 n)
{
	IINT32 index[n];
	IINT32 count = 0;

	for(IINT32 i = 0; i < n; i++) {
		if(data[i] != 0) {
			index[count++] = i;
		}
	}

	if (count < k)
		return -1;

	for(IINT32 i = 0; i < n; i++) {
		if(i < count)
			data[i] = data[index[i]];
		else
			data[i] = 0;
	}
	return fec_decode(code, (void **)data, index, size);
}

IINT32
fec_block_init(fec_block_t *fec_block, IUINT16 k, IUINT16 n)
{
    fec_symbol_t *symbol;

    fec_block->symbol = calloc(n, sizeof(fec_symbol_t));
    if (fec_block->symbol == NULL) {
        return -1;
    }

    fec_block->seq_id = 0;
    fec_block->group_id = 0;
    return 0;
}

void
fec_block_reset(fec_block_t *fec_block, IUINT16 group_id, IUINT8 k, IUINT8 n)
{
    IINT32 i;

    for (i = 0; i < n; i++) {
        if (fec_block->symbol[i].adu_info != NULL) {
            free(fec_block->symbol[i].adu_info);
            fec_block->symbol[i].adu_info = NULL;
        }
    }
    fec_block->max_len = 0;
    fec_block->count = 0;
    fec_block->symbol_set = 0;
    fec_block->parity = 0;
    fec_block->group_id = group_id;
    memset(fec_block->symbol, 0, sizeof(fec_symbol_t) * n);
}

void
fec_block_deinit(fec_block_t *fec_block, IUINT16 k, IUINT16 n)
{
    IINT32 i;

    fec_block_reset(fec_block, 0, k, n);
    free(fec_block->symbol);
}

fec_info_t *
fec_framework_init(IUINT8 k, IUINT8 n)
{
    IINT32 i;
    fec_info_t *fec_info;

    if (n == 0 || n > FEC_FRAMEWORK_N_LIMIT) {
        FEC_LOGE("param err");
        return NULL;
    }
    if (k == 0 || k >= n) {
        FEC_LOGE("param err");
        return NULL;
    }

    if ((fec_info = calloc(1, sizeof(fec_info_t))) == NULL) {
        FEC_LOGE("MEM err");
        return NULL;
    }

    if ((fec_info->code = fec_new(k, n)) == NULL) {
        FEC_LOGE("fec_new ret NULL");
        return NULL;
    }
    fec_info->k = k;
    fec_info->n = n;
    fec_info->expect_symbol_set = (1 << k) - 1;
    fec_block_init(&fec_info->fec_w_block, k, n);
    fec_block_init(&fec_info->fec_r_block, k, n);

    return fec_info;
}

void
fec_framework_deinit(fec_info_t *fec_info)
{
    fec_block_deinit(&fec_info->fec_w_block, fec_info->k, fec_info->n);
    fec_block_deinit(&fec_info->fec_r_block, fec_info->k, fec_info->n);
    
    fec_free(fec_info->code);
    free(fec_info);
}

IINT32 fec_du_encode(struct fec_buf *ubuf, IUINT16 group_id, IUINT16 seq_id, IUINT8 symbol_id,
            IUINT8 buf_type, IUINT8 *data, IUINT16 data_len)
{
    fec_header_t *fec_header_ptr;

    if (data != NULL && data_len > 0)
        fec_buf_put_data(ubuf, data, data_len);

    fec_header_ptr = fec_buf_push(ubuf, sizeof(fec_header_t));
    fec_header_ptr->group_id = htons(group_id);
    fec_header_ptr->seq_id = htons(seq_id);
    fec_header_ptr->symbol_id = symbol_id;
    fec_header_ptr->buf_size = htons(data_len);
    fec_header_ptr->buf_type = buf_type;
}

IINT32
fec_redundancy_generator(fec_info_t *fec_info, fec_block_t *fec_w_block, struct fec_buf **out_ubuf, IINT32 *out_ubuf_count)
{
    IINT32 i;
    IINT32 ret = 0;
    IINT32 parity_symbol_index = 0;
    IUINT8 **data;
    fec_header_t *fec_header_ptr;

    data = calloc(1, sizeof(IUINT8 *) * fec_info->n);
    if (data == NULL) {
        return -1;
    }
    for (i = 0; i < fec_info->k; i++) {
        IINT32 padding_len;
        data[i] = calloc(1, fec_w_block->max_len);
        if (data[i] == NULL) {
            ret = -1;
            goto err;
        }
        memcpy(data[i], fec_w_block->symbol[i].adu_info, fec_w_block->symbol[i].fec_header.buf_size);
        padding_len = fec_w_block->max_len - fec_w_block->symbol[i].fec_header.buf_size;
        if (padding_len > 0) {
            memset(data[i] + fec_w_block->symbol[i].fec_header.buf_size, 0, padding_len);
        }
        free(fec_w_block->symbol[i].adu_info);
        fec_w_block->symbol[i].adu_info = NULL;
    }
    for (parity_symbol_index = i; i < fec_info->n; i++) {
        data[i] = calloc(1, fec_w_block->max_len);
        if (data[i] == NULL) {
            ret = -1;
            goto err;
        }
    }

    rs_encode(fec_info->code, (void **)data, fec_w_block->max_len, fec_info->k, fec_info->n);

    for (; parity_symbol_index < fec_info->n; parity_symbol_index++, (*out_ubuf_count)++) {
        ret = fec_buf_init(&(*out_ubuf)[*out_ubuf_count], sizeof(fec_header_t) + fec_w_block->max_len, sizeof(fec_header_t));
        if (ret != 0) {
            FEC_LOGE("fec_buf_init ret %d", ret);
            break;
        }
        fec_du_encode(&(*out_ubuf)[*out_ubuf_count], fec_w_block->group_id, fec_w_block->seq_id++, parity_symbol_index,
                    UKCP_FEC_TYPE_PARITY_PACKET, data[parity_symbol_index], fec_w_block->max_len);
    }

err:
    for (i = 0; i < fec_info->n; i++) {
        if (data[i] != NULL) {
            free(data[i]);
        }
    }
    free(data);

    return ret;
}

IINT32
fec_framework_encode(fec_info_t *fec_info, struct fec_buf *ubuf, struct fec_buf **out_ubuf, IINT32 *out_ubuf_count)
{
    fec_block_t *fec_w_block;
    IUINT16 symbol_id = 0;
    IINT32 ret;
    IUINT8 **data;
    IUINT16 fec_buf_len, buf_len;
    fec_adu_info_t *adu_info;

    *out_ubuf_count = 0;
    fec_w_block = &fec_info->fec_w_block;

    buf_len = fec_buf_length(ubuf);
    fec_buf_len = sizeof(fec_adu_info_t) + buf_len;
    if (fec_w_block->max_len < fec_buf_len)
        fec_w_block->max_len = fec_buf_len;

    symbol_id = fec_w_block->count++;

    adu_info = fec_buf_push(ubuf, sizeof(fec_adu_info_t));
    adu_info->len = htons(buf_len);

    fec_w_block->symbol[symbol_id].fec_header.group_id = fec_w_block->group_id;
    fec_w_block->symbol[symbol_id].fec_header.seq_id = fec_w_block->seq_id++;
    fec_w_block->symbol[symbol_id].fec_header.symbol_id = symbol_id;
    fec_w_block->symbol[symbol_id].fec_header.buf_type = UKCP_FEC_TYPE_SOURCE_PACKET;
    fec_w_block->symbol[symbol_id].fec_header.buf_size = fec_buf_length(ubuf);
    fec_w_block->symbol[symbol_id].adu_info = calloc(1, fec_buf_length(ubuf));
    memcpy(fec_w_block->symbol[symbol_id].adu_info, fec_buf_data(ubuf), fec_buf_length(ubuf));

    fec_du_encode(ubuf, fec_w_block->group_id, fec_w_block->symbol[symbol_id].fec_header.seq_id, symbol_id,
                fec_w_block->symbol[symbol_id].fec_header.buf_type, NULL,
                fec_w_block->symbol[symbol_id].fec_header.buf_size);

    *out_ubuf = calloc(fec_info->n - fec_info->k + 1, sizeof(struct fec_buf));
    (*out_ubuf)[(*out_ubuf_count)++] = *ubuf;

    if ((fec_w_block->count) % fec_info->k == 0) {
        ret = fec_redundancy_generator(fec_info, fec_w_block, out_ubuf, out_ubuf_count);
        fec_w_block->count = 0;
        fec_w_block->group_id++;
    }

    return 0;
}

IINT32
fec_repair(fec_info_t *fec_info, fec_block_t *fec_r_block, struct fec_buf *out_ubuf, IINT32 *out_ubuf_count)
{
    IINT32 ret = 0;
    IINT32 i;
    fec_adu_info_t **data;
    IINT32 index[32] = {0};
    IINT32 count = 0;

    data = calloc(1, sizeof(fec_adu_info_t *) * fec_info->n);
    for (i = 0; i < fec_info->n; i++) {
        data[i] = NULL;
        if (fec_r_block->symbol[i].adu_info && fec_r_block->symbol[i].fec_header.buf_size) {
            IINT32 padding_len;

            data[i] = calloc(1, fec_r_block->max_len);
            memcpy(data[i], fec_r_block->symbol[i].adu_info, fec_r_block->symbol[i].fec_header.buf_size);
            padding_len = fec_r_block->max_len - fec_r_block->symbol[i].fec_header.buf_size;
            if (padding_len > 0) {
                memset((IUINT8 *)data[i] + fec_r_block->symbol[i].fec_header.buf_size, 0, padding_len);
            }
            free(fec_r_block->symbol[i].adu_info);
            fec_r_block->symbol[i].adu_info = NULL;
        }
        if (data[i] == NULL)
            index[count++] = i;
    }
    ret = rs_decode(fec_info->code, (void **)data, fec_r_block->max_len, fec_info->k, fec_info->n);
    if (ret == 0) {
        for (i = 0; i < count; i++) {
            if (data[index[i]]) {
                IUINT16 adu_len;
                adu_len = ntohs(data[index[i]]->len);
                fec_buf_init(&out_ubuf[*out_ubuf_count], adu_len, 0);
                fec_buf_put_data(&out_ubuf[*out_ubuf_count], data[index[i]]->adu, adu_len);
                (*out_ubuf_count)++;
            }
        }
    }
    for (i = 0; i < fec_info->n; i++) {
        if (data[i] != NULL) {
            free(data[i]);
        }
    }
    free(data);

    return 0;
}

IINT32
fec_framework_decode(fec_info_t *fec_info, struct fec_buf *ubuf, struct fec_buf **out_ubuf, IINT32 *out_ubuf_count)
{
    IINT32 ret;
    fec_block_t *fec_r_block;
    fec_header_t fec_header;
    IUINT16 symbol_id = 0;

    if (fec_buf_length(ubuf) < sizeof(fec_header_t)) {
        FEC_LOGD("R decode err. len %hu", fec_buf_length(ubuf));
        return -1;
    }

    fec_r_block = &fec_info->fec_r_block;

    fec_buf_pull_data(ubuf, &fec_header, sizeof(fec_header_t));
    fec_header.buf_size = ntohs(fec_header.buf_size);
    fec_header.group_id = ntohs(fec_header.group_id);
    fec_header.seq_id = ntohs(fec_header.seq_id);

    *out_ubuf_count = 0;
    *out_ubuf = calloc(fec_info->k, sizeof(struct fec_buf));
    if (*out_ubuf == NULL) {
        FEC_LOGE("MEM err");
        if (fec_header.buf_type == UKCP_FEC_TYPE_SOURCE_PACKET) {
            fec_buf_pull(ubuf, sizeof(fec_adu_info_t));
            (*out_ubuf)[*out_ubuf_count] = *ubuf;
            (*out_ubuf_count)++;
        } else {
            fec_buf_free(ubuf);
        }
        return 0;
    }

    ret = fec_diff(fec_header.group_id, fec_r_block->group_id);
    if (ret < 0) {
        if (fec_header.buf_type == UKCP_FEC_TYPE_SOURCE_PACKET) {
            fec_buf_pull(ubuf, sizeof(fec_adu_info_t));
            (*out_ubuf)[*out_ubuf_count] = *ubuf;
            (*out_ubuf_count)++;
        } else {
            fec_buf_free(ubuf);
        }
        return 0;
    }
    if (ret > 0) {
        fec_block_reset(fec_r_block, fec_header.group_id, fec_info->k, fec_info->n);
    }

    if (fec_header.symbol_id >= fec_info->n)
        return -3;

    if (fec_r_block->parity)
        return -2;

    if (fec_diff(fec_r_block->seq_id, fec_header.seq_id) < 0) {
        fec_r_block->seq_id = fec_header.seq_id;
    }

    symbol_id = fec_header.symbol_id;
    if (fec_r_block->symbol[symbol_id].fec_header.buf_size != 0 &&
                fec_header.symbol_id == fec_r_block->symbol[symbol_id].fec_header.symbol_id) {
        FEC_LOGE("R fec_header.seq_id %u dup", fec_header.seq_id);
        return -2;
    }

    fec_r_block->max_len = _fec_max_(fec_r_block->max_len, fec_header.buf_size);

    fec_r_block->symbol[symbol_id].fec_header.group_id = fec_header.group_id;
    fec_r_block->symbol[symbol_id].fec_header.seq_id = fec_header.seq_id;
    fec_r_block->symbol[symbol_id].fec_header.symbol_id = symbol_id & FEC_SYMBOL_ID_MASK;
    fec_r_block->symbol[symbol_id].fec_header.buf_size = fec_header.buf_size;
    fec_r_block->symbol[symbol_id].fec_header.buf_type = fec_header.buf_type;
    fec_r_block->symbol[symbol_id].adu_info = calloc(1, fec_header.buf_size);
    memcpy(fec_r_block->symbol[symbol_id].adu_info, fec_buf_data(ubuf), fec_header.buf_size);
    fec_r_block->symbol[symbol_id].processed = 1;

    if (fec_r_block->symbol[symbol_id].fec_header.buf_type == UKCP_FEC_TYPE_SOURCE_PACKET) {
        fec_buf_pull(ubuf, sizeof(fec_adu_info_t));
        (*out_ubuf)[*out_ubuf_count] = *ubuf;
        (*out_ubuf_count)++;
    } else {
        fec_buf_free(ubuf);
    }

    fec_r_block->symbol_set |= 1 << symbol_id;
    fec_r_block->count++;
    
    if (fec_r_block->count >= fec_info->k) {
        if ((fec_r_block->symbol_set & fec_info->expect_symbol_set) == fec_info->expect_symbol_set)
            return 0;

        fec_repair(fec_info, fec_r_block, *out_ubuf, out_ubuf_count);
        fec_r_block->parity = 1;
    }

    return 0;
}
