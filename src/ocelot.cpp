#include <fstream>
#include <iostream>
#include <csignal>
#include <thread>
#include <sys/stat.h>
#include <syslog.h>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/stdout_sinks.h"
#include "ocelot.h"
#include "config.h"
#include "db.h"
#include "worker.h"
#include "events.h"

static connection_mother *mother;
static worker *work;
static mysql *db;
static site_comm *sc;
static config *conf;
static schedule *sched;

struct stats_t stats;

static void create_daemon()
{
	pid_t pid;

	/* Fork off the parent process */
	pid = fork();

	/* An error occurred */
	if (pid < 0) {
		exit(EXIT_FAILURE);
	}

	/* Success: Let the parent terminate */
	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	/* On success: The child process becomes session leader */
	if (setsid() < 0)
		exit(EXIT_FAILURE);

	/* Set new file permissions */
	umask(0);

	/* Change the working directory to the root directory */
	/* or another appropriated directory */
	if (chdir("/") < 0) {
		exit(EXIT_FAILURE);
	}

	/* Close all open file descriptors */
	int x;
	for (x = static_cast<int>(sysconf(_SC_OPEN_MAX)); x >= 0; x--) {
		close (x);
	}

	/* Open the log file */
	openlog("ocelot", LOG_PID, LOG_DAEMON);
}

static void sig_handler(int sig) {
	std::shared_ptr<spdlog::logger> logger = spdlog::get("logger");
	if (sig == SIGINT || sig == SIGTERM) {
		logger->info("Caught SIGINT/SIGTERM");
		if (work->shutdown()) {
			exit(0);
		}
	} else if (sig == SIGHUP) {
		logger->info("Reloading config");
		logger->flush();
		conf->reload();
		db->reload_config(conf);
		mother->reload_config(conf);
		sc->reload_config(conf);
		sched->reload_config(conf);
		work->reload_config(conf);
		logger->info("Done reloading config");
	} else if (sig == SIGUSR1) {
		logger->info("Reloading from database");
		std::thread w_thread(&worker::reload_lists, work);
		w_thread.detach();
	}
}

int main(int argc, char **argv) {
	// we don't use printf so make cout/cerr a little bit faster
	std::ios_base::sync_with_stdio(false);

	conf = new config();

	bool verbose = false, conf_arg = false, daemonize = false;
	std::string conf_file_path("./ocelot.conf");

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-v") == 0) {
			verbose = true;
		}
		else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--daemonize") == 0) {
			daemonize = true;
		}
		else if (strcmp(argv[i], "-c") == 0 && i < argc - 1) {
			conf_arg = true;
			conf_file_path = argv[++i];
		}
		else if(strcmp(argv[i], "-V") == 0 || strcmp(argv[i], "--version") == 0) {
			std::cout << "Ocelot, version 1.1" << std::endl;
			return 0;
		}
		else {
			std::cout << "Usage: " << argv[0] << " [-v] [-c configfile]" << std::endl;
			return 0;
		}
	}

	std::ifstream conf_file(conf_file_path);
	if (conf_file.fail()) {
		std::cout << "Using default config because '" << conf_file_path << "' couldn't be opened" << std::endl;
		if (!conf_arg) {
			std::cout << "Start Ocelot with -c <path> to specify config file if necessary" << std::endl;
		}
	}
	else {
		conf->load(conf_file_path, conf_file);
	}

	if (conf->get_bool("daemonize") || daemonize) {
		create_daemon();
	}

	std::vector<spdlog::sink_ptr> sinks;
	if (!conf->get_bool("daemonize") && !daemonize) {
		sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_mt>());
	}
	if (conf->get_bool("log") && !conf->get_str("log_path").empty()) {
		sinks.push_back(std::make_shared<spdlog::sinks::daily_file_sink_mt>(conf->get_str("log_path"), 23, 59));
	}

	auto combined_logger = std::make_shared<spdlog::logger>("logger", begin(sinks), end(sinks));
	// If we don't set flush on info, the file log takes a long while to actually flush
	combined_logger->flush_on(spdlog::level::info);
	spdlog::register_logger(combined_logger);

	db = new mysql(conf);

	if (!db->connected()) {
		combined_logger->critical("Could not connect to DB. Exiting!");
		return 0;
	}
	db->verbose_flush = verbose;

	sc = new site_comm(conf);
	sc->verbose_flush = verbose;

	user_list users_list;
	torrent_list torrents_list;
	std::vector<std::string> whitelist;
	db->load_users(users_list);
	db->load_torrents(torrents_list);
	db->load_whitelist(whitelist);

	stats.open_connections = 0;
	stats.opened_connections = 0;
	stats.connection_rate = 0;
	stats.requests = 0;
	stats.request_rate = 0;
	stats.leechers = 0;
	stats.seeders = 0;
	stats.announcements = 0;
	stats.succ_announcements = 0;
	stats.scrapes = 0;
	stats.bytes_read = 0;
	stats.bytes_written = 0;
	stats.start_time = time(NULL);

	// Create worker object, which handles announces and scrapes and all that jazz
	work = new worker(conf, torrents_list, users_list, whitelist, db, sc);

	// Create schedule object
	sched = new schedule(conf, work, db, sc);

	// Create connection mother, which binds to its socket and handles the event stuff
	mother = new connection_mother(conf, work, db, sc, sched);

	// Add signal handlers now that all objects have been created
	struct sigaction handler{}, ignore{};
	ignore.sa_handler = SIG_IGN;
	handler.sa_handler = sig_handler;
	sigemptyset(&handler.sa_mask);
	handler.sa_flags = 0;

	sigaction(SIGINT, &handler, NULL);
	sigaction(SIGTERM, &handler, NULL);
	sigaction(SIGHUP, &handler, NULL);
	sigaction(SIGUSR1, &handler, NULL);
	sigaction(SIGUSR2, &ignore, NULL);

	mother->run();

	return 0;
}
