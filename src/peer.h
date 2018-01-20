#ifndef PEER_H
#define PEER_H

#include <cstdint>
#include <ctime>
#include <map>
#include <memory>
#include <string>

class user;

struct peer {
	int64_t uploaded;
	int64_t downloaded;
	int64_t corrupt;
	int64_t left;
	time_t last_announced;
	time_t first_announced;
	uint32_t announces;
	uint16_t port;
	bool visible;
	bool invalid_ip;
	std::shared_ptr<class user> user;
	std::string ip_port;
	std::string ip;
};

typedef std::map<std::string, peer> peer_list;

#endif
