#ifndef neural_net_hh
#define neural_net_hh

#include <iosfwd>
#include <vector>
#include <functional>

class network {
public:
  typedef float val_t;
  typedef std::function<val_t()> weight_dist_t;

private:
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
  // Generate a new network with random weights
  network(unsigned ncards, // cards in deck (must be a multiple of 4)
          unsigned nts,    // trump states
          unsigned nps,    // plain states
          const std::vector<unsigned>& nlayers, // hidden neuron layers
          weight_dist_t& weight_dist
  );

  // Read a network from a dot file
  network(std::istream& in);

  // Reset cards' states
  // Call at the beginning of every game
  void reset_states(unsigned trump_suit) noexcept;

  // Compute fields
  // Call before getting new output fields' values
  void eval() noexcept;

  inline void set_state(unsigned card, unsigned state) noexcept {
    states[card].field = (*states[card].fields)[state].field;
  }
  inline val_t get_field(unsigned action) const noexcept {
    return layers.back()[action].field;
  }

  void save(std::ostream& out,
            const char*(*card)(int),
            const char*(*act)(int));
};

#endif
