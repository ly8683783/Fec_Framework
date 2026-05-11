#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "fec_framework.h"
#include "fec_buf.h"

#define TEST_COUNT      24
#define TEST_DATA_LEN   200
#define TEST_HEAD_LEN   20

#define TEST_K  4
#define TEST_N  5
#define TEST_BLOCK_LIMIT   8


#define TEST_ARRAY_COUNT (TEST_COUNT + TEST_COUNT / TEST_K * (TEST_N - TEST_K))

/* for test func */
void
ubuf_printf_test(IINT8 *act, IINT8 * act2, struct fec_buf *ubuf);

void
adu_printf_test(struct fec_buf *ubuf, IINT32 ubuf_count);

void
adu_sort_test(struct fec_buf *ubuf, IINT32 ubuf_count);

void
ubuf_free_test(struct fec_buf *ubuf, IINT32 count);

IINT32
truncated_packet_test(fec_info_t *fec_info);

IINT32
encode_headroom_test(void);

IINT32
parity_size_reset_test(void);

void
adu_printf_test(struct fec_buf *ubuf, IINT32 ubuf_count)
{
    IINT32 i;

    for (i = 0; i < ubuf_count; i++) {
        FEC_LOGD("%d recver: len %03d\t%s", i, fec_buf_length(&ubuf[i]), (IINT8 *)fec_buf_data(&ubuf[i]));
    }
    FEC_LOGD("++++++++++++");
}

IINT32
adu_seq_test(struct fec_buf *ubuf)
{
    IINT8 *data = fec_buf_data(ubuf);

    if (fec_buf_length(ubuf) < 3)
        return TEST_COUNT + 1;
    if (data[0] < '0' || data[0] > '9' ||
            data[1] < '0' || data[1] > '9' ||
            data[2] < '0' || data[2] > '9')
        return TEST_COUNT + 1;

    return (data[0] - '0') * 100 + (data[1] - '0') * 10 + (data[2] - '0');
}

void
adu_sort_test(struct fec_buf *ubuf, IINT32 ubuf_count)
{
    IINT32 i;
    IINT32 j;

    for (i = 0; i < ubuf_count; i++) {
        for (j = i + 1; j < ubuf_count; j++) {
            if (adu_seq_test(&ubuf[i]) > adu_seq_test(&ubuf[j])) {
                struct fec_buf temp = ubuf[i];

                ubuf[i] = ubuf[j];
                ubuf[j] = temp;
            }
        }
    }
}

void
ubuf_printf_test(IINT8 *act, IINT8 * act2, struct fec_buf *ubuf)
{
    static IUINT16 last_seq_id = 0xeabc;
    IUINT16 cur_seq_id;
    IUINT16 payload_len;
    fec_header_t *fec_header_ptr;
    fec_adu_info_t *adu_info_ptr;

    fec_header_ptr = fec_buf_data(ubuf);
    adu_info_ptr = (fec_adu_info_t *)fec_header_ptr->fdu;

    cur_seq_id = ntohs(fec_header_ptr->seq_id);
    payload_len = fec_header_ptr->buf_type == UKCP_FEC_TYPE_PARITY_PACKET ?
                    ntohs(fec_header_ptr->buf_size) : ntohs(adu_info_ptr->len);
    if (act2) {
        FEC_LOGD("%s seq %05u len %d\t%s %s %s",
                        act, cur_seq_id, payload_len,
                        fec_header_ptr->buf_type == UKCP_FEC_TYPE_PARITY_PACKET ? "PARITY" : (char *)adu_info_ptr->adu,
                        cur_seq_id == last_seq_id ? "(dup)" : " ", act2);
    } else {
        FEC_LOGD("%s seq %05u len %d\t%s %s",
                act, cur_seq_id, payload_len,
                fec_header_ptr->buf_type == UKCP_FEC_TYPE_PARITY_PACKET ? "PARITY": (char *)adu_info_ptr->adu,
                cur_seq_id == last_seq_id ? "(dup)" : " ");
    }
    last_seq_id = cur_seq_id;
}

void
ubuf_free_test(struct fec_buf *ubuf, IINT32 count)
{
//    FEC_LOGD("ubuf_free_test free %d", count);
    for (IINT32 i = 0; i < count; i++) {
//        FEC_LOGD("ubuf_free_test free %p", &ubuf[i]);
        fec_buf_free(&ubuf[i]);
    }

    if (ubuf) {
        free(ubuf);
    }
}

IINT32
truncated_packet_test(fec_info_t *fec_info)
{
    IINT32 ret;
    IINT32 out_ubuf_count = 0;
    fec_header_t fec_header;
    struct fec_buf ubuf;
    struct fec_buf *out_ubuf = NULL;

    memset(&fec_header, 0, sizeof(fec_header));
    fec_header.group_id = htons(0);
    fec_header.seq_id = htons(0);
    fec_header.symbol_id = 0;
    fec_header.buf_type = UKCP_FEC_TYPE_SOURCE_PACKET;
    fec_header.buf_size = htons(1000);

    if (fec_buf_init(&ubuf, sizeof(fec_header), 0) != 0)
        return -1;
    if (fec_buf_put_data(&ubuf, &fec_header, sizeof(fec_header)) == NULL) {
        fec_buf_free(&ubuf);
        return -1;
    }

    ret = fec_framework_decode(fec_info, &ubuf, &out_ubuf, &out_ubuf_count);
    if (ret == 0) {
        ubuf_free_test(out_ubuf, out_ubuf_count);
        return -1;
    }

    if (out_ubuf)
        ubuf_free_test(out_ubuf, out_ubuf_count);
    fec_buf_free(&ubuf);

    return 0;
}

IINT32
encode_headroom_test(void)
{
    IINT32 ret;
    IINT32 out_ubuf_count = -1;
    IINT8 payload[8] = "headroom";
    fec_info_t *fec_info;
    struct fec_buf ubuf = {0};
    struct fec_buf *out_ubuf = NULL;

    fec_info = fec_framework_init(TEST_K, TEST_N, TEST_BLOCK_LIMIT);
    if (fec_info == NULL)
        return -1;

    if (fec_buf_init(&ubuf, TEST_DATA_LEN, 0) != 0)
        goto err;
    if (fec_buf_put_data(&ubuf, payload, sizeof(payload)) == NULL)
        goto err;

    ret = fec_framework_encode(fec_info, &ubuf, &out_ubuf, &out_ubuf_count);
    if (ret == 0 || out_ubuf != NULL || out_ubuf_count != 0)
        goto err;
    if (fec_info->fec_w_block.fec_block_list->count != 0 ||
            fec_info->fec_w_block.fec_block_list->max_len != 0)
        goto err;

    fec_buf_free(&ubuf);
    fec_framework_deinit(fec_info);
    return 0;

err:
    if (out_ubuf)
        ubuf_free_test(out_ubuf, out_ubuf_count);
    fec_buf_free(&ubuf);
    fec_framework_deinit(fec_info);
    return -1;
}

IINT32
encode_payload(fec_info_t *fec_info, IINT8 *payload, IINT32 payload_len,
                struct fec_buf **out_ubuf, IINT32 *out_ubuf_count)
{
    struct fec_buf ubuf;

    if (fec_buf_init(&ubuf, TEST_DATA_LEN, TEST_HEAD_LEN) != 0)
        return -1;
    if (fec_buf_put_data(&ubuf, payload, payload_len) == NULL) {
        fec_buf_free(&ubuf);
        return -1;
    }
    if (fec_framework_encode(fec_info, &ubuf, out_ubuf, out_ubuf_count) != 0) {
        fec_buf_free(&ubuf);
        return -1;
    }

    return 0;
}

IINT32
parity_size_reset_test(void)
{
    IINT32 i;
    IINT32 out_ubuf_count = 0;
    IINT32 second_block_parity_len = -1;
    IINT8 large_payload[80];
    IINT8 small_payload[12];
    fec_info_t *fec_info;
    struct fec_buf *out_ubuf = NULL;

    memset(large_payload, 'A', sizeof(large_payload));
    memset(small_payload, 'B', sizeof(small_payload));

    fec_info = fec_framework_init(TEST_K, TEST_N, TEST_BLOCK_LIMIT);
    if (fec_info == NULL)
        return -1;

    for (i = 0; i < TEST_K; i++) {
        IINT32 payload_len = (i == 0) ? sizeof(large_payload) : sizeof(small_payload);

        out_ubuf = NULL;
        if (encode_payload(fec_info, i == 0 ? large_payload : small_payload, payload_len,
                    &out_ubuf, &out_ubuf_count) != 0)
            goto err;
        ubuf_free_test(out_ubuf, out_ubuf_count);
    }

    for (i = 0; i < TEST_K; i++) {
        out_ubuf = NULL;
        if (encode_payload(fec_info, small_payload, sizeof(small_payload),
                    &out_ubuf, &out_ubuf_count) != 0)
            goto err;
        if (i == TEST_K - 1)
            second_block_parity_len = fec_buf_length(&out_ubuf[1]) - sizeof(fec_header_t);
        ubuf_free_test(out_ubuf, out_ubuf_count);
    }

    fec_framework_deinit(fec_info);
    if (second_block_parity_len != sizeof(fec_adu_info_t) + sizeof(small_payload))
        return -1;

    return 0;

err:
    if (out_ubuf)
        ubuf_free_test(out_ubuf, out_ubuf_count);
    fec_framework_deinit(fec_info);
    return -1;
}


IINT32
generate_string(IUINT16 seq, IUINT8 *test_buf, IINT32 buf_len)
{
    memset(test_buf, 0, buf_len);
    buf_len = rand() % (buf_len - 2);
    if (buf_len > TEST_DATA_LEN - TEST_HEAD_LEN - 1)
        buf_len = TEST_DATA_LEN - TEST_HEAD_LEN - 1;
    if (buf_len < 8) {
        buf_len = 8;
    }
    for (IINT32 i = 0; i < buf_len; i++) {
        test_buf[i] = (rand() % 0x0a) + 0x30;
    }

    sprintf(test_buf, "%03u", seq);
    test_buf[strlen(test_buf)] = ' ';
    
    return buf_len;
}

void
send_test(fec_info_t *fec_info, struct fec_buf *send_ubuf, IINT32 *send_ubuf_count)
{
    struct fec_buf ubuf;
    IUINT8 test_buf[TEST_DATA_LEN];
    IINT32 i, j;
    IINT32 buf_len;

    for (i = 0; i < TEST_COUNT; i++) {
        struct fec_buf *out_ubuf = NULL;
        IINT32 out_ubuf_count = 0;

        buf_len = generate_string(i, test_buf, sizeof(test_buf));

        fec_buf_init(&ubuf, TEST_DATA_LEN, TEST_HEAD_LEN);
        fec_buf_put_data(&ubuf, test_buf, strlen(test_buf) + 1);

        fec_framework_encode(fec_info, &ubuf, &out_ubuf, &out_ubuf_count);

        for (j = 0; j < out_ubuf_count; j++) {
//            FEC_LOGD("M %d", *send_ubuf_count);
            fec_buf_copy(&send_ubuf[(*send_ubuf_count)++], &out_ubuf[j]);
            if (rand() % 8 == 0) {
                fec_buf_copy(&send_ubuf[(*send_ubuf_count)++], &out_ubuf[j]);
            }
        }
        ubuf_free_test(out_ubuf, out_ubuf_count);
    }
}

void
recv_test(fec_info_t *fec_info,
            struct fec_buf *recv_ubuf_in, IINT32 recv_ubuf_count_in,
            IINT32 *lost_index,
            struct fec_buf *recv_ubuf_out, IINT32 *recv_ubuf_count_out)
{
    IINT32 ret;
    IINT32 i, j;
    struct fec_buf *recv_out_ubuf;
    IINT32 recv_out_ubuf_count;

    for (i = 0; i < recv_ubuf_count_in; i++) {
        if (lost_index[i]) {
//            FEC_LOGD("F %d %s line %d", i, (char *)fec_buf_data(&recv_ubuf_in[i]), __LINE__);
            fec_buf_free(&recv_ubuf_in[i]);
            continue;
        }
        recv_out_ubuf_count = 0;
        ret = fec_framework_decode(fec_info, &recv_ubuf_in[i], &recv_out_ubuf, &recv_out_ubuf_count);
        if (ret != 0) {
//            FEC_LOGD("F %d %s line %d", i, (char *)fec_buf_data(&recv_ubuf_in[i]), __LINE__);
            fec_buf_free(&recv_ubuf_in[i]);
        }

        for (j = 0; j < recv_out_ubuf_count; j++) {
            fec_buf_copy(&recv_ubuf_out[(*recv_ubuf_count_out)++], &recv_out_ubuf[j]);
        }

        if (ret == 0)
            ubuf_free_test(recv_out_ubuf, recv_out_ubuf_count);
    }
}

void
out_of_order(struct fec_buf *ubuf, IINT32 count)
{
    int i;
    int j, q;

    FEC_LOGD("\n-----------------------Emulate out of order start-------------------------------------");
    for (i = 0; i < count; i++) {
        j = rand() % (count);
        q = rand() % (count);
        struct fec_buf temp = ubuf[q];

        ubuf[q] = ubuf[j];
        ubuf[j] = temp;
    }

    for (i = 0; i < count; i++) {
        printf("%02d ", i);
        ubuf_printf_test("send:", NULL, &ubuf[i]);
    }
}

void
fb_list_prt(fec_block_t **fec_block_list)
{
    fec_block_t **fb_node = fec_block_list, *tmp_node;
    
    while (*fb_node) {
        FEC_LOGD("group_id = %d", (*fb_node)->group_id);
        fb_node = &(*fb_node)->next;
    }
}

//void
//test_list(void)
//{
//    int i = 0;
//    IUINT16 group_id;
//    fec_block_info_t fec_block = {0};
//    fec_block_t *fb_node, *ret;
//
//    fec_block_info_init(&fec_block, 4, 6, 10);
//    for (i = 0; i < 25535; i++) {
//        ret = fec_framework_get_block(&fec_block, group_id = rand()%32675, 6);
//        if (ret)
//            FEC_LOGD("ret->group_id = %u", ret->group_id);
//        else {
//            FEC_LOGD("ret = NULL group_id %u", group_id);
//        }
//    }
//
//    fb_list_prt(&fec_block.fec_block_list);
//    fec_block_info_deinit(&fec_block, 6, "test_list");
//}

IINT32
main(void)
{
    IINT32 i;
    fec_info_t *fec_info;
    struct fec_buf send_ubuf[TEST_ARRAY_COUNT * 2] = {0};
    IINT32 send_ubuf_count = 0;
    IINT32 lost_index[TEST_ARRAY_COUNT * 2] = {0};

    struct fec_buf recv_ubuf_out[TEST_ARRAY_COUNT] = {0};
    IINT32 recv_ubuf_count_out = 0;

    srand(time(NULL));

    fec_info = fec_framework_init(TEST_K, TEST_N, TEST_BLOCK_LIMIT);
    if (fec_info == NULL) {
        FEC_LOGD("fec_framework_init ret NULL"); 
        return -1;
    }

    if (truncated_packet_test(fec_info) != 0) {
        FEC_LOGD("truncated_packet_test failed");
        fec_framework_deinit(fec_info);
        return -1;
    }

    if (encode_headroom_test() != 0) {
        FEC_LOGD("encode_headroom_test failed");
        fec_framework_deinit(fec_info);
        return -1;
    }

//    if (parity_size_reset_test() != 0) {
//        FEC_LOGD("parity_size_reset_test failed");
//        fec_framework_deinit(fec_info);
//        return -1;
//    }
    
    FEC_LOGD("------------------------fec_framework_init k %d n %d--------------------------------", TEST_K, TEST_N);

    FEC_LOGD("\n----------------------------Emulate send start--------------------------------------");
    send_test(fec_info, send_ubuf, &send_ubuf_count);
    for (i = 0; i < send_ubuf_count; i++) {
        printf("%d ", i);
        ubuf_printf_test("send:", NULL, &send_ubuf[i]);
    }

    out_of_order(send_ubuf, send_ubuf_count);

    FEC_LOGD("\n----------------------------Emulate lost start--------------------------------------");
    for (i = 0; i < send_ubuf_count; i++) {
        printf("%02d ", i);
        if (rand() % 6 == 0) {
            ubuf_printf_test("send:", "(lost)", &send_ubuf[i]);
            lost_index[i] = 1;
        } else {
            ubuf_printf_test("send:", NULL, &send_ubuf[i]);
        }
    }

    FEC_LOGD("\n-----------------------------Emulate recv start--------------------------------------");
    recv_test(fec_info, send_ubuf, send_ubuf_count, lost_index, recv_ubuf_out, &recv_ubuf_count_out);

    adu_sort_test(recv_ubuf_out, recv_ubuf_count_out);
    adu_printf_test(recv_ubuf_out, recv_ubuf_count_out);
    for (i = 0; i < recv_ubuf_count_out; i++) {
        fec_buf_free(&recv_ubuf_out[i]);
    }

    fec_framework_deinit(fec_info);

    return 0;
}
