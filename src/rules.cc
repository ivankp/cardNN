#include "rules.hh"

#include <iostream>
#include <algorithm>
#include <utility>

rules::rules(size_t deck_size, std::mt19937& rand)
: deck(deck_size), turn(rand()%2)
{
  std::iota(deck.begin(), deck.end(), 0);
  std::shuffle(deck.begin(), deck.end(), rand);
}

void rules::print_hand(int h, bool show) {
  *out << "Player " << h << ":";

  if (show) for (int card : hand[h]) {
    *out << ' ';
    print_card(card);
  }
  *out << " (" << hand[h].size() << ')' << std::endl;
}

void rules::print_table() {
  *out << "Table   :";
  for (int card : table) {
    *out << ' ';
    print_card(card);
  }
  *out << std::endl;
}

std::ostream* rules::out = &std::cout;
