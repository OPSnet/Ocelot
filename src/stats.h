#ifndef STATS_H
#define STATS_H

#include <atomic>
#include <cstdint>
#include <ctime>

struct stats_t {
	std::atomic<uint32_t> open_connections;
	std::atomic<uint64_t> opened_connections;
	std::atomic<uint64_t> connection_rate;
	std::atomic<uint32_t> leechers;
	std::atomic<uint32_t> seeders;
	std::atomic<uint64_t> requests;
	std::atomic<uint64_t> request_rate;
	std::atomic<uint64_t> announcements;
	std::atomic<uint64_t> succ_announcements;
	std::atomic<uint64_t> scrapes;
	std::atomic<uint64_t> bytes_read;
	std::atomic<uint64_t> bytes_written;
	time_t start_time;
};
extern struct stats_t stats;

#endif
