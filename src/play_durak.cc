#include <iostream>
#include <iomanip>

#include "network.hh"
#include "durak.hh"

using namespace std;

int main(int argc, char **argv)
{
  network nn(36,6,5,{50,38});

  durak game(&nn);

  game.print_trump();

  game.print_hand(0);
  game.print_hand(1);
  cout << endl;

  int winner = -1;

  while ((winner=game.play())==-1) {
    game.print_hand(0);
    game.print_hand(1);
    game.print_table();
    cout << endl;
  }

  cout << (winner ? "Player " : "NN ") << winner << " won!" << endl;

  return 0;
}
