#include "ocelot.h"
#include "db.h"
#include "user.h"
#include "misc_functions.h"
#include "config.h"
#include <string>
#include <iostream>
#include <queue>
#include <unistd.h>
#include <ctime>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <spdlog/spdlog.h>

#define DB_LOCK_TIMEOUT 50

mysql::mysql(config * conf) : update_user_buffer(*this), update_torrent_buffer(*this),
		update_heavy_peer_buffer(*this), update_light_peer_buffer(*this),
		update_snatch_buffer(*this), update_token_buffer(*this),
		u_active(false), t_active(false), p_active(false), s_active(false), tok_active(false) {
	logger = spdlog::get("logger");
	load_config(conf);
	if (mysql_db.empty()) {
		logger->error("No database selected");
		return;
	}

	try {
		mysqlpp::ReconnectOption reconnect(true);
		conn.set_option(&reconnect);
		conn.connect(mysql_db.c_str(), mysql_host.c_str(), mysql_username.c_str(), mysql_password.c_str(), 0);
	} catch (const mysqlpp::Exception &er) {
		logger->error("Failed to connect to MySQL ({})", er.what());
		return;
	}

	if (!readonly) {
		logger->info("Clearing xbt_files_users and resetting peer counts...");
		logger->flush();
		clear_peer_data();
		logger->info("done");
	}
}

void mysql::load_config(config * conf) {
	mysql_db = conf->get_str("mysql_db");
	mysql_host = conf->get_str("mysql_host");
	mysql_username = conf->get_str("mysql_username");
	mysql_password = conf->get_str("mysql_password");
	readonly = conf->get_bool("readonly");
}

void mysql::reload_config(config * conf) {
	load_config(conf);
}

bool mysql::connected() {
	return conn.connected();
}

void mysql::clear_peer_data() {
	try {
		mysqlpp::Query query = conn.query("TRUNCATE xbt_files_users;");
		if (!query.exec()) {
			logger->error("Unable to truncate xbt_files_users!");
		}
		query = conn.query("UPDATE torrents_leech_stats SET Seeders = 0, Leechers = 0;");
		if (!query.exec()) {
			logger->error("Unable to reset seeder and leecher count!");
		}
	} catch (const mysqlpp::Exception &er) {
		logger->error("Query error in clear_peer_data: {}", er.what());
	}
}

void mysql::load_torrents(torrent_list &torrents) {
	mysqlpp::Query query = conn.query("SELECT t.ID, t.info_hash, t.freetorrent, tls.Snatched FROM torrents t INNER JOIN torrents_leech_stats tls ON (tls.TorrentID = t.ID) ORDER BY t.ID");
	try {
		mysqlpp::StoreQueryResult res = query.store();
		std::unordered_set<std::string> cur_keys;
		size_t num_rows = res.num_rows();
		std::lock_guard<std::mutex> tl_lock(torrent_list_mutex);
		if (torrents.size() == 0) {
			torrents.reserve(num_rows * 1.05); // Reserve 5% extra space to prevent rehashing
		} else {
			// Create set with all currently known info hashes to remove nonexistent ones later
			cur_keys.reserve(torrents.size());
			for (auto const &it: torrents) {
				cur_keys.insert(it.first);
			}
		}
		for (size_t i = 0; i < num_rows; i++) {
			std::string info_hash;
			res[i][1].to_string(info_hash);
			if (info_hash.empty()) {
				continue;
			}
			mysqlpp::sql_enum free_torrent(res[i][2]);

			torrent tmp_tor;
			auto it = torrents.insert(std::pair<std::string, torrent>(info_hash, tmp_tor));
			torrent &tor = (it.first)->second;
			if (it.second) {
				tor.id = res[i][0];
				tor.balance = 0;
				tor.completed = res[i][3];
				tor.last_selected_seeder.clear();
			} else {
				tor.tokened_users.clear();
				cur_keys.erase(info_hash);
			}
			if (free_torrent == "1") {
				tor.free_torrent = FREE;
			} else if (free_torrent == "2") {
				tor.free_torrent = NEUTRAL;
			} else {
				tor.free_torrent = NORMAL;
			}
		}
		for (auto const &info_hash: cur_keys) {
			// Remove tracked torrents that weren't found in the database
			auto it = torrents.find(info_hash);
			if (it != torrents.end()) {
				torrent &tor = it->second;
				stats.leechers -= tor.leechers.size();
				stats.seeders -= tor.seeders.size();
				for (auto &p: tor.leechers) {
					p.second.user->decr_leeching();
				}
				for (auto &p: tor.seeders) {
					p.second.user->decr_seeding();
				}
				torrents.erase(it);
			}
		}
	} catch (const mysqlpp::BadQuery &er) {
		logger->error("Query error in load_torrents: {}", er.what());
		return;
	}
	logger->info("Loaded {} torrents", torrents.size());
	load_tokens(torrents);
}

void mysql::load_users(user_list &users) {
	mysqlpp::Query query = conn.query("SELECT ID, can_leech, torrent_pass, (Visible='0' OR IP='127.0.0.1') AS Protected FROM users_main WHERE Enabled='1';");
	try {
		mysqlpp::StoreQueryResult res = query.store();
		size_t num_rows = res.num_rows();
		std::unordered_set<std::string> cur_keys;
		std::lock_guard<std::mutex> ul_lock(user_list_mutex);
		if (users.size() == 0) {
			users.reserve(static_cast<unsigned long>(num_rows * 1.05)); // Reserve 5% extra space to prevent rehashing
		} else {
			// Create set with all currently known user keys to remove nonexistent ones later
			cur_keys.reserve(users.size());
			for (auto const &it: users) {
				cur_keys.insert(it.first);
			}
		}
		for (size_t i = 0; i < num_rows; i++) {
			std::string passkey(res[i][2]);
			bool protect_ip = res[i][3];
			user_ptr tmp_user = std::make_shared<user>(res[i][0], res[i][1], protect_ip);
			auto it = users.insert(std::pair<std::string, user_ptr>(passkey, tmp_user));
			if (!it.second) {
				user_ptr &u = (it.first)->second;
				u->set_leechstatus(res[i][1]);
				u->set_protected(protect_ip);
				u->set_deleted(false);
				cur_keys.erase(passkey);
			}
		}
		for (auto const &passkey: cur_keys) {
			// Remove users that weren't found in the database
			auto it = users.find(passkey);
			if (it != users.end()) {
				it->second->set_deleted(true);
				users.erase(it);
			}
		}
	} catch (const mysqlpp::BadQuery &er) {
		logger->error("Query error in load_users: {}", er.what());
		return;
	}
	logger->info("Loaded {} users", users.size());
}

void mysql::load_tokens(torrent_list &torrents) {
	mysqlpp::Query query = conn.query("SELECT uf.UserID, t.info_hash FROM users_freeleeches AS uf INNER JOIN torrents AS t ON t.ID = uf.TorrentID WHERE uf.Expired = '0';");
	int token_count = 0;
	try {
		mysqlpp::StoreQueryResult res = query.store();
		size_t num_rows = res.num_rows();
		std::lock_guard<std::mutex> tl_lock(torrent_list_mutex);
		for (size_t i = 0; i < num_rows; i++) {
			std::string info_hash;
			res[i][1].to_string(info_hash);
			auto it = torrents.find(info_hash);
			if (it != torrents.end()) {
				torrent &tor = it->second;
				tor.tokened_users.insert(res[i][0]);
				++token_count;
			}
		}
	} catch (const mysqlpp::BadQuery &er) {
		logger->error("Query error in load_tokens: {}", er.what());
		return;
	}
	logger->info("Loaded {} tokens", token_count);
}


void mysql::load_whitelist(std::vector<std::string> &whitelist) {
	mysqlpp::Query query = conn.query("SELECT peer_id FROM xbt_client_whitelist;");
	try {
		mysqlpp::StoreQueryResult res = query.store();
		size_t num_rows = res.num_rows();
		std::lock_guard<std::mutex> wl_lock(whitelist_mutex);
		whitelist.clear();
		for (size_t i = 0; i<num_rows; i++) {
			std::string peer_id;
			res[i][0].to_string(peer_id);
			whitelist.push_back(peer_id);
		}
	} catch (const mysqlpp::BadQuery &er) {
		logger->error("Query error in load_whitelist: {}", er.what());
		return;
	}
	if (whitelist.size() == 0) {
		logger->info("Assuming no whitelist desired, disabling");
	} else {
		logger->info("Loaded {} clients into the whitelist", whitelist.size());
	}
}

void mysql::record_token(const std::string &record) {
	update_token_buffer += record;
}

void mysql::record_user(const std::string &record) {
	update_user_buffer += record;
}

void mysql::record_torrent(const std::string &record) {
	std::lock_guard<std::mutex> tb_lock(torrent_buffer_lock);
	update_torrent_buffer += record;
}

void mysql::record_peer(const std::string &record, const std::string &ip, const std::string &peer_id, const std::string &useragent) {
	mysqlpp::Query q = conn.query();
	q << record << mysqlpp::quote << ip << ',' << mysqlpp::quote << peer_id << ',' << mysqlpp::quote << useragent << "," << time(NULL) << ')';

	update_heavy_peer_buffer += q.str();
}
void mysql::record_peer(const std::string &record, const std::string &peer_id) {
	mysqlpp::Query q = conn.query();
	q << record << mysqlpp::quote << peer_id << ',' << time(NULL) << ')';

	update_light_peer_buffer += q.str();
}

void mysql::record_snatch(const std::string &record, const std::string &ip) {
	mysqlpp::Query q = conn.query();
	q << record << ',' << mysqlpp::quote << ip << ')';
	update_snatch_buffer += q.str();
}

bool mysql::all_clear() {
	return (user_queue.size() == 0 && torrent_queue.size() == 0 && peer_queue.size() == 0 && snatch_queue.size() == 0 && token_queue.size() == 0);
}

void mysql::flush() {
	flush_users();
	flush_torrents();
	flush_snatches();
	flush_peers();
	flush_tokens();
}

void mysql::flush_users() {
	std::string sql;
	std::lock_guard<std::mutex> uq_lock(user_queue_lock);
	size_t qsize = user_queue.size();
	if (verbose_flush || qsize > 0) {
		logger->info("User flush queue size: {}, next query length: {}", qsize, user_queue.front().size());
	}
	if (update_user_buffer.empty()) {
		return;
	}
	sql = "INSERT INTO users_leech_stats (UserID, Uploaded, Downloaded) VALUES " + update_user_buffer.str() +
		" ON DUPLICATE KEY UPDATE Uploaded = Uploaded + VALUES(Uploaded), Downloaded = Downloaded + VALUES(Downloaded)";
	user_queue.push(sql);
	update_user_buffer.clear();
	if (!u_active) {
		std::thread thread([this]() {
			do_flush(user_queue, user_queue_lock, u_active);
		});
		thread.detach();
	}
}

void mysql::flush_torrents() {
	std::lock_guard<std::mutex> tb_lock(torrent_buffer_lock);
	std::string sql;
	std::lock_guard<std::mutex> tq_lock(torrent_queue_lock);
	size_t qsize = torrent_queue.size();
	if (verbose_flush || qsize > 0) {
		logger->info("Torrent flush queue size: {}, next query length: {}", qsize, torrent_queue.front().size());
	}
	if (update_torrent_buffer.empty()) {
		return;
	}
	sql = "INSERT INTO torrents_leech_stats (TorrentID,Seeders,Leechers,Snatched,Balance) VALUES " + update_torrent_buffer.str() +
		" ON DUPLICATE KEY UPDATE Seeders=VALUES(Seeders), Leechers=VALUES(Leechers), " +
		"Snatched=Snatched+VALUES(Snatched), Balance=VALUES(Balance), last_action = " +
		"IF(VALUES(Seeders) > 0, NOW(), last_action)";
	torrent_queue.push(sql);
	update_torrent_buffer.clear();
	sql.clear();
	sql = "DELETE FROM torrents WHERE info_hash = ''";
	torrent_queue.push(sql);
	if (!t_active) {
		std::thread thread([this]() {
			do_flush(torrent_queue, torrent_queue_lock, t_active);
		});
		thread.detach();
	}
}

void mysql::flush_snatches() {
	std::string sql;
	std::lock_guard<std::mutex> sq_lock(snatch_queue_lock);
	size_t qsize = snatch_queue.size();
	if (verbose_flush || qsize > 0) {
		logger->info("Snatch flush queue size: {}, next query length: {}", qsize, snatch_queue.front().size());
	}
	if (update_snatch_buffer.empty()) {
		return;
	}
	sql = "INSERT INTO xbt_snatched (uid, fid, tstamp, IP) VALUES " + update_snatch_buffer.str();
	snatch_queue.push(sql);
	update_snatch_buffer.clear();
	if (!s_active) {
		std::thread thread([this]() {
			do_flush(snatch_queue, snatch_queue_lock, s_active);
		});
		thread.detach();
	}
}

void mysql::flush_peers() {
	std::string sql;
	std::lock_guard<std::mutex> pq_lock(peer_queue_lock);
	size_t qsize = peer_queue.size();
	if (verbose_flush || qsize > 0) {
		logger->info("Peer flush queue size: {}, next query length: {}", qsize, peer_queue.front().size());
	}

	// Nothing to do
	if (update_light_peer_buffer.empty() && update_heavy_peer_buffer.empty()) {
		return;
	}

	if (!update_heavy_peer_buffer.empty()) {
		// Because xfu inserts are slow and ram is not infinite we need to
		// limit this queue's size
		// xfu will be messed up if the light query inserts a new row,
		// but that's better than an oom crash
		if (qsize >= 1000) {
			peer_queue.pop();
		}
		sql = "INSERT INTO xbt_files_users (uid,fid,active,uploaded,downloaded,upspeed,downspeed,remaining,corrupt," +
			std::string("timespent,announced,ip,peer_id,useragent,mtime) VALUES ") + update_heavy_peer_buffer.str() +
					" ON DUPLICATE KEY UPDATE active=VALUES(active), uploaded=VALUES(uploaded), " +
					"downloaded=VALUES(downloaded), upspeed=VALUES(upspeed), " +
					"downspeed=VALUES(downspeed), remaining=VALUES(remaining), " +
					"corrupt=VALUES(corrupt), timespent=VALUES(timespent), " +
					"announced=VALUES(announced), mtime=VALUES(mtime)";
		peer_queue.push(sql);
		update_heavy_peer_buffer.clear();
		sql.clear();
	}
	if (!update_light_peer_buffer.empty()) {
		// See comment above
		if (qsize >= 1000) {
			peer_queue.pop();
		}
		sql = "INSERT INTO xbt_files_users (uid,fid,timespent,announced,peer_id,mtime) VALUES " +
					update_light_peer_buffer.str() +
					" ON DUPLICATE KEY UPDATE upspeed=0, downspeed=0, timespent=VALUES(timespent), " +
					"announced=VALUES(announced), mtime=VALUES(mtime)";
		peer_queue.push(sql);
		update_light_peer_buffer.clear();
		sql.clear();
	}

	if (!p_active) {
		std::thread thread([this]() {
			do_flush(peer_queue, peer_queue_lock, p_active);
		});
		thread.detach();
	}
}

void mysql::flush_tokens() {
	std::string sql;
	std::lock_guard<std::mutex> tq_lock(token_queue_lock);
	size_t qsize = token_queue.size();
	if (verbose_flush || qsize > 0) {
		logger->info("Token flush queue size: {}, next query length: {}", qsize, token_queue.front().size());
	}
	if (update_token_buffer.empty()) {
		return;
	}
	sql = "INSERT INTO users_freeleeches (UserID, TorrentID, Downloaded) VALUES " + update_token_buffer.str() +
		" ON DUPLICATE KEY UPDATE Downloaded = Downloaded + VALUES(Downloaded)";
	token_queue.push(sql);
	update_token_buffer.clear();
	if (!tok_active) {
		std::thread thread([this]() {
			do_flush(token_queue, token_queue_lock, tok_active);
		});
		thread.detach();
	}
}

void mysql::do_flush(std::queue<std::string> &queue, std::mutex &mtx, bool &active) {
	active = true;
	try {
		mysqlpp::Connection c(mysql_db.c_str(), mysql_host.c_str(), mysql_username.c_str(), mysql_password.c_str(), 0);
		while (queue.size() > 0) {
			try {
				std::string sql = queue.front();
				mysqlpp::Query query = c.query(sql);
				if (!query.exec()) {
					logger->info("Token flush failed ({} remain)", queue.size());
					sleep(3);
					continue;
				} else {
					std::lock_guard<std::mutex> lck(mtx);
					queue.pop();
				}
			} catch (const mysqlpp::Exception &er) {
				logger->error("Query error: {} in flush tokens with a qlength: {}, queue size: {}"
							  , er.what(), queue.front().size(), queue.size());
				sleep(3);
				continue;
			}
		}
	}
	catch (const mysqlpp::Exception &er) {
		logger->error("MySQL error in flush_tokens: {}", er.what());
	}
	active = false;
}
