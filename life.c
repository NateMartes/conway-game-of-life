/*
 * Author: Nathaniel Martes
 * File:   life.c
 * Description:
 *   Conway's Game of Life with some neat options in the terminal
 */

#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>

#define MAX_N_LEN 30
#define MAX_M_LEN 30
#define COLOR_WHITE_ID 1
#define COLOR_CLEAR_ID 2
#define ALIVE_CELL_CHAR '#'
#define DELAY_TIME_NANO 100000

#define END_CODE 'q'
#define PAUSE_CODE 'p'
#define RESUME_CODE 'r'
#define START_CODE 's'
#define SPEED_CODE '+'
#define SLOW_CODE '-'

#define USAGE "life [-h|--help] [-c|--cylinder] [-t|--torus] [-m|--mobius-strip] [-k|--kline-bottle] [-p|--projective-plane] init_file"
#define HELP "\nRun Conway's Game of Life in the terminal!\n" \
             "Takes in an inital state file to describe the game, then executes.\n" \
             "First line in the file is number of rows and columns (ex: 3 3)\n" \
             "The rest of the file is the state of the game, 'x' representing alive cells, others consider to be dead.\n" \
             "\noptions:\n\n" \
             " help: [-h | --help]: Print this help message (thats quite helpful you might say)\n\n" \
             " Run as a cylinder:         [-c | --cylinder]:         Run the game in a connected cylinder\n" \
             " Run as a torus:            [-t | --torus]:            Run the game in a torus\n" \
             " Run as a mobius strip:     [-m | --mobius-strip]:     Run the game in a mobius strip\n" \
             " Run as a kline bottle:     [-k | --kline-bottle]:     Run the game in a kline bottle\n" \
             " Run as a projective plane: [-p | --projective-plane]: Run the game in a projective plane\n" \


#define OPTIONS_SHORT "hctmkp"
static struct option OPTIONS_LONG[] = {
    {"help",              no_argument,       0,  'h' },
    {"cylinder",          no_argument,       0,  'c' },
    {"torus",             no_argument,       0,  't' },
    {"mobius-strip",      no_argument,       0,  'm' },
    {"kline-bottle",      no_argument,       0,  'k' },
    {"projective-plane",  no_argument,       0,  'p' },

    // end with zero for the 'null character'
    {0,0,0,0}
};

typedef struct game_modifiers {
  int is_cylinder;
  int is_torus;
  int is_mobius_strip;
  int is_kline_bottle;
  int is_projective_plane;
} game_modifiers;


// Globals
int** init_map;
int** scrn_map;
int** scrn_map_next;
int n = 0;
int m = 0;
int scrn_w = 0;
int scrn_h = 0;

/*
 * The end_terminal_scrn function clears the screen. Used for teardown
 */
void end_terminal_scrn() {
  color_set(COLOR_WHITE_ID, NULL);
  endwin();
}

/*
 * The fatal_error function takes a messages and sends it to STDERR, exiting aswell.
 */
void fatal_error(char* message) {
  if (stdscr != NULL) {
    end_terminal_scrn();
  }
  fprintf(stderr, "life: %s\n", message);
  exit(EXIT_FAILURE);

}

/*
 * The fatal_error_errno function takes a messages and sends it to STDERR
 * with the errno message aswell, exiting aswell.
 */
void fatal_error_errno(char* message) {
  if (stdscr != NULL) {
    end_terminal_scrn();
  }
  fprintf(stderr, "life: %s: %s\n", message, strerror(errno));
  exit(EXIT_FAILURE);
}

/*
 * The get_arguments function gets the arguments for the life.c program.
 * Args:
 *  argc: The number of args (from main)
 *  argv: The vector of args (from main)
 *  is_cylinder: A refernce to where to store the value if the user wants a cylinder
 *  is_torus: A refernce to where to store the value if the user wants a torus
 *  is_mobisu_strip: A refernce to where to store the value if the user wants a mobius strip
 *  is_kline_bottle: A refernce to where to store the value if the user wants a kline bottle
 *  is_projective_plane: A refernce to where to store the value if the user wants a is_projective_plane
 */
void get_arguments(int argc, char* argv[], int* is_cylinder, int* is_torus, int* is_mobius_strip, int* is_kline_bottle, int* is_projective_plane) {

  if (argc < 2) {
    printf("usage: %s\n", USAGE);
    exit(EXIT_FAILURE);
  }

  int index = 0;
  int c;
  while ((c = getopt_long(argc, argv, OPTIONS_SHORT, OPTIONS_LONG, &index)) != -1) {
    switch (c) {
      case 'h':
        printf("%s\nusage: %s\n", HELP, USAGE);
        exit(EXIT_SUCCESS);
        break;
      case 'c':
        *is_cylinder = 1;
        break;
      case 't':
        *is_torus = 1;
        break;
      case 'm':
        *is_mobius_strip = 1;
        break;
      case 'k':
        *is_kline_bottle = 1;
        break;
      case 'p':
        *is_projective_plane = 1;
        break;
      default:
        printf("usage: %s\n", USAGE);
        exit(EXIT_FAILURE);
        break;
    }
  }
}

/*
 * The get_m_val function gets the amount of rows in the map.
 * This function should be called first in parsing the map init file.
 *
 * Args:
 *  fd: A file descriptor to the init file
 * 
 * Returns:
 *  int: The amount of rows
 */
int get_m_val(int fd) {
  char c;
  char m_tmp[MAX_M_LEN + 1];
  int i = 0;

  while (read(fd, &c, sizeof(char)) != 0) {
    if (c == ' ' || i == MAX_M_LEN - 1) {
      m_tmp[i] = '\0';
      break;
    }
    
    m_tmp[i] = c;
    i++;
  }

  return atoi(m_tmp);
}

/*
 * The get_n_val function gets the amount of columns in the map.
 * This function should be called second in parsing the map init file.
 *
 * Args:
 *  fd: A file descriptor to the init file
 * 
 * Returns:
 *  int: The amount of columns 
 */
int get_n_val(int fd) {
  char c;
  char n_tmp[MAX_M_LEN + 1];
  int i = 0;

  while (read(fd, &c, sizeof(char)) != 0) {
    if (c == '\n' || i == MAX_N_LEN - 1) {
      n_tmp[i] = '\0';
      break;
    }
    
    n_tmp[i] = c;
    i++;
  }

  return atoi(n_tmp);
}

/*
 * The build_map_empty function creates a game map with all indexes being 0.
 *
 * Args:
 *  m: The number of rows in the new map
 *  n: The number of columns in the new map
 *
 * Returns:
 *  int**: The new allocated map
 */
int** build_map_empty(int m, int n) {
  int** map = malloc(sizeof(int*) * m);
  for (int i = 0; i < m; i++) {
    map[i] = malloc(sizeof(int) * n);
    memset(map[i], 0, sizeof(int) * n); 
  }
  return map;
}

/*
 * The build_map_from_file function parses the init file and inserts the init map into the map matrix.
 * This function should be called third in parsing the map init file.
 *
 * Args:
 *  fd: A file descriptor to the init file
 *
 * Returns:
 *  int**: The new allocated map
 * 
 */
int** build_map_from_file(int fd, int m, int n) {

  int** map = malloc(sizeof(int*) * m);
  size_t row_size = sizeof(char) * n;

  char c;
  for (int i = 0; i < m; i++) {

    map[i] = malloc(sizeof(int) * n);
    char* curr_row = malloc(sizeof(char) * n);

    // get row
    if (read(fd, curr_row, row_size) != row_size) {
      char error_message[150];
      sprintf(error_message, "failed to parse row %d: expected more rows", i + 1);
      fatal_error(error_message);
    }

    // convert row to map values
    for (int j = 0; j < n; j++) {
      if (curr_row[j] == 'x') map[i][j] = 1;
      else map[i][j] = 0;
    }

    free(curr_row);

    // get newline character
    int res = read(fd, &c, sizeof(char));
    if (res != 0 && c != '\n') {
      char error_message[150];
      sprintf(error_message, "expected newline character for another row, got %c", c);
      fatal_error(error_message);
    }
  }

  return map;
  
}

/*
 * The build_map_from_stdscrn function uses the stdscrn and creates an integer map from it.
 * The stdscrn should be initalized prior to calling this function.
 *
 * Args:
 *  m: The amount of rows in the screen
 *  n: The amount of columns in the screen
 *
 * Returns:
 *  int**: The new allocated map
 * 
 */
int** build_map_from_stdscrn(int m, int n) {

  int** map = malloc(sizeof(int*) * m);
  int row_size = n;

  char c;
  for (int i = 0; i < m; i++) {

    map[i] = malloc(sizeof(int) * n);
    char* curr_row = malloc(sizeof(char) * n);

    // get row
    int res = mvinnstr(i, 0, curr_row, row_size);
    if (res == ERR) {
      char error_message[150];
      sprintf(error_message, "failed to parse row %d: expected %d rows on screen", i + 1, n);
      fatal_error(error_message);
    }

    if (res != row_size) {
      char error_message[150];
      sprintf(error_message, "failed to parse row %d: expected screen width of %d", i + 1, row_size);
      fatal_error(error_message);
    }

    // convert row to map values
    for (int j = 0; j < n; j++) {
      if (curr_row[j] == ALIVE_CELL_CHAR) map[i][j] = 1;
      else map[i][j] = 0;
    }

    free(curr_row);
  }

  return map;
  
}

/*
 * The free_map function frees a dynamically allocated map string array.
 */
void free_map(int** map) {
  for (int i = 0; i < n; i++) {
    free(map[i]);
  }
  free(map);
}

/*
 * The init_game_map function parses the init file for setting up Conway's Game of Life. 
 *
 * Args:
 *  fd: A file descriptor to the init file
 */
void init_game_map(int fd) {
  m = get_m_val(fd);
  n = get_n_val(fd);
  init_map = build_map_from_file(fd, m, n);
}

/*
 * The draw_alive_cell function places an alive cell onto the stdscrn.
 * Args:
 *  y: The y location on the screen
 *  x: The x location on the screen
 */
void draw_alive_cell(int y, int x) {
  mvaddch(y, x, ALIVE_CELL_CHAR | COLOR_PAIR(COLOR_WHITE_ID));
}

/*
 * The draw_dead_cell function places an dead cell onto the stdscrn.
 * Args:
 *  y: The y location on the screen
 *  x: The x location on the screen
 */
void draw_dead_cell(int y, int x) {
  mvaddch(y, x, ' ');
}

/*
 * The draw_map_full function takes a integer map and draws it onto the stdscrn.
 * Args:
 *  map: Integer map to draw
 *  m: The amount of rows in the map
 *  n: The amount of columns in the map
 */
void draw_map_full(int** map, int m, int n) {
  for (int i = 0; i < m; i++) {
    for (int j = 0; j < n; j++) {
      if (map[i][j] == 1 || map[i][j] == 2) draw_alive_cell(i, j);
      else draw_dead_cell(i, j);
    }
  }
}

/*
 * The draw_map_full_offset function takes a integer map and draws it onto the stdscrn with an offset.
 * Args:
 *  map: Integer map to draw
 *  m: The amount of rows in the map
 *  n: The amount of columns in the map
 *  m_offset: The offset of the rows to start with
 *  n_offset: The offset of the columns to start with
 */
void draw_map_full_offset(int** map, int m, int n, int m_offset, int n_offset) {
  for (int i = 0; i < m; i++) {
    for (int j = 0; j < n; j++) {
      if (map[i][j] == 1 || map[i][j] == 2) draw_alive_cell(i + m_offset, j + n_offset);
      else draw_dead_cell(i + m_offset, j + n_offset);
    }
  }
}

/*
 * The init_terminal_scrn function sets up the terminal screen with the
 * inital map from the init file
 */
void init_terminal_scrn() {

  initscr();
  curs_set(0);
  nodelay(stdscr, TRUE);
  getmaxyx(stdscr, scrn_h, scrn_w);
  start_color();
  init_pair(COLOR_WHITE_ID, COLOR_WHITE, 255);
  init_pair(COLOR_CLEAR_ID, COLOR_BLACK, -1);

  // use offset to place local map into scrn map
  draw_map_full_offset(init_map, m, n, (scrn_h / 2) - m, (scrn_w / 2) - n);

  scrn_map = build_map_from_stdscrn(scrn_h, scrn_w);
  scrn_map_next = build_map_empty(scrn_h, scrn_w);

  refresh();
}

/*
 * The get_cell_value_from_map function takes a map and gets the value
 * at a given cell, handling edge cases.
 *
 * Args:
 *  map: The map to get the value from
 *  cell_y: The mth index of the cell to update
 *  cell_x: The nth index of the cell to update
 *  m: The number of rows in map
 *  n: The numbers of columns in map
 *  g: Any modifiers to the game
 *
 * Returns:
 *  int: The value of the cell
 */
int get_cell_value_from_map(int** map, int cell_y, int cell_x, int m, int n, game_modifiers* g) {
  
  if (cell_x < 0 || cell_x >= n) {

    if (g->is_cylinder || g->is_torus || g->is_mobius_strip || g->is_projective_plane) {

      if (cell_x < 0) cell_x = n - 1;
      else cell_x = cell_x % n;

      if (g->is_mobius_strip || g->is_projective_plane) {
        cell_y = m - 1 - cell_y;
      }

    } else {
      return 0;
    }
  }

  if (cell_y < 0 || cell_y >= m) {

    if (g->is_torus || g->is_kline_bottle || g->is_projective_plane) {

      if (cell_y < 0) cell_y = m - 1;
      else cell_y = cell_y % m;

      if (g->is_kline_bottle || g->is_projective_plane) {
        cell_x = n - 1 - cell_x;
      }
    } else {
      return 0;
    }
  }

  return map[cell_y][cell_x];
}

/*
 * The apply_conway_rules_cell function takes a cell from a game map
 * and applys the rules of Conway's game of life to it.
 *
 * Args:
 *  map: The map to use as the inital state
 *  dest: The map to place the results into (assumed to be the same size as 'map')
 *  cell_y: The mth index of the cell to update
 *  cell_x: The nth index of the cell to update
 *  m: The number of rows in map
 *  n: The numbers of columns in map
 *  g: Any modifiers to the game
 */
void apply_conway_rules_cell(int** map, int** dest, int cell_y, int cell_x, int m, int n, game_modifiers* g) {
  int alive_neighbor_count = 0;
  for (int i = cell_y - 1; i <= cell_y + 1; i++) {
    for (int j = cell_x - 1; j <= cell_x + 1; j++) {
      if (i == cell_y && j == cell_x) continue;
      if (get_cell_value_from_map(map, i, j, m, n, g) == 1) {
        alive_neighbor_count++;
      }
    }
  }

  int cell_val = map[cell_y][cell_x];
  if (cell_val == 1) {
    if (alive_neighbor_count < 2 || alive_neighbor_count > 3) {
      dest[cell_y][cell_x] = 0;
    } else {
      dest[cell_y][cell_x] = 1;
    }
  } else {
    if (alive_neighbor_count == 3) {
      dest[cell_y][cell_x] = 1;
    } else {
      dest[cell_y][cell_x] = 0;
    }
  }
}

/*
 * The apply_conway_rules function takes a game map and applys the rules
 * of Conway's game of life to it.
 *
 * Args:
 *  map: The map to use as the inital state
 *  dest: The map to place the results into (assumed to be the same size as 'map')
 *  m: The number of rows in map
 *  n: The numbers of columns in map
 *  g: Any modifiers to the game
 */
void apply_conway_rules(int** map, int** dest, int m, int n, game_modifiers* g) {
  for (int i = 0; i < m; i++) {
    for (int j = 0; j < n; j++) {
      apply_conway_rules_cell(map, dest, i, j, m, n, g);
    }
  }
}

game_modifiers* create_game_modifiers(int is_cylinder, int is_torus, int is_mobius_strip, int is_kline_bottle, int is_projective_plane) {
  
  game_modifiers* g = malloc(sizeof(game_modifiers));

  g->is_cylinder = is_cylinder;
  g->is_torus = is_torus;
  g->is_mobius_strip = is_mobius_strip;
  g->is_kline_bottle = is_kline_bottle;
  g->is_projective_plane = is_projective_plane;

  return g;
}

int main(int argc, char* argv[]) {

  int is_cylinder = 0, is_torus = 0, is_mobius_strip = 0, is_kline_bottle = 0, is_projective_plane = 0;
  get_arguments(argc, argv, &is_cylinder, &is_torus, &is_mobius_strip, &is_kline_bottle, &is_projective_plane);

  char* init_file = argv[argc - 1];
  int fd = open(init_file, O_RDONLY);
  if (fd == -1) fatal_error_errno("failed to open init file");
  init_game_map(fd);
  close(fd);

  init_terminal_scrn();
  free(init_map);

  // setup game modifier struct
  game_modifiers* g = create_game_modifiers(
    is_cylinder,
    is_torus,
    is_mobius_strip,
    is_kline_bottle,
    is_projective_plane
  );

  // run main loop
  char c;
  int flip = 1;
  int started = 0;
  int paused = 1;
  int** curr_map = scrn_map;
  int** next_map = scrn_map_next;
  int sleep_time = DELAY_TIME_NANO;
  for (;;) {

    c = getch();
    if (c == END_CODE) break;
    switch (c) {
      case START_CODE:
        if (started) break;
        started = 1;
      case RESUME_CODE:
        if (!started) break;
        paused = 0;
        break;
      case PAUSE_CODE:
        if (!started) break;
        paused = 1;
        break;
      case SPEED_CODE:
        if (!started) break;
        if (sleep_time >= 50000) sleep_time -= 10000;
        break;
      case SLOW_CODE:
        if (!started) break;
        if (sleep_time <= 100000) sleep_time += 100000;
        break;
      default:
        break;
    }

    refresh();
    usleep(sleep_time);

    if (!paused) {
      apply_conway_rules(curr_map, next_map, scrn_h, scrn_w, g);
      draw_map_full(next_map, scrn_h, scrn_w);

      int** tmp = curr_map;
      curr_map = next_map;
      next_map = tmp;

    }
  }

  // Cleanup
  free_map(scrn_map);
  free_map(scrn_map_next);
 
  end_terminal_scrn();

}
