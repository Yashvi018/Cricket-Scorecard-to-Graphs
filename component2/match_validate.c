#include "match_validate.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define VALIDATION_MAX_ITEMS 10000u

static int fail(char *message, size_t size, const char *format, ...) {
    va_list args;
    if (message && size > 0) {
        va_start(args, format);
        (void)vsnprintf(message, size, format, args);
        va_end(args);
    }
    return 0;
}

static int has_terminator(const char *value, size_t capacity) {
    return value && memchr(value, '\0', capacity) != NULL;
}

static int nonempty(const char *value) {
    return value && value[0] != '\0';
}

static int valid_player(const Player *player, size_t index,
                        const char *kind, char *message, size_t size) {
    if (!has_terminator(player->name, sizeof(player->name)) ||
        !nonempty(player->name))
        return fail(message, size, "%s %zu has an empty or unterminated name",
                    kind, index + 1);

    if (player->runs < 0 || player->balls_faced < 0 || player->fours < 0 ||
        player->sixes < 0 || player->wickets < 0 ||
        player->balls_bowled < 0 || player->runs_conceded < 0)
        return fail(message, size, "%s '%s' has a negative statistic",
                    kind, player->name);

    return 1;
}

static int valid_array(const void *array, size_t count, const char *name,
                       char *message, size_t size) {
    if (count > VALIDATION_MAX_ITEMS)
        return fail(message, size, "%s count exceeds %u", name,
                    (unsigned)VALIDATION_MAX_ITEMS);
    if (count > 0 && !array)
        return fail(message, size, "%s count is nonzero but its array is NULL",
                    name);
    return 1;
}

static int valid_innings(const Innings *innings, size_t index,
                         char *message, size_t size) {
    size_t i;
    char prefix[32];
    int populated;

    (void)snprintf(prefix, sizeof(prefix), "innings %zu", index + 1);
    populated = innings->total_runs != 0 || innings->wickets != 0 ||
                innings->legal_balls != 0 || innings->batter_count != 0 ||
                innings->bowler_count != 0 || innings->over_count != 0 ||
                innings->fow_count != 0;

    if (!has_terminator(innings->batting_team,
                        sizeof(innings->batting_team)))
        return fail(message, size, "%s has an unterminated batting team name",
                    prefix);
    if (populated && !nonempty(innings->batting_team))
        return fail(message, size, "%s is populated but has no batting team",
                    prefix);

    if (innings->total_runs < 0 || innings->wickets < 0 ||
        innings->wickets > 10 || innings->legal_balls < 0)
        return fail(message, size, "%s has invalid total runs, wickets, or legal balls",
                    prefix);

    if (!valid_array(innings->batters, innings->batter_count, "batter", message, size) ||
        !valid_array(innings->bowlers, innings->bowler_count, "bowler", message, size) ||
        !valid_array(innings->overs, innings->over_count, "over", message, size) ||
        !valid_array(innings->fow, innings->fow_count, "fall-of-wicket", message, size))
        return 0;

    for (i = 0; i < innings->batter_count; ++i)
        if (!valid_player(&innings->batters[i], i, "batter", message, size))
            return 0;

    for (i = 0; i < innings->bowler_count; ++i)
        if (!valid_player(&innings->bowlers[i], i, "bowler", message, size))
            return 0;

    for (i = 0; i < innings->over_count; ++i) {
        const Over *over = &innings->overs[i];
        if (over->over_number < 1 || over->runs < 0 || over->wickets < 0 ||
            over->wickets > 10)
            return fail(message, size, "%s contains an invalid over record at index %zu",
                        prefix, i + 1);
        if (i > 0 && over->over_number <= innings->overs[i - 1].over_number)
            return fail(message, size, "%s over numbers must be strictly increasing",
                        prefix);
    }

    for (i = 0; i < innings->fow_count; ++i) {
        const FallOfWicket *wicket = &innings->fow[i];
        if (!has_terminator(wicket->player_out, sizeof(wicket->player_out)))
            return fail(message, size, "%s has an unterminated fall-of-wicket player name",
                        prefix);
        if (wicket->wicket_number < 1 || wicket->wicket_number > 10 ||
            wicket->team_score < 0 || wicket->over_number < 0 ||
            wicket->ball_in_over < 0 || wicket->ball_in_over > 6)
            return fail(message, size, "%s contains an invalid fall-of-wicket record at index %zu",
                        prefix, i + 1);
        if (i > 0 && wicket->wicket_number <= innings->fow[i - 1].wicket_number)
            return fail(message, size, "%s fall-of-wicket numbers must be strictly increasing",
                        prefix);
    }

    return 1;
}

int match_validate(const Match *match, char *error_message, size_t message_size) {
    int i;

    if (error_message && message_size > 0) error_message[0] = '\0';
    if (!match) return fail(error_message, message_size, "match pointer is NULL");

    if (!has_terminator(match->match_id, sizeof(match->match_id)) ||
        !nonempty(match->match_id))
        return fail(error_message, message_size,
                    "match ID is empty or not terminated");
    if (!has_terminator(match->team1, sizeof(match->team1)) ||
        !nonempty(match->team1))
        return fail(error_message, message_size,
                    "team 1 name is empty or not terminated");
    if (!has_terminator(match->team2, sizeof(match->team2)) ||
        !nonempty(match->team2))
        return fail(error_message, message_size,
                    "team 2 name is empty or not terminated");
    if (!has_terminator(match->venue, sizeof(match->venue)))
        return fail(error_message, message_size,
                    "venue is not terminated");
    if (!has_terminator(match->date, sizeof(match->date)))
        return fail(error_message, message_size,
                    "date is not terminated");

    for (i = 0; i < 2; ++i)
        if (!valid_innings(&match->innings[i], (size_t)i,
                           error_message, message_size))
            return 0;

    return 1;
}
