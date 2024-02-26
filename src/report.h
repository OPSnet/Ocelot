#ifndef SRC_REPORT_H_
#define SRC_REPORT_H_

// Copyright [2017-2024] Orpheus

#include <string>

// take a snapshot of the current stats
void copy_stats(const struct stats_t *in, struct stats_t *out);

// status report
std::string report(const uint32_t announce_interval, const uint32_t announce_jitter);

// a snapshot of jemalloc statistics
std::string report_jemalloc_plain(const char *opts, const std::string path);

// a prometheus scrape
std::string report_prom_stats(const char *jemalloc_stats);

// user report
std::string report_user(const user_ptr u);

#endif  // SRC_REPORT_H_
