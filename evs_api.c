// ----------------------------------------------------------------------
// EvServer - Libev Server (Sample) -
// Purpose:
//     Various initialization processing.
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

#include <pthread.h>

#include "evs_main.h"

// --------------------------------
// 型宣言
// --------------------------------
struct EVS_api_t {                                          // APIのpthread用に必要な変数をまとめた拡張構造体
    struct EVS_ev_client_t *client;                         // クライアント用拡張構造体ポインタ
    char *msg_buf;                                          // クライアントから受信したメッセージのポインタ
    ssize_t msg_len;                                        // クライアントから受信したメッセージ長
};

// --------------------------------
// 変数宣言
// --------------------------------

// ----------------------------------------------------------------------
// コード部分
// ----------------------------------------------------------------------
// --------------------------------
// 通常出力処理(pthreadで呼び出す関数はvoid *型となる)
// --------------------------------
void *API_print(void *arg)
{
    int                             api_result;
    char                            log_str[MAX_LOG_LENGTH];
    struct EVS_api_t                *thread_arg = (struct EVS_api_t *)arg;      // pthreadで渡される引数ポインタを、本来のAPI用拡張構造体ポインタとして変換する

    // 受信したメッセージをとりあえず表示する
    snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): message len=%d, %s", __func__, thread_arg->client->socket_fd, thread_arg->msg_len, thread_arg->msg_buf);
    logging(LOGLEVEL_INFO, log_str);

    int                             socket_result;
    struct EVS_ev_client_t          *other_client;                                      // ev_io＋ソケットファイルディスクリプタ＋次のTAILQ構造体への接続とした拡張構造体
    struct EVS_ev_client_t          *this_client = thread_arg->client;                  // libevから渡されたwatcherポインタを、本来の拡張構造体ポインタとして変換する

    // ----------------
    // 非SSL通信(=0)なら
    // ----------------
    if (this_client->ssl_status == 0)
    {
        // ----------------
        // ソケット送信(send : ソケットのファイルディスクリプタに対して、msg_bufからmsg_lenのメッセージを送信する。(ノンブロッキングにするなら0ではなくてMSG_DONTWAIT)
        // ----------------
        socket_result = send(this_client->socket_fd, (void*)thread_arg->msg_buf, thread_arg->msg_len, 0);
        // 送信したバイト数が負(<0)だったら(エラーです)
        if (socket_result < 0)
        {
            snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): send(): Cannot send message? errno=%d (%s)\n", __func__, this_client->socket_fd, errno, strerror(errno));
            logging(LOGLEVEL_ERROR, log_str);
            return;
        }
        snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): send(): OK.\n", __func__, this_client->socket_fd);
        logging(LOGLEVEL_INFO, log_str);
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
        socket_result = SSL_write(this_client->ssl, (void*)thread_arg->msg_buf, thread_arg->msg_len);
        // 送信したバイト数が負(<0)だったら(エラーです)
        if (socket_result < 0)
        {
            snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): SSL_write(): Cannot write encrypted message!?\n", __func__, this_client->socket_fd, ERR_reason_error_string(ERR_get_error()));
            logging(LOGLEVEL_ERROR, log_str);
            return;
        }
        snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): SSL_write(): OK.\n", __func__, this_client->socket_fd);
        logging(LOGLEVEL_INFO, log_str);
    }
    snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): PTHREAD DETACHED.\n", __func__);
    logging(LOGLEVEL_INFO, log_str);

    // スレッド処理終了(exitでもいいけど、デタッチ状態で生成しているから、returnしたらexitと同様)
    return;

}

// --------------------------------
// API開始処理(クライアント別処理分岐、スレッド生成など)
// --------------------------------
int API_start(struct EVS_ev_client_t *client, char *msg_buf, ssize_t msg_len)
{
    int                             api_result;
    char                            log_str[MAX_LOG_LENGTH];

    pthread_t                       thread_id;                          // スレッド識別変数
    pthread_attr_t                  thread_attr;
    struct EVS_api_t                thread_arg;                         // APIのpthread用に必要な変数をまとめた構造体

    // とりあえず表示する
    snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): OK!\n", __func__, client->socket_fd);
    logging(LOGLEVEL_INFO, log_str);

    // スレッド属性を初期化
    api_result = pthread_attr_init(&thread_attr);
    // スレッド属性を初期化できなかったら(エラーです)
    if (api_result != 0)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): Cannot initialize thread attribute!? errno=%d (%s)\n", __func__, errno, strerror(errno));
        logging(LOGLEVEL_ERROR, log_str);
        return api_result;
    }
    // スレッドをデタッチ状態で生成するように設定
    api_result = pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
    // スレッド属性を初期化できなかったら(エラーです)
    if (api_result != 0)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): Cannot set PTHREAD_CREATE_DETACHED!? errno=%d (%s)\n", __func__, errno, strerror(errno));
        logging(LOGLEVEL_ERROR, log_str);
        return api_result;
    }

    // APIのpthread用に必要な変数をまとめた構造体に各種ポインタを設定
    thread_arg.client = client;                                         // クライアント用拡張構造体ポインタ
    thread_arg.msg_buf = msg_buf;                                       // クライアントから受信したメッセージのポインタ
    thread_arg.msg_len = msg_len;                                       // クライアントから受信したメッセージ長

    // ポート別の指定API分岐処理…だけど、今は特に何もしてない
    switch (client->ssl_status)
    {
/*
        case API_PGSQL    :
        case API_HTTP     :
        case API_CHAT     :
            break;
*/
        default            :    // シンプルにクライアントにエコーバックします
            // 通常出力処理スレッドを生成(スレッドはデタッチ状態で生成するのでpthread_join()する必要がない
            api_result = pthread_create(&thread_id, &thread_attr, &API_print, &thread_arg);
    }

    // スレッドを生成できなかったら(エラーです)
    if (api_result != 0)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): pthread_create(): Cannot create %d thread attribute!? errno=%d (%s)\n", __func__, client->ssl_status, errno, strerror(errno));
        logging(LOGLEVEL_ERROR, log_str);
        return api_result;
    }
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): pthread_create(): OK!!!!\n", __func__, client->ssl_status, errno, strerror(errno));
    logging(LOGLEVEL_INFO, log_str);

    // 戻る(実際にはスレッド生成して戻っているだけ)
    return 0;
}
