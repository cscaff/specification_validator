#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"

/* ========= Configuration ========= */

typedef struct { int x, y; } Cell;

typedef struct {
  int xmin, xmax, ymin, ymax;
  int max_steps;

  // barrier cells (i.e., forbidden positions for taxi)
  Cell *barriers;
  size_t barriers_len;

  // allowed passenger spawn locations
  Cell *valid_passenger_locations;
  size_t valid_passenger_locations_len;

  // allowed destination locations
  Cell *valid_destination_locations;
  size_t valid_destination_locations_len;
} Constraints;

static Constraints G;

/* ========= Game state (matches your TSL vars) ========= */

// Taxi position
int taxiX;
int taxiY;

// Taxi state: 0 empty, 1 has passenger, 2 at destination
int taxiState;
int loc;

// Inputs (environment-chosen, but constrained by JSON)
int passengerX;
int passengerY;
int destX;
int destY;

// Global time
int t = 0;

/* ========= Utility: JSON file reading ========= */

static char *read_entire_file(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) return NULL;

  fseek(f, 0, SEEK_END);
  long n = ftell(f);
  fseek(f, 0, SEEK_SET);

  char *buf = (char*)malloc((size_t)n + 1);
  if (!buf) { fclose(f); return NULL; }

  size_t got = fread(buf, 1, (size_t)n, f);
  buf[got] = '\0';
  fclose(f);
  return buf;
}

static int get_int_or_die(cJSON *obj, const char *key) {
  cJSON *it = cJSON_GetObjectItem(obj, key);
  if (!it || !cJSON_IsNumber(it)) {
    fprintf(stderr, "JSON error: expected number for key '%s'\n", key);
    exit(2);
  }
  return it->valueint;
}

static cJSON *get_obj_or_die(cJSON *obj, const char *key) {
  cJSON *it = cJSON_GetObjectItem(obj, key);
  if (!it || !cJSON_IsObject(it)) {
    fprintf(stderr, "JSON error: expected object for key '%s'\n", key);
    exit(2);
  }
  return it;
}

static cJSON *get_arr_or_die(cJSON *obj, const char *key) {
  cJSON *it = cJSON_GetObjectItem(obj, key);
  if (!it || !cJSON_IsArray(it)) {
    fprintf(stderr, "JSON error: expected array for key '%s'\n", key);
    exit(2);
  }
  return it;
}

static void load_cells_array(cJSON *root, const char *key, Cell **out, size_t *out_len) {
  cJSON *arr = get_arr_or_die(root, key);
  int n = cJSON_GetArraySize(arr);
  if (n < 0) n = 0;

  *out_len = (size_t)n;
  *out = (Cell*)calloc(*out_len, sizeof(Cell));
  if (*out_len > 0 && !*out) {
    fprintf(stderr, "OOM allocating '%s'\n", key);
    exit(2);
  }

  for (int i = 0; i < n; i++) {
    cJSON *it = cJSON_GetArrayItem(arr, i);
    if (!it || !cJSON_IsObject(it)) {
      fprintf(stderr, "JSON error: '%s'[%d] must be an object\n", key, i);
      exit(2);
    }
    (*out)[i].x = get_int_or_die(it, "x");
    (*out)[i].y = get_int_or_die(it, "y");
  }
}

static void load_constraints(const char *path) {
  char *text = read_entire_file(path);
  if (!text) { fprintf(stderr, "Failed to read %s\n", path); exit(2); }

  cJSON *root = cJSON_Parse(text);
  free(text);
  if (!root) { fprintf(stderr, "Invalid JSON\n"); exit(2); }

  cJSON *b = get_obj_or_die(root, "bounds");
  G.xmin = get_int_or_die(b, "xmin");
  G.xmax = get_int_or_die(b, "xmax");
  G.ymin = get_int_or_die(b, "ymin");
  G.ymax = get_int_or_die(b, "ymax");

  cJSON *ms = cJSON_GetObjectItem(root, "max_steps");
  G.max_steps = (ms && cJSON_IsNumber(ms)) ? ms->valueint : 1000;

  load_cells_array(root, "barriers", &G.barriers, &G.barriers_len);
  load_cells_array(root, "valid_passenger_locations",
                   &G.valid_passenger_locations, &G.valid_passenger_locations_len);
  load_cells_array(root, "valid_destination_locations",
                   &G.valid_destination_locations, &G.valid_destination_locations_len);

  // Optional: allow setting passenger/dest explicitly in JSON
  // If omitted, we pick the first entry in each valid list.
  cJSON *p = cJSON_GetObjectItem(root, "passenger");
  if (p && cJSON_IsObject(p)) {
    passengerX = get_int_or_die(p, "x");
    passengerY = get_int_or_die(p, "y");
  } else if (G.valid_passenger_locations_len > 0) {
    passengerX = G.valid_passenger_locations[0].x;
    passengerY = G.valid_passenger_locations[0].y;
  } else {
    fprintf(stderr, "JSON error: no passenger specified and valid_passenger_locations is empty\n");
    exit(2);
  }

  cJSON *d = cJSON_GetObjectItem(root, "destination");
  if (d && cJSON_IsObject(d)) {
    destX = get_int_or_die(d, "x");
    destY = get_int_or_die(d, "y");
  } else if (G.valid_destination_locations_len > 0) {
    destX = G.valid_destination_locations[0].x;
    destY = G.valid_destination_locations[0].y;
  } else {
    fprintf(stderr, "JSON error: no destination specified and valid_destination_locations is empty\n");
    exit(2);
  }

  cJSON_Delete(root);
}

/* ========= Constraint predicates ========= */

static bool in_bounds_cfg(int x, int y) {
  return (x >= G.xmin && x <= G.xmax && y >= G.ymin && y <= G.ymax);
}

static bool is_barrier_cfg(int x, int y) {
  for (size_t i = 0; i < G.barriers_len; i++) {
    if (G.barriers[i].x == x && G.barriers[i].y == y) return true;
  }
  return false;
}

static bool in_allowed_list(Cell *lst, size_t n, int x, int y) {
  for (size_t i = 0; i < n; i++) {
    if (lst[i].x == x && lst[i].y == y) return true;
  }
  return false;
}

static bool valid_taxiState(int s) {
  return (s == 0 || s == 1 || s == 2);
}

static void validate_or_abort(void) {
  if (t > G.max_steps) {
    printf("ABORT: timeout (t=%d)\n", t);
    fflush(stdout);
    exit(255);
  }

  if (!valid_taxiState(taxiState)) {
    printf("ABORT: invalid taxiState=%d (t=%d)\n", taxiState, t);
    fflush(stdout);
    exit(255);
  }

  if (!in_bounds_cfg(taxiX, taxiY)) {
    printf("ABORT: out of bounds (x=%d,y=%d) t=%d\n", taxiX, taxiY, t);
    fflush(stdout);
    exit(255);
  }

  if (is_barrier_cfg(taxiX, taxiY)) {
    printf("ABORT: hit barrier (x=%d,y=%d) t=%d\n", taxiX, taxiY, t);
    fflush(stdout);
    exit(255);
  }

  // Ensure environment inputs are constrained too
  if (!in_bounds_cfg(passengerX, passengerY) ||
      !in_allowed_list(G.valid_passenger_locations, G.valid_passenger_locations_len, passengerX, passengerY)) {
    printf("ABORT: invalid passenger location (x=%d,y=%d)\n", passengerX, passengerY);
    fflush(stdout);
    exit(255);
  }

  if (!in_bounds_cfg(destX, destY) ||
      !in_allowed_list(G.valid_destination_locations, G.valid_destination_locations_len, destX, destY)) {
    printf("ABORT: invalid destination location (x=%d,y=%d)\n", destX, destY);
    fflush(stdout);
    exit(255);
  }
}

/* ========= Hooks for synthesized controller ========= */

void read_inputs(void);
void step_controller(void);

/* ========= IO / logging ========= */

static const char *state_name(int s) {
  switch (s) {
    case 0: return "empty";
    case 1: return "has_passenger";
    case 2: return "at_destination";
    default: return "INVALID";
  }
}

void read_inputs(void) {
  printf("t=%d  taxi=(%d,%d) state=%d(%s)  passenger=(%d,%d) dest=(%d,%d)\n",
         t, taxiX, taxiY, taxiState, state_name(taxiState),
         passengerX, passengerY, destX, destY);
  fflush(stdout);

  validate_or_abort();

  // Terminal success condition: controller sets taxiState to 2 at destination
  if (taxiState == 2) {
    printf("SUCCESS: at destination in %d steps!\n", t);
    fflush(stdout);
    exit(0);
  }

  // Advance time after observing/validating this step
  t++;
}

/* ========= Main ========= */

int main(int argc, char **argv) {
  const char *cfg = (argc >= 2) ? argv[1] : "taxi_constraints.json";
  load_constraints(cfg);

  // Initial state (matches your assume: inbounds && barriers && is_empty)
  taxiX = G.xmin;     // choose something deterministic; can also read from JSON if you want
  taxiY = G.ymin;
  taxiState = 0;     // empty
  t = 0;
  loc = 0;

  // Run controller loop (your synthesized controller should call read_inputs() once per step)
  step_controller();

  return 0;
}
/* ======================================== CONTROLLER ======================================== */
void step_controller() {
  {
    int prog_counter = 0;
    prog_counter = 57;
    for(;;)
      {
        if ((prog_counter == 0))
          {
            read_inputs();
            loc = 2;
            prog_counter = 0;
            taxiState = 2;
            taxiX = (-1 + taxiX);
            taxiY = (1 + taxiY);
            continue;
          }
        if (((prog_counter == 23) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
          {
            read_inputs();
            loc = 0;
            prog_counter = 23;
            taxiState = 2;
            taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
            taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
            continue;
          }
        if (((prog_counter == 26) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
          {
            read_inputs();
            loc = 1;
            prog_counter = 26;
            taxiState = 2;
            taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
            continue;
          }
        if (((prog_counter == 28) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
          {
            read_inputs();
            loc = 2;
            prog_counter = 28;
            taxiState = 2;
            continue;
          }
        if ((prog_counter == 0))
          {
            for(;;)
              {
                if ((prog_counter == 0))
                  break;
                if (((prog_counter == 23) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  break;
                if (((prog_counter == 26) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  break;
                if (((prog_counter == 28) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  break;
                if ((prog_counter == 57))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = 0;
                    taxiState = 2;
                    taxiX = (-1 + taxiX);
                    taxiY = (1 + taxiY);
                    continue;
                  }
                if (((prog_counter == 73) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = 23;
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 75) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = 28;
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 76) && (((taxiY == 0) && (taxiX == 6)&& (taxiState == 1)) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : 26);
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 78) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = 28;
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 80) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = ((!((taxiState == 1))) ? 23 : 73);
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 80) && (0 <= taxiY)&& (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiState == 0)))&& (!((taxiX == 2)))))
                  {
                    {
                      int init_loc = 0;
                      int init_taxiState = 0;
                      int init_taxiX = 0;
                      int init_taxiY = 0;
                      for(;;)
                        {
                          if ((prog_counter == 6))
                            prog_counter = 4;
                          if ((prog_counter == 4))
                            {
                              init_loc = loc;
                              init_taxiState = taxiState;
                              init_taxiX = taxiX;
                              init_taxiY = taxiY;
                            }
                          if (((prog_counter == 4) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                            {
                              prog_counter = 80;
                              break;
                            }
                          if (((prog_counter == 4) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiState == 0)))&& (!((taxiState == 1)))&& (!((taxiState == 2)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiState == 0)))&& (!((taxiState == 1)))&& (!((taxiState == 2)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                            {
                              read_inputs();
                              loc = 0;
                              prog_counter = 6;
                              taxiState = 2;
                              taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                              taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                              continue;
                            }
                          if (((prog_counter == 6) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState < 1)&& (init_taxiState < 3)&& ((init_taxiState + (-1 * taxiState)) < 0)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (3 <= taxiState)&& (1 <= init_taxiState)&& (1 <= (init_taxiState + (-1 * taxiState)))&& (!((taxiState == 0)))&& (!((taxiX == 2)))))))
                            {
                              read_inputs();
                              prog_counter = 6;
                              continue;
                            }
                          abort();
                        }
                    }
                    continue;
                  }
                if (((prog_counter == 66) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = 73;
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 73) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))&& (!((taxiY == 0)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 0)))&& (!((taxiX == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = ((!((taxiState == 1))) ? 23 : 73);
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 65) && (((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : 76);
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 69) && (((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : 76);
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 76) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : ((!((taxiState == 1))) ? 26 : 76));
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 75) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = ((!((taxiState == 1))) ? 28 : 78);
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 78) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = ((!((taxiState == 1))) ? 28 : 78);
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 80) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 65 : ((!((taxiState == 0))) ? ((!((taxiState == 1))) ? ((!((taxiState == 2))) ? 80 : 23) : 73) : 66));
                    taxiState = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 1 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? ((((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiX == 3)))) || ((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiY == -1)))&& (!((taxiY == 0))))) ? (-1 + taxiX) : taxiX) : (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX));
                    taxiY = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 66) && (((0 <= taxiY) && (taxiY < 2)&& (taxiX == 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 65 : ((!((taxiState == 0))) ? 73 : 66));
                    taxiState = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 1 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? ((((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiX == 3)))) || ((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiY == -1)))&& (!((taxiY == 0))))) ? (-1 + taxiX) : taxiX) : (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX));
                    taxiY = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 65) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : ((!((taxiState == 0))) ? 76 : 69));
                    taxiState = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 2 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 69) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : ((!((taxiState == 0))) ? 76 : 69));
                    taxiState = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 2 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                abort();
              }
            continue;
          }
        if (((prog_counter == 23) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
          {
            for(;;)
              {
                if ((prog_counter == 0))
                  break;
                if (((prog_counter == 23) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  break;
                if (((prog_counter == 26) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  break;
                if (((prog_counter == 28) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  break;
                if ((prog_counter == 57))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = 0;
                    taxiState = 2;
                    taxiX = (-1 + taxiX);
                    taxiY = (1 + taxiY);
                    continue;
                  }
                if (((prog_counter == 73) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = 23;
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 75) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = 28;
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 76) && (((taxiY == 0) && (taxiX == 6)&& (taxiState == 1)) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : 26);
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 78) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = 28;
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 80) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = ((!((taxiState == 1))) ? 23 : 73);
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 80) && (0 <= taxiY)&& (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiState == 0)))&& (!((taxiX == 2)))))
                  {
                    {
                      int init_loc = 0;
                      int init_taxiState = 0;
                      int init_taxiX = 0;
                      int init_taxiY = 0;
                      for(;;)
                        {
                          if ((prog_counter == 6))
                            prog_counter = 4;
                          if ((prog_counter == 4))
                            {
                              init_loc = loc;
                              init_taxiState = taxiState;
                              init_taxiX = taxiX;
                              init_taxiY = taxiY;
                            }
                          if (((prog_counter == 4) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                            {
                              prog_counter = 80;
                              break;
                            }
                          if (((prog_counter == 4) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiState == 0)))&& (!((taxiState == 1)))&& (!((taxiState == 2)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiState == 0)))&& (!((taxiState == 1)))&& (!((taxiState == 2)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                            {
                              read_inputs();
                              loc = 0;
                              prog_counter = 6;
                              taxiState = 2;
                              taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                              taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                              continue;
                            }
                          if (((prog_counter == 6) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState < 1)&& (init_taxiState < 3)&& ((init_taxiState + (-1 * taxiState)) < 0)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (3 <= taxiState)&& (1 <= init_taxiState)&& (1 <= (init_taxiState + (-1 * taxiState)))&& (!((taxiState == 0)))&& (!((taxiX == 2)))))))
                            {
                              read_inputs();
                              prog_counter = 6;
                              continue;
                            }
                          abort();
                        }
                    }
                    continue;
                  }
                if (((prog_counter == 66) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = 73;
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 73) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))&& (!((taxiY == 0)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 0)))&& (!((taxiX == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = ((!((taxiState == 1))) ? 23 : 73);
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 65) && (((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : 76);
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 69) && (((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : 76);
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 76) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : ((!((taxiState == 1))) ? 26 : 76));
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 75) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = ((!((taxiState == 1))) ? 28 : 78);
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 78) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = ((!((taxiState == 1))) ? 28 : 78);
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 80) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 65 : ((!((taxiState == 0))) ? ((!((taxiState == 1))) ? ((!((taxiState == 2))) ? 80 : 23) : 73) : 66));
                    taxiState = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 1 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? ((((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiX == 3)))) || ((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiY == -1)))&& (!((taxiY == 0))))) ? (-1 + taxiX) : taxiX) : (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX));
                    taxiY = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 66) && (((0 <= taxiY) && (taxiY < 2)&& (taxiX == 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 65 : ((!((taxiState == 0))) ? 73 : 66));
                    taxiState = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 1 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? ((((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiX == 3)))) || ((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiY == -1)))&& (!((taxiY == 0))))) ? (-1 + taxiX) : taxiX) : (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX));
                    taxiY = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 65) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : ((!((taxiState == 0))) ? 76 : 69));
                    taxiState = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 2 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 69) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : ((!((taxiState == 0))) ? 76 : 69));
                    taxiState = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 2 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                abort();
              }
            continue;
          }
        if (((prog_counter == 26) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
          {
            for(;;)
              {
                if ((prog_counter == 0))
                  break;
                if (((prog_counter == 23) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  break;
                if (((prog_counter == 26) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  break;
                if (((prog_counter == 28) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  break;
                if ((prog_counter == 57))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = 0;
                    taxiState = 2;
                    taxiX = (-1 + taxiX);
                    taxiY = (1 + taxiY);
                    continue;
                  }
                if (((prog_counter == 73) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = 23;
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 75) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = 28;
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 76) && (((taxiY == 0) && (taxiX == 6)&& (taxiState == 1)) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : 26);
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 78) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = 28;
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 80) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = ((!((taxiState == 1))) ? 23 : 73);
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 80) && (0 <= taxiY)&& (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiState == 0)))&& (!((taxiX == 2)))))
                  {
                    {
                      int init_loc = 0;
                      int init_taxiState = 0;
                      int init_taxiX = 0;
                      int init_taxiY = 0;
                      for(;;)
                        {
                          if ((prog_counter == 6))
                            prog_counter = 4;
                          if ((prog_counter == 4))
                            {
                              init_loc = loc;
                              init_taxiState = taxiState;
                              init_taxiX = taxiX;
                              init_taxiY = taxiY;
                            }
                          if (((prog_counter == 4) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                            {
                              prog_counter = 80;
                              break;
                            }
                          if (((prog_counter == 4) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiState == 0)))&& (!((taxiState == 1)))&& (!((taxiState == 2)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiState == 0)))&& (!((taxiState == 1)))&& (!((taxiState == 2)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                            {
                              read_inputs();
                              loc = 0;
                              prog_counter = 6;
                              taxiState = 2;
                              taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                              taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                              continue;
                            }
                          if (((prog_counter == 6) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState < 1)&& (init_taxiState < 3)&& ((init_taxiState + (-1 * taxiState)) < 0)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (3 <= taxiState)&& (1 <= init_taxiState)&& (1 <= (init_taxiState + (-1 * taxiState)))&& (!((taxiState == 0)))&& (!((taxiX == 2)))))))
                            {
                              read_inputs();
                              prog_counter = 6;
                              continue;
                            }
                          abort();
                        }
                    }
                    continue;
                  }
                if (((prog_counter == 66) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = 73;
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 73) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))&& (!((taxiY == 0)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 0)))&& (!((taxiX == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = ((!((taxiState == 1))) ? 23 : 73);
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 65) && (((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : 76);
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 69) && (((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : 76);
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 76) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : ((!((taxiState == 1))) ? 26 : 76));
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 75) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = ((!((taxiState == 1))) ? 28 : 78);
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 78) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = ((!((taxiState == 1))) ? 28 : 78);
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 80) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 65 : ((!((taxiState == 0))) ? ((!((taxiState == 1))) ? ((!((taxiState == 2))) ? 80 : 23) : 73) : 66));
                    taxiState = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 1 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? ((((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiX == 3)))) || ((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiY == -1)))&& (!((taxiY == 0))))) ? (-1 + taxiX) : taxiX) : (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX));
                    taxiY = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 66) && (((0 <= taxiY) && (taxiY < 2)&& (taxiX == 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 65 : ((!((taxiState == 0))) ? 73 : 66));
                    taxiState = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 1 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? ((((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiX == 3)))) || ((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiY == -1)))&& (!((taxiY == 0))))) ? (-1 + taxiX) : taxiX) : (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX));
                    taxiY = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 65) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : ((!((taxiState == 0))) ? 76 : 69));
                    taxiState = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 2 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 69) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : ((!((taxiState == 0))) ? 76 : 69));
                    taxiState = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 2 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                abort();
              }
            continue;
          }
        if (((prog_counter == 28) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
          {
            for(;;)
              {
                if ((prog_counter == 0))
                  break;
                if (((prog_counter == 23) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  break;
                if (((prog_counter == 26) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  break;
                if (((prog_counter == 28) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  break;
                if ((prog_counter == 57))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = 0;
                    taxiState = 2;
                    taxiX = (-1 + taxiX);
                    taxiY = (1 + taxiY);
                    continue;
                  }
                if (((prog_counter == 73) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = 23;
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 75) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = 28;
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 76) && (((taxiY == 0) && (taxiX == 6)&& (taxiState == 1)) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : 26);
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 78) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = 28;
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 80) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = ((!((taxiState == 1))) ? 23 : 73);
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 80) && (0 <= taxiY)&& (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiState == 0)))&& (!((taxiX == 2)))))
                  {
                    {
                      int init_loc = 0;
                      int init_taxiState = 0;
                      int init_taxiX = 0;
                      int init_taxiY = 0;
                      for(;;)
                        {
                          if ((prog_counter == 6))
                            prog_counter = 4;
                          if ((prog_counter == 4))
                            {
                              init_loc = loc;
                              init_taxiState = taxiState;
                              init_taxiX = taxiX;
                              init_taxiY = taxiY;
                            }
                          if (((prog_counter == 4) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                            {
                              prog_counter = 80;
                              break;
                            }
                          if (((prog_counter == 4) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiState == 0)))&& (!((taxiState == 1)))&& (!((taxiState == 2)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiState == 0)))&& (!((taxiState == 1)))&& (!((taxiState == 2)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                            {
                              read_inputs();
                              loc = 0;
                              prog_counter = 6;
                              taxiState = 2;
                              taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                              taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                              continue;
                            }
                          if (((prog_counter == 6) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState < 1)&& (init_taxiState < 3)&& ((init_taxiState + (-1 * taxiState)) < 0)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (3 <= taxiState)&& (1 <= init_taxiState)&& (1 <= (init_taxiState + (-1 * taxiState)))&& (!((taxiState == 0)))&& (!((taxiX == 2)))))))
                            {
                              read_inputs();
                              prog_counter = 6;
                              continue;
                            }
                          abort();
                        }
                    }
                    continue;
                  }
                if (((prog_counter == 66) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = 73;
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 73) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))&& (!((taxiY == 0)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 0)))&& (!((taxiX == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = ((!((taxiState == 1))) ? 23 : 73);
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 65) && (((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : 76);
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 69) && (((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : 76);
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 76) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : ((!((taxiState == 1))) ? 26 : 76));
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 75) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = ((!((taxiState == 1))) ? 28 : 78);
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 78) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = ((!((taxiState == 1))) ? 28 : 78);
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 80) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 65 : ((!((taxiState == 0))) ? ((!((taxiState == 1))) ? ((!((taxiState == 2))) ? 80 : 23) : 73) : 66));
                    taxiState = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 1 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? ((((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiX == 3)))) || ((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiY == -1)))&& (!((taxiY == 0))))) ? (-1 + taxiX) : taxiX) : (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX));
                    taxiY = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 66) && (((0 <= taxiY) && (taxiY < 2)&& (taxiX == 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 65 : ((!((taxiState == 0))) ? 73 : 66));
                    taxiState = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 1 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? ((((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiX == 3)))) || ((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiY == -1)))&& (!((taxiY == 0))))) ? (-1 + taxiX) : taxiX) : (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX));
                    taxiY = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 65) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : ((!((taxiState == 0))) ? 76 : 69));
                    taxiState = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 2 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 69) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : ((!((taxiState == 0))) ? 76 : 69));
                    taxiState = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 2 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                abort();
              }
            continue;
          }
        if ((prog_counter == 57))
          {
            for(;;)
              {
                if ((prog_counter == 0))
                  break;
                if (((prog_counter == 23) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  break;
                if (((prog_counter == 26) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  break;
                if (((prog_counter == 28) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  break;
                if ((prog_counter == 57))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = 0;
                    taxiState = 2;
                    taxiX = (-1 + taxiX);
                    taxiY = (1 + taxiY);
                    continue;
                  }
                if (((prog_counter == 73) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = 23;
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 75) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = 28;
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 76) && (((taxiY == 0) && (taxiX == 6)&& (taxiState == 1)) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : 26);
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 78) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = 28;
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 80) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = ((!((taxiState == 1))) ? 23 : 73);
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 80) && (0 <= taxiY)&& (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiState == 0)))&& (!((taxiX == 2)))))
                  {
                    {
                      int init_loc = 0;
                      int init_taxiState = 0;
                      int init_taxiX = 0;
                      int init_taxiY = 0;
                      for(;;)
                        {
                          if ((prog_counter == 6))
                            prog_counter = 4;
                          if ((prog_counter == 4))
                            {
                              init_loc = loc;
                              init_taxiState = taxiState;
                              init_taxiX = taxiX;
                              init_taxiY = taxiY;
                            }
                          if (((prog_counter == 4) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                            {
                              prog_counter = 80;
                              break;
                            }
                          if (((prog_counter == 4) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiState == 0)))&& (!((taxiState == 1)))&& (!((taxiState == 2)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiState == 0)))&& (!((taxiState == 1)))&& (!((taxiState == 2)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                            {
                              read_inputs();
                              loc = 0;
                              prog_counter = 6;
                              taxiState = 2;
                              taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                              taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                              continue;
                            }
                          if (((prog_counter == 6) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState < 1)&& (init_taxiState < 3)&& ((init_taxiState + (-1 * taxiState)) < 0)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (3 <= taxiState)&& (1 <= init_taxiState)&& (1 <= (init_taxiState + (-1 * taxiState)))&& (!((taxiState == 0)))&& (!((taxiX == 2)))))))
                            {
                              read_inputs();
                              prog_counter = 6;
                              continue;
                            }
                          abort();
                        }
                    }
                    continue;
                  }
                if (((prog_counter == 66) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = 73;
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 73) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))&& (!((taxiY == 0)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 0)))&& (!((taxiX == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = ((!((taxiState == 1))) ? 23 : 73);
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 65) && (((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : 76);
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 69) && (((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : 76);
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 76) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : ((!((taxiState == 1))) ? 26 : 76));
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 75) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = ((!((taxiState == 1))) ? 28 : 78);
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 78) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = ((!((taxiState == 1))) ? 28 : 78);
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 80) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 65 : ((!((taxiState == 0))) ? ((!((taxiState == 1))) ? ((!((taxiState == 2))) ? 80 : 23) : 73) : 66));
                    taxiState = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 1 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? ((((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiX == 3)))) || ((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiY == -1)))&& (!((taxiY == 0))))) ? (-1 + taxiX) : taxiX) : (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX));
                    taxiY = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 66) && (((0 <= taxiY) && (taxiY < 2)&& (taxiX == 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 65 : ((!((taxiState == 0))) ? 73 : 66));
                    taxiState = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 1 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? ((((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiX == 3)))) || ((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiY == -1)))&& (!((taxiY == 0))))) ? (-1 + taxiX) : taxiX) : (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX));
                    taxiY = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 65) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : ((!((taxiState == 0))) ? 76 : 69));
                    taxiState = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 2 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 69) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : ((!((taxiState == 0))) ? 76 : 69));
                    taxiState = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 2 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                abort();
              }
            continue;
          }
        if (((prog_counter == 65) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
          {
            for(;;)
              {
                if ((prog_counter == 0))
                  break;
                if (((prog_counter == 23) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  break;
                if (((prog_counter == 26) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  break;
                if (((prog_counter == 28) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  break;
                if ((prog_counter == 57))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = 0;
                    taxiState = 2;
                    taxiX = (-1 + taxiX);
                    taxiY = (1 + taxiY);
                    continue;
                  }
                if (((prog_counter == 73) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = 23;
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 75) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = 28;
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 76) && (((taxiY == 0) && (taxiX == 6)&& (taxiState == 1)) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : 26);
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 78) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = 28;
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 80) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = ((!((taxiState == 1))) ? 23 : 73);
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 80) && (0 <= taxiY)&& (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiState == 0)))&& (!((taxiX == 2)))))
                  {
                    {
                      int init_loc = 0;
                      int init_taxiState = 0;
                      int init_taxiX = 0;
                      int init_taxiY = 0;
                      for(;;)
                        {
                          if ((prog_counter == 6))
                            prog_counter = 4;
                          if ((prog_counter == 4))
                            {
                              init_loc = loc;
                              init_taxiState = taxiState;
                              init_taxiX = taxiX;
                              init_taxiY = taxiY;
                            }
                          if (((prog_counter == 4) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                            {
                              prog_counter = 80;
                              break;
                            }
                          if (((prog_counter == 4) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiState == 0)))&& (!((taxiState == 1)))&& (!((taxiState == 2)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiState == 0)))&& (!((taxiState == 1)))&& (!((taxiState == 2)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                            {
                              read_inputs();
                              loc = 0;
                              prog_counter = 6;
                              taxiState = 2;
                              taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                              taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                              continue;
                            }
                          if (((prog_counter == 6) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState < 1)&& (init_taxiState < 3)&& ((init_taxiState + (-1 * taxiState)) < 0)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (3 <= taxiState)&& (1 <= init_taxiState)&& (1 <= (init_taxiState + (-1 * taxiState)))&& (!((taxiState == 0)))&& (!((taxiX == 2)))))))
                            {
                              read_inputs();
                              prog_counter = 6;
                              continue;
                            }
                          abort();
                        }
                    }
                    continue;
                  }
                if (((prog_counter == 66) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = 73;
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 73) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))&& (!((taxiY == 0)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 0)))&& (!((taxiX == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = ((!((taxiState == 1))) ? 23 : 73);
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 65) && (((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : 76);
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 69) && (((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : 76);
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 76) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : ((!((taxiState == 1))) ? 26 : 76));
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 75) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = ((!((taxiState == 1))) ? 28 : 78);
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 78) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = ((!((taxiState == 1))) ? 28 : 78);
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 80) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 65 : ((!((taxiState == 0))) ? ((!((taxiState == 1))) ? ((!((taxiState == 2))) ? 80 : 23) : 73) : 66));
                    taxiState = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 1 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? ((((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiX == 3)))) || ((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiY == -1)))&& (!((taxiY == 0))))) ? (-1 + taxiX) : taxiX) : (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX));
                    taxiY = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 66) && (((0 <= taxiY) && (taxiY < 2)&& (taxiX == 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 65 : ((!((taxiState == 0))) ? 73 : 66));
                    taxiState = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 1 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? ((((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiX == 3)))) || ((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiY == -1)))&& (!((taxiY == 0))))) ? (-1 + taxiX) : taxiX) : (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX));
                    taxiY = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 65) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : ((!((taxiState == 0))) ? 76 : 69));
                    taxiState = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 2 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 69) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : ((!((taxiState == 0))) ? 76 : 69));
                    taxiState = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 2 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                abort();
              }
            continue;
          }
        if (((prog_counter == 66) && (((0 <= taxiY) && (taxiY < 2)&& (taxiX == 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
          {
            for(;;)
              {
                if ((prog_counter == 0))
                  break;
                if (((prog_counter == 23) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  break;
                if (((prog_counter == 26) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  break;
                if (((prog_counter == 28) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  break;
                if ((prog_counter == 57))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = 0;
                    taxiState = 2;
                    taxiX = (-1 + taxiX);
                    taxiY = (1 + taxiY);
                    continue;
                  }
                if (((prog_counter == 73) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = 23;
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 75) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = 28;
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 76) && (((taxiY == 0) && (taxiX == 6)&& (taxiState == 1)) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : 26);
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 78) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = 28;
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 80) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = ((!((taxiState == 1))) ? 23 : 73);
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 80) && (0 <= taxiY)&& (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiState == 0)))&& (!((taxiX == 2)))))
                  {
                    {
                      int init_loc = 0;
                      int init_taxiState = 0;
                      int init_taxiX = 0;
                      int init_taxiY = 0;
                      for(;;)
                        {
                          if ((prog_counter == 6))
                            prog_counter = 4;
                          if ((prog_counter == 4))
                            {
                              init_loc = loc;
                              init_taxiState = taxiState;
                              init_taxiX = taxiX;
                              init_taxiY = taxiY;
                            }
                          if (((prog_counter == 4) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                            {
                              prog_counter = 80;
                              break;
                            }
                          if (((prog_counter == 4) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiState == 0)))&& (!((taxiState == 1)))&& (!((taxiState == 2)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiState == 0)))&& (!((taxiState == 1)))&& (!((taxiState == 2)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                            {
                              read_inputs();
                              loc = 0;
                              prog_counter = 6;
                              taxiState = 2;
                              taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                              taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                              continue;
                            }
                          if (((prog_counter == 6) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState < 1)&& (init_taxiState < 3)&& ((init_taxiState + (-1 * taxiState)) < 0)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (3 <= taxiState)&& (1 <= init_taxiState)&& (1 <= (init_taxiState + (-1 * taxiState)))&& (!((taxiState == 0)))&& (!((taxiX == 2)))))))
                            {
                              read_inputs();
                              prog_counter = 6;
                              continue;
                            }
                          abort();
                        }
                    }
                    continue;
                  }
                if (((prog_counter == 66) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = 73;
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 73) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))&& (!((taxiY == 0)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 0)))&& (!((taxiX == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = ((!((taxiState == 1))) ? 23 : 73);
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 65) && (((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : 76);
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 69) && (((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : 76);
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 76) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : ((!((taxiState == 1))) ? 26 : 76));
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 75) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = ((!((taxiState == 1))) ? 28 : 78);
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 78) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = ((!((taxiState == 1))) ? 28 : 78);
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 80) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 65 : ((!((taxiState == 0))) ? ((!((taxiState == 1))) ? ((!((taxiState == 2))) ? 80 : 23) : 73) : 66));
                    taxiState = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 1 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? ((((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiX == 3)))) || ((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiY == -1)))&& (!((taxiY == 0))))) ? (-1 + taxiX) : taxiX) : (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX));
                    taxiY = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 66) && (((0 <= taxiY) && (taxiY < 2)&& (taxiX == 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 65 : ((!((taxiState == 0))) ? 73 : 66));
                    taxiState = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 1 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? ((((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiX == 3)))) || ((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiY == -1)))&& (!((taxiY == 0))))) ? (-1 + taxiX) : taxiX) : (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX));
                    taxiY = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 65) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : ((!((taxiState == 0))) ? 76 : 69));
                    taxiState = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 2 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 69) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : ((!((taxiState == 0))) ? 76 : 69));
                    taxiState = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 2 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                abort();
              }
            continue;
          }
        if (((prog_counter == 69) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
          {
            for(;;)
              {
                if ((prog_counter == 0))
                  break;
                if (((prog_counter == 23) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  break;
                if (((prog_counter == 26) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  break;
                if (((prog_counter == 28) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  break;
                if ((prog_counter == 57))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = 0;
                    taxiState = 2;
                    taxiX = (-1 + taxiX);
                    taxiY = (1 + taxiY);
                    continue;
                  }
                if (((prog_counter == 73) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = 23;
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 75) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = 28;
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 76) && (((taxiY == 0) && (taxiX == 6)&& (taxiState == 1)) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : 26);
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 78) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = 28;
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 80) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = ((!((taxiState == 1))) ? 23 : 73);
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 80) && (0 <= taxiY)&& (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiState == 0)))&& (!((taxiX == 2)))))
                  {
                    {
                      int init_loc = 0;
                      int init_taxiState = 0;
                      int init_taxiX = 0;
                      int init_taxiY = 0;
                      for(;;)
                        {
                          if ((prog_counter == 6))
                            prog_counter = 4;
                          if ((prog_counter == 4))
                            {
                              init_loc = loc;
                              init_taxiState = taxiState;
                              init_taxiX = taxiX;
                              init_taxiY = taxiY;
                            }
                          if (((prog_counter == 4) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                            {
                              prog_counter = 80;
                              break;
                            }
                          if (((prog_counter == 4) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiState == 0)))&& (!((taxiState == 1)))&& (!((taxiState == 2)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiState == 0)))&& (!((taxiState == 1)))&& (!((taxiState == 2)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                            {
                              read_inputs();
                              loc = 0;
                              prog_counter = 6;
                              taxiState = 2;
                              taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                              taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                              continue;
                            }
                          if (((prog_counter == 6) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState < 1)&& (init_taxiState < 3)&& ((init_taxiState + (-1 * taxiState)) < 0)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (3 <= taxiState)&& (1 <= init_taxiState)&& (1 <= (init_taxiState + (-1 * taxiState)))&& (!((taxiState == 0)))&& (!((taxiX == 2)))))))
                            {
                              read_inputs();
                              prog_counter = 6;
                              continue;
                            }
                          abort();
                        }
                    }
                    continue;
                  }
                if (((prog_counter == 66) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = 73;
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 73) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))&& (!((taxiY == 0)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 0)))&& (!((taxiX == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = ((!((taxiState == 1))) ? 23 : 73);
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 65) && (((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : 76);
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 69) && (((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : 76);
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 76) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : ((!((taxiState == 1))) ? 26 : 76));
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 75) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = ((!((taxiState == 1))) ? 28 : 78);
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 78) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = ((!((taxiState == 1))) ? 28 : 78);
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 80) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 65 : ((!((taxiState == 0))) ? ((!((taxiState == 1))) ? ((!((taxiState == 2))) ? 80 : 23) : 73) : 66));
                    taxiState = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 1 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? ((((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiX == 3)))) || ((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiY == -1)))&& (!((taxiY == 0))))) ? (-1 + taxiX) : taxiX) : (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX));
                    taxiY = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 66) && (((0 <= taxiY) && (taxiY < 2)&& (taxiX == 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 65 : ((!((taxiState == 0))) ? 73 : 66));
                    taxiState = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 1 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? ((((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiX == 3)))) || ((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiY == -1)))&& (!((taxiY == 0))))) ? (-1 + taxiX) : taxiX) : (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX));
                    taxiY = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 65) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : ((!((taxiState == 0))) ? 76 : 69));
                    taxiState = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 2 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 69) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : ((!((taxiState == 0))) ? 76 : 69));
                    taxiState = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 2 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                abort();
              }
            continue;
          }
        if (((prog_counter == 73) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))&& (!((taxiY == 0)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 0)))&& (!((taxiX == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2)))))))
          {
            for(;;)
              {
                if ((prog_counter == 0))
                  break;
                if (((prog_counter == 23) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  break;
                if (((prog_counter == 26) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  break;
                if (((prog_counter == 28) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  break;
                if ((prog_counter == 57))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = 0;
                    taxiState = 2;
                    taxiX = (-1 + taxiX);
                    taxiY = (1 + taxiY);
                    continue;
                  }
                if (((prog_counter == 73) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = 23;
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 75) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = 28;
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 76) && (((taxiY == 0) && (taxiX == 6)&& (taxiState == 1)) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : 26);
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 78) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = 28;
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 80) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = ((!((taxiState == 1))) ? 23 : 73);
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 80) && (0 <= taxiY)&& (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiState == 0)))&& (!((taxiX == 2)))))
                  {
                    {
                      int init_loc = 0;
                      int init_taxiState = 0;
                      int init_taxiX = 0;
                      int init_taxiY = 0;
                      for(;;)
                        {
                          if ((prog_counter == 6))
                            prog_counter = 4;
                          if ((prog_counter == 4))
                            {
                              init_loc = loc;
                              init_taxiState = taxiState;
                              init_taxiX = taxiX;
                              init_taxiY = taxiY;
                            }
                          if (((prog_counter == 4) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                            {
                              prog_counter = 80;
                              break;
                            }
                          if (((prog_counter == 4) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiState == 0)))&& (!((taxiState == 1)))&& (!((taxiState == 2)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiState == 0)))&& (!((taxiState == 1)))&& (!((taxiState == 2)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                            {
                              read_inputs();
                              loc = 0;
                              prog_counter = 6;
                              taxiState = 2;
                              taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                              taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                              continue;
                            }
                          if (((prog_counter == 6) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState < 1)&& (init_taxiState < 3)&& ((init_taxiState + (-1 * taxiState)) < 0)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (3 <= taxiState)&& (1 <= init_taxiState)&& (1 <= (init_taxiState + (-1 * taxiState)))&& (!((taxiState == 0)))&& (!((taxiX == 2)))))))
                            {
                              read_inputs();
                              prog_counter = 6;
                              continue;
                            }
                          abort();
                        }
                    }
                    continue;
                  }
                if (((prog_counter == 66) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = 73;
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 73) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))&& (!((taxiY == 0)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 0)))&& (!((taxiX == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = ((!((taxiState == 1))) ? 23 : 73);
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 65) && (((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : 76);
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 69) && (((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : 76);
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 76) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : ((!((taxiState == 1))) ? 26 : 76));
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 75) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = ((!((taxiState == 1))) ? 28 : 78);
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 78) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = ((!((taxiState == 1))) ? 28 : 78);
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 80) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 65 : ((!((taxiState == 0))) ? ((!((taxiState == 1))) ? ((!((taxiState == 2))) ? 80 : 23) : 73) : 66));
                    taxiState = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 1 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? ((((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiX == 3)))) || ((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiY == -1)))&& (!((taxiY == 0))))) ? (-1 + taxiX) : taxiX) : (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX));
                    taxiY = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 66) && (((0 <= taxiY) && (taxiY < 2)&& (taxiX == 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 65 : ((!((taxiState == 0))) ? 73 : 66));
                    taxiState = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 1 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? ((((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiX == 3)))) || ((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiY == -1)))&& (!((taxiY == 0))))) ? (-1 + taxiX) : taxiX) : (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX));
                    taxiY = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 65) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : ((!((taxiState == 0))) ? 76 : 69));
                    taxiState = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 2 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 69) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : ((!((taxiState == 0))) ? 76 : 69));
                    taxiState = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 2 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                abort();
              }
            continue;
          }
        if (((prog_counter == 75) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
          {
            for(;;)
              {
                if ((prog_counter == 0))
                  break;
                if (((prog_counter == 23) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  break;
                if (((prog_counter == 26) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  break;
                if (((prog_counter == 28) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  break;
                if ((prog_counter == 57))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = 0;
                    taxiState = 2;
                    taxiX = (-1 + taxiX);
                    taxiY = (1 + taxiY);
                    continue;
                  }
                if (((prog_counter == 73) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = 23;
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 75) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = 28;
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 76) && (((taxiY == 0) && (taxiX == 6)&& (taxiState == 1)) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : 26);
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 78) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = 28;
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 80) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = ((!((taxiState == 1))) ? 23 : 73);
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 80) && (0 <= taxiY)&& (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiState == 0)))&& (!((taxiX == 2)))))
                  {
                    {
                      int init_loc = 0;
                      int init_taxiState = 0;
                      int init_taxiX = 0;
                      int init_taxiY = 0;
                      for(;;)
                        {
                          if ((prog_counter == 6))
                            prog_counter = 4;
                          if ((prog_counter == 4))
                            {
                              init_loc = loc;
                              init_taxiState = taxiState;
                              init_taxiX = taxiX;
                              init_taxiY = taxiY;
                            }
                          if (((prog_counter == 4) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                            {
                              prog_counter = 80;
                              break;
                            }
                          if (((prog_counter == 4) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiState == 0)))&& (!((taxiState == 1)))&& (!((taxiState == 2)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiState == 0)))&& (!((taxiState == 1)))&& (!((taxiState == 2)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                            {
                              read_inputs();
                              loc = 0;
                              prog_counter = 6;
                              taxiState = 2;
                              taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                              taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                              continue;
                            }
                          if (((prog_counter == 6) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState < 1)&& (init_taxiState < 3)&& ((init_taxiState + (-1 * taxiState)) < 0)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (3 <= taxiState)&& (1 <= init_taxiState)&& (1 <= (init_taxiState + (-1 * taxiState)))&& (!((taxiState == 0)))&& (!((taxiX == 2)))))))
                            {
                              read_inputs();
                              prog_counter = 6;
                              continue;
                            }
                          abort();
                        }
                    }
                    continue;
                  }
                if (((prog_counter == 66) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = 73;
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 73) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))&& (!((taxiY == 0)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 0)))&& (!((taxiX == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = ((!((taxiState == 1))) ? 23 : 73);
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 65) && (((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : 76);
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 69) && (((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : 76);
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 76) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : ((!((taxiState == 1))) ? 26 : 76));
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 75) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = ((!((taxiState == 1))) ? 28 : 78);
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 78) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = ((!((taxiState == 1))) ? 28 : 78);
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 80) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 65 : ((!((taxiState == 0))) ? ((!((taxiState == 1))) ? ((!((taxiState == 2))) ? 80 : 23) : 73) : 66));
                    taxiState = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 1 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? ((((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiX == 3)))) || ((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiY == -1)))&& (!((taxiY == 0))))) ? (-1 + taxiX) : taxiX) : (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX));
                    taxiY = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 66) && (((0 <= taxiY) && (taxiY < 2)&& (taxiX == 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 65 : ((!((taxiState == 0))) ? 73 : 66));
                    taxiState = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 1 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? ((((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiX == 3)))) || ((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiY == -1)))&& (!((taxiY == 0))))) ? (-1 + taxiX) : taxiX) : (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX));
                    taxiY = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 65) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : ((!((taxiState == 0))) ? 76 : 69));
                    taxiState = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 2 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 69) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : ((!((taxiState == 0))) ? 76 : 69));
                    taxiState = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 2 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                abort();
              }
            continue;
          }
        if (((prog_counter == 76) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))))))
          {
            for(;;)
              {
                if ((prog_counter == 0))
                  break;
                if (((prog_counter == 23) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  break;
                if (((prog_counter == 26) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  break;
                if (((prog_counter == 28) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  break;
                if ((prog_counter == 57))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = 0;
                    taxiState = 2;
                    taxiX = (-1 + taxiX);
                    taxiY = (1 + taxiY);
                    continue;
                  }
                if (((prog_counter == 73) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = 23;
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 75) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = 28;
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 76) && (((taxiY == 0) && (taxiX == 6)&& (taxiState == 1)) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : 26);
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 78) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = 28;
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 80) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = ((!((taxiState == 1))) ? 23 : 73);
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 80) && (0 <= taxiY)&& (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiState == 0)))&& (!((taxiX == 2)))))
                  {
                    {
                      int init_loc = 0;
                      int init_taxiState = 0;
                      int init_taxiX = 0;
                      int init_taxiY = 0;
                      for(;;)
                        {
                          if ((prog_counter == 6))
                            prog_counter = 4;
                          if ((prog_counter == 4))
                            {
                              init_loc = loc;
                              init_taxiState = taxiState;
                              init_taxiX = taxiX;
                              init_taxiY = taxiY;
                            }
                          if (((prog_counter == 4) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                            {
                              prog_counter = 80;
                              break;
                            }
                          if (((prog_counter == 4) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiState == 0)))&& (!((taxiState == 1)))&& (!((taxiState == 2)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiState == 0)))&& (!((taxiState == 1)))&& (!((taxiState == 2)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                            {
                              read_inputs();
                              loc = 0;
                              prog_counter = 6;
                              taxiState = 2;
                              taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                              taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                              continue;
                            }
                          if (((prog_counter == 6) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState < 1)&& (init_taxiState < 3)&& ((init_taxiState + (-1 * taxiState)) < 0)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (3 <= taxiState)&& (1 <= init_taxiState)&& (1 <= (init_taxiState + (-1 * taxiState)))&& (!((taxiState == 0)))&& (!((taxiX == 2)))))))
                            {
                              read_inputs();
                              prog_counter = 6;
                              continue;
                            }
                          abort();
                        }
                    }
                    continue;
                  }
                if (((prog_counter == 66) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = 73;
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 73) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))&& (!((taxiY == 0)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 0)))&& (!((taxiX == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = ((!((taxiState == 1))) ? 23 : 73);
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 65) && (((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : 76);
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 69) && (((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : 76);
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 76) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : ((!((taxiState == 1))) ? 26 : 76));
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 75) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = ((!((taxiState == 1))) ? 28 : 78);
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 78) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = ((!((taxiState == 1))) ? 28 : 78);
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 80) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 65 : ((!((taxiState == 0))) ? ((!((taxiState == 1))) ? ((!((taxiState == 2))) ? 80 : 23) : 73) : 66));
                    taxiState = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 1 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? ((((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiX == 3)))) || ((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiY == -1)))&& (!((taxiY == 0))))) ? (-1 + taxiX) : taxiX) : (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX));
                    taxiY = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 66) && (((0 <= taxiY) && (taxiY < 2)&& (taxiX == 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 65 : ((!((taxiState == 0))) ? 73 : 66));
                    taxiState = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 1 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? ((((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiX == 3)))) || ((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiY == -1)))&& (!((taxiY == 0))))) ? (-1 + taxiX) : taxiX) : (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX));
                    taxiY = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 65) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : ((!((taxiState == 0))) ? 76 : 69));
                    taxiState = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 2 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 69) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : ((!((taxiState == 0))) ? 76 : 69));
                    taxiState = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 2 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                abort();
              }
            continue;
          }
        if (((prog_counter == 78) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
          {
            for(;;)
              {
                if ((prog_counter == 0))
                  break;
                if (((prog_counter == 23) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  break;
                if (((prog_counter == 26) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  break;
                if (((prog_counter == 28) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  break;
                if ((prog_counter == 57))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = 0;
                    taxiState = 2;
                    taxiX = (-1 + taxiX);
                    taxiY = (1 + taxiY);
                    continue;
                  }
                if (((prog_counter == 73) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = 23;
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 75) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = 28;
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 76) && (((taxiY == 0) && (taxiX == 6)&& (taxiState == 1)) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : 26);
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 78) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = 28;
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 80) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = ((!((taxiState == 1))) ? 23 : 73);
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 80) && (0 <= taxiY)&& (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiState == 0)))&& (!((taxiX == 2)))))
                  {
                    {
                      int init_loc = 0;
                      int init_taxiState = 0;
                      int init_taxiX = 0;
                      int init_taxiY = 0;
                      for(;;)
                        {
                          if ((prog_counter == 6))
                            prog_counter = 4;
                          if ((prog_counter == 4))
                            {
                              init_loc = loc;
                              init_taxiState = taxiState;
                              init_taxiX = taxiX;
                              init_taxiY = taxiY;
                            }
                          if (((prog_counter == 4) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                            {
                              prog_counter = 80;
                              break;
                            }
                          if (((prog_counter == 4) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiState == 0)))&& (!((taxiState == 1)))&& (!((taxiState == 2)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiState == 0)))&& (!((taxiState == 1)))&& (!((taxiState == 2)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                            {
                              read_inputs();
                              loc = 0;
                              prog_counter = 6;
                              taxiState = 2;
                              taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                              taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                              continue;
                            }
                          if (((prog_counter == 6) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState < 1)&& (init_taxiState < 3)&& ((init_taxiState + (-1 * taxiState)) < 0)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (3 <= taxiState)&& (1 <= init_taxiState)&& (1 <= (init_taxiState + (-1 * taxiState)))&& (!((taxiState == 0)))&& (!((taxiX == 2)))))))
                            {
                              read_inputs();
                              prog_counter = 6;
                              continue;
                            }
                          abort();
                        }
                    }
                    continue;
                  }
                if (((prog_counter == 66) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = 73;
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 73) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))&& (!((taxiY == 0)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 0)))&& (!((taxiX == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = ((!((taxiState == 1))) ? 23 : 73);
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 65) && (((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : 76);
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 69) && (((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : 76);
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 76) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : ((!((taxiState == 1))) ? 26 : 76));
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 75) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = ((!((taxiState == 1))) ? 28 : 78);
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 78) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = ((!((taxiState == 1))) ? 28 : 78);
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 80) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 65 : ((!((taxiState == 0))) ? ((!((taxiState == 1))) ? ((!((taxiState == 2))) ? 80 : 23) : 73) : 66));
                    taxiState = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 1 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? ((((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiX == 3)))) || ((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiY == -1)))&& (!((taxiY == 0))))) ? (-1 + taxiX) : taxiX) : (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX));
                    taxiY = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 66) && (((0 <= taxiY) && (taxiY < 2)&& (taxiX == 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 65 : ((!((taxiState == 0))) ? 73 : 66));
                    taxiState = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 1 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? ((((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiX == 3)))) || ((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiY == -1)))&& (!((taxiY == 0))))) ? (-1 + taxiX) : taxiX) : (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX));
                    taxiY = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 65) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : ((!((taxiState == 0))) ? 76 : 69));
                    taxiState = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 2 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 69) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : ((!((taxiState == 0))) ? 76 : 69));
                    taxiState = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 2 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                abort();
              }
            continue;
          }
        if (((prog_counter == 80) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiX == 2)))))))
          {
            for(;;)
              {
                if ((prog_counter == 0))
                  break;
                if (((prog_counter == 23) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  break;
                if (((prog_counter == 26) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  break;
                if (((prog_counter == 28) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  break;
                if ((prog_counter == 57))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = 0;
                    taxiState = 2;
                    taxiX = (-1 + taxiX);
                    taxiY = (1 + taxiY);
                    continue;
                  }
                if (((prog_counter == 73) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = 23;
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 75) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = 28;
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 76) && (((taxiY == 0) && (taxiX == 6)&& (taxiState == 1)) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 1)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : 26);
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 78) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = 28;
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 80) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = ((!((taxiState == 1))) ? 23 : 73);
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 80) && (0 <= taxiY)&& (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiState == 0)))&& (!((taxiX == 2)))))
                  {
                    {
                      int init_loc = 0;
                      int init_taxiState = 0;
                      int init_taxiX = 0;
                      int init_taxiY = 0;
                      for(;;)
                        {
                          if ((prog_counter == 6))
                            prog_counter = 4;
                          if ((prog_counter == 4))
                            {
                              init_loc = loc;
                              init_taxiState = taxiState;
                              init_taxiX = taxiX;
                              init_taxiY = taxiY;
                            }
                          if (((prog_counter == 4) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                            {
                              prog_counter = 80;
                              break;
                            }
                          if (((prog_counter == 4) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiState == 0)))&& (!((taxiState == 1)))&& (!((taxiState == 2)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiState == 0)))&& (!((taxiState == 1)))&& (!((taxiState == 2)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                            {
                              read_inputs();
                              loc = 0;
                              prog_counter = 6;
                              taxiState = 2;
                              taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                              taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                              continue;
                            }
                          if (((prog_counter == 6) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (taxiState < 1)&& (init_taxiState < 3)&& ((init_taxiState + (-1 * taxiState)) < 0)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (3 <= taxiState)&& (1 <= init_taxiState)&& (1 <= (init_taxiState + (-1 * taxiState)))&& (!((taxiState == 0)))&& (!((taxiX == 2)))))))
                            {
                              read_inputs();
                              prog_counter = 6;
                              continue;
                            }
                          abort();
                        }
                    }
                    continue;
                  }
                if (((prog_counter == 66) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiX == 2)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = 73;
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 73) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))&& (!((taxiY == 0)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 0)))&& (!((taxiX == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiState == 0)))&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = ((!((taxiState == 1))) ? 23 : 73);
                    taxiState = 2;
                    taxiX = (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX);
                    taxiY = (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY);
                    continue;
                  }
                if (((prog_counter == 65) && (((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : 76);
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 69) && (((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiX == 2))))|| ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiState == 0)))&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : 76);
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 76) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : ((!((taxiState == 1))) ? 26 : 76));
                    taxiState = 2;
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 75) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = ((!((taxiState == 1))) ? 28 : 78);
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 78) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiX == 2)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (1 <= taxiState)&& (taxiState < 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))))))
                  {
                    read_inputs();
                    loc = 2;
                    prog_counter = ((!((taxiState == 1))) ? 28 : 78);
                    taxiState = 2;
                    continue;
                  }
                if (((prog_counter == 80) && (((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 65 : ((!((taxiState == 0))) ? ((!((taxiState == 1))) ? ((!((taxiState == 2))) ? 80 : 23) : 73) : 66));
                    taxiState = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 1 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? ((((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiX == 3)))) || ((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiY == -1)))&& (!((taxiY == 0))))) ? (-1 + taxiX) : taxiX) : (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX));
                    taxiY = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 66) && (((0 <= taxiY) && (taxiY < 2)&& (taxiX == 3)&& (!((taxiY == 0)))&& (!((taxiY == 1)))) || ((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 0;
                    prog_counter = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 65 : ((!((taxiState == 0))) ? 73 : 66));
                    taxiState = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? 1 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? ((((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiX == 3)))) || ((-1 <= taxiY) && (taxiY < 5)&& (1 <= taxiX)&& (taxiX < 8)&& (!((taxiY == -1)))&& (!((taxiY == 0))))) ? (-1 + taxiX) : taxiX) : (((1 <= taxiX) && (0 <= (-1 + taxiX))&& (!((((-1 + taxiX) == 2) && (taxiY == 0))))&& (!((((-1 + taxiX) == 2) && (taxiY == 1))))) ? (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? taxiX : (-1 + taxiX)) : taxiX));
                    taxiY = (((taxiX == 0) && (taxiY == 0)&& (taxiState == 0)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 65) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : ((!((taxiState == 0))) ? 76 : 69));
                    taxiState = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 2 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                if (((prog_counter == 69) && (((1 <= taxiY) && (taxiY < 3)&& (taxiX == 2)&& (!((taxiY == 0)))&& (!((taxiY == 1)))&& (!((taxiY == 2)))) || ((0 <= taxiY) && (taxiY < 2)&& (taxiX == 1)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiY == 0)))&& (!((taxiY == 1))))|| ((0 <= taxiY) && (taxiY < 6)&& (0 <= taxiX)&& (taxiX < 7)&& (0 <= taxiState)&& (taxiState < 2)&& (!((taxiX == 2)))))))
                  {
                    read_inputs();
                    loc = 1;
                    prog_counter = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 75 : ((!((taxiState == 0))) ? 76 : 69));
                    taxiState = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? 2 : ((!((taxiState == 0))) ? 2 : 1));
                    taxiX = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (-1 + taxiX) : taxiX);
                    taxiY = (((taxiX == 6) && (taxiY == 0)&& (taxiState == 1)) ? (1 + taxiY) : (((1 <= taxiY) && (0 <= (-1 + taxiY))&& (!(((taxiX == 2) && ((-1 + taxiY) == 0))))&& (!(((taxiX == 2) && ((-1 + taxiY) == 1))))) ? (-1 + taxiY) : taxiY));
                    continue;
                  }
                abort();
              }
            continue;
          }
        abort();
      }
  }
}
/* ======================================== CONTROLLER END ======================================== */
