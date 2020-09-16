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
//     0.0.15 pthread removed. Changed timeout judgment of Socket to idle event.
//     0.0.16 Connection count control added.
//     ----------
//     0.1.0  Change base system. (CentOS8, OpenSSL-1.1.x, etc)
//     ----------
//     *Later versions have moved to another project!*
//     0.x.x  Added API for upper module.
//     0.x.x  Added HTTP module. (Evs+FastCGI)
//     0.x.x  Added HTTPS module. (Evs+FastCGI)
//     0.x.x  Added FCGI module. (Evs+FastCGI)
//     0.x.x  Added PostgreSQL module. (TBD)
//     0.x.x  Added PostgreSQL module. (TBD)
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
// 定数宣言
// --------------------------------

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
// ダンプ文字列生成処理(dump_strに対して、targetdataからtagetlenバイトのダンプ文字列を設定して返す)
// --------------------------------
char *getdumpstr(void *targetdata, int targetlen)
{
    char                        *dump_str = NULL;                       // ダンプ結果文字列ポインタ
    char                        *dump_hex_pos;                          // ダンプ結果HEX文字位置
    char                        *dump_char_pos;                         // ダンプ結果CHAR文字位置

    unsigned char               *target_ptr;                            // 対象データポインタ
    int                         target_column;                          // 対象表示文字位置

    // ダンプ結果文字列は最低でも16バイト分の文字列＝70文字(\n\0含む)になる
    dump_str = (char *)calloc(1, ((targetlen + 16 - 1) / 16) * 70);     /// ←なんか計算が変みたいだー
    if (dump_str == NULL)
    {
        return dump_str;
    }
    
    // ダンプ結果HEX文字位置を設定
    dump_hex_pos = dump_str;
    // ダンプ結果CHAR文字位置を設定
    dump_char_pos = dump_hex_pos + 52;

    // 対象データポインタに、対象データの最初のバイトポインタを設定して、長さ分だけループ
    for (target_ptr = (unsigned char *)targetdata; target_ptr < (unsigned char *)targetdata + targetlen; )
    {
        // 8バイト分ループ
        for (target_column = 0; target_column < 8; target_column++)
        {
            // まずは上位4ビットを一文字分設定
            *dump_hex_pos = *target_ptr;    // 0xb6
            *dump_hex_pos >>= 4;            // 0x0b
            *dump_hex_pos &= 0x0F;          // 0x0b
            if (*dump_hex_pos >= 0x00 && *dump_hex_pos <= 0x09)
            {
                *dump_hex_pos += 0x30;      // 0x30..0x39
            }
            else
            {
                *dump_hex_pos += 0x57;      // 0x61..0x66
            }
            // ダンプ結果HEX文字位置を+1進める
            dump_hex_pos += 1;

            // 次に下位4ビットを一文字分設定
            *dump_hex_pos = *target_ptr;    // 0x4a
            *dump_hex_pos &= 0x0F;          // 0x0a
            if (*dump_hex_pos >= 0x00 && *dump_hex_pos <= 0x09)
            {
                *dump_hex_pos += 0x30;      // 0x30..0x39
            }
            else
            {
                *dump_hex_pos += 0x57;      // 0x61..0x66
            }
            // ダンプ結果HEX文字位置を+1進める
            dump_hex_pos += 1;

            // スペース一つあけて
            *dump_hex_pos = ' ';
            // ダンプ結果HEX文字位置を+1進める
            dump_hex_pos += 1;

            if (*target_ptr >= 0x20 && *target_ptr <= 0x7e)
            {
                *dump_char_pos = *target_ptr;
            }
            else
            {
                *dump_char_pos = '.';
            }
            // ダンプ結果CHAR文字位置を+1進める
            dump_char_pos += 1;
            // 対象データポインタを+1進める
            target_ptr++;
        }

        // スペース一つあける
        *dump_hex_pos = ' ';
        *dump_char_pos = ' ';
        // ダンプ結果HEX文字位置を+1進める
        dump_hex_pos += 1;
        // ダンプ結果CHAR文字位置を+1進める
        dump_char_pos += 1;

        // 8バイト分ループ
        for (target_column = 0; target_column < 8; target_column++)
        {
            // まずは上位4ビットを一文字分設定
            *dump_hex_pos = *target_ptr;    // 0x30
            *dump_hex_pos >>= 4;            // 0x03
            *dump_hex_pos &= 0x0F;          // 0x03
            if (*dump_hex_pos >= 0x00 && *dump_hex_pos <= 0x09)
            {
                *dump_hex_pos += 0x30;      // 0x30..0x39
            }
            else
            {
                *dump_hex_pos += 0x57;      // 0x61..0x66
            }
            // ダンプ結果HEX文字位置を+1進める
            dump_hex_pos += 1;

            // 次に下位4ビットを一文字分設定
            *dump_hex_pos = *target_ptr;    // 0x30
            *dump_hex_pos &= 0x0F;          // 0x00
            if (*dump_hex_pos >= 0x00 && *dump_hex_pos <= 0x09)
            {
                *dump_hex_pos += 0x30;      // 0x30..0x39
            }
            else
            {
                *dump_hex_pos += 0x57;      // 0x61..0x66
            }
            // ダンプ結果HEX文字位置を+1進める
            dump_hex_pos += 1;

            // スペース一つあけて
            *dump_hex_pos = ' ';
            // ダンプ結果HEX文字位置を+1進める
            dump_hex_pos += 1;

            if (*target_ptr >= 0x20 && *target_ptr <= 0x7e)
            {
                *dump_char_pos = *target_ptr;
            }
            else
            {
                *dump_char_pos = '.';
            }
            // ダンプ結果CHAR文字位置を+1進める
            dump_char_pos += 1;
            // 対象データポインタを+1進める
            target_ptr++;
        }

        // 3バイト分ループ
        for (target_column = 0; target_column < 3; target_column++)
        {
            // スペース一つあけて
            *dump_hex_pos = ' ';
            // ダンプ結果HEX文字位置を+1進める
            dump_hex_pos += 1;
        }

        // まだデータがあるなら
        if (target_ptr < (unsigned char *)targetdata + targetlen)
        {
            // 改行(=0x0a)をつける
            *dump_char_pos = 0x0A;
        }
        // そうではないなら
        else
        {
            // 文字列の終わり(=0x00)を設定する
            *dump_char_pos = 0x00;            
        }
        // ダンプ結果HEX文字位置を改行の次(+1)に進める
        dump_hex_pos = dump_char_pos + 1;
        // ダンプ結果CHAR文字位置を+53進める
        dump_char_pos = dump_hex_pos + 52;
    }
    return dump_str;
}

// --------------------------------
// ログ出力処理
// --------------------------------
void logging(int level, char * logstr)
{
    // ログレベルが設定値以上なら
    if (level >= EVS_config.log_level)
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
                // ログファイルを書き込みで開く(ノンブロッキングにするなら「 | O_NONBLOCK」を追加)
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

    // ログファイル名の領域を最後に解放
    free(EVS_config.log_file);

    return 0;
}