#include <iostream>
#include <spdlog/spdlog.h>

#include "ocelot.h"
#include "config.h"
#include "db.h"
#include "worker.h"
#include "schedule.h"

schedule::schedule(config * conf, worker * worker_obj, mysql * db_obj, site_comm * sc_obj) : work(worker_obj), db(db_obj), sc(sc_obj) {
	load_config(conf);
	counter = 0;
	last_opened_connections = 0;
	last_request_count = 0;
	next_reap_peers = reap_peers_interval;
	logger = spdlog::get("logger");
}

void schedule::load_config(config * conf) {
	reap_peers_interval = conf->get_uint("reap_peers_interval");
	schedule_interval = conf->get_uint("schedule_interval");
}

void schedule::reload_config(config * conf) {
	load_config(conf);
}

//---------- Schedule - gets called every schedule_interval seconds
void schedule::handle(ev::timer &watcher, int events_flags) {
	unsigned int cur_schedule_interval = watcher.repeat;
	stats.connection_rate = (stats.opened_connections - last_opened_connections) / cur_schedule_interval;
	stats.request_rate = (stats.requests - last_request_count) / cur_schedule_interval;
	if (counter % 20 == 0) {
		logger->info("{} open, {} connections ({}/s), {} requests ({}/s)",
					 stats.open_connections, stats.opened_connections, stats.connection_rate,
					 stats.requests, stats.request_rate);
	}

	if (work->get_status() == CLOSING && db->all_clear() && sc->all_clear()) {
		logger->info("all clear, shutting down");
		exit(0);
	}

	last_opened_connections = stats.opened_connections;
	last_request_count = stats.requests;

	db->flush();
	sc->flush_tokens();

	next_reap_peers -= cur_schedule_interval;
	if (next_reap_peers <= 0) {
		work->start_reaper();
		next_reap_peers = reap_peers_interval;
	}

	counter++;
	if (schedule_interval != cur_schedule_interval) {
		watcher.set(schedule_interval, schedule_interval);
	}
}
