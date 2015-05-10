#include <stdio.h>
#include <string.h>
#include <poll.h>
#include <unistd.h>

#include <ncurses.h>

#include "ncui.h"

#define PROMPT_Y 30
#define PROMPT_X 1

static int speed = 5;
static int human = 0;

static void print_cards(int y, int x, const struct card *cards, int num_cards)
{
	static const char* ranks[] = {
		"2",
		"3",
		"4",
		"5",
		"6",
		"7",
		"8",
		"9",
		"10",
		"J",
		"Q",
		"K",
		"A"
	};

	static const char* suits[] = {
		"Spades",
		"Hearts",
		"Diamonds",
		"Clubs"
	};

	for(int i = 0; i < num_cards; i++) {
		const char* rank = ranks[cards[i].rank];
		const char* suit = suits[cards[i].suit];
		mvprintw(y + i, x, "%s of %s", rank, suit);
	}
}

static void print_community_cards(const struct texas_holdem *th)
{
	print_cards(10, 11, th->community_cards, th->num_community_cards);
}

static void print_pot(const struct texas_holdem *th)
{
	mvprintw(16, 16, "Pot: %d", th->pot);
}

static void get_player_coordinates(int plnum, int *y, int *x)
{
	*x = 5 + (plnum % 5) * 30;
	*y = plnum < 5 ? 2 : 20;
}

static void print_player(const struct texas_holdem *th, int player_index, int show_all_holes)
{
	int x, y;
	get_player_coordinates(player_index, &y, &x);

	mvprintw(y,     x, "%s", th->players[player_index].name);
	mvprintw(y + 1, x, "Money: %d   ", th->players[player_index].money);
	if(show_all_holes || th->players[player_index].decide == human_decision) {
		print_cards(y + 2, x, th->players[player_index].hole_cards, 2);
	}
	if(!th->players[player_index].active) {
		mvprintw(y + 4, x, "folds");
	}
}

static void print_players(const struct texas_holdem *th, int show_all_holes)
{
	for(int i = 0; i < TH_MAX_PLAYERS; i++) {
		if(!th->players[i].decide)
			continue;

		print_player(th, i, show_all_holes);
	}
}

enum th_decision human_decision(const struct texas_holdem *th, int plnum, int raised_to, void *data)
{
	clear();
	while(1) {
		print_players(th, 0);
		print_community_cards(th);
		print_pot(th);

		if(raised_to)
			mvprintw(PROMPT_Y, PROMPT_X, "(f)old, c(a)ll, (r)aise?\n");
		else
			mvprintw(PROMPT_Y, PROMPT_X, "(c)heck, (r)aise?\n");
		refresh();

		char buf[256];
		memset(buf, 0x00, sizeof(buf));
		read(STDIN_FILENO, buf, 256);
		switch(buf[0]) {
			case 'c':
				if(!raised_to)
					return DEC_CHECK;
				break;
			case 'f':
				if(raised_to)
					return DEC_FOLD;
				break;
			case 'a':
				if(raised_to)
					return DEC_CALL;
				break;
			case 'r':
				return DEC_RAISE;
			default:
				break;
		}
	}
}

static void print_end_of_round(const struct texas_holdem *th, const struct th_event *ev, int wait)
{
	for(int i = 0; i < TH_MAX_PLAYERS; i++) {
		if(!th->players[i].active)
			continue;
		print_player(th, i, 1);
		if(wait) {
			poll(NULL, 0, 100 * speed);
			refresh();
		}
		int y, x;
		get_player_coordinates(i, &y, &x);
		mvprintw(y + 4, x, "%s", ev->best_hands[i].cat->name);
		if(wait) {
			poll(NULL, 0, 100 * speed);
			refresh();
		}
	}
	if(!wait)
		refresh();
}

void ncui_event_callback(const struct texas_holdem *th, const struct th_event *ev)
{
	int x, y;
	get_player_coordinates(ev->player_index, &y, &x);
	clear();
	print_players(th, !human);
	print_community_cards(th);
	print_pot(th);

	switch(ev->type) {
		case TH_EVENT_DECISION:
			switch(ev->decision) {
				case DEC_CHECK:
					mvprintw(y + 4, x, "checks");
					break;

				case DEC_FOLD:
					mvprintw(y + 4, x, "folds");
					break;

				case DEC_CALL:
					mvprintw(y + 4, x, "calls");
					break;

				case DEC_RAISE:
					mvprintw(y + 4, x, "raises by %d", ev->raise_amount);
					break;
			}
			break;

		case TH_EVENT_END_OF_ROUND:
			print_end_of_round(th, ev, 1);
			break;

		case TH_EVENT_WIN:
			print_end_of_round(th, ev, 0);
			for(int i = 0; i < ev->num_winners; i++) {
				get_player_coordinates(ev->winner_index[i], &y, &x);
				mvprintw(y + 5, x, "wins %d",
						ev->winner_money[i]);
			}

			poll(NULL, 0, 100 * speed);
			refresh();
			{
				mvprintw(PROMPT_Y, PROMPT_X, "Hit Enter to continue");
				char buf[256];
				refresh();
				read(STDIN_FILENO, buf, 256);
			}
			break;

		case TH_EVENT_BET_ROUND_BEGIN:
			print_community_cards(th);
			print_players(th, !human);
			print_pot(th);
			poll(NULL, 0, 100 * speed);
			refresh();
			break;

		case TH_EVENT_HANDS_DEALT:
			break;
	}
	poll(NULL, 0, 100 * speed);
	refresh();
}

void ncui_set_speed(int s)
{
	speed = s;
}

void ncui_set_human(int h)
{
	human = h;
}

void ncui_init(void)
{
	initscr();
	noecho();
	curs_set(0);
}

void ncui_deinit(void)
{
	endwin();
}

