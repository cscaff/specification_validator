#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "cJSON.h"


// Configuration Constraints
typedef struct { int x, y; } Cell;

typedef struct {
  int xmin, xmax, ymin, ymax;
  int max_steps;
  Cell *forbidden;
  size_t forbidden_len;
} Constraints;

static Constraints G;

static bool in_bounds_cfg(int x, int y) {
  return (x >= G.xmin && x <= G.xmax && y >= G.ymin && y <= G.ymax);
}

static bool is_forbidden_cfg(int x, int y) {
  for (size_t i = 0; i < G.forbidden_len; i++) {
    if (G.forbidden[i].x == x && G.forbidden[i].y == y) return true;
  }
  return false;
}

static void validate_or_abort(int x, int y, int t) {
  if (t > G.max_steps) {
    printf("ABORT: timeout (t=%d)\n", t);
    fflush(stdout);
    exit(255);
  }
  if (!in_bounds_cfg(x, y)) {
    printf("ABORT: out of bounds (x=%d,y=%d) t=%d\n", x, y, t);
    fflush(stdout);
    exit(255);
  }
  if (is_forbidden_cfg(x, y)) {
    printf("ABORT: forbidden cell (x=%d,y=%d) t=%d\n", x, y, t);
    fflush(stdout);
    exit(255);
  }
}

// Json Configuration Parser
static char *read_entire_file(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) return NULL;
  fseek(f, 0, SEEK_END);
  long n = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *buf = (char*)malloc((size_t)n + 1);
  if (!buf) { fclose(f); return NULL; }
  fread(buf, 1, (size_t)n, f);
  buf[n] = '\0';
  fclose(f);
  return buf;
}

static void load_constraints(const char *path) {
  char *text = read_entire_file(path);
  if (!text) { fprintf(stderr, "Failed to read %s\n", path); exit(2); }

  cJSON *root = cJSON_Parse(text);
  free(text);
  if (!root) { fprintf(stderr, "Invalid JSON\n"); exit(2); }

  cJSON *b = cJSON_GetObjectItem(root, "bounds");
  G.xmin = cJSON_GetObjectItem(b, "xmin")->valueint;
  G.xmax = cJSON_GetObjectItem(b, "xmax")->valueint;
  G.ymin = cJSON_GetObjectItem(b, "ymin")->valueint;
  G.ymax = cJSON_GetObjectItem(b, "ymax")->valueint;

  cJSON *ms = cJSON_GetObjectItem(root, "max_steps");
  G.max_steps = ms ? ms->valueint : 1000;

  cJSON *arr = cJSON_GetObjectItem(root, "forbidden");
  G.forbidden_len = (size_t)cJSON_GetArraySize(arr);
  G.forbidden = (Cell*)calloc(G.forbidden_len, sizeof(Cell));

  for (size_t i = 0; i < G.forbidden_len; i++) {
    cJSON *it = cJSON_GetArrayItem(arr, (int)i);
    G.forbidden[i].x = cJSON_GetObjectItem(it, "x")->valueint;
    G.forbidden[i].y = cJSON_GetObjectItem(it, "y")->valueint;
  }

  cJSON_Delete(root);
}


// Prototypes
void read_inputs(void);
void step_controller(void);


// Player Global Position
int playerX;
int playerY;

// Goal
int goalX = 3;
int goalY = 3;

// Global Time
int t = 0;


int main(int argc, char **argv) {
  const char *cfg = (argc >= 2) ? argv[1] : "constraints.json";
  load_constraints(cfg);

  // Initial state
  playerX = 0;
  playerY = 0;

  step_controller();

  return 0;
}


void read_inputs() {
  // Log Moves
  printf("t=%d  (x=%d, y=%d)\n", t, playerX, playerY);
  fflush(stdout);

  // Abort conditions
  validate_or_abort(playerX, playerY, t);

  // Goal Condition
  if (playerX == goalX && playerY == goalY) {
    printf("GOAL REACHED in %d steps!\n", t);
    fflush(stdout);
    exit(0);
  }
  
  // Update Time 
  t++;
}


/* ======================================== CONTROLLER ======================================== */
void step_controller() {
  {
    int prog_counter = 0;
    prog_counter = 26;
    for(;;)
      {
        if ((prog_counter == 1))
          {
            read_inputs();
            prog_counter = 1;
            continue;
          }
        if (((prog_counter == 2) && (playerY == 3)&& (playerX == 3)))
          {
            read_inputs();
            playerX = (playerX - 1);
            prog_counter = 3;
            continue;
          }
        if (((prog_counter == 3) && (playerX == 2)&& (playerY == 3)))
          {
            read_inputs();
            playerX = (playerX - 1);
            prog_counter = 5;
            continue;
          }
        if (((prog_counter == 4) && (playerX == 2)&& (playerY == 2)))
          {
            read_inputs();
            playerX = (playerX - 1);
            prog_counter = 7;
            continue;
          }
        if (((prog_counter == 5) && (playerY == 3)&& (playerX == 1)))
          {
            read_inputs();
            playerX = (playerX + 1);
            prog_counter = 3;
            continue;
          }
        if (((prog_counter == 6) && (playerX == 2)&& (playerY == 1)))
          {
            read_inputs();
            playerY = (playerY - 1);
            prog_counter = 8;
            continue;
          }
        if (((prog_counter == 7) && (playerY == 2)&& (playerX == 1)))
          {
            read_inputs();
            playerX = (playerX - 1);
            prog_counter = 9;
            continue;
          }
        if (((prog_counter == 8) && (playerY == 0)&& (playerX == 2)))
          {
            read_inputs();
            playerX = (playerX - 1);
            prog_counter = 11;
            continue;
          }
        if (((prog_counter == 9) && (playerY == 2)&& (playerX == 0)))
          {
            read_inputs();
            playerX = (playerX + 1);
            prog_counter = 7;
            continue;
          }
        if (((prog_counter == 10) && (playerY == 0)&& (playerX == 3)))
          {
            read_inputs();
            playerX = (playerX - 1);
            prog_counter = 8;
            continue;
          }
        if (((prog_counter == 11) && (playerY == 0)&& (playerX == 1)))
          {
            read_inputs();
            playerX = (playerX - 1);
            prog_counter = 13;
            continue;
          }
        if (((prog_counter == 12) && (playerY == 1)&& (playerX == 0)))
          {
            read_inputs();
            playerY = (playerY - 1);
            prog_counter = 13;
            continue;
          }
        if (((prog_counter == 13) && (playerY == 0)&& (playerX == 0)))
          {
            read_inputs();
            playerX = (playerX + 1);
            prog_counter = 11;
            continue;
          }
        if ((prog_counter == 1))
          {
            for(;;)
              {
                if ((prog_counter == 1))
                  break;
                if (((prog_counter == 2) && (playerY == 3)&& (playerX == 3)))
                  break;
                if (((prog_counter == 3) && (playerX == 2)&& (playerY == 3)))
                  break;
                if (((prog_counter == 4) && (playerX == 2)&& (playerY == 2)))
                  break;
                if (((prog_counter == 5) && (playerY == 3)&& (playerX == 1)))
                  break;
                if (((prog_counter == 6) && (playerX == 2)&& (playerY == 1)))
                  break;
                if (((prog_counter == 7) && (playerY == 2)&& (playerX == 1)))
                  break;
                if (((prog_counter == 8) && (playerY == 0)&& (playerX == 2)))
                  break;
                if (((prog_counter == 9) && (playerY == 2)&& (playerX == 0)))
                  break;
                if (((prog_counter == 10) && (playerY == 0)&& (playerX == 3)))
                  break;
                if (((prog_counter == 11) && (playerY == 0)&& (playerX == 1)))
                  break;
                if (((prog_counter == 12) && (playerY == 1)&& (playerX == 0)))
                  break;
                if (((prog_counter == 13) && (playerY == 0)&& (playerX == 0)))
                  break;
                if (((prog_counter == 25) && (playerX == 3)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 3;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))&& ((!((playerY == 2))) || (3 <= playerX)|| (playerX <= 0))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (3 <= playerX)|| (playerX <= 0))&& ((!((playerY == 1))) || (3 <= playerX)|| (playerX <= 0)|| (playerX == 1))&& ((!((playerX == 1))) || (!((playerY == 3))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? (playerX - 1) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? (playerX + 1) : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? (playerY - 1) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? 1 : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 25 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 21) && (playerY == 3)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 25;
                    continue;
                  }
                if (((prog_counter == 19) && (playerX == 1)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 20) && (playerY == 2)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (playerX <= 0)|| (3 <= playerX))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 20 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 21 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 17) && (playerX == 1)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 24) && (playerY == 1)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 15) && (playerX == 0)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 17;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : (playerX - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX - 1) : ((playerX > 0) ? ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1))) : (playerX - 1))))))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : (playerY - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : 1)) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 24 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 22) && (playerX == 2)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 24;
                    continue;
                  }
                if (((prog_counter == 14) && (playerX == 0)&& (playerY == 1)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 15;
                    continue;
                  }
                if (((prog_counter == 18) && (playerX == 1)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 22;
                    continue;
                  }
                if (((prog_counter == 23) && (playerX == 3)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 22;
                    continue;
                  }
                if ((prog_counter == 26))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX - 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? (playerY - 1) : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : ((playerY == 3) ? 1 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 18 : 1))))) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 22 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 18 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 16) && (playerX == 0)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 18;
                    continue;
                  }
                {
                  prog_counter = 26;
                  continue;
                }
              }
            continue;
          }
        if (((prog_counter == 2) && (playerY == 3)&& (playerX == 3)))
          {
            for(;;)
              {
                if ((prog_counter == 1))
                  break;
                if (((prog_counter == 2) && (playerY == 3)&& (playerX == 3)))
                  break;
                if (((prog_counter == 3) && (playerX == 2)&& (playerY == 3)))
                  break;
                if (((prog_counter == 4) && (playerX == 2)&& (playerY == 2)))
                  break;
                if (((prog_counter == 5) && (playerY == 3)&& (playerX == 1)))
                  break;
                if (((prog_counter == 6) && (playerX == 2)&& (playerY == 1)))
                  break;
                if (((prog_counter == 7) && (playerY == 2)&& (playerX == 1)))
                  break;
                if (((prog_counter == 8) && (playerY == 0)&& (playerX == 2)))
                  break;
                if (((prog_counter == 9) && (playerY == 2)&& (playerX == 0)))
                  break;
                if (((prog_counter == 10) && (playerY == 0)&& (playerX == 3)))
                  break;
                if (((prog_counter == 11) && (playerY == 0)&& (playerX == 1)))
                  break;
                if (((prog_counter == 12) && (playerY == 1)&& (playerX == 0)))
                  break;
                if (((prog_counter == 13) && (playerY == 0)&& (playerX == 0)))
                  break;
                if (((prog_counter == 25) && (playerX == 3)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 3;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))&& ((!((playerY == 2))) || (3 <= playerX)|| (playerX <= 0))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (3 <= playerX)|| (playerX <= 0))&& ((!((playerY == 1))) || (3 <= playerX)|| (playerX <= 0)|| (playerX == 1))&& ((!((playerX == 1))) || (!((playerY == 3))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? (playerX - 1) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? (playerX + 1) : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? (playerY - 1) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? 1 : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 25 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 21) && (playerY == 3)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 25;
                    continue;
                  }
                if (((prog_counter == 19) && (playerX == 1)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 20) && (playerY == 2)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (playerX <= 0)|| (3 <= playerX))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 20 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 21 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 17) && (playerX == 1)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 24) && (playerY == 1)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 15) && (playerX == 0)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 17;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : (playerX - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX - 1) : ((playerX > 0) ? ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1))) : (playerX - 1))))))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : (playerY - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : 1)) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 24 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 22) && (playerX == 2)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 24;
                    continue;
                  }
                if (((prog_counter == 14) && (playerX == 0)&& (playerY == 1)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 15;
                    continue;
                  }
                if (((prog_counter == 18) && (playerX == 1)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 22;
                    continue;
                  }
                if (((prog_counter == 23) && (playerX == 3)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 22;
                    continue;
                  }
                if ((prog_counter == 26))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX - 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? (playerY - 1) : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : ((playerY == 3) ? 1 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 18 : 1))))) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 22 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 18 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 16) && (playerX == 0)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 18;
                    continue;
                  }
                {
                  prog_counter = 26;
                  continue;
                }
              }
            continue;
          }
        if (((prog_counter == 3) && (playerX == 2)&& (playerY == 3)))
          {
            for(;;)
              {
                if ((prog_counter == 1))
                  break;
                if (((prog_counter == 2) && (playerY == 3)&& (playerX == 3)))
                  break;
                if (((prog_counter == 3) && (playerX == 2)&& (playerY == 3)))
                  break;
                if (((prog_counter == 4) && (playerX == 2)&& (playerY == 2)))
                  break;
                if (((prog_counter == 5) && (playerY == 3)&& (playerX == 1)))
                  break;
                if (((prog_counter == 6) && (playerX == 2)&& (playerY == 1)))
                  break;
                if (((prog_counter == 7) && (playerY == 2)&& (playerX == 1)))
                  break;
                if (((prog_counter == 8) && (playerY == 0)&& (playerX == 2)))
                  break;
                if (((prog_counter == 9) && (playerY == 2)&& (playerX == 0)))
                  break;
                if (((prog_counter == 10) && (playerY == 0)&& (playerX == 3)))
                  break;
                if (((prog_counter == 11) && (playerY == 0)&& (playerX == 1)))
                  break;
                if (((prog_counter == 12) && (playerY == 1)&& (playerX == 0)))
                  break;
                if (((prog_counter == 13) && (playerY == 0)&& (playerX == 0)))
                  break;
                if (((prog_counter == 25) && (playerX == 3)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 3;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))&& ((!((playerY == 2))) || (3 <= playerX)|| (playerX <= 0))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (3 <= playerX)|| (playerX <= 0))&& ((!((playerY == 1))) || (3 <= playerX)|| (playerX <= 0)|| (playerX == 1))&& ((!((playerX == 1))) || (!((playerY == 3))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? (playerX - 1) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? (playerX + 1) : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? (playerY - 1) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? 1 : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 25 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 21) && (playerY == 3)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 25;
                    continue;
                  }
                if (((prog_counter == 19) && (playerX == 1)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 20) && (playerY == 2)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (playerX <= 0)|| (3 <= playerX))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 20 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 21 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 17) && (playerX == 1)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 24) && (playerY == 1)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 15) && (playerX == 0)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 17;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : (playerX - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX - 1) : ((playerX > 0) ? ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1))) : (playerX - 1))))))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : (playerY - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : 1)) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 24 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 22) && (playerX == 2)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 24;
                    continue;
                  }
                if (((prog_counter == 14) && (playerX == 0)&& (playerY == 1)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 15;
                    continue;
                  }
                if (((prog_counter == 18) && (playerX == 1)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 22;
                    continue;
                  }
                if (((prog_counter == 23) && (playerX == 3)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 22;
                    continue;
                  }
                if ((prog_counter == 26))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX - 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? (playerY - 1) : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : ((playerY == 3) ? 1 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 18 : 1))))) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 22 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 18 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 16) && (playerX == 0)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 18;
                    continue;
                  }
                {
                  prog_counter = 26;
                  continue;
                }
              }
            continue;
          }
        if (((prog_counter == 4) && (playerX == 2)&& (playerY == 2)))
          {
            for(;;)
              {
                if ((prog_counter == 1))
                  break;
                if (((prog_counter == 2) && (playerY == 3)&& (playerX == 3)))
                  break;
                if (((prog_counter == 3) && (playerX == 2)&& (playerY == 3)))
                  break;
                if (((prog_counter == 4) && (playerX == 2)&& (playerY == 2)))
                  break;
                if (((prog_counter == 5) && (playerY == 3)&& (playerX == 1)))
                  break;
                if (((prog_counter == 6) && (playerX == 2)&& (playerY == 1)))
                  break;
                if (((prog_counter == 7) && (playerY == 2)&& (playerX == 1)))
                  break;
                if (((prog_counter == 8) && (playerY == 0)&& (playerX == 2)))
                  break;
                if (((prog_counter == 9) && (playerY == 2)&& (playerX == 0)))
                  break;
                if (((prog_counter == 10) && (playerY == 0)&& (playerX == 3)))
                  break;
                if (((prog_counter == 11) && (playerY == 0)&& (playerX == 1)))
                  break;
                if (((prog_counter == 12) && (playerY == 1)&& (playerX == 0)))
                  break;
                if (((prog_counter == 13) && (playerY == 0)&& (playerX == 0)))
                  break;
                if (((prog_counter == 25) && (playerX == 3)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 3;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))&& ((!((playerY == 2))) || (3 <= playerX)|| (playerX <= 0))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (3 <= playerX)|| (playerX <= 0))&& ((!((playerY == 1))) || (3 <= playerX)|| (playerX <= 0)|| (playerX == 1))&& ((!((playerX == 1))) || (!((playerY == 3))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? (playerX - 1) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? (playerX + 1) : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? (playerY - 1) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? 1 : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 25 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 21) && (playerY == 3)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 25;
                    continue;
                  }
                if (((prog_counter == 19) && (playerX == 1)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 20) && (playerY == 2)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (playerX <= 0)|| (3 <= playerX))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 20 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 21 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 17) && (playerX == 1)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 24) && (playerY == 1)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 15) && (playerX == 0)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 17;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : (playerX - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX - 1) : ((playerX > 0) ? ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1))) : (playerX - 1))))))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : (playerY - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : 1)) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 24 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 22) && (playerX == 2)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 24;
                    continue;
                  }
                if (((prog_counter == 14) && (playerX == 0)&& (playerY == 1)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 15;
                    continue;
                  }
                if (((prog_counter == 18) && (playerX == 1)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 22;
                    continue;
                  }
                if (((prog_counter == 23) && (playerX == 3)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 22;
                    continue;
                  }
                if ((prog_counter == 26))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX - 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? (playerY - 1) : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : ((playerY == 3) ? 1 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 18 : 1))))) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 22 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 18 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 16) && (playerX == 0)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 18;
                    continue;
                  }
                {
                  prog_counter = 26;
                  continue;
                }
              }
            continue;
          }
        if (((prog_counter == 5) && (playerY == 3)&& (playerX == 1)))
          {
            for(;;)
              {
                if ((prog_counter == 1))
                  break;
                if (((prog_counter == 2) && (playerY == 3)&& (playerX == 3)))
                  break;
                if (((prog_counter == 3) && (playerX == 2)&& (playerY == 3)))
                  break;
                if (((prog_counter == 4) && (playerX == 2)&& (playerY == 2)))
                  break;
                if (((prog_counter == 5) && (playerY == 3)&& (playerX == 1)))
                  break;
                if (((prog_counter == 6) && (playerX == 2)&& (playerY == 1)))
                  break;
                if (((prog_counter == 7) && (playerY == 2)&& (playerX == 1)))
                  break;
                if (((prog_counter == 8) && (playerY == 0)&& (playerX == 2)))
                  break;
                if (((prog_counter == 9) && (playerY == 2)&& (playerX == 0)))
                  break;
                if (((prog_counter == 10) && (playerY == 0)&& (playerX == 3)))
                  break;
                if (((prog_counter == 11) && (playerY == 0)&& (playerX == 1)))
                  break;
                if (((prog_counter == 12) && (playerY == 1)&& (playerX == 0)))
                  break;
                if (((prog_counter == 13) && (playerY == 0)&& (playerX == 0)))
                  break;
                if (((prog_counter == 25) && (playerX == 3)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 3;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))&& ((!((playerY == 2))) || (3 <= playerX)|| (playerX <= 0))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (3 <= playerX)|| (playerX <= 0))&& ((!((playerY == 1))) || (3 <= playerX)|| (playerX <= 0)|| (playerX == 1))&& ((!((playerX == 1))) || (!((playerY == 3))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? (playerX - 1) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? (playerX + 1) : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? (playerY - 1) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? 1 : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 25 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 21) && (playerY == 3)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 25;
                    continue;
                  }
                if (((prog_counter == 19) && (playerX == 1)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 20) && (playerY == 2)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (playerX <= 0)|| (3 <= playerX))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 20 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 21 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 17) && (playerX == 1)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 24) && (playerY == 1)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 15) && (playerX == 0)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 17;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : (playerX - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX - 1) : ((playerX > 0) ? ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1))) : (playerX - 1))))))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : (playerY - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : 1)) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 24 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 22) && (playerX == 2)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 24;
                    continue;
                  }
                if (((prog_counter == 14) && (playerX == 0)&& (playerY == 1)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 15;
                    continue;
                  }
                if (((prog_counter == 18) && (playerX == 1)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 22;
                    continue;
                  }
                if (((prog_counter == 23) && (playerX == 3)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 22;
                    continue;
                  }
                if ((prog_counter == 26))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX - 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? (playerY - 1) : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : ((playerY == 3) ? 1 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 18 : 1))))) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 22 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 18 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 16) && (playerX == 0)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 18;
                    continue;
                  }
                {
                  prog_counter = 26;
                  continue;
                }
              }
            continue;
          }
        if (((prog_counter == 6) && (playerX == 2)&& (playerY == 1)))
          {
            for(;;)
              {
                if ((prog_counter == 1))
                  break;
                if (((prog_counter == 2) && (playerY == 3)&& (playerX == 3)))
                  break;
                if (((prog_counter == 3) && (playerX == 2)&& (playerY == 3)))
                  break;
                if (((prog_counter == 4) && (playerX == 2)&& (playerY == 2)))
                  break;
                if (((prog_counter == 5) && (playerY == 3)&& (playerX == 1)))
                  break;
                if (((prog_counter == 6) && (playerX == 2)&& (playerY == 1)))
                  break;
                if (((prog_counter == 7) && (playerY == 2)&& (playerX == 1)))
                  break;
                if (((prog_counter == 8) && (playerY == 0)&& (playerX == 2)))
                  break;
                if (((prog_counter == 9) && (playerY == 2)&& (playerX == 0)))
                  break;
                if (((prog_counter == 10) && (playerY == 0)&& (playerX == 3)))
                  break;
                if (((prog_counter == 11) && (playerY == 0)&& (playerX == 1)))
                  break;
                if (((prog_counter == 12) && (playerY == 1)&& (playerX == 0)))
                  break;
                if (((prog_counter == 13) && (playerY == 0)&& (playerX == 0)))
                  break;
                if (((prog_counter == 25) && (playerX == 3)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 3;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))&& ((!((playerY == 2))) || (3 <= playerX)|| (playerX <= 0))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (3 <= playerX)|| (playerX <= 0))&& ((!((playerY == 1))) || (3 <= playerX)|| (playerX <= 0)|| (playerX == 1))&& ((!((playerX == 1))) || (!((playerY == 3))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? (playerX - 1) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? (playerX + 1) : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? (playerY - 1) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? 1 : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 25 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 21) && (playerY == 3)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 25;
                    continue;
                  }
                if (((prog_counter == 19) && (playerX == 1)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 20) && (playerY == 2)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (playerX <= 0)|| (3 <= playerX))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 20 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 21 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 17) && (playerX == 1)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 24) && (playerY == 1)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 15) && (playerX == 0)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 17;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : (playerX - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX - 1) : ((playerX > 0) ? ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1))) : (playerX - 1))))))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : (playerY - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : 1)) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 24 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 22) && (playerX == 2)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 24;
                    continue;
                  }
                if (((prog_counter == 14) && (playerX == 0)&& (playerY == 1)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 15;
                    continue;
                  }
                if (((prog_counter == 18) && (playerX == 1)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 22;
                    continue;
                  }
                if (((prog_counter == 23) && (playerX == 3)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 22;
                    continue;
                  }
                if ((prog_counter == 26))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX - 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? (playerY - 1) : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : ((playerY == 3) ? 1 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 18 : 1))))) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 22 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 18 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 16) && (playerX == 0)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 18;
                    continue;
                  }
                {
                  prog_counter = 26;
                  continue;
                }
              }
            continue;
          }
        if (((prog_counter == 7) && (playerY == 2)&& (playerX == 1)))
          {
            for(;;)
              {
                if ((prog_counter == 1))
                  break;
                if (((prog_counter == 2) && (playerY == 3)&& (playerX == 3)))
                  break;
                if (((prog_counter == 3) && (playerX == 2)&& (playerY == 3)))
                  break;
                if (((prog_counter == 4) && (playerX == 2)&& (playerY == 2)))
                  break;
                if (((prog_counter == 5) && (playerY == 3)&& (playerX == 1)))
                  break;
                if (((prog_counter == 6) && (playerX == 2)&& (playerY == 1)))
                  break;
                if (((prog_counter == 7) && (playerY == 2)&& (playerX == 1)))
                  break;
                if (((prog_counter == 8) && (playerY == 0)&& (playerX == 2)))
                  break;
                if (((prog_counter == 9) && (playerY == 2)&& (playerX == 0)))
                  break;
                if (((prog_counter == 10) && (playerY == 0)&& (playerX == 3)))
                  break;
                if (((prog_counter == 11) && (playerY == 0)&& (playerX == 1)))
                  break;
                if (((prog_counter == 12) && (playerY == 1)&& (playerX == 0)))
                  break;
                if (((prog_counter == 13) && (playerY == 0)&& (playerX == 0)))
                  break;
                if (((prog_counter == 25) && (playerX == 3)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 3;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))&& ((!((playerY == 2))) || (3 <= playerX)|| (playerX <= 0))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (3 <= playerX)|| (playerX <= 0))&& ((!((playerY == 1))) || (3 <= playerX)|| (playerX <= 0)|| (playerX == 1))&& ((!((playerX == 1))) || (!((playerY == 3))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? (playerX - 1) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? (playerX + 1) : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? (playerY - 1) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? 1 : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 25 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 21) && (playerY == 3)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 25;
                    continue;
                  }
                if (((prog_counter == 19) && (playerX == 1)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 20) && (playerY == 2)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (playerX <= 0)|| (3 <= playerX))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 20 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 21 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 17) && (playerX == 1)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 24) && (playerY == 1)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 15) && (playerX == 0)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 17;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : (playerX - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX - 1) : ((playerX > 0) ? ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1))) : (playerX - 1))))))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : (playerY - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : 1)) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 24 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 22) && (playerX == 2)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 24;
                    continue;
                  }
                if (((prog_counter == 14) && (playerX == 0)&& (playerY == 1)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 15;
                    continue;
                  }
                if (((prog_counter == 18) && (playerX == 1)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 22;
                    continue;
                  }
                if (((prog_counter == 23) && (playerX == 3)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 22;
                    continue;
                  }
                if ((prog_counter == 26))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX - 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? (playerY - 1) : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : ((playerY == 3) ? 1 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 18 : 1))))) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 22 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 18 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 16) && (playerX == 0)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 18;
                    continue;
                  }
                {
                  prog_counter = 26;
                  continue;
                }
              }
            continue;
          }
        if (((prog_counter == 8) && (playerY == 0)&& (playerX == 2)))
          {
            for(;;)
              {
                if ((prog_counter == 1))
                  break;
                if (((prog_counter == 2) && (playerY == 3)&& (playerX == 3)))
                  break;
                if (((prog_counter == 3) && (playerX == 2)&& (playerY == 3)))
                  break;
                if (((prog_counter == 4) && (playerX == 2)&& (playerY == 2)))
                  break;
                if (((prog_counter == 5) && (playerY == 3)&& (playerX == 1)))
                  break;
                if (((prog_counter == 6) && (playerX == 2)&& (playerY == 1)))
                  break;
                if (((prog_counter == 7) && (playerY == 2)&& (playerX == 1)))
                  break;
                if (((prog_counter == 8) && (playerY == 0)&& (playerX == 2)))
                  break;
                if (((prog_counter == 9) && (playerY == 2)&& (playerX == 0)))
                  break;
                if (((prog_counter == 10) && (playerY == 0)&& (playerX == 3)))
                  break;
                if (((prog_counter == 11) && (playerY == 0)&& (playerX == 1)))
                  break;
                if (((prog_counter == 12) && (playerY == 1)&& (playerX == 0)))
                  break;
                if (((prog_counter == 13) && (playerY == 0)&& (playerX == 0)))
                  break;
                if (((prog_counter == 25) && (playerX == 3)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 3;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))&& ((!((playerY == 2))) || (3 <= playerX)|| (playerX <= 0))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (3 <= playerX)|| (playerX <= 0))&& ((!((playerY == 1))) || (3 <= playerX)|| (playerX <= 0)|| (playerX == 1))&& ((!((playerX == 1))) || (!((playerY == 3))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? (playerX - 1) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? (playerX + 1) : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? (playerY - 1) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? 1 : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 25 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 21) && (playerY == 3)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 25;
                    continue;
                  }
                if (((prog_counter == 19) && (playerX == 1)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 20) && (playerY == 2)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (playerX <= 0)|| (3 <= playerX))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 20 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 21 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 17) && (playerX == 1)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 24) && (playerY == 1)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 15) && (playerX == 0)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 17;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : (playerX - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX - 1) : ((playerX > 0) ? ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1))) : (playerX - 1))))))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : (playerY - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : 1)) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 24 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 22) && (playerX == 2)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 24;
                    continue;
                  }
                if (((prog_counter == 14) && (playerX == 0)&& (playerY == 1)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 15;
                    continue;
                  }
                if (((prog_counter == 18) && (playerX == 1)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 22;
                    continue;
                  }
                if (((prog_counter == 23) && (playerX == 3)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 22;
                    continue;
                  }
                if ((prog_counter == 26))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX - 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? (playerY - 1) : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : ((playerY == 3) ? 1 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 18 : 1))))) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 22 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 18 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 16) && (playerX == 0)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 18;
                    continue;
                  }
                {
                  prog_counter = 26;
                  continue;
                }
              }
            continue;
          }
        if (((prog_counter == 9) && (playerY == 2)&& (playerX == 0)))
          {
            for(;;)
              {
                if ((prog_counter == 1))
                  break;
                if (((prog_counter == 2) && (playerY == 3)&& (playerX == 3)))
                  break;
                if (((prog_counter == 3) && (playerX == 2)&& (playerY == 3)))
                  break;
                if (((prog_counter == 4) && (playerX == 2)&& (playerY == 2)))
                  break;
                if (((prog_counter == 5) && (playerY == 3)&& (playerX == 1)))
                  break;
                if (((prog_counter == 6) && (playerX == 2)&& (playerY == 1)))
                  break;
                if (((prog_counter == 7) && (playerY == 2)&& (playerX == 1)))
                  break;
                if (((prog_counter == 8) && (playerY == 0)&& (playerX == 2)))
                  break;
                if (((prog_counter == 9) && (playerY == 2)&& (playerX == 0)))
                  break;
                if (((prog_counter == 10) && (playerY == 0)&& (playerX == 3)))
                  break;
                if (((prog_counter == 11) && (playerY == 0)&& (playerX == 1)))
                  break;
                if (((prog_counter == 12) && (playerY == 1)&& (playerX == 0)))
                  break;
                if (((prog_counter == 13) && (playerY == 0)&& (playerX == 0)))
                  break;
                if (((prog_counter == 25) && (playerX == 3)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 3;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))&& ((!((playerY == 2))) || (3 <= playerX)|| (playerX <= 0))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (3 <= playerX)|| (playerX <= 0))&& ((!((playerY == 1))) || (3 <= playerX)|| (playerX <= 0)|| (playerX == 1))&& ((!((playerX == 1))) || (!((playerY == 3))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? (playerX - 1) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? (playerX + 1) : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? (playerY - 1) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? 1 : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 25 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 21) && (playerY == 3)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 25;
                    continue;
                  }
                if (((prog_counter == 19) && (playerX == 1)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 20) && (playerY == 2)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (playerX <= 0)|| (3 <= playerX))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 20 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 21 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 17) && (playerX == 1)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 24) && (playerY == 1)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 15) && (playerX == 0)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 17;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : (playerX - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX - 1) : ((playerX > 0) ? ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1))) : (playerX - 1))))))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : (playerY - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : 1)) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 24 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 22) && (playerX == 2)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 24;
                    continue;
                  }
                if (((prog_counter == 14) && (playerX == 0)&& (playerY == 1)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 15;
                    continue;
                  }
                if (((prog_counter == 18) && (playerX == 1)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 22;
                    continue;
                  }
                if (((prog_counter == 23) && (playerX == 3)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 22;
                    continue;
                  }
                if ((prog_counter == 26))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX - 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? (playerY - 1) : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : ((playerY == 3) ? 1 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 18 : 1))))) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 22 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 18 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 16) && (playerX == 0)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 18;
                    continue;
                  }
                {
                  prog_counter = 26;
                  continue;
                }
              }
            continue;
          }
        if (((prog_counter == 10) && (playerY == 0)&& (playerX == 3)))
          {
            for(;;)
              {
                if ((prog_counter == 1))
                  break;
                if (((prog_counter == 2) && (playerY == 3)&& (playerX == 3)))
                  break;
                if (((prog_counter == 3) && (playerX == 2)&& (playerY == 3)))
                  break;
                if (((prog_counter == 4) && (playerX == 2)&& (playerY == 2)))
                  break;
                if (((prog_counter == 5) && (playerY == 3)&& (playerX == 1)))
                  break;
                if (((prog_counter == 6) && (playerX == 2)&& (playerY == 1)))
                  break;
                if (((prog_counter == 7) && (playerY == 2)&& (playerX == 1)))
                  break;
                if (((prog_counter == 8) && (playerY == 0)&& (playerX == 2)))
                  break;
                if (((prog_counter == 9) && (playerY == 2)&& (playerX == 0)))
                  break;
                if (((prog_counter == 10) && (playerY == 0)&& (playerX == 3)))
                  break;
                if (((prog_counter == 11) && (playerY == 0)&& (playerX == 1)))
                  break;
                if (((prog_counter == 12) && (playerY == 1)&& (playerX == 0)))
                  break;
                if (((prog_counter == 13) && (playerY == 0)&& (playerX == 0)))
                  break;
                if (((prog_counter == 25) && (playerX == 3)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 3;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))&& ((!((playerY == 2))) || (3 <= playerX)|| (playerX <= 0))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (3 <= playerX)|| (playerX <= 0))&& ((!((playerY == 1))) || (3 <= playerX)|| (playerX <= 0)|| (playerX == 1))&& ((!((playerX == 1))) || (!((playerY == 3))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? (playerX - 1) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? (playerX + 1) : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? (playerY - 1) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? 1 : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 25 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 21) && (playerY == 3)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 25;
                    continue;
                  }
                if (((prog_counter == 19) && (playerX == 1)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 20) && (playerY == 2)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (playerX <= 0)|| (3 <= playerX))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 20 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 21 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 17) && (playerX == 1)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 24) && (playerY == 1)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 15) && (playerX == 0)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 17;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : (playerX - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX - 1) : ((playerX > 0) ? ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1))) : (playerX - 1))))))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : (playerY - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : 1)) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 24 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 22) && (playerX == 2)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 24;
                    continue;
                  }
                if (((prog_counter == 14) && (playerX == 0)&& (playerY == 1)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 15;
                    continue;
                  }
                if (((prog_counter == 18) && (playerX == 1)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 22;
                    continue;
                  }
                if (((prog_counter == 23) && (playerX == 3)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 22;
                    continue;
                  }
                if ((prog_counter == 26))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX - 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? (playerY - 1) : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : ((playerY == 3) ? 1 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 18 : 1))))) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 22 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 18 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 16) && (playerX == 0)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 18;
                    continue;
                  }
                {
                  prog_counter = 26;
                  continue;
                }
              }
            continue;
          }
        if (((prog_counter == 11) && (playerY == 0)&& (playerX == 1)))
          {
            for(;;)
              {
                if ((prog_counter == 1))
                  break;
                if (((prog_counter == 2) && (playerY == 3)&& (playerX == 3)))
                  break;
                if (((prog_counter == 3) && (playerX == 2)&& (playerY == 3)))
                  break;
                if (((prog_counter == 4) && (playerX == 2)&& (playerY == 2)))
                  break;
                if (((prog_counter == 5) && (playerY == 3)&& (playerX == 1)))
                  break;
                if (((prog_counter == 6) && (playerX == 2)&& (playerY == 1)))
                  break;
                if (((prog_counter == 7) && (playerY == 2)&& (playerX == 1)))
                  break;
                if (((prog_counter == 8) && (playerY == 0)&& (playerX == 2)))
                  break;
                if (((prog_counter == 9) && (playerY == 2)&& (playerX == 0)))
                  break;
                if (((prog_counter == 10) && (playerY == 0)&& (playerX == 3)))
                  break;
                if (((prog_counter == 11) && (playerY == 0)&& (playerX == 1)))
                  break;
                if (((prog_counter == 12) && (playerY == 1)&& (playerX == 0)))
                  break;
                if (((prog_counter == 13) && (playerY == 0)&& (playerX == 0)))
                  break;
                if (((prog_counter == 25) && (playerX == 3)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 3;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))&& ((!((playerY == 2))) || (3 <= playerX)|| (playerX <= 0))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (3 <= playerX)|| (playerX <= 0))&& ((!((playerY == 1))) || (3 <= playerX)|| (playerX <= 0)|| (playerX == 1))&& ((!((playerX == 1))) || (!((playerY == 3))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? (playerX - 1) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? (playerX + 1) : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? (playerY - 1) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? 1 : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 25 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 21) && (playerY == 3)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 25;
                    continue;
                  }
                if (((prog_counter == 19) && (playerX == 1)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 20) && (playerY == 2)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (playerX <= 0)|| (3 <= playerX))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 20 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 21 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 17) && (playerX == 1)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 24) && (playerY == 1)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 15) && (playerX == 0)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 17;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : (playerX - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX - 1) : ((playerX > 0) ? ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1))) : (playerX - 1))))))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : (playerY - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : 1)) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 24 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 22) && (playerX == 2)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 24;
                    continue;
                  }
                if (((prog_counter == 14) && (playerX == 0)&& (playerY == 1)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 15;
                    continue;
                  }
                if (((prog_counter == 18) && (playerX == 1)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 22;
                    continue;
                  }
                if (((prog_counter == 23) && (playerX == 3)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 22;
                    continue;
                  }
                if ((prog_counter == 26))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX - 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? (playerY - 1) : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : ((playerY == 3) ? 1 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 18 : 1))))) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 22 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 18 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 16) && (playerX == 0)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 18;
                    continue;
                  }
                {
                  prog_counter = 26;
                  continue;
                }
              }
            continue;
          }
        if (((prog_counter == 12) && (playerY == 1)&& (playerX == 0)))
          {
            for(;;)
              {
                if ((prog_counter == 1))
                  break;
                if (((prog_counter == 2) && (playerY == 3)&& (playerX == 3)))
                  break;
                if (((prog_counter == 3) && (playerX == 2)&& (playerY == 3)))
                  break;
                if (((prog_counter == 4) && (playerX == 2)&& (playerY == 2)))
                  break;
                if (((prog_counter == 5) && (playerY == 3)&& (playerX == 1)))
                  break;
                if (((prog_counter == 6) && (playerX == 2)&& (playerY == 1)))
                  break;
                if (((prog_counter == 7) && (playerY == 2)&& (playerX == 1)))
                  break;
                if (((prog_counter == 8) && (playerY == 0)&& (playerX == 2)))
                  break;
                if (((prog_counter == 9) && (playerY == 2)&& (playerX == 0)))
                  break;
                if (((prog_counter == 10) && (playerY == 0)&& (playerX == 3)))
                  break;
                if (((prog_counter == 11) && (playerY == 0)&& (playerX == 1)))
                  break;
                if (((prog_counter == 12) && (playerY == 1)&& (playerX == 0)))
                  break;
                if (((prog_counter == 13) && (playerY == 0)&& (playerX == 0)))
                  break;
                if (((prog_counter == 25) && (playerX == 3)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 3;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))&& ((!((playerY == 2))) || (3 <= playerX)|| (playerX <= 0))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (3 <= playerX)|| (playerX <= 0))&& ((!((playerY == 1))) || (3 <= playerX)|| (playerX <= 0)|| (playerX == 1))&& ((!((playerX == 1))) || (!((playerY == 3))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? (playerX - 1) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? (playerX + 1) : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? (playerY - 1) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? 1 : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 25 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 21) && (playerY == 3)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 25;
                    continue;
                  }
                if (((prog_counter == 19) && (playerX == 1)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 20) && (playerY == 2)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (playerX <= 0)|| (3 <= playerX))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 20 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 21 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 17) && (playerX == 1)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 24) && (playerY == 1)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 15) && (playerX == 0)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 17;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : (playerX - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX - 1) : ((playerX > 0) ? ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1))) : (playerX - 1))))))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : (playerY - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : 1)) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 24 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 22) && (playerX == 2)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 24;
                    continue;
                  }
                if (((prog_counter == 14) && (playerX == 0)&& (playerY == 1)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 15;
                    continue;
                  }
                if (((prog_counter == 18) && (playerX == 1)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 22;
                    continue;
                  }
                if (((prog_counter == 23) && (playerX == 3)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 22;
                    continue;
                  }
                if ((prog_counter == 26))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX - 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? (playerY - 1) : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : ((playerY == 3) ? 1 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 18 : 1))))) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 22 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 18 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 16) && (playerX == 0)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 18;
                    continue;
                  }
                {
                  prog_counter = 26;
                  continue;
                }
              }
            continue;
          }
        if (((prog_counter == 13) && (playerY == 0)&& (playerX == 0)))
          {
            for(;;)
              {
                if ((prog_counter == 1))
                  break;
                if (((prog_counter == 2) && (playerY == 3)&& (playerX == 3)))
                  break;
                if (((prog_counter == 3) && (playerX == 2)&& (playerY == 3)))
                  break;
                if (((prog_counter == 4) && (playerX == 2)&& (playerY == 2)))
                  break;
                if (((prog_counter == 5) && (playerY == 3)&& (playerX == 1)))
                  break;
                if (((prog_counter == 6) && (playerX == 2)&& (playerY == 1)))
                  break;
                if (((prog_counter == 7) && (playerY == 2)&& (playerX == 1)))
                  break;
                if (((prog_counter == 8) && (playerY == 0)&& (playerX == 2)))
                  break;
                if (((prog_counter == 9) && (playerY == 2)&& (playerX == 0)))
                  break;
                if (((prog_counter == 10) && (playerY == 0)&& (playerX == 3)))
                  break;
                if (((prog_counter == 11) && (playerY == 0)&& (playerX == 1)))
                  break;
                if (((prog_counter == 12) && (playerY == 1)&& (playerX == 0)))
                  break;
                if (((prog_counter == 13) && (playerY == 0)&& (playerX == 0)))
                  break;
                if (((prog_counter == 25) && (playerX == 3)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 3;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))&& ((!((playerY == 2))) || (3 <= playerX)|| (playerX <= 0))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (3 <= playerX)|| (playerX <= 0))&& ((!((playerY == 1))) || (3 <= playerX)|| (playerX <= 0)|| (playerX == 1))&& ((!((playerX == 1))) || (!((playerY == 3))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? (playerX - 1) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? (playerX + 1) : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? (playerY - 1) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? 1 : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 25 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 21) && (playerY == 3)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 25;
                    continue;
                  }
                if (((prog_counter == 19) && (playerX == 1)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 20) && (playerY == 2)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (playerX <= 0)|| (3 <= playerX))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 20 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 21 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 17) && (playerX == 1)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 24) && (playerY == 1)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 15) && (playerX == 0)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 17;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : (playerX - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX - 1) : ((playerX > 0) ? ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1))) : (playerX - 1))))))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : (playerY - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : 1)) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 24 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 22) && (playerX == 2)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 24;
                    continue;
                  }
                if (((prog_counter == 14) && (playerX == 0)&& (playerY == 1)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 15;
                    continue;
                  }
                if (((prog_counter == 18) && (playerX == 1)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 22;
                    continue;
                  }
                if (((prog_counter == 23) && (playerX == 3)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 22;
                    continue;
                  }
                if ((prog_counter == 26))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX - 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? (playerY - 1) : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : ((playerY == 3) ? 1 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 18 : 1))))) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 22 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 18 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 16) && (playerX == 0)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 18;
                    continue;
                  }
                {
                  prog_counter = 26;
                  continue;
                }
              }
            continue;
          }
        if (((prog_counter == 14) && (playerX == 0)&& (playerY == 1)))
          {
            for(;;)
              {
                if ((prog_counter == 1))
                  break;
                if (((prog_counter == 2) && (playerY == 3)&& (playerX == 3)))
                  break;
                if (((prog_counter == 3) && (playerX == 2)&& (playerY == 3)))
                  break;
                if (((prog_counter == 4) && (playerX == 2)&& (playerY == 2)))
                  break;
                if (((prog_counter == 5) && (playerY == 3)&& (playerX == 1)))
                  break;
                if (((prog_counter == 6) && (playerX == 2)&& (playerY == 1)))
                  break;
                if (((prog_counter == 7) && (playerY == 2)&& (playerX == 1)))
                  break;
                if (((prog_counter == 8) && (playerY == 0)&& (playerX == 2)))
                  break;
                if (((prog_counter == 9) && (playerY == 2)&& (playerX == 0)))
                  break;
                if (((prog_counter == 10) && (playerY == 0)&& (playerX == 3)))
                  break;
                if (((prog_counter == 11) && (playerY == 0)&& (playerX == 1)))
                  break;
                if (((prog_counter == 12) && (playerY == 1)&& (playerX == 0)))
                  break;
                if (((prog_counter == 13) && (playerY == 0)&& (playerX == 0)))
                  break;
                if (((prog_counter == 25) && (playerX == 3)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 3;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))&& ((!((playerY == 2))) || (3 <= playerX)|| (playerX <= 0))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (3 <= playerX)|| (playerX <= 0))&& ((!((playerY == 1))) || (3 <= playerX)|| (playerX <= 0)|| (playerX == 1))&& ((!((playerX == 1))) || (!((playerY == 3))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? (playerX - 1) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? (playerX + 1) : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? (playerY - 1) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? 1 : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 25 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 21) && (playerY == 3)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 25;
                    continue;
                  }
                if (((prog_counter == 19) && (playerX == 1)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 20) && (playerY == 2)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (playerX <= 0)|| (3 <= playerX))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 20 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 21 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 17) && (playerX == 1)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 24) && (playerY == 1)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 15) && (playerX == 0)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 17;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : (playerX - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX - 1) : ((playerX > 0) ? ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1))) : (playerX - 1))))))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : (playerY - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : 1)) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 24 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 22) && (playerX == 2)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 24;
                    continue;
                  }
                if (((prog_counter == 14) && (playerX == 0)&& (playerY == 1)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 15;
                    continue;
                  }
                if (((prog_counter == 18) && (playerX == 1)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 22;
                    continue;
                  }
                if (((prog_counter == 23) && (playerX == 3)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 22;
                    continue;
                  }
                if ((prog_counter == 26))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX - 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? (playerY - 1) : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : ((playerY == 3) ? 1 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 18 : 1))))) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 22 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 18 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 16) && (playerX == 0)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 18;
                    continue;
                  }
                {
                  prog_counter = 26;
                  continue;
                }
              }
            continue;
          }
        if (((prog_counter == 15) && (playerX == 0)&& (playerY == 2)))
          {
            for(;;)
              {
                if ((prog_counter == 1))
                  break;
                if (((prog_counter == 2) && (playerY == 3)&& (playerX == 3)))
                  break;
                if (((prog_counter == 3) && (playerX == 2)&& (playerY == 3)))
                  break;
                if (((prog_counter == 4) && (playerX == 2)&& (playerY == 2)))
                  break;
                if (((prog_counter == 5) && (playerY == 3)&& (playerX == 1)))
                  break;
                if (((prog_counter == 6) && (playerX == 2)&& (playerY == 1)))
                  break;
                if (((prog_counter == 7) && (playerY == 2)&& (playerX == 1)))
                  break;
                if (((prog_counter == 8) && (playerY == 0)&& (playerX == 2)))
                  break;
                if (((prog_counter == 9) && (playerY == 2)&& (playerX == 0)))
                  break;
                if (((prog_counter == 10) && (playerY == 0)&& (playerX == 3)))
                  break;
                if (((prog_counter == 11) && (playerY == 0)&& (playerX == 1)))
                  break;
                if (((prog_counter == 12) && (playerY == 1)&& (playerX == 0)))
                  break;
                if (((prog_counter == 13) && (playerY == 0)&& (playerX == 0)))
                  break;
                if (((prog_counter == 25) && (playerX == 3)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 3;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))&& ((!((playerY == 2))) || (3 <= playerX)|| (playerX <= 0))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (3 <= playerX)|| (playerX <= 0))&& ((!((playerY == 1))) || (3 <= playerX)|| (playerX <= 0)|| (playerX == 1))&& ((!((playerX == 1))) || (!((playerY == 3))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? (playerX - 1) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? (playerX + 1) : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? (playerY - 1) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? 1 : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 25 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 21) && (playerY == 3)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 25;
                    continue;
                  }
                if (((prog_counter == 19) && (playerX == 1)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 20) && (playerY == 2)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (playerX <= 0)|| (3 <= playerX))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 20 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 21 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 17) && (playerX == 1)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 24) && (playerY == 1)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 15) && (playerX == 0)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 17;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : (playerX - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX - 1) : ((playerX > 0) ? ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1))) : (playerX - 1))))))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : (playerY - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : 1)) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 24 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 22) && (playerX == 2)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 24;
                    continue;
                  }
                if (((prog_counter == 14) && (playerX == 0)&& (playerY == 1)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 15;
                    continue;
                  }
                if (((prog_counter == 18) && (playerX == 1)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 22;
                    continue;
                  }
                if (((prog_counter == 23) && (playerX == 3)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 22;
                    continue;
                  }
                if ((prog_counter == 26))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX - 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? (playerY - 1) : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : ((playerY == 3) ? 1 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 18 : 1))))) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 22 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 18 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 16) && (playerX == 0)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 18;
                    continue;
                  }
                {
                  prog_counter = 26;
                  continue;
                }
              }
            continue;
          }
        if (((prog_counter == 16) && (playerX == 0)&& (playerY == 0)))
          {
            for(;;)
              {
                if ((prog_counter == 1))
                  break;
                if (((prog_counter == 2) && (playerY == 3)&& (playerX == 3)))
                  break;
                if (((prog_counter == 3) && (playerX == 2)&& (playerY == 3)))
                  break;
                if (((prog_counter == 4) && (playerX == 2)&& (playerY == 2)))
                  break;
                if (((prog_counter == 5) && (playerY == 3)&& (playerX == 1)))
                  break;
                if (((prog_counter == 6) && (playerX == 2)&& (playerY == 1)))
                  break;
                if (((prog_counter == 7) && (playerY == 2)&& (playerX == 1)))
                  break;
                if (((prog_counter == 8) && (playerY == 0)&& (playerX == 2)))
                  break;
                if (((prog_counter == 9) && (playerY == 2)&& (playerX == 0)))
                  break;
                if (((prog_counter == 10) && (playerY == 0)&& (playerX == 3)))
                  break;
                if (((prog_counter == 11) && (playerY == 0)&& (playerX == 1)))
                  break;
                if (((prog_counter == 12) && (playerY == 1)&& (playerX == 0)))
                  break;
                if (((prog_counter == 13) && (playerY == 0)&& (playerX == 0)))
                  break;
                if (((prog_counter == 25) && (playerX == 3)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 3;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))&& ((!((playerY == 2))) || (3 <= playerX)|| (playerX <= 0))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (3 <= playerX)|| (playerX <= 0))&& ((!((playerY == 1))) || (3 <= playerX)|| (playerX <= 0)|| (playerX == 1))&& ((!((playerX == 1))) || (!((playerY == 3))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? (playerX - 1) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? (playerX + 1) : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? (playerY - 1) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? 1 : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 25 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 21) && (playerY == 3)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 25;
                    continue;
                  }
                if (((prog_counter == 19) && (playerX == 1)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 20) && (playerY == 2)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (playerX <= 0)|| (3 <= playerX))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 20 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 21 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 17) && (playerX == 1)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 24) && (playerY == 1)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 15) && (playerX == 0)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 17;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : (playerX - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX - 1) : ((playerX > 0) ? ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1))) : (playerX - 1))))))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : (playerY - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : 1)) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 24 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 22) && (playerX == 2)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 24;
                    continue;
                  }
                if (((prog_counter == 14) && (playerX == 0)&& (playerY == 1)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 15;
                    continue;
                  }
                if (((prog_counter == 18) && (playerX == 1)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 22;
                    continue;
                  }
                if (((prog_counter == 23) && (playerX == 3)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 22;
                    continue;
                  }
                if ((prog_counter == 26))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX - 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? (playerY - 1) : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : ((playerY == 3) ? 1 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 18 : 1))))) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 22 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 18 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 16) && (playerX == 0)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 18;
                    continue;
                  }
                {
                  prog_counter = 26;
                  continue;
                }
              }
            continue;
          }
        if (((prog_counter == 17) && (playerX == 1)&& (playerY == 2)))
          {
            for(;;)
              {
                if ((prog_counter == 1))
                  break;
                if (((prog_counter == 2) && (playerY == 3)&& (playerX == 3)))
                  break;
                if (((prog_counter == 3) && (playerX == 2)&& (playerY == 3)))
                  break;
                if (((prog_counter == 4) && (playerX == 2)&& (playerY == 2)))
                  break;
                if (((prog_counter == 5) && (playerY == 3)&& (playerX == 1)))
                  break;
                if (((prog_counter == 6) && (playerX == 2)&& (playerY == 1)))
                  break;
                if (((prog_counter == 7) && (playerY == 2)&& (playerX == 1)))
                  break;
                if (((prog_counter == 8) && (playerY == 0)&& (playerX == 2)))
                  break;
                if (((prog_counter == 9) && (playerY == 2)&& (playerX == 0)))
                  break;
                if (((prog_counter == 10) && (playerY == 0)&& (playerX == 3)))
                  break;
                if (((prog_counter == 11) && (playerY == 0)&& (playerX == 1)))
                  break;
                if (((prog_counter == 12) && (playerY == 1)&& (playerX == 0)))
                  break;
                if (((prog_counter == 13) && (playerY == 0)&& (playerX == 0)))
                  break;
                if (((prog_counter == 25) && (playerX == 3)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 3;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))&& ((!((playerY == 2))) || (3 <= playerX)|| (playerX <= 0))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (3 <= playerX)|| (playerX <= 0))&& ((!((playerY == 1))) || (3 <= playerX)|| (playerX <= 0)|| (playerX == 1))&& ((!((playerX == 1))) || (!((playerY == 3))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? (playerX - 1) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? (playerX + 1) : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? (playerY - 1) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? 1 : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 25 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 21) && (playerY == 3)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 25;
                    continue;
                  }
                if (((prog_counter == 19) && (playerX == 1)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 20) && (playerY == 2)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (playerX <= 0)|| (3 <= playerX))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 20 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 21 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 17) && (playerX == 1)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 24) && (playerY == 1)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 15) && (playerX == 0)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 17;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : (playerX - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX - 1) : ((playerX > 0) ? ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1))) : (playerX - 1))))))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : (playerY - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : 1)) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 24 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 22) && (playerX == 2)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 24;
                    continue;
                  }
                if (((prog_counter == 14) && (playerX == 0)&& (playerY == 1)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 15;
                    continue;
                  }
                if (((prog_counter == 18) && (playerX == 1)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 22;
                    continue;
                  }
                if (((prog_counter == 23) && (playerX == 3)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 22;
                    continue;
                  }
                if ((prog_counter == 26))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX - 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? (playerY - 1) : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : ((playerY == 3) ? 1 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 18 : 1))))) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 22 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 18 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 16) && (playerX == 0)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 18;
                    continue;
                  }
                {
                  prog_counter = 26;
                  continue;
                }
              }
            continue;
          }
        if (((prog_counter == 18) && (playerX == 1)&& (playerY == 0)))
          {
            for(;;)
              {
                if ((prog_counter == 1))
                  break;
                if (((prog_counter == 2) && (playerY == 3)&& (playerX == 3)))
                  break;
                if (((prog_counter == 3) && (playerX == 2)&& (playerY == 3)))
                  break;
                if (((prog_counter == 4) && (playerX == 2)&& (playerY == 2)))
                  break;
                if (((prog_counter == 5) && (playerY == 3)&& (playerX == 1)))
                  break;
                if (((prog_counter == 6) && (playerX == 2)&& (playerY == 1)))
                  break;
                if (((prog_counter == 7) && (playerY == 2)&& (playerX == 1)))
                  break;
                if (((prog_counter == 8) && (playerY == 0)&& (playerX == 2)))
                  break;
                if (((prog_counter == 9) && (playerY == 2)&& (playerX == 0)))
                  break;
                if (((prog_counter == 10) && (playerY == 0)&& (playerX == 3)))
                  break;
                if (((prog_counter == 11) && (playerY == 0)&& (playerX == 1)))
                  break;
                if (((prog_counter == 12) && (playerY == 1)&& (playerX == 0)))
                  break;
                if (((prog_counter == 13) && (playerY == 0)&& (playerX == 0)))
                  break;
                if (((prog_counter == 25) && (playerX == 3)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 3;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))&& ((!((playerY == 2))) || (3 <= playerX)|| (playerX <= 0))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (3 <= playerX)|| (playerX <= 0))&& ((!((playerY == 1))) || (3 <= playerX)|| (playerX <= 0)|| (playerX == 1))&& ((!((playerX == 1))) || (!((playerY == 3))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? (playerX - 1) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? (playerX + 1) : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? (playerY - 1) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? 1 : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 25 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 21) && (playerY == 3)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 25;
                    continue;
                  }
                if (((prog_counter == 19) && (playerX == 1)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 20) && (playerY == 2)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (playerX <= 0)|| (3 <= playerX))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 20 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 21 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 17) && (playerX == 1)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 24) && (playerY == 1)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 15) && (playerX == 0)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 17;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : (playerX - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX - 1) : ((playerX > 0) ? ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1))) : (playerX - 1))))))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : (playerY - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : 1)) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 24 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 22) && (playerX == 2)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 24;
                    continue;
                  }
                if (((prog_counter == 14) && (playerX == 0)&& (playerY == 1)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 15;
                    continue;
                  }
                if (((prog_counter == 18) && (playerX == 1)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 22;
                    continue;
                  }
                if (((prog_counter == 23) && (playerX == 3)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 22;
                    continue;
                  }
                if ((prog_counter == 26))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX - 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? (playerY - 1) : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : ((playerY == 3) ? 1 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 18 : 1))))) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 22 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 18 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 16) && (playerX == 0)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 18;
                    continue;
                  }
                {
                  prog_counter = 26;
                  continue;
                }
              }
            continue;
          }
        if (((prog_counter == 19) && (playerX == 1)&& (playerY == 3)))
          {
            for(;;)
              {
                if ((prog_counter == 1))
                  break;
                if (((prog_counter == 2) && (playerY == 3)&& (playerX == 3)))
                  break;
                if (((prog_counter == 3) && (playerX == 2)&& (playerY == 3)))
                  break;
                if (((prog_counter == 4) && (playerX == 2)&& (playerY == 2)))
                  break;
                if (((prog_counter == 5) && (playerY == 3)&& (playerX == 1)))
                  break;
                if (((prog_counter == 6) && (playerX == 2)&& (playerY == 1)))
                  break;
                if (((prog_counter == 7) && (playerY == 2)&& (playerX == 1)))
                  break;
                if (((prog_counter == 8) && (playerY == 0)&& (playerX == 2)))
                  break;
                if (((prog_counter == 9) && (playerY == 2)&& (playerX == 0)))
                  break;
                if (((prog_counter == 10) && (playerY == 0)&& (playerX == 3)))
                  break;
                if (((prog_counter == 11) && (playerY == 0)&& (playerX == 1)))
                  break;
                if (((prog_counter == 12) && (playerY == 1)&& (playerX == 0)))
                  break;
                if (((prog_counter == 13) && (playerY == 0)&& (playerX == 0)))
                  break;
                if (((prog_counter == 25) && (playerX == 3)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 3;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))&& ((!((playerY == 2))) || (3 <= playerX)|| (playerX <= 0))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (3 <= playerX)|| (playerX <= 0))&& ((!((playerY == 1))) || (3 <= playerX)|| (playerX <= 0)|| (playerX == 1))&& ((!((playerX == 1))) || (!((playerY == 3))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? (playerX - 1) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? (playerX + 1) : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? (playerY - 1) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? 1 : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 25 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 21) && (playerY == 3)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 25;
                    continue;
                  }
                if (((prog_counter == 19) && (playerX == 1)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 20) && (playerY == 2)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (playerX <= 0)|| (3 <= playerX))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 20 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 21 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 17) && (playerX == 1)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 24) && (playerY == 1)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 15) && (playerX == 0)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 17;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : (playerX - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX - 1) : ((playerX > 0) ? ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1))) : (playerX - 1))))))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : (playerY - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : 1)) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 24 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 22) && (playerX == 2)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 24;
                    continue;
                  }
                if (((prog_counter == 14) && (playerX == 0)&& (playerY == 1)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 15;
                    continue;
                  }
                if (((prog_counter == 18) && (playerX == 1)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 22;
                    continue;
                  }
                if (((prog_counter == 23) && (playerX == 3)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 22;
                    continue;
                  }
                if ((prog_counter == 26))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX - 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? (playerY - 1) : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : ((playerY == 3) ? 1 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 18 : 1))))) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 22 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 18 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 16) && (playerX == 0)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 18;
                    continue;
                  }
                {
                  prog_counter = 26;
                  continue;
                }
              }
            continue;
          }
        if (((prog_counter == 20) && (playerY == 2)&& (playerX == 2)))
          {
            for(;;)
              {
                if ((prog_counter == 1))
                  break;
                if (((prog_counter == 2) && (playerY == 3)&& (playerX == 3)))
                  break;
                if (((prog_counter == 3) && (playerX == 2)&& (playerY == 3)))
                  break;
                if (((prog_counter == 4) && (playerX == 2)&& (playerY == 2)))
                  break;
                if (((prog_counter == 5) && (playerY == 3)&& (playerX == 1)))
                  break;
                if (((prog_counter == 6) && (playerX == 2)&& (playerY == 1)))
                  break;
                if (((prog_counter == 7) && (playerY == 2)&& (playerX == 1)))
                  break;
                if (((prog_counter == 8) && (playerY == 0)&& (playerX == 2)))
                  break;
                if (((prog_counter == 9) && (playerY == 2)&& (playerX == 0)))
                  break;
                if (((prog_counter == 10) && (playerY == 0)&& (playerX == 3)))
                  break;
                if (((prog_counter == 11) && (playerY == 0)&& (playerX == 1)))
                  break;
                if (((prog_counter == 12) && (playerY == 1)&& (playerX == 0)))
                  break;
                if (((prog_counter == 13) && (playerY == 0)&& (playerX == 0)))
                  break;
                if (((prog_counter == 25) && (playerX == 3)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 3;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))&& ((!((playerY == 2))) || (3 <= playerX)|| (playerX <= 0))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (3 <= playerX)|| (playerX <= 0))&& ((!((playerY == 1))) || (3 <= playerX)|| (playerX <= 0)|| (playerX == 1))&& ((!((playerX == 1))) || (!((playerY == 3))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? (playerX - 1) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? (playerX + 1) : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? (playerY - 1) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? 1 : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 25 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 21) && (playerY == 3)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 25;
                    continue;
                  }
                if (((prog_counter == 19) && (playerX == 1)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 20) && (playerY == 2)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (playerX <= 0)|| (3 <= playerX))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 20 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 21 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 17) && (playerX == 1)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 24) && (playerY == 1)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 15) && (playerX == 0)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 17;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : (playerX - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX - 1) : ((playerX > 0) ? ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1))) : (playerX - 1))))))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : (playerY - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : 1)) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 24 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 22) && (playerX == 2)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 24;
                    continue;
                  }
                if (((prog_counter == 14) && (playerX == 0)&& (playerY == 1)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 15;
                    continue;
                  }
                if (((prog_counter == 18) && (playerX == 1)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 22;
                    continue;
                  }
                if (((prog_counter == 23) && (playerX == 3)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 22;
                    continue;
                  }
                if ((prog_counter == 26))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX - 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? (playerY - 1) : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : ((playerY == 3) ? 1 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 18 : 1))))) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 22 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 18 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 16) && (playerX == 0)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 18;
                    continue;
                  }
                {
                  prog_counter = 26;
                  continue;
                }
              }
            continue;
          }
        if (((prog_counter == 21) && (playerY == 3)&& (playerX == 2)))
          {
            for(;;)
              {
                if ((prog_counter == 1))
                  break;
                if (((prog_counter == 2) && (playerY == 3)&& (playerX == 3)))
                  break;
                if (((prog_counter == 3) && (playerX == 2)&& (playerY == 3)))
                  break;
                if (((prog_counter == 4) && (playerX == 2)&& (playerY == 2)))
                  break;
                if (((prog_counter == 5) && (playerY == 3)&& (playerX == 1)))
                  break;
                if (((prog_counter == 6) && (playerX == 2)&& (playerY == 1)))
                  break;
                if (((prog_counter == 7) && (playerY == 2)&& (playerX == 1)))
                  break;
                if (((prog_counter == 8) && (playerY == 0)&& (playerX == 2)))
                  break;
                if (((prog_counter == 9) && (playerY == 2)&& (playerX == 0)))
                  break;
                if (((prog_counter == 10) && (playerY == 0)&& (playerX == 3)))
                  break;
                if (((prog_counter == 11) && (playerY == 0)&& (playerX == 1)))
                  break;
                if (((prog_counter == 12) && (playerY == 1)&& (playerX == 0)))
                  break;
                if (((prog_counter == 13) && (playerY == 0)&& (playerX == 0)))
                  break;
                if (((prog_counter == 25) && (playerX == 3)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 3;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))&& ((!((playerY == 2))) || (3 <= playerX)|| (playerX <= 0))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (3 <= playerX)|| (playerX <= 0))&& ((!((playerY == 1))) || (3 <= playerX)|| (playerX <= 0)|| (playerX == 1))&& ((!((playerX == 1))) || (!((playerY == 3))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? (playerX - 1) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? (playerX + 1) : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? (playerY - 1) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? 1 : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 25 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 21) && (playerY == 3)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 25;
                    continue;
                  }
                if (((prog_counter == 19) && (playerX == 1)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 20) && (playerY == 2)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (playerX <= 0)|| (3 <= playerX))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 20 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 21 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 17) && (playerX == 1)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 24) && (playerY == 1)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 15) && (playerX == 0)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 17;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : (playerX - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX - 1) : ((playerX > 0) ? ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1))) : (playerX - 1))))))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : (playerY - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : 1)) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 24 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 22) && (playerX == 2)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 24;
                    continue;
                  }
                if (((prog_counter == 14) && (playerX == 0)&& (playerY == 1)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 15;
                    continue;
                  }
                if (((prog_counter == 18) && (playerX == 1)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 22;
                    continue;
                  }
                if (((prog_counter == 23) && (playerX == 3)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 22;
                    continue;
                  }
                if ((prog_counter == 26))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX - 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? (playerY - 1) : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : ((playerY == 3) ? 1 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 18 : 1))))) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 22 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 18 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 16) && (playerX == 0)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 18;
                    continue;
                  }
                {
                  prog_counter = 26;
                  continue;
                }
              }
            continue;
          }
        if (((prog_counter == 22) && (playerX == 2)&& (playerY == 0)))
          {
            for(;;)
              {
                if ((prog_counter == 1))
                  break;
                if (((prog_counter == 2) && (playerY == 3)&& (playerX == 3)))
                  break;
                if (((prog_counter == 3) && (playerX == 2)&& (playerY == 3)))
                  break;
                if (((prog_counter == 4) && (playerX == 2)&& (playerY == 2)))
                  break;
                if (((prog_counter == 5) && (playerY == 3)&& (playerX == 1)))
                  break;
                if (((prog_counter == 6) && (playerX == 2)&& (playerY == 1)))
                  break;
                if (((prog_counter == 7) && (playerY == 2)&& (playerX == 1)))
                  break;
                if (((prog_counter == 8) && (playerY == 0)&& (playerX == 2)))
                  break;
                if (((prog_counter == 9) && (playerY == 2)&& (playerX == 0)))
                  break;
                if (((prog_counter == 10) && (playerY == 0)&& (playerX == 3)))
                  break;
                if (((prog_counter == 11) && (playerY == 0)&& (playerX == 1)))
                  break;
                if (((prog_counter == 12) && (playerY == 1)&& (playerX == 0)))
                  break;
                if (((prog_counter == 13) && (playerY == 0)&& (playerX == 0)))
                  break;
                if (((prog_counter == 25) && (playerX == 3)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 3;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))&& ((!((playerY == 2))) || (3 <= playerX)|| (playerX <= 0))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (3 <= playerX)|| (playerX <= 0))&& ((!((playerY == 1))) || (3 <= playerX)|| (playerX <= 0)|| (playerX == 1))&& ((!((playerX == 1))) || (!((playerY == 3))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? (playerX - 1) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? (playerX + 1) : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? (playerY - 1) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? 1 : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 25 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 21) && (playerY == 3)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 25;
                    continue;
                  }
                if (((prog_counter == 19) && (playerX == 1)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 20) && (playerY == 2)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (playerX <= 0)|| (3 <= playerX))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 20 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 21 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 17) && (playerX == 1)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 24) && (playerY == 1)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 15) && (playerX == 0)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 17;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : (playerX - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX - 1) : ((playerX > 0) ? ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1))) : (playerX - 1))))))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : (playerY - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : 1)) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 24 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 22) && (playerX == 2)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 24;
                    continue;
                  }
                if (((prog_counter == 14) && (playerX == 0)&& (playerY == 1)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 15;
                    continue;
                  }
                if (((prog_counter == 18) && (playerX == 1)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 22;
                    continue;
                  }
                if (((prog_counter == 23) && (playerX == 3)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 22;
                    continue;
                  }
                if ((prog_counter == 26))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX - 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? (playerY - 1) : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : ((playerY == 3) ? 1 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 18 : 1))))) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 22 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 18 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 16) && (playerX == 0)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 18;
                    continue;
                  }
                {
                  prog_counter = 26;
                  continue;
                }
              }
            continue;
          }
        if (((prog_counter == 23) && (playerX == 3)&& (playerY == 0)))
          {
            for(;;)
              {
                if ((prog_counter == 1))
                  break;
                if (((prog_counter == 2) && (playerY == 3)&& (playerX == 3)))
                  break;
                if (((prog_counter == 3) && (playerX == 2)&& (playerY == 3)))
                  break;
                if (((prog_counter == 4) && (playerX == 2)&& (playerY == 2)))
                  break;
                if (((prog_counter == 5) && (playerY == 3)&& (playerX == 1)))
                  break;
                if (((prog_counter == 6) && (playerX == 2)&& (playerY == 1)))
                  break;
                if (((prog_counter == 7) && (playerY == 2)&& (playerX == 1)))
                  break;
                if (((prog_counter == 8) && (playerY == 0)&& (playerX == 2)))
                  break;
                if (((prog_counter == 9) && (playerY == 2)&& (playerX == 0)))
                  break;
                if (((prog_counter == 10) && (playerY == 0)&& (playerX == 3)))
                  break;
                if (((prog_counter == 11) && (playerY == 0)&& (playerX == 1)))
                  break;
                if (((prog_counter == 12) && (playerY == 1)&& (playerX == 0)))
                  break;
                if (((prog_counter == 13) && (playerY == 0)&& (playerX == 0)))
                  break;
                if (((prog_counter == 25) && (playerX == 3)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 3;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))&& ((!((playerY == 2))) || (3 <= playerX)|| (playerX <= 0))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (3 <= playerX)|| (playerX <= 0))&& ((!((playerY == 1))) || (3 <= playerX)|| (playerX <= 0)|| (playerX == 1))&& ((!((playerX == 1))) || (!((playerY == 3))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? (playerX - 1) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? (playerX + 1) : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? (playerY - 1) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? 1 : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 25 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 21) && (playerY == 3)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 25;
                    continue;
                  }
                if (((prog_counter == 19) && (playerX == 1)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 20) && (playerY == 2)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (playerX <= 0)|| (3 <= playerX))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 20 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 21 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 17) && (playerX == 1)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 24) && (playerY == 1)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 15) && (playerX == 0)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 17;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : (playerX - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX - 1) : ((playerX > 0) ? ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1))) : (playerX - 1))))))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : (playerY - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : 1)) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 24 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 22) && (playerX == 2)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 24;
                    continue;
                  }
                if (((prog_counter == 14) && (playerX == 0)&& (playerY == 1)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 15;
                    continue;
                  }
                if (((prog_counter == 18) && (playerX == 1)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 22;
                    continue;
                  }
                if (((prog_counter == 23) && (playerX == 3)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 22;
                    continue;
                  }
                if ((prog_counter == 26))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX - 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? (playerY - 1) : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : ((playerY == 3) ? 1 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 18 : 1))))) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 22 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 18 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 16) && (playerX == 0)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 18;
                    continue;
                  }
                {
                  prog_counter = 26;
                  continue;
                }
              }
            continue;
          }
        if (((prog_counter == 24) && (playerY == 1)&& (playerX == 2)))
          {
            for(;;)
              {
                if ((prog_counter == 1))
                  break;
                if (((prog_counter == 2) && (playerY == 3)&& (playerX == 3)))
                  break;
                if (((prog_counter == 3) && (playerX == 2)&& (playerY == 3)))
                  break;
                if (((prog_counter == 4) && (playerX == 2)&& (playerY == 2)))
                  break;
                if (((prog_counter == 5) && (playerY == 3)&& (playerX == 1)))
                  break;
                if (((prog_counter == 6) && (playerX == 2)&& (playerY == 1)))
                  break;
                if (((prog_counter == 7) && (playerY == 2)&& (playerX == 1)))
                  break;
                if (((prog_counter == 8) && (playerY == 0)&& (playerX == 2)))
                  break;
                if (((prog_counter == 9) && (playerY == 2)&& (playerX == 0)))
                  break;
                if (((prog_counter == 10) && (playerY == 0)&& (playerX == 3)))
                  break;
                if (((prog_counter == 11) && (playerY == 0)&& (playerX == 1)))
                  break;
                if (((prog_counter == 12) && (playerY == 1)&& (playerX == 0)))
                  break;
                if (((prog_counter == 13) && (playerY == 0)&& (playerX == 0)))
                  break;
                if (((prog_counter == 25) && (playerX == 3)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 3;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))&& ((!((playerY == 2))) || (3 <= playerX)|| (playerX <= 0))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (3 <= playerX)|| (playerX <= 0))&& ((!((playerY == 1))) || (3 <= playerX)|| (playerX <= 0)|| (playerX == 1))&& ((!((playerX == 1))) || (!((playerY == 3))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? (playerX - 1) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? (playerX + 1) : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? (playerY - 1) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? 1 : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 25 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 21) && (playerY == 3)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 25;
                    continue;
                  }
                if (((prog_counter == 19) && (playerX == 1)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 20) && (playerY == 2)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (playerX <= 0)|| (3 <= playerX))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 20 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 21 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 17) && (playerX == 1)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 24) && (playerY == 1)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 15) && (playerX == 0)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 17;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : (playerX - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX - 1) : ((playerX > 0) ? ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1))) : (playerX - 1))))))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : (playerY - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : 1)) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 24 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 22) && (playerX == 2)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 24;
                    continue;
                  }
                if (((prog_counter == 14) && (playerX == 0)&& (playerY == 1)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 15;
                    continue;
                  }
                if (((prog_counter == 18) && (playerX == 1)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 22;
                    continue;
                  }
                if (((prog_counter == 23) && (playerX == 3)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 22;
                    continue;
                  }
                if ((prog_counter == 26))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX - 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? (playerY - 1) : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : ((playerY == 3) ? 1 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 18 : 1))))) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 22 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 18 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 16) && (playerX == 0)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 18;
                    continue;
                  }
                {
                  prog_counter = 26;
                  continue;
                }
              }
            continue;
          }
        if (((prog_counter == 25) && (playerX == 3)&& (playerY == 3)))
          {
            for(;;)
              {
                if ((prog_counter == 1))
                  break;
                if (((prog_counter == 2) && (playerY == 3)&& (playerX == 3)))
                  break;
                if (((prog_counter == 3) && (playerX == 2)&& (playerY == 3)))
                  break;
                if (((prog_counter == 4) && (playerX == 2)&& (playerY == 2)))
                  break;
                if (((prog_counter == 5) && (playerY == 3)&& (playerX == 1)))
                  break;
                if (((prog_counter == 6) && (playerX == 2)&& (playerY == 1)))
                  break;
                if (((prog_counter == 7) && (playerY == 2)&& (playerX == 1)))
                  break;
                if (((prog_counter == 8) && (playerY == 0)&& (playerX == 2)))
                  break;
                if (((prog_counter == 9) && (playerY == 2)&& (playerX == 0)))
                  break;
                if (((prog_counter == 10) && (playerY == 0)&& (playerX == 3)))
                  break;
                if (((prog_counter == 11) && (playerY == 0)&& (playerX == 1)))
                  break;
                if (((prog_counter == 12) && (playerY == 1)&& (playerX == 0)))
                  break;
                if (((prog_counter == 13) && (playerY == 0)&& (playerX == 0)))
                  break;
                if (((prog_counter == 25) && (playerX == 3)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 3;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))&& ((!((playerY == 2))) || (3 <= playerX)|| (playerX <= 0))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (3 <= playerX)|| (playerX <= 0))&& ((!((playerY == 1))) || (3 <= playerX)|| (playerX <= 0)|| (playerX == 1))&& ((!((playerX == 1))) || (!((playerY == 3))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? (playerX - 1) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? (playerX + 1) : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? (playerY - 1) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? 1 : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 25 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 21) && (playerY == 3)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 25;
                    continue;
                  }
                if (((prog_counter == 19) && (playerX == 1)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 20) && (playerY == 2)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (playerX <= 0)|| (3 <= playerX))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 20 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 21 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 17) && (playerX == 1)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 24) && (playerY == 1)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 15) && (playerX == 0)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 17;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : (playerX - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX - 1) : ((playerX > 0) ? ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1))) : (playerX - 1))))))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : (playerY - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : 1)) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 24 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 22) && (playerX == 2)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 24;
                    continue;
                  }
                if (((prog_counter == 14) && (playerX == 0)&& (playerY == 1)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 15;
                    continue;
                  }
                if (((prog_counter == 18) && (playerX == 1)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 22;
                    continue;
                  }
                if (((prog_counter == 23) && (playerX == 3)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 22;
                    continue;
                  }
                if ((prog_counter == 26))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX - 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? (playerY - 1) : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : ((playerY == 3) ? 1 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 18 : 1))))) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 22 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 18 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 16) && (playerX == 0)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 18;
                    continue;
                  }
                {
                  prog_counter = 26;
                  continue;
                }
              }
            continue;
          }
        if ((prog_counter == 26))
          {
            for(;;)
              {
                if ((prog_counter == 1))
                  break;
                if (((prog_counter == 2) && (playerY == 3)&& (playerX == 3)))
                  break;
                if (((prog_counter == 3) && (playerX == 2)&& (playerY == 3)))
                  break;
                if (((prog_counter == 4) && (playerX == 2)&& (playerY == 2)))
                  break;
                if (((prog_counter == 5) && (playerY == 3)&& (playerX == 1)))
                  break;
                if (((prog_counter == 6) && (playerX == 2)&& (playerY == 1)))
                  break;
                if (((prog_counter == 7) && (playerY == 2)&& (playerX == 1)))
                  break;
                if (((prog_counter == 8) && (playerY == 0)&& (playerX == 2)))
                  break;
                if (((prog_counter == 9) && (playerY == 2)&& (playerX == 0)))
                  break;
                if (((prog_counter == 10) && (playerY == 0)&& (playerX == 3)))
                  break;
                if (((prog_counter == 11) && (playerY == 0)&& (playerX == 1)))
                  break;
                if (((prog_counter == 12) && (playerY == 1)&& (playerX == 0)))
                  break;
                if (((prog_counter == 13) && (playerY == 0)&& (playerX == 0)))
                  break;
                if (((prog_counter == 25) && (playerX == 3)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 3;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))&& ((!((playerY == 2))) || (3 <= playerX)|| (playerX <= 0))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (3 <= playerX)|| (playerX <= 0))&& ((!((playerY == 1))) || (3 <= playerX)|| (playerX <= 0)|| (playerX == 1))&& ((!((playerX == 1))) || (!((playerY == 3))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? (playerX - 1) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? (playerX + 1) : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? (playerY - 1) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? 1 : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 25 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 21) && (playerY == 3)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 25;
                    continue;
                  }
                if (((prog_counter == 19) && (playerX == 1)&& (playerY == 3)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 20) && (playerY == 2)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 21;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (playerX <= 0)|| (3 <= playerX))&& ((!((playerX == 0))) || (!((playerY == 2))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerX == 0))) || (!((playerY == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? (playerX - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1))))));
                    playerY = ((playerX == 0) ? (playerY - 1) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : (playerY - 1)))))));
                    prog_counter = ((playerX == 0) ? 1 : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 20 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 21 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : 1))))));
                    continue;
                  }
                if (((prog_counter == 17) && (playerX == 1)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 24) && (playerY == 1)&& (playerX == 2)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 20;
                    continue;
                  }
                if (((prog_counter == 15) && (playerX == 0)&& (playerY == 2)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 17;
                    continue;
                  }
                if (((prog_counter == 26) && ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 3))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 0))))&& ((!((playerY <= 0))) || (!((playerY >= 0)))|| (!((playerX == 1))))))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : (playerX - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : (playerX - 1)))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX - 1) : ((playerX > 0) ? ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1))) : (playerX - 1))))))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : (playerY - 1))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : (playerY - 1)))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1)) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? (playerY + 1) : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : 1)) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : 1))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : 1))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 20 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 24 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 22) && (playerX == 2)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 24;
                    continue;
                  }
                if (((prog_counter == 14) && (playerX == 0)&& (playerY == 1)))
                  {
                    read_inputs();
                    playerY = (playerY + 1);
                    prog_counter = 15;
                    continue;
                  }
                if (((prog_counter == 18) && (playerX == 1)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 22;
                    continue;
                  }
                if (((prog_counter == 23) && (playerX == 3)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX - 1);
                    prog_counter = 22;
                    continue;
                  }
                if ((prog_counter == 26))
                  {
                    read_inputs();
                    playerX = ((playerX == 0) ? ((playerY == 1) ? playerX : ((playerY == 2) ? (playerX + 1) : ((playerY == 3) ? (playerX - 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerX - 1) : ((playerY == 2) ? (playerX - 1) : ((playerY == 3) ? (playerX + 1) : ((playerY > 0) ? (playerX - 1) : ((playerY >= 0) ? (playerX + 1) : (playerX - 1)))))) : ((playerX == 3) ? (playerX - 1) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? playerX : (playerX - 1)) : (playerX - 1)) : (playerX - 1)))));
                    playerY = ((playerX == 0) ? ((playerY == 1) ? (playerY + 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? (playerY - 1) : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 1) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? playerY : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerX == 3) ? ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? (playerY - 1) : ((playerY == 3) ? playerY : ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? playerY : (playerY - 1)))))) : ((playerY == 1) ? (playerY - 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1)) : ((playerX > 0) ? ((playerY > 0) ? (playerY - 1) : ((playerY >= 0) ? ((playerX < 3) ? playerY : (playerY - 1)) : (playerY - 1))) : (playerY - 1))))))));
                    prog_counter = ((playerX == 0) ? ((playerY == 1) ? 15 : ((playerY == 2) ? 17 : ((playerY == 3) ? 1 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 18 : 1))))) : ((playerX == 1) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 15 : ((playerY == 3) ? 21 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerX == 3) ? ((playerY == 1) ? 1 : ((playerY == 2) ? 1 : ((playerY == 3) ? 3 : ((playerY > 0) ? 1 : ((playerY >= 0) ? 22 : 1))))) : ((playerY == 1) ? ((playerX > 0) ? ((playerX < 3) ? 22 : 1) : 1) : ((playerY == 2) ? ((playerX > 0) ? ((playerX < 3) ? 17 : 1) : 1) : ((playerY == 3) ? ((playerX > 0) ? ((playerX < 3) ? 19 : 1) : 1) : ((playerX > 0) ? ((playerY > 0) ? 1 : ((playerY >= 0) ? ((playerX < 3) ? 18 : 1) : 1)) : 1)))))));
                    continue;
                  }
                if (((prog_counter == 16) && (playerX == 0)&& (playerY == 0)))
                  {
                    read_inputs();
                    playerX = (playerX + 1);
                    prog_counter = 18;
                    continue;
                  }
                {
                  prog_counter = 26;
                  continue;
                }
              }
            continue;
          }
        {
          prog_counter = 26;
          continue;
        }
      }
  }
}


/* ======================================== CONTROLLER END ======================================== */
