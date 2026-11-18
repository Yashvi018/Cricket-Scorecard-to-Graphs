#ifndef MATCH_VALIDATE_H
#define MATCH_VALIDATE_H

#include "match_model.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Validate the structure and basic score ranges of a match.
   Returns 1 when valid and 0 when invalid. If error_message is non-NULL,
   writes a short explanation into the supplied buffer. */
int match_validate(const Match *match, char *error_message, size_t message_size);

#ifdef __cplusplus
}
#endif

#endif
