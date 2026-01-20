"""
Spec Generator module for the Spec Validator Pipeline.

Generates TSLMT specifications from game configurations.
Each game type has its own generator function that takes configuration
parameters and produces a complete TSLMT spec.
"""

from typing import Any


def get_default_moves(var_name: str) -> str:
    """Return the default moves expression for a variable: identity, +1, -1."""
    return f"[{var_name} <- {var_name}] || [{var_name} <- add {var_name} i1()] || [{var_name} <- sub {var_name} i1()]"


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
        hole_constants.append(f"hole{i}x = i{hole['x']}();")
        hole_constants.append(f"hole{i}y = i{hole['y']}();")
    hole_constants_str = "\n".join(hole_constants) if hole_constants else "/* No holes */"

    # Get custom variable updates if specified (raw TSL expressions)
    # If not specified, use default: identity, +1, -1
    variable_updates = params.get("variable_updates", {})
    x_moves = variable_updates.get("x", get_default_moves("x"))
    y_moves = variable_updates.get("y", get_default_moves("y"))

    spec = f'''var Int x
var Int y

SPECIFICATION

/* Ice Lake: Robot navigates {grid_size}x{grid_size} grid avoiding holes to reach the goal */
/* Goal: ({goal_x},{goal_y}), Start: ({start_x},{start_y}), Holes: {len(holes)} */

goalx = i{goal_x}();
goaly = i{goal_y}();
startx = i{start_x}();
starty = i{start_y}();
BOUND_MIN = i0();
BOUND_MAX = i{bound_max}();

/* Hole positions */
{hole_constants_str}

/* Position predicates */
inBounds = (gte x BOUND_MIN) && (lte x BOUND_MAX) && (gte y BOUND_MIN) && (lte y BOUND_MAX);

/* Potential Variable Updates */
xMoves = {x_moves};
yMoves = {y_moves};

assume {{
    eq x startx;
    eq y starty;
}}

guarantee {{
    /* Stay in bounds */
    G inBounds;

    /* Movement */
    G ((xMoves && [y <- y]) || ([x <- x] && yMoves));

    /* Objective */
    {objective};
}}
'''
    return spec


def generate_taxi_spec(params: dict[str, Any], objective: str) -> str:
    """
    Generate a Taxi TSLMT specification.

    Args:
        params: Configuration with grid_size, pickup, dropoff, locations, start_pos
        objective: The TSL guarantee objective

    Returns:
        Complete TSLMT specification string
    """
    grid_size = params.get("grid_size", 5)
    pickup = params.get("pickup", params.get("PickUp", {"x": 0, "y": 0}))
    dropoff = params.get("dropoff", params.get("Dropoff", {"x": 4, "y": 4}))
    locations = params.get("locations", {})  # Named locations like {b: {x: 1, y: 2}, y: {x: 3, y: 4}}
    start_pos = params.get("start_pos", {"x": 2, "y": 2})

    bound_max = grid_size - 1
    pickup_x = pickup.get("x", 0)
    pickup_y = pickup.get("y", 0)
    dropoff_x = dropoff.get("x", bound_max)
    dropoff_y = dropoff.get("y", bound_max)
    start_x = start_pos.get("x", 2)
    start_y = start_pos.get("y", 2)

    # Generate location constants (e.g., locbx, locby, locyx, locy)
    location_constants = []
    for loc_name, loc_pos in locations.items():
        location_constants.append(f"loc{loc_name}x = i{loc_pos['x']}();")
        location_constants.append(f"loc{loc_name}y = i{loc_pos['y']}();")
    location_constants_str = "\n".join(location_constants) if location_constants else "/* No additional locations */"

    # Get custom variable updates if specified (raw TSL expressions)
    # If not specified, use default: identity, +1, -1
    variable_updates = params.get("variable_updates", {})
    x_moves = variable_updates.get("x", get_default_moves("x"))
    y_moves = variable_updates.get("y", get_default_moves("y"))

    spec = f'''var Int x
var Int y
var Bool passengerInTaxi

SPECIFICATION

/* Taxi: Navigate {grid_size}x{grid_size} grid, pickup passenger, deliver to destination */
/* Pickup: ({pickup_x},{pickup_y}), Dropoff: ({dropoff_x},{dropoff_y}), Start: ({start_x},{start_y}) */

MINB = i0();
MAXB = i{bound_max}();

startx = i{start_x}();
starty = i{start_y}();
pickupx = i{pickup_x}();
pickupy = i{pickup_y}();
destinationx = i{dropoff_x}();
destinationy = i{dropoff_y}();

/* Additional locations */
{location_constants_str}

/* Position checks */
inBounds = (gte x MINB) && (lte x MAXB) && (gte y MINB) && (lte y MAXB);

/* Potential Variable Updates */
xMoves = {x_moves};
yMoves = {y_moves};

assume {{
    eq x startx;
    eq y starty;
    ! passengerInTaxi;
}}

guarantee {{
    /* Initial state */
    /* [passengerInTaxi <- false]; */

    /* Safety */
    G inBounds;

    /* Movement */
    G ((xMoves && [y <- y]) || ([x <- x] && yMoves));

    /* Pickup logic */
    G (!passengerInTaxi && !((eq x pickupx) && (eq y pickupy)) -> [passengerInTaxi <- false]);
    G ((passengerInTaxi || ((eq x pickupx) && (eq y pickupy))) -> [passengerInTaxi <- true]);

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

    # Get custom variable updates if specified (raw TSL expressions)
    # If not specified, use default: identity, +1, -1
    variable_updates = params.get("variable_updates", {})
    x_moves = variable_updates.get("x", get_default_moves("x"))
    y_moves = variable_updates.get("y", get_default_moves("y"))

    spec = f'''var Int x
var Int y

SPECIFICATION

/* Cliff Walking: Robot navigates {grid_cols}x{grid_rows} grid avoiding cliff to reach goal */
/* Goal: ({goal_x},{goal_y}), Start: ({start_x},{start_y}), Cliff: y=0, x=[{cliff_min},{cliff_max}] */

MIN_X = i0();
MAX_X = i{max_x}();
MIN_Y = i0();
MAX_Y = i{max_y}();

START_X = i{start_x}();
START_Y = i{start_y}();
goalx = i{goal_x}();
goaly = i{goal_y}();

cliffy = i0();
cliffXMin = i{cliff_min}();
cliffXMax = i{cliff_max}();

/* Position predicates */
inBounds = (gte x MIN_X) && (lte x MAX_X) && (gte y MIN_Y) && (lte y MAX_Y);

/* Potential Variable Updates */
xMoves = {x_moves};
yMoves = {y_moves};

assume {{
    eq x START_X;
    eq y START_Y;
}}

guarantee {{
    /* Stay in bounds */
    G inBounds;

    /* Movement */
    G ((xMoves && [y <- y]) || ([x <- x] && yMoves));

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
