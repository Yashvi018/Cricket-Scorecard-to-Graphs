COMPONENT 2 - MATCH DATA MODEL AND STORAGE

WHAT THIS PART DOES
- Defines Match, Innings, Player, Over, and FallOfWicket structures.
- Keeps multiple matches in memory using MatchStore.
- Adds, selects, lists, removes, saves, and loads matches.
- Validates basic structure safety and rejects negative/invalid values before storage.

FILES
- match_model.h       shared match data structures
- match_store.h       storage API
- match_store.c       dynamic memory, store operations, binary save/load
- match_validate.h   validation API
- match_validate.c   validation implementation
- test_storage.c     standalone demonstration program

BUILD THE COMPONENT 2 DEMO (GCC / MinGW)
From this folder:
  gcc -std=c11 -Wall -Wextra -pedantic match_store.c match_validate.c test_storage.c -o test_storage

Run on Windows:
  .\test_storage.exe

The demo writes sample_match.dat in the current folder. This sample is fictional. Do not include the generated executable or .dat file in the submission unless requested.

IMPORTANT OWNERSHIP RULES
- Call store_init before using a MatchStore and store_free before leaving.
- A successful store_add_match makes a deep copy. The caller still owns and must free its original Match data.
- Call match_free on a loaded match when finished with it.
- match_load expects an empty/zero-initialized destination Match. Free its allocations before reusing it.
- store_get_match and store_remove_match use zero-based indexes. A menu showing item 1 should call the API with index 0.
- Counts and pointers in each innings must agree: a nonzero count requires a non-NULL array.

INTEGRATION LIMIT WITH THE SUPPLIED COMPONENT 1 FILES
The supplied input parser produces only label-and-score DataPoint entries (for example, India and 185). It does not produce all the match, innings, player, over, and wicket fields stored by this model. It also has a standalone main and parser functions declared static in its .c file even though its header advertises public functions. The group must agree how Component 1 will expose detailed data before the parser can fill a complete Match. Do not claim that the current parser automatically builds this model.

INTEGRATION
- Component 1 reads and validates the agreed scorecard format.
- Component 2 owns the structures and storage API in these files.
- Component 3 reads Match/ Inning data using the shared model header.
- Component 5 can list/select matches through MatchStore.
- Keep test_storage.c out of the final application if another file has main; the final executable must have one main function.
