// ----------------------------------------------------------------------
// EvServer - Libev Server (Sample) -
// Purpose:
//     Various closing processing.
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
// クライアント接続終了処理(イベントの停止、クライアントキューからの削除、SSL接続情報開放、ソケットクローズ、クライアント情報開放)
// --------------------------------
void CLOSE_client(struct ev_loop* loop, struct ev_io *watcher, int revents)
{
    int                             socket_result;
    char                            log_str[MAX_LOG_LENGTH];

    struct EVS_ev_client_t          *this_client = (struct EVS_ev_client_t *)watcher;   // libevから渡されたwatcherポインタを、本来の拡張構造体ポインタとして変換する

    // この接続のイベントを停止する
    ev_io_stop(loop, &this_client->io_watcher);
    snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): ev_io_stop(): OK.\n", __func__, this_client->socket_fd);
    logging(LOGLEVEL_DEBUG, log_str);

    // クライアント用テールキューからこの接続の情報を削除する
    TAILQ_REMOVE(&EVS_client_tailq, this_client, entries);
    snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): TAILQ_REMOVE(EVS_client_tailq): OK.\n", __func__, this_client->socket_fd);
    logging(LOGLEVEL_DEBUG, log_str);

    // クライアント毎のリクエストヘッダが確保されているなら
    if (this_client->request_buf != NULL)
    {
        free(this_client->request_buf);
    }

    // SSLハンドシェイク前、もしくはSSL接続中なら
    if (this_client->ssl_status != 0)
    {
        // ----------------
        // OpenSSL(SSL_free : SSL接続情報のメモリ領域を開放)
        // ----------------
        SSL_free(this_client->ssl);
        snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): SSL_free(): OK.\n", __func__, this_client->socket_fd);
        logging(LOGLEVEL_DEBUG, log_str);
    }

    // この接続のソケットを閉じる
    socket_result = close(this_client->socket_fd);
    // ソケットのクローズ結果がエラーだったら
    if (socket_result < 0)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): close(): Cannot socket close? errno=%d (%s)\n", __func__, this_client->socket_fd, errno, strerror(errno));
        logging(LOGLEVEL_ERROR, log_str);
        return;
    }
    snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): close(): OK.\n", __func__, this_client->socket_fd);
    logging(LOGLEVEL_INFO, log_str);

    // この接続のクライアント用拡張構造体のメモリ領域を開放する
    free(this_client);

    return;
}

// --------------------------------
// 終了処理
// --------------------------------
int CLOSE_all(void)
{
    int                             close_result;
    char                            log_str[MAX_LOG_LENGTH];

    struct EVS_ev_client_t          *client_watcher;                    // クライアント別設定用構造体ポインタ
    struct EVS_ev_server_t          *server_watcher;                    // サーバー別設定用構造体ポインタ
    struct EVS_timer_t              *timeout_watcher;                   // タイマー別構造体ポインタ
    struct EVS_port_t               *listen_port;                       // ポート別設定用構造体ポインタ

    // --------------------------------
    // クライアント別クローズ処理
    // --------------------------------
    // クライアント用テールキューからポート情報を取得して全て処理
    TAILQ_FOREACH (client_watcher, &EVS_client_tailq, entries)
    {
        // ----------------
        // ソケット終了(close : ソケットのファイルディスクリプタを閉じる)
        // ----------------
        close_result = close(client_watcher->socket_fd);
        // ソケットが閉じれなかったら
        if (close_result < 0)
        {
            snprintf(log_str, MAX_LOG_LENGTH, "%s(): close(fd=%d): Cannot socket close? errno=%d (%s)\n", __func__, client_watcher->socket_fd, errno, strerror(errno));
            logging(LOGLEVEL_ERROR, log_str);
            return -1;
        }
       snprintf(log_str, MAX_LOG_LENGTH, "%s(): close(fd=%d): OK.\n", __func__, client_watcher->socket_fd);
       logging(LOGLEVEL_INFO, log_str);
    }
    // クライアント用テールキューをすべて削除
    while (!TAILQ_EMPTY(&EVS_client_tailq))
    {
        client_watcher = TAILQ_FIRST(&EVS_client_tailq);
        TAILQ_REMOVE(&EVS_client_tailq, client_watcher, entries);
        free(client_watcher);
    }
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): TAILQ_REMOVE(EVS_client_tailq): OK.\n", __func__);
    logging(LOGLEVEL_DEBUG, log_str);

    // --------------------------------
    // サーバー別クローズ処理
    // --------------------------------
    // サーバー用テールキューからポート情報を取得して全て処理
    TAILQ_FOREACH (server_watcher, &EVS_server_tailq, entries)
    {
        // ----------------
        // ソケット終了(close : ソケットのファイルディスクリプタを閉じる)
        // ----------------
        close_result = close(server_watcher->socket_fd);
        // ソケットが閉じれなかったら
        if (close_result < 0)
        {
            snprintf(log_str, MAX_LOG_LENGTH, "%s(): close(fd=%d): Cannot socket close? errno=%d (%s)\n", __func__, server_watcher->socket_fd, errno, strerror(errno));
            logging(LOGLEVEL_ERROR, log_str);
            return -1;
        }
        // 該当ソケットのプロトコルファミリーがPF_UNIX(=UNIXドメインソケットなら
        if (server_watcher->socket_address.sa_un.sun_family == PF_UNIX)
        {
            // bindしたUNIXドメインソケットアドレスを削除
            close_result = unlink(server_watcher->socket_address.sa_un.sun_path);
            // UNIXドメインソケットアドレスが削除できなかったら
            if (close_result < 0)
            {
                snprintf(log_str, MAX_LOG_LENGTH, "%s(): unlink(%s): Cannot unlink? errno=%d (%s)\n", __func__, server_watcher->socket_address.sa_un.sun_path, errno, strerror(errno));
                logging(LOGLEVEL_ERROR, log_str);
                return -1;
            }
            snprintf(log_str, MAX_LOG_LENGTH, "%s(): unlink(%s): OK.\n", __func__, server_watcher->socket_address.sa_un.sun_path);
            logging(LOGLEVEL_DEBUG, log_str);
        }
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): close(fd=%d): OK.\n", __func__, server_watcher->socket_fd);
        logging(LOGLEVEL_INFO, log_str);
    }
    // サーバー用テールキューをすべて削除
    while (!TAILQ_EMPTY(&EVS_server_tailq))
    {
        server_watcher = TAILQ_FIRST(&EVS_server_tailq);
        TAILQ_REMOVE(&EVS_server_tailq, server_watcher, entries);
        free(server_watcher);
    }
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): TAILQ_REMOVE(EVS_server_tailq): OK.\n", __func__);
    logging(LOGLEVEL_DEBUG, log_str);

    // --------------------------------
    // ポート別クローズ処理
    // --------------------------------
    // ポート用テールキューをすべて削除
    while (!TAILQ_EMPTY(&EVS_port_tailq))
    {
        listen_port = TAILQ_FIRST(&EVS_port_tailq);
        TAILQ_REMOVE(&EVS_port_tailq, listen_port, entries);
        free(listen_port);
    }
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): TAILQ_REMOVE(EVS_port_tailq): OK.\n", __func__);
    logging(LOGLEVEL_DEBUG, log_str);

    // --------------------------------
    // libev関連終了処理 ※タイマー用に確保したメモリ領域のfree()を忘れずに
    // --------------------------------
    // タイマー用テールキューをすべて削除
    while (!TAILQ_EMPTY(&EVS_timer_tailq))
    {
        timeout_watcher = TAILQ_FIRST(&EVS_timer_tailq);
        TAILQ_REMOVE(&EVS_timer_tailq, timeout_watcher, entries);
        free(timeout_watcher);
    }
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): TAILQ_REMOVE(EVS_timer_tailq): OK.\n", __func__);
    logging(LOGLEVEL_DEBUG, log_str);

    // --------------------------------
    // 各種設定関連終了処理 ※設定用に確保した文字列のメモリ領域のfree()を忘れずに
    // --------------------------------
    if (EVS_config.domain_socketfile != NULL)
    {
        free(EVS_config.domain_socketfile);
    }
    if (EVS_config.ssl_ca_file != NULL)
    {
        free(EVS_config.ssl_ca_file);
    }
    if (EVS_config.ssl_cert_file != NULL)
    {
        free(EVS_config.ssl_cert_file);
    }
    if (EVS_config.ssl_key_file != NULL)
    {
        free(EVS_config.ssl_key_file);
    }

    // --------------------------------
    // PIDファイル処理
    // --------------------------------
    // PIDファイルを削除
    close_result = unlink(EVS_config.pid_file);
    // PIDファイルが削除できなかったら
    if (close_result < 0)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): unlink(%s): Cannot unlink? errno=%d (%s)\n", __func__, EVS_config.pid_file, errno, strerror(errno));
        logging(LOGLEVEL_ERROR, log_str);
        free(EVS_config.pid_file);
        return -1;
    }
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): unlink(%s): OK.\n", __func__, EVS_config.pid_file);
    logging(LOGLEVEL_INFO, log_str);
    free(EVS_config.pid_file);

    // 戻る
    return close_result;
}
