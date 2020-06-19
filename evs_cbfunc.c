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

// evs_cbfunc.c はコールバック関数だけをまとめたファイルで、evs_init.cがごちゃごちゃするので分離している。
// evs_init.c からincludeされることを想定しているので、evs_main.hなどのヘッダファイルはincludeしていない。

// ----------------------------------------------------------------------
// ヘッダ部分
// ----------------------------------------------------------------------
// --------------------------------
// インクルード宣言
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
// シグナル処理(SIGHUP)のコールバック処理
// --------------------------------
static void CB_sighup(struct ev_loop* loop, struct ev_signal *watcher, int revents)
{
    // イベントにエラーフラグが含まれていたら
    if (EV_ERROR & revents)
    {
        printf("ERROR : %s(): Invalid event!?\n", __func__);
        return;
    }

    printf("INFO  : %s(): Catch SIGHUP!\n", __func__);
////    ev_break(loop, EVBREAK_CANCEL);                                            // わざわざこう書いてもいいけど、書かなくてもループは続けてくれる
}

// --------------------------------
// シグナル処理(SIGINT)のコールバック処理
// --------------------------------
static void CB_sigint(struct ev_loop* loop, struct ev_signal *watcher, int revents)
{
    // イベントにエラーフラグが含まれていたら
    if (EV_ERROR & revents)
    {
        printf("ERROR : %s(): Invalid event!?\n", __func__);
        return;
    }

    printf("INFO  : %s(): Catch SIGINT!\n", __func__);
    ev_break(loop, EVBREAK_ALL);
}

// --------------------------------
// シグナル処理(SIGTERM)のコールバック処理
// --------------------------------
static void CB_sigterm(struct ev_loop* loop, struct ev_signal *watcher, int revents)
{
    // イベントにエラーフラグが含まれていたら
    if (EV_ERROR & revents)
    {
        printf("ERROR : %s(): Invalid event!?\n", __func__);
        return;
    }

    printf("INFO  : %s(): Catch SIGTERM!\n", __func__);
    ev_break(loop, EVBREAK_ALL);
}

// ----------------
// タイムアウトのコールバック処理
// ----------------
static void CB_timeout(struct ev_loop* loop, struct ev_timer *watcher, int revents)
{
    struct EVS_timer_t          *this_timeout;
    struct EVS_ev_client_t      *this_client;
    ev_tstamp                   after_time;                             // タイマーの経過時間(最終アクティブ日時 - 現在日時 + タイムアウト)
    
    printf("INFO  : %s(): Timeout check start!\n", __func__);
    // タイマー用テールキューに登録されているタイマーを確認
    TAILQ_FOREACH (this_timeout, &EVS_timer_tailq, entries)
    {
        // タイマー用キューの対象オブジェクトポインタを、タイマー監視対象の拡張構造体ポインタに変換する
        this_client = (struct EVS_ev_client_t *)this_timeout->target;
        printf("INFO  : %s(fd=%d): last_activity=%f, timeout=%f, ev_now=%f\n", __func__, this_client->socket_fd, this_client->last_activity, this_timeout->timeout, ev_now(loop));
        // 該当タイマーの経過時間を算出する
        after_time = this_client->last_activity - ev_now(loop) + this_timeout->timeout;
        // 該当タイマーがすでにタイムアウトしていたら
        if (after_time < 0)
        {
            printf("INFO  : %s(fd=%d): Timeout!!!\n", __func__, this_client->socket_fd);
            // テールキューからこの接続の情報を削除する
            TAILQ_REMOVE(&EVS_timer_tailq, this_timeout, entries);
            printf("INFO  : %s(fd=%d): TAILQ_REMOVE(EVS_timer_tailq): OK.\n", __func__, this_client->socket_fd);
            // ----------------
            // クライアント接続終了処理(イベントの停止、クライアントキューからの削除、SSL接続情報開放、ソケットクローズ、クライアント情報開放)
            // ----------------
            CLOSE_client(loop, (struct ev_io *)this_client, revents);
        }
        else
        {
            printf("INFO  : %s(fd=%d): Not Timeout!?\n", __func__, this_client->socket_fd);
        }
    }
    // ----------------
    // タイマーオブジェクトに対して、タイムアウト確認間隔(timeout_checkintval秒)、そして繰り返し回数(0回)を設定する(つまり次のタイマーを設定している)
    // ----------------
    ev_timer_set(&timeout_watcher, EVS_config.timeout_checkintval, 0);              // ※&timeout_watcherの代わりに&watcherとしても同じこと
    ev_timer_start(loop, &timeout_watcher);
}

// --------------------------------
// ソケット受信(recv : ソケットに対してメッセージが送られてきたときに発生するイベント)のコールバック処理
// --------------------------------
static void CB_recv(struct ev_loop* loop, struct ev_io *watcher, int revents)
{
    int                             socket_result;

    struct EVS_ev_client_t          *other_client;                                      // ev_io＋ソケットファイルディスクリプタ＋次のTAILQ構造体への接続とした拡張構造体
    struct EVS_ev_client_t          *this_client = (struct EVS_ev_client_t *)watcher;   // libevから渡されたwatcherポインタを、本来の拡張構造体ポインタとして変換する

    char msg_buf[MAX_MESSAGE_LENGTH];
    ssize_t msg_len = 0;

    // イベントにエラーフラグが含まれていたら
    if (EV_ERROR & revents)
    {
        printf("ERROR : %s(): Invalid event!?\n", __func__);
        return;
    }

    printf("INFO  : %s(fd=%d): OK. ssl_status=%d\n", __func__, this_client->socket_fd, this_client->ssl_status);

    // ----------------
    // 無通信タイムアウトチェックをする(=1:有効)なら
    // ----------------
    if (EVS_config.nocommunication_check == 1)
    {
        ev_now_update(loop);                                            // イベントループの日時を現在の日時に更新
        this_client->last_activity = ev_now(loop);                      // 最終アクティブ日時(監視対象が最後にアクティブとなった=タイマー更新した日時)を設定する
    }

    // ----------------
    // 非SSL通信(=0)なら
    // ----------------
    if (this_client->ssl_status == 0)
    {
        // ----------------
        // ソケット受信(recv : ソケットのファイルディスクリプタから、msg_bufに最大sizeof(msg_buf)だけメッセージを受信する。(ノンブロッキングにするなら0ではなくてMSG_DONTWAIT)
        // ----------------
        socket_result = recv(this_client->socket_fd, (void *)msg_buf, sizeof(msg_buf), 0);
        // 読み込めたメッセージ量が負(<0)だったら(エラーです)
        if (socket_result < 0)
        {
            printf("ERROR : %s(): Cannot recv message? errno=%d (%s)\n", __func__, errno, strerror(errno));
            return;
        }
    }
    // ----------------
    // SSLハンドシェイク前なら
    // ----------------
    else if (this_client->ssl_status == 1)
    {
        // ----------------
        // OpenSSL(SSL_accept : SSL/TLSハンドシェイクを開始)
        // ----------------
        socket_result = SSL_accept(this_client->ssl);
        printf("INFO  : %s(fd=%d): SSL_accept(): socket_result=%d.\n", __func__, this_client->socket_fd, socket_result);
        // SSL/TLSハンドシェイクの結果コードを取得
        socket_result = SSL_get_error(this_client->ssl, socket_result);
        printf("INFO  : %s(fd=%d): SSL_get_error(): socket_result=%d.\n", __func__, this_client->socket_fd, socket_result);

        // SSL/TLSハンドシェイクの結果コード別処理分岐
        switch (socket_result)
        {
            case SSL_ERROR_NONE : 
                // エラーなし(ハンドシェイク成功)
                // SSL接続中に設定
                this_client->ssl_status = 2;
                printf("INFO  : %s(fd=%d): SSL/TLS handshake OK.\n", __func__, this_client->socket_fd);
                break;
            case SSL_ERROR_SSL :
            case SSL_ERROR_SYSCALL :
                // SSL/TLSハンドシェイクがエラー
                printf("ERROR : %s(fd=%d): Cannot SSL/TLS handshake!? %s\n", __func__, this_client->socket_fd, ERR_reason_error_string(ERR_get_error()));
                // ----------------
                // クライアント接続終了処理(イベントの停止、クライアントキューからの削除、SSL接続情報開放、ソケットクローズ、クライアント情報開放)
                // ----------------
                CLOSE_client(loop, (struct ev_io *)this_client, revents);
                break;
            case SSL_ERROR_WANT_READ :
            case SSL_ERROR_WANT_WRITE :
                // まだハンドシェイクが完了するほどのメッセージが届いていなようなので、次のメッセージ受信イベントを待つ
                // ※たいていはSSLポートに対してSSLではない接続が来た時にこの分岐処理となる
                break;
        }
        // ----------------
        // SSLハンドシェイク処理が終了したら、いったんコールバック関数から抜ける(メッセージが来るのは次のイベント)
        // ----------------
        return;
    }
    // ----------------
    // SSL接続中なら
    // ----------------
    else if (this_client->ssl_status == 2)
    {
        // ----------------
        // OpenSSL(SSL_read : SSLデータ読み込み)
        // ----------------
        socket_result = SSL_read(this_client->ssl, (void *)msg_buf, sizeof(msg_buf));
        // SSLデータ読み込みがエラーだったら
        if (socket_result < 0)
        {
            printf("ERROR : %s(fd=%d): SL_read(): Cannot read decrypted message!?\n", __func__, this_client->socket_fd, ERR_reason_error_string(ERR_get_error()));
            // ----------------
            // クライアント接続終了処理(イベントの停止、クライアントキューからの削除、SSL接続情報開放、ソケットクローズ、クライアント情報開放)
            // ----------------
            CLOSE_client(loop, (struct ev_io *)this_client, revents);
            return;
        }
    }

    // メッセージ長を設定する(メッセージの終端に'\0'(!=NULL)を設定してはっきりとさせておく)
    msg_len = socket_result;
    msg_buf[msg_len] = '\0';

    // 読み込めたメッセージ量が0だったら(切断処理をする)
    if (msg_len == 0)
    {
        // ----------------
        // クライアント接続終了処理(イベントの停止、クライアントキューからの削除、SSL接続情報開放、ソケットクローズ、クライアント情報開放)
        // ----------------
        CLOSE_client(loop, (struct ev_io *)this_client, revents);
        return;
    }

    // 受信したメッセージをとりあえず表示する
    printf("INFO  : %s(fd=%d): message len=%d, %s", __func__, this_client->socket_fd, msg_len, msg_buf);

/*
    // 接続している他のクライアントに対して受信したメッセージを送信する
    TAILQ_FOREACH (other_client, &EVS_client_tailq, entries)
    {
        // その他のクライアントなら(クライアント用テールキューをすべてなめているので、メッセージを送ってきたクライアントには送信しない)
        if (other_client != this_client)
        {
            // ----------------
            // ソケット送信(send : ソケットのファイルディスクリプタに対して、msg_bufからmsg_lenのメッセージを送信する。(ノンブロッキングにするなら0ではなくてMSG_DONTWAIT)
            // ----------------
            socket_result = send(other_client->socket_fd, (void*)msg_buf, msg_len, 0);
            // 送信したバイト数が負(<0)だったら(エラーです)
            if (socket_result < 0)
            {
                printf("ERROR : %s(fd=%d): send(fd=%d): Cannot send message? errno=%d (%s)\n", __func__, this_client->socket_fd, other_client->socket_fd, errno, strerror(errno));
                return;
            }
////            printf("INFO  : %s(fd=%d): send(fd=%d): OK.\n", __func__, this_client->socket_fd, other_client->socket_fd);
        }
    }
*/
}

// --------------------------------
// IPv6ソケットアクセプト(accept : ソケットに対して接続があったときに発生するイベント)のコールバック処理
// --------------------------------
static void CB_accept_ipv6(struct ev_loop* loop, struct EVS_ev_server_t * server_watcher, int revents)
{
    int                             socket_result;
    int                             nonblocking_flag = 1;               // ノンブロッキング対応(0:非対応、1:対応))

    struct sockaddr_in              client_sockaddr_in6;                                        // IPv6ソケットアドレス
    socklen_t                       client_sockaddr_len = sizeof(client_sockaddr_in6);          // IPv6ソケットアドレス構造体のサイズ (バイト単位)
    struct EVS_ev_client_t          *client_watcher;                                            // クライアント別設定用構造体ポインタ
    struct EVS_timer_t              *timeout_watcher;                                           // タイマー別構造体ポインタ

    // ----------------
    // ソケットアクセプト(accept : リッスンポートに接続があった)
    // ----------------
    socket_result = accept(server_watcher->socket_fd, (struct sockaddr *)&client_sockaddr_in6, &client_sockaddr_len);
    // アクセプトしたソケットのディスクリプタがエラーだったら
    if (socket_result < 0)
    {
        // ここでもソケットをクローズをしたほうがいいかな？それともソケットを再設定したほうがいいかな？
        printf("ERROR : %s(fd=%d): Cannot socket accepting? errno=%d (%s)\n", __func__, server_watcher->socket_fd, errno, strerror(errno));
        return;
    }
    printf("INFO  : %s(fd=%d): OK. client fd=%d\n", __func__, server_watcher->socket_fd, socket_result);

    // クライアント別設定用構造体ポインタのメモリ領域を確保
    client_watcher = (struct EVS_ev_client_t *)calloc(1, sizeof(*client_watcher));
    // メモリ領域が確保できなかったら
    if (client_watcher == NULL)
    {
        printf("ERROR : %s(fd=%d): Cannot calloc client_watcher's memory? errno=%d (%s)\n", __func__, server_watcher->socket_fd, errno, strerror(errno));
        return;
    }

    // このソケットでSSL/TLS対応しなければいけない(ssl_support==1)なら
    if (server_watcher->ssl_support == 1)
    {
        printf("INFO  : %s(fd=%d): SSL port.\n", __func__, server_watcher->socket_fd);
        // ----------------
        // OpenSSL(SSL_new : SSL設定情報を元に、SSL接続情報を生成)
        // ----------------
        client_watcher->ssl = SSL_new(EVS_ctx);
        // SSL設定情報を元に、SSL接続情報を生成がエラーだったら
        if (client_watcher->ssl == NULL)
        {
            printf("ERROR : %s(fd=%d): SSL_new(): Cannot get new SSL structure!? %s\n", __func__, server_watcher->socket_fd, ERR_reason_error_string(ERR_get_error()));
            return;
        }
        printf("INFO  : %s(fd=%d): SSL_new(): OK.\n", __func__, server_watcher->socket_fd);

        // ----------------
        // OpenSSL(SSL_set_fd : 接続してきたソケットファイルディスクリプタを、SSL_set_fd()でSSL接続情報に紐づけ)
        // ----------------
        // 接続してきたソケットファイルディスクリプタを、SSL_set_fd()でSSL接続情報に紐づけがエラーだったら
        if (SSL_set_fd(client_watcher->ssl, socket_result) == 0)
        {
            // ここでもソケットをクローズをしたほうがいいかな？それともソケットを再設定したほうがいいかな？
            printf("ERROR : %s(fd=%d): SSL_set_fd(client fd=%d): Cannot SSL socket binding!? %s\n", __func__, server_watcher->socket_fd, socket_result, ERR_reason_error_string(ERR_get_error()));
            return;
        }
        printf("INFO  : %s(fd=%d): SSL_set_fd(client fd=%d): OK.\n", __func__, server_watcher->socket_fd, socket_result);

        // SSLハンドシェイク前(=1)に設定
        client_watcher->ssl_status = 1;
    }
    // 非SSL/TLSソケットなら
    else
    {
        printf("INFO  : %s(fd=%d): Not SSL port.\n", __func__, server_watcher->socket_fd);
        // 非SSL通信(=0)に設定
        client_watcher->ssl_status = 0;
    }

    // ----------------
    // 無通信タイムアウトチェックをする(=1:有効)なら
    // ----------------
    if (EVS_config.nocommunication_check == 1)
    {
        // ----------------
        // タイマー別構造体ポインタに、タイムアウト監視を設定する
        // ----------------
        // タイマー別構造体ポインタのメモリ領域を確保
        timeout_watcher = (struct EVS_timer_t *)calloc(1, sizeof(*timeout_watcher));
        // メモリ領域が確保できなかったら
        if (timeout_watcher == NULL)
        {
            printf("ERROR : %s(): Cannot calloc memory? errno=%d (%s)\n", __func__, errno, strerror(errno));
            return;
        }

        // タイマー別構造体ポインタにクライアント別設定用構造体ポインタを設定
        timeout_watcher->timeout = EVS_config.nocommunication_timeout;      // タイムアウト秒(last_activityのtimeout秒後)を設定する
        timeout_watcher->target = (void *)client_watcher;                   // クライアント別設定用構造体ポインタを設定する
        ev_now_update(loop);                                                // イベントループの日時を現在の日時に更新
        client_watcher->last_activity = ev_now(loop);                       // 最終アクティブ日時(監視対象が最後にアクティブとなった=タイマー更新した日時)を設定する

        // テールキューの最後にこの接続の情報を追加する
        TAILQ_INSERT_TAIL(&EVS_timer_tailq, timeout_watcher, entries);
        printf("INFO  : %s(): TAILQ_INSERT_TAIL(timeout_watcher): OK.\n", __func__);
    }

    // クライアント別設定用構造体ポインタにアクセプトしたソケットの情報を設定
    client_watcher->socket_fd = socket_result;                                                                      // ディスクリプタを設定する
    memcpy((void *)&client_watcher->socket_address.sa_ipv6, (void *)&client_sockaddr_in6, client_sockaddr_len);     // アドレス構造体を設定する

    // ----------------
    // クライアントとの接続ソケットをノンブロッキングモードにする
    // ----------------
    socket_result = ioctl(client_watcher->socket_fd, FIONBIO, &nonblocking_flag);
    // メモリ領域が確保できなかったら
    if (socket_result < 0)
    {
        printf("ERROR : %s(fd=%d): ioctl(): Cannot set Non-Blocking mode!? errno=%d (%s)\n", __func__, client_watcher->socket_fd, errno, strerror(errno));
        return;
    }

    // テールキューの最後にこの接続の情報を追加する
    TAILQ_INSERT_TAIL(&EVS_client_tailq, client_watcher, entries);
    printf("INFO  : %s(): TAILQ_INSERT_TAIL(client fd=%d): OK.\n", __func__, client_watcher->socket_fd);

    // ----------------
    // クライアント別設定用構造体ポインタのI/O監視オブジェクトに対して、コールバック処理とソケットファイルディスクリプタ、そしてイベントのタイプを設定する
    // ----------------
    ev_io_init(&client_watcher->io_watcher, CB_recv, client_watcher->socket_fd, EV_READ);
    ev_io_start(loop, &client_watcher->io_watcher);
    printf("INFO  : %s(): ev_io_init(CB_recv, client fd=%d, EV_READ): OK.\n", __func__, client_watcher->socket_fd);
    printf("INFO  : %s(): ev_io_start(): OK.\n", __func__);
}

// --------------------------------
// IPv4ソケットアクセプト(accept : ソケットに対して接続があったときに発生するイベント)のコールバック処理
// --------------------------------
static void CB_accept_ipv4(struct ev_loop* loop, struct EVS_ev_server_t * server_watcher, int revents)
{
    int                             socket_result;
    int                             nonblocking_flag = 1;               // ノンブロッキング対応(0:非対応、1:対応))

    struct sockaddr_in              client_sockaddr_in;                                     // IPv4ソケットアドレス
    socklen_t                       client_sockaddr_len = sizeof(client_sockaddr_in);       // IPv4ソケットアドレス構造体のサイズ (バイト単位)
    struct EVS_ev_client_t          *client_watcher;                                        // クライアント別設定用構造体ポインタ
    struct EVS_timer_t              *timeout_watcher;                                       // タイマー別構造体ポインタ

    // ----------------
    // ソケットアクセプト(accept : リッスンポートに接続があった)
    // ----------------
    socket_result = accept(server_watcher->socket_fd, (struct sockaddr *)&client_sockaddr_in, &client_sockaddr_len);
    // アクセプトしたソケットのディスクリプタがエラーだったら
    if (socket_result < 0)
    {
        // ここでもソケットをクローズをしたほうがいいかな？それともソケットを再設定したほうがいいかな？
        printf("ERROR : %s(fd=%d): Cannot socket accepting? errno=%d (%s)\n", __func__, server_watcher->socket_fd, errno, strerror(errno));
        return;
    }
    printf("INFO  : %s(fd=%d): OK. client fd=%d\n", __func__, server_watcher->socket_fd, socket_result);

    // クライアント別設定用構造体ポインタのメモリ領域を確保
    client_watcher = (struct EVS_ev_client_t *)calloc(1, sizeof(*client_watcher));
    // メモリ領域が確保できなかったら
    if (client_watcher == NULL)
    {
        printf("ERROR : %s(fd=%d): Cannot calloc client_watcher's memory? errno=%d (%s)\n", __func__, server_watcher->socket_fd, errno, strerror(errno));
        return;
    }

    // このソケットでSSL/TLS対応しなければいけない(ssl_support==1)なら
    if (server_watcher->ssl_support == 1)
    {
        printf("INFO  : %s(fd=%d): SSL port.\n", __func__, server_watcher->socket_fd);
        // ----------------
        // OpenSSL(SSL_new : SSL設定情報を元に、SSL接続情報を生成)
        // ----------------
        client_watcher->ssl = SSL_new(EVS_ctx);
        // SSL設定情報を元に、SSL接続情報を生成がエラーだったら
        if (client_watcher->ssl == NULL)
        {
            printf("ERROR : %s(fd=%d): SSL_new(): Cannot get new SSL structure!? %s\n", __func__, server_watcher->socket_fd, ERR_reason_error_string(ERR_get_error()));
            return;
        }
        printf("INFO  : %s(fd=%d): SSL_new(): OK.\n", __func__, server_watcher->socket_fd);

        // ----------------
        // OpenSSL(SSL_set_fd : 接続してきたソケットファイルディスクリプタを、SSL_set_fd()でSSL接続情報に紐づけ)
        // ----------------
        // 接続してきたソケットファイルディスクリプタを、SSL_set_fd()でSSL接続情報に紐づけがエラーだったら
        if (SSL_set_fd(client_watcher->ssl, socket_result) == 0)
        {
            // ここでもソケットをクローズをしたほうがいいかな？それともソケットを再設定したほうがいいかな？
            printf("ERROR : %s(fd=%d): SSL_set_fd(client fd=%d): Cannot SSL socket binding!? %s\n", __func__, server_watcher->socket_fd, socket_result, ERR_reason_error_string(ERR_get_error()));
            return;
        }
        printf("INFO  : %s(fd=%d): SSL_set_fd(client fd=%d): OK.\n", __func__, server_watcher->socket_fd, socket_result);

        // SSLハンドシェイク前(=1)に設定
        client_watcher->ssl_status = 1;
    }
    // 非SSL/TLSソケットなら
    else
    {
        printf("INFO  : %s(fd=%d): Not SSL port.\n", __func__, server_watcher->socket_fd);
        // 非SSL通信(=0)に設定
        client_watcher->ssl_status = 0;
    }

    // ----------------
    // 無通信タイムアウトチェックをする(=1:有効)なら
    // ----------------
    if (EVS_config.nocommunication_check == 1)
    {
        // ----------------
        // タイマー別構造体ポインタに、タイムアウト監視を設定する
        // ----------------
        // タイマー別構造体ポインタのメモリ領域を確保
        timeout_watcher = (struct EVS_timer_t *)calloc(1, sizeof(*timeout_watcher));
        // メモリ領域が確保できなかったら
        if (timeout_watcher == NULL)
        {
            printf("ERROR : %s(): Cannot calloc memory? errno=%d (%s)\n", __func__, errno, strerror(errno));
            return;
        }

        // タイマー別構造体ポインタにクライアント別設定用構造体ポインタを設定
        timeout_watcher->timeout = EVS_config.nocommunication_timeout;      // タイムアウト秒(last_activityのtimeout秒後)を設定する
        timeout_watcher->target = (void *)client_watcher;                   // クライアント別設定用構造体ポインタを設定する
        ev_now_update(loop);                                                // イベントループの日時を現在の日時に更新
        client_watcher->last_activity = ev_now(loop);                       // 最終アクティブ日時(監視対象が最後にアクティブとなった=タイマー更新した日時)を設定する

        // テールキューの最後にこの接続の情報を追加する
        TAILQ_INSERT_TAIL(&EVS_timer_tailq, timeout_watcher, entries);
        printf("INFO  : %s(): TAILQ_INSERT_TAIL(timeout_watcher): OK.\n", __func__);
    }

    // クライアント別設定用構造体ポインタにアクセプトしたソケットの情報を設定
    client_watcher->socket_fd = socket_result;                                                                      // ディスクリプタを設定する
    memcpy((void *)&client_watcher->socket_address.sa_ipv4, (void *)&client_sockaddr_in, client_sockaddr_len);      // アドレス構造体を設定する

    // ----------------
    // クライアントとの接続ソケットをノンブロッキングモードにする
    // ----------------
    socket_result = ioctl(client_watcher->socket_fd, FIONBIO, &nonblocking_flag);
    // メモリ領域が確保できなかったら
    if (socket_result < 0)
    {
        printf("ERROR : %s(fd=%d): ioctl(): Cannot set Non-Blocking mode!? errno=%d (%s)\n", __func__, client_watcher->socket_fd, errno, strerror(errno));
        return;
    }

    // テールキューの最後にこの接続の情報を追加する
    TAILQ_INSERT_TAIL(&EVS_client_tailq, client_watcher, entries);
    printf("INFO  : %s(): TAILQ_INSERT_TAIL(client fd=%d): OK.\n", __func__, client_watcher->socket_fd);

    // ----------------
    // クライアント別設定用構造体ポインタのI/O監視オブジェクトに対して、コールバック処理とソケットファイルディスクリプタ、そしてイベントのタイプを設定する
    // ----------------
    ev_io_init(&client_watcher->io_watcher, CB_recv, client_watcher->socket_fd, EV_READ);
    ev_io_start(loop, &client_watcher->io_watcher);
    printf("INFO  : %s(): ev_io_init(CB_recv, client fd=%d, EV_READ): OK.\n", __func__, client_watcher->socket_fd);
    printf("INFO  : %s(): ev_io_start(): OK.\n", __func__);
}

// --------------------------------
// UNIXドメインソケットアクセプト(accept : ソケットに対して接続があったときに発生するイベント)のコールバック処理
// --------------------------------
static void CB_accept_unix(struct ev_loop* loop, struct EVS_ev_server_t * server_watcher, int revents)
{
    int                             socket_result;

    struct sockaddr_un              client_sockaddr_un;                                     // UNIXドメインソケットアドレス
    socklen_t                       client_sockaddr_len = sizeof(client_sockaddr_un);       // UNIXドメインソケットアドレス構造体のサイズ (バイト単位)
    struct EVS_ev_client_t          *client_watcher;                                        // クライアント別設定用構造体ポインタ
    struct EVS_timer_t              *timeout_watcher;                                       // タイマー別構造体ポインタ

    // ----------------
    // ソケットアクセプト(accept : リッスンポートに接続があった)
    // ----------------
    socket_result = accept(server_watcher->socket_fd, (struct sockaddr *)&client_sockaddr_un, &client_sockaddr_len);
    // アクセプトしたソケットのディスクリプタがエラーだったら
    if (socket_result < 0)
    {
        // ここでもソケットクローズをしたほうがいいかな？
        printf("ERROR : %s(fd=%d): Cannot socket accepting? errno=%d (%s)\n", __func__, server_watcher->socket_fd, errno, strerror(errno));
        return;
    }
    printf("INFO  : %s(fd=%d): OK. client fd=%d\n", __func__, server_watcher->socket_fd, socket_result);

    // クライアント別設定用構造体ポインタのメモリ領域を確保
    client_watcher = (struct EVS_ev_client_t *)calloc(1, sizeof(*client_watcher));
    // メモリ領域が確保できなかったら
    if (client_watcher == NULL)
    {
        printf("ERROR : %s(fd=%d): Cannot calloc client_watcher's memory? errno=%d (%s)\n", __func__, server_watcher->socket_fd, errno, strerror(errno));
        return;
    }

    // ----------------
    // 無通信タイムアウトチェックをする(=1:有効)なら
    // ----------------
    if (EVS_config.nocommunication_check == 1)
    {
        // ----------------
        // タイマー別構造体ポインタに、タイムアウト監視を設定する
        // ----------------
        // タイマー別構造体ポインタのメモリ領域を確保
        timeout_watcher = (struct EVS_timer_t *)calloc(1, sizeof(*timeout_watcher));
        // メモリ領域が確保できなかったら
        if (timeout_watcher == NULL)
        {
            printf("ERROR : %s(): Cannot calloc memory? errno=%d (%s)\n", __func__, errno, strerror(errno));
            return;
        }

        // タイマー別構造体ポインタにクライアント別設定用構造体ポインタを設定
        timeout_watcher->timeout = EVS_config.nocommunication_timeout;      // タイムアウト秒(last_activityのtimeout秒後)を設定する
        timeout_watcher->target = (void *)client_watcher;                   // クライアント別設定用構造体ポインタを設定する
        ev_now_update(loop);                                                // イベントループの日時を現在の日時に更新
        client_watcher->last_activity = ev_now(loop);                       // 最終アクティブ日時(監視対象が最後にアクティブとなった=タイマー更新した日時)を設定する

        // テールキューの最後にこの接続の情報を追加する
        TAILQ_INSERT_TAIL(&EVS_timer_tailq, timeout_watcher, entries);
        printf("INFO  : %s(): TAILQ_INSERT_TAIL(timeout_watcher): OK.\n", __func__);
    }

    // クライアント別設定用構造体ポインタにアクセプトしたソケットの情報を設定
    client_watcher->socket_fd = socket_result;                                                                  // ディスクリプタを設定する
    memcpy((void *)&client_watcher->socket_address.sa_un, (void *)&client_sockaddr_un, client_sockaddr_len);    // アドレス構造体を設定する

    // テールキューの最後にこの接続の情報を追加する
    TAILQ_INSERT_TAIL(&EVS_client_tailq, client_watcher, entries);
    printf("INFO  : %s(): TAILQ_INSERT_TAIL(client fd=%d): OK.\n", __func__, client_watcher->socket_fd);

    // ----------------
    // クライアント別設定用構造体ポインタのI/O監視オブジェクトに対して、コールバック処理とソケットファイルディスクリプタ、そしてイベントのタイプを設定する
    // ----------------
    ev_io_init(&client_watcher->io_watcher, CB_recv, client_watcher->socket_fd, EV_READ);
    ev_io_start(loop, &client_watcher->io_watcher);
    printf("INFO  : %s(): ev_io_init(CB_recv, client fd=%d, EV_READ): OK.\n", __func__, client_watcher->socket_fd);
    printf("INFO  : %s(): ev_io_start(): OK.\n", __func__);
}

// --------------------------------
// ソケットアクセプト(accept : ソケットに対して接続があったときに発生するイベント)のコールバック処理
// --------------------------------
static void CB_accept(struct ev_loop* loop, struct ev_io *watcher, int revents)
{
    struct EVS_ev_server_t    *server_watcher = (struct EVS_ev_server_t *)watcher;          // サーバー別設定用構造体ポインタ

    // イベントにエラーフラグが含まれていたら
    if (EV_ERROR & revents)
    {
        printf("ERROR : %s(): Invalid event!?\n", __func__);
        return;
    }

    // --------------------------------
    // プロトコルファミリー別に各種初期化処理
    // --------------------------------
    // 該当ソケットのプロトコルファミリーがPF_UNIX(=UNIXドメインソケットなら
    if (server_watcher->socket_address.sa_un.sun_family == PF_UNIX)
    {
        // ----------------
        // UNIXドメインソケットの初期化
        // ----------------
        printf("INFO  : %s(): CB_accept_unix(): Go!\n", __func__);              // 呼ぶ前にログを出力
        CB_accept_unix(loop, server_watcher, revents);                          // UNIXドメインソケットのアクセプト処理
    }
    // 該当ソケットのプロトコルファミリーがPF_INET(=IPv4でのアクセス)なら
    else if (server_watcher->socket_address.sa_ipv4.sin_family == PF_INET)
    {
        // ----------------
        // IPv4ソケットの初期化
        // ----------------
        printf("INFO  : %s(): CB_accept_ipv4(): Go!\n", __func__);              // 呼ぶ前にログを出力
        CB_accept_ipv4(loop, server_watcher, revents);                          // IPv4ソケットのアクセプト処理
    } 
    // 該当ソケットのプロトコルファミリーがPF_INET6(=IPv6でのアクセス)なら
    else if (server_watcher->socket_address.sa_ipv6.sin6_family == PF_INET6)
    {
        // ----------------
        // IPv6ソケットの初期化
        // ----------------
        printf("INFO  : %s(): CB_accept_ipv6(): Go!\n", __func__);              // 呼ぶ前にログを出力
        CB_accept_ipv6(loop, server_watcher, revents);                          // IPv6ソケットのアクセプト処理
    }
    else
    {
        printf("ERROR : %s(): Cannot support protocol family!? 2\n", __func__);
    }

    // 戻る
    return;
}