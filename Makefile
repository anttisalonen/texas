HEADERS = cards.h \
       th.h \
       pool.h \
       ai_common.h \
       ai_random.h \
       ann1.h \
       ann_common.h \
       ai_manual.h \
       ai_manual2.h \
       ai_config.h \
       ncui.h \
       pdb.h

SHSRCS = cards.c \
       th.c \
       pool.c \
       ai_common.c \
       ai_random.c \
       ann1.c \
       ann_common.c \
       ai_manual.c \
       ai_manual2.c \
       ai_config.c \
       pdb.c

TXSRCS = ncui.c texas.c

PLSRCS = players.c

SHOBJS = $(SHSRCS:.c=.o)
TXOBJS = $(TXSRCS:.c=.o)
PLOBJS = $(PLSRCS:.c=.o)

default: texas players

CFLAGS = -Wall -Werror -g3

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

texas: $(SHOBJS) $(TXOBJS) ais
	$(CC) $(CFLAGS) -lfann -lm -ldb -lncurses $(SHOBJS) $(TXOBJS) -o $@

players: $(SHOBJS) $(PLOBJS)
	$(CC) $(CFLAGS) -lfann -lm -ldb $(SHOBJS) $(PLOBJS) -o $@

ais:
	mkdir -p ais

clean:
	rm -f $(SHOBJS)
	rm -f $(TXOBJS)
	rm -f $(PLOBJS)
	rm -f texas players
