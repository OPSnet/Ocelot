#include <iostream>
#include <map>
#include <sstream>
#include "ocelot.h"
#include "misc_functions.h"
#include "report.h"
#include "response.h"
#include "user.h"

std::string report(params_type &params, user_list &users_list, client_opts_t &client_opts) {
	std::stringstream output;
	std::string action = params["get"];
	if (action.empty()) {
		output << "Invalid action\n";
	} else if (action == "stats") {
		time_t uptime = time(NULL) - stats.start_time;
		int up_d = uptime / 86400;
		uptime -= up_d * 86400;
		int up_h = uptime / 3600;
		uptime -= up_h * 3600;
		int up_m = uptime / 60;
		int up_s = uptime - up_m * 60;
		std::string up_ht = up_h <= 9 ? '0' + inttostr(up_h) : inttostr(up_h);
		std::string up_mt = up_m <= 9 ? '0' + inttostr(up_m) : inttostr(up_m);
		std::string up_st = up_s <= 9 ? '0' + inttostr(up_s) : inttostr(up_s);

		output << "Uptime: " << up_d << " days, " << up_ht << ':' << up_mt << ':' << up_st << '\n'
			   << stats.opened_connections << " connections opened\n"
			   << stats.open_connections << " open connections\n"
			   << stats.connection_rate << " connections/s\n"
			   << stats.requests << " requests handled\n"
			   << stats.request_rate << " requests/s\n"
			   << stats.succ_announcements << " successful announcements\n"
			   << (stats.announcements - stats.succ_announcements) << " failed announcements\n"
			   << stats.scrapes << " scrapes\n"
			   << stats.leechers << " leechers tracked\n"
			   << stats.seeders << " seeders tracked\n"
			   << stats.bytes_read << " bytes read\n"
			   << stats.bytes_written << " bytes written\n";
	} else if (action == "prom_stats") {
		time_t uptime = time(NULL) - stats.start_time;
		output << "ocelot_uptime " << uptime << "\n"
			   << "ocelot_open_connections " << stats.open_connections << "\n"
			   << "ocelot_connection_rate " << stats.connection_rate << "\n"
			   << "ocelot_requests " << stats.requests << "\n"
			   << "ocelot_request_rate " << stats.request_rate << "\n"
			   << "ocelot_succ_announcements " << stats.succ_announcements << "\n"
			   << "ocelot_total_announcements " << stats.announcements << "\n"
			   << "ocelot_scrapes " << stats.scrapes << "\n"
			   << "ocelot_leechers " << stats.leechers << "\n"
			   << "ocelot_seeders " << stats.seeders << "\n"
			   << "ocelot_bytes_read " << stats.bytes_read << "\n"
			   << "ocelot_bytes_written " << stats.bytes_written << "\n#";
	} else if (action == "user") {
		std::string key = params["key"];
		if (key.empty()) {
			output << "Invalid action\n";
		} else {
			user_list::const_iterator u = users_list.find(key);
			if (u != users_list.end()) {
				output << u->second->get_leeching() << " leeching" << std::endl
					   << u->second->get_seeding() << " seeding" << std::endl;
			}
		}
	} else {
		output << "Invalid action\n";
	}
	output << "success";
	return response(output.str(), client_opts);
}
