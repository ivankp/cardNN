#include <iostream>
#include <iomanip>
#include <random>
#include <chrono>

#include "network.hh"
#include "durak.hh"

using namespace std;

int main(int argc, char **argv)
{
  std::mt19937 gen(std::chrono::system_clock::now().time_since_epoch().count());
  std::uniform_real_distribution<network::val_t> weight_dist(-2.,2.);
  network::weight_dist_t weight_dist_fcn = [&](){ return weight_dist(gen); };

  auto t1 = chrono::steady_clock::now();
  /*for (int i=0; i<1000; ++i) {
    array<network,1> nn1 {{
      {36,6,5,{50,38},weight_dist_fcn}
    }};
    array<network,1> nn2 {{
      {36,6,5,{50,38},weight_dist_fcn}
    }};

    durak game(gen,&nn1,&nn2);

    int winner = -1;
    while ((winner=game.play())==-1) { }
  }*/
  auto t2 = chrono::steady_clock::now();
  cout << chrono::duration_cast<chrono::duration<double>>(t2-t1).count()
       << " seconds" << endl;

  return 0;
}
