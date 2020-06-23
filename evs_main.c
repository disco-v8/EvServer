// ----------------------------------------------------------------------
// EvServer - Libev Server (Sample) -
// Purpose:
//     Base process of Server and Client.
//
// Version:
//     0.0.1  libv trial program
//     0.0.2  libev + UNIX Domain Socket Server trial program
//     0.0.3  Changed for automake
//     0.0.4  Separate initialization process, and Measures for generalization
//     0.0.5  Support PF_INET/PF_INET6 connection
//     0.0.6  SSL/TLS Support
//     0.0.7  Introduced a configuration file & Change to process by listen port
//     0.0.8  OpenSSL's BIO support (Discarded version!)
//     0.0.9  Change to Non-Blocking FD
//     0.0.10 No-communication timer
//     0.0.11 Bug fix free() and ?alloc() parameter.
//     0.x.x  Daemon mode support. (TBD)
//     0.x.x  Added API for upper module. (TBD)
//     0.x.x  Added PostgreSQL module. (TBD)
//     0.x.x  Added PostgreSQL module. (TBD)
//     0.x.x  Added HTTP module. (TBD)
//     0.x.x  Added HTTPS module. (TBD)
//     0.x.x  Added OREORE module. (TBD)
//
// Program:
//     Takeshi Kaburagi/MyDNS.JP    disco-v8@4x4.jp
//
// Usage:
//     ./evserver [/etc/evserver/evserver.ini]
// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
// ヘッダ部分
// ----------------------------------------------------------------------
// --------------------------------
// インクルード宣言
// --------------------------------
// autoconf用宣言
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "evs_main.h"

// --------------------------------
// 型宣言
// --------------------------------

// --------------------------------
// 変数宣言
// --------------------------------

// ----------------------------------------------------------------------
// コード部分
// ----------------------------------------------------------------------

// --------------------------------
// メイン処理
// --------------------------------
int main (int argc, char *argv[])
{
    // --------------------------------
    // 初期化処理
    // --------------------------------
    INIT_all(argc, argv);

    // ----------------
    // イベントループ開始(libev Ver4.x以降はev_run() 他にもいくつか変更点あり。flag=0がデフォルトで、ノンブロッキングはEVRUN_NOWAIT=1、一度きりはEVRUN_ONCE=2)
    // ----------------
    printf("INFO  : ev_run(): Go!\n");                                  // en_run()から出てくるときはイベントループが終わった時なので、呼ぶ前にログを出力
    ev_run(EVS_loop, 0);

    // ----------------
    // 作成したイベントループの破棄
    // ----------------
    ev_loop_destroy(EVS_loop);
    printf("INFO  : ev_loop_destroy(): OK.\n");

    // --------------------------------
    // 終了処理
    // --------------------------------
    CLOSE_all();
    printf("INFO  : CLOSE_all(): OK.\n");

    return 0;
}
