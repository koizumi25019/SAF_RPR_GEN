#include "ccadical.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
int main(int argc, char **argv) {
  if (argc != 3) return 2;
  const int npi = std::stoi(argv[2]);
  CCaDiCaL *s = ccadical_init();
  ccadical_set_option(s, "quiet", 1);
  int declared = 0;
  std::ifstream in(argv[1]);
  if (!in) return 3;
  std::string line;
  while (std::getline(in,line)) {
    if (line.empty() || line[0]=='c') continue;
    if (line[0]=='p') {
      std::istringstream hdr(line); std::string p,cnf; int nc;
      hdr>>p>>cnf>>declared>>nc;
      ccadical_declare_more_variables(s,declared);
      continue;
    }
    std::istringstream ls(line); int lit;
    while(ls>>lit) ccadical_add(s,lit);
  }
  for (int i=1;i<=npi;i++) ccadical_freeze(s,i);
  while (std::getline(std::cin,line)) {
    std::istringstream ls(line); char cmd; ls>>cmd;
    if(cmd=='q') break;
    int lit; std::vector<int> lits;
    while(ls>>lit && lit) {
      if(std::abs(lit)>declared) {
        ccadical_declare_more_variables(s,std::abs(lit)-declared);
        declared=std::abs(lit);
      }
      lits.push_back(lit);
    }
    if(cmd=='a') {
      for(int x:lits) ccadical_add(s,x);
      ccadical_add(s,0);
    } else if(cmd=='s') {
      for(int x:lits) ccadical_assume(s,x);
      const int rc=ccadical_solve(s); std::cout<<rc;
      if(rc==10) for(int i=1;i<=npi;i++) std::cout<<' '<<(ccadical_val(s,i)>0?i:-i);
      else if(rc==20) for(int x:lits) if(ccadical_failed(s,x)) std::cout<<' '<<x;
      std::cout<<std::endl;
    }
  }
  ccadical_release(s);
}
