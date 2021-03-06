#include "cado.h"
#include "las-dlog-base.h"
#include "las-types.h"

#include <cstdio>
#include <stdint.h>     /* cstdint is c++11-only */
#include <cctype>


#include "utils.h"

void las_dlog_base::declare_parameter_usage(param_list pl)
{
    param_list_decl_usage(pl, "renumber", "renumber table (for the descent)");
    param_list_decl_usage(pl, "log", "log table, as built by reconstructlog");
}

void las_dlog_base::lookup_parameters(param_list pl)
{
    const char * tmp;
    if ((tmp = param_list_lookup_string(pl, "renumber")) != NULL) {
        renumberfilename = strdup(tmp);
    }
    if ((tmp = param_list_lookup_string(pl, "log")) != NULL) {
        logfilename = strdup(tmp);
    }
    if (!logfilename != !renumberfilename) {
        fprintf(stderr, "In descent mode, want either renumber+log, or none\n");
        exit(EXIT_FAILURE);
    }
    if (!param_list_parse_ulong(pl, "lpb0", &(lpb[0]))) {
        fprintf(stderr, "In descent mode, want lpb0 for the final descent\n");
        exit(EXIT_FAILURE);
    }
    if (!param_list_parse_ulong(pl, "lpb1", &(lpb[1]))) {
        fprintf(stderr, "In descent mode, want lpb1 for the final descent\n");
        exit(EXIT_FAILURE);
    }
}

las_dlog_base::las_dlog_base()
{
    renumberfilename = NULL;
    logfilename = NULL;
    renumber_init_for_reading(renumber_table);
}

las_dlog_base::~las_dlog_base()
{
    renumber_clear (renumber_table);
    free(renumberfilename);
    free(logfilename);
}

bool las_dlog_base::is_known(int side, uint64_t p, uint64_t r) const {
    if (p >> lpb[side])
        return false;
    if (renumberfilename) {
        /* For now we assume that we know the log of all bad ideals */
        /* If we want to be able to do a complete lookup for bad ideals
         * too, then we need the badidealinfo file, as well as a piece of
         * code which is currently in dup2 */
        if (renumber_is_bad (NULL, NULL, renumber_table, p, r, side))
            return true;
        index_t h = renumber_get_index_from_p_r(renumber_table, p, r, side);
        return known_logs[h];
    }
    return true;
}


void las_dlog_base::read()
{
    if (!renumberfilename) {
        verbose_output_print(0, 1, "# Descent: no access to renumber table given, using lpb(%lu/%lu) to decide what are the supposedly known logs\n",
                lpb[0], lpb[1]);
        return;
    }

    verbose_output_print(0, 1, "# Descent: will get list of known logs from %s, using also %s for mapping\n", logfilename, renumberfilename);

    renumber_read_table(renumber_table, renumberfilename);
    uint64_t nprimes = renumber_table->size;
    known_logs.reserve(nprimes);
    /* format of the log table: there are FIVE different line types.
     *
     * [index] added column [log]
     * [index] bad ideals [log]
     * [index] [p] [side] rat [log]
     * [index] [p] [side] [r] [log]
     * [index] SM col [i] [log]
     *
     * Here we care only about the index anyway. By the way, this index
     * is written in hex.
     */
    FILE * f = fopen(logfilename, "r");
    ASSERT_ALWAYS(f != NULL);
    for(int lnum = 0 ; ; lnum++) {
        char line[1024];
        char * x = fgets(line, sizeof(line), f);
        if (x == NULL) break;
        for( ; *x && isspace(*x) ; x++) ;
        if (*x == '#') continue;
        if (!*x) continue;
        errno=0;
        unsigned long z = strtoul(x, &x, 16);
        if (errno) {
            fprintf(stderr, "Parse error at line %d in %s: %s\n", lnum, logfilename, line);
            break;
        }
        known_logs[z] = true;
    }
    fclose(f);
}


