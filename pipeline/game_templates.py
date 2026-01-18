"""
Game Templates module for the Spec Validator Pipeline.

Generates C game harness files from configuration parameters.
Each game type has its own template function that takes configuration
parameters and produces a complete C game file (without the controller).
"""

from typing import Any


def generate_ice_lake_game(params: dict[str, Any]) -> str:
    """
    Generate an Ice Lake game harness.

    Args:
        params: Configuration with grid_size, goal, holes

    Returns:
        Complete C game harness string (controller appended separately)
    """
    grid_size = params.get("grid_size", 4)
    goal = params.get("goal", {"x": 3, "y": 3})
    holes = params.get("holes", [{"x": 1, "y": 1}])

    goal_x = goal.get("x", grid_size - 1)
    goal_y = goal.get("y", grid_size - 1)

    # Generate hole arrays
    hole_count = len(holes)
    if hole_count > 0:
        x_holes = "{" + ", ".join(str(h["x"]) for h in holes) + "};"
        y_holes = "{" + ", ".join(str(h["y"]) for h in holes) + "};"
    else:
        x_holes = "{-1};"
        y_holes = "{-1};"

    game = f'''/*
 * Ice Lake Game - Automated Validation Harness
 * Generated from configuration parameters
 *
 * Grid: {grid_size}x{grid_size}, Goal: ({goal_x},{goal_y}), Holes: {hole_count}
 *
 * Exit codes:
 *   0 = Success (goal reached)
 *   1 = Fell into hole
 *   2 = Out of bounds
 *   3 = Step timeout
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/* Configuration from pipeline */
#define GRID_SIZE {grid_size}
#define GOAL_X {goal_x}
#define GOAL_Y {goal_y}
#define MAX_STEPS 1000
#define NUM_HOLES {hole_count}

static int hole_x[] = {x_holes}
static int hole_y[] = {y_holes}
static int num_holes = NUM_HOLES;

/* Step counter */
static int step_count = 0;

/* Forward declarations for controller state variables */
extern bool atGoal;
extern int x;
extern int y;

/* Check if position is a hole */
static bool is_hole(int px, int py) {{
    for (int i = 0; i < num_holes; i++) {{
        if (hole_x[i] >= 0 && px == hole_x[i] && py == hole_y[i]) {{
            return true;
        }}
    }}
    return false;
}}

/* Check if position is in bounds */
static bool in_bounds(int px, int py) {{
    return px >= 0 && px < GRID_SIZE && py >= 0 && py < GRID_SIZE;
}}

/* Check if at goal */
static bool at_goal(int px, int py) {{
    return px == GOAL_X && py == GOAL_Y;
}}

/*
 * read_inputs() - Called by controller at each step
 * Performs validation and handles termination
 */
void read_inputs(void) {{
    step_count++;

    // Print current position for debugging
    printf("Step %d: Position (%d,%d)\\n", step_count, x, y);

    /* Check for hole */
    if (is_hole(x, y)) {{
        printf("FAIL: Fell into hole at (%d,%d) after %d steps\\n", x, y, step_count);
        exit(1);
    }}

    /* Check bounds */
    if (!in_bounds(x, y)) {{
        printf("FAIL: Out of bounds at (%d,%d) after %d steps\\n", x, y, step_count);
        exit(2);
    }}

    /* Check for goal */
    if (at_goal(x, y) || atGoal) {{
        printf("SUCCESS: Goal reached at (%d,%d) in %d steps\\n", x, y, step_count);
        exit(0);
    }}

    /* Check step limit */
    if (step_count >= MAX_STEPS) {{
        printf("FAIL: Step timeout (%d steps) at (%d,%d)\\n", MAX_STEPS, x, y);
        exit(3);
    }}
}}

'''
    return game


def generate_taxi_game(params: dict[str, Any]) -> str:
    """
    Generate a Taxi game harness.

    Args:
        params: Configuration with grid_size, pickup, dropoff, barriers

    Returns:
        Complete C game harness string (controller appended separately)
    """
    grid_size = params.get("grid_size", 5)
    pickup = params.get("pickup", params.get("PickUp", {"x": 0, "y": 0}))
    dropoff = params.get("dropoff", params.get("Dropoff", {"x": 4, "y": 4}))
    barriers = params.get("barriers", params.get("Barriers", []))

    pickup_x = pickup.get("x", 0)
    pickup_y = pickup.get("y", 0)
    dropoff_x = dropoff.get("x", grid_size - 1)
    dropoff_y = dropoff.get("y", grid_size - 1)

    # Generate barrier arrays
    barrier_count = len(barriers)
    if barrier_count > 0:
        x_barriers = "{" + ", ".join(str(b["x"]) for b in barriers) + "};"
        y_barriers = "{" + ", ".join(str(b["y"]) for b in barriers) + "};"
    else:
        x_barriers = "{-1};"
        y_barriers = "{-1};"

    game = f'''/*
 * Taxi Game - Automated Validation Harness
 * Generated from configuration parameters
 *
 * Grid: {grid_size}x{grid_size}, Pickup: ({pickup_x},{pickup_y}), Dropoff: ({dropoff_x},{dropoff_y}), Barriers: {barrier_count}
 *
 * Exit codes:
 *   0 = Success (passenger delivered)
 *   1 = Hit barrier
 *   2 = Out of bounds
 *   3 = Step timeout
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/* Configuration from pipeline */
#define GRID_SIZE {grid_size}
#define PICKUP_X {pickup_x}
#define PICKUP_Y {pickup_y}
#define DEST_X {dropoff_x}
#define DEST_Y {dropoff_y}
#define MAX_STEPS 1000
#define NUM_BARRIERS {barrier_count}

static int barrier_x[] = {x_barriers}
static int barrier_y[] = {y_barriers}
static int num_barriers = NUM_BARRIERS;

/* Step counter */
static int step_count = 0;

/* Forward declarations for controller state variables */
extern bool hasPassenger;
extern bool delivered;
extern int x;
extern int y;

/* Check if position is a barrier */
static bool is_barrier(int px, int py) {{
    for (int i = 0; i < num_barriers; i++) {{
        if (barrier_x[i] >= 0 && px == barrier_x[i] && py == barrier_y[i]) {{
            return true;
        }}
    }}
    return false;
}}

/* Check if position is in bounds */
static bool in_bounds(int px, int py) {{
    return px >= 0 && px < GRID_SIZE && py >= 0 && py < GRID_SIZE;
}}

/*
 * read_inputs() - Called by controller at each step
 * Performs validation and handles termination
 */
void read_inputs(void) {{
    step_count++;

    // Print current position for debugging
    printf("Step %d: Position (%d,%d)\\n", step_count, x, y);

    /* Check for barrier */
    if (is_barrier(x, y)) {{
        printf("FAIL: Hit barrier at (%d,%d) after %d steps\\n", x, y, step_count);
        exit(1);
    }}

    /* Check bounds */
    if (!in_bounds(x, y)) {{
        printf("FAIL: Out of bounds at (%d,%d) after %d steps\\n", x, y, step_count);
        exit(2);
    }}

    /* Check for delivery complete */
    if (delivered) {{
        printf("SUCCESS: Passenger delivered in %d steps\\n", step_count);
        exit(0);
    }}

    /* Check step limit */
    if (step_count >= MAX_STEPS) {{
        printf("FAIL: Step timeout (%d steps) at (%d,%d), hasPassenger=%d\\n",
               MAX_STEPS, x, y, hasPassenger);
        exit(3);
    }}
}}

'''
    return game


def generate_cliff_walking_game(params: dict[str, Any]) -> str:
    """
    Generate a Cliff Walking game harness.

    Args:
        params: Configuration with grid_size, grid_rows, cliff_min, cliff_max, goal_pos

    Returns:
        Complete C game harness string (controller appended separately)
    """
    grid_cols = params.get("grid_size", 12)
    grid_rows = params.get("grid_rows", 4)
    cliff_min = params.get("cliff_min", 1)
    cliff_max = params.get("cliff_max", 10)
    goal_pos = params.get("goal_pos", {"x": 11, "y": 0})

    goal_x = goal_pos.get("x", grid_cols - 1)
    goal_y = goal_pos.get("y", 0)

    game = f'''/*
 * Cliff Walking Game - Automated Validation Harness
 * Generated from configuration parameters
 *
 * Grid: {grid_cols}x{grid_rows}, Goal: ({goal_x},{goal_y}), Cliff: y=0, x=[{cliff_min},{cliff_max}]
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

/* Configuration from pipeline */
#define GRID_COLS {grid_cols}
#define GRID_ROWS {grid_rows}
#define GOAL_X {goal_x}
#define GOAL_Y {goal_y}
#define CLIFF_Y 0
#define CLIFF_X_MIN {cliff_min}
#define CLIFF_X_MAX {cliff_max}
#define MAX_STEPS 1000

/* Step counter */
static int step_count = 0;

/* Forward declarations for controller state variables */
extern bool atGoal;
extern int x;
extern int y;

/* Check if position is on cliff */
static bool is_cliff(int px, int py) {{
    return py == CLIFF_Y && px >= CLIFF_X_MIN && px <= CLIFF_X_MAX;
}}

/* Check if position is in bounds */
static bool in_bounds(int px, int py) {{
    return px >= 0 && px < GRID_COLS && py >= 0 && py < GRID_ROWS;
}}

/* Check if at goal */
static bool at_goal(int px, int py) {{
    return px == GOAL_X && py == GOAL_Y;
}}

/*
 * read_inputs() - Called by controller at each step
 * Performs validation and handles termination
 */
void read_inputs(void) {{
    step_count++;

    // Print current position for debugging
    printf("Step %d: Position (%d,%d)\\n", step_count, x, y);

    /* Check for cliff */
    if (is_cliff(x, y)) {{
        printf("FAIL: Fell off cliff at (%d,%d) after %d steps\\n", x, y, step_count);
        exit(1);
    }}

    /* Check bounds */
    if (!in_bounds(x, y)) {{
        printf("FAIL: Out of bounds at (%d,%d) after %d steps\\n", x, y, step_count);
        exit(2);
    }}

    /* Check for goal */
    if (at_goal(x, y) || atGoal) {{
        printf("SUCCESS: Goal reached at (%d,%d) in %d steps\\n", x, y, step_count);
        exit(0);
    }}

    /* Check step limit */
    if (step_count >= MAX_STEPS) {{
        printf("FAIL: Step timeout (%d steps) at (%d,%d)\\n", MAX_STEPS, x, y);
        exit(3);
    }}
}}

'''
    return game


def generate_blackjack_game(params: dict[str, Any]) -> str:
    """
    Generate a Blackjack game harness.

    Args:
        params: Configuration (minimal for blackjack)

    Returns:
        Complete C game harness string (controller appended separately)
    """
    game = '''/*
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
            printf("WRONG: %s, got %s\\n",
                   tc->description,
                   shouldHit ? "Hit" : "Stand");
        }
    }

    /* Check if all tests done */
    if (current_test >= num_tests) {
        printf("\\nResults: %d/%d correct\\n", passed, num_tests);
        if (failed == 0) {
            printf("SUCCESS: All decisions correct in %d steps\\n", num_tests);
            exit(0);
        } else {
            printf("FAIL: %d incorrect decisions\\n", failed);
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

'''
    return game


# Mapping from game name to generator function
GAME_GENERATORS = {
    "ice_lake": generate_ice_lake_game,
    "taxi": generate_taxi_game,
    "cliff_walking": generate_cliff_walking_game,
    "blackjack": generate_blackjack_game,
}


def generate_game(game_name: str, params: dict[str, Any]) -> str:
    """
    Generate a C game harness for the given game and configuration.

    Args:
        game_name: Name of the game (ice_lake, taxi, cliff_walking, blackjack)
        params: Game-specific configuration parameters

    Returns:
        Complete C game harness string (without controller)

    Raises:
        ValueError: If game_name is not recognized
    """
    generator = GAME_GENERATORS.get(game_name.lower())
    if generator is None:
        raise ValueError(f"Unknown game type: {game_name}. Supported: {list(GAME_GENERATORS.keys())}")

    return generator(params)
