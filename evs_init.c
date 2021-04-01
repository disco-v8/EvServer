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
// 定数宣言
// --------------------------------

// --------------------------------
// 型宣言
// --------------------------------

// --------------------------------
// 変数宣言
// --------------------------------
// ----------------
// 設定値関連
// ----------------
struct EVS_config_t             EVS_config;                     // システム設定値

// ----------------
// libev 関連
// ----------------
ev_idle                         idle_socket_watcher;            // アイドルオブジェクト(なにもイベントがないときに呼ばれる、監視対象イベントごとに設定しないといけない)
ev_io                           stdin_watcher;                  // I/O監視オブジェクト
ev_timer                        timeout_watcher;                // タイマーオブジェクト
ev_signal                       signal_watcher_sighup;          // シグナルオブジェクト(シグナルごとにウォッチャーを分けないといけない)
ev_signal                       signal_watcher_sigint;          // シグナルオブジェクト(シグナルごとにウォッチャーを分けないといけない)
ev_signal                       signal_watcher_sigterm;         // シグナルオブジェクト(シグナルごとにウォッチャーを分けないといけない)

struct ev_loop                  *EVS_loop;                      // イベントループ

// ----------------
// ソケット関連
// ----------------
const char                      *pf_name_list[MAX_PF_NUM] = {   // プロトコルファミリーの一部の名称を文字列テーブル化
                                    "PF_UNSPEC",                // 0
                                    "PF_UNIX",                  // 1
                                    "PF_INET",                  // 2
                                    "PF_AX25",                  // 3
                                    "PF_IPX",                   // 4
                                    "PF_APPLETALK",             // 5
                                    "PF_NETROM",                // 6
                                    "PF_BRIDGE",                // 7
                                    "PF_ATMPVC",                // 8
                                    "PF_X25",                   // 9
                                    "PF_INET6",                 // 10
                                    "PF_ROSE",                  // 11
                                    "PF_DECnet",                // 12
                                    "PF_NETBEUI",               // 13
                                    "PF_SECURITY",              // 14
                                    "PF_KEY"                    // 15
};

int                             EVS_connect_num = 0;            // クライアント接続数

// ----------------
// SSL/TLS関連
// ----------------
SSL_CTX                         *EVS_ctx;                       // SSL設定情報

// ----------------
// その他の変数
// ----------------
const char                      *loglevel_list[] = {            // ログレベル文字列テーブル
                                    "DEBUG",
                                    "INFO ",
                                    "TEST ",
                                    "LOG  ",
                                    "WARN ",
                                    "ERROR",
};
int                             EVS_log_fd = 0;                 // ログファイルディスクリプタ

// ----------------
// 以下、個別のAPI関連
// ----------------

// ----------------------------------------------------------------------
// コード部分
// ----------------------------------------------------------------------
// ------------------------------------------------
// 初期化部分
// ------------------------------------------------
// --------------------------------
// 使い方の表示
// --------------------------------

// --------------------------------
// 設定値読み込み関数
// --------------------------------
// evs_config.c は設定ファイルから設定値を読み込む関数だけをまとめたファイルで、evs_init.cがごちゃごちゃするので分離している。
// evs_init.c からincludeされることを想定しているので、evs_main.hなどのヘッダファイルはincludeしていない。
#include "evs_config.c"

// --------------------------------
// コールバック関数
// --------------------------------
// evs_cbfunc.c はコールバック関数だけをまとめたファイルで、evs_init.cがごちゃごちゃするので分離している。
// evs_init.c からincludeされることを想定しているので、evs_main.hなどのヘッダファイルはincludeしていない。
#include "evs_cbfunc.c"

// --------------------------------
// libev関連初期化処理
// --------------------------------
int INIT_libev(void)
{
    char                            log_str[MAX_LOG_LENGTH];

    // ----------------
    // イベントループ生成
    // ----------------
    EVS_loop = ev_loop_new(EVFLAG_AUTO);                                // EVFLAG_AUTO(=0)でイベントループを生成。(ev_default_loopではスレッドセーフではないので)
    // イベントループの生成ができなかったら
    if (!EVS_loop)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): ev_loop_new(EVFLAG_AUTO): Cannot make new loop!?\n", __func__);
        logging(LOGLEVEL_ERROR, log_str);
        return -1;
    }

    // ----------------
    // アイドルイベント初期化処理
    // ----------------
    ev_idle_init(&idle_socket_watcher, CB_idle_socket);
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): ev_signal_init(CB_idle_socket): OK.\n", __func__);
    logging(LOGLEVEL_DEBUG, log_str);
    // ev_idle_start()自体は、accept()とrecv()系イベントで呼び出す

    // ----------------
    // シグナル系イベント初期化処理
    // ----------------
    ev_signal_init(&signal_watcher_sighup, CB_sighup, SIGHUP);
    ev_signal_start(EVS_loop, &signal_watcher_sighup);
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): ev_signal_init(CB_sighup): OK.\n", __func__);
    logging(LOGLEVEL_INFO, log_str);
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): ev_signal_start(): OK.\n", __func__);
    logging(LOGLEVEL_INFO, log_str);

    ev_signal_init(&signal_watcher_sigint, CB_sigint, SIGINT);
    ev_signal_start(EVS_loop, &signal_watcher_sigint);
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): ev_signal_init(CB_sigint): OK.\n", __func__);
    logging(LOGLEVEL_INFO, log_str);
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): ev_signal_start(): OK.\n", __func__);
    logging(LOGLEVEL_INFO, log_str);

    ev_signal_init(&signal_watcher_sigterm, CB_sigterm, SIGTERM);
    ev_signal_start(EVS_loop, &signal_watcher_sigterm);
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): ev_signal_init(CB_sigterm): OK.\n", __func__);
    logging(LOGLEVEL_INFO, log_str);
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): ev_signal_start(): OK.\n", __func__);
    logging(LOGLEVEL_INFO, log_str);

    return 0;
}

// --------------------------------
// OpenSSL関連初期化処理
// --------------------------------
int INIT_openssl(void)
{
    int                             init_result;
    char                            log_str[MAX_LOG_LENGTH];

    // ----------------
    // OpenSSLの各種初期化処理(OpenSSL 1.1.0以降非推奨)
    // ----------------
    // サーバー証明書(PEM)CAファイルがNULLなら
    if (EVS_config.ssl_ca_file == NULL)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): Cannot open SSL/TLS CA file!? %s\n", __func__, EVS_config.ssl_ca_file);
        logging(LOGLEVEL_ERROR, log_str);
        return 0;
    }
    // サーバー証明書(PEM)CERTファイルがNULLなら
    if (EVS_config.ssl_cert_file == NULL)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): Cannot open SSL/TLS CERT file!? %s\n", __func__, EVS_config.ssl_cert_file);
        logging(LOGLEVEL_ERROR, log_str);
        return 0;
    }
    // サーバー証明書(PEM)KEYファイルがNULLなら
    if (EVS_config.ssl_key_file == NULL)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): Cannot open SSL/TLS KEY file!? %s\n", __func__, EVS_config.ssl_key_file);
        logging(LOGLEVEL_ERROR, log_str);
        return 0;
    }

    // ----------------
    // SSL設定情報を作成
    //     OpenSSL 1.1.0以降は初期化関数、OPENSSL_init_ssl()およびOPENSSL_init_crypto()を呼ぶ必要すらなくなったが、証明書ファイルの指定や、細かい制限をSSL_CTX_set_options()等でする必要はある
    //     サーバー用のTLSメソッドを指定、1.1.0以降はTLS_server_method()を指定すること。SSL_CTX_set_options()でいずれにしても許可するプロトコルバージョンを指定すること
    // ----------------
    EVS_ctx = SSL_CTX_new(TLS_server_method());
    if (EVS_ctx == NULL)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): SSL_CTX_new(): Cannot initialize SSL_CTX!? %s\n", __func__, ERR_reason_error_string(ERR_get_error()));
        logging(LOGLEVEL_ERROR, log_str);
        return init_result;
    }
    logging(LOGLEVEL_INFO, "SSL_CTX_new(): OK.\n");
    // SSL設定でTLSv1.2以上しか許可しない(1.1.0以降はSSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION)、でいい)
    SSL_CTX_set_options(EVS_ctx, (SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1));

    // ----------------
    // サーバー証明書関連を設定
    // ----------------
    // サーバー証明書(PEM)CAファイルの設定
    init_result = SSL_CTX_load_verify_locations(EVS_ctx, EVS_config.ssl_ca_file, NULL);
    if (init_result != 1)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): SSL_CTX_load_verify_locations(%s): %s\n", __func__, EVS_config.ssl_ca_file, ERR_reason_error_string(ERR_get_error()));
        logging(LOGLEVEL_ERROR, log_str);
        return 0;
    }
    snprintf(log_str, MAX_LOG_LENGTH, "SSL_CTX_load_verify_locations(%s): OK.\n", EVS_config.ssl_ca_file);
    logging(LOGLEVEL_INFO, log_str);

    // サーバー証明書(PEM)CERTファイルの設定
    init_result = SSL_CTX_use_certificate_file(EVS_ctx, EVS_config.ssl_cert_file, SSL_FILETYPE_PEM);
    if (init_result != 1)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): SSL_CTX_use_certificate_file(%s): %s\n", __func__, EVS_config.ssl_cert_file, ERR_reason_error_string(ERR_get_error()));
       logging(LOGLEVEL_ERROR, log_str);
        return 0;
    }
    snprintf(log_str, MAX_LOG_LENGTH, "SSL_CTX_use_certificate_file(%s): OK.\n", EVS_config.ssl_cert_file);
    logging(LOGLEVEL_INFO, log_str);

    // サーバー証明書(PEM)KEYファイルの設定
    init_result = SSL_CTX_use_PrivateKey_file(EVS_ctx, EVS_config.ssl_key_file, SSL_FILETYPE_PEM);
    if (init_result != 1)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): SSL_CTX_use_PrivateKey_file(%s): %s\n", __func__, EVS_config.ssl_key_file, ERR_reason_error_string(ERR_get_error()));
        logging(LOGLEVEL_ERROR, log_str);
        return 0;
    }
    snprintf(log_str, MAX_LOG_LENGTH, "SSL_CTX_use_PrivateKey_file(%s): OK.\n", EVS_config.ssl_key_file);
    logging(LOGLEVEL_INFO, log_str);

    // CERTとKEYとの整合性をチェック
    init_result = SSL_CTX_check_private_key(EVS_ctx);
    if (init_result != 1)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): SSL_CTX_check_private_key(): %s\n", __func__, ERR_reason_error_string(ERR_get_error()));
        logging(LOGLEVEL_ERROR, log_str);
        return 0;
    }
    snprintf(log_str, MAX_LOG_LENGTH, "SSL_CTX_check_private_key(): OK.\n");
    logging(LOGLEVEL_INFO, log_str);

    return 1;
}

// --------------------------------
// KeepAliveの初期化
// --------------------------------
int INIT_keepalive(struct EVS_ev_server_t * server_watcher)
{
    int                             socket_result;
    char                            log_str[MAX_LOG_LENGTH];

    // ----------------
    // UNIXドメイン以外のソケット(!= PF_UNIX)なら、ソケットのその他のパラメータを設定(KeepAlive、Timeout、バッファサイズ、バッファリングOFF、etc...)
    // /proc/sys/net/ipv4/tcp_keepalive～でシステムのデフォルト値が確認できるが、IPv6についてもIPv4の設定が使われるらしい!?
    // ----------------
    // KeepAlive設定がON(=1)ではないなら
    if (EVS_config.keepalive != 1)
    {
        // 改めてKeepAlive設定をOFF(=0)に設定する
        EVS_config.keepalive = 0;
    }
    // SO_KEEPALIVEを設定 → https://linuxjm.osdn.jp/html/LDP_man-pages/man7/socket.7.html
    socket_result = setsockopt(server_watcher->socket_fd, SOL_SOCKET, SO_KEEPALIVE, &EVS_config.keepalive, sizeof(EVS_config.keepalive));
    // ソケットのオプション設定が出来なかったら
    if (socket_result < 0)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): setsockopt(fd=%d, SOL_SOCKET, SO_KEEPALIVE, keepalive=%d): Cannot set socket option!? errno=%d (%s)\n", __func__, server_watcher->socket_fd, EVS_config.keepalive, errno, strerror(errno));
        logging(LOGLEVEL_ERROR, log_str);
        return -1;
    }
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): setsockopt(socket_fd=%d, SOL_SOCKET, SO_KEEPALIVE, keepalive=%d): OK.\n", __func__, server_watcher->socket_fd, EVS_config.keepalive);
    logging(LOGLEVEL_INFO, log_str);

    // KeepAliveのパラメータは 
    // ・TCP_KEEPIDLE       → /proc/sys/net/ipv6/tcp_keepalive_time     (デフォルト7200秒)
    // ・TCP_KEEPINTVL      → /proc/sys/net/ipv6/tcp_keepalive_intvl    (デフォルト75秒) 
    // ・TCP_KEEPCNT        → /proc/sys/net/ipv6/tcp_keepalive_probes   (デフォルト9回)
    // で確認できる。(デフォルトで切断確認開始まで7200秒(=120分)って長くね？)

    // KeepAlive設定がON(=1)なら
    if (EVS_config.keepalive == 1)
    {
        // TCP_KEEPIDLEを設定 → https://linuxjm.osdn.jp/html/LDP_man-pages/man7/tcp.7.html
        socket_result = setsockopt(server_watcher->socket_fd, IPPROTO_TCP, TCP_KEEPIDLE, &EVS_config.keepalive_idletime, sizeof(EVS_config.keepalive_idletime));
        // ソケットのオプション設定が出来なかったら
        if (socket_result < 0)
        {
            snprintf(log_str, MAX_LOG_LENGTH, "%s(): setsockopt(fd=%d, IPPROTO_TCP, TCP_KEEPIDLE, idletime=%d): Cannot set socket option!? errno=%d (%s)\n", __func__, server_watcher->socket_fd, EVS_config.keepalive_idletime, errno, strerror(errno));
            logging(LOGLEVEL_ERROR, log_str);
            return -1;
        }
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): setsockopt(fd=%d, IPPROTO_TCP, TCP_KEEPIDLE, idletime=%d): OK.\n", __func__, server_watcher->socket_fd, EVS_config.keepalive_idletime);
        logging(LOGLEVEL_INFO, log_str);

        // TCP_KEEPINTVLを設定 → https://linuxjm.osdn.jp/html/LDP_man-pages/man7/tcp.7.html
        socket_result = setsockopt(server_watcher->socket_fd, IPPROTO_TCP, TCP_KEEPINTVL, &EVS_config.keepalive_intval, sizeof(EVS_config.keepalive_intval));
        // ソケットのオプション設定が出来なかったら
        if (socket_result < 0)
        {
            snprintf(log_str, MAX_LOG_LENGTH, "%s(): setsockopt(fd=%d, IPPROTO_TCP, TCP_KEEPINTVL, intval=%d): Cannot set socket option!? errno=%d (%s)\n", __func__, server_watcher->socket_fd, EVS_config.keepalive_intval, errno, strerror(errno));
            logging(LOGLEVEL_ERROR, log_str);
            return -1;
        }
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): setsockopt(fd=%d, IPPROTO_TCP, TCP_KEEPINTVL, intval=%d): OK.\n", __func__, server_watcher->socket_fd, EVS_config.keepalive_intval);
        logging(LOGLEVEL_INFO, log_str);

        // TCP_KEEPCNTを設定 → https://linuxjm.osdn.jp/html/LDP_man-pages/man7/tcp.7.html
        socket_result = setsockopt(server_watcher->socket_fd, IPPROTO_TCP, TCP_KEEPCNT, &EVS_config.keepalive_probes, sizeof(EVS_config.keepalive_probes));
        // ソケットのオプション設定が出来なかったら
        if (socket_result < 0)
        {
            snprintf(log_str, MAX_LOG_LENGTH, "%s(): setsockopt(fd=%d, IPPROTO_TCP, TCP_KEEPCNT, probes=%d): Cannot set socket option!? errno=%d (%s)\n", __func__, server_watcher->socket_fd, EVS_config.keepalive_probes, errno, strerror(errno));
            logging(LOGLEVEL_ERROR, log_str);
            return -1;
        }
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): setsockopt(fd=%d, IPPROTO_TCP, TCP_KEEPCNT, probes=%d): OK.\n", __func__, server_watcher->socket_fd, EVS_config.keepalive_probes);
        logging(LOGLEVEL_INFO, log_str);
    }
    return 0;
}

// --------------------------------
// IPv6ソケットの初期化
// --------------------------------
int INIT_pf_inet6(struct EVS_ev_server_t * server_watcher)
{
    int                             socket_result;
    int                             ipv6only_flag = 1;                  // IPv6対応(0:非対応、1:対応))
    char                            log_str[MAX_LOG_LENGTH];

    // --------------------------------
    // ソケット処理
    // --------------------------------
    // プロトコルファミリーがPF_INET6でなければ
    if (server_watcher->socket_address.sa_ipv6.sin6_family != PF_INET6)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(%d): Cannot support protocol family!?\n", __func__, server_watcher->socket_address.sa_ipv6.sin6_family);
        logging(LOGLEVEL_ERROR, log_str);
        return -1;
    }

    // ----------------
    // ソケット生成(socket : IPv6ソケットでかつストリームで)
    // ----------------
    socket_result = socket(server_watcher->socket_address.sa_ipv6.sin6_family, SOCK_STREAM, 0);
    // ソケット生成が出来なかったら
    if (socket_result < 0)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): socket(%s, SOCK_STREAM, 0): Cannot create new socket? errno=%d (%s)\n", __func__, pf_name_list[server_watcher->socket_address.sa_ipv6.sin6_family], errno, strerror(errno));
        logging(LOGLEVEL_ERROR, log_str);
        return -1;
    }
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): socket(%s, SOCK_STREAM, 0): Create new socket. fd=%d\n", __func__, pf_name_list[server_watcher->socket_address.sa_ipv6.sin6_family], socket_result);
    logging(LOGLEVEL_INFO, log_str);
    // ソケットディスクリプタを設定
    server_watcher->socket_fd = socket_result;

    // ----------------
    // IPv6ソケット以外のソケット(!= PF_UNIX)で、SO_REUSEPORT使用となっているなら、ソケットのオプションを設定(親プロセス→複数子プロセス、としてイベントドリブンするなら必要)
    // ----------------
    // 今回は採用しない

    // ----------------
    // IPV6_V6ONLYを設定 → https://linuxjm.osdn.jp/html/LDP_man-pages/man7/ipv6.7.html (IPv4アプリケーションとIPv6アプリケーションが同時に一つのポートをバインドできる)
    // ----------------
    socket_result = setsockopt(server_watcher->socket_fd, IPPROTO_IPV6, IPV6_V6ONLY, &ipv6only_flag, sizeof(ipv6only_flag));
    // ソケットのオプション設定が出来なかったら
    if (socket_result < 0)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): setsockopt(fd=%d, IPPROTO_IPV6, IPV6_V6ONLY, ipv6=%d): Cannot set socket option!? errno=%d (%s)\n", __func__, server_watcher->socket_fd, ipv6only_flag, errno, strerror(errno));
        logging(LOGLEVEL_ERROR, log_str);
        return -1;
    }
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): setsockopt(fd=%d, IPPROTO_IPV6, IPV6_V6ONLY, ipv6=%d): OK.\n", __func__, server_watcher->socket_fd, ipv6only_flag);
    logging(LOGLEVEL_INFO, log_str);

    // ----------------
    // ソケット紐づけ(bind : ソケットのファイルディスクリプタとIPv6ソケットソケットアドレスを紐づけ)
    // ----------------
    socket_result = bind(server_watcher->socket_fd, (struct sockaddr *)&server_watcher->socket_address.sa_ipv6, sizeof(server_watcher->socket_address.sa_ipv6));
    // ソケットアドレスの紐づけが出来なかったら
    if (socket_result < 0)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): bind(fd=%d, in6addr_any): Cannot socket binding? errno=%d (%s)\n", __func__, server_watcher->socket_fd, errno, strerror(errno));
        logging(LOGLEVEL_ERROR, log_str);
        return -1;
    }
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): bind(fd=%d, in6addr_any): OK.\n", __func__, server_watcher->socket_fd);
    logging(LOGLEVEL_INFO, log_str);

    // ----------------
    // KeepAliveの初期化
    // ----------------
    socket_result = INIT_keepalive(server_watcher);
    // ソケットのKeepAlive設定が出来なかったら
    if (socket_result < 0)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): INIT_keepalive(fd=%d, keepalive=%d): Cannot set socket option!? errno=%d (%s)\n", __func__, server_watcher->socket_fd, EVS_config.keepalive, errno, strerror(errno));
        logging(LOGLEVEL_ERROR, log_str);
        return -1;
    }
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): INIT_keepalive(fd=%d, keepalive=%d): OK.\n", __func__, server_watcher->socket_fd, EVS_config.keepalive);
    logging(LOGLEVEL_INFO, log_str);

    // ----------------
    // ソケットリッスン(listen : SOMAXCONN = /proc/sys/net/core/somaxconnの値を接続最大数としてリッスン開始)
    // ----------------
    socket_result = listen(server_watcher->socket_fd, SOMAXCONN);
    // ソケットアドレスの紐づけが出来なかったら
    if (socket_result < 0)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): listen(fd=%d, %d): Cannot socket listen? errno=%d (%s)\n", __func__, server_watcher->socket_fd, SOMAXCONN, errno, strerror(errno));
        logging(LOGLEVEL_ERROR, log_str);
        return -1;
    }
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): listen(fd=%d, %d): OK.\n", __func__, server_watcher->socket_fd, SOMAXCONN);
    logging(LOGLEVEL_INFO, log_str);

    // --------------------------------
    // テールキュー処理
    // --------------------------------
    // テールキューの最後にこの接続の情報を追加する
    TAILQ_INSERT_TAIL(&EVS_server_tailq, server_watcher, entries);
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): TAILQ_INSERT_TAIL(server fd=%d): OK.\n", __func__, server_watcher->socket_fd);
    logging(LOGLEVEL_INFO, log_str);

    // --------------------------------
    // libev 処理
    // --------------------------------
    // サーバー別設定用構造体ポインタのI/O監視オブジェクトに対して、コールバック処理とソケットファイルディスクリプタ、そしてイベントのタイプを設定する
    ev_io_init(&server_watcher->io_watcher, CB_accept, server_watcher->socket_fd, EV_READ);
    ev_io_start(EVS_loop, &server_watcher->io_watcher);
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): ev_io_init(CB_accept, server fd=%d, EV_READ): OK.\n", __func__, server_watcher->socket_fd);
    logging(LOGLEVEL_INFO, log_str);
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): ev_io_start(): OK. Priority=%d\n", __func__, ev_priority(&server_watcher->io_watcher));
    logging(LOGLEVEL_INFO, log_str);

    return socket_result;
}

// --------------------------------
// IPv4ソケットの初期化
// --------------------------------
int INIT_pf_inet(struct EVS_ev_server_t * server_watcher)
{
    int                             socket_result;
    char                            log_str[MAX_LOG_LENGTH];

    // --------------------------------
    // ソケット処理
    // --------------------------------
    // プロトコルファミリーがPF_INETでなければ
    if (server_watcher->socket_address.sa_ipv4.sin_family != PF_INET)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(%d): Cannot support protocol family!?\n", __func__, server_watcher->socket_address.sa_ipv4.sin_family);
        logging(LOGLEVEL_ERROR, log_str);
        return -1;
    }

    // ----------------
    // ソケット生成(socket : IPv4ソケットでかつストリームで)
    // ----------------
    socket_result = socket(server_watcher->socket_address.sa_ipv4.sin_family, SOCK_STREAM, 0);
    // ソケット生成が出来なかったら
    if (socket_result < 0)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): socket(%s, SOCK_STREAM, 0): Cannot create new socket? errno=%d (%s)\n", __func__, pf_name_list[server_watcher->socket_address.sa_ipv4.sin_family], errno, strerror(errno));
        logging(LOGLEVEL_ERROR, log_str);
        return -1;
    }
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): socket(%s, SOCK_STREAM, 0): Create new socket. fd=%d\n", __func__, pf_name_list[server_watcher->socket_address.sa_ipv4.sin_family], socket_result);
    logging(LOGLEVEL_INFO, log_str);
    // ソケットディスクリプタを設定
    server_watcher->socket_fd = socket_result;

    // ----------------
    // IPv4ソケット以外のソケット(!= PF_UNIX)で、SO_REUSEPORT使用となっているなら、ソケットのオプションを設定(親プロセス→複数子プロセス、としてイベントドリブンするなら必要)
    // ----------------
    // 今回は採用しない

    // ----------------
    // ソケット紐づけ(bind : ソケットのファイルディスクリプタとIPv4ソケットソケットアドレスを紐づけ)
    // ----------------
    socket_result = bind(server_watcher->socket_fd, (struct sockaddr *)&server_watcher->socket_address.sa_ipv4, sizeof(server_watcher->socket_address.sa_ipv4));
    // ソケットアドレスの紐づけが出来なかったら
    if (socket_result < 0)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): bind(fd=%d, INADDR_ANY): Cannot socket binding? errno=%d (%s)\n", __func__, server_watcher->socket_fd, errno, strerror(errno));
        logging(LOGLEVEL_ERROR, log_str);
        return -1;
    }
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): bind(fd=%d, INADDR_ANY): OK.\n", __func__, server_watcher->socket_fd);
    logging(LOGLEVEL_INFO, log_str);

    // ----------------
    // KeepAliveの初期化
    // ----------------
    socket_result = INIT_keepalive(server_watcher);
    // ソケットのKeepAlive設定が出来なかったら
    if (socket_result < 0)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): INIT_keepalive(fd=%d, keepalive=%d): Cannot set socket option!? errno=%d (%s)\n", __func__, server_watcher->socket_fd, EVS_config.keepalive, errno, strerror(errno));
        logging(LOGLEVEL_ERROR, log_str);
        return -1;
    }
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): INIT_keepalive(fd=%d, keepalive=%d): OK.\n", __func__, server_watcher->socket_fd, EVS_config.keepalive);
    logging(LOGLEVEL_INFO, log_str);

    // ----------------
    // ソケットリッスン(listen : SOMAXCONN = /proc/sys/net/core/somaxconnの値を接続最大数としてリッスン開始)
    // ----------------
    socket_result = listen(server_watcher->socket_fd, SOMAXCONN);
    // ソケットアドレスの紐づけが出来なかったら
    if (socket_result < 0)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): listen(fd=%d, %d): Cannot socket listen? errno=%d (%s)\n", __func__, server_watcher->socket_fd, SOMAXCONN, errno, strerror(errno));
        logging(LOGLEVEL_ERROR, log_str);
        return -1;
    }
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): listen(fd=%d, %d): OK.\n", __func__, server_watcher->socket_fd, SOMAXCONN);
    logging(LOGLEVEL_INFO, log_str);

    // --------------------------------
    // テールキュー処理
    // --------------------------------
    // テールキューの最後にこの接続の情報を追加する
    TAILQ_INSERT_TAIL(&EVS_server_tailq, server_watcher, entries);
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): TAILQ_INSERT_TAIL(server fd=%d): OK.\n", __func__, server_watcher->socket_fd);
    logging(LOGLEVEL_INFO, log_str);

    // --------------------------------
    // libev 処理
    // --------------------------------
    // サーバー別設定用構造体ポインタのI/O監視オブジェクトに対して、コールバック処理とソケットファイルディスクリプタ、そしてイベントのタイプを設定する
    ev_io_init(&server_watcher->io_watcher, CB_accept, server_watcher->socket_fd, EV_READ);
    ev_io_start(EVS_loop, &server_watcher->io_watcher);
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): ev_io_init(CB_accept, server fd=%d, EV_READ): OK.\n", __func__, server_watcher->socket_fd);
    logging(LOGLEVEL_INFO, log_str);
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): ev_io_start(): OK. Priority=%d\n", __func__, ev_priority(&server_watcher->io_watcher));
    logging(LOGLEVEL_INFO, log_str);

    return socket_result;
}

// --------------------------------
// UNIXドメインソケットの初期化
// --------------------------------
int INIT_pf_unix(struct EVS_ev_server_t * server_watcher)
{
    int                             socket_result;
    char                            log_str[MAX_LOG_LENGTH];

    // --------------------------------
    // ソケット処理
    // --------------------------------
    // プロトコルファミリーがPF_UNIXでなければ
    if (server_watcher->socket_address.sa_un.sun_family != PF_UNIX)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(%d): Cannot support protocol family!?\n", __func__, server_watcher->socket_address.sa_un.sun_family);
        logging(LOGLEVEL_ERROR, log_str);
        return -1;
    }

    // ----------------
    // ソケット生成(socket : UNIXドメインソケットでかつストリームで)
    // ----------------
    socket_result = socket(server_watcher->socket_address.sa_un.sun_family, SOCK_STREAM, 0);
    // ソケット生成が出来なかったら
    if (socket_result < 0)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): socket(%s, SOCK_STREAM): Cannot create new socket? errno=%d (%s)\n", __func__, pf_name_list[server_watcher->socket_address.sa_un.sun_family], errno, strerror(errno));
        logging(LOGLEVEL_ERROR, log_str);
        return -1;
    }
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): socket(%s, SOCK_STREAM): Create new socket. fd=%d\n", __func__, pf_name_list[server_watcher->socket_address.sa_un.sun_family], socket_result);
    logging(LOGLEVEL_INFO, log_str);
    // ソケットディスクリプタを設定
    server_watcher->socket_fd = socket_result;

    // ----------------
    // UNIXドメイン以外のソケット(!= PF_UNIX)で、SO_REUSEPORT使用となっているなら、ソケットのオプションを設定(親プロセス→複数子プロセス、としてイベントドリブンするなら必要)
    // ----------------
    // 今回は採用しない

    // ----------------
    // ソケット紐づけ(bind : ソケットのファイルディスクリプタとUNIXドメインソケットアドレスを紐づけ)
    // ----------------
    socket_result = bind(server_watcher->socket_fd, (struct sockaddr *)&server_watcher->socket_address.sa_un, sizeof(server_watcher->socket_address.sa_un));
    // ソケットアドレスの紐づけが出来なかったら
    if (socket_result < 0)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): bind(fd=%d, %s): Cannot socket binding? errno=%d (%s)\n", __func__, server_watcher->socket_fd, server_watcher->socket_address.sa_un.sun_path, errno, strerror(errno));
        logging(LOGLEVEL_ERROR, log_str);
        return -1;
    }
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): bind(fd=%d, %s): OK.\n", __func__, server_watcher->socket_fd, server_watcher->socket_address.sa_un.sun_path);
    logging(LOGLEVEL_INFO, log_str);

    // ----------------
    // UNIXドメイン以外のソケット(!= PF_UNIX)なら、ソケットのその他のパラメータを設定(KeepAlive、Timeout、バッファサイズ、バッファリングOFF、etc...)
    // ----------------
    // PF_UNIXなのでスルー
    
    // ----------------
    // ソケットリッスン(listen : SOMAXCONN = /proc/sys/net/core/somaxconnの値を接続最大数としてリッスン開始)
    // ----------------
    socket_result = listen(server_watcher->socket_fd, SOMAXCONN);
    // ソケットアドレスの紐づけが出来なかったら
    if (socket_result < 0)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): listen(fd=%d, %d):Cannot socket listen? errno=%d (%s)\n", __func__, server_watcher->socket_fd, SOMAXCONN, errno, strerror(errno));
        logging(LOGLEVEL_ERROR, log_str);
        return -1;
    }
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): listen(fd=%d, %d): OK.\n", __func__, server_watcher->socket_fd, SOMAXCONN);
    logging(LOGLEVEL_INFO, log_str);

    // --------------------------------
    // テールキュー処理
    // --------------------------------
    // テールキューの最後にこの接続の情報を追加する
    TAILQ_INSERT_TAIL(&EVS_server_tailq, server_watcher, entries);
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): TAILQ_INSERT_TAIL(server fd=%d): OK.\n", __func__, server_watcher->socket_fd);
    logging(LOGLEVEL_INFO, log_str);

    // --------------------------------
    // libev 処理
    // --------------------------------
    // サーバー別設定用構造体ポインタのI/O監視オブジェクトに対して、コールバック処理とソケットファイルディスクリプタ、そしてイベントのタイプを設定する
    ev_io_init(&server_watcher->io_watcher, CB_accept, server_watcher->socket_fd, EV_READ);
    ev_io_start(EVS_loop, &server_watcher->io_watcher);
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): ev_io_init(CB_accept, server fd=%d, EV_READ): OK.\n", __func__, server_watcher->socket_fd);
    logging(LOGLEVEL_INFO, log_str);
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): ev_io_start(): OK. Priority=%d\n", __func__, ev_priority(&server_watcher->io_watcher));
    logging(LOGLEVEL_INFO, log_str);

    return socket_result;
}

// --------------------------------
// 各種ソケット初期化処理
// --------------------------------
int INIT_socket(struct EVS_port_t *listen_port)
{
    int                             init_result;
    struct EVS_ev_server_t          *server_watcher;                // ev_io＋ソケットファイルディスクリプタ、ソケットアドレスなどの拡張構造体
    char                            log_str[MAX_LOG_LENGTH];

    // --------------------------------
    // プロトコルファミリー別に各種初期化処理
    // --------------------------------
    // ----------------
    // ポート別に処理分岐
    // ----------------
    // ボート番号0(=UNIXドメインソケット)なら
    if (listen_port->port == 0)
    {
        // ----------------
        // サーバー別設定用構造体ポインタのメモリ領域を確保 (それぞれのポート＆プロトコルファミリー毎に確保すること)
        // ----------------
        server_watcher = (struct EVS_ev_server_t *)calloc(1, sizeof(struct EVS_ev_server_t));
        // メモリ領域が確保できなかったら
        if (server_watcher == NULL)
        {
            snprintf(log_str, MAX_LOG_LENGTH, "%s(): Cannot calloc server_watcher's memory? errno=%d (%s)\n", __func__, errno, strerror(errno));
            logging(LOGLEVEL_ERROR, log_str);
            return -1;
        }
        // ----------------
        // UNIXドメインソケットの初期化
        // ----------------
        server_watcher->socket_address.sa_un.sun_family = PF_UNIX;                      // プロトコルファミリーを設定
        snprintf(server_watcher->socket_address.sa_un.sun_path, sizeof(server_watcher->socket_address.sa_un.sun_path), "%s", EVS_config.domain_socketfile); // UNIXドメインソケットアドレスのsun_pathにソケットファイル名を設定(snprintfを使うことで、UNIX_PATH_MAX-1文字しか設定されない)
        server_watcher->ssl_support = listen_port->ssl;                                 // SSL/TLS対応を設定
        init_result = INIT_pf_unix(server_watcher);                                     // UNIXドメインソケットの初期化処理
    }
    // それ以外のポート番号なら
    else if (listen_port->port > 0 && listen_port->port <= 65535)
    {
        // IPv4対応なら
        if (listen_port->ipv4 == 1)
        {
            // ----------------
            // サーバー別設定用構造体ポインタのメモリ領域を確保 (それぞれのポート＆プロトコルファミリー毎に確保すること)
            // ----------------
            server_watcher = (struct EVS_ev_server_t *)calloc(1, sizeof(struct EVS_ev_server_t));
            // メモリ領域が確保できなかったら
            if (server_watcher == NULL)
            {
                snprintf(log_str, MAX_LOG_LENGTH, "%s(): Cannot calloc server_watcher's memory? errno=%d (%s)\n", __func__, errno, strerror(errno));
                logging(LOGLEVEL_ERROR, log_str);
                return -1;
            }
            // ----------------
            // IPv4ソケットの初期化
            // ----------------
            server_watcher->socket_address.sa_ipv4.sin_family = PF_INET;                // プロトコルファミリーを設定
            server_watcher->socket_address.sa_ipv4.sin_port = htons(listen_port->port); // ポート番号を設定
            server_watcher->socket_address.sa_ipv4.sin_addr.s_addr = INADDR_ANY;        // 待ち受けアドレス(INADDR_ANY:どこからの接続でもOK)を設定
            server_watcher->ssl_support = listen_port->ssl;                             // SSL/TLS対応を設定
            init_result = INIT_pf_inet(server_watcher);                                 // IPv4ソケットの初期化処理
        }
        // IPv6対応なら
        if (listen_port->ipv6 == 1)
        {
            // ----------------
            // サーバー別設定用構造体ポインタのメモリ領域を確保 (それぞれのポート＆プロトコルファミリー毎に確保すること)
            // ----------------
            server_watcher = (struct EVS_ev_server_t *)calloc(1, sizeof(struct EVS_ev_server_t));
            // メモリ領域が確保できなかったら
            if (server_watcher == NULL)
            {
                snprintf(log_str, MAX_LOG_LENGTH, "%s(): Cannot calloc server_watcher's memory? errno=%d (%s)\n", __func__, errno, strerror(errno));
                logging(LOGLEVEL_ERROR, log_str);
                return -1;
            }
            // ----------------
            // IPv6ソケットの初期化
            // ----------------
            server_watcher->socket_address.sa_ipv6.sin6_family = PF_INET6;              // プロトコルファミリーを設定
            server_watcher->socket_address.sa_ipv6.sin6_port = htons(listen_port->port);// ポート番号を設定
            server_watcher->socket_address.sa_ipv6.sin6_addr = in6addr_any;             // 待ち受けアドレス(in6addr_any:どこからの接続でもOKを設定※IN6ADDR_ANY_INITは変数宣言時にしか設定できないョ!!)を設定
            server_watcher->ssl_support = listen_port->ssl;                             // SSL/TLS対応を設定
            init_result = INIT_pf_inet6(server_watcher);                                // IPv6ソケットの初期化処理
        }
    }
    // 上記以外場合には
    else
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): Cannot support port number!? %d\n", listen_port->port);
        logging(LOGLEVEL_ERROR, log_str);
        init_result = -1;
    }
    // 戻る
    return init_result;
}

// --------------------------------
// 初期化処理
// --------------------------------
int INIT_all(int argc, char *argv[])
{
    int                             init_result;
    struct EVS_port_t               *listen_port;                       // ポート別設定用構造体ポインタ
    char                            log_str[MAX_LOG_LENGTH];

    pid_t                           pid;                                // フォーク後のプロセスID
    int                             pidfile_fd = 0;                     // PIDファイルディスクリプタ

    int                             env_num = 0;                        // 環境変数の番号

    // --------------------------------
    // 各テールキューの初期化処理
    // --------------------------------
    // ポート用テールキューの初期化
    TAILQ_INIT(&EVS_port_tailq);
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): TAILQ_INIT(&EVS_port_tailq): OK.\n", __func__);
    logging(LOGLEVEL_INFO, log_str);

    // サーバー用テールキューの初期化
    TAILQ_INIT(&EVS_server_tailq);
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): TAILQ_INIT(&EVS_server_tailq): OK.\n", __func__);
    logging(LOGLEVEL_INFO, log_str);

    // クライアント用テールキューの初期化
    TAILQ_INIT(&EVS_client_tailq);
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): TAILQ_INIT(&EVS_client_tailq): OK.\n", __func__);
    logging(LOGLEVEL_INFO, log_str);

    // タイマー用テールキューの初期化
    TAILQ_INIT(&EVS_timer_tailq);
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): TAILQ_INIT(&EVS_timer_tailq): OK.\n", __func__);
    logging(LOGLEVEL_INFO, log_str);

    // --------------------------------
    // 各種設定値初期化処理
    // --------------------------------
    init_result = INIT_config(argc, argv);
    if (init_result < 0)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): INIT_config(): Cannot initialize config!?\n", __func__);
        logging(LOGLEVEL_ERROR, log_str);
        return init_result;
    }
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): INIT_config(): OK.\n", __func__);
    logging(LOGLEVEL_INFO, log_str);

    // pidファイルの設定がないなら
    if (EVS_config.pid_file == NULL)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): Cannot find PID filename!?\n", __func__);
        logging(LOGLEVEL_ERROR, log_str);
        return -1;
    }
    // pidファイルを読み込みで開いてみる
    pidfile_fd = open(EVS_config.pid_file, (O_RDONLY));
    // pidファイルが開けてしまうなら
    if (pidfile_fd != -1)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): Found other process PID file!? errno=%d (%s)\n", __func__, errno, strerror(errno));
        logging(LOGLEVEL_ERROR, log_str);
        // PIDファイルを閉じる
        close(pidfile_fd);
        return -1;
    }

    // --------------------------------
    // デーモン化処理
    // --------------------------------
    // // デーモンモードが1:ONなら
    if (EVS_config.daemon == 1)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): daemon(0, 0): Go!\n", __func__);               // daemon(0, 0): を呼ぶ前にログを出力
        logging(LOGLEVEL_INFO, log_str);
    
        // プロセスをdaemon()で子プロセスを生成する
        init_result = daemon(0, 1);
        // 子プロセスが生成できなかったら
        if (init_result == -1)
        {
            snprintf(log_str, MAX_LOG_LENGTH, "%s(): Cannot daemonized new process!? errno=%d (%s)\n", __func__, errno, strerror(errno));
            logging(LOGLEVEL_ERROR, log_str);
            return -1;
        }
        // 子プロセスが生成できたら
        else if (init_result != 0)
        {
            // 親プロセスはここで終わり(実際にはここには来ないけど念のためexit(0)しといて、あとはデーモン化したプロセスに任せる)
            logging(LOGLEVEL_INFO, "daemonized OK!!\n");                    // デーモン化した旨を出力
            exit(0);
        }
    }

    // --------------------------------
    // PIDファイル処理
    // --------------------------------
    // PIDファイルを新規書き込みで開く
    pidfile_fd = open(EVS_config.pid_file, (O_WRONLY | O_CREAT | O_APPEND), (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
    // PIDファイルが開けなかったら
    if (pidfile_fd == -1)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): Cannot create PID file!? errno=%d (%s)\n", __func__, errno, strerror(errno));
        logging(LOGLEVEL_ERROR, log_str);
        return -1;
    }
    // プロセスIDを取得
    pid = getpid();
    // プロセスID文字列を生成
    snprintf(log_str, MAX_LOG_LENGTH, "%d\n", (int)getpid());
    // PIDファイルにプロセスIDを書き出す
    init_result = write(pidfile_fd, log_str, strlen(log_str) );
    // PIDファイルにプロセスIDを書き出せなかったら
    if (init_result != strlen(log_str))
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): Cannot write PID file!? init_result=%d!=%d errno=%d (%s)\n", __func__, init_result, strlen(log_str) + 1,errno, strerror(errno));
        logging(LOGLEVEL_ERROR, log_str);
        // PIDファイルを閉じる
        close(pidfile_fd);
        return -1;
    }
    // PIDファイルを閉じる
    close(pidfile_fd);

    // --------------------------------
    // UNIXドメインソケット関連初期化処理
    // --------------------------------
    // ポート別設定用構造体ポインタのメモリ領域を確保
    listen_port = (struct EVS_port_t *)calloc(1, sizeof(struct EVS_port_t));
    // メモリ領域が確保できなかったら
    if (listen_port == NULL)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): Cannot calloc memory? errno=%d (%s)\n", __func__, errno, strerror(errno));
        logging(LOGLEVEL_ERROR, log_str);
        return -1;
    }
    listen_port->port = 0;                              // ポート番号を設定する(以下、念のため同様に)
    listen_port->ipv4 = 0;                              // IPv4フラグ(0:OFF、1:ON)に0(=OFF)を設定する
    listen_port->ipv6 = 0;                              // IPv6フラグ(0:OFF、1:ON)に0(=OFF)を設定する
    listen_port->ssl = 0;                               // SSL/TLSフラグ(0:OFF、1:ON)に1を設定する
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): UNIX Domain socket preparation.\n", __func__);
    logging(LOGLEVEL_INFO, log_str);
    // テールキューの最後にこの接続の情報を追加する
    TAILQ_INSERT_TAIL(&EVS_port_tailq, listen_port, entries);
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): TAILQ_INSERT_TAIL(listen_port): OK.\n", __func__);
    logging(LOGLEVEL_INFO, log_str);

    // --------------------------------
    // OpenSSL関連初期化処理
    // --------------------------------
    // SSL/TLS対応しないなら
    if (EVS_config.ssl_support == 0)
    {
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): SSL/TLS not support by config.\n", __func__);
    logging(LOGLEVEL_INFO, log_str);
    }
    // SSL/TLS対応するなら
    else
    {
        // OpenSSL関連初期化処理
        init_result = INIT_openssl();
        if (init_result == 0)                                // OpenSSL関連は、戻り値が0だとエラーなのでこの判定条件になる
        {
            snprintf(log_str, MAX_LOG_LENGTH, "%s(): INIT_openssl(): Cannot initialize openssl etc...!?\n", __func__);
            logging(LOGLEVEL_ERROR, log_str);
            return -1;                                        // なので、ここでは0を返さず-1を返すのがいいだろう
        }
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): INIT_openssl(): OK.\n", __func__);
    logging(LOGLEVEL_INFO, log_str);
    }
    // あとはポート毎に、SSL/TLS対応するかどうかを設定する

    // --------------------------------
    // libev関連初期化処理
    // --------------------------------
    init_result = INIT_libev();
    if (init_result < 0)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): INIT_libev(): Cannot initialize libev etc...!?\n", __func__);
        logging(LOGLEVEL_ERROR, log_str);
        return init_result;
    }
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): INIT_libev(): OK.\n", __func__);
    logging(LOGLEVEL_INFO, log_str);

    // --------------------------------
    // ポート別初期化処理 (getaddrinfo()を使う方法もあるが、どうせPF_UNIXは別処理しないといけないし、結局今回はポート別にsocket→bind→listenする)
    // --------------------------------
    // ポート用テールキューからポート情報を取得して全て処理
    TAILQ_FOREACH (listen_port, &EVS_port_tailq, entries)
    {
        // プロトコルファミリー別に初期化
        init_result = INIT_socket(listen_port);
        if (init_result < 0)
        {
            snprintf(log_str, MAX_LOG_LENGTH, "%s(): INIT_socket(): Cannot initialize socket!?\n", __func__);
            logging(LOGLEVEL_ERROR, log_str);
            return init_result;
        }
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): INIT_socket(port=%d): OK.\n", __func__, listen_port->port);
        logging(LOGLEVEL_INFO, log_str);
    }

    // --------------------------------
    // タイマーイベント初期化処理
    // --------------------------------
    // タイマーオブジェクトに対して、コールバック処理とタイマーイベント確認間隔(timer_checkintval秒)、そして繰り返し回数(0回)を設定する
    ev_timer_init(&timeout_watcher, CB_timeout, EVS_config.timer_checkintval, 0);
    ev_timer_start(EVS_loop, &timeout_watcher);

    // ----------------------------------------------------------------
    // 以下、個別のAPI関連の初期化処理
    // ----------------------------------------------------------------

    // 戻る
    return init_result;
}
