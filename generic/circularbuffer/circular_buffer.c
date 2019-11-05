
#include <stdint.h>
#include <string.h>
#include "circular_buffer.h"


static void advance_pointer(circular_buf_t *cbuf, int step)
{
    if (cbuf->full)
    {
        cbuf->tail = (cbuf->tail + step) % cbuf->max;
    }

    cbuf->head = (cbuf->head + step) % cbuf->max;

    // We mark full because we will advance tail on the next time around
    cbuf->full = (cbuf->head == cbuf->tail);
}

static void retreat_pointer(circular_buf_t *cbuf, int step)
{
    cbuf->full = 0;
    cbuf->tail = (cbuf->tail + step) % cbuf->max;
}

int circular_buf_init(circular_buf_t *cbuf, uint8_t *buffer, size_t size)
{
    if (!buffer || size <= 0)
        return 1;

    cbuf->buffer = buffer;
    cbuf->max    = size;
    circular_buf_reset(cbuf);

    return 0;
}

void circular_buf_reset(circular_buf_t *cbuf)
{
    if (cbuf)
    {
        cbuf->head = 0;
        cbuf->tail = 0;
        cbuf->full = 0;
    }
}

size_t circular_buf_size(circular_buf_t *cbuf)
{
    if (!cbuf)
        return 0;

    size_t size = cbuf->max;

    if (!cbuf->full)
    {
        if (cbuf->head >= cbuf->tail)
        {
            size = (cbuf->head - cbuf->tail);
        }
        else
        {
            size = (cbuf->max + cbuf->head - cbuf->tail);
        }
    }

    return size;
}

size_t circular_buf_capacity(circular_buf_t *cbuf)
{
    if (!cbuf)
        return 0;

    return cbuf->max;
}

int circular_buf_put(circular_buf_t *cbuf, uint8_t *data, int len)
{
    if (!(cbuf && cbuf->buffer))
        return -1;

    while (len > 0)
    {
        int chunk = len > cbuf->max - cbuf->head ? cbuf->max - cbuf->head : len;
        memcpy(&cbuf->buffer[cbuf->head], data, chunk);
        advance_pointer(cbuf, chunk);
        data += chunk;
        len -= chunk;
    }

    return 0;
}

static int _circular_buf_get(circular_buf_t *cbuf, uint8_t *data, int len, int consume)
{
    int read = 0;
    if (!(cbuf && cbuf->buffer))
        return -1;

    len = len < circular_buf_size(cbuf) ? len : circular_buf_size(cbuf);

    while (len > 0)
    {
        int chunk;
        if (cbuf->head > cbuf->tail)
            chunk = cbuf->head - cbuf->tail;
        else
            chunk = cbuf->max - cbuf->tail;

        chunk = chunk > len ? len : chunk;

        if (data) {
            memcpy(data, &cbuf->buffer[cbuf->tail], chunk);
            data += chunk;
        }
        read += chunk;
        len -= chunk;

        if (consume)
            retreat_pointer(cbuf, chunk);
    }

    return read;
}



int circular_buf_peek(circular_buf_t *cbuf, uint8_t *data, int len)
{
    return _circular_buf_get(cbuf, data, len, 0);
}

int circular_buf_get(circular_buf_t *cbuf, uint8_t *data, int len)
{
    return _circular_buf_get(cbuf, data, len, 1);
}

int circular_buf_empty(circular_buf_t *cbuf)
{
    if (!cbuf)
        return 1;

    return (!cbuf->full && (cbuf->head == cbuf->tail));
}

int circular_buf_full(circular_buf_t *cbuf)
{
    if (!cbuf)
        return 0;

    return cbuf->full;
}