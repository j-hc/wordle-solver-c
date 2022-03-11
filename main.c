#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "./solver.h"

void solve(Guesser *guesser, const char *answer);

int main()
{
    Guesser guesser = new_guesser();

    char *answer1 = "dürüm";
    solve(&guesser, answer1);
    drop_guesser(&guesser);
    printf("\n\n");

    guesser = new_guesser();
    char *answer2 = "zurna";
    solve(&guesser, answer2);
    drop_guesser(&guesser);
    printf("\n\n");

    guesser = new_guesser();
    char *answer3 = "adsal";
    solve(&guesser, answer3);
    drop_guesser(&guesser);

    return 0;
}

void solve(Guesser *guesser, const char *answer)
{
    for (size_t i = 0; i < 32; ++i)
    {
        StringView guess = guess_based_on_history(guesser);
        if (strcmp(guess.s, answer) == 0)
        {
            printf("%zuth TRY, ANSWER FOUND: %s", i + 1, guess.s);
            break;
        }
        LetterStatus *word_stat = (LetterStatus *)malloc(5 * sizeof(LetterStatus));
        calc_word_status(guess.s, answer, word_stat);
        printf("GUESS: %s, MASK: ", guess.s);
        print_word_status(word_stat);
        StringView h_word = create_string_view(guess.s, strlen(guess.s));

        Guess h = {.ls = word_stat, .word = h_word};
        guesser->history[i] = h;
        guesser->history_size += 1;
    }
}