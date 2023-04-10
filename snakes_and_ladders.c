#include <string.h> // For strlen(), strcmp(), strcpy()
#include "markov_chain.h"

#define MAX(X, Y) (((X) < (Y)) ? (Y) : (X))

#define EMPTY -1
#define BOARD_SIZE 100
#define MAX_GENERATION_LENGTH 60

#define DICE_MAX 6
#define NUM_OF_TRANSITIONS 20

/**
 * represents the transitions by ladders and snakes in the game
 * each tuple (x,y) represents a ladder from x to if x<y or a snake otherwise
 */
const int transitions[][2] = {{13, 4},
                              {85, 17},
                              {95, 67},
                              {97, 58},
                              {66, 89},
                              {87, 31},
                              {57, 83},
                              {91, 25},
                              {28, 50},
                              {35, 11},
                              {8,  30},
                              {41, 62},
                              {81, 43},
                              {69, 32},
                              {20, 39},
                              {33, 70},
                              {79, 99},
                              {23, 76},
                              {15, 47},
                              {61, 14}};

/**
 * struct represents a Cell in the game board
 */
typedef struct Cell {
    int number; // Cell number 1-100
    int ladder_to;  // ladder_to represents the jump of the ladder
    // in case there is one from this square
    int snake_to;  // snake_to represents the jump of the snake
    // in case there is one from this square
    //both ladder_to and snake_to should be -1 if the Cell doesn't have them
} Cell;

/** Error handler **/
static int handle_error(char *error_msg, MarkovChain **database)
{
  printf("%s", error_msg);
  if (database != NULL)
  {
    free_markov_chain(database);
  }
  return EXIT_FAILURE;
}


static int create_board(Cell *cells[BOARD_SIZE])
{
  for (int i = 0; i < BOARD_SIZE; i++)
  {
    cells[i] = malloc(sizeof(Cell));
    if (cells[i] == NULL)
    {
      for (int j = 0; j < i; j++) {
        free(cells[j]);
      }
      handle_error(ALLOCATION_ERROR_MASSAGE,NULL);
      return EXIT_FAILURE;
    }
    *(cells[i]) = (Cell) {i + 1, EMPTY, EMPTY};
  }

  for (int i = 0; i < NUM_OF_TRANSITIONS; i++)
  {
    int from = transitions[i][0];
    int to = transitions[i][1];
    if (from < to)
    {
      cells[from - 1]->ladder_to = to;
    }
    else
    {
      cells[from - 1]->snake_to = to;
    }
  }
  return EXIT_SUCCESS;
}

/**
 * fills database
 * @param markov_chain
 * @return EXIT_SUCCESS or EXIT_FAILURE
 */
static int fill_database(MarkovChain *markov_chain)
{
  Cell* cells[BOARD_SIZE];
  if(create_board(cells) == EXIT_FAILURE)
  {
    return EXIT_FAILURE;
  }
  MarkovNode *from_node = NULL, *to_node = NULL;
  size_t index_to;
  for (size_t i = 0; i < BOARD_SIZE; i++)
  {
    add_to_database(markov_chain,cells[i]);
  }

  for (size_t i = 0; i < BOARD_SIZE; i++)
  {
    from_node = get_node_from_database(markov_chain,cells[i])->data;

    if (cells[i]->snake_to != EMPTY || cells[i]->ladder_to != EMPTY)
    {
      index_to = MAX(cells[i]->snake_to,cells[i]->ladder_to) - 1;
      to_node = get_node_from_database(markov_chain, cells[index_to])
          ->data;
      add_node_to_counter_list(from_node, to_node, markov_chain);
    }
    else
    {
      for (int j = 1; j <= DICE_MAX; j++)
      {
        index_to = ((Cell*) (from_node->data))->number + j - 1;
        if (index_to >= BOARD_SIZE)
        {
          break;
        }
        to_node = get_node_from_database(markov_chain, cells[index_to])
            ->data;
        add_node_to_counter_list(from_node, to_node, markov_chain);
      }
    }
  }
  // free temp arr
  for (size_t i = 0; i < BOARD_SIZE; i++)
  {
    free(cells[i]);
  }
  return EXIT_SUCCESS;
}

static void print_func_cell(void *data)
{
  Cell *p = (Cell *) data;
  if ((p->ladder_to == EMPTY) && (p->snake_to == EMPTY))
  {
    fprintf (stdout,"[%d] -> ", p->number);
  }
  else if (p->ladder_to == EMPTY)
  {
    fprintf (stdout,"[%d]-snake to %d -> ", p->number,
             p->snake_to);
  }
  else
  {
    fprintf (stdout, "[%d]-ladder to %d -> ", p->number,
             p->ladder_to);
  }
}

static int comp_func_cell (void *data_one, void *data_two)
{
  return ((Cell *) data_one)->number - ((Cell *) data_two)->number;
}

static void free_data_cell (void *data)
{
  Cell *p = (Cell *) data;
  free (p);
  p = NULL;
}

static void *copy_func_cell (void *data)
{
  Cell *s = (Cell *) data;
  Cell *p = malloc (sizeof (Cell));
  if (!p) return NULL;
  p->number = s->number;
  p->ladder_to = s->ladder_to;
  p->snake_to = s->snake_to;
  return p;
}

static bool is_last_cell (void *data)
{
  Cell *p = (Cell *) data;
  if (p->number == 100) return true;
  return false;
}

static MarkovChain *initialize_markov_chain(
    Print_Func print_func,
    Comp_Func comp_func,
    Free_Data free_data,
    Copy_Func copy_func,
    Is_Last is_last
)
{
  MarkovChain *markov_chain = malloc (sizeof (MarkovChain));
  if (!markov_chain) return NULL;

  LinkedList *list = malloc (sizeof (LinkedList));
  if (!list)
  {
    free (markov_chain);
    markov_chain = NULL;
    return NULL;
  }

  list->first = NULL;
  list->last = NULL;
  list->size = 0;
  markov_chain->database = list;
  markov_chain->print_func = print_func;
  markov_chain->comp_func = comp_func;
  markov_chain->free_data = free_data;
  markov_chain->copy_func = copy_func;
  markov_chain->is_last = is_last;
  return markov_chain;
}

void run_random_walk(MarkovChain *markov_chain)
{
  MarkovNode *p = markov_chain->database->first->data;
  for (int i = 0; i < MAX_GENERATION_LENGTH; ++i)
  {
    if (((Cell*)p->data)->number == BOARD_SIZE)
    {
      fprintf (stdout, "[%d]", ((Cell*)p->data)->number);
      break;
    }
    markov_chain->print_func(p->data);
    p = get_next_random_node(p);
  }
}

/**
 * @param argc num of arguments
 * @param argv 1) Seed
 *             2) Number of sentences to generate
 * @return EXIT_SUCCESS or EXIT_FAILURE
 */
int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    fprintf (stdout, "Usage:Something went wrong.\n"
                     "The parameters that needed:\n"
                     "1)Seed value.\n"
                     "2)Number of sentences to generate.\n");
    return EXIT_FAILURE;
  }

  srand ((unsigned int)strtol(argv[1], NULL, 10));

  MarkovChain *markov_chain = initialize_markov_chain (
      print_func_cell,
      comp_func_cell,
      free_data_cell,
      copy_func_cell,
      is_last_cell);
  if (!markov_chain)
  {
    fprintf (stdout, ALLOCATION_ERROR_MASSAGE);
    return EXIT_FAILURE;
  }

  if (fill_database (markov_chain))
  {
    fprintf (stdout, ALLOCATION_ERROR_MASSAGE);
    return EXIT_FAILURE;
  }

  for (int i = 1; i <= (int) strtol (argv[2], NULL, 10); ++i)
  {
    fprintf (stdout, "Random Walk %d: ", i);
    run_random_walk(markov_chain);
    fprintf (stdout, "\n");
  }

  free_markov_chain (&markov_chain);
  return EXIT_SUCCESS;
}