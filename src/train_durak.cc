#include <iostream>
#include <iomanip>
#include <chrono>

#include "network.hh"
#include "durak.hh"

using namespace std;

int main(int argc, char **argv)
{
  auto t1 = chrono::steady_clock::now();
  for (int i=0; i<1000; ++i) {
    network nn1(36,6,5,{50,38});
    network nn2(36,6,5,{50,50,38});

    durak game(&nn1,&nn2);

    int winner = -1;
    while ((winner=game.play())==-1) { }
  }
  auto t2 = chrono::steady_clock::now();
  cout << chrono::duration_cast<chrono::duration<double>>(t2-t1).count()
       << " seconds" << endl;

  return 0;
}
