#include "match_store.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    MatchStore store;
    Match sample;
    Match loaded;
    Match *saved_match;
    int i;

    memset(&sample, 0, sizeof(Match));
    memset(&loaded, 0, sizeof(Match));

    /* Fictional sample data for demonstrating Component 2. */
    strcpy(sample.match_id, "MATCH001");
    strcpy(sample.team1, "India");
    strcpy(sample.team2, "Australia");
    strcpy(sample.venue, "Ahmedabad");
    strcpy(sample.date, "2026-11-04");

    /* First innings: India */
    strcpy(sample.innings[0].batting_team, "India");
    sample.innings[0].total_runs = 185;
    sample.innings[0].wickets = 6;
    sample.innings[0].legal_balls = 120;

    sample.innings[0].batter_count = 2;
    sample.innings[0].batters = calloc(2, sizeof(Player));
    if (!sample.innings[0].batters) {
        printf("Memory allocation failed.\n");
        return 1;
    }

    strcpy(sample.innings[0].batters[0].name, "Rohit");
    sample.innings[0].batters[0].runs = 72;
    sample.innings[0].batters[0].balls_faced = 45;
    sample.innings[0].batters[0].fours = 6;
    sample.innings[0].batters[0].sixes = 4;

    strcpy(sample.innings[0].batters[1].name, "Virat");
    sample.innings[0].batters[1].runs = 58;
    sample.innings[0].batters[1].balls_faced = 40;
    sample.innings[0].batters[1].fours = 5;
    sample.innings[0].batters[1].sixes = 2;

    sample.innings[0].over_count = 3;
    sample.innings[0].overs = calloc(3, sizeof(Over));
    if (!sample.innings[0].overs) {
        match_free(&sample);
        printf("Memory allocation failed.\n");
        return 1;
    }

    sample.innings[0].overs[0] = (Over){1, 8, 0};
    sample.innings[0].overs[1] = (Over){2, 12, 1};
    sample.innings[0].overs[2] = (Over){3, 6, 0};

    /* Second innings: Australia */
    strcpy(sample.innings[1].batting_team, "Australia");
    sample.innings[1].total_runs = 180;
    sample.innings[1].wickets = 8;
    sample.innings[1].legal_balls = 120;

    sample.innings[1].bowler_count = 1;
    sample.innings[1].bowlers = calloc(1, sizeof(Player));
    if (!sample.innings[1].bowlers) {
        match_free(&sample);
        printf("Memory allocation failed.\n");
        return 1;
    }

    strcpy(sample.innings[1].bowlers[0].name, "Bumrah");
    sample.innings[1].bowlers[0].wickets = 3;
    sample.innings[1].bowlers[0].balls_bowled = 24;
    sample.innings[1].bowlers[0].runs_conceded = 25;

    /* Initialize storage and add a copy of the sample match. */
    store_init(&store);

    if (!store_add_match(&store, &sample)) {
        printf("Could not add match to storage.\n");
        match_free(&sample);
        store_free(&store);
        return 1;
    }

    printf("MATCHES IN STORAGE\n");
    store_list_matches(&store);

    /* Save the first stored match to a file. */
    saved_match = store_get_match(&store, 0);

    if (!saved_match || !match_save(saved_match, "sample_match.dat")) {
        printf("Could not save match.\n");
        match_free(&sample);
        store_free(&store);
        return 1;
    }

    printf("\nMatch saved in sample_match.dat\n");

    /* Load the saved match into a new Match variable. */
    if (!match_load(&loaded, "sample_match.dat")) {
        printf("Could not load match.\n");
        match_free(&sample);
        store_free(&store);
        return 1;
    }

    printf("\nLOADED MATCH\n");
    printf("%s vs %s\n", loaded.team1, loaded.team2);
    printf("%s: %d/%d\n",
           loaded.innings[0].batting_team,
           loaded.innings[0].total_runs,
           loaded.innings[0].wickets);
    printf("%s: %d/%d\n",
           loaded.innings[1].batting_team,
           loaded.innings[1].total_runs,
           loaded.innings[1].wickets);

    printf("\nRuns in first three overs: ");
    for (i = 0; i < (int)loaded.innings[0].over_count; i++)
        printf("%d ", loaded.innings[0].overs[i].runs);
    printf("\n");

    /* Free all dynamically allocated memory. */
    match_free(&loaded);
    match_free(&sample);
    store_free(&store);

    printf("\nMemory released. Test completed.\n");

    return 0;
}
