// Copyright [2017-2024] Orpheus

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/resource.h>

#include <jemalloc/jemalloc.h>
#include <spdlog/spdlog.h> // debug

#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>  // for std::setfill() and std::setw()
#include <map>
#include <mutex>
#include <thread>

#include "ocelot.h"
#include "misc_functions.h"
#include "jemalloc_parse.h"
#include "report.h"
#include "response.h"
#include "user.h"

int jemalloc_fd;
std::mutex jemalloc_fd_mutex;

/* Getting jemalloc stats back to Gazelle is a bit cumbersome
 * because all jemalloc can do is write to a file descriptor.
 * So we let it write to a file (which is why using a ramdisk
 * is important) and then read it back to return as an HTTP
 * response.
 */

// callback function for malloc_stats_print()
void jemalloc_stats_export(void *opaque, const char *buf) {
    if (jemalloc_fd > 0) {
        write(jemalloc_fd, buf, strlen(buf));
    }
}

int dump_jemalloc(const char *filename, const char *opts) {
    // open the file descriptor
    std::lock_guard<std::mutex> guard(jemalloc_fd_mutex);
    jemalloc_fd = open(filename, O_CREAT|O_WRONLY, 0600);
    if (jemalloc_fd == -1) {
        return jemalloc_fd;
    }

    // dump stats
    malloc_stats_print(jemalloc_stats_export, NULL, opts);

    // tidy up
    int result = close(jemalloc_fd);
    jemalloc_fd = 0;
    return result;
}

std::string report_jemalloc_plain(const char *opts, std::string path) {
    std::string filename(path + "/jemalloc.json." + std::to_string(pthread_self()));
    if (dump_jemalloc(filename.c_str(), opts) != 0) {
        // return an empty string if something went bananas
        unlink(filename.c_str());
        return std::string("");
    }

    // slurp the contents into a string
    std::ifstream input(filename);
    std::ostringstream output;
    while(input >> output.rdbuf())
        ;
    unlink(filename.c_str());
    output << "\n";

    std::shared_ptr<spdlog::logger> logger(spdlog::get("logger"));
    logger->debug("jemalloc stats written to " + filename + ", size=" + std::to_string(output.str().size()));
    return output.str();
}

std::string report(params_type &params, user_list &users_list, unsigned int announce_interval, unsigned int announce_jitter) {
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
            << "jemalloc_version: " << JEMALLOC_VERSION_MAJOR << '.' << JEMALLOC_VERSION_MINOR << '.' << JEMALLOC_VERSION_BUGFIX << "\n"
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

        struct rusage r;
        if (getrusage(RUSAGE_SELF, &r) == 0) {
            output << r.ru_utime.tv_sec << '.' << std::setfill('0') << std::setw(6) << r.ru_utime.tv_usec << " user time\n"
                << r.ru_stime.tv_sec << '.' << std::setfill('0') << std::setw(6) << r.ru_stime.tv_usec << " system time\n"
                << r.ru_maxrss << " maximum resident set size\n"
                << r.ru_minflt << " minor page faults\n"
                << r.ru_majflt << " major page faults\n"
                << r.ru_inblock << " blocks in\n"
                << r.ru_oublock << " blocks out\n"
                << r.ru_nvcsw  << " voluntary context switches\n"
                << r.ru_nivcsw  << " involuntary context switches\n"
                ;
        }
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
                return output.str();
            }
        }
    } else {
        output << "Invalid action\n";
    }
    output << "success";
    return output.str();
}

std::string report_prom_stats(const char *jemalloc) {
    std::stringstream output;
    struct ocelot_alloc_info ji;
    int result = jemalloc_parse(jemalloc, &ji);

    output << "ocelot_uptime " << time(NULL) - stats.start_time << "\n"
        << "ocelot_version " << version() << "\n"
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
        << "ocelot_bytes_written " << stats.bytes_written << "\n"
        << "jemalloc_version " << JEMALLOC_VERSION_MAJOR << '.' << JEMALLOC_VERSION_MINOR << '.' << JEMALLOC_VERSION_BUGFIX << "\n"
        << "jemalloc_parse_error " << result << "\n"
        << "jemalloc_mem_allocated " << ji.mem_allocated << "\n"
        << "jemalloc_arena_total " << ji.nr_arena << "\n"
        << "jemalloc_bin_small_total " << ji.nr_bin_small << "\n"
        << "jemalloc_bin_thread_cache_total " << ji.nr_bin_tc << "\n"
        << "jemalloc_bin_large_total " << ji.nr_bin_large << "\n"
        << "jemalloc_memory_allocated " << ji.mem_allocated << "\n"
        << "jemalloc_memory_active " << ji.mem_active << "\n"
        << "jemalloc_memory_metadata " << ji.mem_metadata << "\n"
        << "jemalloc_memory_resident " << ji.mem_resident << "\n"
        << "jemalloc_memory_mapped " << ji.mem_mapped << "\n"
        << "jemalloc_memory_retained " << ji.mem_retained << "\n"
        << "jemalloc_small_allocated " << ji.small.allocated << "\n"
        << "jemalloc_small_nmalloc_total " << ji.small.nmalloc_total << "\n"
        << "jemalloc_small_nmalloc_rate " << ji.small.nmalloc_rate << "\n"
        << "jemalloc_small_ndalloc_total " << ji.small.ndalloc_total << "\n"
        << "jemalloc_small_ndalloc_rate " << ji.small.ndalloc_rate << "\n"
        << "jemalloc_small_nrequests_total " << ji.small.nrequests_total << "\n"
        << "jemalloc_small_nrequests_rate " << ji.small.nrequests_rate << "\n"
        << "jemalloc_small_nfill_total " << ji.small.nfill_total << "\n"
        << "jemalloc_small_fill_rate " << ji.small.nfill_rate << "\n"
        << "jemalloc_small_nflush_total " << ji.small.nflush_total << "\n"
        << "jemalloc_small_nflush_rate " << ji.small.nflush_rate << "\n"
        << "jemalloc_large_allocated " << ji.large.allocated << "\n"
        << "jemalloc_large_nmalloc_total " << ji.large.nmalloc_total << "\n"
        << "jemalloc_large_nmalloc_rate " << ji.large.nmalloc_rate << "\n"
        << "jemalloc_large_ndalloc_total " << ji.large.ndalloc_total << "\n"
        << "jemalloc_large_ndalloc_rate " << ji.large.ndalloc_rate << "\n"
        << "jemalloc_large_nrequests_total " << ji.large.nrequests_total << "\n"
        << "jemalloc_large_nrequests_rate " << ji.large.nrequests_rate << "\n"
        << "jemalloc_large_nfill_total " << ji.large.nfill_total << "\n"
        << "jemalloc_large_fill_rate " << ji.large.nfill_rate << "\n"
        << "jemalloc_large_nflush_total " << ji.large.nflush_total << "\n"
        << "jemalloc_large_nflush_rate " << ji.large.nflush_rate << "\n"
        ;

    struct rusage r;
    if (getrusage(RUSAGE_SELF, &r) == 0) {
        output << "ocelot_time_user " << r.ru_utime.tv_sec << '.' << std::setfill('0') << std::setw(6) << r.ru_utime.tv_usec << "\n"
            "ocelot_time_system " << r.ru_stime.tv_sec << '.' << std::setfill('0') << std::setw(6) << r.ru_stime.tv_usec << "\n"
            "ocelot_max_rss " << r.ru_maxrss << "\n"
            "ocelot_minor_fault " << r.ru_minflt << "\n"
            "ocelot_major_fault " << r.ru_majflt << "\n"
            "ocelot_blk_in " << r.ru_inblock << "\n"
            "ocelot_blk_out " << r.ru_oublock << "\n"
            "ocelot_nvcsw " << r.ru_nvcsw  << "\n"
            "ocelot_nivcsw " << r.ru_nivcsw  << "\n"
            ;
    }
    output << '#';
    return output.str();
}
