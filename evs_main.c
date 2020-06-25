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
//     0.0.12 Daemon mode support.
//     0.0.13 Client Address support.
//     0.0.14 pthread support.
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
// ログ出力処理
// --------------------------------
void logging(int level, char * logstr)
{
    // デーモンモードではないなら
    if (EVS_config.daemon != 1)
    {
        // 標準出力
        printf("%s : %s", loglevel_list[level], logstr);
    }
    else
    {
        // ログファイル名の指定があるのにログファイルが開いていなければ
        if (EVS_config.log_file != NULL && EVS_log_fd == 0)
        {
            // ログファイルを書き込みで開く
            EVS_log_fd = open(EVS_config.log_file, (O_WRONLY | O_CREAT | O_APPEND), (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
        }
        // ログファイルが開けていないなら
        if (EVS_log_fd == 0 || EVS_log_fd == -1)
        {
            // 標準出力
            printf("%s : %s", loglevel_list[level], logstr);
        }
        else
        {
            // ログファイルに出力
            write(EVS_log_fd, logstr, strlen(logstr));
        }
    }
}

// --------------------------------
// メイン処理
// --------------------------------
int main (int argc, char *argv[])
{
    int                             result;
    char                            log_str[MAX_LOG_LENGTH];

    // --------------------------------
    // 初期化処理
    // --------------------------------
    result = INIT_all(argc, argv);

    // 処理結果がOK(=0)なら
    if (result == 0)
    {
        // ----------------
        // イベントループ開始(libev Ver4.x以降はev_run() 他にもいくつか変更点あり。flag=0がデフォルトで、ノンブロッキングはEVRUN_NOWAIT=1、一度きりはEVRUN_ONCE=2)
        // ----------------
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): ev_run(): Go!\n", __func__);               // daemon(0, 0): を呼ぶ前にログを出力
        logging(LOGLEVEL_INFO, log_str);
        ev_run(EVS_loop, 0);
    }
    // ----------------
    // 作成したイベントループの破棄
    // ----------------
    ev_loop_destroy(EVS_loop);
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): ev_loop_destroy(): Go!\n", __func__);          // daemon(0, 0): を呼ぶ前にログを出力
    logging(LOGLEVEL_INFO, log_str);

    // --------------------------------
    // 終了処理
    // --------------------------------
    CLOSE_all();
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): CLOSE_all(): Go!\n", __func__);                // daemon(0, 0): を呼ぶ前にログを出力
    logging(LOGLEVEL_INFO, log_str);

    return 0;
}
