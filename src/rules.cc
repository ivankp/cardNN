#include "rules.hh"

#include <iostream>
#include <random>
#include <algorithm>
#include <numeric>
#include <stdexcept>

rules::rules(size_t deck_size, network* nn1, network* nn2)
: deck(deck_size), nn{nn1,nn2}, turn(std::random_device{}()%2)
{
  std::iota(deck.begin(), deck.end(), 0);
  std::shuffle(deck.begin(), deck.end(), std::mt19937{std::random_device{}()});
}

void rules::print_hand(int h) {
  if (nn[h]) *out << "NN     ";
  else       *out << "Player ";
  *out << h << ":";

  for (int card : hand[h]) {
    *out << ' ';
    print_card(card);
  }
  *out << std::endl;
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
