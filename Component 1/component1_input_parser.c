/*
 * Component 2: Input and Parsing - Cricket Score Graph Project
 *
 * Reads cricket scores (team/player + runs) from a file or from the keyboard,
 * validates them, and produces a Dataset whose `points` array uses the SAME
 * DataPoint layout as Component 4 (the graph rendering engine):
 *
 *     typedef struct { const char *label; double value; } DataPoint;
 *
 * Accepted line formats (one entry per line):
 *     Tigers, 78            comma separated (CSV)
 *     Eagles: 92            colon / semicolon / tab separated
 *     South Africa 245      whitespace separated (last token is the score)
 *     India, 287/6          "runs/wickets" - only the runs are kept
 *     # comment             ignored, as are blank lines
 *     Team,Runs             a non-numeric first line is treated as a header
 *
 * Build:  cc -std=c99 -Wall -Wextra -pedantic component2_input_parser.c -o parser
 * Run:    ./parser scores.csv        (read from a file)
 *         ./parser                   (type the scores in interactively)
 *
 * Integration with Component 4:
 *     Dataset ds;  dataset_init(&ds);
 *     if (load_file("scores.csv", &ds) > 0) render_all(ds.points, ds.count);
 *     dataset_free(&ds);
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>

#define MAX_LINE   256
#define MAX_LABEL  32      /* longest label kept (incl. terminating '\0') */
#define MAX_POINTS 1000

/* ------- Same type Component 4 uses, so the data can be passed straight in */
typedef struct { const char *label; double value; } DataPoint;

typedef struct {
    DataPoint *points;     /* labels are heap copies owned by the dataset */
    size_t count;
    size_t capacity;
} Dataset;

typedef enum { PARSE_OK, PARSE_SKIP, PARSE_BAD_FORMAT, PARSE_BAD_VALUE } ParseStatus;

/* ---------------------------- Dataset helpers --------------------------- */
static void dataset_init(Dataset *ds) {
    ds->points = NULL; ds->count = 0; ds->capacity = 0;
}

static void dataset_free(Dataset *ds) {
    size_t i;
    for (i = 0; i < ds->count; ++i) free((void *)ds->points[i].label);
    free(ds->points);
    dataset_init(ds);
}

static int dataset_add(Dataset *ds, const char *label, double value) {
    char *copy;
    if (ds->count >= MAX_POINTS) return 0;
    if (ds->count == ds->capacity) {
        size_t ncap = ds->capacity ? ds->capacity * 2 : 8;
        DataPoint *np = (DataPoint *)realloc(ds->points, ncap * sizeof(DataPoint));
        if (!np) return 0;
        ds->points = np; ds->capacity = ncap;
    }
    copy = (char *)malloc(strlen(label) + 1);
    if (!copy) return 0;
    strcpy(copy, label);
    ds->points[ds->count].label = copy;
    ds->points[ds->count].value = value;
    ds->count++;
    return 1;
}

/* ------------------------------ String utils ---------------------------- */
static char *trim(char *s) {
    char *end;
    while (isspace((unsigned char)*s)) ++s;
    end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) --end;
    *end = '\0';
    return s;
}

/* Remove one pair of surrounding double quotes, e.g. "Tigers" -> Tigers */
static void strip_quotes(char *s) {
    size_t n = strlen(s);
    if (n >= 2 && s[0] == '"' && s[n - 1] == '"') {
        memmove(s, s + 1, n - 2);
        s[n - 2] = '\0';
    }
}

/* ------------------------------ Parsing --------------------------------- */
/* Parse "245", "78.5" or "245/6" (runs/wickets). Must consume whole string. */
static int parse_value(const char *s, double *out) {
    char *end; double v;
    errno = 0;
    v = strtod(s, &end);
    if (end == s || errno == ERANGE) return 0;
    if (*end == '/') {                       /* runs/wickets */
        char *end2; long w = strtol(end + 1, &end2, 10);
        if (end2 == end + 1 || w < 0 || w > 10) return 0;
        end = end2;
    }
    while (isspace((unsigned char)*end)) ++end;
    if (*end != '\0') return 0;
    if (!isfinite(v) || v < 0.0) return 0;   /* scores cannot be negative */
    *out = v;
    return 1;
}

/* Find the separator between label and score. The LAST separator is used so
   labels such as "South Africa" or "Smith, J" still work. */
static char *find_separator(char *s) {
    static const char seps[] = ",:;\t";
    size_t i; char *p;
    for (i = 0; seps[i]; ++i) {
        p = strrchr(s, seps[i]);
        if (p) return p;
    }
    p = s + strlen(s);
    while (p > s && !isspace((unsigned char)p[-1])) --p;   /* last token */
    return (p > s) ? p - 1 : NULL;
}

/* Parse one line. On PARSE_OK, label_out/value_out are filled. */
static ParseStatus parse_line(const char *line, char label_out[MAX_LABEL], double *value_out) {
    char buf[MAX_LINE], *s, *sep, *label, *val;

    if (strlen(line) >= MAX_LINE - 1 && line[strlen(line) - 1] != '\n') return PARSE_BAD_FORMAT;
    strcpy(buf, line);
    s = trim(buf);
    if (*s == '\0' || *s == '#') return PARSE_SKIP;

    sep = find_separator(s);
    if (!sep) return PARSE_BAD_FORMAT;
    *sep = '\0';
    label = trim(s);
    val = trim(sep + 1);
    strip_quotes(label);
    label = trim(label);
    if (*label == '\0' || *val == '\0') return PARSE_BAD_FORMAT;

    if (!parse_value(val, value_out)) return PARSE_BAD_VALUE;

    strncpy(label_out, label, MAX_LABEL - 1);
    label_out[MAX_LABEL - 1] = '\0';         /* truncate overly long labels */
    return PARSE_OK;
}

/* ------------------------------- Input ---------------------------------- */
/* Load from a file. Bad lines are reported (with line numbers) and skipped.
   Returns the number of entries added, or -1 if the file cannot be opened. */
static int load_file(const char *path, Dataset *ds) {
    FILE *f = fopen(path, "r");
    char line[MAX_LINE], label[MAX_LABEL];
    int line_no = 0, added = 0, seen_data = 0;
    double value;

    if (!f) { fprintf(stderr, "Error: cannot open '%s': %s\n", path, strerror(errno)); return -1; }

    while (fgets(line, sizeof(line), f)) {
        ParseStatus st;
        ++line_no;
        st = parse_line(line, label, &value);
        if (st == PARSE_SKIP) continue;
        if (st == PARSE_BAD_VALUE && !seen_data) { seen_data = 1; continue; } /* header row */
        seen_data = 1;
        if (st == PARSE_BAD_FORMAT) {
            fprintf(stderr, "Warning: line %d skipped (expected \"name, score\")\n", line_no);
        } else if (st == PARSE_BAD_VALUE) {
            fprintf(stderr, "Warning: line %d skipped (score must be a non-negative number)\n", line_no);
        } else if (!dataset_add(ds, label, value)) {
            fprintf(stderr, "Error: out of memory or more than %d entries\n", MAX_POINTS);
            break;
        } else {
            ++added;
        }
    }
    if (ferror(f)) fprintf(stderr, "Error: problem while reading '%s'\n", path);
    fclose(f);
    return added;
}

/* Interactive entry from the keyboard. A blank line (or EOF) finishes input. */
static int load_interactive(Dataset *ds) {
    char line[MAX_LINE], label[MAX_LABEL];
    int added = 0; double value;

    printf("Enter one entry per line as:  name, score   (e.g. Tigers, 78)\n");
    printf("Press Enter on an empty line to finish.\n");
    for (;;) {
        char *t; ParseStatus st;
        printf("> "); fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) break;
        t = line; while (isspace((unsigned char)*t)) ++t;
        if (*t == '\0') break;
        st = parse_line(line, label, &value);
        if (st == PARSE_SKIP) continue;
        if (st == PARSE_BAD_FORMAT) { printf("  Invalid format - use: name, score\n"); continue; }
        if (st == PARSE_BAD_VALUE)  { printf("  Score must be a non-negative number\n"); continue; }
        if (!dataset_add(ds, label, value)) { printf("  Cannot store more entries\n"); break; }
        ++added;
    }
    return added;
}

/* ------------------------------ Demo main -------------------------------- */
int main(int argc, char **argv) {
    Dataset ds; int n; size_t i;
    dataset_init(&ds);

    n = (argc > 1) ? load_file(argv[1], &ds) : load_interactive(&ds);
    if (n < 0) { dataset_free(&ds); return EXIT_FAILURE; }
    if (n == 0) {
        fprintf(stderr, "No valid data was read.\n");
        dataset_free(&ds);
        return EXIT_FAILURE;
    }

    printf("\nParsed %d entr%s:\n", n, n == 1 ? "y" : "ies");
    printf("%-4s %-*s %s\n", "#", MAX_LABEL, "Label", "Value");
    for (i = 0; i < ds.count; ++i)
        printf("%-4zu %-*s %.2f\n", i + 1, MAX_LABEL, ds.points[i].label, ds.points[i].value);

    /* ds.points / ds.count are ready to hand to Component 4's renderer */
    dataset_free(&ds);
    return EXIT_SUCCESS;
}
