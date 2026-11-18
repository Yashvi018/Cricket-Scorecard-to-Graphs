/*
 * component2_input_parser.h
 * Component 2: Input and Parsing - Cricket Score Graph Project
 *
 * Public interface for reading cricket scores (team/player + runs) from a
 * file or the keyboard, validating them, and producing a Dataset whose
 * `points` array uses the same DataPoint layout as Component 4.
 */
#ifndef COMPONENT2_INPUT_PARSER_H
#define COMPONENT2_INPUT_PARSER_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------ Constants ------------------------------- */
#define MAX_LINE   256   /* longest input line read                         */
#define MAX_LABEL  32    /* longest label kept (incl. terminating '\0')     */
#define MAX_POINTS 1000  /* maximum number of entries in a Dataset          */

/* ------------------------------- Types ---------------------------------- */
/* Shared with Component 4 (graph rendering). The guard lets both headers
   define it without a redefinition error. */
#ifndef DATAPOINT_DEFINED
#define DATAPOINT_DEFINED
typedef struct {
    const char *label;
    double value;
} DataPoint;
#endif

typedef struct {
    DataPoint *points;   /* labels are heap copies owned by the dataset */
    size_t count;
    size_t capacity;
} Dataset;

typedef enum {
    PARSE_OK,
    PARSE_SKIP,          /* blank line or comment                  */
    PARSE_BAD_FORMAT,    /* no label/score separator, empty field  */
    PARSE_BAD_VALUE      /* score is not a valid non-negative number */
} ParseStatus;

/* --------------------------- Dataset helpers ---------------------------- */
/* Initialise an empty dataset. Call before any other use. */
void dataset_init(Dataset *ds);

/* Free all labels and the points array; leaves ds empty and reusable. */
void dataset_free(Dataset *ds);

/* Append a (label, value) pair; the label is copied.
   Returns 1 on success, 0 on out-of-memory or MAX_POINTS reached. */
int dataset_add(Dataset *ds, const char *label, double value);

/* -------------------------------- Parsing ------------------------------- */
/* Parse "245", "78.5" or "245/6" (runs/wickets; only runs kept).
   Returns 1 and sets *out on success, 0 on invalid input. */
int parse_value(const char *s, double *out);

/* Parse one input line into a label and score.
   Returns PARSE_OK (label_out/value_out filled) or another ParseStatus. */
ParseStatus parse_line(const char *line, char label_out[MAX_LABEL], double *value_out);

/* --------------------------------- Input -------------------------------- */
/* Load entries from a file. Bad lines are reported on stderr and skipped;
   a non-numeric first line is treated as a header.
   Returns the number of entries added, or -1 if the file cannot be opened. */
int load_file(const char *path, Dataset *ds);

/* Read entries from the keyboard until a blank line or EOF.
   Returns the number of entries added. */
int load_interactive(Dataset *ds);

#ifdef __cplusplus
}
#endif

#endif /* COMPONENT2_INPUT_PARSER_H */
