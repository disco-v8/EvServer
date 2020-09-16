// ----------------------------------------------------------------------
// EvServer - Libev Server (Sample) -
// Purpose:
//     Various API processing.
//
// Program:
//     Takeshi Kaburagi/MyDNS.JP    disco-v8@4x4.jp
//
// Usage:
//     ./evserver [OPTIONS]
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
// 通常出力処理
// --------------------------------
int API_print(struct EVS_ev_client_t *this_client, char *msg_buf, ssize_t msg_len)
{
    int                             api_result;
    char                            log_str[MAX_LOG_LENGTH];

    if (EVS_config.log_level <= LOGLEVEL_DEBUG)
    {
        // 受信したメッセージをとりあえず表示する
        snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): message len=%d, %s", __func__, this_client->socket_fd, msg_len, msg_buf);
        logging(LOGLEVEL_DEBUG, log_str);
    }
    
    // ----------------
    // 非SSL通信(=0)なら
    // ----------------
    if (this_client->ssl_status == 0)
    {
        // ----------------
        // ソケット送信(send : ソケットのファイルディスクリプタに対して、msg_bufからmsg_lenのメッセージを送信する。(ノンブロッキングにするなら0ではなくてMSG_DONTWAIT)
        // ----------------
        api_result = send(this_client->socket_fd, (void*)msg_buf, msg_len, 0);
        // 送信したバイト数が負(<0)だったら(エラーです)
        if (api_result < 0)
        {
            snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): send(): Cannot send message? errno=%d (%s)\n", __func__, this_client->socket_fd, errno, strerror(errno));
            logging(LOGLEVEL_ERROR, log_str);
            return api_result;
        }
        if (EVS_config.log_level <= LOGLEVEL_DEBUG)
        {
            snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): send(): OK.\n", __func__, this_client->socket_fd);
            logging(LOGLEVEL_DEBUG, log_str);
        }
    }
    // ----------------
    // SSLハンドシェイク前なら
    // ----------------
    else if (this_client->ssl_status == 1)
    {
        // ERROR!?!? (TBD)
    }
    // ----------------
    // SSL接続中なら
    // ----------------
    else if (this_client->ssl_status == 2)
    {
        // ----------------
        // OpenSSL(SSL_write : SSLデータ書き込み)
        // ----------------
        api_result = SSL_write(this_client->ssl, (void*)msg_buf, msg_len);
        // 送信したバイト数が負(<0)だったら(エラーです)
        if (api_result < 0)
        {
            snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): SSL_write(): Cannot write encrypted message!?\n", __func__, this_client->socket_fd, ERR_reason_error_string(ERR_get_error()));
            logging(LOGLEVEL_ERROR, log_str);
            return api_result;
        }
        if (EVS_config.log_level <= LOGLEVEL_DEBUG)
        {
            snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): SSL_write(): OK.\n", __func__, this_client->socket_fd);
            logging(LOGLEVEL_DEBUG, log_str);
        }
    }
    // 戻る
    return 0;
}

// --------------------------------
// API開始処理(クライアント別処理分岐、スレッド生成など)
// --------------------------------
int API_start(struct EVS_ev_client_t *this_client, char *msg_buf, ssize_t msg_len)
{
    int                             api_result;
    char                            log_str[MAX_LOG_LENGTH];

    if (EVS_config.log_level <= LOGLEVEL_INFO)
    {
        // とりあえず表示する
        snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): START!\n", __func__, this_client->socket_fd);
        logging(LOGLEVEL_INFO, log_str);
    }
/*
    // ポート別の指定API分岐処理…だけど、今は特に何もしてない
    switch (this_client->ポート別API指定、とか)
    {
        case API_PGSQL    :
            // PostgreSQL処理を通常呼出
            api_result = API_pgsql(this_client, msg_buf, msg_len);
            break;
        case API_HTTP     :
            // HTTP処理を通常呼出
            api_result = API_http(this_client, msg_buf, msg_len);
            break;
        default            :
*/
            // 通常出力処理を通常呼出
            api_result = API_print(this_client, msg_buf, msg_len);
/*
    }
*/
    // クライアント毎のリクエストで処理時間が変わるので、スレッド化して分散処理をしたほうがいいかと思ったけれど、スレッドを生成すると、クライアントからの接続をとりこぼす事象が発生した。
    // この後でHTTP処理について作りこみをしたが、少なくともabでApache(Prefork)との処理速度比較をすると、変にスレッド生成しないのが一番速いという結果がでた。
    // libevとpthreadの関係だと思うけど、深いところまでは追っていないので、調べて教えてくれると嬉しいです。

    // 戻る
    return api_result;
}