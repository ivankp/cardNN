#include <iostream>
#include <fstream>
#include <cstring>

#include "network.hh"

using namespace std;

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

#define ncards 8

const char* print_act(int act) {
  if (act  < ncards) return print_card(act);
  if (act == ncards) return "done";
  if (act == ncards+1) return "take";
  return "?";
}

int main(int argc, char** argv)
{
  network nn(ncards,4,3,{5,4,3,4,5,ncards+2});

  ofstream file(argv[1]);
  nn.save(file,print_card,print_act);
  file.close();

  return 0;
}
