#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "fec_buf.h"

IINT32 fec_buf_size(const struct fec_buf *b)
{
    return b->end - b->head;
}

IINT32 fec_buf_length(const struct fec_buf *b)
{
    return b->tail - b->data;
}

static inline IINT32 fec_buf_headroom(const struct fec_buf *b)
{
    return b->data - b->head;
}

static inline IINT32 fec_buf_tailroom(const struct fec_buf *b)
{
    return b->end - b->tail;
}

IINT32 fec_buf_resize(struct fec_buf *b, IINT32 size)
{
    IUINT8 *head;
    IINT32 data_len = fec_buf_length(b);

    head = calloc(1, size);
    if (head == NULL)
        return -1;

    b->head = b->data = head;
    b->tail = b->data + data_len;
    b->end = b->head + size;

    if (b->tail > b->end)
        b->tail = b->end;

    return 0;
}

IINT32 fec_buf_init(struct fec_buf *b, IINT32 size, IINT32 offset)
{
    IINT32 ret;

    memset(b, 0, sizeof(struct fec_buf));

    if (size > 0) {
        if ((ret = fec_buf_resize(b, size)) != 0)
            return ret;
        b->data += offset;
        b->tail = b->data;
    }

    return 0;
}

void fec_buf_free(struct fec_buf *b)
{
    if (b->head) {
        free(b->head);
        memset(b, 0, sizeof(struct fec_buf));
    }
}

void *fec_buf_put(struct fec_buf *b, IINT32 len)
{
    void *tmp;

    if (fec_buf_tailroom(b) < len) {
        FEC_LOGE("fec_buf_tailroom(b) %d len %d", fec_buf_tailroom(b), len);
        return NULL;
    }

    tmp = b->tail;
    b->tail += len;

    return tmp;
}

void *fec_buf_put_data(struct fec_buf *b, const void *data, IINT32 len)
{
    void *tmp = fec_buf_put(b, len);

    if (tmp)
        memcpy(tmp, data, len);
    return tmp;
}


void *fec_buf_push(struct fec_buf *b, IINT32 len)
{
    if (fec_buf_headroom(b) < len)
        return NULL;

    b->data -= len;

    return b->data;
}

IINT32 fec_buf_pull_data(struct fec_buf *b, void *dest, IINT32 len)
{
    if (len > fec_buf_length(b))
        len = fec_buf_length(b);

    if (dest)
        memcpy(dest, b->data, len);

    b->data += len;

    return len;
}

IINT32 fec_buf_pull(struct fec_buf *b, IINT32 len)
{
    if (len > fec_buf_length(b))
        len = fec_buf_length(b);

    b->data += len;

    return len;
}

IINT32 fec_buf_get(struct fec_buf *b, IINT32 offset, void *dest, IINT32 len)
{
    if (fec_buf_length(b) - 1 < offset)
        return 0;

    if (len > fec_buf_length(b) - offset)
        len = fec_buf_length(b) - offset;

    if (len > 0)
        memcpy(dest, b->data + offset, len);

    return len;
}

void *fec_buf_data(const struct fec_buf *b)
{
    return b->data;
}

IINT32 fec_buf_copy(struct fec_buf *dst, struct fec_buf *src)
{
    IINT32 ret;

    if ((ret = fec_buf_init(dst, fec_buf_length(src), 0)) != 0) {
        return ret;
    }
    if (fec_buf_put_data(dst, fec_buf_data(src), fec_buf_length(src)) == NULL) {
        return -1;
    }
    return 0;
}
