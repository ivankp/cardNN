#ifndef durak_rules_hh
#define durak_rules_hh

#include "rules.hh"

class durak: public rules {

  int trump_suit;

  void deal(int h);
  void play_action(int a);
  bool in_hand(int card) const;

public:
  durak(network* nn1=nullptr, network* nn2=nullptr);

  virtual void print_card(int card) override;
  virtual void print_trump();

  virtual bool is_playable(int action) const override;
  virtual int play() override;
  virtual void play_nn() override;
  virtual void play_human() override;

};

#endif
