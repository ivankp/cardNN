#include <iostream>
#include <iomanip>
#include <random>
#include <chrono>
#include <vector>
#include <utility>
#include <algorithm>
#include <list>
#include <deque>
#include <thread>
#include <mutex>

#include "network.hh"
#include "durak.hh"

using namespace std;

using nn_arr = array<network,3>;

struct nn_wrap {
  nn_arr *nn;
  int wins;
  bool active;
  nn_wrap(nn_arr* nn): nn(nn), wins(0), active(false) { }
  nn_wrap(nn_wrap&& other)
  : nn(other.nn), wins(other.wins), active(other.active) {
    other.nn = nullptr;
  }
  nn_wrap& operator=(nn_wrap&& other) noexcept {
    nn = other.nn;
    wins = other.wins;
    active = other.active;
    other.nn = nullptr;
    return *this;
  }
  nn_wrap(const nn_wrap&) = delete;
  // nn_wrap& operator=(const nn_wrap&) = delete;
  ~nn_wrap() { delete nn; }
};

using nn_vec  = vector<nn_wrap>;
using nn_iter = nn_vec::iterator;

std::mt19937 gen;
list<pair<nn_iter,nn_iter>> nn_pairs;
mutex nn_mutex;

void play_next_match(int n) {
  for ( ; ; ) {

    nn_mutex.lock();
    if (nn_pairs.size()==0) {
      nn_mutex.unlock();
      break;
    }

    nn_iter nn1, nn2;
    bool found_available = false;
    while (!found_available) {
      for (auto it=nn_pairs.begin(), end=nn_pairs.end(); it!=end; ++it)
        if (!it->first->active && !it->second->active) {
          found_available = true;
          nn1 = it->first;
          nn2 = it->second;
          nn_pairs.erase(it);
          break;
        }
    }

    nn1->active = true;
    nn2->active = true;
    nn_mutex.unlock();

    pair<int,int> wins;
    for (int k=0; k<n; ++k) {
      durak game(gen,nn1->nn,nn2->nn);

      int winner = -1;
      while ((winner=game.play())==-1) { }

      if (winner==0) ++wins.first;
      else ++wins.second;
    }
    if (wins.first > wins.second) ++(nn1->wins);
    else ++(nn2->wins);

    nn1->active = false;
    nn2->active = false;
  }
}

int main(int argc, char **argv)
{
  gen.seed(std::chrono::system_clock::now().time_since_epoch().count());
  std::uniform_real_distribution<network::val_t> weight_dist(-2.,2.);
  network::weight_dist_t weight_dist_fcn = [&](){ return weight_dist(gen); };

  const int n = 10;
  nn_vec nns;
  nns.reserve(n);

  for (int i=nns.size(); i<n; ++i)
    nns.emplace_back(
      new nn_arr{{
        {36,6,5,{50,50,50,38},weight_dist_fcn},
        {36,6,5,{50,50,50,38},weight_dist_fcn},
        {36,6,5,{25,25,38},weight_dist_fcn}
      }});

  auto t1 = chrono::steady_clock::now();

  for (auto it1=nns.begin()+1, end1=nns.end(); it1!=end1; ++it1)
    for (auto it2=nns.begin(); it2!=it1; ++it2)
      nn_pairs.emplace_back(it1,it2);

  vector<thread> threads;
  const unsigned num_threads = thread::hardware_concurrency();
  threads.reserve(num_threads);
  for (unsigned i=0; i<num_threads; ++i)
    threads.emplace_back(play_next_match,51);

  for(auto& thread : threads) thread.join();

  auto t2 = chrono::steady_clock::now();

  sort(nns.begin(), nns.end(),
    [](const nn_wrap& a, const nn_wrap& b){
      return a.wins > b.wins;
    });

  for (const auto& nn : nns)
    cout << nn.wins << ' ';
  cout << ": " << chrono::duration_cast<chrono::duration<double>>(t2-t1).count()
       << " seconds" << endl;

  return 0;
}
