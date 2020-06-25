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
    char                            log_str[MAX_LOG_LENGTH];

    // イベントにエラーフラグが含まれていたら
    if (EV_ERROR & revents)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): Invalid event!?\n", __func__);
        logging(LOGLEVEL_ERROR, log_str);
        return;
    }

    snprintf(log_str, MAX_LOG_LENGTH, "%s(): Catch SIGHUP!\n", __func__);
    logging(LOGLEVEL_INFO, log_str);

////    ev_break(loop, EVBREAK_CANCEL);                                            // わざわざこう書いてもいいけど、書かなくてもループは続けてくれる
}

// --------------------------------
// シグナル処理(SIGINT)のコールバック処理
// --------------------------------
static void CB_sigint(struct ev_loop* loop, struct ev_signal *watcher, int revents)
{
    char                            log_str[MAX_LOG_LENGTH];

    // イベントにエラーフラグが含まれていたら
    if (EV_ERROR & revents)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): Invalid event!?\n", __func__);
        logging(LOGLEVEL_ERROR, log_str);
        return;
    }

    snprintf(log_str, MAX_LOG_LENGTH, "%s(): Catch SIGINT!\n", __func__);
    logging(LOGLEVEL_INFO, log_str);

    ev_break(loop, EVBREAK_ALL);
}

// --------------------------------
// シグナル処理(SIGTERM)のコールバック処理
// --------------------------------
static void CB_sigterm(struct ev_loop* loop, struct ev_signal *watcher, int revents)
{
    char                            log_str[MAX_LOG_LENGTH];

    // イベントにエラーフラグが含まれていたら
    if (EV_ERROR & revents)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): Invalid event!?\n", __func__);
        logging(LOGLEVEL_ERROR, log_str);
        return;
    }

    snprintf(log_str, MAX_LOG_LENGTH, "%s(): Catch SIGTERM!\n", __func__);
    logging(LOGLEVEL_INFO, log_str);

    ev_break(loop, EVBREAK_ALL);
}

// ----------------
// タイムアウトのコールバック処理
// ----------------
static void CB_timeout(struct ev_loop* loop, struct ev_timer *watcher, int revents)
{
    struct EVS_timer_t              *this_timeout;
    struct EVS_ev_client_t          *this_client;
    ev_tstamp                       after_time;                         // タイマーの経過時間(最終アクティブ日時 - 現在日時 + タイムアウト)
    char                            log_str[MAX_LOG_LENGTH];
    
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): Timeout check start!\n", __func__);
    logging(LOGLEVEL_INFO, log_str);
    // タイマー用テールキューに登録されているタイマーを確認
    TAILQ_FOREACH (this_timeout, &EVS_timer_tailq, entries)
    {
        // タイマー用キューのクライアント接続構造体のポインタを変換する
        this_client = (struct EVS_ev_client_t *)this_timeout->client_target;
        snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): last_activity=%f, timeout=%f, ev_now=%f\n", __func__, this_client->socket_fd, this_client->last_activity, this_timeout->timeout, ev_now(loop));
        logging(LOGLEVEL_INFO, log_str);
        // 該当タイマーの経過時間を算出する
        after_time = this_client->last_activity - ev_now(loop) + this_timeout->timeout;
        // 該当タイマーがすでにタイムアウトしていたら
        if (after_time < 0)
        {
            snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): Timeout!!!\n", __func__, this_client->socket_fd);
            logging(LOGLEVEL_WARN, log_str);
            // テールキューからこの接続の情報を削除する
            TAILQ_REMOVE(&EVS_timer_tailq, this_timeout, entries);
            snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): TAILQ_REMOVE(EVS_timer_tailq): OK.\n", __func__, this_client->socket_fd);
            logging(LOGLEVEL_WARN, log_str);
            // ----------------
            // クライアント接続終了処理(イベントの停止、クライアントキューからの削除、SSL接続情報開放、ソケットクローズ、クライアント情報開放)
            // ----------------
            CLOSE_client(loop, (struct ev_io *)this_client, revents);
        }
        else
        {
            snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): Not Timeout!?\n", __func__, this_client->socket_fd);
            logging(LOGLEVEL_INFO, log_str);
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

    char                            msg_buf[MAX_MESSAGE_LENGTH];
    ssize_t                         msg_len = 0;

    char                            log_str[MAX_LOG_LENGTH];

    // イベントにエラーフラグが含まれていたら
    if (EV_ERROR & revents)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): Invalid event!?\n", __func__);
        logging(LOGLEVEL_ERROR, log_str);
        return;
    }

    snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): OK. ssl_status=%d\n", __func__, this_client->socket_fd, this_client->ssl_status);
    logging(LOGLEVEL_INFO, log_str);

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
            snprintf(log_str, MAX_LOG_LENGTH, "%s(): Cannot recv message? errno=%d (%s)\n", __func__, errno, strerror(errno));
            logging(LOGLEVEL_ERROR, log_str);
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
        snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): SSL_accept(): socket_result=%d.\n", __func__, this_client->socket_fd, socket_result);
        logging(LOGLEVEL_INFO, log_str);
        // SSL/TLSハンドシェイクの結果コードを取得
        socket_result = SSL_get_error(this_client->ssl, socket_result);
        snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): SSL_get_error(): socket_result=%d.\n", __func__, this_client->socket_fd, socket_result);
        logging(LOGLEVEL_INFO, log_str);

        // SSL/TLSハンドシェイクの結果コード別処理分岐
        switch (socket_result)
        {
            case SSL_ERROR_NONE : 
                // エラーなし(ハンドシェイク成功)
                // SSL接続中に設定
                this_client->ssl_status = 2;
                snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): SSL/TLS handshake OK.\n", __func__, this_client->socket_fd);
                logging(LOGLEVEL_INFO, log_str);
                break;
            case SSL_ERROR_SSL :
            case SSL_ERROR_SYSCALL :
                // SSL/TLSハンドシェイクがエラー
                snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): Cannot SSL/TLS handshake!? %s\n", __func__, this_client->socket_fd, ERR_reason_error_string(ERR_get_error()));
                logging(LOGLEVEL_ERROR, log_str);
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
            snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): SL_read(): Cannot read decrypted message!?\n", __func__, this_client->socket_fd, ERR_reason_error_string(ERR_get_error()));
            logging(LOGLEVEL_ERROR, log_str);
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

    // --------------------------------
    // API関連
    // --------------------------------
    // API開始処理(クライアント別処理分岐、スレッド生成など)
    socket_result = API_start(this_client, msg_buf, msg_len);
    // APIの処理結果がエラー(!=0)だったら(切断処理をする)
    if (socket_result != 0)
    {
        // ----------------
        // クライアント接続終了処理(イベントの停止、クライアントキューからの削除、SSL接続情報開放、ソケットクローズ、クライアント情報開放)
        // ----------------
        CLOSE_client(loop, (struct ev_io *)this_client, revents);
        return;
    }
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

    char                            log_str[MAX_LOG_LENGTH];

    // ----------------
    // ソケットアクセプト(accept : リッスンポートに接続があった)
    // ----------------
    socket_result = accept(server_watcher->socket_fd, (struct sockaddr *)&client_sockaddr_in6, &client_sockaddr_len);
    // アクセプトしたソケットのディスクリプタがエラーだったら
    if (socket_result < 0)
    {
        // ここでもソケットをクローズをしたほうがいいかな？それともソケットを再設定したほうがいいかな？
        snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): Cannot socket accepting? errno=%d (%s)\n", __func__, server_watcher->socket_fd, errno, strerror(errno));
        logging(LOGLEVEL_ERROR, log_str);
        return;
    }
    snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): OK. client fd=%d\n", __func__, server_watcher->socket_fd, socket_result);
    logging(LOGLEVEL_INFO, log_str);

    // クライアント別設定用構造体ポインタのメモリ領域を確保
    client_watcher = (struct EVS_ev_client_t *)calloc(1, sizeof(*client_watcher));
    // メモリ領域が確保できなかったら
    if (client_watcher == NULL)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): Cannot calloc client_watcher's memory? errno=%d (%s)\n", __func__, server_watcher->socket_fd, errno, strerror(errno));
        logging(LOGLEVEL_ERROR, log_str);
        return;
    }

    // このソケットでSSL/TLS対応しなければいけない(ssl_support==1)なら
    if (server_watcher->ssl_support == 1)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): SSL port.\n", __func__, server_watcher->socket_fd);
        logging(LOGLEVEL_INFO, log_str);
        // ----------------
        // OpenSSL(SSL_new : SSL設定情報を元に、SSL接続情報を生成)
        // ----------------
        client_watcher->ssl = SSL_new(EVS_ctx);
        // SSL設定情報を元に、SSL接続情報を生成がエラーだったら
        if (client_watcher->ssl == NULL)
        {
            snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): SSL_new(): Cannot get new SSL structure!? %s\n", __func__, server_watcher->socket_fd, ERR_reason_error_string(ERR_get_error()));
            logging(LOGLEVEL_ERROR, log_str);
            free(client_watcher);
            return;
        }
        snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): SSL_new(): OK.\n", __func__, server_watcher->socket_fd);
        logging(LOGLEVEL_INFO, log_str);

        // ----------------
        // OpenSSL(SSL_set_fd : 接続してきたソケットファイルディスクリプタを、SSL_set_fd()でSSL接続情報に紐づけ)
        // ----------------
        // 接続してきたソケットファイルディスクリプタを、SSL_set_fd()でSSL接続情報に紐づけがエラーだったら
        if (SSL_set_fd(client_watcher->ssl, socket_result) == 0)
        {
            // ここでもソケットをクローズをしたほうがいいかな？それともソケットを再設定したほうがいいかな？
            snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): SSL_set_fd(client fd=%d): Cannot SSL socket binding!? %s\n", __func__, server_watcher->socket_fd, socket_result, ERR_reason_error_string(ERR_get_error()));
            logging(LOGLEVEL_ERROR, log_str);
            free(client_watcher);
            return;
        }
        snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): SSL_set_fd(client fd=%d): OK.\n", __func__, server_watcher->socket_fd, socket_result);
        logging(LOGLEVEL_INFO, log_str);

        // SSLハンドシェイク前(=1)に設定
        client_watcher->ssl_status = 1;
    }
    // 非SSL/TLSソケットなら
    else
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): Not SSL port.\n", __func__, server_watcher->socket_fd);
        logging(LOGLEVEL_INFO, log_str);
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
            snprintf(log_str, MAX_LOG_LENGTH, "%s(): Cannot calloc memory? errno=%d (%s)\n", __func__, errno, strerror(errno));
            logging(LOGLEVEL_ERROR, log_str);
            free(client_watcher);
            return;
        }

        // タイマー別構造体ポインタにクライアント別設定用構造体ポインタを設定
        timeout_watcher->timeout = EVS_config.nocommunication_timeout;      // タイムアウト秒(last_activityのtimeout秒後)を設定する
        timeout_watcher->client_target = (void *)client_watcher;            // クライアント別設定用構造体ポインタを設定する
        ev_now_update(loop);                                                // イベントループの日時を現在の日時に更新
        client_watcher->last_activity = ev_now(loop);                       // 最終アクティブ日時(監視対象が最後にアクティブとなった=タイマー更新した日時)を設定する
        client_watcher->timeout_target = (void *)timeout_watcher;           // クライアント接続を切断するときに、該当接続のタイマーオブジェクトを削除するために設定しておく

        // テールキューの最後にこの接続の情報を追加する
        TAILQ_INSERT_TAIL(&EVS_timer_tailq, timeout_watcher, entries);
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): TAILQ_INSERT_TAIL(timeout_watcher): OK.\n", __func__);
        logging(LOGLEVEL_INFO, log_str);
    }

    // クライアント別設定用構造体ポインタにアクセプトしたソケットの情報を設定
    client_watcher->socket_fd = socket_result;                                                                      // ディスクリプタを設定する
    getpeername(client_watcher->socket_fd, (struct sockaddr *)&client_sockaddr_in6, &client_sockaddr_len);          // クライアントのアドレス情報を取得しなおす(し直さないと、IPv6でのローカル接続時に惑わされることになるョ)
    memcpy((void *)&client_watcher->socket_address.sa_ipv6, (void *)&client_sockaddr_in6, client_sockaddr_len);     // アドレス構造体を設定する

    // クライアントのアドレスを文字列として格納
    inet_ntop(PF_INET6, (void *)&client_watcher->socket_address.sa_ipv6.sin6_addr, client_watcher->addr_str, sizeof(client_watcher->addr_str));
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): inet_ntop(PF_INET6): Client address=%s\n", __func__, client_watcher->addr_str);
    logging(LOGLEVEL_INFO, log_str);

    // ----------------
    // クライアントとの接続ソケットをノンブロッキングモードにする
    // ----------------
    socket_result = ioctl(client_watcher->socket_fd, FIONBIO, &nonblocking_flag);
    // ノンブロッキングモードにできなかったら
    if (socket_result < 0)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): ioctl(): Cannot set Non-Blocking mode!? errno=%d (%s)\n", __func__, client_watcher->socket_fd, errno, strerror(errno));
        logging(LOGLEVEL_ERROR, log_str);
        free(client_watcher);
        free(timeout_watcher);
        return;
    }

    // テールキューの最後にこの接続の情報を追加する
    TAILQ_INSERT_TAIL(&EVS_client_tailq, client_watcher, entries);
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): TAILQ_INSERT_TAIL(client fd=%d): OK.\n", __func__, client_watcher->socket_fd);
    logging(LOGLEVEL_INFO, log_str);

    // ----------------
    // クライアント別設定用構造体ポインタのI/O監視オブジェクトに対して、コールバック処理とソケットファイルディスクリプタ、そしてイベントのタイプを設定する
    // ----------------
    ev_io_init(&client_watcher->io_watcher, CB_recv, client_watcher->socket_fd, EV_READ);
    ev_io_start(loop, &client_watcher->io_watcher);
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): ev_io_init(CB_recv, client fd=%d, EV_READ): OK.\n", __func__, client_watcher->socket_fd);
    logging(LOGLEVEL_INFO, log_str);
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): ev_io_start(): OK.\n", __func__);
    logging(LOGLEVEL_INFO, log_str);
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

    char                            log_str[MAX_LOG_LENGTH];

    // ----------------
    // ソケットアクセプト(accept : リッスンポートに接続があった)
    // ----------------
    socket_result = accept(server_watcher->socket_fd, (struct sockaddr *)&client_sockaddr_in, &client_sockaddr_len);
    // アクセプトしたソケットのディスクリプタがエラーだったら
    if (socket_result < 0)
    {
        // ここでもソケットをクローズをしたほうがいいかな？それともソケットを再設定したほうがいいかな？
        snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): Cannot socket accepting? errno=%d (%s)\n", __func__, server_watcher->socket_fd, errno, strerror(errno));
        logging(LOGLEVEL_ERROR, log_str);
        return;
    }
    snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): OK. client fd=%d\n", __func__, server_watcher->socket_fd, socket_result);
    logging(LOGLEVEL_INFO, log_str);

    // クライアント別設定用構造体ポインタのメモリ領域を確保
    client_watcher = (struct EVS_ev_client_t *)calloc(1, sizeof(*client_watcher));
    // メモリ領域が確保できなかったら
    if (client_watcher == NULL)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): Cannot calloc client_watcher's memory? errno=%d (%s)\n", __func__, server_watcher->socket_fd, errno, strerror(errno));
        logging(LOGLEVEL_ERROR, log_str);
        return;
    }

    // このソケットでSSL/TLS対応しなければいけない(ssl_support==1)なら
    if (server_watcher->ssl_support == 1)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): SSL port.\n", __func__, server_watcher->socket_fd);
        logging(LOGLEVEL_INFO, log_str);
        // ----------------
        // OpenSSL(SSL_new : SSL設定情報を元に、SSL接続情報を生成)
        // ----------------
        client_watcher->ssl = SSL_new(EVS_ctx);
        // SSL設定情報を元に、SSL接続情報を生成がエラーだったら
        if (client_watcher->ssl == NULL)
        {
            snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): SSL_new(): Cannot get new SSL structure!? %s\n", __func__, server_watcher->socket_fd, ERR_reason_error_string(ERR_get_error()));
            logging(LOGLEVEL_ERROR, log_str);
            free(client_watcher);
            return;
        }
        snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): SSL_new(): OK.\n", __func__, server_watcher->socket_fd);
        logging(LOGLEVEL_INFO, log_str);

        // ----------------
        // OpenSSL(SSL_set_fd : 接続してきたソケットファイルディスクリプタを、SSL_set_fd()でSSL接続情報に紐づけ)
        // ----------------
        // 接続してきたソケットファイルディスクリプタを、SSL_set_fd()でSSL接続情報に紐づけがエラーだったら
        if (SSL_set_fd(client_watcher->ssl, socket_result) == 0)
        {
            // ここでもソケットをクローズをしたほうがいいかな？それともソケットを再設定したほうがいいかな？
            snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): SSL_set_fd(client fd=%d): Cannot SSL socket binding!? %s\n", __func__, server_watcher->socket_fd, socket_result, ERR_reason_error_string(ERR_get_error()));
            logging(LOGLEVEL_ERROR, log_str);
            free(client_watcher);
            return;
        }
        snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): SSL_set_fd(client fd=%d): OK.\n", __func__, server_watcher->socket_fd, socket_result);
        logging(LOGLEVEL_INFO, log_str);

        // SSLハンドシェイク前(=1)に設定
        client_watcher->ssl_status = 1;
    }
    // 非SSL/TLSソケットなら
    else
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): Not SSL port.\n", __func__, server_watcher->socket_fd);
        logging(LOGLEVEL_INFO, log_str);
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
            snprintf(log_str, MAX_LOG_LENGTH, "%s(): Cannot calloc memory? errno=%d (%s)\n", __func__, errno, strerror(errno));
            logging(LOGLEVEL_ERROR, log_str);
            free(client_watcher);
            return;
        }

        // タイマー別構造体ポインタにクライアント別設定用構造体ポインタを設定
        timeout_watcher->timeout = EVS_config.nocommunication_timeout;      // タイムアウト秒(last_activityのtimeout秒後)を設定する
        timeout_watcher->client_target = (void *)client_watcher;            // クライアント別設定用構造体ポインタを設定する
        ev_now_update(loop);                                                // イベントループの日時を現在の日時に更新
        client_watcher->last_activity = ev_now(loop);                       // 最終アクティブ日時(監視対象が最後にアクティブとなった=タイマー更新した日時)を設定する
        client_watcher->timeout_target = (void *)timeout_watcher;           // クライアント接続を切断するときに、該当接続のタイマーオブジェクトを削除するために設定しておく

        // テールキューの最後にこの接続の情報を追加する
        TAILQ_INSERT_TAIL(&EVS_timer_tailq, timeout_watcher, entries);
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): TAILQ_INSERT_TAIL(timeout_watcher): OK.\n", __func__);
        logging(LOGLEVEL_INFO, log_str);
    }

    // クライアント別設定用構造体ポインタにアクセプトしたソケットの情報を設定
    client_watcher->socket_fd = socket_result;                                                                      // ディスクリプタを設定する
    getpeername(client_watcher->socket_fd, (struct sockaddr *)&client_sockaddr_in, &client_sockaddr_len);           // クライアントのアドレス情報を取得しなおす(し直さないと、IPv6でのローカル接続時に惑わされることになるョ。IPv4ではしなくてもいい気もするが…)
    memcpy((void *)&client_watcher->socket_address.sa_ipv4, (void *)&client_sockaddr_in, client_sockaddr_len);      // アドレス構造体を設定する

    // クライアントのアドレスを文字列として格納
    inet_ntop(PF_INET, (void *)&client_watcher->socket_address.sa_ipv4.sin_addr, client_watcher->addr_str, sizeof(client_watcher->addr_str));
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): inet_ntop(PF_INET): Client address=%s\n", __func__, client_watcher->addr_str);
    logging(LOGLEVEL_INFO, log_str);

    // ----------------
    // クライアントとの接続ソケットをノンブロッキングモードにする
    // ----------------
    socket_result = ioctl(client_watcher->socket_fd, FIONBIO, &nonblocking_flag);
    // ノンブロッキングモードにできなかったら
    if (socket_result < 0)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): ioctl(): Cannot set Non-Blocking mode!? errno=%d (%s)\n", __func__, client_watcher->socket_fd, errno, strerror(errno));
        logging(LOGLEVEL_ERROR, log_str);
        free(client_watcher);
        free(timeout_watcher);
        return;
    }

    // テールキューの最後にこの接続の情報を追加する
    TAILQ_INSERT_TAIL(&EVS_client_tailq, client_watcher, entries);
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): TAILQ_INSERT_TAIL(client fd=%d): OK.\n", __func__, client_watcher->socket_fd);
    logging(LOGLEVEL_INFO, log_str);

    // ----------------
    // クライアント別設定用構造体ポインタのI/O監視オブジェクトに対して、コールバック処理とソケットファイルディスクリプタ、そしてイベントのタイプを設定する
    // ----------------
    ev_io_init(&client_watcher->io_watcher, CB_recv, client_watcher->socket_fd, EV_READ);
    ev_io_start(loop, &client_watcher->io_watcher);
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): ev_io_init(CB_recv, client fd=%d, EV_READ): OK.\n", __func__, client_watcher->socket_fd);
    logging(LOGLEVEL_INFO, log_str);
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): ev_io_start(): OK.\n", __func__);
    logging(LOGLEVEL_INFO, log_str);
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

    char                            log_str[MAX_LOG_LENGTH];

    // ----------------
    // ソケットアクセプト(accept : リッスンポートに接続があった)
    // ----------------
    socket_result = accept(server_watcher->socket_fd, (struct sockaddr *)&client_sockaddr_un, &client_sockaddr_len);
    // アクセプトしたソケットのディスクリプタがエラーだったら
    if (socket_result < 0)
    {
        // ここでもソケットクローズをしたほうがいいかな？
        snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): Cannot socket accepting? errno=%d (%s)\n", __func__, server_watcher->socket_fd, errno, strerror(errno));
        logging(LOGLEVEL_ERROR, log_str);
        return;
    }
    snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): OK. client fd=%d\n", __func__, server_watcher->socket_fd, socket_result);
    logging(LOGLEVEL_INFO, log_str);

    // クライアント別設定用構造体ポインタのメモリ領域を確保
    client_watcher = (struct EVS_ev_client_t *)calloc(1, sizeof(*client_watcher));
    // メモリ領域が確保できなかったら
    if (client_watcher == NULL)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(fd=%d): Cannot calloc client_watcher's memory? errno=%d (%s)\n", __func__, server_watcher->socket_fd, errno, strerror(errno));
        logging(LOGLEVEL_ERROR, log_str);
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
            snprintf(log_str, MAX_LOG_LENGTH, "%s(): Cannot calloc memory? errno=%d (%s)\n", __func__, errno, strerror(errno));
            logging(LOGLEVEL_ERROR, log_str);
            free(client_watcher);
            return;
        }

        // タイマー別構造体ポインタにクライアント別設定用構造体ポインタを設定
        timeout_watcher->timeout = EVS_config.nocommunication_timeout;      // タイムアウト秒(last_activityのtimeout秒後)を設定する
        timeout_watcher->client_target = (void *)client_watcher;            // クライアント別設定用構造体ポインタを設定する
        ev_now_update(loop);                                                // イベントループの日時を現在の日時に更新
        client_watcher->last_activity = ev_now(loop);                       // 最終アクティブ日時(監視対象が最後にアクティブとなった=タイマー更新した日時)を設定する
        client_watcher->timeout_target = (void *)timeout_watcher;           // クライアント接続を切断するときに、該当接続のタイマーオブジェクトを削除するために設定しておく

        // テールキューの最後にこの接続の情報を追加する
        TAILQ_INSERT_TAIL(&EVS_timer_tailq, timeout_watcher, entries);
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): TAILQ_INSERT_TAIL(timeout_watcher): OK.\n", __func__);
        logging(LOGLEVEL_INFO, log_str);
    }

    // クライアント別設定用構造体ポインタにアクセプトしたソケットの情報を設定
    client_watcher->socket_fd = socket_result;                                                                  // ディスクリプタを設定する
    memcpy((void *)&client_watcher->socket_address.sa_un, (void *)&client_sockaddr_un, client_sockaddr_len);    // アドレス構造体を設定する

    // アドレスを文字列として格納
    strcpy(client_watcher->addr_str, "UNIX DOMAIN SOCKET");
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): inet_ntop(PF_UNIX): Client address=%s\n", __func__, client_watcher->addr_str);
    logging(LOGLEVEL_INFO, log_str);

    // テールキューの最後にこの接続の情報を追加する
    TAILQ_INSERT_TAIL(&EVS_client_tailq, client_watcher, entries);
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): TAILQ_INSERT_TAIL(client fd=%d): OK.\n", __func__, client_watcher->socket_fd);
    logging(LOGLEVEL_INFO, log_str);

    // ----------------
    // クライアント別設定用構造体ポインタのI/O監視オブジェクトに対して、コールバック処理とソケットファイルディスクリプタ、そしてイベントのタイプを設定する
    // ----------------
    ev_io_init(&client_watcher->io_watcher, CB_recv, client_watcher->socket_fd, EV_READ);
    ev_io_start(loop, &client_watcher->io_watcher);
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): ev_io_init(CB_recv, client fd=%d, EV_READ): OK.\n", __func__, client_watcher->socket_fd);
    logging(LOGLEVEL_INFO, log_str);
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): ev_io_start(): OK.\n", __func__);
    logging(LOGLEVEL_INFO, log_str);
}

// --------------------------------
// ソケットアクセプト(accept : ソケットに対して接続があったときに発生するイベント)のコールバック処理
// --------------------------------
static void CB_accept(struct ev_loop* loop, struct ev_io *watcher, int revents)
{
    struct EVS_ev_server_t          *server_watcher = (struct EVS_ev_server_t *)watcher;          // サーバー別設定用構造体ポインタ
    char                            log_str[MAX_LOG_LENGTH];

    // イベントにエラーフラグが含まれていたら
    if (EV_ERROR & revents)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): Invalid event!?\n", __func__);
        logging(LOGLEVEL_ERROR, log_str);
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
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): CB_accept_unix(): Go!\n", __func__);              // 呼ぶ前にログを出力
        logging(LOGLEVEL_INFO, log_str);
        CB_accept_unix(loop, server_watcher, revents);                          // UNIXドメインソケットのアクセプト処理
    }
    // 該当ソケットのプロトコルファミリーがPF_INET(=IPv4でのアクセス)なら
    else if (server_watcher->socket_address.sa_ipv4.sin_family == PF_INET)
    {
        // ----------------
        // IPv4ソケットの初期化
        // ----------------
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): CB_accept_ipv4(): Go!\n", __func__);              // 呼ぶ前にログを出力
        logging(LOGLEVEL_INFO, log_str);
        CB_accept_ipv4(loop, server_watcher, revents);                          // IPv4ソケットのアクセプト処理
    } 
    // 該当ソケットのプロトコルファミリーがPF_INET6(=IPv6でのアクセス)なら
    else if (server_watcher->socket_address.sa_ipv6.sin6_family == PF_INET6)
    {
        // ----------------
        // IPv6ソケットの初期化
        // ----------------
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): CB_accept_ipv6(): Go!\n", __func__);              // 呼ぶ前にログを出力
        logging(LOGLEVEL_INFO, log_str);
        CB_accept_ipv6(loop, server_watcher, revents);                          // IPv6ソケットのアクセプト処理
    }
    else
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): Cannot support protocol family!? 2\n", __func__);
        logging(LOGLEVEL_ERROR, log_str);
    }

    // 戻る
    return;
}
