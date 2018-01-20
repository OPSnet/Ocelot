#ifndef USER_H
#define USER_H

#include <atomic>
#include <memory>
#include <unordered_map>

typedef uint32_t userid_t;

class user {
	private:
		userid_t id;
		bool deleted;
		bool leechstatus;
		bool protect_ip;
		struct {
			std::atomic<uint32_t> leeching;
			std::atomic<uint32_t> seeding;
		} stats;
	public:
		user(userid_t uid, bool leech, bool protect) : id(uid), deleted(false),
			leechstatus(leech), protect_ip(protect) {
			stats.leeching = 0;
			stats.seeding = 0;
		}
		userid_t get_id() { return id; }
		bool is_deleted() { return deleted; }
		void set_deleted(bool status) { deleted = status; }
		bool is_protected() { return protect_ip; }
		void set_protected(bool status) { protect_ip = status; }
		bool can_leech() { return leechstatus; }
		void set_leechstatus(bool status) { leechstatus = status; }
		void decr_leeching() { --stats.leeching; }
		void decr_seeding() { --stats.seeding; }
		void incr_leeching() { ++stats.leeching; }
		void incr_seeding() { ++stats.seeding; }
		uint32_t get_leeching() { return stats.leeching; }
		uint32_t get_seeding() { return stats.seeding; }
};

typedef std::shared_ptr<user> user_ptr;
typedef std::unordered_map<std::string, user_ptr> user_list;

#endif
