#include "durak.hh"

#include <iostream>
#include <iomanip>
#include <algorithm>

#include "network.hh"

using namespace std;

#define test(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

enum {
  not_seen     = 0,
  in_own_hand  = 1,
  in_op_hand   = 2,
  on_table     = 3,
  discarded    = 4,
  last_in_deck = 5
};

enum {
  ncards = 36,
  nacts  = 38
};

enum {
  done = 36,
  take = 37
};

durak::durak(std::mt19937& rand, nnarr_t* nn1, nnarr_t* nn2)
: rules(ncards,rand), nn{nn1,nn2}
{
  const int trump_card = deck.front();
  for (int i=0; i<2; ++i) {
    if (nn[i]) {
      for (network& net : *nn[i]) {
        net.reset_states(trump_suit = trump_card%4);
        net.set_state(trump_card,last_in_deck);
      }
    }
  }

  deal(0);
  deal(1);

  adding = false;
}

void durak::print_trump() {
  *out << "Trump: ";
  print_card(deck.front());
  *out << endl << endl;
}

void durak::deal(int h) {
  for (size_t i=hand[h].size(); i<6 && deck.size(); ++i) {
    int card = deck.back();
    deck.pop_back();
    hand[h].push_back(card);
    if (nn[h])
      for (network& net : *nn[h])
        net.set_state(card,in_own_hand);
  }
}

// TODO:
// -- play multiple cards?

// Check if the card is in the currently played hand
bool durak::in_hand(int card) const {
  const auto begin=hand[turn].begin(), end=hand[turn].end();
  return find(begin,end,card)!=end;
}

bool durak::is_playable(int action) const {
  if (adding) {
    if (action<ncards) {
      if (!in_hand(action)) return false;

      return find_if(table.begin(),table.end(),
        [action](int a){ return (a/4)==(action/4); })!=table.end();

    } else {
      switch (action) {
        case done: return  true; break;
        case take: return false; break;
      }
    }
  } else {
    if (action<ncards) {
      if (!in_hand(action)) return false;
      if (table.size()==0) return true;

      if (table.size()%2) { // need to beat

        if ((table.back()%4)==(action%4)) {
          return ((table.back()/4) <(action/4));
        } else {
          return ((action%4)==trump_suit);
        }

      } else { // add a card

        return find_if(table.begin(),table.end(),
          [action](int a){ return (a/4)==(action/4); })!=table.end();

      }
    } else {
      if (table.size()==0) return false;
      else if (table.size()%2) { // beat or take
        switch (action) {
          case done: return false; break;
          case take: return  true; break;
        }
      } else { // add or done
        switch (action) {
          case done: return  true; break;
          case take: return false; break;
        }
      }
    }
  }
  return false;
}

void durak::play_action(int a) {
  if (a<ncards) {
    hand[turn].remove(a);
    table.push_back(a);
    for (int i=0; i<2; ++i)
      if (nn[i])
        for (network& net : *nn[i])
          net.set_state(a,on_table);
    if (adding) ++adding;
  } else if (a==done) { // turn done
    if (adding) {
      adding = 0;
      turn = other();
      play_take();
    } else {
      for (int card : table)
        for (int i=0; i<2; ++i)
          if (nn[i])
            for (network& net : *nn[i])
              net.set_state(card,discarded);
      table.clear();
      deal(turn);
      deal(other());
    }
  } else if (a==take) { // declare take

    // allow oponent to add more cards
    for (size_t i=0, ni=table.size(); i<ni; ++i)
      for (int h : hand[other()])
        if ((h/4)==(table[i]/4)) {
          i = ni;
          adding = 1;
          break;
        }

    if (!adding) play_take();
  }
}

inline void durak::play_take() {
  for (int card : table) {
    hand[turn].push_back(card);
    if (nn[turn])
      for (network& net : *nn[turn])
        net.set_state(card,in_own_hand);
    if (nn[other()])
      for (network& net : *nn[other()])
        net.set_state(card,in_op_hand);
  }
  table.clear();
  deal(other());
}

int durak::play() {
  if (nn[turn]) play_nn();
  else play_human();

  if (hand[0].size()==0 || hand[1].size()==0) {
    if (deck.size()) play_action(done);
    else return (hand[0].size()==0 ? 0 : 1);
  }

  if (adding < 2) turn = other();
  else if (adding > hand[other()].size()) {
    turn = other();
    play_take();
    turn = other();
  }

  return -1;
}

void durak::play_nn() {
  const size_t nni = (
    adding ? 2 : table.size()%2
  );
  // 0 - lead
  // 1 - beat or take
  // 2 - adding
  (*nn[turn])[nni].eval();
  int a = -1;
  float max_field = -std::numeric_limits<float>::max();
  for (int i=0; i<nacts; ++i) {
    // in_hand check is only for optimization
    if ((in_hand(i) || i>=ncards) && is_playable(i)) {
      const float field = (*nn[turn])[nni].get_field(i);
      if (max_field < field) {
        max_field = field;
        a = i;
      }
    }
  }
  if (a==-1) throw std::runtime_error("NN has no playable action");
  play_action(a);
}

void durak::play_human() {
  int a;
  cout << "Action: ";
  cin >> a;
  if (is_playable(a)) {
    play_action(a);
  } else {
    cout << "Action " << a << " is not playable!" << endl;
    play_human();
  }
}

void durak::print_card(int card) {
  const int suit = card%4, num = card/4;

  if (suit%2) *out << "\033[1;47;2;31m";
  else        *out << "\033[1;47;2;30m";

  switch (num) {
    case  0: *out << '6'; break;
    case  1: *out << '7'; break;
    case  2: *out << '8'; break;
    case  3: *out << '9'; break;
    case  4: *out << 'T'; break;
    case  5: *out << 'J'; break;
    case  6: *out << 'Q'; break;
    case  7: *out << 'K'; break;
    case  8: *out << 'A'; break;
    default: break;
  }

  switch (suit) {
    case 0: *out << "♠"; break;
    case 1: *out << "♥"; break;
    case 2: *out << "♣"; break;
    case 3: *out << "♦"; break;
    default: break;
  }

  *out << setw(2) << card << "\033[0m";
}
