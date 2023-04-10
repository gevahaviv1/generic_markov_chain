CC = gcc
FLAGS = -Wvla -Wextra -Wall -std=c99 -c

tweets: tweets_generator.o markov_chain.o linked_list.o
	${CC} -o tweets_generator tweets_generator.o markov_chain.o linked_list.o

tweets_generator.o: tweets_generator.c
	${CC} ${FLAGS} tweets_generator.c

markov_chain.o: markov_chain.c markov_chain.h
	${CC} ${FLAGS} markov_chain.c

snakes_and_ladders.o: snakes_and_ladders.c
	${CC} ${FLAGS} snakes_and_ladders.c

snake: snakes_and_ladders.o markov_chain.o linked_list.o
	${CC} -o snakes_and_ladders snakes_and_ladders.o markov_chain.o linked_list.o
