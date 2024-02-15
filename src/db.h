#ifndef SRC_DB_H_
#define SRC_DB_H_

// Copyright [2017-2024] Orpheus

#pragma GCC visibility push(default)

#include <mysql++/mysql++.h>
#include <spdlog/spdlog.h>
#include <string>
#include <unordered_map>
#include <queue>
#include <memory>
#include <vector>
#include "config.h"

class mysql {
 private:
    mysqlpp::Connection conn;
    std::string update_user_buffer;
    std::string update_torrent_buffer;
    std::string update_heavy_peer_buffer;
    std::string update_light_peer_buffer;
    std::string update_snatch_buffer;
    std::string update_token_buffer;

    std::queue<std::string> user_queue;
    std::queue<std::string> torrent_queue;
    std::queue<std::string> peer_queue;
    std::queue<std::string> snatch_queue;
    std::queue<std::string> token_queue;

    std::string mysql_db, mysql_host, mysql_username, mysql_password;
    unsigned int mysql_port;
    bool u_active, t_active, p_active, s_active, tok_active;
    bool readonly;

    // These locks prevent more than one thread from reading/writing the buffers.
    // These should be held for the minimum time possible.
    std::mutex user_queue_lock;
    std::mutex torrent_buffer_lock;
    std::mutex torrent_queue_lock;
    std::mutex peer_queue_lock;
    std::mutex snatch_queue_lock;
    std::mutex token_queue_lock;

    std::shared_ptr<spdlog::logger> logger;

    void load_config(config * conf);
    void load_tokens(torrent_list &torrents);
    mysqlpp::Connection create_connection();

    void do_flush_users();
    void do_flush_torrents();
    void do_flush_snatches();
    void do_flush_peers();
    void do_flush_tokens();

    void flush_users();
    void flush_torrents();
    void flush_snatches();
    void flush_peers();
    void flush_tokens();
    void clear_peer_data();

 public:
    bool verbose_flush;

    mysql(config * conf);
    void reload_config(config * conf);
    void flush();
    bool connected();
    bool all_clear();
    void load_torrents(torrent_list &torrents);
    void load_users(user_list &users);
    void load_whitelist(std::vector<std::string> &whitelist);

    // (id,uploaded_change,downloaded_change)
    void record_user(const std::string &record);

    // (id,seeders,leechers,snatched_change,balance)
    void record_torrent(const std::string &record);

    // (uid,fid,tstamp)
    void record_snatch(const std::string &record, const std::string &ip);

    // (uid,fid,active,peerid,useragent,ip,uploaded,downloaded,upspeed,downspeed,left,timespent,announces,tstamp)
    void record_peer(const std::string &record, const std::string &ip, const std::string &peer_id, const std::string &useragent);

    // (fid,peerid,timespent,announces,tstamp)
    void record_peer(const std::string &record, const std::string &peer_id);

    void record_token(const std::string &record);

    std::mutex torrent_list_mutex;
    std::mutex user_list_mutex;
    std::mutex whitelist_mutex;
};

#pragma GCC visibility pop

#endif  // SRC_DB_H_
