#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "markov_chain.h"
#include "linked_list.h"
#define MAX_LINE 1000
#define INPUT_1 5
#define INPUT_2 4
#define MAX_WORDS_IN_TWEET 20

static void print_func_char(void *data)
{
  char *s = (char *) data;
  fprintf (stdout, "%s\n", s);
}

static int comp_func_char (void *data_one, void *data_two)
{
  return strcmp ((char *) data_one, (char *) data_two);
}

static void free_data_char (void *data)
{
  char *s = (char *) data;
  free (s);
  s = NULL;
}

static size_t get_size (char *data_ptr)
{

  size_t size = 0;
  while (strcmp (data_ptr, "\0") != 0)
  {
    data_ptr++;
    size++;
  }
  size++;
  return size;
}

static void *copy_func_char (void *data)
{
  char *s = (char *) data;
  char *p = malloc (sizeof (char) * get_size(s));
  if (!p) return NULL;
  strcpy (p, s);
  return p;
}

static bool is_last_char (void *data)
{
  char *s = (char *) data;
  if (!strcmp (s + strlen (s) - 1, "."))
    return true;
  return false;
}

/**
 * Create and allocate new memory for markov chain and linked list.
 * Also initialize them.
 * @return Pointer from type MarkovChain.
 */
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

/**
 * Check if the word is already exist in the markov chain.
 * @param markov_chain the chain we search on.
 * @param word the word we search.
 * @return True \ False value.
 */
static bool word_in_chain(MarkovChain *markov_chain, char *word)
{
  Node *p = markov_chain->database->first;
  for (int i = 0; i < markov_chain->database->size; ++i)
  {
    if (!strcmp (word, (char *) p->data->data)) return true;

    p = p->next;
  }
  return false;
}

/**
 * Remove spaces from the word and clean it from ' ' or '\n'.
 * @param word the word we want to remove spaces from her.
 * @return Pointer to the word after the removing.
 */
static char *remove_spaces(char *word)
{
  char *end;
  while(isspace((unsigned char)*word))
  {
    word++;
  }

  if(*word == 0)
  {
    return word;
  }

  end = word + strlen(word) - 1;
  while(end > word && isspace((unsigned char)*end))
  {
    end--;
  }

  end[1] = '\0';
  return word;
}


static int fill_database(FILE *fp, int words_to_read,
                         MarkovChain *markov_chain)
{
  Node *p;
  Node *current;
  Node *prev;
  char line[MAX_LINE];
  char *word;

  while (fgets (line, MAX_LINE, fp))
  {
    word = strtok (line, " ");
    while (word)
    {
      word = remove_spaces (word);
      if (markov_chain->database->size == 0)
      {
        add_to_database (markov_chain,
                         (void *) word);
        p = markov_chain->database->first;
      }
      else if (word_in_chain(markov_chain, word))
      {
        current = get_node_from_database (markov_chain, (void *) word);
        if (p && !markov_chain->is_last ((void *) (prev->data->data)))
        {
          add_node_to_counter_list (prev->data,
                                    current->data, markov_chain);
        }
        prev = current;
        word = strtok (NULL, " ");
        continue;
      }
      else
      {
        add_to_database (markov_chain, (void *) word);
        p = p->next;
        if (p && !markov_chain->is_last ((void *) (prev->data->data)))
        {
          add_node_to_counter_list (prev->data, p->data,
                                    markov_chain);
        }
      }

      if (!p)
      {
        return 0;
      }
      prev = p;
      if (markov_chain->database->size == words_to_read)
      {
        return 1;
      }
      word = strtok (NULL, " ");
    }
  }
  return 1;
}

/**
 * Write tweets function.
 * @param markov_chain the data struct we work on.
 * @param num_of_tweets the number of tweets we want to write.
 */
static void write_tweets(MarkovChain *markov_chain, long num_of_tweets)
{
  MarkovNode *word;
  int num_of_words_in_tweet;
  for (int i = 1; i <= num_of_tweets; ++i)
  {
    do
    {
      word = get_first_random_node (markov_chain);
    }
    while (markov_chain->is_last ((void *) (word->data)));
    fprintf (stdout, "Tweet %d: %s", i, (char *) word->data);
    num_of_words_in_tweet = 1;
    while (!markov_chain->is_last ((void *) word->data) &&
           num_of_words_in_tweet < MAX_WORDS_IN_TWEET)
    {
      word = get_next_random_node ((void *) word);
      fprintf (stdout, " %s", (char *) word->data);
      num_of_words_in_tweet++;
    }

    fprintf (stdout, "\n");
  }
}

int main(int argc, char *argv[]){
  if ((argc != INPUT_1) && (argc != INPUT_2))
  {
    fprintf (stdout, "Usage:Something went wrong.\n"
                     "The parameters that needed:\n"
                     "1)Seed value.\n"
                     "2)Num of tweets.\n"
                     "3)Path file.\n"
                     "4)Number of words to read from the path file.");
    return EXIT_FAILURE;
  }

  FILE *fp = fopen (argv[3], "r");
  if (!fp)
  {
    fprintf (stdout, "Error:The path is not working.");
    return EXIT_FAILURE;
  }

  srand ((unsigned int)strtol(argv[1], NULL, 10));
  MarkovChain *markov_chain = initialize_markov_chain (
      print_func_char,
      comp_func_char,
      free_data_char,
      copy_func_char,
      is_last_char);
  if (!markov_chain)
  {
    fprintf (stdout, ALLOCATION_ERROR_MASSAGE);
    return EXIT_FAILURE;
  }
  int words_to_read = - 1;
  if (argv[4]) words_to_read = strtol (argv[4], NULL, 10);
  if (!fill_database (fp, words_to_read, markov_chain))
  {
    fprintf (stdout, ALLOCATION_ERROR_MASSAGE);
    free_markov_chain (&markov_chain);
    return EXIT_FAILURE;
  }

  write_tweets (markov_chain, strtol(argv[2], NULL, 10));

  fclose (fp);
  free_markov_chain (&markov_chain);
  return EXIT_SUCCESS;
}