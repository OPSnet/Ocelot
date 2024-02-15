#ifndef SRC_REPORT_H_
#define SRC_REPORT_H_

// Copyright [2017-2024] Orpheus

#include <string>

std::string report(
    params_type &params,
    user_list &users_list,
    client_opts_t &client_opts,
    unsigned int announce_interval,
    unsigned int announce_jitter
);

#endif  // SRC_REPORT_H_
