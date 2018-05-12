#ifndef REPORT_H
#define REPORT_H

#include <string>
#include "user.h"
#include "params.h"

struct client_opts_t;

std::string report(params_type &params, user_list &users_list, client_opts_t &client_opts);

#endif
