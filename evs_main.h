// ----------------------------------------------------------------------
// EvServer - Libev Server (Sample) -
// Version:
//     Please show evs_main.c.
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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>                                          // 標準入出力関連
#include <unistd.h>                                         // 標準入出力ファイルディスクリプタの型定義とか
#include <stdlib.h>                                         // 標準処理関連
#include <string.h>                                         // 文字列関連
#include <ctype.h>                                          // 文字関連
#include <fcntl.h>                                          // ファイル関連

#include <sys/queue.h>                                      // リスト・テール(tail)キュー・循環キュー関連
#include <sys/types.h>                                      // データタイプ(型)関連
#include <sys/socket.h>                                     // ソケット関連
#include <sys/ioctl.h>                                      // I/O関連
#include <sys/un.h>                                         // UNIXドメインソケット関連
#include <sys/stat.h>                                       // ステータス関連

#include <arpa/inet.h>                                      // アドレス変換関連

#include <netinet/in.h>                                     // IPv4関連
#include <netinet/tcp.h>                                    // TCP関連

#include <openssl/ssl.h>                                    // OpenSSL関連
#include <openssl/err.h>                                    // OpenSSL関連
#include <openssl/crypto.h>                                 // OpenSSL関連

#include <ev.h>                                             // libev関連

#include <errno.h>                                          // エラー番号関連

// --------------------------------
// 定数宣言
// --------------------------------
#define EVS_NAME                "EvServer"
#define EVS_VERSION             "0.0.9"

#define MAX_STRING_LENGTH       1024                        // 設定ファイル中とかの、一行当たりの最大文字数
#define MAX_LOG_LENGTH          1024                        // ログの、一行当たりの最大文字数
#define MAX_MESSAGE_LENGTH      8192                        // ソケット通信時の一回の送受信(recv/send)当たりの最大文字数
#define MAX_SIZE_16K            16384                       // 定数16KB
#define MAX_SIZE_32K            32768                       // 定数32KB
#define MAX_SIZE_64K            65536                       // 定数64KB
#define MAX_SIZE_128K           131072                      // 定数128KB

#define MAX_PF_NUM              16                          // 対応するプロトコルファミリーの最大数(PF_KEYまで…実際にはPF_UNIX、PF_INET、PF_INET6しか扱わない)

enum    loglevel {                                          // ログレベル
                                LOGLEVEL_DEBUG,
                                LOGLEVEL_INFO,
                                LOGLEVEL_TEST,
                                LOGLEVEL_LOG,
                                LOGLEVEL_WARN,
                                LOGLEVEL_ERROR,
};

// ----------------
// 以下、個別のAPI関連
// ----------------

// --------------------------------
// 型宣言
// --------------------------------
struct EVS_config_t {                                       // 各種設定用構造体
    int             daemon;                                 // デーモン化(0:フロントプロセス、1:デーモン化)

    char            *pid_file;                              // PIDファイル名のフルパス
    char            *log_file;                              // ログファイル名のフルパス
    int             log_level;                              // ログに出力するレベル(0:DEBUG, 1:INFO, 2:WARN, 3:ERROR)

    char            *domain_socketfile;                     // UNIXドメインソケットファイル名のフルパス

    int             ssl_support;                            // SSL/TLS対応(0:非対応、1:対応)
    char            *ssl_ca_file;                           // サーバー証明書(PEM)CAファイルのフルパス
    char            *ssl_cert_file;                         // サーバー証明書(PEM)CERTファイルのフルパス
    char            *ssl_key_file;                          // サーバー証明書(PEM)KEYファイルのフルパス

    ev_tstamp       timer_checkintval;                      // タイマーイベント確認間隔(秒)

    int             nocommunication_check;                  // 無通信タイムアウトチェック(0:無効、1:有効)
    ev_tstamp       nocommunication_timeout;                // 無通信タイムアウト(秒)
    
    int             keepalive;                              // KeepAlive(0:無効、1:有効)
    int             keepalive_idletime;                     // KeepAlive Idle(秒)
    int             keepalive_intval;                       // KeepAlive Interval(秒)
    int             keepalive_probes;                       // KeepAlive Probes(回数)
};

struct EVS_port_t {                                         // ポート別設定用構造体
    int             port;                                   // ポート番号(1～65535)
    int             ipv4;                                   // IPv4フラグ(0:OFF、1:ON)
    int             ipv6;                                   // IPv4フラグ(0:OFF、1:ON)
    int             ssl;                                    // SSL/TLSフラグ(0:OFF、1:ON)
    TAILQ_ENTRY (EVS_port_t) entries;                       // 次のTAILQ構造体への接続 → man3/queue.3.html
};

struct EVS_ev_server_t {                                    // コールバック関数内でソケットのファイルディスクリプタも知りたいので拡張した構造体を宣言する、こちらはサーバー用
    ev_io           io_watcher;                             // libevのev_io、これをev_io_init()＆ev_io_start()に渡す
    int             socket_fd;                              // socket_fd、コールバック関数内でstruct ev_io*で渡される変数のポインタをEVS_ev_server_t*に型変換することで参照する
    int             ssl_support;                            // SSL/TLS対応状態(0:非対応、1:SSL/TLS対応)
    union {                                                 // ソケットアドレス構造体の共用体
        struct sockaddr_in  sa_ipv4;                        //  IPv4用ソケットアドレス構造体
        struct sockaddr_in6 sa_ipv6;                        //  IPv6用ソケットアドレス構造体
        struct sockaddr_un  sa_un;                          //  UNIXドメイン用ソケットアドレス構造体
        struct sockaddr     sa;                             //  ソケットアドレス構造体
    } socket_address;
    TAILQ_ENTRY (EVS_ev_server_t) entries;                  // 次のTAILQ構造体への接続 → man3/queue.3.html
};

struct EVS_ev_client_t {                                    // コールバック関数内でソケットのファイルディスクリプタも知りたいので拡張した構造体を宣言する、こちらはクライアント用
    ev_io           io_watcher;                             // libevのev_io、これをev_io_init()＆ev_io_start()に渡す
    ev_tstamp       last_activity;                          // 最終アクティブ日時(監視対象が最後にアクティブとなった=タイマー更新した日時)
    int             socket_fd;                              // socket_fd、コールバック関数内でstruct ev_io*で渡される変数のポインタをEVS_ev_client_t*に型変換することで参照する
    int             client_status;                          // クライアント毎の状態(0:accept直後、1:ヘッダ受信中、2:コンテンツ受信中、など)
    int             ssl_status;                             // SSL接続状態(0:非SSL/SSL接続前、1:SSLハンドシェイク前、2:SSL接続中)
    SSL             *ssl;                                   // SSL接続情報
/*
    union {                                                 // ソケットアドレス構造体の共用体
        struct sockaddr_in  sa_ipv4;                        //  IPv4用ソケットアドレス構造体
        struct sockaddr_in6 sa_ipv6;                        //  IPv6用ソケットアドレス構造体
        struct sockaddr_un  sa_un;                          //  UNIXドメイン用ソケットアドレス構造体
        struct sockaddr     sa;                             //  ソケットアドレス構造体
    } socket_address;
*/
    char            addr_str[64];                           // アドレスを文字列として格納する(UNIX DOMAIN SOCKET/xxx.xxx.xxx.xxx(IPv4)/xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx(IPv6))
    TAILQ_ENTRY (EVS_ev_client_t) entries;                  // 次のTAILQ構造体への接続 → man3/queue.3.html
};

struct EVS_timer_t {                                        // タイマー別構造体
    ev_tstamp       timeout;                                // タイムアウト秒(ev_now + タイムアウト時間)
    void            *target;                                // タイムアウトに必要な構造体のポインタ
    TAILQ_ENTRY (EVS_timer_t) entries;                      // 次のTAILQ構造体への接続 → man3/queue.3.html
};

// --------------------------------
// 変数宣言
// --------------------------------
// ----------------
// 設定値関連
// ----------------
extern struct EVS_config_t              EVS_config;                     // システム設定値

// ----------------
// libev 関連
// ----------------
extern ev_idle                          idle_socket_watcher;            // アイドルオブジェクト(なにもイベントがないときに呼ばれる、監視対象イベントごとに設定しないといけない)
extern ev_io                            stdin_watcher;                  // I/O監視オブジェクト
extern ev_timer                         timeout_watcher;                // タイマーオブジェクト
extern ev_signal                        signal_watcher_sighup;          // シグナルオブジェクト(シグナルごとにウォッチャーを分けないといけない)
extern ev_signal                        signal_watcher_sigint;          // シグナルオブジェクト(シグナルごとにウォッチャーを分けないといけない)
extern ev_signal                        signal_watcher_sigterm;         // シグナルオブジェクト(シグナルごとにウォッチャーを分けないといけない)

extern struct ev_loop                   *EVS_loop;                      // イベントループ

extern struct EVS_ev_server_t           *server_watcher[];              // ev_io＋ソケットファイルディスクリプタ、ソケットアドレスなどの拡張構造体

// ----------------
// ソケット関連
// ----------------
extern const char                       *pf_name_list[];                // プロトコルファミリーの一部の名称を文字列テーブル化
extern int                              EVS_connect_num;                // クライアント接続数

// ----------------
// SSL/TLS関連
// ----------------
extern SSL_CTX                          *EVS_ctx;                       // SSL設定情報

// ----------------
// その他の変数
// ----------------
extern const char                       *loglevel_list[];               // ログレベル文字列テーブル
extern int                              EVS_log_fd;                     // ログファイルディスクリプタ

// ----------------
// 以下、個別のAPI関連
// ----------------

// ----------------------------------------------------------------------
// コード部分
// ----------------------------------------------------------------------
// --------------------------------
// プロトタイプ宣言
// --------------------------------
extern char *getdumpstr(void *, int);                                   // ダンプ文字列生成処理
extern void logging(int, char *);                                       // ログ出力処理

extern int INIT_all(int, char *[]);                                     // 初期化処理

extern void CLOSE_client(struct ev_loop *, struct ev_io *, int);        // クライアント接続終了処理
extern int CLOSE_all(void);                                             // 終了処理

extern int API_start(struct EVS_ev_client_t *, char *, ssize_t);        // API開始処理(クライアント別処理分岐、スレッド生成など)

// ----------------
// テールキュー関連
// ----------------
TAILQ_HEAD(EVS_port_tailq_head, EVS_port_t)         EVS_port_tailq;     // ポート用TAILQ_HEAD構造体 → man3/queue.3.html
TAILQ_HEAD(EVS_server_tailq_head, EVS_ev_server_t)  EVS_server_tailq;   // サーバー用TAILQ_HEAD構造体 → man3/queue.3.html
TAILQ_HEAD(EVS_client_tailq_head, EVS_ev_client_t)  EVS_client_tailq;   // クライアント用TAILQ_HEAD構造体 → man3/queue.3.html
TAILQ_HEAD(EVS_timer_tailq_head, EVS_timer_t)       EVS_timer_tailq;    // タイマー用TAILQ_HEAD構造体 → man3/queue.3.html
