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

std::string report_jemalloc_plain(const char *opts, const std::string path) {
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

#define ITEM_BYTE(LABEL,    VALUE) "\"" LABEL "\":{\"type\":\"byte\",\"value\":" << VALUE << "}"
#define ITEM_ELAPSED(LABEL, VALUE) "\"" LABEL "\":{\"type\":\"elapsed\",\"value\":" << VALUE << "}"
#define ITEM_NUM(LABEL,     VALUE) "\"" LABEL "\":{\"type\":\"number\",\"value\":" << VALUE << "}"
#define ITEM_STR(LABEL,     VALUE) "\"" LABEL "\":{\"type\":\"string\",\"value\":\"" VALUE "\"}"
#define ITEM_VERSION(LABEL, MAJ, MIN, BUGFIX) "\"" LABEL "\":{\"type\":\"string\",\"value\":\"" << MAJ << "." << MIN << "." << BUGFIX << "\"}"

std::string report(const uint32_t announce_interval, const uint32_t announce_jitter) {
    std::ostringstream output;
    output << "{"
        << ITEM_STR("version", OCELOT_VERSION) << ','
        << ITEM_VERSION("jemalloc version", JEMALLOC_VERSION_MAJOR, JEMALLOC_VERSION_MINOR, JEMALLOC_VERSION_BUGFIX) << ','
        << ITEM_NUM("connections opened", stats.opened_connections) << ','
        << ITEM_NUM("open connections", stats.open_connections) << ','
        << ITEM_NUM("peak connections", stats.peak_connections) << ','
        << ITEM_NUM("connections/s", stats.connection_rate) << ','
        << ITEM_NUM("requests handled", stats.requests) << ','
        << ITEM_NUM("requests/s", stats.request_rate) << ','
        << ITEM_NUM("successful announcements", stats.succ_announcements) << ','
        << ITEM_NUM("failed announcements", (stats.announcements - stats.succ_announcements)) << ','
        << ITEM_NUM("scrapes", stats.scrapes) << ','
        << ITEM_NUM("leechers tracked", stats.leechers) << ','
        << ITEM_NUM("seeders tracked", stats.seeders) << ','
        << ITEM_NUM("items in user queue", stats.user_queue_size) << ','
        << ITEM_NUM("items in torrent queue", stats.torrent_queue_size) << ','
        << ITEM_NUM("items in peer queue", stats.peer_queue_size) << ','
        << ITEM_NUM("items in snatch queue", stats.snatch_queue_size) << ','
        << ITEM_NUM("items in token queue", stats.token_queue_size) << ','
        << ITEM_BYTE("max client request length", stats.max_client_request_len) << ','
        << ITEM_BYTE("bytes read", stats.bytes_read) << ','
        << ITEM_BYTE("bytes written", stats.bytes_written) << ','
        << ITEM_NUM("bad tracker secret received", stats.auth_error_secret) << ','
        << ITEM_NUM("bad report secret received", stats.auth_error_report) << ','
        << ITEM_NUM("bad announce key received", stats.auth_error_announce_key) << ','
        << ITEM_NUM("bad client configuration", stats.client_error) << ','
        << ITEM_NUM("bad http request", stats.http_error) << ','
        << ITEM_NUM("announce interval", announce_interval) << ','
        << ITEM_NUM("announce jitter", announce_jitter) << ','
        << ITEM_ELAPSED("uptime", (time(NULL) - stats.start_time))
        ;

    struct rusage r;
    if (getrusage(RUSAGE_SELF, &r) == 0) {
        output << ','
            << ITEM_ELAPSED("user time", r.ru_utime.tv_sec << '.' << std::setfill('0') << std::setw(6) << r.ru_utime.tv_usec) << ','
            << ITEM_ELAPSED("system time", r.ru_stime.tv_sec << '.' << std::setfill('0') << std::setw(6) << r.ru_stime.tv_usec) << ','
            << ITEM_BYTE("maximum resident set size", r.ru_maxrss * 1024) << ','
            << ITEM_NUM("minor page faults", r.ru_minflt) << ','
            << ITEM_NUM("major page faults", r.ru_majflt) << ','
            << ITEM_NUM("blocks in", r.ru_inblock) << ','
            << ITEM_NUM("blocks out", r.ru_oublock) << ','
            << ITEM_NUM("voluntary context switches", r.ru_nvcsw ) << ','
            << ITEM_NUM("involuntary context switches", r.ru_nivcsw )
            ;
    }
    output << "}\n";
    return output.str();
}

std::string report_prom_stats(const char *jemalloc) {
    std::ostringstream output;

    struct ocelot_alloc_info ji;
    int result = jemalloc_parse(jemalloc, &ji);

    output << "ocelot_uptime " << time(NULL) - stats.start_time << "\n"
        "ocelot_version " << OCELOT_VERSION_MAJOR
            << std::setfill('0') << std::setw(3) << OCELOT_VERSION_MINOR
            << std::setfill('0') << std::setw(3) << OCELOT_VERSION_BUGFIX
            << std::setfill('0') << std::setw(3) << OCELOT_VERSION_NREV << "\n"
        "jemalloc_version " << JEMALLOC_VERSION_MAJOR
            << std::setfill('0') << std::setw(3) << JEMALLOC_VERSION_MINOR
            << std::setfill('0') << std::setw(3) << JEMALLOC_VERSION_BUGFIX << "\n"
        "ocelot_open_connections " << stats.open_connections << "\n"

        "#TYPE ocelot_peak_connections counter\n"
        "ocelot_peak_connections "    << stats.peak_connections << "\n"
        "ocelot_connection_rate "     << stats.connection_rate << "\n"
        "ocelot_requests "            << stats.requests << "\n"
        "ocelot_request_rate "        << stats.request_rate << "\n"
        "ocelot_succ_announcements "  << stats.succ_announcements << "\n"
        "ocelot_total_announcements " << stats.announcements << "\n"
        "ocelot_scrapes "             << stats.scrapes << "\n"
        "ocelot_leechers "            << stats.leechers << "\n"
        "ocelot_seeders "             << stats.seeders << "\n"
        "ocelot_user_queue "          << stats.user_queue_size << "\n"
        "ocelot_torrent_queue "       << stats.torrent_queue_size << "\n"
        "ocelot_peer_queue "          << stats.peer_queue_size << "\n"
        "ocelot_snatch_queue "        << stats.snatch_queue_size << "\n"
        "ocelot_token_queue "         << stats.token_queue_size << "\n"
        "ocelot_bytes_read "          << stats.bytes_read << "\n"
        "ocelot_bytes_written "       << stats.bytes_written << "\n"

        "#TYPE ocelot_max_client_request_len counter\n"
        "ocelot_max_client_request_len " << stats.max_client_request_len << "\n"

        "#TYPE ocelot_error counter counter\n"
        "ocelot_error{kind=\"secret\"} "         << stats.auth_error_secret << "\n"
        "ocelot_error{kind=\"report\"} "         << stats.auth_error_report << "\n"
        "ocelot_error{kind=\"announce\"} "       << stats.auth_error_announce_key << "\n"
        "ocelot_error{kind=\"client\"} "         << stats.client_error << "\n"
        "ocelot_error{kind=\"http\"} "           << stats.http_error << "\n"
        "ocelot_error{kind=\"jemalloc_parse\"} " << result << "\n"

        "#TYPE jemalloc_arena_total counter\n"
        "jemalloc_arena_total " << ji.nr_arena << "\n"

        "#TYPE jemalloc_bin_total counter\n"
        "jemalloc_bin_total{bin=\"small\"} "        << ji.nr_bin_small << "\n"
        "jemalloc_bin_total{bin=\"thread_cache\"} " << ji.nr_bin_tc << "\n"
        "jemalloc_bin_total{bin=\"large\"} "        << ji.nr_bin_large << "\n"

        "#TYPE jemalloc_allocated counter\n"
        "jemalloc_allocated{bucket=\"small\"} " << ji.small.allocated << "\n"
        "jemalloc_allocated{bucket=\"large\"} " << ji.large.allocated << "\n"

        "#TYPE jemalloc_nmalloc_total counter\n"
        "jemalloc_nmalloc_total{bucket=\"small\"} " << ji.small.nmalloc_total << "\n"
        "jemalloc_nmalloc_total{bucket=\"large\"} " << ji.large.nmalloc_total << "\n"

        "#TYPE jemalloc_ndalloc_total counter\n"
        "jemalloc_ndalloc_total{bucket=\"small\"} " << ji.small.ndalloc_total << "\n"
        "jemalloc_ndalloc_total{bucket=\"large\"} " << ji.large.ndalloc_total << "\n"

        "#TYPE jemalloc_nrequests_total counter\n"
        "jemalloc_nrequests_total{bucket=\"small\"} " << ji.small.nrequests_total << "\n"
        "jemalloc_nrequests_total{bucket=\"large\"} " << ji.large.nrequests_total << "\n"

        "#TYPE jemalloc_nfill_total counter\n"
        "jemalloc_nfill_total{bucket=\"small\"} " << ji.small.nfill_total << "\n"
        "jemalloc_nfill_total{bucket=\"large\"} " << ji.large.nfill_total << "\n"

        "#TYPE jemalloc_nflush_total counter\n"
        "jemalloc_nflush_total{bucket=\"small\"} " << ji.small.nflush_total << "\n"
        "jemalloc_nflush_total{bucket=\"large\"} " << ji.large.nflush_total << "\n"

        "#TYPE jemalloc_nflush_total gauge\n"
        "jemalloc_memory{global=\"allocated\"} " << ji.mem_allocated << "\n"
        "jemalloc_memory{global=\"active\"} "    << ji.mem_active << "\n"
        "jemalloc_memory{global=\"metadata\"} "  << ji.mem_metadata << "\n"
        "jemalloc_memory{global=\"resident\"} "  << ji.mem_resident << "\n"
        "jemalloc_memory{global=\"mapped\"} "   << ji.mem_mapped << "\n"
        "jemalloc_memory{global=\"retained\"} "  << ji.mem_retained << "\n"

        "#TYPE jemalloc_nflush_rate gauge\n"
        "jemalloc_allocated{bucket=\"small\"} " << ji.small.allocated << "\n"
        "jemalloc_allocated{bucket=\"large\"} " << ji.large.allocated << "\n"

        "#TYPE jemalloc_nflush_rate gauge\n"
        "jemalloc_nmalloc_rate{bucket=\"small\"} " << ji.small.nmalloc_rate << "\n"
        "jemalloc_nmalloc_rate{bucket=\"large\"} " << ji.large.nmalloc_rate << "\n"

        "#TYPE jemalloc_nflush_total gauge\n"
        "jemalloc_ndalloc_rate{bucket=\"small\"} " << ji.small.ndalloc_rate << "\n"
        "jemalloc_ndalloc_rate{bucket=\"large\"} " << ji.large.ndalloc_rate << "\n"

        "#TYPE jemalloc_nflush_rate gauge\n"
        "jemalloc_nrequests_rate{bucket=\"small\"} " << ji.small.nrequests_rate << "\n"
        "jemalloc_nrequests_rate{bucket=\"large\"} " << ji.large.nrequests_rate << "\n"

        "#TYPE jemalloc_nflush_rate gauge\n"
        "jemalloc_fill_rate{bucket=\"small\"} " << ji.small.nfill_rate << "\n"
        "jemalloc_fill_rate{bucket=\"large\"} " << ji.large.nfill_rate << "\n"

        "#TYPE jemalloc_nflush_rate gauge\n"
        "jemalloc_nflush_rate{bucket=\"small\"} " << ji.small.nflush_rate << "\n"
        "jemalloc_nflush_rate{bucket=\"large\"} " << ji.large.nflush_rate << "\n"
        ;

    struct rusage r;
    if (getrusage(RUSAGE_SELF, &r) == 0) {
        output << "ocelot_time_user " << r.ru_utime.tv_sec << '.' << std::setfill('0') << std::setw(6) << r.ru_utime.tv_usec << "\n"
            "ocelot_time_system " << r.ru_stime.tv_sec << '.' << std::setfill('0') << std::setw(6) << r.ru_stime.tv_usec << "\n"
            "ocelot_max_rss " << r.ru_maxrss << "\n"
            "ocelot_minor_fault " << r.ru_minflt << "\n"
            "ocelot_major_fault " << r.ru_majflt << "\n"
            "#TYPE ocelot_block counter\n"
            "ocelot_block{op=\"in\"} " << r.ru_inblock << "\n"
            "ocelot_block{op=\"out\"} " << r.ru_oublock << "\n"
            "ocelot_nvcsw " << r.ru_nvcsw  << "\n"
            "ocelot_nivcsw " << r.ru_nivcsw  << "\n"
            ;
    }
    output << '#';
    return output.str();
}

std::string report_user(const user_ptr u) {
    std::ostringstream output;
    output << "{\"id\":"     << u->get_id()
        << ",\"leeching\":"  << u->get_leeching()
        << ",\"seeding\":"   << u->get_seeding()
        << ",\"deleted\":"   << u->is_deleted()
        << ",\"protected\":" << u->is_protected()
        << ",\"can_leech\":" << u->can_leech()
        << "}\n";
    return output.str();
}
