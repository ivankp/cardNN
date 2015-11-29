#include <iostream>
#include <iomanip>
#include <random>
#include <chrono>
#include <cstring>

#include "network.hh"
#include "durak.hh"

using namespace std;

int main(int argc, char **argv)
{
  const bool test = (argc==2 ? !strcmp(argv[1],"test") : false);

  // std::mt19937 gen{std::random_device{}()};
  std::mt19937 gen(std::chrono::system_clock::now().time_since_epoch().count());
  std::uniform_real_distribution<network::val_t> weight_dist(-2.,2.);
  network::weight_dist_t weight_dist_fcn = [&](){ return weight_dist(gen); };

  array<network,3> nn {{
    {36,6,5,{50,50,50,38},weight_dist_fcn},
    {36,6,5,{50,50,50,38},weight_dist_fcn},
    {36,6,5,{25,25,25,38},weight_dist_fcn}
  }};

  durak game(gen,&nn);

  game.print_trump();

  game.print_hand(0,test);
  game.print_hand(1,true);
  cout << endl;

  int winner = -1;

  while ((winner=game.play())==-1) {
    game.print_hand(0,test);
    game.print_hand(1,true);
    game.print_table();
    cout << endl;
  }

  cout << (winner ? "Player " : "NN ") << winner << " won!" << endl;

  return 0;
}
