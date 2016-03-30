#include <iostream>
#include <iomanip>
#include <random>
#include <chrono>
#include <vector>
#include <array>
#include <tuple>
#include <iterator>
#include <thread>

#include "network.hh"
#include "durak.hh"

#define test(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

using namespace std;

template <typename Int> // n choose 2
inline Int npairs(Int n) { return (n*(n-1))>>2; }

template <class InputIterator, class Function>
inline void for_each_pair(
  InputIterator first, InputIterator last, Function fn
) {
  for (auto it1=next(first); it1!=last; ++it1)
    for (auto it2=first; it2!=it1; ++it2)
      fn(it1,it2);
}

struct nn_wrap {
  durak::nnarr_t *nn_arr;
  int id, wins;
  bool active;

  nn_wrap(durak::nnarr_t *nn_arr)
  : nn_arr(nn_arr), id(num++), wins(0), active(false) { }

  static int num;
};
int nn_wrap::num = 0;

class tourney {
  static std::mt19937 gen;
  static std::uniform_real_distribution<network::val_t> weight_dist;
  static network::weight_dist_t weight_dist_fcn;

  unsigned match_len;

  vector<nn_wrap> nns;
  using nn_it = decltype(nns)::iterator;
  vector<tuple<nn_it,nn_it,bool>> pairs;

  void play_match(decltype(pairs)::reference nnpair) {
    get<2>(nnpair) = true;
    auto& nnw1 = *get<0>(nnpair);
    auto& nnw2 = *get<1>(nnpair);

    array<int,2> wins{0,0};
    for (unsigned k=0; k<match_len; ++k) {
      durak game(gen,nnw1.nn_arr,nnw2.nn_arr);

      int winner = -1;
      while ((winner=game.play())==-1) { }

      ++wins[winner];
    }
    cout << nnw1.id <<':'<< nnw2.id << "  "
         << get<0>(wins) <<':'<< get<1>(wins) << endl;
    ++(get<0>(wins)>get<1>(wins) ? nnw1.wins : nnw2.wins);
  }

public:
  tourney(unsigned num_nets, unsigned match_len): match_len(match_len) {
    // test(gen())
    nns.reserve(num_nets);
    for (unsigned i=0; i<num_nets; ++i)
      nns.emplace_back( new durak::nnarr_t{{
        {36,6,5,{50,50,38},weight_dist_fcn},
        {36,6,5,{50,50,38},weight_dist_fcn},
        {36,6,5,{25,25,38},weight_dist_fcn}
      }});
    pairs.reserve(npairs(num_nets));
    for_each_pair(nns.begin(),nns.end(),[this](nn_it nn1,nn_it nn2){
      this->pairs.emplace_back(nn1,nn2,false);
    });
  }

  void operator()() {
    auto t1 = chrono::steady_clock::now();

    for (auto& pair : pairs) play_match(pair);

    auto t2 = chrono::steady_clock::now();
    cout << chrono::duration_cast<chrono::duration<double>>(t2-t1).count()
         << " seconds" << endl;

    for (const auto& nn : nns)
      cout << nn.id << ": " << nn.wins << endl;
  }
};

std::mt19937 tourney::gen(
  std::chrono::system_clock::now().time_since_epoch().count());
std::uniform_real_distribution<network::val_t> tourney::weight_dist(-2.,2.);
network::weight_dist_t tourney::weight_dist_fcn(
  [&](){ return tourney::weight_dist(tourney::gen); });

int main(int argc, char **argv)
{
  thread t(tourney(10,51));
  t.join();

  return 0;
}
