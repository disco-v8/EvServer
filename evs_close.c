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
// クライアント接続終了処理(イベントの停止、クライアントキューからの削除、SSL接続情報開放、ソケットクローズ、クライアント情報開放)
// --------------------------------
void CLOSE_client(struct ev_loop* loop, struct ev_io *watcher, int revents)
{
    int                             socket_result;
    struct EVS_ev_client_t          *this_client = (struct EVS_ev_client_t *)watcher;   // libevから渡されたwatcherポインタを、本来の拡張構造体ポインタとして変換する

    // この接続のイベントを停止する
    ev_io_stop(loop, &this_client->io_watcher);
    printf("INFO  : %s(): ev_io_stop(): OK.\n", __func__);

    // テールキューからこの接続の情報を削除する
    TAILQ_REMOVE(&EVS_client_tailq, this_client, entries);
    printf("INFO  : %s(): TAILQ_REMOVE(EVS_client_tailq): OK.\n", __func__);

    // SSLハンドシェイク前、もしくはSSL接続中なら
    if (this_client->ssl_status != 0)
    {
        // ----------------
        // OpenSSL(SSL_free : SSL接続情報のメモリ領域を開放)
        // ----------------
        SSL_free(this_client->ssl);
        printf("INFO  : %s(fd=%d): SSL_free(): OK.\n", __func__, this_client->socket_fd);
    }

    // この接続のソケットを閉じる
    socket_result = close(this_client->socket_fd);
    // ソケットのクローズ結果がエラーだったら
    if (socket_result < 0)
    {
        printf("ERROR : %s(fd=%d): close(): Cannot socket close? errno=%d (%s)\n", __func__, this_client->socket_fd, errno, strerror(errno));
        return;
    }
    printf("INFO  : %s(fd=%d): close(): OK.\n", __func__, this_client->socket_fd);

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
    struct EVS_ev_client_t          *client_watcher;                    // クライアント別設定用構造体ポインタ
    struct EVS_ev_server_t          *server_watcher;                    // サーバー別設定用構造体ポインタ
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
            printf("ERROR : %s(): close(fd=%d): Cannot socket close? errno=%d (%s)\n", __func__, client_watcher->socket_fd, errno, strerror(errno));
            return -1;
        }
        printf("INFO  : %s(): close(fd=%d): OK.\n", __func__, client_watcher->socket_fd);
    }
    // クライアント用テールキューをすべて削除
    while (EVS_client_tailq.tqh_first != NULL)
    {
        TAILQ_REMOVE(&EVS_client_tailq, EVS_client_tailq.tqh_first, entries);
    }
    printf("INFO  : %s(): TAILQ_REMOVE(EVS_client_tailq): OK.\n", __func__);

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
            printf("ERROR : %s(): close(fd=%d): Cannot socket close? errno=%d (%s)\n", __func__, server_watcher->socket_fd, errno, strerror(errno));
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
                printf("ERROR : %s(): unlink(%s): Cannot unlink? errno=%d (%s)\n", __func__, server_watcher->socket_address.sa_un.sun_path, errno, strerror(errno));
                return -1;
            }
            printf("INFO  : %s(): unlink(%s): OK.\n", __func__, server_watcher->socket_address.sa_un.sun_path);
        }
        printf("INFO  : %s(): close(fd=%d): OK.\n", __func__, server_watcher->socket_fd);
    }
    // サーバー用テールキューをすべて削除
    while (EVS_server_tailq.tqh_first != NULL)
    {
        TAILQ_REMOVE(&EVS_server_tailq, EVS_server_tailq.tqh_first, entries);
    }
    printf("INFO  : %s(): TAILQ_REMOVE(EVS_server_tailq): OK.\n", __func__);

    // --------------------------------
    // ポート別クローズ処理
    // --------------------------------
    // ポート用テールキューをすべて削除
    while (EVS_port_tailq.tqh_first != NULL)
    {
        TAILQ_REMOVE(&EVS_port_tailq, EVS_port_tailq.tqh_first, entries);
    }
    printf("INFO  : %s(): TAILQ_REMOVE(EVS_port_tailq): OK.\n", __func__);

    // --------------------------------
    // libev関連終了処理
    // --------------------------------

    // 戻る
    return close_result;
}
