#include "user.h"

user::user(userid_t uid, bool leech, bool protect, bool track_ipv6) : id(uid), deleted(false), leechstatus(leech), protect_ip(protect), ipv6(track_ipv6) {
	stats.leeching = 0;
	stats.seeding = 0;
}
