#include "markov_chain.h"
#include <string.h>

/**
* Get random number between 0 and max_number [0, max_number).
* @param max_number maximal number to return (not including)
* @return Random number
*/
int get_random_number(int max_number)
{
  return rand() % max_number;
}

/**
 * Helper function for the main "get_first_random_node".
 * Helps to search the needed MarkovNode.
 * Validate that it is not an end word.
 * @param markov_chain the chain we work with.
 * @param r_number random number that indicate the place of the MarkovNode.
 * @return MarkovNode.
 */
MarkovNode *get_first_random_node_helper(MarkovChain *markov_chain,
                                         int r_number){
  Node *p = markov_chain->database->first;
  for (int i = 0 ; i < r_number; ++i){
    p = p->next;
  }
  return p->data;
}

MarkovNode *get_first_random_node (MarkovChain *markov_chain)
{
  MarkovNode *p = get_first_random_node_helper (markov_chain,
          get_random_number (markov_chain->database->size));
  return p;
}

MarkovNode *get_next_random_node (MarkovNode *state_struct_ptr)
{
  int r_size = get_random_number
      ((int)state_struct_ptr->counter_list_sum);
  NextNodeCounter *p = state_struct_ptr->counter_list;
  int sum = 0, i = 0;
  while (i < (int)state_struct_ptr->counter_list_size)
  {
    sum += p->frequency;
    if (sum > r_size)
    {
      break;
    }
    i++;
    p++;
  }
  return p->next_word;
}

void generate_random_sequence(MarkovChain *markov_chain, MarkovNode *
first_node, int max_length)
{
  int i = 0;
  MarkovNode *p = first_node;
  if (first_node == NULL)
    first_node = get_first_random_node (markov_chain);
  else
    p = first_node;

  markov_chain->print_func(first_node->data);
  while ((p->counter_list != NULL) && (i < max_length))
  {
    p = get_next_random_node (p);
    markov_chain->print_func(p->data);
    i++;
  }
}

void free_markov_chain(MarkovChain ** ptr_chain)
{
  if (ptr_chain)
  {
    if (*ptr_chain)
    {
      if ((*ptr_chain)->database)
      {
        Node *p = (*ptr_chain)->database->first;
        Node *tmp = NULL;
        for (int i = 0; i < (*ptr_chain)->database->size; ++i)
        {
          if (p->data)
          {
            if (p->data->data)
            {
              (*ptr_chain)->free_data (p->data->data);
            }
            if (p->data->counter_list)
            {
              free (p->data->counter_list);
              p->data->counter_list = NULL;
            }
            free (p->data);
            p->data = NULL;
          }
          tmp = p->next;
          free (p);
          p = tmp;
        }
        p = NULL;
        free ((*ptr_chain)->database);
        (*ptr_chain)->database = NULL;
      }
      free (*ptr_chain);
      *ptr_chain = NULL;
    }
  }
}

/**
 * Check if the MarkovNode who needed to add to the counter list is already
 * exists in the list.If so, the function return pointer from type
 * NextNodeCounter of the MarkovNode.
 * Otherwise its return NULL.
 * @param first_node The MarkovNode we use is counter list.
 * @param second_node The MarkovNode we check if he is already exists in list.
 * @return Pointer type NextNodeCounter \ NULL.
 */
NextNodeCounter *node_in_counter_list(MarkovChain *markov_chain, MarkovNode
*first_node, MarkovNode *second_node)
{
  NextNodeCounter *p = first_node->counter_list;
  for (size_t i = 0; i < first_node->counter_list_size; ++i)
  {
    if (!markov_chain->comp_func (p->next_word->data, second_node->data))
    {
      return p;
    }
    p++;
  }
  return NULL;
}

bool add_node_to_counter_list (MarkovNode *first_node, MarkovNode
*second_node, MarkovChain *markov_chain)
{
  NextNodeCounter *p = node_in_counter_list(markov_chain, first_node,
                                            second_node);
  if (p)
  {
    p->frequency++;
    first_node->counter_list_sum++;
    return true;
  }

  NextNodeCounter *tmp = realloc (first_node->counter_list,
                                  sizeof (NextNodeCounter) *
                                  (first_node->counter_list_size + 1));
  if (!tmp)
  {
    fprintf (stderr, ALLOCATION_ERROR_MASSAGE);
    return false;
  }

  first_node->counter_list = tmp;
  first_node->counter_list[first_node->counter_list_size].next_word
      = second_node;
  first_node->counter_list[first_node->counter_list_size].frequency = 1;
  first_node->counter_list_size++;
  first_node->counter_list_sum++;

  return true;
}

Node* get_node_from_database(MarkovChain *markov_chain, void *data_ptr)
{
  void *data = (void *) markov_chain->copy_func (data_ptr);
  if (!data)
  {
    fprintf (stderr, ALLOCATION_ERROR_MASSAGE);
    return NULL;
  }

  Node *p = markov_chain->database->first;
  Node *result = NULL;
  for (int i = 0; i < markov_chain->database->size; ++i)
  {
    if (!markov_chain->comp_func (p->data->data, data))
    {
      result = p;
      break;
    }
    if (p->next != NULL) p = p->next;
  }
  free (data);
  data = NULL;
  return result;
}

/**
 * Create new node from type Node and initialize it.
 * Create the node by using malloc.
 * Create new markovnode from type MarkovNode using malloc.
 * Create new data from type char using malloc.
 * @param data the data of the markovnode inside the node.
 * @return Pointer to the new node.
 */
Node *create_new_node(MarkovChain *markov_chain, void *data)
{
  Node *new_node = malloc (sizeof(Node));
  if (!new_node) return NULL;

  MarkovNode *new_markov_node = malloc (sizeof(MarkovNode));
  if (!new_markov_node)
  {
    free (new_node);
    new_node = NULL;
    return NULL;
  }

  void *new_data = markov_chain->copy_func (data);
  if (!new_data)
  {
    free (new_node);
    new_node = NULL;
    free (new_markov_node);
    new_markov_node = NULL;
    return NULL;
  }

  new_markov_node->data = new_data;
  new_markov_node->counter_list = NULL;
  new_markov_node->counter_list_size = 0;
  new_markov_node->counter_list_sum = 0;
  new_node->data = new_markov_node;
  new_node->next = NULL;
  return new_node;
}

Node* add_to_database(MarkovChain *markov_chain, void *data_ptr)
{
  void *data = (void *) markov_chain->copy_func (data_ptr);
  if (!data)
  {
    fprintf (stderr, ALLOCATION_ERROR_MASSAGE);
    return NULL;
  }

  Node *p = markov_chain->database->first;
  for (int i = 0; i < markov_chain->database->size; ++i)
  {
    if (!markov_chain->comp_func (p->data->data, data))
    {
      free (data);
      data = NULL;
      return p;
    }
    if (p->next != NULL) p = p->next;
  }

  Node *new_node = create_new_node (markov_chain, data);
  free (data);
  data = NULL;
  if (!new_node) return NULL;
  if (markov_chain->database->size == 0)
  {
    markov_chain->database->first = new_node;
  }
  else
  {
    p->next = new_node;
  }
  markov_chain->database->size++;
  return new_node;
}