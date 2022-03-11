#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>

#include "./solver.h"

#define DICTIONARY_SIZE 5532

static Guess history[64] = {0};

StringView create_string_view(char *str, size_t len)
{
    char *s = malloc(len);
    memcpy(s, str, len);
    StringView sv = {.s = s};
    return sv;
}

WordFreq create_word_freq(char *str)
{
    size_t space_i = 0;
    while (str[++space_i] != ' ');
    assert(space_i != 0);
    str[space_i] = 0;
    StringView sv = create_string_view(str, space_i);
    float wfreq = atof(str + space_i + 1);
    WordFreq wq = {.word = sv, .freq = wfreq};
    return wq;
}

size_t load_txt_lines(char *path, StringView buf_sv[])
{
    FILE *f = fopen(path, "r");
    size_t i = 0;
    char buffer[64];
    while (fgets(buffer, sizeof(buffer), f))
    {
        assert(i <= DICTIONARY_SIZE);
        size_t len = strlen(buffer);
        StringView str = create_string_view(buffer, len - 1);
        buf_sv[i++] = str;
        memset(buffer, 0, sizeof(buffer));
    };
    fclose(f);
    return i;
}

int array_index(const char val, const char *arr, size_t size, bool skip[5])
{
    for (size_t i = 0; i < size; ++i)
    {
        if (skip[i])
            continue;
        if (arr[i] == val)
            return i;
    }
    return -1;
}

void calc_word_status(const char guess[5], const char answer[5], LetterStatus let_stat_buf[5])
{
    bool used[5] = {false};
    for (size_t i = 0; i < 5; ++i)
    {
        int index = array_index(guess[i], answer, 5, used);
        if (index != -1)
        {
            if (index == (int)i)
            {
                let_stat_buf[i] = CORRECT;
                used[i] = true;
                continue;
            }
            else if (!used[index])
            {
                let_stat_buf[i] = MISPLACED;
                used[index] = true;
                continue;
            }
        }
        used[i] = true;
        let_stat_buf[i] = WRONG;
    }
}

void print_word_status(LetterStatus let_stat_buf[5])
{
    for (size_t i = 0; i < 5; ++i)
    {
        switch (let_stat_buf[i])
        {
        case MISPLACED:
            printf("M");
            break;
        case CORRECT:
            printf("C");
            break;
        case WRONG:
            printf("W");
            break;
        default:
            assert(false);
            break;
        }
        printf(" ");
    }
    printf("\n");
}

bool is_word_status_same(const LetterStatus ws_1[5], const LetterStatus ws_2[5])
{
    return !memcmp(ws_1, ws_2, 5 * sizeof(LetterStatus));
}

bool is_word_ls_matches(const Guess *guess, const char answer[5])
{
    LetterStatus let_stat_buf[5] = {0};
    calc_word_status(guess->word.s, answer, let_stat_buf);
    return is_word_status_same(let_stat_buf, guess->ls);
}

void load_all_patterns(LetterStatus all_patterns[243][5])
{
    // cartesian product of 5 [M, W, C] sets
    size_t s = 0;
    for (size_t i = 0; i < 3; ++i)
    {
        for (size_t j = 0; j < 3; ++j)
        {
            for (size_t k = 0; k < 3; ++k)
            {
                for (size_t l = 0; l < 3; ++l)
                {
                    for (size_t m = 0; m < 3; ++m)
                    {
                        all_patterns[s][0] = i;
                        all_patterns[s][1] = j;
                        all_patterns[s][2] = k;
                        all_patterns[s][3] = l;
                        all_patterns[s][4] = m;
                        s += 1;
                    }
                }
            }
        }
    }
}

void filter_remaining(Guesser *guesser)
{
    WordFreq *new_rem = realloc(guesser->remaining, guesser->remaining_size * sizeof(WordFreq));
    Guess last_guess = guesser->history[guesser->history_size - 1];
    size_t new_rem_size = 0;
    for (size_t i = 0; i < guesser->remaining_size; ++i)
    {
        if (is_word_ls_matches(&last_guess, guesser->remaining[i].word.s))
        {
            memcpy(&new_rem[new_rem_size++], &guesser->remaining[i], sizeof(WordFreq));
        }
    }
    guesser->remaining = new_rem;
    guesser->remaining_size = new_rem_size;
}

void filter_patterns(Guesser *guesser, WordFreq *wf, float *sum, float remaining_count)
{
    LetterStatus *new_pats = realloc(guesser->patterns, guesser->patterns_size);
    size_t new_pats_size = 0;
    for (size_t i = 0; i < guesser->patterns_size; ++i)
    {
        float pattern_total = 0;
        for (size_t i = 0; i < guesser->remaining_size; ++i)
        {
            Guess g = {.word = wf->word, .ls = malloc(5 * sizeof(LetterStatus))};
            memcpy(g.ls, &guesser->patterns[i], 5 * sizeof(LetterStatus));
            if (is_word_ls_matches(&g, wf->word.s))
            {
                pattern_total = wf->freq;
            }
            if (pattern_total != 0)
            {
                float prob_pattern = pattern_total / remaining_count;
                *sum = prob_pattern * log2f(prob_pattern);
                memcpy(&new_pats[new_pats_size++], &guesser->patterns[i], sizeof(LetterStatus) * 5);
            }
        }
    }
    guesser->patterns = new_pats;
    guesser->patterns_size = new_pats_size;
}

StringView guess_based_on_history(Guesser *guesser)
{
    if (guesser->history_size != 0)
    {
        filter_remaining(guesser);
    }
    else
    {
        StringView starting = {.s = "aideu"};
        return starting;
    }

    CandidateWord best_word = {.word = guesser->remaining[0].word, .score = 0.0};
    for (size_t i = 0; i < guesser->remaining_size; ++i)
    {
        WordFreq wf = guesser->remaining[i];
        float sum = 0.0;
        float remaining_count = 0.0;
        for (size_t i = 0; i < guesser->remaining_size; ++i)
            remaining_count += guesser->remaining[i].freq;
        filter_patterns(guesser, &wf, &sum, remaining_count);
        float prob_word = guesser->remaining[i].freq / remaining_count;
        float score = prob_word * -sum;
        if (score > best_word.score)
        {
            StringView word = guesser->remaining[i].word;
            best_word.word = create_string_view(word.s, strlen(word.s));
            best_word.score = score;
        }
    }
    return best_word.word;
}

size_t load_freq_dict(WordFreq *freq_dict)
{
    StringView dict[DICTIONARY_SIZE] = {0};
    size_t size = load_txt_lines("./dataset/wordle_tur_freq_dict.txt", dict);
    for (size_t i = 0; i < size; i++)
    {
        WordFreq wf = create_word_freq(dict[i].s);
        // C'yi s2m
        if (wf.word.s[strlen(wf.word.s) - 1] == 127)
            wf.word.s[strlen(wf.word.s) - 1] = 0;
        freq_dict[i] = wf;
    }
    return size;
}

Guesser new_guesser()
{
    memset(history, 0, sizeof(Guess) * 32);
    WordFreq *freq_dict = malloc(DICTIONARY_SIZE * sizeof(WordFreq));
    size_t freq_dict_size = load_freq_dict(freq_dict);

    LetterStatus(*all_patterns)[243][5] = malloc(243 * 5 * sizeof(LetterStatus));
    load_all_patterns((LetterStatus(*)[5])all_patterns);

    Guesser guesser = {.history = history, .history_size = 0, .patterns = (LetterStatus *)all_patterns, .patterns_size = 243, .remaining = freq_dict, .remaining_size = freq_dict_size};
    return guesser;
}

void drop_guesser(Guesser *guesser)
{
    free(guesser->patterns);
    free(guesser->remaining);
}

void solver_test(void)
{
    LetterStatus let_state_buf[5] = {0};

    calc_word_status("abcde", "abcde", let_state_buf);
    LetterStatus l1[5] = {CORRECT, CORRECT, CORRECT, CORRECT, CORRECT};
    assert(is_word_status_same(l1, let_state_buf));

    calc_word_status("abcde", "fghij", let_state_buf);
    LetterStatus l2[5] = {WRONG, WRONG, WRONG, WRONG, WRONG};
    assert(is_word_status_same(l2, let_state_buf));

    calc_word_status("abcde", "eabcd", let_state_buf);
    LetterStatus l3[5] = {MISPLACED, MISPLACED, MISPLACED, MISPLACED, MISPLACED};
    assert(is_word_status_same(l3, let_state_buf));

    calc_word_status("baaaa", "aaccc", let_state_buf);
    LetterStatus l4[5] = {WRONG, CORRECT, MISPLACED, WRONG, WRONG};
    assert(is_word_status_same(l4, let_state_buf));
}
