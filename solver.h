#ifndef SOLVER_H
#define SOLVER_H

#include <stdbool.h>

typedef struct
{
    char *s;
    // size_t ?
} StringView;

typedef struct
{
    StringView word;
    float freq;
} WordFreq;

typedef enum
{
    MISPLACED,
    CORRECT,
    WRONG,
} LetterStatus;

typedef struct
{
    StringView word;
    LetterStatus *ls;
} Guess;

typedef struct
{
    Guess *history;
    size_t history_size;
    WordFreq *remaining;
    size_t remaining_size;
    LetterStatus *patterns;
    size_t patterns_size;
} Guesser;

typedef struct
{
    StringView word;
    float score;
} CandidateWord;

StringView create_string_view(char *str, size_t len);
WordFreq create_word_freq(char *str);

size_t load_txt_lines(char *path, StringView buf_sv[]);

void drop_guesser(Guesser* guesser);

int array_index(const char val, const char *arr, size_t size, bool skip[5]);

void calc_word_status(const char guess[5], const char answer[5], LetterStatus let_stat_buf[5]);

void print_word_status(LetterStatus let_stat_buf[5]);

bool is_word_status_same(const LetterStatus ws_1[5], const LetterStatus ws_2[5]);

bool is_word_ls_matches(const Guess *guess, const char answer[5]);

void load_all_patterns(LetterStatus all_patterns[243][5]);

void filter_remaining(Guesser *guesser);

void filter_patterns(Guesser *guesser, WordFreq *wf, float *sum, float remaining_count);

StringView guess_based_on_history(Guesser *guesser);

size_t load_freq_dict(WordFreq *freq_dict);

Guesser new_guesser();

#endif