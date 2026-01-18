// Sleep function
#include <unistd.h>

#include <string>
#include <iostream>

// Threading
#include <pthread.h>
#include <chrono>
#include <thread>
#include <atomic>

// Randomization
#include <ctime>
#include <math.h>

// Config File
#include "cJSON.h"

// Prototypes
void read_inputs(void);
void step_controller(void);

// Grid Size
#define GRID_SIZE 7

// ===================== Configuration Constraints =====================

typedef struct {
  int xmin, xmax, ymin, ymax;
  int max_steps;
} Constraints;

static Constraints G;

static bool in_bounds_cfg(int x, int y) {
  return (x >= G.xmin && x <= G.xmax &&
          y >= G.ymin && y <= G.ymax);
}

static void validate_or_abort(int x, int y, int t) {
  if (t > G.max_steps) {
    fprintf(stderr, "ABORT: timeout (t=%d)\n", t);
    exit(255);
  }
  if (!in_bounds_cfg(x, y)) {
    fprintf(stderr, "ABORT: out of bounds (x=%d,y=%d) t=%d\n", x, y, t);
    exit(255);
  }
}

// ===================== JSON Configuration Parser =====================

static char *read_entire_file(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) return NULL;

  if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
  long n = ftell(f);
  if (n < 0) { fclose(f); return NULL; }
  if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return NULL; }

  char *buf = (char*)malloc((size_t)n + 1);
  if (!buf) { fclose(f); return NULL; }

  size_t got = fread(buf, 1, (size_t)n, f);
  buf[got] = '\0';
  fclose(f);
  return buf;
}

static int get_int_or_die(cJSON *obj, const char *key) {
  cJSON *it = cJSON_GetObjectItem(obj, key);
  if (!cJSON_IsNumber(it)) {
    fprintf(stderr, "Config error: expected number for key '%s'\n", key);
    exit(2);
  }
  return it->valueint;
}

static void load_constraints(const char *path) {
  char *text = read_entire_file(path);
  if (!text) {
    fprintf(stderr, "Failed to read config file: %s\n", path);
    exit(2);
  }

  cJSON *root = cJSON_Parse(text);
  free(text);

  if (!root) {
    fprintf(stderr, "Invalid JSON in %s\n", path);
    exit(2);
  }

  cJSON *b = cJSON_GetObjectItem(root, "bounds");
  if (!cJSON_IsObject(b)) {
    fprintf(stderr, "Config error: missing/invalid 'bounds' object\n");
    cJSON_Delete(root);
    exit(2);
  }

  G.xmin = get_int_or_die(b, "xmin");
  G.xmax = get_int_or_die(b, "xmax");
  G.ymin = get_int_or_die(b, "ymin");
  G.ymax = get_int_or_die(b, "ymax");

  cJSON *ms = cJSON_GetObjectItem(root, "max_steps");
  G.max_steps = cJSON_IsNumber(ms) ? ms->valueint : 1000;

  cJSON_Delete(root);
}

// Enviornment Inputs
int cop_x = 0;
int cop_y = 0;

int robber_x = 6;
int robber_y = 6;

// Player Inputs
volatile int new_input_ready = 0;
volatile int player_dx = 0;
volatile int player_dy = 0;

// Trace Output
std::string trace = "";

void* controller_thread(void* arg) {
    step_controller(); // synthesized controller
    return NULL;
}

void movement_generator() {
    // Block while controller reads:
    while (new_input_ready == 1) {
        usleep(1000);
    }

    int dir = rand() % 5;
    
    switch (dir) {
        case 0: { player_dx = 0; player_dy = -1; break;}
        case 1: { player_dx = 0; player_dy = 1; break;}
        case 2: { player_dx = -1; player_dy = 0; break;}
        case 3: { player_dx = 1; player_dy = 0; break;}
        case 4: { player_dx = 0; player_dy = 0; break;}
    }
    
    // Free Lock
    new_input_ready = 1;
}

int main() {
    std::cout << "================================\nCops and Robbers\n================================" << std::endl;

    // Thread Creation 
    pthread_t tid;
    pthread_create(&tid, NULL, controller_thread, NULL);

    // Random cop position
    cop_x = rand() % GRID_SIZE;
    cop_y = rand() % GRID_SIZE;

    // Random robber position
    do {
        robber_x = rand() % GRID_SIZE;
        robber_y = rand() % GRID_SIZE;
    } while (robber_x == cop_x && robber_y == cop_y);

    while (true) {
            // Random Robber Movement
            movement_generator();
            
            // --- Check for capture ---
            bool caught = (cop_x == robber_x && cop_y == robber_y);
            
            // Check for capture
            if (caught) {
                break;
            }
            
            // Add a small delay to prevent busy-waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
}

void read_inputs() {
    // Wait until the game sets new input
    while (!new_input_ready || (robber_x == cop_x && robber_y == cop_y)) {
        usleep(1000);
    }
    
    // Calculate new position
    int new_robber_x = robber_x + player_dx;
    int new_robber_y = robber_y + player_dy;
    
    // Apply boundary constraints before updating position
    if (new_robber_x >= 0 && new_robber_x < GRID_SIZE) {
        robber_x = new_robber_x;
    }
    if (new_robber_y >= 0 && new_robber_y < GRID_SIZE) {
        robber_y = new_robber_y;
    }
    
    // Record Trace
    std::string curr_trace = std::string("{\"RobberX\":") + std::to_string(robber_x) +
        ", \"RobberY\": " + std::to_string(robber_y) +
        ", \"CopX\": " + std::to_string(cop_x) +
        ", \"CopY\": " + std::to_string(cop_y) + "}\n";
    trace += curr_trace;
    
    // Reset flag
    player_dx = 0;
    player_dy = 0;
    new_input_ready = 0;
}
