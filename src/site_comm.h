#ifndef SRC_SITE_COMM_H_
#define SRC_SITE_COMM_H_

// Copyright [2017-2024] Orpheus

#include <spdlog/spdlog.h>

#include <string>
#include <queue>
#include <memory>
#include <mutex>

#include <boost/asio.hpp>

#include "config.h"

using boost::asio::ip::tcp;

class site_comm {
 private:
    std::string site_host;
    std::string site_service;
    std::string site_path;
    std::string site_password;
    std::mutex expire_queue_lock;
    std::string expire_token_buffer;
    std::queue<std::string> token_queue;
    std::shared_ptr<spdlog::logger> logger;
    bool readonly;
    bool t_active;
    void load_config(config * conf);
    void do_flush_tokens();

 public:
    bool verbose_flush;
    site_comm(config * conf);
    void reload_config(config * conf);
    bool all_clear();
    void expire_token(int torrent, int user);
    void flush_tokens();
    ~site_comm();
};

#endif  // SRC_SITE_COMM_H_
