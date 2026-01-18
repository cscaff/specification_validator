/*
 * Blackjack Game - Automated Validation Harness
 * Generated from configuration parameters
 *
 * Tests various hand/dealer combinations to verify basic strategy.
 *
 * Exit codes:
 *   0 = Success (all decisions correct)
 *   1 = Incorrect decision made
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/* Forward declarations for controller variables */
extern int handValue;
extern int dealerCard;
extern bool isSoft;
extern bool shouldHit;

/* Test case structure */
typedef struct {
    int hand;
    int dealer;
    bool soft;
    bool should_hit;
    const char* description;
} TestCase;

/* Basic strategy test cases */
static TestCase test_cases[] = {
    /* Hard hands - always hit 11 or less */
    {8, 5, false, true, "Hard 8 vs 5 -> Hit"},
    {10, 9, false, true, "Hard 10 vs 9 -> Hit"},
    {11, 6, false, true, "Hard 11 vs 6 -> Hit"},

    /* Hard 12 - hit vs 2-3, 7-A; stand vs 4-6 */
    {12, 2, false, true, "Hard 12 vs 2 -> Hit"},
    {12, 4, false, false, "Hard 12 vs 4 -> Stand"},
    {12, 6, false, false, "Hard 12 vs 6 -> Stand"},
    {12, 7, false, true, "Hard 12 vs 7 -> Hit"},

    /* Hard 13-16 - stand vs 2-6, hit vs 7-A */
    {13, 3, false, false, "Hard 13 vs 3 -> Stand"},
    {14, 6, false, false, "Hard 14 vs 6 -> Stand"},
    {15, 7, false, true, "Hard 15 vs 7 -> Hit"},
    {16, 10, false, true, "Hard 16 vs 10 -> Hit"},

    /* Hard 17+ - always stand */
    {17, 10, false, false, "Hard 17 vs 10 -> Stand"},
    {19, 11, false, false, "Hard 19 vs A -> Stand"},
    {20, 5, false, false, "Hard 20 vs 5 -> Stand"},

    /* Soft hands - hit soft 17 or less */
    {13, 5, true, true, "Soft 13 vs 5 -> Hit"},
    {15, 8, true, true, "Soft 15 vs 8 -> Hit"},
    {17, 3, true, true, "Soft 17 vs 3 -> Hit"},

    /* Soft 18 - stand vs 2-8, hit vs 9-A */
    {18, 6, true, false, "Soft 18 vs 6 -> Stand"},
    {18, 9, true, true, "Soft 18 vs 9 -> Hit"},
    {18, 11, true, true, "Soft 18 vs A -> Hit"},

    /* Soft 19+ - always stand */
    {19, 9, true, false, "Soft 19 vs 9 -> Stand"},
    {20, 10, true, false, "Soft 20 vs 10 -> Stand"},
};

static int num_tests = sizeof(test_cases) / sizeof(test_cases[0]);
static int current_test = 0;
static int passed = 0;
static int failed = 0;

/*
 * read_inputs() - Called by controller at each step
 * Sets up next test case and validates previous decision
 */
void read_inputs(void) {
    /* Check previous decision (except first call) */
    if (current_test > 0) {
        TestCase* tc = &test_cases[current_test - 1];
        bool correct = (shouldHit == tc->should_hit);

        if (correct) {
            passed++;
        } else {
            failed++;
            printf("WRONG: %s, got %s\n",
                   tc->description,
                   shouldHit ? "Hit" : "Stand");
        }
    }

    /* Check if all tests done */
    if (current_test >= num_tests) {
        printf("\nResults: %d/%d correct\n", passed, num_tests);
        if (failed == 0) {
            printf("SUCCESS: All decisions correct in %d steps\n", num_tests);
            exit(0);
        } else {
            printf("FAIL: %d incorrect decisions\n", failed);
            exit(1);
        }
    }

    /* Set up next test case */
    TestCase* tc = &test_cases[current_test];
    handValue = tc->hand;
    dealerCard = tc->dealer;
    isSoft = tc->soft;
    current_test++;
}


/* ======================================== CONTROLLER ======================================== */
#include <stdlib.h>
int dealerCard = 0;
int handValue = 0;
bool isSoft = false;
bool shouldHit = false;
int main() {
  {
    int prog_counter = 0;
    prog_counter = 2;
    for(;;)
      {
        if ((prog_counter == 1))
          {
            read_inputs();
            prog_counter = 1;
            shouldHit = true;
            continue;
          }
        if ((prog_counter == 2))
          {
            read_inputs();
            prog_counter = (((2 <= dealerCard) && (dealerCard < 12)) ? (((4 <= handValue) && (handValue < 22)) ? 2 : 1) : 1);
            shouldHit = (((2 <= dealerCard) && (dealerCard < 12)) ? (((4 <= handValue) && (handValue < 22)) ? (((((((!(isSoft)) && (handValue < 12)) || ((!(isSoft)) && (handValue == 12)&& (!(((4 <= dealerCard) && (dealerCard < 7)))))) || ((!(isSoft)) && (13 <= handValue)&& (handValue < 17)&& (!(((2 <= dealerCard) && (dealerCard < 7)))))) || (isSoft && (handValue < 18))) || (isSoft && (handValue == 18)&& (9 <= dealerCard)&& (dealerCard < 12))) ? true : false) : true) : true);
            continue;
          }
        if ((prog_counter == 1))
          {
            for(;;)
              {
                if ((prog_counter == 1))
                  break;
                if ((prog_counter == 2))
                  break;
                abort();
              }
            continue;
          }
        if ((prog_counter == 2))
          {
            for(;;)
              {
                if ((prog_counter == 1))
                  break;
                if ((prog_counter == 2))
                  break;
                abort();
              }
            continue;
          }
        abort();
      }
  }
}
/* ======================================== CONTROLLER END ======================================== */
