#include "network.hh"

#include <iostream>
#include <iomanip>
#include <string>
#include <stdexcept>
#include <cmath>

#define test(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

using namespace std;

network::network(unsigned ncards, // cards in deck (must be a multiple of 4)
                 unsigned nts,    // trump states
                 unsigned nps,    // plain states
                 const vector<unsigned>& nlayers, // hidden neuron layers
                 weight_dist_t& weight_dist
): trumps(ncards/4,vector<node>(nts)),
   plains(ncards/4,vector<node>(nps)),
   states(ncards),
   layers(nlayers.size())
{
  for (auto& nodes : trumps)
    for (auto& x : nodes) x.field = weight_dist();

  for (auto& nodes : plains)
    for (auto& x : nodes) x.field = weight_dist();

  for (size_t l=0, nl=layers.size(); l<nl; ++l) {
    layers[l].resize(nlayers[l]);
    for (auto& x : layers[l]) { // loop over neurons
      if (l==0) x.weights.resize(ncards);
      else x.weights.resize(nlayers[l-1]);
      for (auto& w : x.weights) w = weight_dist();
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
    for (const auto& s : states) field += s.field;
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

/*
void replace_all(string& str, const string& from, const string& to) {
  size_t pos = 0;
  while((pos=label.find(from,pos))!=string::npos)
    label.replace(pos,from.size(),to);
}
*/

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

      int i2 = 0;
      pos2 = 0;
      string i2s;
      while ((pos2=label.find(i2s='<'+to_string(i2++)+'>',pos2))!=string::npos) {
        const size_t l = pos2+i2s.size();
        const size_t r = label.find('|',l);
        nodes.second->back().push_back({std::stof(label.substr(l,r-l))});
        pos2 = r+1;
      }
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

      int i2 = 0;
      pos2 = 0;
      string i2s;
      while ((pos2=label.find(i2s='<'+to_string(i2++)+'>',pos2))!=string::npos) {
        pos2 = label.find('|',pos2+i2s.size())+1;
      }

      if (i2) states.resize(i2-1);
      else throw runtime_error("Empty states label in NN file");

    } else throw runtime_error("No states nodes in NN file");
  }

  {
    int i1 = 0;
    size_t pos = 0;
    string layer_str;
    while ((pos=dot.find(layer_str="layer"+to_string(i1++),pos))!=string::npos) {
      layers.emplace_back();

      const size_t l = dot.find("[label=\"",pos)+8;
      const size_t r = dot.find("\"];",pos);

      if (l==string::npos || r==string::npos)
        throw runtime_error("Badly formatted label");

      string label = dot.substr(l,r-l);

      size_t pos2 = 0;
      while((pos2=label.find("\\\n",pos2))!=string::npos)
        label.erase(pos2,2);

      int i2 = 0;
      pos2 = 0;
      string i2s;
      while ((pos2=label.find(i2s='<'+to_string(i2++)+'>',pos2))!=string::npos) {
        pos2 = label.find('|',pos2+i2s.size())+1;
      }

      if (i2) layers.back().resize(i2-1);
      else throw runtime_error("Empty "+layer_str+" label in NN file");
    }
  }

  {
    layers.emplace_back();

    size_t pos = dot.find("actions");
    if (pos!=string::npos) {

      const size_t l = dot.find("[label=\"",pos)+8;
      const size_t r = dot.find("\"];",pos);

      if (l==string::npos || r==string::npos)
        throw runtime_error("Badly formatted label");

      string label = dot.substr(l,r-l);

      size_t pos2 = 0;
      while((pos2=label.find("\\\n",pos2))!=string::npos)
        label.erase(pos2,2);

      int i2 = 0;
      pos2 = 0;
      string i2s;
      while ((pos2=label.find(i2s='<'+to_string(i2++)+'>',pos2))!=string::npos) {
        pos2 = label.find('|',pos2+i2s.size())+1;
      }

      if (i2) layers.back().resize(i2-1);
      else throw runtime_error("Empty actions label in NN file");

    } else throw runtime_error("No actions nodes in NN file");
  }

  {
    vector<string> names { "states" };
    for (size_t i=0; i<layers.size(); ++i)
      names.emplace_back("layer"+to_string(i));
    names.emplace_back("actions");

    size_t pos = 0;
    for (size_t i1=0, n1=layers.size(); i1!=n1; ++i1) {
      for (size_t i2=0, n2=layers[i1].size(); i2!=n2;) {
        auto& w = layers[i1][i2].weights;
        size_t pos2 = dot.find(
          ':' + to_string(w.size()) + " -> "
          + ( i1==n1-1 ? string("actions") : "layer"+to_string(i1) )
          + ':' + to_string(i2),
          pos
        );
        if (pos2==string::npos) {
          ++i2;
          continue;
        } else {
          pos = pos2;
        }
        pos = dot.find('[',pos);
        pos = dot.find("weight",pos);
        pos2 = dot.find("=",pos)+1;
        w.push_back(stof(dot.substr(pos2,(pos=dot.find(']',pos2))-pos2)));
      }
    }

    // check that all layers got weights
    for (size_t l=0, nl=layers.size(); l<nl; ++l) {
      for (auto& x : layers[l]) {
        if (x.weights.size() != (l==0 ? states.size() : layers[l-1].size()) )
          throw runtime_error("Number of weights does not match the "
                              "number of preceding layers");
      }
    }

  }
}
