#ifndef durak_rules_hh
#define durak_rules_hh

#include "rules.hh"

#include <array>

class network;

class durak: public rules {
public:
  typedef std::array<network,3> nnarr_t;

private:
  int trump_suit;
  unsigned adding;
  std::array<nnarr_t*,2> nn;

  void deal(int h);
  void play_action(int a);
  void play_take();
  bool in_hand(int card) const;

public:
  durak(std::mt19937& rand, nnarr_t* nn1=nullptr, nnarr_t* nn2=nullptr);

  virtual void print_card(int card) override;
  virtual void print_trump();

  virtual bool is_playable(int action) const override;
  virtual int play() override;
  virtual void play_nn() override;
  virtual void play_human() override;

};

#endif
