#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "fec_framework.h"
#include "fec_buf.h"

#define TEST_COUNT      5
#define TEST_DATA_LEN   200
#define TEST_HEAD_LEN   20

#define TEST_K  5
#define TEST_N  7

#define TEST_ARRAY_COUNT (TEST_COUNT + TEST_COUNT / TEST_K * (TEST_N - TEST_K))

/* for test func */
void
ubuf_printf_test(IINT8 *act, IINT8 * act2, struct fec_buf *ubuf);

void
adu_printf_test(struct fec_buf *ubuf, IINT32 ubuf_count);

void
ubuf_free_test(struct fec_buf *ubuf, IINT32 count);

void
adu_printf_test(struct fec_buf *ubuf, IINT32 ubuf_count)
{
    IINT32 i;

    for (i = 0; i < ubuf_count; i++) {
        FEC_LOGD("%d recver: len %d\t%s", i, fec_buf_length(&ubuf[i]), (IINT8 *)fec_buf_data(&ubuf[i]));
    }
    FEC_LOGD("++++++++++++");
}

void
ubuf_printf_test(IINT8 *act, IINT8 * act2, struct fec_buf *ubuf)
{
    static IUINT16 last_seq_id = 0xeabc;
    IUINT16 cur_seq_id;
    fec_header_t *fec_header_ptr;
    fec_adu_info_t *adu_info_ptr;

    fec_header_ptr = fec_buf_data(ubuf);
    adu_info_ptr = (fec_adu_info_t *)fec_header_ptr->fdu;

    cur_seq_id = ntohs(fec_header_ptr->seq_id);
    if (act2) {
        FEC_LOGD("%s seq %u len %d\t%s %s %s",
                        act, cur_seq_id, ntohs(adu_info_ptr->len), 
                        fec_header_ptr->buf_type == UKCP_FEC_TYPE_PARITY_PACKET ? "PARITY" : (char *)adu_info_ptr->adu,
                        cur_seq_id == last_seq_id ? "(dup)" : " ", act2);
    } else {
        FEC_LOGD("%s seq %u len %d\t%s %s",
                act, cur_seq_id, ntohs(adu_info_ptr->len),
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

    sprintf(test_buf, "%u", seq);
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
//            if (rand() % 8 == 0) {
//                fec_buf_copy(&send_ubuf[(*send_ubuf_count)++], &out_ubuf[j]);
//            }
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
        ret = fec_framework_decode(fec_info, &recv_ubuf_in[i], &recv_out_ubuf, &recv_out_ubuf_count);
        if (ret != 0) {
//            FEC_LOGD("F %d %s line %d", i, (char *)fec_buf_data(&recv_ubuf_in[i]), __LINE__);
            fec_buf_free(&recv_ubuf_in[i]);
        }

        for (j = 0; j < recv_out_ubuf_count; j++) {
            fec_buf_copy(&recv_ubuf_out[(*recv_ubuf_count_out)++], &recv_out_ubuf[j]);
        }

        ubuf_free_test(recv_out_ubuf, recv_out_ubuf_count);
    }
}

void
out_of_order(struct fec_buf *ubuf, IINT32 count)
{
    int i;
    int j, q;

    FEC_LOGD("\n-----------------------Emulate out of order start-------------------------------------");
    for (i = 0; i < 1; i++) {
        j = rand() % (count);
        q = rand() % (count);
        struct fec_buf temp = ubuf[q];

        ubuf[q] = ubuf[j];
        ubuf[j] = temp;
    }

    for (i = 0; i < count; i++) {
        printf("%d ", i);
        ubuf_printf_test("send:", NULL, &ubuf[i]);
    }
}

IINT32
main(void)
{
    IINT32 i;
    fec_info_t *fec_info;
    struct fec_buf send_ubuf[TEST_ARRAY_COUNT] = {0};
    IINT32 send_ubuf_count = 0;
    IINT32 lost_index[TEST_ARRAY_COUNT] = {0};

    struct fec_buf recv_ubuf_out[TEST_ARRAY_COUNT] = {0};
    IINT32 recv_ubuf_count_out = 0;

    srand(time(NULL));

    fec_info = fec_framework_init(TEST_K, TEST_N);
    if (fec_info == NULL) {
        FEC_LOGD("fec_framework_init ret NULL"); 
        return -1;
    }
    
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
        printf("%d ", i);
        if (rand() % 5 == 0) {
            ubuf_printf_test("send:", "(lost)", &send_ubuf[i]);
            lost_index[i] = 1;
        } else {
            ubuf_printf_test("send:", NULL, &send_ubuf[i]);
        }
    }

    FEC_LOGD("\n-----------------------------Emulate recv start--------------------------------------");
    recv_test(fec_info, send_ubuf, send_ubuf_count, lost_index, recv_ubuf_out, &recv_ubuf_count_out);

    adu_printf_test(recv_ubuf_out, recv_ubuf_count_out);
    for (i = 0; i < recv_ubuf_count_out; i++) {
        fec_buf_free(&recv_ubuf_out[i]);
    }

    fec_framework_deinit(fec_info);

    return 0;
}
