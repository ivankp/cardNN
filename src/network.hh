#ifndef solaris_neural_net_hh
#define solaris_neural_net_hh

#include <iosfwd>
#include <vector>

class network {
  typedef float val_t;

  struct node { val_t field; };
  std::vector<std::vector<node>> trumps, plains;

  struct state: public node {
    std::vector<node> *fields;
  };
  std::vector<state> states;

  struct neuron: public node {
    std::vector<val_t> weights;
  };
  std::vector<std::vector<neuron>> layers;

public:
  network(unsigned ncards, // cards in deck (must be a multiple of 4)
          unsigned nts,    // trump states
          unsigned nps,    // plain states
          const std::vector<unsigned>& nlayers // hidden neuron layers
  );

  void reset(unsigned trump_suit) noexcept;
  void set_state(unsigned card, unsigned state) noexcept;
  void eval() noexcept;
  val_t get_field(unsigned action) noexcept;
  void save(std::ostream& out,
            const char*(*card)(int),
            const char*(*act)(int));
};

#endif
