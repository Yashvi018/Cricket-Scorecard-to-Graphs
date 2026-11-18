#ifndef MATCH_STORE_H
#define MATCH_STORE_H

#include "match_model.h"

typedef struct {
    Match *matches;
    size_t count;
    size_t capacity;
} MatchStore;

void store_init(MatchStore *store);
void match_free(Match *match);
void store_free(MatchStore *store);

int store_add_match(MatchStore *store, const Match *match);
Match *store_get_match(MatchStore *store, size_t index);
int store_remove_match(MatchStore *store, size_t index);
void store_list_matches(const MatchStore *store);

int match_save(const Match *match, const char *filename);
/* match_load expects an empty/zero-initialized destination Match. */
int match_load(Match *match, const char *filename);

#endif

