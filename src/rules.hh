#ifndef rules_hh
#define rules_hh

#include <iosfwd>
#include <deque>
#include <list>

class network;

class rules {

protected:
  std::deque<int> deck, table;
  network *nn[2];
  std::list<int> hand[2];
  int turn;

  inline int other() const noexcept { return (turn==0 ? 1 : 0); }

public:
  rules(size_t deck_size, network* nn1=nullptr, network* nn2=nullptr);

  virtual void print_card(int card) =0;
  virtual void print_hand(int h);
  virtual void print_table();

  virtual bool is_playable(int action) const =0;
  virtual int play() =0;
  virtual void play_nn() =0;
  virtual void play_human() =0;

  static std::ostream* out;
};

#endif
