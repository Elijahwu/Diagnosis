#include "atpg.h"
#include <map>
#include <bitset>
#include <algorithm>    // std::sort

unordered_map<int, int> ptnIdx_to_vecIdx;

void ATPG::gen_FailLog(){
      diagnosis_map_insert(); //ssf_diag
      diagnosis_fault_insert();
      diag_fsim_vectors();
      return;
}

void ATPG::diagnosis_fault_insert(){

    // //insert map
    // unordered_map<string, int> wire2index;
    // unordered_map<string, int> type2int;
    // wptr w;
    // for (auto pos : sort_wlist){
    // w = pos;
    // wire2index[w->name.substr(0,w->name.find("("))] = w->wlist_index;
    // }
    // type2int["GI"] = 0;
    // type2int["GO"] = 1;
    // type2int["SA0"] = 0;
    // type2int["SA1"] = 1;


    //cout << diag_flist[1][3] << diag_flist[0][3] << " jdfhg" << endl;
    fptr f;
    // bool get_fault = false;
    for (auto pos = flist_undetect.cbegin(); pos != flist_undetect.cend(); ++pos) {
    //int fault_detected[num_of_faults_in_parallel] = {0}; //for n-det
    f = *pos;
    //     cout << f->fault_no << " " << f->io << " "  << f->node << " " << sort_wlist[f->to_swlist]->name << " " << f->to_swlist << " " << f->index << " " << f->node->name << endl;
    for(int i=0;i<diag_flist.size();i++){
        // if( wire2index[diag_flist[i][0]]==f->to_swlist && nfind(diag_flist[i][1])==f->node && type2int[diag_flist[i][2]]==f->io && type2int[diag_flist[i][3]]==f->fault_type ){
        if( wfind(diag_flist[i][0])==sort_wlist[f->to_swlist] && nfind(diag_flist[i][1])==f->node && type2int[diag_flist[i][2]]==f->io && type2int[diag_flist[i][3]]==f->fault_type ){
        
            f->diag_flag=true;
            // get_fault = true;
        }
        wptr w = wfind(diag_flist[i][0]);
        // w->print();
        // if(wfind(diag_flist[i][0])==sort_wlist[f->to_swlist])
        // if(wire2index[diag_flist[i][0]]==f->to_swlist)
        // {cout << f->to_swlist << endl;
        // cout << sort_wlist[f->to_swlist]->name << endl;
        // cout << sort_wlist[f->to_swlist]->inode[0]->name<< endl;
        // cout << sort_wlist[f->to_swlist]->onode[0]->name<< endl;
        // cout << f->node->name << endl;
        // cout << nfind(diag_flist[i][1])->name << endl;
        // cout << f->io<< endl;
        // for(auto s: diag_flist[i]){
        //   cout << s <<" ";
        // }
        // cout << wire2index[diag_flist[i][0]]<<endl;
        // cout << endl;
        // }
    }

    }
    // cout <<"get fault: "<<get_fault<<endl;

    return;
}
//#define num_of_faults_in_parallel  16
void ATPG::diag_fsim_vectors(){
    int i;
    for (i = vectors.size()-1; i >= 0; i--) {
    diag_fsim_a_vector(vectors[i],i);
    }
    return;
}

/* fault simulate a single test vector */
void ATPG::diag_fsim_a_vector(const string &vec, int vector_index) 
{
  wptr w, faulty_wire;
  /* array of 16 fptrs, which points to the 16 faults in a simulation packet  */
  //fptr simulated_fault_list[num_of_faults_in_parallel];
  fptr f;
  int fault_type;
  int i, start_wire_index, nckt;
  int num_of_fault;

  num_of_fault = 0; // counts the number of faults in a packet
  /* num_of_current_detect is used to keep track of the number of undetected faults
   * detected by this vector.  Initialize it to zero */
  //num_of_current_detect = 0;

  /* Keep track of the minimum wire index of 16 faults in a packet.
   * the start_wire_index is used to keep track of the
   * first gate that needs to be evaluated.
   * This reduces unnecessary check of scheduled events.*/
  start_wire_index = 10000;

  /* for every input, set its value to the current vector value */
  for (i = 0; i < cktin.size(); i++) {
    cktin[i]->value = ctoi(vec[i]);
  }

  /* initialize the circuit - mark all inputs as changed and all other
   * nodes as unknown (2) */
  nckt = sort_wlist.size();
  for (i = 0; i < nckt; i++) {
    if (i < cktin.size()) {
      sort_wlist[i]->set_changed();
    } else {
      sort_wlist[i]->value = U;
    }
  }

  sim(); /* do a fault-free simulation, see sim.cpp */
  if (debug) { display_io(); }

  /* expand the fault-free value into 32 bits (00 = logic zero, 11 = logic one, 01 = unknown)
   * and store it in wire_value1 (good value) and wire_value2 (faulty value)*/
  for (i = 0; i < nckt; i++) {
    switch (sort_wlist[i]->value) {
      case 1:
        sort_wlist[i]->wire_value1 = ALL_ONE;  // 11 represents logic one
        sort_wlist[i]->wire_value2 = ALL_ONE;
        break;
      case 2:
        sort_wlist[i]->wire_value1 = 0x55555555; // 01 represents unknown
        sort_wlist[i]->wire_value2 = 0x55555555;
        break;
      case 0:
        sort_wlist[i]->wire_value1 = ALL_ZERO; // 00 represents logic zero
        sort_wlist[i]->wire_value2 = ALL_ZERO;
        break;
    }
  } // for in

 /// delete GI fault whose gate output with a fault
  for (auto pos = flist_undetect.cbegin(); pos != flist_undetect.cend(); ++pos) {
    f = *pos;
    if(f->diag_flag && f->io==GI){
      for (auto pos2 = flist_undetect.cbegin(); pos2 != flist_undetect.cend(); ++pos2) {
        fptr f2 = *pos2;
        if(f2->diag_flag && f->node->owire[0]==sort_wlist[f2->to_swlist] && f2->io==GO) f->diag_flag=false;
      }
    }
  } 
 ///create map <fault, wire_index>
  unordered_map<fptr, int> fault2level_vec;
  // vector<int> level_fault;
  // map<int,fptr> wire2fault;
  map<int,list<fptr>>wire2fault;
  // level_fault.clear();
  for (auto pos = flist_undetect.cbegin(); pos != flist_undetect.cend(); ++pos) {
    f = *pos;
    if(!f->diag_flag) continue;

    int f_level = (f->io==GO)?f->to_swlist:f->node->owire[0]->wlist_index;
    fault2level_vec[f] = f_level;

    // int isp=0;
    // for(int i=0;i<level_fault.size();i++){
    //   if(level_fault[i] < f_level) isp=i+1;
    // }
    // if(isp<level_fault.size())
    //   level_fault.insert(level_fault.begin()+isp,f_level);
    // else
    //   level_fault.push_back(f_level);
    if(wire2fault.find(f_level) == wire2fault.end()){
      // wire2fault[f_level] = f;
      list<fptr> temp_list; 
      temp_list.push_back(f);
      wire2fault[f_level] = temp_list;
    }
    else{
      //2 input fault
      // cerr<<"2 GI fault" << endl;
      wire2fault[f_level].push_back(f);
    }
  }

  // for (auto pos = flist_undetect.cbegin(); pos != flist_undetect.cend(); ++pos) {
  //   f = *pos;
  //   cout << f->to_swlist << endl;
  // }
  // for(int i=0;i<level_fault.size();i++){
  //   cout << level_fault[i] << endl;
  // }

 ///
    // for (auto& it: fault2level_vec) {
    //   cout << fault2level_vec[it.first] << " " << sort_wlist[it.first->to_swlist]->name << " " << it.first->io << endl;
    // }

  ///////////////////

  /* walk through every undetected fault
   * the undetected fault list is linked by pnext_undetect */
 ///multi-fsim start 
  // for (int lf_i=0;lf_i<level_fault.size();lf_i++) {
  // cout <<" fault size"<<wire2fault.size()<<endl;
  for (auto it:wire2fault){
    //int fault_detected[num_of_faults_in_parallel] = {0}; //for n-det
      // cout << "lf_i is  "<< lf_i<<endl;

    // for (auto& it: fault2level_vec) {
    //   if(it.second==level_fault[lf_i]) {
    //     f=it.first; 
    //     // cout << sort_wlist[f->to_swlist]->name << " " << f->io << " " << level_fault[lf_i] << " " << fault2level_vec[f] << endl;
    //   }
    // }
    list<fptr> sim_flist = it.second;
    f = sim_flist.front();
    // f = it.second;
    ////////// in
    ////////////
    // f = *pos;
    //if (f->detect == REDUNDANT) { continue; } /* ignore redundant faults */
    
    // if(!f->diag_flag) continue;

    /* if f is an gate output fault */
    // if(f->io == GI) cout <<"fkjdgdurh";
    if (f->io == GO) {

      /* if this wire is not yet marked as faulty, mark the wire as faulty
        * and insert the corresponding wire to the list of faulty wires. */
      if (!(sort_wlist[f->to_swlist]->is_faulty())) {
        // cout<<endl<<"faulty"<<endl;
        sort_wlist[f->to_swlist]->set_faulty();
        wlist_faulty.push_front(sort_wlist[f->to_swlist]);
      }

      /* add the fault to the simulated fault list and inject the fault */
      //simulated_fault_list[num_of_fault] = f;
      inject_fault_value(sort_wlist[f->to_swlist], num_of_fault, f->fault_type);

      /* mark the wire as having a fault injected
        * and schedule the outputs of this gate */
      sort_wlist[f->to_swlist]->set_fault_injected();
      
      for (auto pos_n : sort_wlist[f->to_swlist]->onode) {
          ///diag
          if(pos_n->type != OUTPUT)
            pos_n->owire.front()->set_scheduled();
        //   else
        //     sort_wlist[f->to_swlist]->diag_flag = true;
      }

      /* increment the number of simulated faults in this packet */
      //num_of_fault++;
      /* start_wire_index keeps track of the smallest level of fault in this packet.
        * this saves simulation time.  */
      start_wire_index = f->to_swlist; ////
    }  // if gate output fault

      /* the fault is a gate input fault */
    else {

      /* if the fault is propagated, set faulty_wire equal to the faulty wire.
        * faulty_wire is the gate output of f.  */
      if(sim_flist.size() > 1){
        faulty_wire = diag_GI_get_faulty_wire(sim_flist,fault_type);
      }
      else {
        faulty_wire = diag_get_faulty_wire(f, fault_type);

      }
      if (faulty_wire != nullptr) {
  ///
  // cout << faulty_wire->wlist_index << " " << fault_type << " " << vector_index << endl;
  ///
          /* if faulty_wire is not already marked as faulty, mark it as faulty
            * and add the wire to the list of faulty wires. */
          if (!(faulty_wire->is_faulty())) {
            faulty_wire->set_faulty();
            wlist_faulty.push_front(faulty_wire);
          }

          /* add the fault to the simulated list and inject it */
          //simulated_fault_list[num_of_fault] = f;
          inject_fault_value(faulty_wire, num_of_fault, fault_type);

          /* mark the faulty_wire as having a fault injected
            *  and schedule the outputs of this gate */
          faulty_wire->set_fault_injected();
          for (auto pos_n : faulty_wire->onode) {
            ///diag
            if(pos_n->type != OUTPUT)
                pos_n->owire.front()->set_scheduled();
          }

          //num_of_fault++;
          start_wire_index = fault2level_vec[f]; ////

      }
      else{
        start_wire_index = f->to_swlist;
      }
      // start_wire_index = fault2level_vec[f];
    }



    //////// simulation after insert fault
    
      for (i = start_wire_index; i < nckt; i++) {
        if (sort_wlist[i]->is_scheduled()) {
          sort_wlist[i]->remove_scheduled();
          diag_fault_sim_evaluate(sort_wlist[i]);
        }
      } /* event evaluations end here */

      // for(auto w: wlist_faulty){
      //     if(w->is_fault_injected()){
      //     cout << "has fault: "<<w->name <<endl;
      //   }
      //   else 
      //     w->remove_faulty();

      // }
      // while (!wlist_faulty.empty()) {
      //   w = wlist_faulty.front();
      //   wlist_faulty.pop_front();
      //   w->remove_faulty();
      //   if(w->is_fault_injected()){
      //     cout << "has fault: "<<w->name <<endl;
      //   }
      //   // w->remove_fault_injected();
      //   // w->set_fault_free();

      //   }
      //   //w->wire_value2 = w->wire_value1;  // reset to fault-free values
      //   /* end TODO*/
      // } // pop out all faulty wires
    // // /////////////


  } 


      /* starting with start_wire_index, evaulate all scheduled wires
       * start_wire_index helps to save time. */
      // for (i = start_wire_index; i < nckt; i++) {
      //   if (sort_wlist[i]->is_scheduled()) {
      //     sort_wlist[i]->remove_scheduled();
      //     fault_sim_evaluate(sort_wlist[i]);
      //   }
      // } /* event evaluations end here */

      while (!wlist_faulty.empty()) {
        w = wlist_faulty.front();
        wlist_faulty.pop_front();
        w->remove_faulty();
        w->remove_fault_injected();
        w->set_fault_free();

      }
      //   //w->wire_value2 = w->wire_value1;  // reset to fault-free values
      //   /* end TODO*/
      // } // pop out all faulty wires
 ///if value1 != value2, report it
      for (auto pos : sort_wlist){
          w = pos;
          if (w->is_output()) { // if primary output
              if ((w->wire_value2 & Mask[0]) ^ (w->wire_value1 & Mask[0])) { // if value1 != value2
                  if (((w->wire_value2 & Mask[0]) ^ Unknown[0]) && ((w->wire_value1 & Mask[0]) ^ Unknown[0])) { // and not unknowns
                      cout << "vector[" << vector_index << "] " << w->name << " expect ";  //ssf_diag need mod  w->name.substr(0,w->name.find("("))
                      if(w->value==1) cout << "H, observe L  # T'"; else cout << "L, observe H  # T'";
                      for (i = 0; i < cktin.size(); i++) {
                          cout << vec[i];
                      }
                      cout << "' " << endl;
                  }
              }
          }
          w->wire_value2 = w->wire_value1;  // reset to fault-free values
      }


      num_of_fault = 0;  // reset the counter of faults in a packet
      start_wire_index = 10000;  //reset this index to a very large value.

}/* end of fault_sim_a_vector */


ATPG::wptr ATPG::diag_get_faulty_wire(const fptr f, int &fault_type) {
  int i, nin;
  bool is_faulty;

  is_faulty = true;
  nin = f->node->iwire.size();
  switch (f->node->type) {

    /* this case should not occur,
     * because we do not create fault in the NOT BUF gate input */


    //////////tdfsim
    case NOT:
      if (f->fault_type == 0)
        fault_type = STF;
      else
        fault_type = STR;
      break;
    case BUF:
      //fprintf(stdout, "something is fishy(get_faulty_net)...\n");
      if (f->fault_type == 0)
        fault_type = STR;
      else
        fault_type = STF;
      break;

      /*check every gate input of AND
       if any input is zero or unknown, then fault f is not propagated */
    case AND:
      for (i = 0; i < nin; i++) {
        if (f->node->iwire[i] != sort_wlist[f->to_swlist]) {
          if ((f->node->iwire[i]->wire_value2 & Mask[0]) ^ (ALL_ONE & Mask[0])) {      ///need compare to faulty value
            is_faulty = false;  // not propagated
          }
        }
      }
      /* AND gate input stuck-at one fault is propagated to
         AND gate output stuck-at one fault */
      if (f->fault_type == 0)
        fault_type = STR;
      else
        fault_type = STF;
      break;

    case NAND:
      for (i = 0; i < nin; i++) {
        if (f->node->iwire[i] != sort_wlist[f->to_swlist]) {
          if ((f->node->iwire[i]->wire_value2 & Mask[0]) ^ (ALL_ONE & Mask[0])) {
            is_faulty = false;
          }
        }
      }
      if (f->fault_type == 0)
        fault_type = STF;
      else
        fault_type = STR;
      break;
    case OR:
      for (i = 0; i < nin; i++) {
        if (f->node->iwire[i] != sort_wlist[f->to_swlist]) {
          if ((f->node->iwire[i]->wire_value2 & Mask[0]) ^ (ALL_ZERO & Mask[0])) {
            is_faulty = false;
          }
        }
      }
      if (f->fault_type == 0)
        fault_type = STR;
      else
        fault_type = STF;
      break;
    case NOR:
      for (i = 0; i < nin; i++) {
        if (f->node->iwire[i] != sort_wlist[f->to_swlist]) {
          if ((f->node->iwire[i]->wire_value2 & Mask[0]) ^ (ALL_ZERO & Mask[0])) {
            is_faulty = false;
          }
        }
      }
      if (f->fault_type == 0)
        fault_type = STF;
      else
        fault_type = STR;
      break;
    case XOR:
      for (i = 0; i < nin; i++) {
        if (f->node->iwire[i] != sort_wlist[f->to_swlist]) {
          if ((f->node->iwire[i]->wire_value2 & Mask[0]) == (ALL_ZERO & Mask[0])) {
            fault_type = f->fault_type;
          } else {
            fault_type = f->fault_type ^ 1;
          }
        }
      }
      break;
    case EQV:
      for (i = 0; i < nin; i++) {
        if (f->node->iwire[i] != sort_wlist[f->to_swlist]) {
          if ((f->node->iwire[i]->wire_value2 & Mask[0]) == (ALL_ZERO & Mask[0])) {
            fault_type = f->fault_type ^ 1;
          } else {
            fault_type = f->fault_type;
          }
        }
      }
      break;
  }
  if (is_faulty) {
    return (f->node->owire.front());
  }
  return (nullptr);
}/* end of get_faulty_wire */

void ATPG::read_failLog(const string &logFile) {
  string s, vec;
  vector<string> strs;
  
  int num_failpo = 0;
  ifstream file(logFile, std::ifstream::in); // open the input vectors' file
  if (!file) { // if the ifstream obj does not exist, fail to open the file
    fprintf(stderr, "File %s could not be opened\n", logFile.c_str());
    exit(EXIT_FAILURE);
  }

  while (!file.eof() && !file.bad()) {

    getline(file, s);
    if(s[0]=='#' || s=="") {continue;}
    else{
      //vector[0]  22GAT  expect L, observe H    #  T'00110'
      FLOG* log = new FLOG;
      
      strs.clear();
      vec.clear();
      size_t pos1 = 0, pos2 = 0;
      while(strs.size()<8){
        pos1 = s.find_first_not_of(" ", pos2);
        pos2 = s.find_first_of(" ", pos1);
        strs.push_back(s.substr(pos1, pos2-pos1));
        // cerr<<s.substr(pos1, pos2-pos1)<<endl;
      }
      
      s = strs[0];
      vec.clear();
      for(char c: s){
        if (c == 'v') continue;  // ignore "v"
        if (c == 'e') continue;  // ignore "e"
        if (c == 'c') continue;  // ignore "c"
        if (c == 't') continue;  // ignore "t"
        if (c == 'o') continue;  // ignore "o"
        if (c == 'r') continue;  // ignore "r"
        if (c == '[') continue;  // ignore "["
        if (c == ']') continue;  // ignore "]"
        vec.push_back(c);
      }
      int ptn_idx = stoi(vec);
      log->failing_index = ptn_idx;
      num_failpo++;
      if(ptnIdx_to_vecIdx.find(ptn_idx)==ptnIdx_to_vecIdx.end()){
        ptnIdx_to_vecIdx[ptn_idx] = num_failptn;
        vector<FLOG*> log_for_certain_ptn;
        log_for_certain_ptn.push_back(log);

        failLog.push_back(log_for_certain_ptn);
        ++num_failptn;
      }
      else{
        failLog[ptnIdx_to_vecIdx.at(ptn_idx)].push_back(log);
      }

      s = strs[1];
      log->failing_PO = s;
      failPOs[s] = nullptr;

      s = strs[5];
      assert(s == "H" || s == "L");
      log->failing_type = (s=="H") ? 1:0;

      s = strs[7];
      vec.clear();
      for (char c: s) {
        if (c == 'T') continue;  // ignore "T"
        if (c == '\'') continue; // ignore "'"
        vec.push_back(c);
      }
      log->failing_pattern = vec;
    }

  }
  file.close(); // close the file

  // output the number of vector and failing output
    fprintf(stdout, "#number of vectors = %d\n", int(failLog.size()));
    fprintf(stdout, "#number of failing outputs = %d\n", int(num_failpo));
}/* end of read_failLog */

void ATPG::diag_initial_candidate(){
  ++DFS_times;
  
  for(wptr w: cktout){
    string truncated_name = w->name/*.substr(0,w->name.find("("))*/;
    //since name in faillog are truncated, cannot use wfind or nfind to find failing PO
    //cmoplexity O(1) -> O(# of PO)
    auto iter = failPOs.find(truncated_name);
    if(iter!=failPOs.end()){
      iter->second = w;
      DFS(w);
    }
  }

  // cout<<endl<<"initial candidates: num= "<<candidate_faults.size()<<endl;
  // cerr<<endl<<"initial candidates: num= "<<candidate_faults.size()<<endl;
  // for(auto it=candidate_faults.begin() ; it!=candidate_faults.end() ; ++it){
  //   string wname = it->second->name.substr(0,it->second->name.find("("));
  //   string pos = it->first->io?"GO":"GI";
  //   string type = it->first->fault_type? "SA1":"SA0";
  //   cerr<<wname<<" "<<it->first->node->name<<" "<<pos<<" "<<type<<endl;
  // }
}/* end of diag_initial_candidate */

bool ATPG::diag_remove_candidate(){
  bool remove_at_least_1candidate = 0;

  //method1: only deal with failing pattern 
  //method2: consider all test pattern ->slower but remove more candidate
  for(int i=0;i<vectors.size();++i){
    set<string> failingPO_set;
    string failing_pattern;

    if(ptnIdx_to_vecIdx.find(i)!=ptnIdx_to_vecIdx.end()){
      vector<FLOG*>& log_for_certain_ptn = failLog[ptnIdx_to_vecIdx.at(i)];
      failing_pattern = log_for_certain_ptn[0]->failing_pattern;
      // int ptn_idx = log_for_certain_ptn[0]->failing_index;

      for(int j=0;j<log_for_certain_ptn.size();++j){
        //use set -> 
        //when checking if fault-free-PO change to Unknown
        //can quickly find if PO faulty or not?
        failingPO_set.insert(log_for_certain_ptn[j]->failing_PO);
      }
    }
    else{
      failing_pattern = vectors[i];
    }

    //1. fault free sim
    for (int j = 0; j < cktin.size(); j++) { cktin[j]->value = ctoi(failing_pattern[j]); }
    for (int j = 0; j < sort_wlist.size(); j++) {
      if (j < cktin.size()) { sort_wlist[j]->set_changed(); }
      else { sort_wlist[j]->value = U; }
    }
    sim();

    for (int j = 0; j < sort_wlist.size(); j++) {
      //use wire_value1 to store fault free value
      //use wire_value2 to store how value was decided
      sort_wlist[j]->remove_faulty();
      switch (sort_wlist[j]->value) {
        case 1:
          sort_wlist[j]->wire_value1 = 1;
          break;
        case 2:
          sort_wlist[j]->wire_value1 = U;
          break;
        case 0:
          sort_wlist[j]->wire_value1 = 0;
          break;
      }
    }
    //end of 1.
    // display_io();

    //2. set faulty wire value to Unknown and sim
    for(auto it=candidate_faults.begin() ; it!=candidate_faults.end() ; ++it){
      it->second->set_faulty();
      it->second->value = U;
      it->second->set_changed();
    }
    sim(true);
    //end of 2.
    // display_io();
    
    //3. check if there are fault-free wires change to Unknown
    //   if yes, change them back to fault-free-value
    queue<wptr> changing_queue;
    for(wptr w: cktout){
      string truncated_name = w->name/*.substr(0,w->name.find("("))*/;
      if(w->value==U && failingPO_set.find(truncated_name)==failingPO_set.end()){
        w->value = w->wire_value1;
        changing_queue.push(w);
      }
    }
    //end of 3.
    
    //4. implication
    while(!changing_queue.empty()){
      wptr changing_wptr = changing_queue.front();
      nptr changing_nptr = changing_wptr->inode.front();
      int nin = changing_nptr->iwire.size();
      int desired_logic_value = changing_wptr->value;
      changing_queue.pop();

      assert(desired_logic_value!=U);

      switch(changing_nptr->type){
        case BUF: 
          for(fptr f: changing_nptr->iwire[0]->flist_w){
            if(f->node == changing_nptr->iwire[0]->inode[0] &&
              ((desired_logic_value==1 && f->fault_type==0) || (desired_logic_value==0 && f->fault_type==1)))
            {
              if(candidate_faults.find(f) != candidate_faults.end()){
                remove_at_least_1candidate = 1;
                candidate_faults.erase(f);
                // cerr<<"remove candidate: BUF "<<f->node->name<<" "<<sort_wlist[f->to_swlist]->name<<" SA"<<f->fault_type<<endl;
              }
            }
          }
          break;
        case NOT: 
          for(fptr f: changing_nptr->iwire[0]->flist_w){
            if(f->node == changing_nptr->iwire[0]->inode[0] &&
              ((desired_logic_value==1 && f->fault_type==1) || (desired_logic_value==0 && f->fault_type==0)))
            {
              if(candidate_faults.find(f) != candidate_faults.end()){
                remove_at_least_1candidate = 1;
                candidate_faults.erase(f);
                // cerr<<"remove candidate: NOT "<<f->node->name<<" "<<sort_wlist[f->to_swlist]->name<<" SA"<<f->fault_type<<endl;
              }
            }
          }
          break;
        case AND:
          if(desired_logic_value == 0){
            bool only_one_input_with_value0 = 0;
            wptr wire_with_value0;
            for(int j=0;j<nin;++j){
              //if only one wire has fault free value=0, it must not faulty
              if(changing_nptr->iwire[j]->wire_value1==0){
                if(only_one_input_with_value0 == 1){
                  only_one_input_with_value0 = 0;
                  break;
                }
                else{
                  wire_with_value0 = changing_nptr->iwire[j];
                  only_one_input_with_value0 = 1;
                }
              }
            }

            if(only_one_input_with_value0 && wire_with_value0->value==U){
              if(wire_with_value0->is_faulty()){
                //the only value0input is candidate -> remove it

                for(fptr f: wire_with_value0->flist_w){
                  if((f->node == wire_with_value0->inode[0] || f->node == changing_nptr) &&
                    f->fault_type==1)
                  {
                    if(candidate_faults.find(f) != candidate_faults.end()){
                      remove_at_least_1candidate = 1;
                      candidate_faults.erase(f);
                      // cerr<<"remove candidate: AND "<<f->node->name<<" "<<sort_wlist[f->to_swlist]->name<<" SA"<<f->fault_type<<endl;
                    }
                  }
                }

              }
              else{
                //the only candidate is not candidate but with value U -> backward imply

                wire_with_value0->value = 0;
                assert(wire_with_value0->wire_value1==0);
                
                changing_queue.push(wire_with_value0);
              }
            }
          }
          else{
            //when desired_logic_value==1, can remove SA0 at input,
            //but after collapsing, no input SA0 for AND gate
            //hence only let fanin all = 1
            for(int j=0;j<nin;++j){
              if(changing_nptr->iwire[j]->value==U && !changing_nptr->iwire[j]->is_faulty()){
                changing_nptr->iwire[j]->value = 1;

                assert(changing_nptr->iwire[j]->wire_value1==1);
                
                changing_queue.push(changing_nptr->iwire[j]);
              }
            }
          }
          break;
        case NAND:
          if(desired_logic_value == 1){
            bool only_one_input_with_value0 = 0;
            wptr wire_with_value0;
            for(int j=0;j<nin;++j){
              //if only one wire has fault free value=0, it must not faulty
              if(changing_nptr->iwire[j]->wire_value1==0){
                if(only_one_input_with_value0 == 1){
                  only_one_input_with_value0 = 0;
                  break;
                }
                else{
                  wire_with_value0 = changing_nptr->iwire[j];
                  only_one_input_with_value0 = 1;
                }
              }
            }

            if(only_one_input_with_value0 && wire_with_value0->value==U){
              if(wire_with_value0->is_faulty()){
                //the only value0input is candidate -> remove it

                for(fptr f: wire_with_value0->flist_w){
                  if((f->node == wire_with_value0->inode[0] || f->node == changing_nptr) &&
                    f->fault_type==1)
                  {
                    if(candidate_faults.find(f) != candidate_faults.end()){
                      remove_at_least_1candidate = 1;
                      candidate_faults.erase(f);
                      // cerr<<"remove candidate: NAND "<<f->node->name<<" "<<sort_wlist[f->to_swlist]->name<<" SA"<<f->fault_type<<endl;
                    }
                  }
                }

              }
              else{
                //the only candidate is not candidate but with value U -> backward imply

                wire_with_value0->value = 0;
                assert(wire_with_value0->wire_value1==0);
                
                changing_queue.push(wire_with_value0);
              }
            }
          }
          else{
            //when desired_logic_value==0, can remove SA0 at input,
            //but after collapsing, no input SA0 for AND gate
            //hence only let fanin all = 1
            for(int j=0;j<nin;++j){
              if(changing_nptr->iwire[j]->value==U && !changing_nptr->iwire[j]->is_faulty()){
                changing_nptr->iwire[j]->value = 1;

                assert(changing_nptr->iwire[j]->wire_value1==1);
                
                changing_queue.push(changing_nptr->iwire[j]);
              }
            }
          }
          break;
        case OR:
          if(desired_logic_value == 1){
            bool only_one_input_with_value1 = 0;
            wptr wire_with_value1;
            for(int j=0;j<nin;++j){
              //if only one wire has fault free value=0, it must not faulty
              if(changing_nptr->iwire[j]->wire_value1==1){
                if(only_one_input_with_value1 == 1){
                  only_one_input_with_value1 = 0;
                  break;
                }
                else{
                  wire_with_value1 = changing_nptr->iwire[j];
                  only_one_input_with_value1 = 1;
                }
              }
            }

            if(only_one_input_with_value1 && wire_with_value1->value==U){
              if(wire_with_value1->is_faulty()){
                //the only value1input is candidate -> remove it

                for(fptr f: wire_with_value1->flist_w){
                  if((f->node == wire_with_value1->inode[0] || f->node == changing_nptr) &&
                    f->fault_type==0)
                  {
                    if(candidate_faults.find(f) != candidate_faults.end()){
                      remove_at_least_1candidate = 1;
                      candidate_faults.erase(f);
                      // cerr<<"remove candidate: OR "<<f->node->name<<" "<<sort_wlist[f->to_swlist]->name<<" SA"<<f->fault_type<<endl;
                    }
                  }
                }

              }
              else{
                //the only candidate is not candidate but with value U -> backward imply

                wire_with_value1->value = 1;
                assert(wire_with_value1->wire_value1==1);
                
                changing_queue.push(wire_with_value1);
              }
            }
          }
          else{
            //when desired_logic_value==0, can remove SA1 at input,
            //but after collapsing, no input SA1 for OR gate
            //hence only let fanin all = 0
            for(int j=0;j<nin;++j){
              if(changing_nptr->iwire[j]->value==U && !changing_nptr->iwire[j]->is_faulty()){
                changing_nptr->iwire[j]->value = 0;

                assert(changing_nptr->iwire[j]->wire_value1==0);
                
                changing_queue.push(changing_nptr->iwire[j]);
              }
            }
          }
          break;
        case NOR:
          if(desired_logic_value == 0){
            bool only_one_input_with_value1 = 0;
            wptr wire_with_value1;
            for(int j=0;j<nin;++j){
              //if only one wire has fault free value=0, it must not faulty
              if(changing_nptr->iwire[j]->wire_value1==1){
                if(only_one_input_with_value1 == 1){
                  only_one_input_with_value1 = 0;
                  break;
                }
                else{
                  wire_with_value1 = changing_nptr->iwire[j];
                  only_one_input_with_value1 = 1;
                }
              }
            }

            if(only_one_input_with_value1 && wire_with_value1->value==U){
              if(wire_with_value1->is_faulty()){
                //the only value1input is candidate -> remove it

                for(fptr f: wire_with_value1->flist_w){
                  if((f->node == wire_with_value1->inode[0] || f->node == changing_nptr) &&
                    f->fault_type==0)
                  {
                    if(candidate_faults.find(f) != candidate_faults.end()){
                      remove_at_least_1candidate = 1;
                      candidate_faults.erase(f);
                      // cerr<<"remove candidate: NOR "<<f->node->name<<" "<<sort_wlist[f->to_swlist]->name<<" SA"<<f->fault_type<<endl;
                    }
                  }
                }

              }
              else{
                //the only candidate is not candidate but with value U -> backward imply

                wire_with_value1->value = 1;
                assert(wire_with_value1->wire_value1==1);
                
                changing_queue.push(wire_with_value1);
              }
            }
          }
          else{
            //when desired_logic_value==1, can remove SA1 at input,
            //but after collapsing, no input SA1 for NOR gate
            //hence only let fanin all = 0
            for(int j=0;j<nin;++j){
              if(changing_nptr->iwire[j]->value==U && !changing_nptr->iwire[j]->is_faulty()){
                changing_nptr->iwire[j]->value = 0;

                assert(changing_nptr->iwire[j]->wire_value1==0);
                
                changing_queue.push(changing_nptr->iwire[j]);
              }
            }
          }
          break;
        case XOR: break;
        case EQV: break;
      }
    }
    //end of 4.

  }

  return remove_at_least_1candidate;
}/* end of diag_remove_candidate */

void ATPG::diag_keep_remove_candidate(){
  bool next_iter = 1;
  while(next_iter){
    next_iter = diag_remove_candidate();
  }

  // cout<<"after removal, candidates num= "<<candidate_faults.size()<<endl;
  // cerr<<"after removal, candidates num= "<<candidate_faults.size()<<endl;
}

void ATPG::DFS(const wptr w){
  if(w->DFS_index != DFS_times){
    w->DFS_index = DFS_times;

    for(fptr f :w->flist_w){
      assert(candidate_faults.find(f)==candidate_faults.end());
      candidate_faults[f] = w;
    }
    
    nptr n = w->inode.front();
    for(int i=0;i<n->iwire.size();++i){
      DFS(n->iwire[i]);
    }
  }
}

///genfailLog
void ATPG::diag_fault_sim_evaluate(const wptr w){
  unsigned int new_value;
  nptr n;
  int i, nin, nout;

  n = w->inode.front();
  nin = n->iwire.size();
    // for (i = 0; i < nin; i++) {
    //     n->iwire[i]->print();
    //   }

  switch (n->type) {
    /*break a multiple-input gate into multiple two-input gates */
    case AND:
    case BUF:
    case NAND:
      new_value = ALL_ONE;
      for (i = 0; i < nin; i++) {
        new_value &= n->iwire[i]->wire_value2;
      }
      if (n->type == NAND) {
        new_value = PINV(new_value);  // PINV is for three-valued inversion
      }
      break;
      /*  */
    case OR:
    case NOR:
      new_value = ALL_ZERO;
      for (i = 0; i < nin; i++) {
        new_value |= n->iwire[i]->wire_value2;
      }
      if (n->type == NOR) {
        new_value = PINV(new_value);
      }
      break;

    case NOT:
      new_value = PINV(n->iwire.front()->wire_value2);
      break;

    case XOR:
      new_value = PEXOR(n->iwire[0]->wire_value2, n->iwire[1]->wire_value2);
      break;

    case EQV:
      new_value = PEQUIV(n->iwire[0]->wire_value2, n->iwire[1]->wire_value2);
      break;
  }

  /* if the new_value is different than the wire_value1 (the good value),
   * save it */
  int current_value = w->wire_value1;
  if ((w->is_faulty())) {
    current_value = w->wire_value2;
  }

  if (current_value != new_value) {

    /* if this wire is faulty, make sure the fault remains injected */
    if (w->is_fault_injected()) {
      combine(w, new_value);
    }

    /* update wire_value2 */
    w->wire_value2 = new_value;

    /* insert wire w into the faulty_wire list */
    if (!(w->is_faulty())) {
      w->set_faulty();
      wlist_faulty.push_front(w);
    }
    else {
      w->remove_faulty();
    }

    /* schedule new events */
    for (i = 0, nout = w->onode.size(); i < nout; i++) {
      if (w->onode[i]->type != OUTPUT) {
        w->onode[i]->owire.front()->set_scheduled();
      }
    }
  } // if new_value is different
    // if new_value is the same as the good value, do not schedule any new event
    // w->print();
}/* end of fault_sim_evaluate */

void ATPG::diag_greedy_ranking(){
  vector<fptr> sorted_candidates;
  vector<fptr> final_candidates;

  for(auto it: candidate_faults){
    fptr f = it.first;
    wptr w = it.second;
    int fault_type = f->fault_type;
    f->greedy_score_all = 0;
    f->greedy_score_single = 0;

    string failing_pattern;

    // cerr<<"doing fault: "
    //     <<sort_wlist[f->to_swlist]->name.substr(0,sort_wlist[f->to_swlist]->name.find("("))
    //     <<" "<<f->node->name<<" "<<(f->io?"GO":"GI")<<" SA"<<f->fault_type<<endl;

    for(int i=0;i<vectors.size();++i){
      int single_match = 0;
      bool all_match = 1;
      failing_pattern = vectors[i];

      //fault free sim
      for (int j = 0; j < cktin.size(); j++) { cktin[j]->value = ctoi(failing_pattern[j]); }
      for (int j = 0; j < sort_wlist.size(); j++) {
        if (j < cktin.size()) { sort_wlist[j]->set_changed(); }
        else { sort_wlist[j]->value = U; }
      }
      sim();

      for (int j = 0; j < sort_wlist.size(); j++) {
        //use wire_value1 to store fault free value
        switch (sort_wlist[j]->value) {
          case 1:
            sort_wlist[j]->wire_value1 = 1;
            break;
          case 2:
            sort_wlist[j]->wire_value1 = U;
            break;
          case 0:
            sort_wlist[j]->wire_value1 = 0;
            break;
        }
      }
      //inject fault and sim
      if(f->io==GI){
        //fault is on fanout branch
        if(f->node->type==OUTPUT){
          //if f is at output wire
          w->value = fault_type;
        }
        else{
          //f not at output wire, need to find wire->onode->owire
          wptr w_tmp = get_faulty_wire(f, fault_type);
          if(w_tmp!=nullptr){
            w_tmp->value = fault_type;
            w_tmp->set_changed();
            sim(true);
          }
        }
      }
      else if(w->value != fault_type){
        w->value = fault_type;
        w->set_changed();
        sim(true);
      }

      //calculate score
      if(ptnIdx_to_vecIdx.find(i)!=ptnIdx_to_vecIdx.end()){
        //this is a fail pattern
        vector<FLOG*>& log_for_certain_ptn = failLog[ptnIdx_to_vecIdx.at(i)];

        unordered_map<string, wptr> failPOs_sim;
        for(wptr w_out: cktout){
          if(w_out->wire_value1 != w_out->value){
            failPOs_sim[w_out->name] = w_out;
          }
        }

        for(FLOG* flog: log_for_certain_ptn){
          if(failPOs_sim.find(flog->failing_PO)!=failPOs_sim.end()){
            failPOs_sim.erase(flog->failing_PO);
          }

          wptr w_flog = wfind(flog->failing_PO);
          if(w_flog->wire_value1 != w_flog->value){
            ++single_match;
          }
          else{ all_match = 0; }
        }

        if(failPOs_sim.size()>0) all_match = 0;
      }
      else{
        //this is a pass pattern
        for(wptr w_out: cktout){
          if(w_out->wire_value1 == w_out->value){
            ++single_match;
          }
          else{ all_match = 0; }
        }
      }
      f->greedy_score_single += single_match;
      f->greedy_score_all += all_match;
    }
    sorted_candidates.push_back(f);
  }

  sort(sorted_candidates.begin(), sorted_candidates.end(), [](const fptr& lhs, const fptr& rhs){
    // if(lhs->greedy_score_all > rhs->greedy_score_all) return true;
    // else if(lhs->greedy_score_all = rhs->greedy_score_all) return lhs->greedy_score_single > rhs->greedy_score_single;
    // else return false;
    return ((lhs->greedy_score_all<<20)+lhs->greedy_score_single) > ((rhs->greedy_score_all<<20)+rhs->greedy_score_single);
  });

  final_candidates.push_back(sorted_candidates[0]);
  sorted_candidates[0]->choosed = 1;
  for(int i=1;i<sorted_candidates.size();++i){
    fptr f = sorted_candidates[i];
    if( f->greedy_score_all==final_candidates[0]->greedy_score_all
      &&f->greedy_score_single==final_candidates[0]->greedy_score_single){
      final_candidates.push_back(f);
      f->choosed = 1;
    }
    else break;
  }

  if(final_candidates[0]->greedy_score_all != vectors.size()){
    diag_greedy_ranking_2nd_iter(final_candidates);
  }

  diag_print_result(final_candidates);
}

void ATPG::diag_greedy_ranking_2nd_iter(vector<fptr>& final_candidates){
  vector<fptr> sorted_candidates;

  for(auto it: candidate_faults){
    fptr f = it.first;
    wptr w = it.second;
    f->greedy_score_all = 0;
    f->greedy_score_single = 0;

    string failing_pattern;

    // cerr<<"doing fault: "
    //     <<sort_wlist[f->to_swlist]->name.substr(0,sort_wlist[f->to_swlist]->name.find("("))
    //     <<" "<<f->node->name<<" "<<(f->io?"GO":"GI")<<" SA"<<f->fault_type<<endl;

    for(int i=0;i<vectors.size();++i){
      int single_match = 0;
      bool all_match = 1;
      failing_pattern = vectors[i];

      //fault free sim
      for (int j = 0; j < cktin.size(); j++) { cktin[j]->value = ctoi(failing_pattern[j]); }
      for (int j = 0; j < sort_wlist.size(); j++) {
        if (j < cktin.size()) { sort_wlist[j]->set_changed(); }
        else { sort_wlist[j]->value = U; }
      }
      sim();

      for (int j = 0; j < sort_wlist.size(); j++) {
        //use wire_value1 to store fault free value
        switch (sort_wlist[j]->value) {
          case 1:
            sort_wlist[j]->wire_value1 = 1;
            break;
          case 2:
            sort_wlist[j]->wire_value1 = U;
            break;
          case 0:
            sort_wlist[j]->wire_value1 = 0;
            break;
        }
      }

      //inject fault and sim
      diag_faultsim_by_level(f, final_candidates[0]);

      //calculate score
      if(ptnIdx_to_vecIdx.find(i)!=ptnIdx_to_vecIdx.end()){
        //this is a fail pattern
        vector<FLOG*>& log_for_certain_ptn = failLog[ptnIdx_to_vecIdx.at(i)];

        unordered_map<string, wptr> failPOs_sim;
        for(wptr w_out: cktout){
          if(w_out->wire_value1 != w_out->value){
            failPOs_sim[w_out->name] = w_out;
          }
        }

        for(FLOG* flog: log_for_certain_ptn){
          if(failPOs_sim.find(flog->failing_PO)!=failPOs_sim.end()){
            failPOs_sim.erase(flog->failing_PO);
          }

          wptr w_flog = wfind(flog->failing_PO);
          if(w_flog->wire_value1 != w_flog->value){
            ++single_match;
          }
          else{ all_match = 0; }
        }

        if(failPOs_sim.size()>0) all_match = 0;
      }
      else{
        //this is a pass pattern
        for(wptr w_out: cktout){
          if(w_out->wire_value1 == w_out->value){
            ++single_match;
          }
          else{ all_match = 0; }
        }
      }
      f->greedy_score_single += single_match;
      f->greedy_score_all += all_match;
    }
    sorted_candidates.push_back(f);
  }

  sort(sorted_candidates.begin(), sorted_candidates.end(), [](const fptr& lhs, const fptr& rhs){
    // if(lhs->greedy_score_all > rhs->greedy_score_all) return true;
    // else if(lhs->greedy_score_all = rhs->greedy_score_all) return lhs->greedy_score_single > rhs->greedy_score_single;
    // else return false;
    return ((lhs->greedy_score_all<<20)+lhs->greedy_score_single) > ((rhs->greedy_score_all<<20)+rhs->greedy_score_single);
  });

  int score_all_prev = 0, score_single_prev = 0, has_all_matched = 0;
  for(int i=0, candidate_size=0; i<sorted_candidates.size(); ++i){
    fptr f_curr = sorted_candidates[i];

    if((score_all_prev!=f_curr->greedy_score_all) ||
       (score_single_prev!=f_curr->greedy_score_single)){
      ++candidate_size;
      if((candidate_size > 4)||has_all_matched) break;
    }

    if(f_curr->greedy_score_all==vectors.size()){has_all_matched = 1;}
    
    if(!f_curr->choosed) {
      f_curr->choosed = 1;
      final_candidates.push_back(f_curr);
    }

    score_all_prev = f_curr->greedy_score_all;
    score_single_prev = f_curr->greedy_score_single;
  }
}

void ATPG::diag_print_result(const vector<fptr>& final_candidates){
  int score_all_prev = 0, score_single_prev = 0, groupID = 0;

  cout<<endl<<"Ranked suspect faults"<<endl;
  for(int i = 0;i<final_candidates.size();++i){
    fptr f = final_candidates[i];

    if((f->greedy_score_all!=score_all_prev)||(f->greedy_score_single!=score_single_prev)) ++groupID;
    score_all_prev = f->greedy_score_all;
    score_single_prev = f->greedy_score_single;

    // cerr<<"No."<<i+1<<" "
    cout<<"No."<<i+1<<" "
        <<sort_wlist[f->to_swlist]->name.substr(0,sort_wlist[f->to_swlist]->name.find("("))
        <<" "<<f->node->name<<" "<<(f->io?"GO":"GI")<<" SA"<<f->fault_type
        <<", groupID="<<groupID
        <<", perfect_match_score="<<f->greedy_score_all
        <<", single_match_score="<<f->greedy_score_single<<" [ equivalent faults: ";
    EQUIVALENT_LIST* linklist = f->fault_linklist;
    while(linklist!=nullptr){
      cout<<linklist->fault_name<<", ";
      // cerr<<linklist->fault_name<<", ";
      linklist = linklist->next_fault;
    }
    cout<<"]"<<endl;
    // cerr<<"]"<<endl;
  }
}

void ATPG::diag_ssf_print_result(fptr final_suspect){
  int score_all_prev = 0, score_single_prev = 0, groupID = 0;

  
    fptr f = final_suspect;
    // cerr<<"No."<<i+1<<" "
    cout<<endl<<"Ranked suspect faults"<<endl<<"No."<<1<<" "
        <<sort_wlist[f->to_swlist]->name.substr(0,sort_wlist[f->to_swlist]->name.find("("))
        <<" "<<f->node->name<<" "<<(f->io?"GO":"GI")<<" SA"<<f->fault_type
        <<", perfectly match!!!" <<" [ equivalent faults: ";
    EQUIVALENT_LIST* linklist = f->fault_linklist;
    while(linklist!=nullptr){
      cout<<linklist->fault_name<<", ";
      // cerr<<linklist->fault_name<<", ";
      linklist = linklist->next_fault;
    }
    cout<<"]"<<endl;
    // cerr<<"]"<<endl;
  
}

void ATPG::diag_faultsim_by_level(fptr f1, fptr f2){
  wptr w1 = sort_wlist[f1->to_swlist];
  wptr w2 = sort_wlist[f2->to_swlist];
  int fault_type1 = f1->fault_type;
  int fault_type2 = f2->fault_type;
  
  if(w1->level > w2->level){
    //sim w2 first
    if(f2->io==GI){
      //fault is on fanout branch
      if(f2->node->type==OUTPUT){
        //if f is at output wire
        w2->value = fault_type2;
      }
      else{
        //f not at output wire, need to find wire->onode->owire
        wptr w_tmp = get_faulty_wire(f2, fault_type2);
        if(w_tmp!=nullptr){
          w_tmp->value = fault_type2;
          w_tmp->set_changed();
          sim(true);
        }
      }
    }
    else if(w2->value != fault_type2){
      w2->value = fault_type2;
      w2->set_changed();
      sim(true);
    }
    if(f1->io==GI){
      //fault is on fanout branch
      if(f1->node->type==OUTPUT){
        //if f is at output wire
        w1->value = fault_type1;
      }
      else{
        //f not at output wire, need to find wire->onode->owire
        wptr w_tmp = get_faulty_wire(f1, fault_type1);
        if(w_tmp!=nullptr){
          w_tmp->value = fault_type1;
          w_tmp->set_changed();
          sim(true);
        }
      }
    }
    else if(w1->value != fault_type1){
      w1->value = fault_type1;
      w1->set_changed();
      sim(true);
    }
  }
  else if(w1->level == w2->level){
    if(f1->io==GI){
      //sim w2 first
      if(f2->io==GI){
        //fault is on fanout branch
        if(f2->node->type==OUTPUT){
          //if f is at output wire
          w2->value = fault_type2;
        }
        else{
          //f not at output wire, need to find wire->onode->owire
          wptr w_tmp = get_faulty_wire(f2, fault_type2);
          if(w_tmp!=nullptr){
            w_tmp->value = fault_type2;
            w_tmp->set_changed();
            sim(true);
          }
        }
      }
      else if(w2->value != fault_type2){
        w2->value = fault_type2;
        w2->set_changed();
        sim(true);
      }
      if(f1->io==GI){
        //fault is on fanout branch
        if(f1->node->type==OUTPUT){
          //if f is at output wire
          w1->value = fault_type1;
        }
        else{
          //f not at output wire, need to find wire->onode->owire
          wptr w_tmp = get_faulty_wire(f1, fault_type1);
          if(w_tmp!=nullptr){
            w_tmp->value = fault_type1;
            w_tmp->set_changed();
            sim(true);
          }
        }
      }
      else if(w1->value != fault_type1){
        w1->value = fault_type1;
        w1->set_changed();
        sim(true);
      }
    }
    else{
      //sim w1 first
      if(f1->io==GI){
        //fault is on fanout branch
        if(f1->node->type==OUTPUT){
          //if f is at output wire
          w1->value = fault_type1;
        }
        else{
          //f not at output wire, need to find wire->onode->owire
          wptr w_tmp = get_faulty_wire(f1, fault_type1);
          if(w_tmp!=nullptr){
            w_tmp->value = fault_type1;
            w_tmp->set_changed();
            sim(true);
          }
        }
      }
      else if(w1->value != fault_type1){
        w1->value = fault_type1;
        w1->set_changed();
        sim(true);
      }
      if(f2->io==GI){
        //fault is on fanout branch
        if(f2->node->type==OUTPUT){
          //if f is at output wire
          w2->value = fault_type2;
        }
        else{
          //f not at output wire, need to find wire->onode->owire
          wptr w_tmp = get_faulty_wire(f2, fault_type2);
          if(w_tmp!=nullptr){
            w_tmp->value = fault_type2;
            w_tmp->set_changed();
            sim(true);
          }
        }
      }
      else if(w2->value != fault_type2){
        w2->value = fault_type2;
        w2->set_changed();
        sim(true);
      }
    }
  }
  else{
    //sim w1 first
    if(f1->io==GI){
      //fault is on fanout branch
      if(f1->node->type==OUTPUT){
        //if f is at output wire
        w1->value = fault_type1;
      }
      else{
        //f not at output wire, need to find wire->onode->owire
        wptr w_tmp = get_faulty_wire(f1, fault_type1);
        if(w_tmp!=nullptr){
          w_tmp->value = fault_type1;
          w_tmp->set_changed();
          sim(true);
        }
      }
    }
    else if(w1->value != fault_type1){
      w1->value = fault_type1;
      w1->set_changed();
      sim(true);
    }
    if(f2->io==GI){
      //fault is on fanout branch
      if(f2->node->type==OUTPUT){
        //if f is at output wire
        w2->value = fault_type2;
      }
      else{
        //f not at output wire, need to find wire->onode->owire
        wptr w_tmp = get_faulty_wire(f2, fault_type2);
        if(w_tmp!=nullptr){
          w_tmp->value = fault_type2;
          w_tmp->set_changed();
          sim(true);
        }
      }
    }
    else if(w2->value != fault_type2){
      w2->value = fault_type2;
      w2->set_changed();
      sim(true);
    }
  }    
}

ATPG::wptr ATPG::diag_GI_get_faulty_wire( list<fptr> flist, int &fault_type) {
  fptr f = flist.front();
  if(!diag_GI_non_control(flist,fault_type)) {
    // cout << fault_type << endl;
    return f->node->owire.front();
  }
  // check side input 
  int i, nin;
  bool is_faulty;
  // bool check_side_input = true;
  list<wptr> side_input_list {};
  vector<int> fault_index_vec;
  is_faulty = true;
  for(auto GI_fault: flist){
    fault_index_vec.push_back(GI_fault->index);
  }
  std::sort(fault_index_vec.begin(), fault_index_vec.end());           
  nin = f->node->iwire.size();
  int vec_idx = 0; // fault_index_vec idx
  for(i = 0; i < nin; i++){
    if(i == fault_index_vec[vec_idx]){
      vec_idx++;
      continue;
    }
    side_input_list.push_back(f->node->iwire[i]);
  }
  is_faulty = true;
  // cout <<"size: side_input_list: "<<side_input_list.size()<<endl;
  switch (f->node->type) {
    /*check every gate input of AND
      if any input is zero or unknown, then fault f is not propagated */
  case AND:
    for (auto w: side_input_list) {
      if ( (w->wire_value2 & Mask[0]) ^ (ALL_ONE & Mask[0])) {      ///need compare to faulty value
        is_faulty = false;  // not propagated
      }
    }
    
    /* AND gate input stuck-at one fault is propagated to
        AND gate output stuck-at one fault */
    if (f->fault_type == 0)
      fault_type = STR;
    else
      fault_type = STF;
    break;

  case NAND:
    for (auto w: side_input_list) {
      if ((w->wire_value2 & Mask[0]) ^ (ALL_ONE & Mask[0])) {
        is_faulty = false;
      }
    }
    if (f->fault_type == 0)
      fault_type = STF;
    else
      fault_type = STR;
    break;
  case OR:
    for (auto w: side_input_list) {
      if ((w->wire_value2 & Mask[0]) ^ (ALL_ZERO & Mask[0])) {
        is_faulty = false;
      }
    }
    if (f->fault_type == 0)
      fault_type = STR;
    else
      fault_type = STF;
    break;
  case NOR:
    for (auto w: side_input_list) {
      if ((w->wire_value2 & Mask[0]) ^ (ALL_ZERO & Mask[0])) {
        is_faulty = false;
      }
    }
    if (f->fault_type == 0)
      fault_type = STF;
    else
      fault_type = STR;
    break;
  case XOR:
    for (auto w:side_input_list) {
      if ((w->wire_value2 & Mask[0]) == (ALL_ONE & Mask[0])) {
          // fault_type = f->fault_type;
        } else {
          // fault_type = f->fault_type ^ 1;
          fault_type ^= 1;
        }
      }
    break;
  case EQV:
    for (auto w: side_input_list) {
      if ((w->wire_value2 & Mask[0]) == (ALL_ONE & Mask[0])) {
        fault_type = f->fault_type ^ 1;
      } else {
          // fault_type = f->fault_type;
      }
    }
    break;
  }
  if (is_faulty) {
        // cout << fault_type << endl;
    return (f->node->owire.front());
  }
  return (nullptr);
}/* end of diag_GI_get_faulty_wire */


bool ATPG::diag_GI_non_control(list<fptr> flist, int & fault_type){
//return true if all fault in list is non controling value
  int i, nin;
  bool is_faulty;

  is_faulty = true;
  bool non_control_value = 0;
  fptr f  = flist.front();
  nin = f->node->iwire.size();
  fault_type = STUCK1;
  switch (f->node->type) {
    case AND:
    case NAND:
      non_control_value = 1;
      break;
    case OR:
    case NOR:
      non_control_value = 0;
      break;
    default:
      return true;
  }
  switch (f->node->type) {
    case AND:
    case NOR:
      fault_type = STUCK0;
      break;
    case OR:
    case NAND:
      fault_type = STUCK1;
      break;
    case XOR:
      fault_type = STUCK0;
    case EQV:
    //fault_type = STUCK1;
      for(auto GI_fault: flist){
        if (GI_fault->fault_type == STUCK1) {
            fault_type ^= 1;
          }
      }
    default:
      return true;
  }
  for(auto GI_fault: flist){
    if(GI_fault->fault_type != non_control_value)
      return false;
      //stuck at controlling value
  }
  return true;

}

//1st iteration: find one candidate (if same score, also add to list)

//if the 1st iteration perfect_score != # of ptn
//enter 2st iteration

//2st iteration: inject only "one" candidate found at 1st iteration, then score&rank (sim based on their level).
//choose some candidade candidate to the list
//note that when # of candidates from 1st iter > 1, they must with the same score