#include "network.hh"

#include <iostream>
#include <iomanip>
#include <random>
#include <cmath>
// #include <chrono>

using namespace std;

network::network(unsigned ncards, // cards in deck (must be a multiple of 4)
                 unsigned nts,    // trump states
                 unsigned nps,    // plain states
                 const vector<unsigned>& nlayers // hidden neuron layers
): trumps(ncards/4,vector<node>(nts)),
   plains(ncards/4,vector<node>(nps)),
   states(ncards),
   layers(nlayers.size())
{
  // std::mt19937 gen(std::chrono::system_clock::now().time_since_epoch().count());
  std::mt19937 gen{std::random_device{}()};
  std::uniform_real_distribution<float> dist(-2.,2.);

  for (auto& nodes : trumps)
    for (auto& x : nodes) x.field = dist(gen);

  for (auto& nodes : plains)
    for (auto& x : nodes) x.field = dist(gen);

  for (size_t l=0, nl=layers.size(); l<nl; ++l) {
    layers[l].resize(nlayers[l]);
    for (auto& x : layers[l]) { // loop over neurons
      if (l==0) x.weights.resize(ncards);
      else x.weights.resize(nlayers[l-1]);
      for (auto& w : x.weights) w = dist(gen);
    }
  }
}

void network::reset_states(unsigned trump_suit) noexcept {
  for (size_t i=0, n=states.size(); i<n; ++i)
    states[i].field = (*(
      states[i].fields = (i%4==trump_suit) ? &plains[i/4] : &trumps[i/4]
    ))[0].field;
}

void network::eval() noexcept {
  for (size_t i=0, n=layers[0].size(); i<n; ++i) {
    auto& field = layers[0][i].field;
    field = 0.;
    for (auto s : states) field += s.field;
    field = tanh(field);
  }
  for (size_t l=1, nl = layers.size(); l<nl; ++l) {
    for (size_t i=0, n=layers[l].size(); i<n; ++i) {
      auto& field = layers[l][i].field;
      field = 0.;
      for (size_t w=0, nw=layers[l][i].weights.size(); w<nw; ++w)
        field += layers[l-1][w].field*layers[l][i].weights[w];
      field = tanh(field); // apply transfer function
    }
  }

  for (auto x : states) cout << x.field << endl;
  cout << endl;
  for (auto x : layers.back()) cout << x.field << endl;
}

void network::save(std::ostream& out,
                   const char*(*card)(int), const char*(*act)(int))
{
  const auto flags = out.flags();
  out << fixed << setprecision(5);

  out <<
  "digraph G {\n"
  "nodesep=.05;\n"
  "rankdir=LR;\n"
  "node [shape=record,width=.1,height=.1];\n\n";

  for (size_t i=0, ni=trumps.size(); i<ni; ++i) {
    out << "trump" << i << " [label=\"{";
    for (size_t j=0, nj=trumps[i].size(); j<nj; ++j) {
      if (j) out << '|';
      if (nj>4 && j%4==0) out << "\\\n";
      out << '<' << j << "> " << setw(8) << trumps[i][j].field;
    }
    out << "}\"];\n";
  }
  out << '\n';

  for (size_t i=0, ni=plains.size(); i<ni; ++i) {
    out << "plain" << i << " [label=\"{";
    for (size_t j=0, nj=plains[i].size(); j<nj; ++j) {
      if (j) out << '|';
      if (nj>4 && j%4==0) out << "\\\n";
      out << '<' << j << "> " << setw(8) << plains[i][j].field;
    }
    out << "}\"];\n";
  }
  out << '\n';

  out << "states [label =\"";
  for (size_t i=0, n=states.size(); i<n; ++i) {
    if (i) out << '|';
    if (i%4==0) out << "\\\n";
    out << '<' << i << "> " << card(i);
  }
  out << "\"];\n\n";

  for (size_t l=0, nl=layers.size()-1; l<nl; ++l) {
    out << "layer" << l << " [label=\"";
    for (size_t j=0, nj=layers[l].size(); j<nj; ++j) {
      if (j) out << '|';
      if (nj>10 && j%10==0) out << "\\\n";
      out << '<' << j << '>';
    }
    out << "\"];\n";
  }
  out << '\n';

  out << "actions [label =\"";
  for (size_t i=0, n=layers.back().size(); i<n; ++i) {
    if (i) out << '|';
    if (i%4==0) out << "\\\n";
    out << '<' << i << "> " << act(i);
  }
  out << "\"];\n\n";

  for (size_t i=0, n=states.size(); i<n; ++i) {
    if (i%4) out << "plain";
    else     out << "trump";
    out << i/4 << " -> states:" << i << ";\n";
  }
  out << '\n';

  for (size_t l=0, nl=layers.size(); l<nl; ++l) { // layers
    const auto& layer = layers[l];
    for (size_t i=0, ni=layer.size(); i<ni; ++i) { // nodes
      for (size_t w=0, nw=layer[i].weights.size(); w<nw; ++w) { // weights
        if (l) out << "layer" << l-1;
        else out << "states";
        out << ':' << w << " -> ";
        if (l==nl-1) out << "actions";
        else out << "layer" << l;
        out << ':';
        out << i << " [weight=" << setw(8) << layer[i].weights[w] << "];\n";
      }
      out << '\n';
    }
  }

  out << '}';
  out.flags(flags);
}

network::network(const char* filename) {

}
