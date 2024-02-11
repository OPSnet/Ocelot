#include <iostream>
#include <map>
#include <sstream>
#include "ocelot.h"
#include "misc_functions.h"
#include "report.h"
#include "response.h"
#include "user.h"

std::string report(params_type &params, user_list &users_list, client_opts_t &client_opts, unsigned int announce_interval, unsigned int announce_jitter) {
	std::stringstream output;
	std::string action = params["get"];
	if (action == "stats") {
		time_t uptime = time(NULL) - stats.start_time;
		int up_d = uptime / 86400;
		uptime -= up_d * 86400;
		int up_h = uptime / 3600;
		uptime -= up_h * 3600;
		int up_m = uptime / 60;
		int up_s = uptime - up_m * 60;

		output << "Uptime: " << up_d << " days, "
				<< (up_h < 10 ? "0" : "") << inttostr(up_h) << ':'
				<< (up_m < 10 ? "0" : "") << inttostr(up_m) << ':'
				<< (up_s < 10 ? "0" : "") << inttostr(up_s) << "\n"
			<< "version: " << version() << "\n"
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
			<< stats.user_queue_size << " items in user queue\n"
			<< stats.torrent_queue_size << " items in torrent queue\n"
			<< stats.peer_queue_size << " items in peer queue\n"
			<< stats.snatch_queue_size << " items in snatch queue\n"
			<< stats.token_queue_size << " items in token queue\n"
			<< stats.bytes_read << " bytes read\n"
			<< stats.bytes_written << " bytes written\n"
			<< announce_interval << " announce interval\n"
			<< announce_jitter << " announce jitter\n"
			;
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
			<< "ocelot_user_queue " << stats.user_queue_size << "\n"
			<< "ocelot_torrent_queue " << stats.torrent_queue_size << "\n"
			<< "ocelot_peer_queue " << stats.peer_queue_size << "\n"
			<< "ocelot_snatch_queue " << stats.snatch_queue_size << "\n"
			<< "ocelot_token_queue " << stats.token_queue_size << "\n"
			<< "ocelot_bytes_read " << stats.bytes_read << "\n"
			<< "ocelot_bytes_written " << stats.bytes_written << "\n#";
	} else if (action == "user") {
		std::string key = params["key"];
		if (key.empty()) {
			output << "Invalid action\n";
		} else {
			user_list::const_iterator u = users_list.find(key);
			if (u != users_list.end()) {
				output << "{\"id\":" << u->second->get_id()
					<< ",\"leeching\":" << u->second->get_leeching()
					<< ",\"seeding\":" << u->second->get_seeding()
					<< ",\"deleted\":" << u->second->is_deleted()
					<< ",\"protected\":" << u->second->is_protected()
					<< ",\"can_leech\":" << u->second->can_leech()
					<< "}" << "\n";
				return response(output.str(), client_opts);
			}
		}
	} else {
		output << "Invalid action\n";
	}
	output << "success";
	return response(output.str(), client_opts);
}
