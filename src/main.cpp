/**********************************************************************/
/*           main function for atpg                */
/*                                                                    */
/*           Author: Bing-Chen (Benson) Wu                            */
/*           last update : 01/21/2018                                 */
/**********************************************************************/

#include "atpg.h"

void usage();

int main(int argc, char *argv[]) {
  string inpFile, vetFile, logFile;
  int i, j;
  ATPG atpg; // create an ATPG obj, named atpg

  atpg.timer(stdout, "START");
  atpg.detected_num = 1;
  i = 1;

/* parse the input switches & arguments */
  while (i < argc) {
    // number of test generation attempts for each fault.  used in podem.cpp
    if (strcmp(argv[i], "-anum") == 0) {
      atpg.set_total_attempt_num(atoi(argv[i + 1]));
      i += 2;
    } else if (strcmp(argv[i], "-bt") == 0) {
      atpg.set_backtrack_limit(atoi(argv[i + 1]));
      i += 2;
    } else if (strcmp(argv[i], "-fsim") == 0) {
      vetFile = string(argv[i + 1]);
      atpg.set_fsim_only(true);
      i += 2;
    } else if (strcmp(argv[i], "-tdfsim") == 0) {
      vetFile = string(argv[i + 1]);
      atpg.set_tdfsim_only(true);
      i += 2;
    }

    // for genFailLog ///diag
    else if (strcmp(argv[i], "-genFailLog") == 0) {
      vetFile = string(argv[i + 1]);
      inpFile = string(argv[i + 2]);
      atpg.set_diag_only(true);
      i += 3;

      //int df_num= 0;
      while(strcmp(argv[i], "-fault") == 0){
        //cout << "sdfhjg" << endl;
        vector<string> row;
        row.push_back(argv[i+1]);
        row.push_back(argv[i+2]);
        row.push_back(argv[i+3]);
        row.push_back(argv[i+4]);
        atpg.diag_flist.push_back(row);
        i += 5;
        if(i >= argc)
          break;
      }
      //<< atpg.diag_flist[1][3]
      //cout << atpg.diag_flist[1][3] << atpg.diag_flist[0][3] << " jdfhg" << endl;
      //cout << "@@@@@@@@@@@@fhg" << endl;
      // if(atpg.get_diag_only())
      //   cout << inpFile << endl;

      continue;
    }

    // for -diag //diag
    else if (strcmp(argv[i], "-diag") == 0){
      vetFile = string(argv[i + 1]);
      inpFile = string(argv[i + 2]);
      logFile = string(argv[i + 3]);
      atpg.set_diag_diag(true); 
      /* need fault collapse for simplicity */
      i += 4;

      continue;
    }

      // for N-detect fault simulation
    else if (strcmp(argv[i], "-ndet") == 0) {
      atpg.detected_num = atoi(argv[i + 1]);
      i += 2;
    } else if (argv[i][0] == '-') {
      j = 1;
      while (argv[i][j] != '\0') {
        if (argv[i][j] == 'd') {
          j++;
        } else {
          fprintf(stderr, "atpg: unknown option\n");
          usage();
        }
      }
      i++;
    } else {
      inpFile = string(argv[i]);
      i++;
    }
  }

/* an input file was not specified, so describe the proper usage */
  if (inpFile.empty()) { usage(); }

/* read in and parse the input file */
  atpg.input(inpFile); // input.cpp

/* if vector file is provided, read it */
  if (!vetFile.empty()) { atpg.read_vectors(vetFile); }
  if (atpg.get_diag_only() ) { 
    atpg.level_circuit();  // level.cpp
    atpg.rearrange_gate_inputs();  //level.cpp
    atpg.create_dummy_gate(); //init_flist.cpp
    atpg.generate_tdfault_list(); 
    atpg.gen_FailLog();
    exit(EXIT_SUCCESS);}

  if(atpg.get_diag_diag()){
    atpg.level_circuit();  // level.cpp
    atpg.rearrange_gate_inputs();  //level.cpp
    atpg.create_dummy_gate(); //init_flist.cpp
    atpg.generate_fault_list();
    atpg.read_failLog(logFile);
    atpg.test(); 
    atpg.timer(stdout, "runtimes: ");
    // atpg.gen_FailLog();
    exit(EXIT_SUCCESS);
  }
    
  atpg.timer(stdout, "for reading in circuit");

  atpg.level_circuit();  // level.cpp
  atpg.timer(stdout, "for levelling circuit");

  atpg.rearrange_gate_inputs();  //level.cpp
  atpg.timer(stdout, "for rearranging gate inputs");

  atpg.create_dummy_gate(); //init_flist.cpp
  atpg.timer(stdout, "for creating dummy nodes");

  if (atpg.get_tdfsim_only() || atpg.get_diag_only()) atpg.generate_tdfault_list(); //init_flist.cpp
  else atpg.generate_fault_list();
  atpg.timer(stdout, "for generating fault list");

  if(atpg.get_diag_diag()) { atpg.read_failLog(logFile); }

  atpg.test(); //atpg.cpp
  if (!atpg.get_tdfsim_only() )atpg.compute_fault_coverage(); //init_flist.cpp
  atpg.timer(stdout, "for test pattern generation");
  exit(EXIT_SUCCESS);
}

void usage() {

  fprintf(stderr, "usage: atpg [options] infile\n");
  fprintf(stderr, "Options\n");
  fprintf(stderr, "    -fsim <filename>: fault simulation only; filename provides vectors\n");
  fprintf(stderr, "    -anum <num>: <num> specifies number of vectors per fault\n");
  fprintf(stderr, "    -bt <num>: <num> specifies number of backtracks\n");
  ///diag
  fprintf(stderr, "    -genFailLog <pattern file>  <circuit file> -fault <wire> <gate> <io> <fault type>: diagnosis mode\n");
  ///
  exit(EXIT_FAILURE);

} /* end of usage() */




void ATPG::set_fsim_only(const bool &b) {
  this->fsim_only = b;
}

void ATPG::set_tdfsim_only(const bool &b) {
  this->tdfsim_only = b;
}

void ATPG::set_total_attempt_num(const int &i) {
  this->total_attempt_num = i;
}

void ATPG::set_backtrack_limit(const int &i) {
  this->backtrack_limit = i;
}

///diag
void ATPG::set_diag_only(const bool &b){
  this->diag_only = b;
}

void ATPG::set_diag_diag(const bool &b){
  this->diag_diag = b;
}
///
