#ifndef __UKCP_BUF_H__
#define __UKCP_BUF_H__

#include "fec_platform.h"

struct fec_buf {
    IUINT8 *head;
    IUINT8 *data;
    IUINT8 *tail;
    IUINT8 *end;
};

IINT32 fec_buf_init(struct fec_buf *b, IINT32 size, IINT32 offset);

IINT32 fec_buf_resize(struct fec_buf *b, IINT32 size);

void fec_buf_free(struct fec_buf *b);

IINT32 fec_buf_length(const struct fec_buf *b);

IINT32 fec_buf_size(const struct fec_buf *b);

void *fec_buf_data(const struct fec_buf *b);

void *fec_buf_put(struct fec_buf *b, IINT32 len);

void *fec_buf_put_data(struct fec_buf *b, const void *data,   IINT32 len);

void *fec_buf_push(struct fec_buf *b, IINT32 len);

IINT32 fec_buf_pull_data(struct fec_buf *b, void *dest, IINT32 len);

IINT32 fec_buf_pull(struct fec_buf *b, IINT32 len);

IINT32 fec_buf_get(struct fec_buf *b, IINT32 offset, void *dest, IINT32 len);

IINT32 fec_buf_copy(struct fec_buf *dst, struct fec_buf *src);
#endif
