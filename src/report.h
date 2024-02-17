#ifndef SRC_REPORT_H_
#define SRC_REPORT_H_

// Copyright [2017-2024] Orpheus

#include <string>

std::string report(
    params_type &params,
    user_list &users_list,
    unsigned int announce_interval,
    unsigned int announce_jitter
);

// return a snapshot of jemalloc statistics
std::string report_jemalloc_plain(const char *opts, std::string path);

// return output for a prometheus scrape
std::string report_prom_stats(const char *jemalloc_stats);

#endif  // SRC_REPORT_H_
