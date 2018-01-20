#ifndef WORKER_H
#define WORKER_H

class config;
class mysql;
class site_comm;
struct torrent;
struct client_opts_t;

#include <string>
#include <list>
#include <vector>
#include <mutex>
#include <memory>
#include <unordered_map>
#include <ctime>

#include <boost/optional.hpp>
#include <spdlog/logger.h>

#include "torrent.h"
#include "params.h"
#include "user.h"
#include "peer.h"

enum tracker_status { OPEN, PAUSED, CLOSING }; // tracker status

class worker {
	private:
		struct del_message {
			enum class reason {
				DUPE, 
				TRUMP, 
				BAD_FILE_NAMES, 
				BAD_FOLDER_NAMES, 
				BAD_TAGS, 
				BAD_FORMAT, 
				DISCS_MISSING, 
				DISCOGRAPHY,
				EDITED_LOG,
				INACCURATE_BITRATE, 
				LOW_BITRATE, 
				MUTT_RIP,
				BAD_SOURCE,
				ENCODE_ERRORS,
				BANNED, 
				TRACKS_MISSING,
				TRANSCODE, 
				CASSETTE, 
				UNSPLIT_ALBUM, 
				USER_COMPILATION, 
				WRONG_FORMAT, 
				WRONG_MEDIA, 
				AUDIENCE,
				reason_max_
			};
			time_t m_time;
			boost::optional<reason> m_reason;
			std::string del_reason() const;
			del_message(const params_type &params);
		};
		config * conf;
		mysql * db;
		site_comm * s_comm;
		torrent_list &torrents_list;
		user_list &users_list;
		std::vector<std::string> &whitelist;
		std::unordered_map<std::string, del_message> del_reasons;
		tracker_status status;
		bool reaper_active;
		time_t cur_time;
		std::shared_ptr<spdlog::logger> logger;

		unsigned int announce_interval;
		unsigned int del_reason_lifetime;
		unsigned int peers_timeout;
		unsigned int numwant_limit;
		bool keepalive_enabled;
		std::string site_password;
		std::string report_password;

		std::mutex del_reasons_lock;
		void load_config(config * conf);
		void do_start_reaper();
		void reap_peers();
		void reap_del_reasons();
		peer_list::iterator add_peer(peer_list &peer_list, const std::string &peer_id);
		inline bool peer_is_visible(user_ptr &u, peer *p);

	public:
		worker(config * conf_obj, torrent_list &torrents, user_list &users, std::vector<std::string> &_whitelist, mysql * db_obj, site_comm * sc);
		void reload_config(config * conf);
		std::string work(const std::string &input, std::string &ip, client_opts_t &client_opts);
		std::string announce(const std::string &input, torrent &tor, user_ptr &u, params_type &params, params_type &headers, std::string &ip, client_opts_t &client_opts);
		std::string scrape(const std::list<std::string> &infohashes, params_type &headers, client_opts_t &client_opts);
		std::string update(params_type &params, client_opts_t &client_opts);

		void reload_lists();
		bool shutdown();

		tracker_status get_status() { return status; }

		void start_reaper();
};
#endif
