#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jemalloc_parse.h"

/* parse a size_t decimal text representation */
const char *parse_sz(const char *in, const char *find, size_t *target) {
    const char *p = strstr(in, find);
    if (!p) {
        // didn't find what we were looking for
        return NULL;
    }
    p += strlen(find);

    // copy the digits to a null-terminated string in order to pass it to strtoul()
    // the largest unsigned long decimal representation is 4294967295 (10 bytes)
    char convert[11];
    char *c = convert;
    while (isspace(*p)) {
        p++;
    }
    while (isdigit(*p) && c < convert + sizeof(convert) - 1) {
        *c++ = *p++;
    }
    *c = '\0';
    *target = strtoul(convert, &c, 10);

    return p + 1; // one past the last digit
}

/* parse an unsigned long long decimal text representation */
const char *parse_ull(const char *in, const char *find, unsigned long long *target) {
    const char *p = strstr(in, find);
    if (!p) {
        // didn't find what we were looking for
        return NULL;
    }
    p += strlen(find);

    // copy the digits to a null-terminated string in order to pass it to strtoull()
    // the largest unsigned long long decimal representation is 18446744073709551615 (21 bytes)
    char convert[22];
    char *c = convert;
    while (isspace(*p)) {
        p++;
    }
    while(isdigit(*p) && c < convert + sizeof(convert) - 1) {
        *c++ = *p++;
    }
    *c = '\0';
    *target = strtoull(convert, &c, 10);

    return p + 1; // one past the last digit
}

int jemalloc_parse(const char *in, struct ocelot_alloc_info *out) {
    /* Parse the output of jemalloc_status() plain output
     * This is an ugly hack; it would be faster to build directly what we want
     * rather than walk down a formatted string that has generated lots of
     * information that is discarded.
     * If the input breaks, the error code indicates where the parsing failed.
     */

    // set everything to zero
    memset (out, 0, sizeof(struct ocelot_alloc_info));

    /*
___ Begin jemalloc statistics ___
Version: "5.3.0-0-g54eaed1d8b56b1aa528be3bdd1877e59c56fa90c"
Build-time option settings
  ...
Run-time option settings
  ...
Profiling settings
  ...
Arenas: 17
Quantum size: 16
Page size: 4096
Maximum thread-cached size class: 32768
Number of bin size classes: 36
Number of thread-cache bin size classes: 41
Number of large size classes: 196
    */
    if (!(in = strstr(in, "___ Begin jemalloc statistics ___"))) {
        return 1;
    }

    if (!(in = parse_sz(in, "Arenas: ", &out->nr_arena))) {
        return 10;
    }
    if (!(in = parse_sz(in, "Number of bin size classes: ", &out->nr_bin_small))) {
        return 11;
    }
    if (!(in = parse_sz(in, "Number of thread-cache bin size classes: ", &out->nr_bin_tc))) {
        return 12;
    }
    if (!(in = parse_sz(in, "Number of large size classes: ", &out->nr_bin_large))) {
        return 13;
    }

    // Allocated: 895744, active: 1024000, metadata: 3047664 (n_thp 0), resident: 3985408, mapped: 7315456, retained: 1073152
    if (!(in = parse_ull(in, "Allocated: ", &out->mem_allocated))) {
        return 20;
    }
    if (!(in = parse_ull(in, "active: ", &out->mem_active))) {
        return 21;
    }
    if (!(in = parse_ull(in, "metadata: ", &out->mem_metadata))) {
        return 22;
    }
    if (!(in = parse_ull(in, "resident: ", &out->mem_resident))) {
        return 23;
    }
    if (!(in = parse_ull(in, "mapped: ", &out->mem_mapped))) {
        return 24;
    }
    if (!(in = parse_ull(in, "retained: ", &out->mem_retained))) {
        return 25;
    }

    /*
                            allocated         nmalloc   (#/sec)         ndalloc   (#/sec)       nrequests   (#/sec)           nfill   (#/sec)          nflush   (#/sec)
small:                         247840          131452         0          129036         0           24219         0            2402         0            8543         0
large:                         290816             815         0             808         0             971         0             815         0            2351         0
total:                         538656          132267         0          129844         0   
    */
    if (!(in = strstr(in, "allocated         nmalloc   (#/sec)         ndalloc   (#/sec)"))) {
        return 30;
    }
    if (!(in = strstr(in, "\nsmall:"))) {
        return 31;
    }

    if (!(in = parse_sz(in, " ", &out->small.allocated))) {
        return 40;
    }
    if (!(in = parse_sz(in, " ", &out->small.nmalloc_total))) {
        return 41;
    }
    if (!(in = parse_sz(in, " ", &out->small.ndalloc_rate))) {
        return 42;
    }
    if (!(in = parse_sz(in, " ", &out->small.ndalloc_total))) {
        return 43;
    }
    if (!(in = parse_sz(in, " ", &out->small.nrequests_rate))) {
        return 44;
    }
    if (!(in = parse_sz(in, " ", &out->small.nrequests_total))) {
        return 45;
    }
    if (!(in = parse_sz(in, " ", &out->small.nfill_rate))) {
        return 46;
    }
    if (!(in = parse_sz(in, " ", &out->small.nfill_total))) {
        return 47;
    }
    if (!(in = parse_sz(in, " ", &out->small.nflush_rate))) {
        return 48;
    }
    if (!(in = parse_sz(in, " ", &out->small.nflush_total))) {
        return 49;
    }

    if (!(in = strstr(in, "\nlarge:"))) {
        return 60;
    }

    if (!(in = parse_sz(in, " ", &out->large.allocated))) {
        return 70;
    }
    if (!(in = parse_sz(in, " ", &out->large.nmalloc_total))) {
        return 71;
    }
    if (!(in = parse_sz(in, " ", &out->large.ndalloc_rate))) {
        return 72;
    }
    if (!(in = parse_sz(in, " ", &out->large.ndalloc_total))) {
        return 73;
    }
    if (!(in = parse_sz(in, " ", &out->large.nrequests_rate))) {
        return 74;
    }
    if (!(in = parse_sz(in, " ", &out->large.nrequests_total))) {
        return 75;
    }
    if (!(in = parse_sz(in, " ", &out->large.nfill_rate))) {
        return 76;
    }
    if (!(in = parse_sz(in, " ", &out->large.nfill_total))) {
        return 77;
    }
    if (!(in = parse_sz(in, " ", &out->large.nflush_rate))) {
        return 78;
    }
    if (!(in = parse_sz(in, " ", &out->large.nflush_total))) {
        return 79;
    }

    return 0;
}
