#pragma once

#include <tk/tkernel.h>
#include <cassert>

namespace osal {

namespace detail {
struct microbit_msgbuf_ctx {
    ID id;
    size_t max_msg_size;
};
}  // namespace detail

inline message_buffer::message_buffer(size_t buf_size, size_t max_msg_size) {
    static_assert(sizeof(storage_) >= sizeof(detail::microbit_msgbuf_ctx), "storage too small for microbit_msgbuf_ctx");
    auto *ctx = reinterpret_cast<detail::microbit_msgbuf_ctx *>(storage_);

    T_CMBF cmbf = {};
    cmbf.exinf = nullptr;
    cmbf.mbfatr = TA_TFIFO;
    cmbf.bufsz = static_cast<SZ>(buf_size);
    cmbf.maxmsz = static_cast<SZ>(max_msg_size);
    ID id = tk_cre_mbf(&cmbf);
    assert(id > 0);
    ctx->id = id;
    ctx->max_msg_size = max_msg_size;
}

inline message_buffer::~message_buffer() {
    auto *ctx = reinterpret_cast<detail::microbit_msgbuf_ctx *>(storage_);
    if (ctx->id > 0) {
        tk_del_mbf(ctx->id);
    }
}

inline bool message_buffer::send(const void *data, size_t size, uint32_t timeout_ms) {
    auto *ctx = reinterpret_cast<detail::microbit_msgbuf_ctx *>(storage_);
    if (size == 0 || size > ctx->max_msg_size)
        return false;
    TMO tmo = (timeout_ms == UINT32_MAX) ? TMO_FEVR : static_cast<TMO>(timeout_ms);
    ER rc = tk_snd_mbf(ctx->id, data, static_cast<SZ>(size), tmo);
    return rc == E_OK;
}

inline size_t message_buffer::receive(void *data, size_t max_size, uint32_t timeout_ms) {
    auto *ctx = reinterpret_cast<detail::microbit_msgbuf_ctx *>(storage_);
    TMO tmo = (timeout_ms == UINT32_MAX) ? TMO_FEVR : static_cast<TMO>(timeout_ms);
    INT sz = tk_rcv_mbf(ctx->id, data, tmo);
    if (sz < 0)
        return 0;
    return static_cast<size_t>(sz);
}

}  // namespace osal
