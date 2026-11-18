#ifndef MATCH_MODEL_H
#define MATCH_MODEL_H

#include <stddef.h>

#define NAME_LEN 50
#define VENUE_LEN 80
#define DATE_LEN 20
#define MATCH_ID_LEN 20

typedef struct {
    char name[NAME_LEN];
    int runs;
    int balls_faced;
    int fours;
    int sixes;
    int wickets;
    int balls_bowled;
    int runs_conceded;
} Player;

typedef struct {
    int over_number;
    int runs;
    int wickets;
} Over;

typedef struct {
    int wicket_number;
    int team_score;
    int over_number;
    int ball_in_over;
    char player_out[NAME_LEN];
} FallOfWicket;

typedef struct {
    char batting_team[NAME_LEN];
    int total_runs;
    int wickets;
    int legal_balls;

    Player *batters;
    size_t batter_count;

    Player *bowlers;
    size_t bowler_count;

    Over *overs;
    size_t over_count;

    FallOfWicket *fow;
    size_t fow_count;
} Innings;

typedef struct {
    char match_id[MATCH_ID_LEN];
    char team1[NAME_LEN];
    char team2[NAME_LEN];
    char venue[VENUE_LEN];
    char date[DATE_LEN];
    Innings innings[2];
} Match;

#endif
