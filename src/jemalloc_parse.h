#ifndef SRC_PARSE_JEMALLOC_H_
#define SRC_PARSE_JEMALLOC_H_

struct ocelot_alloc_stat {
    size_t allocated;
    size_t nmalloc_total;
    size_t nmalloc_rate;
    size_t ndalloc_total;
    size_t ndalloc_rate;
    size_t nrequests_total;
    size_t nrequests_rate;
    size_t nfill_total;
    size_t nfill_rate;
    size_t nflush_total;
    size_t nflush_rate;
};

struct ocelot_alloc_info {
    size_t nr_arena;
    size_t nr_bin_small;
    size_t nr_bin_tc;
    size_t nr_bin_large;

    unsigned long long mem_allocated;
    unsigned long long mem_active;
    unsigned long long mem_metadata;
    unsigned long long mem_resident;
    unsigned long long mem_mapped;
    unsigned long long mem_retained;

    struct ocelot_alloc_stat small;
    struct ocelot_alloc_stat large;
};

int jemalloc_parse(const char *in, struct ocelot_alloc_info *out);

#endif  // SRC_PARSE_JEMALLOC_H_
