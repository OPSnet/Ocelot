#ifndef TORRENT_H
#define TORRENT_H

#include <set>
#include <string>
#include <cstdint>
#include <unordered_map>

#include "user.h"
#include "peer.h"

typedef uint32_t torid_t;

enum freetype { NORMAL, FREE, NEUTRAL };

struct torrent {
	torid_t id;
	uint32_t completed;
	int64_t balance;
	freetype free_torrent;
	time_t last_flushed;
	peer_list seeders;
	peer_list leechers;
	std::string last_selected_seeder;
	std::set<userid_t> tokened_users;
};

typedef std::unordered_map<std::string, torrent> torrent_list;

#endif
