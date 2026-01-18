"""
Spec Generator module for the Spec Validator Pipeline.

Generates TSLMT specifications from game configurations.
Each game type has its own generator function that takes configuration
parameters and produces a complete TSLMT spec.
"""

from typing import Any


def generate_ice_lake_spec(params: dict[str, Any], objective: str) -> str:
    """
    Generate an Ice Lake TSLMT specification.

    Args:
        params: Configuration with grid_size, goal, holes, start_pos
        objective: The TSL guarantee objective (e.g., "F atGoal")

    Returns:
        Complete TSLMT specification string
    """
    grid_size = params.get("grid_size", 4)
    goal = params.get("goal", {"x": 3, "y": 3})
    holes = params.get("holes", [{"x": 1, "y": 1}])
    start_pos = params.get("start_pos", {"x": 0, "y": 0})
    start_x = start_pos.get("x", 0)
    start_y = start_pos.get("y", 0)

    goal_x = goal.get("x", grid_size - 1)
    goal_y = goal.get("y", grid_size - 1)
    bound_max = grid_size - 1

    # Generate hole constants
    hole_constants = []
    for i, hole in enumerate(holes):
        hole_constants.append(f"HOLE_{i}_X = i{hole['x']}();")
        hole_constants.append(f"HOLE_{i}_Y = i{hole['y']}();")

    hole_constants_str = "\n".join(hole_constants) if hole_constants else "/* No holes */"

    # Generate atHolePos predicate (OR of all hole positions)
    if len(holes) == 0:
        at_hole_pos = "false"
    elif len(holes) == 1:
        at_hole_pos = "(eq x HOLE_0_X) && (eq y HOLE_0_Y)"
    else:
        hole_checks = [f"((eq x HOLE_{i}_X) && (eq y HOLE_{i}_Y))" for i in range(len(holes))]
        at_hole_pos = " || ".join(hole_checks)

    spec = f'''var Int x
var Int y
var Bool atGoal

SPECIFICATION

/* Ice Lake: Robot navigates {grid_size}x{grid_size} grid avoiding holes to reach the goal */
/* Goal: ({goal_x},{goal_y}), Start: ({start_x},{start_y}), Holes: {len(holes)} */

GOAL_X = i{goal_x}();
GOAL_Y = i{goal_y}();
START_X = i{start_x}();
START_Y = i{start_y}();
BOUND_MIN = i0();
BOUND_MAX = i{bound_max}();

/* Hole positions */
{hole_constants_str}

/* Movement predicates */
stayX = [x <- x];
stayY = [y <- y];
moveRight = [x <- add x i1()] && [y <- y];
moveLeft = [x <- sub x i1()] && [y <- y];
moveUp = [y <- add y i1()] && [x <- x];
moveDown = [y <- sub y i1()] && [x <- x];

/* Position predicates */
atGoalPos = (eq x GOAL_X) && (eq y GOAL_Y);
inBounds = (gte x BOUND_MIN) && (lte x BOUND_MAX) && (gte y BOUND_MIN) && (lte y BOUND_MAX);

/* Hole position check - true if on any hole */
atHolePos = {at_hole_pos};

assume {{
    eq x START_X;
    eq y START_Y;
}}

guarantee {{
    /* Initial state */
    stayX && stayY && [atGoal <- false];

    /* Stay in bounds */
    G inBounds;

    /* Movement: exactly one direction or stay */
    X G ((stayX && stayY) ||
         (moveRight && (lt x BOUND_MAX)) ||
         (moveLeft && (gt x BOUND_MIN)) ||
         (moveUp && (lt y BOUND_MAX)) ||
         (moveDown && (gt y BOUND_MIN)));

    /* Goal flag: set when reaching goal */
    X G (atGoalPos -> [atGoal <- true]);
    X G (atGoal -> [atGoal <- true]);
    X G (!atGoalPos && !atGoal -> [atGoal <- false]);

    /* Objective */
    {objective};
}}
'''
    return spec


def generate_taxi_spec(params: dict[str, Any], objective: str) -> str:
    """
    Generate a Taxi TSLMT specification.

    Args:
        params: Configuration with grid_size, pickup, dropoff, barriers, start_pos
        objective: The TSL guarantee objective (e.g., "F delivered")

    Returns:
        Complete TSLMT specification string
    """
    grid_size = params.get("grid_size", 5)
    pickup = params.get("pickup", params.get("PickUp", {"x": 0, "y": 0}))
    dropoff = params.get("dropoff", params.get("Dropoff", {"x": 4, "y": 4}))
    barriers = params.get("barriers", params.get("Barriers", []))
    start_pos = params.get("start_pos", {"x": 2, "y": 2})

    bound_max = grid_size - 1
    pickup_x = pickup.get("x", 0)
    pickup_y = pickup.get("y", 0)
    dropoff_x = dropoff.get("x", bound_max)
    dropoff_y = dropoff.get("y", bound_max)
    start_x = start_pos.get("x", 2)
    start_y = start_pos.get("y", 2)

    # Generate barrier constants
    barrier_constants = []
    for i, barrier in enumerate(barriers):
        barrier_constants.append(f"BAR_{i}_X = i{barrier['x']}();")
        barrier_constants.append(f"BAR_{i}_Y = i{barrier['y']}();")

    barrier_constants_str = "\n".join(barrier_constants) if barrier_constants else "/* No barriers */"

    # Generate atBarrier predicate
    if len(barriers) == 0:
        at_barrier = "false"
    elif len(barriers) == 1:
        at_barrier = "(eq x BAR_0_X) && (eq y BAR_0_Y)"
    else:
        barrier_checks = [f"((eq x BAR_{i}_X) && (eq y BAR_{i}_Y))" for i in range(len(barriers))]
        at_barrier = " || ".join(barrier_checks)

    spec = f'''var Int x
var Int y
var Bool hasPassenger
var Bool delivered

SPECIFICATION

/* Taxi: Navigate {grid_size}x{grid_size} grid, pickup passenger, deliver to destination */
/* Pickup: ({pickup_x},{pickup_y}), Dropoff: ({dropoff_x},{dropoff_y}), Start: ({start_x},{start_y}), Barriers: {len(barriers)} */

MINB = i0();
MAXB = i{bound_max}();

START_X = i{start_x}();
START_Y = i{start_y}();
PICKUP_X = i{pickup_x}();
PICKUP_Y = i{pickup_y}();
DEST_X = i{dropoff_x}();
DEST_Y = i{dropoff_y}();

/* Barrier positions */
{barrier_constants_str}

/* Position checks */
inBounds = (gte x MINB) && (lte x MAXB) && (gte y MINB) && (lte y MAXB);
atBarrier = {at_barrier};
atPickup = (eq x PICKUP_X) && (eq y PICKUP_Y);
atDest = (eq x DEST_X) && (eq y DEST_Y);

/* Movement */
stayX = [x <- x];
stayY = [y <- y];
moveRight = [x <- add x i1()] && [y <- y];
moveLeft = [x <- sub x i1()] && [y <- y];
moveUp = [y <- add y i1()] && [x <- x];
moveDown = [y <- sub y i1()] && [x <- x];

assume {{
    eq x START_X;
    eq y START_Y;
}}

guarantee {{
    /* Initial */
    stayX && stayY && [hasPassenger <- false] && [delivered <- false];

    /* Safety */
    G inBounds;

    /* Movement */
    X G ((stayX && stayY) ||
         (moveRight && (lt x MAXB)) ||
         (moveLeft && (gt x MINB)) ||
         (moveUp && (lt y MAXB)) ||
         (moveDown && (gt y MINB)));

    /* Pickup */
    X G (!hasPassenger && !delivered && atPickup -> [hasPassenger <- true] && [delivered <- false]);
    X G (!hasPassenger && !delivered && !atPickup -> [hasPassenger <- false] && [delivered <- false]);

    /* Dropoff */
    X G (hasPassenger && !delivered && atDest -> [hasPassenger <- false] && [delivered <- true]);
    X G (hasPassenger && !delivered && !atDest -> [hasPassenger <- true] && [delivered <- false]);

    /* Done */
    X G (delivered -> [hasPassenger <- false] && [delivered <- true] && stayX && stayY);

    /* Objective */
    {objective};
}}
'''
    return spec


def generate_cliff_walking_spec(params: dict[str, Any], objective: str) -> str:
    """
    Generate a Cliff Walking TSLMT specification.

    Args:
        params: Configuration with grid_size, cliff_min, cliff_max, start_pos, goal_pos
        objective: The TSL guarantee objective

    Returns:
        Complete TSLMT specification string
    """
    grid_cols = params.get("grid_size", 12)
    grid_rows = params.get("grid_rows", 4)
    cliff_min = params.get("cliff_min", 1)
    cliff_max = params.get("cliff_max", 10)
    start_pos = params.get("start_pos", {"x": 0, "y": 0})
    goal_pos = params.get("goal_pos", {"x": 11, "y": 0})

    max_x = grid_cols - 1
    max_y = grid_rows - 1
    goal_x = goal_pos.get("x", max_x)
    goal_y = goal_pos.get("y", 0)
    start_x = start_pos.get("x", 0)
    start_y = start_pos.get("y", 0)

    # Cliff is typically at y=0, x from cliff_min to cliff_max
    # If cliff_min == cliff_max == 0, there's no cliff
    has_cliff = cliff_min < cliff_max

    if has_cliff:
        cliff_predicate = f"(eq y CLIFF_Y) && (gte x CLIFF_X_MIN) && (lte x CLIFF_X_MAX)"
    else:
        cliff_predicate = "false"

    spec = f'''var Int x
var Int y
var Bool atGoal

SPECIFICATION

/* Cliff Walking: Robot navigates {grid_cols}x{grid_rows} grid avoiding cliff to reach goal */
/* Goal: ({goal_x},{goal_y}), Start: ({start_x},{start_y}), Cliff: y=0, x=[{cliff_min},{cliff_max}] */

MIN_X = i0();
MAX_X = i{max_x}();
MIN_Y = i0();
MAX_Y = i{max_y}();

START_X = i{start_x}();
START_Y = i{start_y}();
GOAL_X = i{goal_x}();
GOAL_Y = i{goal_y}();

CLIFF_Y = i0();
CLIFF_X_MIN = i{cliff_min}();
CLIFF_X_MAX = i{cliff_max}();

/* Movement predicates */
stayX = [x <- x];
stayY = [y <- y];
moveRight = [x <- add x i1()] && [y <- y];
moveLeft = [x <- sub x i1()] && [y <- y];
moveUp = [y <- add y i1()] && [x <- x];
moveDown = [y <- sub y i1()] && [x <- x];

/* Position predicates */
atGoalPos = (eq x GOAL_X) && (eq y GOAL_Y);
inBounds = (gte x MIN_X) && (lte x MAX_X) && (gte y MIN_Y) && (lte y MAX_Y);

/* Cliff check */
onCliff = {cliff_predicate};

assume {{
    eq x START_X;
    eq y START_Y;
}}

guarantee {{
    /* Initial state */
    stayX && stayY && [atGoal <- false];

    /* Stay in bounds */
    G inBounds;

    /* Never on cliff */
    G !onCliff;

    /* Movement: exactly one direction or stay */
    X G ((stayX && stayY) ||
         (moveRight && (lt x MAX_X)) ||
         (moveLeft && (gt x MIN_X)) ||
         (moveUp && (lt y MAX_Y)) ||
         (moveDown && (gt y MIN_Y)));

    /* Goal flag: set when reaching goal */
    X G (atGoalPos -> [atGoal <- true]);
    X G (atGoal -> [atGoal <- true]);
    X G (!atGoalPos && !atGoal -> [atGoal <- false]);

    /* Once at goal, stay there */
    X G (atGoal -> stayX && stayY);

    /* Objective */
    {objective};
}}
'''
    return spec


def generate_blackjack_spec(params: dict[str, Any], objective: str) -> str:
    """
    Generate a Blackjack TSLMT specification.

    Args:
        params: Configuration (minimal for blackjack)
        objective: The TSL guarantee objective

    Returns:
        Complete TSLMT specification string
    """
    spec = f'''inp Int handValue
inp Int dealerCard
inp Bool isSoft
var Bool shouldHit

SPECIFICATION

/* Blackjack Basic Strategy Controller */

/* Constants for card values */
DEALER_TWO = i2();
DEALER_FOUR = i4();
DEALER_SIX = i6();
DEALER_NINE = i9();
DEALER_ACE = i11();

/* Hand value thresholds */
ELEVEN = i11();
TWELVE = i12();
THIRTEEN = i13();
SIXTEEN = i16();
SEVENTEEN = i17();
EIGHTEEN = i18();

/* Valid input ranges */
MIN_HAND = i4();
MAX_HAND = i21();
MIN_DEALER = i2();
MAX_DEALER = i11();

/* Decision outputs */
hit = [shouldHit <- true];
stand = [shouldHit <- false];

/* Dealer card categories */
dealerWeak = (gte dealerCard DEALER_TWO) && (lte dealerCard DEALER_SIX);
dealerStrong = (gte dealerCard DEALER_NINE) && (lte dealerCard DEALER_ACE);
dealerVeryWeak = (gte dealerCard DEALER_FOUR) && (lte dealerCard DEALER_SIX);

/* Hard hand hit conditions */
hardHitAlways = !isSoft && (lte handValue ELEVEN);
hardHitTwelve = !isSoft && (eq handValue TWELVE) && !dealerVeryWeak;
hardHitThirteenToSixteen = !isSoft && (gte handValue THIRTEEN) && (lte handValue SIXTEEN) && !dealerWeak;

/* Soft hand hit conditions */
softHitSeventeenOrLess = isSoft && (lte handValue SEVENTEEN);
softHitEighteen = isSoft && (eq handValue EIGHTEEN) && dealerStrong;

/* Combined hit condition */
shouldHitCondition = hardHitAlways || hardHitTwelve || hardHitThirteenToSixteen || softHitSeventeenOrLess || softHitEighteen;

assume {{
    (gte handValue MIN_HAND) && (lte handValue MAX_HAND);
    (gte dealerCard MIN_DEALER) && (lte dealerCard MAX_DEALER);
}}

always assume {{
    (gte handValue MIN_HAND) && (lte handValue MAX_HAND);
    (gte dealerCard MIN_DEALER) && (lte dealerCard MAX_DEALER);
}}

guarantee {{
    /* Objective */
    {objective};
}}
'''
    return spec


# Mapping from game name to generator function
SPEC_GENERATORS = {
    "ice_lake": generate_ice_lake_spec,
    "taxi": generate_taxi_spec,
    "cliff_walking": generate_cliff_walking_spec,
    "blackjack": generate_blackjack_spec,
}


def generate_spec(game_name: str, params: dict[str, Any], objective: str) -> str:
    """
    Generate a TSLMT specification for the given game and configuration.

    Args:
        game_name: Name of the game (ice_lake, taxi, cliff_walking, blackjack)
        params: Game-specific configuration parameters
        objective: The TSL guarantee objective

    Returns:
        Complete TSLMT specification string

    Raises:
        ValueError: If game_name is not recognized
    """
    generator = SPEC_GENERATORS.get(game_name.lower())
    if generator is None:
        raise ValueError(f"Unknown game type: {game_name}. Supported: {list(SPEC_GENERATORS.keys())}")

    return generator(params, objective)
