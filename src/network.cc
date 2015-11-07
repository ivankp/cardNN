#include "network.hh"

#include <iostream>
#include <iomanip>
// #include <sstream>
#include <string>
#include <vector>
#include <random>
#include <stdexcept>
#include <cmath>
// #include <chrono>

/*
#define GCC_VERSION (__GNUC__ * 10000 \
                              + __GNUC_MINOR__ * 100 \
                              + __GNUC_PATCHLEVEL__)

#if GCC_VERSION < 40900 // Test for GCC < 4.9.0
#include <boost/regex.hpp>
using boost::regex;
using boost::smatch;
using boost::regex_match;
#define regex_icase boost::regex::icase
#else
#include <regex>
using std::regex;
using std::smatch;
using std::regex_match;
using std::regex_constants::icase;
#define regex_icase std::regex_constants::icase
#endif
*/

#define test(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

using std::cout;
using std::cerr;
using std::endl;
using std::setw;
using std::setprecision;
using std::fixed;
using std::string;
using std::to_string;
using std::pair;
using std::vector;
using std::runtime_error;

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

  out << "states [label=\"";
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

  out << "actions [label=\"";
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

network::network(std::istream& in) {
  string dot(std::istreambuf_iterator<char>(in), {});

  for (auto& nodes : {
    std::make_pair("trump",&trumps),
    std::make_pair("plain",&plains)
  }) {

    int i1 = 0;
    size_t pos = 0;
    while ((pos=dot.find(nodes.first+to_string(i1++),pos))!=string::npos) {
      nodes.second->emplace_back();

      const size_t l = dot.find("[label=\"{",pos)+9;
      const size_t r = dot.find("}\"];",pos);

      if (l==string::npos || r==string::npos)
        throw runtime_error("Badly formatted label");

      string label = dot.substr(l,r-l);

      size_t pos2 = 0;
      while((pos2=label.find("\\\n",pos2))!=string::npos)
        label.erase(pos2,2);
      test(label)

      int i2 = 0;
      pos2 = 0;
      string i2s;
      while ((pos2=label.find(i2s='<'+to_string(i2++)+'>',pos2))!=string::npos) {
        const size_t l = pos2+i2s.size();
        const size_t r = label.find('|',l);
        nodes.second->back().push_back({std::stof(label.substr(l,r-l))});
        pos2 = r+1;
      }

      for (auto& t : nodes.second->back()) test(t.field)
    }

    if (nodes.second->size()==0)
      throw runtime_error(string("No ")+nodes.first+" nodes in NN file");

  }

  {
    size_t pos = dot.find("states");
    if (pos!=string::npos) {

      const size_t l = dot.find("[label=\"",pos)+8;
      const size_t r = dot.find("\"];",pos);

      if (l==string::npos || r==string::npos)
        throw runtime_error("Badly formatted label");

      string label = dot.substr(l,r-l);

      size_t pos2 = 0;
      while((pos2=label.find("\\\n",pos2))!=string::npos)
        label.erase(pos2,2);
      test(label)

      int i2 = 0;
      pos2 = 0;
      string i2s;
      while ((pos2=label.find(i2s='<'+to_string(i2++)+'>',pos2))!=string::npos) {
        pos2 = label.find('|',pos2+i2s.size())+1;
      }

      if (i2) states.resize(i2-1);
      else throw runtime_error("Empty states label in NN file");
      test(states.size())

    } else throw runtime_error("No states nodes in NN file");

  }
}
