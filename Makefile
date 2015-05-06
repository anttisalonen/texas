HEADERS = cards.h \
       th.h \
       pool.h \
       ai_common.h \
       ai_random.h \
       ann1.h \
       ai_config.h \

SRCS = cards.c \
       th.c \
       pool.c \
       ai_common.c \
       ai_random.c \
       ann1.c \
       ai_config.c \
       texas.c

OBJS = $(SRCS:.c=.o)

default: texas

CFLAGS = -Wall -Werror -std=c99 -g3

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

texas: $(OBJS)
	$(CC) $(CFLAGS) -lfann -lm -lncurses $(OBJS) -o $@

clean:
	rm -f $(OBJS)
	rm -f texas
