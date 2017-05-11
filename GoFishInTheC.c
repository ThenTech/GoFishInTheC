#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>

#define INPUT_BUFFER_SIZE	10u

#ifdef _MSC_VER
	#include <intrin.h>
	#define countLeftZeros	__lzcnt
	#define countOnes		__popcnt64
	#define gets(s)			gets_s(s, INPUT_BUFFER_SIZE)
	#define strcasecmp		_stricmp
#else
	#define countLeftZeros	__builtin_clz
	#define countOnes		__builtin_popcountll
#endif // _MSC_VER

#ifndef __cplusplus
	typedef enum { false = 0, true = 1 } bool;
	#define PointerT(T, p)	((T*)&p)
#endif // __cplusplus

#ifdef _WIN32
	#include <windows.h>
	#define setWindowTitle(S)	SetConsoleTitle(S)
	#define clearConsole()		system("CLS");
#else
	#define setWindowTitle(S)	printf("%c]0;%s%c", '\033', S, '\007');
	#define clearConsole()		system("clear");
#endif // __WIN32

#define PLAYER_START_CARDS		9u
#define AI_CHEAT_PROBABILITY	0.3f
#define AI_ASK_HISTORY			5u

//#define DEBUG_AI


typedef enum CARDS {
	S1 = 0x0001ull, S2 = 0x0002ull, S3 = 0x0004ull, S4 = 0x0008ull,  S5 = 0x0010ull,
	S6 = 0x0020ull, S7 = 0x0040ull, S8 = 0x0080ull, S9 = 0x0100ull, S10 = 0x0200ull,
	SJ = 0x0400ull, SQ = 0x0800ull, SK = 0x1000ull,

	K1 = 0x0002000ull, K2 = 0x0004000ull, K3 = 0x0008000ull, K4 = 0x0010000ull,  K5 = 0x0020000ull,
	K6 = 0x0040000ull, K7 = 0x0080000ull, K8 = 0x0100000ull, K9 = 0x0200000ull, K10 = 0x0400000ull,
	KJ = 0x0800000ull, KQ = 0x1000000ull, KK = 0x2000000ull,

	R1 = 0x0004000000ull, R2 = 0x0008000000ull, R3 = 0x0010000000ull, R4 = 0x0020000000ull,  R5 = 0x0040000000ull,
	R6 = 0x0080000000ull, R7 = 0x0100000000ull, R8 = 0x0200000000ull, R9 = 0x0400000000ull, R10 = 0x0800000000ull,
	RJ = 0x1000000000ull, RQ = 0x2000000000ull, RK = 0x4000000000ull,

	H1 = 0x0008000000000ull, H2 = 0x0010000000000ull, H3 = 0x0020000000000ull, H4 = 0x0040000000000ull,  H5 = 0x0080000000000ull,
	H6 = 0x0100000000000ull, H7 = 0x0200000000000ull, H8 = 0x0400000000000ull, H9 = 0x0800000000000ull, H10 = 0x1000000000000ull,
	HJ = 0x2000000000000ull, HQ = 0x4000000000000ull, HK = 0x8000000000000ull,

	SCHOPPEN = (SK << 1ull) - 1ull,
	KLAVEREN = (KK << 1ull) - 1ull - SCHOPPEN,
	RUITEN   = (RK << 1ull) - 1ull - KLAVEREN - SCHOPPEN,
	HARTEN   = (HK << 1ull) - 1ull - RUITEN - KLAVEREN - SCHOPPEN,
	ALL      = (HK << 1ull) - 1ull,
	NONE     = 0ull,

	SUIT_SIZE =  4ull,
	RANK_SIZE = 13ull,
	DECK_SIZE = 52ull,

	ONES   = S1 | K1 | R1 | H1,
	TWOS   = S2 | K2 | R2 | H2,
	THREES = S3 | K3 | R3 | H3,
	FOURS  = S4 | K4 | R4 | H4,
	FIVES  = S5 | K5 | R5 | H5,
	SIXES  = S6 | K6 | R6 | H6,
	SEVENS = S7 | K7 | R7 | H7,
	EIGHTS = S8 | K8 | R8 | H8,
	NINES  = S9 | K9 | R9 | H9,
	TENS   = S10 | K10 | R10 | H10,
	JACKS  = SJ | KJ | RJ | HJ,
	QUEENS = SQ | KQ | RQ | HQ,
	KINGS  = SK | KK | RK | HK
} CARD;

typedef uint64_t DECK;

typedef struct PLAYER {
	const char* name;
	DECK hand;
	DECK stacks;
} Player;

static const char  suits[SUIT_SIZE] = { 'S', 'K', 'R', 'H' };
static const char* ranks[RANK_SIZE] = { "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K" };

static const CARD  stackMasks[] = { ONES, TWOS, THREES, FOURS, FIVES, SIXES, SEVENS, EIGHTS, NINES, TENS, JACKS, QUEENS, KINGS };

static void updateTitle(const Player * const p1, const uint32_t p1_stacks, const Player * const p2, const uint32_t p2_stacks, const DECK gamedeck) {
	static const char *game_title_format = "Go Fish in the C :: [Score] %6s: % 2d cards in hand and % 2d stacks, %6s: % 2d cards in hand and % 2d stacks, %2d cards left in stack";
	static char titlebuffer[150u];

	#ifdef _MSC_VER
		sprintf_s(titlebuffer, 150u, game_title_format, p1->name, countOnes(p1->hand), p1_stacks, p2->name, countOnes(p2->hand), p2_stacks, countOnes(gamedeck));
	#else
		sprintf(titlebuffer, game_title_format, p1->name, countOnes(p1->hand), p1_stacks, p2->name, countOnes(p2->hand), p2_stacks, countOnes(gamedeck));
	#endif

	setWindowTitle(titlebuffer);
}

void printCard(const CARD c, bool rank_only /* = false */) {
	if (c != NONE) {
		const uint64_t suit =         c & SCHOPPEN
							 ? 0ull : c & KLAVEREN
							 ? 1ull : c & RUITEN
							 ? 2ull : c & HARTEN
							 ? 3ull : 0ull;

		if (!rank_only) putchar(suits[suit]);
		printf(rank_only ? "%s" : "%-2s", ranks[31u - countLeftZeros((uint32_t)(c >> (suit * RANK_SIZE)))]);
	}
}

CARD getCardRankFromString(const char * const buff) {
	int32_t rank = -1;

	for (uint32_t m = 0u; m < RANK_SIZE; m++) {
		if (strcasecmp(buff, ranks[m]) == 0) {
			rank = m;
			break;
		}
	}

	return (rank >= 0) ? stackMasks[rank] : NONE;
}

CARD getCardRank(const CARD card) {
	int32_t rank = -1;

	for (uint32_t m = 0u; m < RANK_SIZE; m++) {
		if (card & stackMasks[m]) {
			rank = m;
			break;
		}
	}

	return (rank >= 0) ? stackMasks[rank] : NONE;
}

void printDeck(const DECK d) {
	// Print CARDS::DECK_SIZE bits representing the given DECK
	for (uint64_t b = HK; b; b >>= 1ull) {
		if ((b < HK) && (b & KINGS)) putchar(' ');
		putchar((d & b) ? '1' : '0');
	}
}

void printDeckCards(const DECK d, bool rank_only /* = false */) {
	bool is_not_first_card = false;

	for (uint64_t b = 0u; b < RANK_SIZE; b++) {
		const bool has_cards = (d & stackMasks[b]) != NONE;

		if (has_cards) {
			const uint64_t rank = (1ull << b);

			if (rank_only) {
				if (is_not_first_card) printf(", ");
				else                   is_not_first_card = true;
				printCard((CARD) rank, rank_only);
			} else {
				for (uint64_t suit = 0ull; suit < SUIT_SIZE; suit++) {
					const uint64_t card = d & (rank << (RANK_SIZE * suit));

					if (card != NONE) {
						if (is_not_first_card) printf(", ");
						else                   is_not_first_card = true;
						printCard((CARD) card, false);
					}
				}
			}
		}
	}
}

void printPlayerDeck(const Player * const p) {
	printf("%6s's hand: ", p->name);
	printDeckCards(p->hand, false);
	putchar('\n');
}

CARD drawCardFromDeck(DECK * const d) {
	CARD c;
	assert(*d != 0ull);

	do {
		c = (CARD)(1ull << (rand() % DECK_SIZE));
	} while ((*d & c) == NONE);

	*d &= ~c;

	return c;
}

static inline void cardSwitchDecks(const CARD c, DECK *from, DECK *to) {
	*to   |= (*from & c);
	*from &= ~c;
}

CARD playerGoFish(DECK * const gamedeck, Player * const player) {
	CARD c = drawCardFromDeck(gamedeck);

	player->hand |= c;

	return c;
}

void playerCheckHandForStacks(Player * const player) {
	for (uint32_t m = 0u; m < RANK_SIZE; m++) {
		const CARD stack = (CARD) (player->hand & stackMasks[m]);

		if (stack && countOnes(stack) == 4ull) {
			printf("\n*** %s has a stack of %ss! ***\n\n", player->name, ranks[m]);
			cardSwitchDecks(stack, PointerT(DECK, player->hand), PointerT(DECK, player->stacks));
		}
	}
}

static inline uint32_t playerCountStacks(const Player * const player) {
	return (uint32_t)countOnes(player->stacks) / SUIT_SIZE;
}

bool playerRequestRank(DECK * const gamedeck, Player * const player, Player *const pc) {
	char buff[INPUT_BUFFER_SIZE];
	CARD c;

	if (!player->hand) {
		c = playerGoFish(gamedeck, player);
		printf("You went fishing and got: "); printCard(c, false); putchar('\n');
		return false;
	}

	do {
		printf("What card rank would you like to ask for (Choose from a rank you have at least 1 card of)? ");
		gets(buff);
		c = getCardRankFromString(buff);
	} while(!c || !(player->hand & c));

	if (pc->hand & c) {
		printf("%s: Dang it! You got me!\n", pc->name);
		printf("%s received ", player->name); printDeckCards(pc->hand & c, false); printf(" from %s!\n\n", pc->name);
		cardSwitchDecks(c, PointerT(DECK, pc->hand), PointerT(DECK, player->hand));
		playerCheckHandForStacks(player);
		return true; // Continue asking
	} else {
		printf("%s: %s, go fish!\n", pc->name, player->name);
		c = playerGoFish(gamedeck, player);
		printf("You went fishing and got: "); printCard(c, false); putchar('\n');
		playerCheckHandForStacks(player);
		return false; // Stop asking
	}
}


/*
 *	1. Hou bij waar speler naar vraagt en als player_request_history nu in hand zit, vraag ernaar
 *	2. Vraag naar meest voorkomende rank in hand en sla op
 *     (2de keer: vraag naar volgende),
 *     na x keer, vraag opnieuw naar hoogste
 *  3. Cheat? Kijk naar overeenkomsten in pc.hand en player.hand en vraag naar die kaarten
 *     >> eventueel met setting? of met kans vb 7/10 => doe 2; 3/10 => doe 3?
 */

CARD pcUpdateAskHistory(const CARD asked) {
	static uint32_t times_asked = 0u;
	static CARD		AskHistory[AI_ASK_HISTORY] = { NONE };
	static DECK     history_mask = 0ull;

	if (asked == NONE) {
		memset(AskHistory, NONE, AI_ASK_HISTORY);
		history_mask = 0ull;
	} else {
		history_mask ^= AskHistory[times_asked];
		history_mask |= (AskHistory[times_asked++] = asked);

		if (times_asked >= AI_ASK_HISTORY)
			times_asked = 0u;
	}

	return (CARD)history_mask;
}

bool pcRequestRank(DECK * const gamedeck, Player * const pc, Player * const player) {
	char buff[INPUT_BUFFER_SIZE];
	DECK cards_to_ask_mask = NONE;
	CARD ask_for_card, ask_for_card_rank;
	uint32_t tried_finding_card = 0u;

	static CARD last_asked = NONE;

	if (!pc->hand) {
		ask_for_card = playerGoFish(gamedeck, pc);

		printf("%s went fishing...", pc->name);

		#ifdef DEBUG_AI
			printf(" and got: "); printCard(ask_for_card, false);
		#endif

		putchar('\n');

		return false;
	}

	//*******  CHEAT  *****************************************************
	if ((float)rand() / RAND_MAX < AI_CHEAT_PROBABILITY) {
		#ifdef DEBUG_AI
			printf("[AI] Determining perfect cards_to_ask_mask... ");
		#endif
		for (uint64_t rank = 0ull; rank < RANK_SIZE; rank++) {
			const bool player_has_cards = (player->hand & stackMasks[rank]) != NONE;

			if (player_has_cards && (pc->hand & stackMasks[rank])) {
				cards_to_ask_mask |= stackMasks[rank];
			}
		}

		#ifdef DEBUG_AI
			printf(cards_to_ask_mask ? "succeeded\n" : "failed\n");
		#endif
	}

	if (!cards_to_ask_mask)
		cards_to_ask_mask = pc->hand;

	do {
		if (!cards_to_ask_mask || (++tried_finding_card > (AI_ASK_HISTORY * 3u))) {
			last_asked		  = pcUpdateAskHistory(NONE);
			cards_to_ask_mask = pc->hand;

			#ifdef DEBUG_AI
				printf("[Ask history] Found no suitable card to ask, ask history was resetted\n");
			#endif
			tried_finding_card = 0u;
		}

		ask_for_card      = drawCardFromDeck(PointerT(DECK, cards_to_ask_mask));
		ask_for_card_rank = getCardRank(ask_for_card);
	} while (ask_for_card_rank & last_asked);

	do {
		printf("%s: Do you have any... ", pc->name);
		printCard(ask_for_card, true);
		printf("s? (Y/N) ");

		gets(buff);

		if ((strcasecmp(buff, "N") == 0) && ((player->hand & ask_for_card_rank) != NONE)) {
			printf("%s: CHEATER!\n", pc->name);
		} else if ((player->hand & ask_for_card_rank) == NONE) {
			printf("%s: I don't have any!\n", player->name);
			break;
		};
	} while(strcasecmp(buff, "Y"));

	last_asked = pcUpdateAskHistory(ask_for_card_rank);
	#ifdef DEBUG_AI
		printf("[Ask history]: "); printDeckCards((DECK)last_asked, true); putchar('\n');
	#endif

	if (player->hand & ask_for_card_rank) {
		printf("%s: Haha!\n", pc->name);
		printf("%s received ", pc->name); printDeckCards(player->hand & ask_for_card_rank, false); printf(" from %s!\n\n", player->name);
		cardSwitchDecks(ask_for_card_rank, PointerT(DECK, player->hand), PointerT(DECK, pc->hand));
		playerCheckHandForStacks(pc);

		return true; // Continue asking
	} else {
		printf("%s: %s, go fish!\n", player->name, pc->name);
		ask_for_card = playerGoFish(gamedeck, pc);
		printf("%s went fishing...", pc->name);

		#ifdef DEBUG_AI
			printf(" and got: "); printCard(ask_for_card, false);
		#endif

		putchar('\n');

		playerCheckHandForStacks(pc);
		return false; // Stop asking
	}

	//*********************************************************************
}

bool checkGameWon(const DECK deck_stapel, const Player * const p1, const Player * const p2) {
	static const char *format_win  = "%s won with %d stacks: ",
					  *format_lose = "\n%s had %d stacks: ";

	const uint32_t p1_stacks = playerCountStacks(p1),
				   p2_stacks = playerCountStacks(p2);

	updateTitle(p1, p1_stacks, p2, p2_stacks, deck_stapel);

	if (deck_stapel || p1->hand || p2->hand) return false;

	putchar('\n');

	if (p1_stacks > p2_stacks) {
		printf(format_win, p1->name, p1_stacks);
		printDeckCards(p1->stacks, true);
		printf(format_lose, p2->name, p2_stacks);
		printDeckCards(p2->stacks, true);
	} else {
		printf(format_win, p2->name, p2_stacks);
		printDeckCards(p2->stacks, true);
		printf(format_lose, p1->name, p1_stacks);
		printDeckCards(p1->stacks, true);
	}

	putchar('\n');

	return true;
}

int main() {
	srand((uint32_t)time(NULL));

	// Set-up game
	bool won = false;
	DECK deck_stapel = ALL;
	Player player_1	 = { "Player", NONE, NONE },
		   player_pc = { "PC",     NONE, NONE };

	for (uint32_t i = 0u; i < PLAYER_START_CARDS; i++) {
		playerGoFish(PointerT(DECK, deck_stapel), PointerT(Player, player_1));
		playerGoFish(PointerT(DECK, deck_stapel), PointerT(Player, player_pc));
	}

	updateTitle(PointerT(Player, player_1), 0u, PointerT(Player, player_pc), 0u, deck_stapel);

	#ifdef DEBUG_AI
		printf("Player  1 : "); printDeck(player_1.hand);  putchar('\n');
		printf("Player pc : "); printDeck(player_pc.hand); putchar('\n');
		printf("Stack left: "); printDeck(deck_stapel); putchar('\n');
	#endif

	while(!won) {
		#ifdef DEBUG_AI
			printPlayerDeck(PointerT(Player, player_pc));
		#endif

		printf("****************** %s's turn ******************\n\n", player_1.name);
		printPlayerDeck(PointerT(Player, player_1));
		putchar('\n');

		while(!won && playerRequestRank(PointerT(DECK, deck_stapel), PointerT(Player, player_1), PointerT(Player, player_pc)))
			won = checkGameWon(deck_stapel, PointerT(Player, player_1), PointerT(Player, player_pc));
		if (won || checkGameWon(deck_stapel, PointerT(Player, player_1), PointerT(Player, player_pc))) break;

		putchar('\n');
		printPlayerDeck(PointerT(Player, player_1));

		printf("\n****************** %s's turn ******************\n\n", player_pc.name);
		while(!won && pcRequestRank(PointerT(DECK, deck_stapel), PointerT(Player, player_pc), PointerT(Player, player_1)))
			won = checkGameWon(deck_stapel, PointerT(Player, player_1), PointerT(Player, player_pc));
		if (won || checkGameWon(deck_stapel, PointerT(Player, player_1), PointerT(Player, player_pc))) break;

		getchar();
		clearConsole();
	}

	getchar();

	return 0;
}
