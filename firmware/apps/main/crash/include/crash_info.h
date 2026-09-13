#pragma once

/**
 * @file crash_info.h
 * @brief メインアプリケーションのクラッシュ情報
 *
 * Fault handlerがSettings pageに保存するクラッシュ情報と、
 * その読み出し・消去を定義する。
 */

#include <cstdint>

namespace crash {

/** クラッシュ情報の有効性を示すマジックナンバー ("CRAS") */
constexpr uint32_t INFO_MAGIC = 0x43524153U;

/** スタックダンプのワード数 */
constexpr uint32_t STACK_DUMP_WORDS = 16;

/** フォルト種別 */
enum class FaultType : uint32_t {
    Hard = 1,
    Mem = 2,
    Bus = 3,
    Usage = 4,
    NMI = 5,
};

/** クラッシュ情報構造体 */
struct Info {
    uint32_t magic;
    uint32_t fault_type;

    /* Exception frame (CPUが自動pushした値) */
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t xpsr;

    /* Fault status registers */
    uint32_t hfsr;
    uint32_t cfsr;
    uint32_t mmfar;
    uint32_t bfar;

    /* Context */
    uint32_t sp;
    uint32_t exc_return;

    uint32_t stack_dump[STACK_DUMP_WORDS];
};

/** フォルトハンドラの型 */
using HandlerCallback = void (*)(uint32_t type, uint32_t *frame, uint32_t exc_return);

/**
 * @brief Fault handlerと永続化処理をまとめたsingleton
 *
 * Fault handler自体は例外コンテキストで実行されるため、
 * utkernel::taskとしては実行しない。例外トランポリンからこのsingletonへ
 * ディスパッチする。
 */
class CrashHandler {
  public:
    static CrashHandler &instance();

    CrashHandler(const CrashHandler &) = delete;
    CrashHandler &operator=(const CrashHandler &) = delete;

    /** フォルトハンドラを設定する。nullptrでデフォルトに戻す */
    void set_handler(HandlerCallback callback);

    /** 例外トランポリンから呼び出される。通常は戻らない */
    [[noreturn]] void dispatch(uint32_t type, uint32_t *frame, uint32_t exc_return);

    /** Settings pageからクラッシュ情報を読み出す */
    int32_t info_read(Info *info) const;

    /** Settings pageのクラッシュ情報を消去する */
    void info_clear() const;

  private:
    constexpr CrashHandler()
        : m_handler(nullptr) {
    }

    [[noreturn]] static void default_handler(uint32_t type, uint32_t *frame, uint32_t exc_return);

    static CrashHandler s_instance;
    HandlerCallback m_handler;
};

}  // namespace crash
