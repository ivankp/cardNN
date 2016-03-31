#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <array>
#include <algorithm>
#include <random>
#include <chrono>
#include <cstring>

#include "network.hh"
#include "durak.hh"

#define test(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

using namespace std;

std::mt19937 gen;
std::uniform_real_distribution<network::val_t> weight_dist(-2.,2.);
network::weight_dist_t weight_dist_fcn([&](){ return weight_dist(gen); });

const char* print_card(int card) {
  static char str[8]; // not thread safe
  const int suit = card%4, num = card/4;

  char *strp = str;
  switch (num) {
    case  0: strcpy(strp, "6"); strp+=1; break;
    case  1: strcpy(strp, "7"); strp+=1; break;
    case  2: strcpy(strp, "8"); strp+=1; break;
    case  3: strcpy(strp, "9"); strp+=1; break;
    case  4: strcpy(strp,"10"); strp+=2; break;
    case  5: strcpy(strp, "J"); strp+=1; break;
    case  6: strcpy(strp, "Q"); strp+=1; break;
    case  7: strcpy(strp, "K"); strp+=1; break;
    case  8: strcpy(strp, "A"); strp+=1; break;
  }

  switch (suit) {
    case 0: strcpy(strp,"♠"); break;
    case 1: strcpy(strp,"♥"); break;
    case 2: strcpy(strp,"♣"); break;
    case 3: strcpy(strp,"♦"); break;
  }

  return str;
}

#define ncards 36

const char* print_act(int act) {
  if (act  < ncards) return print_card(act);
  if (act == ncards) return "done";
  if (act == ncards+1) return "take";
  return "?";
}

struct nn_wrap {
  durak::nnarr_t *nn_arr;
  int id, wins;

  nn_wrap()
  : nn_arr( new durak::nnarr_t{{
      {36,6,5,{50,50,38},weight_dist_fcn},
      {36,6,5,{50,50,38},weight_dist_fcn},
      {36,6,5,{25,25,38},weight_dist_fcn}
    }} ), id(num++), wins(0) { }
  ~nn_wrap() { delete nn_arr; }

  nn_wrap(nn_wrap&& w) noexcept
  : nn_arr(w.nn_arr), id(w.id), wins(w.wins) {
    w.nn_arr = nullptr;
  }
  nn_wrap& operator=(nn_wrap&& w) {
    delete nn_arr;
    nn_arr = w.nn_arr;
    w.nn_arr = nullptr;
    id = w.id;
    wins = w.wins;
    return *this;
  }

  // forbid copying
  nn_wrap(const nn_wrap&) = delete;
  nn_wrap& operator=(const nn_wrap&) = delete;

  static int num;
};
int nn_wrap::num = 0;

int main(int argc, char **argv)
{
  gen.seed(std::chrono::system_clock::now().time_since_epoch().count());
  const unsigned num_nets = 12, match_len = 31, num_rounds = 100,
                 num_best = 4;

  vector<nn_wrap> nns(num_nets);

  for (unsigned r=1; r<=num_rounds; ++r) {
    cout << "Round " << r << endl;
    auto t1 = chrono::steady_clock::now();

    // loop over pairs of nets
    for (auto nn1=nns.begin()+1, end1=nns.end(); nn1!=end1; ++nn1) {
      for (auto nn2=nns.begin(); nn2!=nn1; ++nn2) {
        array<int,2> wins{0,0}; // wins in the current match
        for (unsigned k=0; k<match_len; ++k) {
          durak game(gen,nn1->nn_arr,nn2->nn_arr); // play 1 game
          int winner = -1;
          while ((winner=game.play())==-1) { }
          ++wins[winner];
        }

        ++(get<0>(wins)>get<1>(wins) ? nn1->wins : nn2->wins);
        // cout << nn1->id <<':'<< nn2->id << "  "
        //      << get<0>(wins) <<':'<< get<1>(wins) << endl;
      }
    }

    auto t2 = chrono::steady_clock::now();
    cout << chrono::duration_cast<chrono::duration<double>>(t2-t1).count()
         << " seconds" << endl;

    // keep 5 best nets after round
    vector<unsigned> winners(nns.size());
    std::iota(winners.begin(),winners.end(),0);
    sort(winners.begin(),winners.end(),[&nns](unsigned i, unsigned j){
      return nns[i].wins > nns[j].wins
             || (!(nns[j].wins > nns[i].wins) && nns[i].id < nns[j].id);
    });
    for (auto w : winners) {
      cout << nns[w].id <<'('<< nns[w].wins <<") ";
      nns[w].wins = 0;
    }
    cout << endl;

    if (r%10==0) {
      for (size_t i=0; i<num_best; ++i) {
        auto& nnw = nns[winners[i]];

        for (int j=0; j<3; ++j) {
          stringstream ss;
          ss << "nets/" << setfill('0') << setw(3) << r
             <<'_'<< setfill('0') << setw(3) << nnw.id <<'_'<< j << ".nn";
          ofstream file(ss.str());
          (*nnw.nn_arr)[j].save(file,print_card,print_act);
          file.close();
        }
      }
    }

    if (r<num_rounds) {
      for (size_t i=num_best; i<num_nets; ++i)
        nns[winners[i]] = nn_wrap();
    }

  }

  return 0;
}
