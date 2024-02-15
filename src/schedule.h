#ifndef SRC_SCHEDULE_H_
#define SRC_SCHEDULE_H_

// Copyright [2017-2024] Orpheus

#ifdef EV_ERROR
#undef EV_ERROR
#endif

#include <ev++.h>

class schedule {
 private:
    void load_config(config * conf);

    unsigned int reap_peers_interval;
    worker * work;
    mysql * db;
    site_comm * sc;
    uint64_t last_opened_connections;
    uint64_t last_request_count;
    unsigned int counter;
    int next_reap_peers;
    std::shared_ptr<spdlog::logger> logger;

 public:
    schedule(config * conf, worker * worker_obj, mysql * db_obj, site_comm * sc_obj);
    void reload_config(config * conf);
    void handle(ev::timer &watcher, int events_flags);

    unsigned int schedule_interval;
};

#endif  // SRC_SCHEDULE_H_
