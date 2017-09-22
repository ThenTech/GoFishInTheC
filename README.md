Go Fish In The C
================

Basic card game where you collect stacks of the same rank, untill no cards are left.
The player with the most stacks, wins.

### Features
 - Minimal space required for each player and the game state (`uint64_t`s)
 - Can be compiled with both GCC and MSVC on Windows and Unix
 - Includes methods to manage cards in a deck represented as 52 bits in a `uint64_t`
 
   e.g. printing, transferring cards, checking stacks by masking, draw a card...
 - Main logic is handled by:
   1. **playerRequestRank**
   
      Ask for user input and gives appropriate response when requested rank is available or not;
   2. **pcRequestRank**
   
      Configure percentage that causes the AI to make a 'smart' guess.
      Creates a mask of possible cards to ask (rank included in hand),
      and might narrow it down by 'cheating' and looking at the player's cards.
      Then picks a card at random from the mask to actually ask.
      Last guesses are cached to avoid continuously asking the same card.
      Additionally, the player cannot cheat by saying `no` when he has the requested card in his hand.
 - Score is kept in window title, when no cards are left, both the players's hands and the deck,
   the game is won by the player with the most stacks.
