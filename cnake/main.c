#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <sys/time.h>
#if defined(__unix__) || defined(_APPLE_)
# warning building on Unix or Darwin
# include <termios.h>
# include <unistd.h>
#elif defined(_WIN32)
# warning building on Windows
# include <termiwin.h>
# include <synchapi.h>
# define sleep(m) SleepEX((m) / 1000, 1)
#else
# error unknown operating system
#endif
//#include <assert.h>

/* enum tiles { */
/*   HEAD, */
/*   BODY, */
/*   TILE, */
/*   WALL */
/* }; */

#define TRUE 1
#define FALSE 0
#define FPS 10.0f

typedef struct Node {
  unsigned int x;
  unsigned int y;
  struct Node *prev;
  struct Node *next;
} node;

// Among the application I use the reverse order of parameters then curses
// standard; x first, y second; as it seems more intuitive for games
node *new_node(unsigned int x, unsigned int y) {
  node *new = malloc(sizeof(node));
  new->x = x;
  new->y = y;
  new->prev = NULL;
  new->next = NULL;
  
  return new;
}

node *root = NULL;

// Some top level globals
static const unsigned int delta = (unsigned int)(1.0f / FPS * 1000000.0f);
static unsigned int frames = 0;
struct timeval t_input;
static _Bool done = FALSE;

// User input multiplex (BIG chance this AIN'T gonna work for multiplayer input)
static _Bool multiplayer = FALSE;
static char user_input[2] = {0};
static fd_set input_set;

// TTL
struct termios tinfo;

// Game settings
static _Bool board = TRUE;

// Window
static WINDOW* window = NULL;
static WINDOW* hud = NULL;
static unsigned int w;
static unsigned int h;

// Methods' declarations
static void init_ttl(void);
static void reset_ttl(void);
static void set_game(void);
static void init_win(unsigned int, const unsigned int);
static void reset_win(void);
static void draw_board(_Bool);
static void update_frame(void);
static void init_player(void);
static void draw_player(node*);
static void update_in(void);
static void reset_game(void);

// Main procedure ***********************************************
int main(void) {
  atexit(reset_win);
  atexit(reset_ttl);

  init_ttl();
  set_game();
  
  init_win(20u, 20u);
  init_player();
  
  draw_board(board);
  
  while (!done) {
    mvwprintw(hud, 0, 0, "%itick | %id", frames, delta);
    draw_player(root);
    update_in();
    update_frame();
  }
  
  return EXIT_SUCCESS;
}

// Private declarations *****************************************
static void _draw_node(const node *n);

static void
init_ttl(void) {
  tcgetattr(0, &tinfo);
  tinfo.c_lflag &= ~ICANON;
  tinfo.c_cc[VMIN] = 1;
  tinfo.c_cc[VTIME] = 0;
  tcsetattr(0, TCSANOW, &tinfo);
}

static void
reset_ttl(void) {
  tinfo.c_lflag &= ICANON;
  tcsetattr(0, TCSAFLUSH, &tinfo);
}

static void
set_game(void) {
  char c;
  printf("Choose difficulty level (1..2): ");
  while ((c = getchar()) != '1' && c != '2')
    ;;
  if (c == '2')
    board = FALSE;
  printf("\nChoose number of players (1..2): ");
  while ((c = getchar()) != '1' && c != '2')
    ;;
  if (c == '2')
    multiplayer = TRUE;
  putchar('\n');
  
  fprintf(stderr, "Starting Cnake..\n");
}

static void
init_win(unsigned int width, const unsigned int height) {
  initscr();
  int ttl_height;
  int ttl_width;
  w = width = width * 2;
  h = height;
  getmaxyx(stdscr, ttl_height, ttl_width);
  hud    = newwin(1, width, (ttl_height / 2) - (height / 2) - 1,
		  (ttl_width / 2) - (width / 2));
  window = newwin(height, width, (ttl_height / 2) - (height / 2),
		  (ttl_width / 2) - (width / 2));
}

static void
reset_win(void) {
  endwin();
  reset_game();
  fprintf(stderr, "Cnake finished\n");
}

static void
draw_board(_Bool border) {
  if (border)
    box(window, 0, 0);
  else {
    wmove(window, 0, 0);
    whline(window, ACS_HLINE, w);
    wmove(window, h, 0);
    whline(window, ACS_HLINE, w);
  }
}

static void
update_frame(void) {
  wrefresh(hud);
  wrefresh(window);
  usleep(delta);
  ++frames;
}

static void
init_player(void) {
  // Initialize first two nodes
  root = new_node(w / 2, h / 2);
  node *tail = new_node(root->x, root->y - 1);
  root->prev = tail;
  tail->next = root;
}

static void
draw_player(node *n) {
  if (n != NULL)
    _draw_node(n);
  if (n->prev != NULL)
    draw_player(n->prev);
}

static void
update_in(void) {
  // Set user input file descriptor
  FD_ZERO(&input_set);
  FD_SET(STDIN_FILENO, &input_set);
  user_input[0] = ' ';
  user_input[1] = ' ';
  t_input.tv_usec = delta;
  const int ready = select(1, &input_set, NULL, NULL, &t_input);
  if (ready == -1) {
    mvwprintw(hud, 0, w-5, "error");
  }
  else if (ready == 0) {
    mvwprintw(hud, 0, w-5, "     ");
  }
  else if (ready) {
    // fd 0 is the ttl
    const int bytes = read(0, user_input, (int)multiplayer + 1);
    mvwprintw(hud, 0, w-5, "%i:%c%c", (int)multiplayer + 1, user_input[0], user_input[1]);
    if (bytes == 0) {
      ;;
    } else {
      for (unsigned char i = 0; i < (int)multiplayer + 1; ++i) {
	switch (user_input[i]) {
	case 'q':
	  done = true; break;
	default:
	  break;
	}
      }
    }
  }
}

static void
_draw_node(const node *n) {
  mvwaddch(window, n->y, n->x, '(');
  mvwaddch(window, n->y, n->x + 1, ')');
}

static void
free_nodes(node *n, unsigned int count) {
  if (n != NULL) {
    node *prev = n->prev;
    fprintf(stderr, "Clean node: cleaning %i\n", count);
    free(n);
    if (prev != NULL) {
      fprintf(stderr, "Clean node: moving back\n");
      free_nodes(prev, count + 1);
    }
  }
}

static void
reset_game(void) {
  fprintf(stderr, "Cleanup..\n");
  free_nodes(root, 1);
}
