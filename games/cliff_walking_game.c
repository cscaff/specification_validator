/*
 * Cliff Walking Game - Automated Validation Harness
 *
 * Runs the synthesized controller and validates:
 * - Goal is reached within step limit
 * - Robot never falls off cliff
 * - Robot stays in bounds
 *
 * Exit codes:
 *   0 = Success (goal reached)
 *   1 = Fell off cliff
 *   2 = Out of bounds
 *   3 = Step timeout
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/* Configuration */
#ifndef GRID_COLS
#define GRID_COLS 12
#endif

#ifndef GRID_ROWS
#define GRID_ROWS 4
#endif

#ifndef GOAL_X
#define GOAL_X 11
#endif

#ifndef GOAL_Y
#define GOAL_Y 0
#endif

#ifndef CLIFF_Y
#define CLIFF_Y 0
#endif

#ifndef CLIFF_X_MIN
#define CLIFF_X_MIN 1
#endif

#ifndef CLIFF_X_MAX
#define CLIFF_X_MAX 10
#endif

#define MAX_STEPS 1000

/* Step counter */
static int step_count = 0;

/* Forward declarations for controller state variables */
extern bool atGoal;
extern int x;
extern int y;

/* Check if position is on cliff */
static bool is_cliff(int px, int py) {
    return py == CLIFF_Y && px >= CLIFF_X_MIN && px <= CLIFF_X_MAX;
}

/* Check if position is in bounds */
static bool in_bounds(int px, int py) {
    return px >= 0 && px < GRID_COLS && py >= 0 && py < GRID_ROWS;
}

/* Check if at goal */
static bool at_goal(int px, int py) {
    return px == GOAL_X && py == GOAL_Y;
}

/*
 * read_inputs() - Called by controller at each step
 * Performs validation and handles termination
 */
void read_inputs(void) {
    step_count++;

    /* Check for cliff */
    if (is_cliff(x, y)) {
        printf("FAIL: Fell off cliff at (%d,%d) after %d steps\n", x, y, step_count);
        exit(1);
    }

    /* Check bounds */
    if (!in_bounds(x, y)) {
        printf("FAIL: Out of bounds at (%d,%d) after %d steps\n", x, y, step_count);
        exit(2);
    }

    /* Check for goal */
    if (at_goal(x, y) || atGoal) {
        printf("SUCCESS: Goal reached at (%d,%d) in %d steps\n", x, y, step_count);
        exit(0);
    }

    /* Check step limit */
    if (step_count >= MAX_STEPS) {
        printf("FAIL: Step timeout (%d steps) at (%d,%d)\n", MAX_STEPS, x, y);
        exit(3);
    }
}

/* Rename controller's main to step_controller */
#define main step_controller

/* Controller will be embedded below by the pipeline */
