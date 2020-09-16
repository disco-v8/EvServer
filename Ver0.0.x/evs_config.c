// ----------------------------------------------------------------------
// EvServer - Libev Server (Sample) -
// Purpose:
//     Read from config file & set to variable.
//
// Program:
//     Takeshi Kaburagi/MyDNS.JP    disco-v8@4x4.jp
//
// Usage:
//     ./evserver [OPTIONS]
// ----------------------------------------------------------------------

// evs_config.c は設定ファイルから設定値を読み込む関数だけをまとめたファイルで、evs_init.cがごちゃごちゃするので分離している。
// evs_init.c からincludeされることを想定しているので、evs_main.hなどのヘッダファイルはincludeしていない。

// ----------------------------------------------------------------------
// ヘッダ部分
// ----------------------------------------------------------------------
// --------------------------------
// インクルード宣言
// --------------------------------

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
// 設定用文字列の整理(パラメータ名の大文字化＆余計な文字の削除。変換後の文字列は不要になったら破棄:free()すること)
// --------------------------------
char *config_str_cleaner(char *target, int target_len)
{
    char                            log_str[MAX_LOG_LENGTH];

    char                            *src_pos;                           // 元の文字列のポインタ
    char                            *temp_pos;                          // 大文字変換作業用文字列のポインタ
    char                            *dest_pos;                          // 大文字変換後の文字列のポインタ

    // 大文字変換用文字列のメモリ領域を確保(+ 1バイトを忘れずに!!)
    dest_pos = (char *)calloc(1, target_len + 1);
    // メモリ領域が確保できなかったら
    if (dest_pos == NULL)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): Cannot calloc memory? errno=%d (%s)\n", __func__, errno, strerror(errno));
        logging(LOGLEVEL_ERROR, log_str);
        return dest_pos;
    }

    // 大文字変換作業用文字列のポインタに大文字変換後の文字列のポインタを設定(temp_posはどんどん進んでしまうので)
    temp_pos = dest_pos;

    // 指定された文字列の最初の文字から調べていく
    for (src_pos = target; *src_pos; src_pos++)
    {
        // *src_pos がブランク文字の場合には
        if (isspace(*src_pos) || isblank(*src_pos))
        {
            // スルー
            continue;
        }
        // *temp_pos:変換後の文字列に大文字にした*src_posを設定
        *temp_pos = toupper(*src_pos);
        // temp_pos:変換後の文字列を一文字進める
        temp_pos ++;
        // *src_pos が「=」だったら
        if (*src_pos == '=')
        {
            break;
        }
    }
    // 残りの文字列を格納していく
    for (src_pos += 1; *src_pos; src_pos++)
    {
        // *src_pos がブランク文字の場合には
        if (isspace(*src_pos) || isblank(*src_pos))
        {
            // スルー
            continue;
        }
        // *temp_pos:変換後の文字列に*src_posを設定
        *temp_pos = *src_pos;
        // temp_pos:変換後の文字列を一文字進める
        temp_pos ++;
    }
    // 大文字変換したあとの文字列の最後に'\0'を設定
    *temp_pos = '\0';

    return dest_pos;
}

// --------------------------------
// 設定用文字列の変換(パラメータ名別に設定値の取得。変換後の文字列は不要になったら破棄:free()すること)
// --------------------------------
int config_str_convert(char *target, int target_len)
{
    int                             init_result = 0;                    // 設定用文字列変換結果、初期値=0(正常終了)
    char                            log_str[MAX_LOG_LENGTH];

    char                            *changed_str;                       // 整理後の文字列ポインタ
    char                            *key_str;                           // 設定値名文字列ポインタ
    char                            *value_str;                         // 設定値文字列ポインタ
    char                            value[3][MAX_STRING_LENGTH];        // 各設定値文字列
    struct EVS_port_t               *listen_port;                       // ポート別設定用構造体ポインタ

    // 設定値名文字列のメモリ領域を確保(+ 1バイトを忘れずに!!)
    key_str = (char *)calloc(1, target_len + 1);
    // メモリ領域が確保できなかったら
    if (key_str == NULL)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): Cannot calloc memory? errno=%d (%s)\n", __func__, errno, strerror(errno));
        logging(LOGLEVEL_ERROR, log_str);
        return -1;
    }

    // 設定値文字列のメモリ領域を確保(+ 1バイトを忘れずに!!)
    value_str = (char *)calloc(1, target_len + 1);
    // メモリ領域が確保できなかったら
    if (value_str == NULL)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): Cannot calloc memory? errno=%d (%s)\n", __func__, errno, strerror(errno));
        logging(LOGLEVEL_ERROR, log_str);
        // 設定値名のメモリ領域は不要になったので破棄
        free(key_str);
        return -1;
    }

    // 設定用文字列の整理(パラメータ名の大文字化＆余計な文字の削除。変換後の文字列は不要になったら破棄:free()すること)
    changed_str = config_str_cleaner(target, target_len);
    // 文字列変換ができなかったら
    if (changed_str == NULL)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): Cannot clearning string!?\n", __func__);
        logging(LOGLEVEL_ERROR, log_str);
        // 設定値名のメモリ領域は不要になったので破棄
        free(key_str);
        // 設定値のメモリ領域は不要になったので破棄
        free(value_str);
        return -1;
    }
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): OK %s\n", __func__, changed_str); 
    logging(LOGLEVEL_INFO, log_str);
    
    // 整理出来たら、設定値名と設定値に変換、その1
    init_result = sscanf(changed_str, "%[^=]=%s", key_str, value_str);
    // 変換後の文字列のメモリ領域は不要になったので破棄
    free(changed_str);
    // 変換数が2ではないなら
    if (init_result != 2)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): Cannot get key_str and value_str!? %d %s\n", __func__, init_result, key_str);
        logging(LOGLEVEL_ERROR, log_str);
        // 設定値名のメモリ領域は不要になったので破棄
        free(key_str);
        // 設定値のメモリ領域は不要になったので破棄
        free(value_str);
        return -1;
    }

    // ------------------------------------
    // 設定値名別処理分岐
    // ------------------------------------
    // ----------------
    // デーモン設定なら
    // ----------------
    if (strcmp("DAEMON", key_str) == 0)
    {
        // 設定値の中に"ON"か'1'があれば
        if (strstr(value_str, "ON") != NULL || strstr(value_str, "On") != NULL || strstr(value_str, "on") != NULL || strchr(value_str, '1') != NULL)
        {
            // デーモンモードを1:ONに設定
            EVS_config.daemon = 1;
            snprintf(log_str, MAX_LOG_LENGTH, "%s(): DAEMON=%d\n", __func__, EVS_config.daemon);
            logging(LOGLEVEL_INFO, log_str);
            }
        else
        {
            // デーモンモードを0:OFFに設定
            EVS_config.daemon = 0;
            snprintf(log_str, MAX_LOG_LENGTH, "%s(): DAEMON=%d\n", __func__, EVS_config.daemon);
            logging(LOGLEVEL_INFO, log_str);
        }
    }
    // ----------------
    // PIDファイル設定なら
    // ----------------
    else if (strcmp("PIDFILE", key_str) == 0)
    {
        // 設定値文字列のメモリ領域を確保(+ 1バイトを忘れずに!!)
        EVS_config.pid_file = (char *)realloc((void *)EVS_config.pid_file, strlen(value_str) + 1);
        // メモリ領域が確保できなかったら
        if (EVS_config.pid_file == NULL)
        {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): Cannot realloc memory? errno=%d (%s)\n", __func__, errno, strerror(errno));
        logging(LOGLEVEL_ERROR, log_str);
            // 設定値名のメモリ領域は不要になったので破棄
            free(key_str);
            // 設定値のメモリ領域は不要になったので破棄
            free(value_str);
            return -1;
        }
        // PIDファイルを設定
        memcpy((void *)EVS_config.pid_file, (void *)value_str, strlen(value_str));
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): PidFile=%s\n", __func__, EVS_config.pid_file);
        logging(LOGLEVEL_INFO, log_str);
    }
    // ----------------
    // ログファイル設定なら
    // ----------------
    else if (strcmp("LOGFILE", key_str) == 0)
    {
        // 設定値文字列のメモリ領域を確保(+ 1バイトを忘れずに!!)
        EVS_config.log_file = (char *)realloc((void *)EVS_config.log_file, strlen(value_str) + 1);
        // メモリ領域が確保できなかったら
        if (EVS_config.log_file == NULL)
        {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): Cannot realloc memory? errno=%d (%s)\n", __func__, errno, strerror(errno));
        logging(LOGLEVEL_ERROR, log_str);
            // 設定値名のメモリ領域は不要になったので破棄
            free(key_str);
            // 設定値のメモリ領域は不要になったので破棄
            free(value_str);
            return -1;
        }
        // ログファイルを設定
        memcpy((void *)EVS_config.log_file, (void *)value_str, strlen(value_str));
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): LogFile=%s\n", __func__, EVS_config.log_file);
        logging(LOGLEVEL_INFO, log_str);
    }
    // ----------------
    // ログレベル設定なら
    // ----------------
    else if (strcmp("LOGLEVEL", key_str) == 0)
    {
        // ログレベルを設定(0:DEBUG, 1:INFO, 2:WARN, 3:ERROR)
        EVS_config.log_level = atoi(value_str);
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): log_level=%d\n", __func__, atoi(value_str));
        logging(LOGLEVEL_DEBUG, log_str);
    }
    // ----------------
    // UNIXドメインソケットファイル設定なら
    // ----------------
    else if (strcmp("SOCKETFILE", key_str) == 0)
    {
        // 設定値文字列のメモリ領域を確保(+ 1バイトを忘れずに!!)
        EVS_config.domain_socketfile = (char *)realloc((void *)EVS_config.domain_socketfile, strlen(value_str) + 1);
        // メモリ領域が確保できなかったら
        if (EVS_config.domain_socketfile == NULL)
        {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): Cannot realloc memory? errno=%d (%s)\n", __func__, errno, strerror(errno));
        logging(LOGLEVEL_ERROR, log_str);
            // 設定値名のメモリ領域は不要になったので破棄
            free(key_str);
            // 設定値のメモリ領域は不要になったので破棄
            free(value_str);
            return -1;
        }
        // UNIXドメインソケットファイルを設定
        memcpy((void *)EVS_config.domain_socketfile, (void *)value_str, strlen(value_str));
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): SocketFile=%s\n", __func__, EVS_config.domain_socketfile);
        logging(LOGLEVEL_INFO, log_str);
    }
    // ----------------
    // SSL/TLS対応設定なら
    // ----------------
    else if (strcmp("SSLTLS", key_str) == 0)
    {
        // 設定値の中に"ON"か'1'があれば
        if (strstr(value_str, "ON") != NULL || strstr(value_str, "On") != NULL || strstr(value_str, "on") != NULL || strchr(value_str, '1') != NULL)
        {
            // SSL/TLS対応を1:ONに設定
            EVS_config.ssl_support = 1;
            snprintf(log_str, MAX_LOG_LENGTH, "%s(): SSL/TLS=%d\n", __func__, EVS_config.ssl_support);
            logging(LOGLEVEL_INFO, log_str);
        }
        else
        {
            // SSL/TLS対応を0:OFFに設定
            EVS_config.ssl_support = 0;
            snprintf(log_str, MAX_LOG_LENGTH, "%s(): SSL/TLS=%d\n", __func__, EVS_config.ssl_support);
            logging(LOGLEVEL_INFO, log_str);
        }
    }
    // ----------------
    // SSL/TLS CAファイル設定なら
    // ----------------
    else if (strcmp("CA_FILE", key_str) == 0)
    {
        // 設定値文字列のメモリ領域を確保(+ 1バイトを忘れずに!!)
        EVS_config.ssl_ca_file = (char *)calloc(1, strlen(value_str) + 1);
        // メモリ領域が確保できなかったら
        if (EVS_config.ssl_ca_file == NULL)
        {
            snprintf(log_str, MAX_LOG_LENGTH, "%s(): Cannot calloc memory? errno=%d (%s)\n", __func__, errno, strerror(errno));
            logging(LOGLEVEL_ERROR, log_str);
            // 設定値名のメモリ領域は不要になったので破棄
            free(key_str);
            // 設定値のメモリ領域は不要になったので破棄
            free(value_str);
            return -1;
        }
        memcpy((void *)EVS_config.ssl_ca_file, (void *)value_str, strlen(value_str));
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): SSL/TLS CA_FILE=%s\n", __func__, EVS_config.ssl_ca_file); 
        logging(LOGLEVEL_INFO, log_str);
    }
    // ----------------
    // SSL/TLS CERTファイル設定なら
    // ----------------
    else if (strcmp("CERT_FILE", key_str) == 0)
    {
        // 設定値文字列のメモリ領域を確保(+ 1バイトを忘れずに!!)
        EVS_config.ssl_cert_file = (char *)calloc(1, strlen(value_str) + 1);
        // メモリ領域が確保できなかったら
        if (EVS_config.ssl_cert_file == NULL)
        {
            snprintf(log_str, MAX_LOG_LENGTH, "%s(): Cannot calloc memory? errno=%d (%s)\n", __func__, errno, strerror(errno));
            logging(LOGLEVEL_ERROR, log_str);
            // 設定値名のメモリ領域は不要になったので破棄
            free(key_str);
            // 設定値のメモリ領域は不要になったので破棄
            free(value_str);
            return -1;
        }
        memcpy((void *)EVS_config.ssl_cert_file, (void *)value_str, strlen(value_str));
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): SSL/TLS CERT_FILE=%s\n", __func__, EVS_config.ssl_cert_file); 
        logging(LOGLEVEL_INFO, log_str);
    }
    // ----------------
    // SSL/TLS KEYファイル設定なら
    // ----------------
    else if (strcmp("KEY_FILE", key_str) == 0)
    {
        // 設定値文字列のメモリ領域を確保(+ 1バイトを忘れずに!!)
        EVS_config.ssl_key_file = (char *)calloc(1, strlen(value_str) + 1);
        // メモリ領域が確保できなかったら
        if (EVS_config.ssl_key_file == NULL)
        {
            snprintf(log_str, MAX_LOG_LENGTH, "%s(): Cannot calloc memory? errno=%d (%s)\n", __func__, errno, strerror(errno));
            logging(LOGLEVEL_ERROR, log_str);
            // 設定値名のメモリ領域は不要になったので破棄
            free(key_str);
            // 設定値のメモリ領域は不要になったので破棄
            free(value_str);
            return -1;
        }
        memcpy((void *)EVS_config.ssl_key_file, (void *)value_str, strlen(value_str));
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): SSL/TLS KEY_FILE=%s\n", __func__, EVS_config.ssl_key_file); 
        logging(LOGLEVEL_INFO, log_str);
    }
    // ----------------
    // 無通信タイムアウトチェック(0:無効、1:有効)設定なら
    // ----------------
    else if (strcmp("NO_COMMUNICATION_CHECK", key_str) == 0)
    {
        // 設定値の中に"ON"か'1'があれば
        if (strstr(value_str, "ON") != NULL || strstr(value_str, "On") != NULL || strstr(value_str, "on") != NULL || strchr(value_str, '1') != NULL)
        {
            // 無通信タイムアウトチェックを1:ONに設定
            EVS_config.nocommunication_check = 1;
            snprintf(log_str, MAX_LOG_LENGTH, "%s(): No-Communication Check=%d\n", __func__, EVS_config.nocommunication_check);
            logging(LOGLEVEL_DEBUG, log_str);
        }
        else
        {
            // 無通信タイムアウトチェックを0:OFFに設定
            EVS_config.nocommunication_check = 0;
            snprintf(log_str, MAX_LOG_LENGTH, "%s(): No-Communication Check=%d\n", __func__, EVS_config.nocommunication_check);
            logging(LOGLEVEL_DEBUG, log_str);
        }
    }
    // ----------------
    // 無通信タイムアウト(秒)設定なら
    // ----------------
    else if (strcmp("NO_COMMUNICATION_TIMEOUT", key_str) == 0)
    {
        // 無通信タイムアウト(秒)を設定
        EVS_config.nocommunication_timeout = (ev_tstamp)atoi(value_str);
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): No-Communication Timeout=%f\n", __func__, (ev_tstamp)atoi(value_str));
        logging(LOGLEVEL_DEBUG, log_str);
    }
    // ----------------
    // タイマーイベント確認間隔(秒)設定なら
    // ----------------
    else if (strcmp("TIMER_CHECKINTERVAL", key_str) == 0)
    {
        // タイマーイベント確認間隔(秒)を設定
        EVS_config.timer_checkintval = (ev_tstamp)atoi(value_str);
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): Timer Check Interval=%f\n", __func__, (ev_tstamp)atoi(value_str));
        logging(LOGLEVEL_DEBUG, log_str);
    }
    // ----------------
    // TCP KeepAlive設定なら
    // ----------------
    else if (strcmp("KEEPALIVE", key_str) == 0)
    {
        // 設定値の中に"ON"か'1'があれば
        if (strstr(value_str, "ON") != NULL || strstr(value_str, "On") != NULL || strstr(value_str, "on") != NULL || strchr(value_str, '1') != NULL)
        {
            // KeepAliveを1:ONに設定
            EVS_config.keepalive = 1;
            snprintf(log_str, MAX_LOG_LENGTH, "%s(): KeepAlive=%d\n", __func__, EVS_config.keepalive);
            logging(LOGLEVEL_DEBUG, log_str);
        }
        else
        {
            // KeepAliveを1:ONに設定
            EVS_config.keepalive = 0;
            snprintf(log_str, MAX_LOG_LENGTH, "%s(): KeepAlive=%d\n", __func__, EVS_config.keepalive);
            logging(LOGLEVEL_DEBUG, log_str);
        }
    }
    // ----------------
    // TCP KeepAlive IdleTime設定なら
    // ----------------
    else if (strcmp("KEEPALIVE_IDLETIME", key_str) == 0)
    {
        // Keepalive Idletimeを設定
        EVS_config.keepalive_idletime = atoi(value_str);
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): Keepalive IdleTime=%d\n", __func__, atoi(value_str));
        logging(LOGLEVEL_DEBUG, log_str);
    }
    // ----------------
    // TCP KeepAlive Interval設定なら
    // ----------------
    else if (strcmp("KEEPALIVE_INTERVAL", key_str) == 0)
    {
        // Keepalive Intervalを設定
        EVS_config.keepalive_intval = atoi(value_str);
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): Keepalive Interval=%d\n", __func__, atoi(value_str));
        logging(LOGLEVEL_DEBUG, log_str);
    }
    // ----------------
    // TCP KeepAlive Probes設定なら
    // ----------------
    else if (strcmp("KEEPALIVE_PROBES", key_str) == 0)
    {
        // Keepalive Probesを設定
        EVS_config.keepalive_probes = atoi(value_str);
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): Keepalive Probes=%d\n", __func__, atoi(value_str));
        logging(LOGLEVEL_DEBUG, log_str);
    }
    // ----------------
    // 待ち受けポート設定なら
    // ----------------
    else if (strcmp("LISTEN", key_str) == 0)
    {
        // ポート別設定用構造体ポインタのメモリ領域を確保
        listen_port = (struct EVS_port_t *)calloc(1, sizeof(*listen_port));
        // メモリ領域が確保できなかったら
        if (listen_port == NULL)
        {
            snprintf(log_str, MAX_LOG_LENGTH, "%s(): Cannot calloc memory? errno=%d (%s)\n", __func__, errno, strerror(errno));
            logging(LOGLEVEL_ERROR, log_str);
            // 設定値名のメモリ領域は不要になったので破棄
            free(key_str);
            // 設定値のメモリ領域は不要になったので破棄
            free(value_str);
            return -1;
        }
        // 設定値を個別に変換、その2
        init_result = sscanf(value_str, "%[^,],%[^,],%s", value[0], value[1], value[2]);
        // 変換数が3ではないなら
        if (init_result != 3)
        {
            snprintf(log_str, MAX_LOG_LENGTH, "%s(): sscanf(): Cannot get value[]!? %d %s\n", __func__, init_result, value_str);
            logging(LOGLEVEL_ERROR, log_str);
            // 設定値名のメモリ領域は不要になったので破棄
            free(key_str);
            // 設定値のメモリ領域は不要になったので破棄
            free(value_str);
            return -1;
        }
        // 設定値1が数値変換できて、かつ1～65535なら
        if (atoi(value[0]) >= 1 && atoi(value[0]) <=65535)
        {
            listen_port->port = atoi(value[0]);                             // ポート番号を設定する
            snprintf(log_str, MAX_LOG_LENGTH, "%s(): LISTEN port=%d\n", __func__, listen_port->port); 
            logging(LOGLEVEL_INFO, log_str);
        }
        else
        {
            snprintf(log_str, MAX_LOG_LENGTH, "%s(): Cannot read LISTEN PORT=%s!?\n", __func__, value[0]);
            logging(LOGLEVEL_ERROR, log_str);
            // 設定値名のメモリ領域は不要になったので破棄
            free(key_str);
            // 設定値のメモリ領域は不要になったので破棄
            free(value_str);
            return -1;
        }
        // 設定値2の中に'4'があれば
        if (strchr(value[1], '4') != NULL)
        {
            listen_port->ipv4 = 1;                                          // IPv4フラグ(0:OFF、1:ON)に1(=ON)を設定する
            snprintf(log_str, MAX_LOG_LENGTH, "%s(): LISTEN IPv4=%d\n", __func__, listen_port->ipv4); 
            logging(LOGLEVEL_INFO, log_str);
        }
        else
        {
            listen_port->ipv4 = 0;                                          // IPv4フラグ(0:OFF、1:ON)に0(=OFF)を設定する
            snprintf(log_str, MAX_LOG_LENGTH, "%s(): LISTEN IPv4=%d\n", __func__, listen_port->ipv4); 
            logging(LOGLEVEL_INFO, log_str);
        }
        // 設定値2の中に'6'があれば
        if (strchr(value[1], '6') != NULL)
        {
            listen_port->ipv6 = 1;                                          // IPv6フラグ(0:OFF、1:ON)に1(=ON)を設定する
            snprintf(log_str, MAX_LOG_LENGTH, "%s(): LISTEN IPv6=%d\n", __func__, listen_port->ipv6); 
            logging(LOGLEVEL_INFO, log_str);
        }
        else
        {
            listen_port->ipv6 = 0;                                          // IPv6フラグ(0:OFF、1:ON)に0(=OFF)を設定する
            snprintf(log_str, MAX_LOG_LENGTH, "%s(): LISTEN IPv6=%d\n", __func__, listen_port->ipv6); 
            logging(LOGLEVEL_INFO, log_str);
        }
        // 設定値3の中に"ON"か'1'があれば
        if (strstr(value[2], "ON") != NULL || strstr(value[2], "On") != NULL || strstr(value[2], "on") != NULL || strchr(value[2], '1') != NULL)
        {
            listen_port->ssl = 1;                                           // SSL/TLSフラグ(0:OFF、1:ON)に1を設定する
            snprintf(log_str, MAX_LOG_LENGTH, "%s(): LISTEN SSL/TLS Support=%d\n", __func__, listen_port->ssl); 
            logging(LOGLEVEL_INFO, log_str);
        }
        else
        {
            listen_port->ssl = 0;                                           // SSL/TLSフラグ(0:OFF、1:ON)に0を設定する
            snprintf(log_str, MAX_LOG_LENGTH, "%s(): LISTEN SSL/TLS Support=%d\n", __func__, listen_port->ssl); 
            logging(LOGLEVEL_INFO, log_str);
        }
        // テールキューの最後に待ち受けポートの情報を追加する
        TAILQ_INSERT_TAIL(&EVS_port_tailq, listen_port, entries);
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): TAILQ_INSERT_TAIL(listen_port): OK.\n", __func__);
        logging(LOGLEVEL_INFO, log_str);
    }
    else
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): Not support parameter!? '%s'\n", __func__, target);
        logging(LOGLEVEL_ERROR, log_str);
        // 設定値名のメモリ領域は不要になったので破棄
        free(key_str);
        // 設定値のメモリ領域は不要になったので破棄
        free(value_str);
        return -1;
    }
    // 設定値名のメモリ領域は不要になったので破棄
    free(key_str);
    // 設定値のメモリ領域は不要になったので破棄
    free(value_str);
    return 0;
}

// --------------------------------
// 各種設定値デフォルト初期化処理
// --------------------------------
int INIT_config_default(void)
{
    int                             init_result;
    char                            log_str[MAX_LOG_LENGTH];

    // ----------------
    // デーモンモードを1:ONに設定
    // ----------------
    EVS_config.daemon = 1;
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): DAEMON=%d\n", __func__, EVS_config.daemon);
    logging(LOGLEVEL_DEBUG, log_str);

    // ----------------
    // PIDファイルを設定
    // ----------------
    char                            *pid_file = "/var/run/EvServer/.EvServer.pid";
    // 設定値文字列のメモリ領域を確保(+ 1バイトを忘れずに!!)
    EVS_config.pid_file = (char *)calloc(1, strlen(pid_file) + 1);
    // メモリ領域が確保できなかったら
    if (EVS_config.pid_file == NULL)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): Cannot calloc memory? errno=%d (%s)\n", __func__, errno, strerror(errno));
        logging(LOGLEVEL_ERROR, log_str);
        return -1;
    }
    // PIDファイルを設定
    memcpy((void *)EVS_config.pid_file, (void *)pid_file, strlen(pid_file));
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): PidFile=%s\n", __func__, EVS_config.pid_file);
    logging(LOGLEVEL_DEBUG, log_str);

    // ----------------
    // ログファイルを設定
    // ----------------
    char                            *log_file = "/var/log/EvServer/evserver.log";
    // 設定値文字列のメモリ領域を確保(+ 1バイトを忘れずに!!)
    EVS_config.log_file = (char *)calloc(1, strlen(log_file) + 1);
    // メモリ領域が確保できなかったら
    if (EVS_config.log_file == NULL)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): Cannot calloc memory? errno=%d (%s)\n", __func__, errno, strerror(errno));
        logging(LOGLEVEL_ERROR, log_str);
        return -1;
    }
    // ログファイルを設定
    memcpy((void *)EVS_config.log_file, (void *)log_file, strlen(log_file));
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): LogFile=%s\n", __func__, EVS_config.log_file);
    logging(LOGLEVEL_DEBUG, log_str);

    // ----------------
    // ログレベルを2:LOGLEVEL_INFOに設定
    // ----------------
    EVS_config.log_level = LOGLEVEL_INFO;
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): Log Level=%d\n", __func__, EVS_config.log_level);
    logging(LOGLEVEL_DEBUG, log_str);

    // ----------------
    // UNIXドメインソケットファイルを設定
    // ----------------
    char                            *domain_socketfile = "/var/run/EvServer/.EvServer.sock";
    // 設定値文字列のメモリ領域を確保(+ 1バイトを忘れずに!!)
    EVS_config.domain_socketfile = (char *)calloc(1, strlen(domain_socketfile) + 1);
    // メモリ領域が確保できなかったら
    if (EVS_config.domain_socketfile == NULL)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): Cannot calloc memory? errno=%d (%s)\n", __func__, errno, strerror(errno));
        logging(LOGLEVEL_ERROR, log_str);
        return -1;
    }
    // UNIXドメインソケットファイルを設定
    memcpy((void *)EVS_config.domain_socketfile, (void *)domain_socketfile, strlen(domain_socketfile));
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): SocketFile=%s\n", __func__, EVS_config.domain_socketfile);
    logging(LOGLEVEL_DEBUG, log_str);

    // ----------------
    // SSL/TLS対応を0:OFFに設定
    // ----------------
    EVS_config.ssl_support = 0;
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): SSL/TLS=%d\n", __func__, EVS_config.ssl_support);
    logging(LOGLEVEL_DEBUG, log_str);

    // ----------------
    // タイマーイベント確認間隔(秒)を60秒に設定
    // ----------------
    EVS_config.timer_checkintval = 60.;
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): Timeout Check Interval=%f\n", __func__, EVS_config.timer_checkintval);
    logging(LOGLEVEL_DEBUG, log_str);

    // ----------------
    // KeepAliveを1:ONに設定
    // ----------------
    EVS_config.keepalive = 1;
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): KeepAlive=%d\n", __func__, EVS_config.keepalive);
    logging(LOGLEVEL_DEBUG, log_str);

    // Keepalive Idletimeを設定
    EVS_config.keepalive_idletime = 180;
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): Keepalive IdleTime=%d\n", __func__, EVS_config.keepalive_idletime);
    logging(LOGLEVEL_DEBUG, log_str);

    // Keepalive Intervalを設定
    EVS_config.keepalive_intval = 30;
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): Keepalive Interval=%d\n", __func__, EVS_config.keepalive_intval);
    logging(LOGLEVEL_DEBUG, log_str);

    // Keepalive Probesを設定
    EVS_config.keepalive_probes = 5;
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): Keepalive Probes=%d\n", __func__, EVS_config.keepalive_probes);
    logging(LOGLEVEL_DEBUG, log_str);

    snprintf(log_str, MAX_LOG_LENGTH, "%s(): OK.\n", __func__);
    logging(LOGLEVEL_DEBUG, log_str);

    return 0;
}

// --------------------------------
// 各種設定値初期化処理
// --------------------------------
int INIT_config(int argc, char *argv[])
{
    int                             init_result;
    FILE                            *config_fp;                         // コンフィグファイルポインタ
    char                            config_data[MAX_STRING_LENGTH];     // コンフィグファイル読み込みデータ
    char                            log_str[MAX_LOG_LENGTH];

    // ----------------
    // システム設定値を0クリア
    // ----------------
    memset((void *)&EVS_config, 0, sizeof(struct EVS_config_t));
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): Clear config OK.\n", __func__);
    logging(LOGLEVEL_INFO, log_str);

    // 各種設定値デフォルト初期化処理
    init_result = INIT_config_default();
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): Load default config OK.\n", __func__);
    logging(LOGLEVEL_INFO, log_str);

    // ----------------
    // 引数が設定ファイルとして存在しているかどうか確認
    // ----------------
    config_fp = fopen(argv[1], "r");
    if (config_fp == NULL)
    {
        snprintf(log_str, MAX_LOG_LENGTH, "%s(): fopen(%s): Cannot open config file!? use default settings.\n", __func__, argv[1]);
        logging(LOGLEVEL_INFO, log_str);
        return init_result;
    }
    snprintf(log_str, MAX_LOG_LENGTH, "%s(): fopen(%s): OK.\n", __func__, argv[1]);
    logging(LOGLEVEL_INFO, log_str);

    // ----------------
    // 設定ファイルからパラメータ読み込み
    // ----------------
    // コンフィグファイルから一行読み込み…ができる限りループ
    while (fgets(config_data, sizeof(config_data), config_fp) != NULL)
    {
        // データの先頭が'#'や' 'ではないなら
        if (config_data[0] != '#' && config_data[0] != ';' && config_data[0] != '/' && config_data[0] != ' ' && strlen(config_data) >= 3)
        {
            // 設定値読み込み
            init_result = config_str_convert(config_data, strlen(config_data));
            if (init_result < 0)
            {
                snprintf(log_str, MAX_LOG_LENGTH, "%s(): config_str_convert(): Cannot convert string to parameter!?\n", __func__);
                logging(LOGLEVEL_ERROR, log_str);
                return init_result;
            }
        }
    }

    // コンフィグファイルポインタを閉じる
    fclose(config_fp);

    return 0;
}
