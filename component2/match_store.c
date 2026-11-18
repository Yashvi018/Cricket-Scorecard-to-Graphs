#include "match_store.h"
#include "match_validate.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define FILE_VERSION 1u
#define MAX_ITEMS 10000u

static int write_data(FILE *fp, const void *data, size_t size) {
    return fwrite(data, 1, size, fp) == size;
}

static int read_data(FILE *fp, void *data, size_t size) {
    return fread(data, 1, size, fp) == size;
}

static int write_int(FILE *fp, int value) {
    int32_t number = (int32_t)value;
    return write_data(fp, &number, sizeof(number));
}

static int read_int(FILE *fp, int *value) {
    int32_t number;
    if (!read_data(fp, &number, sizeof(number))) return 0;
    *value = (int)number;
    return 1;
}

static int write_string(FILE *fp, const char *value, size_t max_length) {
    size_t length = strlen(value);
    uint32_t saved_length;

    if (length >= max_length) return 0;
    saved_length = (uint32_t)length;

    return write_data(fp, &saved_length, sizeof(saved_length)) &&
           write_data(fp, value, length);
}

static int read_string(FILE *fp, char *value, size_t max_length) {
    uint32_t length;

    if (!read_data(fp, &length, sizeof(length))) return 0;
    if (length >= max_length) return 0;
    if (!read_data(fp, value, length)) return 0;

    value[length] = '\0';
    return 1;
}

static int write_count(FILE *fp, size_t count) {
    uint32_t saved_count;
    if (count > MAX_ITEMS) return 0;
    saved_count = (uint32_t)count;
    return write_data(fp, &saved_count, sizeof(saved_count));
}

static int read_count(FILE *fp, size_t *count) {
    uint32_t saved_count;
    if (!read_data(fp, &saved_count, sizeof(saved_count))) return 0;
    if (saved_count > MAX_ITEMS) return 0;
    *count = (size_t)saved_count;
    return 1;
}

static int write_player(FILE *fp, const Player *p) {
    return write_string(fp, p->name, NAME_LEN) &&
           write_int(fp, p->runs) &&
           write_int(fp, p->balls_faced) &&
           write_int(fp, p->fours) &&
           write_int(fp, p->sixes) &&
           write_int(fp, p->wickets) &&
           write_int(fp, p->balls_bowled) &&
           write_int(fp, p->runs_conceded);
}

static int read_player(FILE *fp, Player *p) {
    return read_string(fp, p->name, NAME_LEN) &&
           read_int(fp, &p->runs) &&
           read_int(fp, &p->balls_faced) &&
           read_int(fp, &p->fours) &&
           read_int(fp, &p->sixes) &&
           read_int(fp, &p->wickets) &&
           read_int(fp, &p->balls_bowled) &&
           read_int(fp, &p->runs_conceded);
}

static int write_innings(FILE *fp, const Innings *in) {
    size_t i;

    if (!write_string(fp, in->batting_team, NAME_LEN) ||
        !write_int(fp, in->total_runs) ||
        !write_int(fp, in->wickets) ||
        !write_int(fp, in->legal_balls) ||
        !write_count(fp, in->batter_count)) return 0;

    for (i = 0; i < in->batter_count; i++)
        if (!write_player(fp, &in->batters[i])) return 0;

    if (!write_count(fp, in->bowler_count)) return 0;
    for (i = 0; i < in->bowler_count; i++)
        if (!write_player(fp, &in->bowlers[i])) return 0;

    if (!write_count(fp, in->over_count)) return 0;
    for (i = 0; i < in->over_count; i++) {
        if (!write_int(fp, in->overs[i].over_number) ||
            !write_int(fp, in->overs[i].runs) ||
            !write_int(fp, in->overs[i].wickets)) return 0;
    }

    if (!write_count(fp, in->fow_count)) return 0;
    for (i = 0; i < in->fow_count; i++) {
        const FallOfWicket *w = &in->fow[i];
        if (!write_int(fp, w->wicket_number) ||
            !write_int(fp, w->team_score) ||
            !write_int(fp, w->over_number) ||
            !write_int(fp, w->ball_in_over) ||
            !write_string(fp, w->player_out, NAME_LEN)) return 0;
    }

    return 1;
}

static int read_innings(FILE *fp, Innings *in) {
    size_t i, count;

    if (!read_string(fp, in->batting_team, NAME_LEN) ||
        !read_int(fp, &in->total_runs) ||
        !read_int(fp, &in->wickets) ||
        !read_int(fp, &in->legal_balls) ||
        !read_count(fp, &count)) return 0;

    if (count > 0) {
        in->batters = calloc(count, sizeof(Player));
        if (!in->batters) return 0;
        in->batter_count = count;
        for (i = 0; i < count; i++)
            if (!read_player(fp, &in->batters[i])) return 0;
    }

    if (!read_count(fp, &count)) return 0;
    if (count > 0) {
        in->bowlers = calloc(count, sizeof(Player));
        if (!in->bowlers) return 0;
        in->bowler_count = count;
        for (i = 0; i < count; i++)
            if (!read_player(fp, &in->bowlers[i])) return 0;
    }

    if (!read_count(fp, &count)) return 0;
    if (count > 0) {
        in->overs = calloc(count, sizeof(Over));
        if (!in->overs) return 0;
        in->over_count = count;
        for (i = 0; i < count; i++) {
            if (!read_int(fp, &in->overs[i].over_number) ||
                !read_int(fp, &in->overs[i].runs) ||
                !read_int(fp, &in->overs[i].wickets)) return 0;
        }
    }

    if (!read_count(fp, &count)) return 0;
    if (count > 0) {
        in->fow = calloc(count, sizeof(FallOfWicket));
        if (!in->fow) return 0;
        in->fow_count = count;
        for (i = 0; i < count; i++) {
            FallOfWicket *w = &in->fow[i];
            if (!read_int(fp, &w->wicket_number) ||
                !read_int(fp, &w->team_score) ||
                !read_int(fp, &w->over_number) ||
                !read_int(fp, &w->ball_in_over) ||
                !read_string(fp, w->player_out, NAME_LEN)) return 0;
        }
    }

    return 1;
}

void match_free(Match *match) {
    int i;

    if (!match) return;

    for (i = 0; i < 2; i++) {
        free(match->innings[i].batters);
        free(match->innings[i].bowlers);
        free(match->innings[i].overs);
        free(match->innings[i].fow);
    }

    memset(match, 0, sizeof(Match));
}

void store_init(MatchStore *store) {
    if (!store) return;

    store->matches = NULL;
    store->count = 0;
    store->capacity = 0;
}

void store_free(MatchStore *store) {
    size_t i;

    if (!store) return;

    for (i = 0; i < store->count; i++)
        match_free(&store->matches[i]);

    free(store->matches);
    store_init(store);
}

static int copy_array(void **destination, const void *source,
                      size_t count, size_t item_size) {
    *destination = NULL;

    if (count == 0) return 1;
    if (!source) return 0;

    *destination = malloc(count * item_size);
    if (!*destination) return 0;

    memcpy(*destination, source, count * item_size);
    return 1;
}

static int copy_innings(Innings *destination, const Innings *source) {
    destination->total_runs = source->total_runs;
    destination->wickets = source->wickets;
    destination->legal_balls = source->legal_balls;
    strcpy(destination->batting_team, source->batting_team);

    destination->batter_count = source->batter_count;
    destination->bowler_count = source->bowler_count;
    destination->over_count = source->over_count;
    destination->fow_count = source->fow_count;

    if (!copy_array((void **)&destination->batters, source->batters,
                    source->batter_count, sizeof(Player))) return 0;
    if (!copy_array((void **)&destination->bowlers, source->bowlers,
                    source->bowler_count, sizeof(Player))) return 0;
    if (!copy_array((void **)&destination->overs, source->overs,
                    source->over_count, sizeof(Over))) return 0;
    if (!copy_array((void **)&destination->fow, source->fow,
                    source->fow_count, sizeof(FallOfWicket))) return 0;

    return 1;
}

static int copy_match(Match *destination, const Match *source) {
    int i;

    memset(destination, 0, sizeof(Match));

    strcpy(destination->match_id, source->match_id);
    strcpy(destination->team1, source->team1);
    strcpy(destination->team2, source->team2);
    strcpy(destination->venue, source->venue);
    strcpy(destination->date, source->date);

    for (i = 0; i < 2; i++) {
        if (!copy_innings(&destination->innings[i], &source->innings[i])) {
            match_free(destination);
            return 0;
        }
    }

    return 1;
}

int store_add_match(MatchStore *store, const Match *match) {
    Match copy;
    Match *new_array;
    size_t new_capacity;
    char validation_error[160];

    if (!store || !match) return 0;
    if (!match_validate(match, validation_error, sizeof(validation_error))) {
        fprintf(stderr, "Invalid match: %s\n", validation_error);
        return 0;
    }
    if (!copy_match(&copy, match)) return 0;

    if (store->count == store->capacity) {
        new_capacity = (store->capacity == 0) ? 2 : store->capacity * 2;

        new_array = realloc(store->matches, new_capacity * sizeof(Match));
        if (!new_array) {
            match_free(&copy);
            return 0;
        }

        store->matches = new_array;
        store->capacity = new_capacity;
    }

    store->matches[store->count] = copy;
    store->count++;

    return 1;
}

Match *store_get_match(MatchStore *store, size_t index) {
    if (!store || index >= store->count) return NULL;
    return &store->matches[index];
}

int store_remove_match(MatchStore *store, size_t index) {
    size_t i;

    if (!store || index >= store->count) return 0;

    match_free(&store->matches[index]);

    for (i = index; i + 1 < store->count; i++)
        store->matches[i] = store->matches[i + 1];

    store->count--;
    memset(&store->matches[store->count], 0, sizeof(Match));

    return 1;
}

void store_list_matches(const MatchStore *store) {
    size_t i;

    if (!store || store->count == 0) {
        printf("No matches stored.\n");
        return;
    }

    for (i = 0; i < store->count; i++) {
        const Match *m = &store->matches[i];

        printf("%zu. %s | %s vs %s | Venue: %s\n",
               i, m->match_id, m->team1, m->team2, m->venue);
    }
}

int match_save(const Match *match, const char *filename) {
    FILE *fp;
    uint32_t version = FILE_VERSION;
    int success;
    char validation_error[160];

    if (!match || !filename) return 0;
    if (!match_validate(match, validation_error, sizeof(validation_error))) {
        fprintf(stderr, "Invalid match: %s\n", validation_error);
        return 0;
    }

    fp = fopen(filename, "wb");
    if (!fp) return 0;

    success = write_data(fp, "CRIC", 4) &&
              write_data(fp, &version, sizeof(version)) &&
              write_string(fp, match->match_id, MATCH_ID_LEN) &&
              write_string(fp, match->team1, NAME_LEN) &&
              write_string(fp, match->team2, NAME_LEN) &&
              write_string(fp, match->venue, VENUE_LEN) &&
              write_string(fp, match->date, DATE_LEN) &&
              write_innings(fp, &match->innings[0]) &&
              write_innings(fp, &match->innings[1]);

    if (fclose(fp) != 0) success = 0;

    return success;
}

int match_load(Match *match, const char *filename) {
    FILE *fp;
    char magic[4];
    uint32_t version;
    Match temporary;
    int success;

    if (!match || !filename) return 0;

    memset(&temporary, 0, sizeof(Match));

    fp = fopen(filename, "rb");
    if (!fp) return 0;

    success = read_data(fp, magic, 4) &&
              memcmp(magic, "CRIC", 4) == 0 &&
              read_data(fp, &version, sizeof(version)) &&
              version == FILE_VERSION &&
              read_string(fp, temporary.match_id, MATCH_ID_LEN) &&
              read_string(fp, temporary.team1, NAME_LEN) &&
              read_string(fp, temporary.team2, NAME_LEN) &&
              read_string(fp, temporary.venue, VENUE_LEN) &&
              read_string(fp, temporary.date, DATE_LEN) &&
              read_innings(fp, &temporary.innings[0]) &&
              read_innings(fp, &temporary.innings[1]);

    fclose(fp);

    if (success) {
        char validation_error[160];
        if (!match_validate(&temporary, validation_error, sizeof(validation_error))) {
            fprintf(stderr, "Invalid match file: %s\n", validation_error);
            success = 0;
        }
    }

    if (!success) {
        match_free(&temporary);
        return 0;
    }

    /* Caller must provide an empty/zero-initialized Match. */
    *match = temporary;
    return 1;
}

