#include "atpg.h"
#define num_of_faults_in_parallel  16

void ATPG::diagnosis_map_insert(){  
    wptr w;
    int i,j;
    for (auto pos : sort_wlist){
    w = pos;
    wire2index[w->name] = w->wlist_index;  //ssf_diag need mod  w->name.substr(0,w->name.find("("))
    }
    type2int["GI"] = 0;
    type2int["GO"] = 1;
    type2int["SA0"] = 0;
    type2int["SA1"] = 1;

    for(j=0;j<vectors.size();j++){
    ///fault free sim
    for (i = 0; i < cktin.size(); i++) {
        cktin[i]->value = ctoi(vectors[j][i]);
        //cout << cktin[i]->value;
    }
    //nckt = sort_wlist.size();
    for (i = 0; i < sort_wlist.size(); i++) {
        if (i < cktin.size()) {
        sort_wlist[i]->set_changed();
        } else {
        sort_wlist[i]->value = U;
        }
    }
    sim(); 

    unordered_map<string, bool> map_temp;
    for (auto pos : sort_wlist) {
      w = pos;
        if (w->is_output()) { /////////can replaced by finding map index
            map_temp[w->name]=w->value;  //ssf_diag need mod
            //
            //add failingPO
            // cout << w->name << " " << map_temp[w->name.substr(0,w->name.find("("))] << endl; //ssf_diag need mod
            //
        }
    }
    exact_response.push_back(map_temp);

    }

    for(i=0;i<failLog.size();i++){
        for(j=0;j<failLog[i].size();j++){
            exact_response[failLog[i][j]->failing_index][failLog[i][j]->failing_PO] = failLog[i][j]->failing_type;
        }
    }


}


void ATPG::single_diag(){

    ///for all fail_pattern
    int i,j;
    int check_num;
    wptr w;
    // vector<wptr> temp;

    for(int n_f_pat=0;n_f_pat<failLog.size();n_f_pat++){

    temp.clear();
    for (i = 0; i < sort_wlist.size(); i++) {
        sort_wlist[i]->diag_flag=false;
    }

    ///fault free sim
    for (i = 0; i < cktin.size(); i++) {
        cktin[i]->value = ctoi(failLog[n_f_pat][0]->failing_pattern[i]);
        //cout << cktin[i]->value;
    }
    //nckt = sort_wlist.size();
    for (i = 0; i < sort_wlist.size(); i++) {
        if (i < cktin.size()) {
        sort_wlist[i]->set_changed();
        } else {
        sort_wlist[i]->value = U;
        }
    }
    sim(); 

    for(int n_f_bit=0;n_f_bit<failLog[n_f_pat].size();n_f_bit++){
        //cout << failLog[n_f_pat][n_f_bit]->failing_PO << "  " << failLog[n_f_pat][n_f_bit]->failing_pattern << "  " << failLog[n_f_pat][n_f_bit]->failing_index << "  " << failLog[n_f_pat][n_f_bit]->failing_type << endl;
        w = sort_wlist[wire2index[failLog[n_f_pat][n_f_bit]->failing_PO]];
        //cout << w->name << endl;
        ssf_DFS(w);
    }

    //cout << "vdshkslb" << ssf_cand.size() << endl;
    // for(i=0;i<temp.size();i++){
    //     cout << temp[i]->name << " ";
    // }
    // cout << endl;

    //intersection
    //check_num=0;
    if(n_f_pat==0)
        ssf_cand = temp;
    else{
        for(i=0;i<ssf_cand.size();i++){
            check_num=0;
            for(j=0;j<temp.size();j++){
                if(temp[j]==ssf_cand[i])
                    check_num=1;
            }
            if(!check_num){
                ssf_cand.erase(ssf_cand.begin()+i);
                i--;
            }
        }
    }

    }
    //all intersection result
//     cout<<"cand"<<endl;
//     for(i=0;i<ssf_cand.size();i++){
//         cout << ssf_cand[i]->name << " ";
//     }
// cout << "AAAAAAAAAAAAAAAAAAAAAA" << endl;
    //gen cand fault
    j=0;
    fptr f;
    for (auto pos = flist_undetect.cbegin(); pos != flist_undetect.cend(); ++pos) {
    //int fault_detected[num_of_faults_in_parallel] = {0}; //for n-det
    f = *pos;
    // if(f->diag_flag==false)
    //     i++;
    for(i=0;i<ssf_cand.size();i++){
        if(ssf_cand[i]->name.compare(sort_wlist[f->to_swlist]->name) == 0){
            ssf_flist.push_front(f);
            j++;
        }
    }

    }
    ////
    // for (auto pos = flist.cbegin(); pos != flist.cend(); ++pos) {
    // //int fault_detected[num_of_faults_in_parallel] = {0}; //for n-det
    // j++;

    // }
    //
    // cout << "II" << j << endl;

    for(auto pos = ssf_flist.cbegin(); pos != ssf_flist.cend(); ++pos){
        f = *pos;
    //
    //cout << "the name " << sort_wlist[f->to_swlist]->name << endl;
    ///
        cand_response.clear();
        for(j=0;j<vectors.size();j++){
          //
          // cout << "AAAAAAAAAAAA" << f->io << "AAAAAAAAAA" << sort_wlist[f->to_swlist]->name << f->node->name  << endl;
          //
            ssf_diag_sim_a_vector(vectors[j],f,j);
            ////debug
            if(f->diag_flag)
              break;
                // break;
        }
        if(f->diag_flag)
            continue;
    }




    ///
    for(auto pos = ssf_flist.cbegin(); pos != ssf_flist.cend(); ++pos){
        f = *pos;
        if(!f->diag_flag){
          diag_ssf_print_result(f);
          //cout << "********** " << sort_wlist[f->to_swlist]->name << " " << f->node->name << " " << ((f->io)?"GO":"GI") << " " << ((f->fault_type)?"SA1":"SA0") << endl;
          ssf_det = true;
          break;
        }
            
    }
    ///


    return;
}

void ATPG::ssf_DFS(const wptr w){
    ///
    if(w->diag_flag)
      return;
    ///


    if(!w->diag_flag){
        temp.push_back(w);
        w->diag_flag = true;
    }
    
    if(w->inode[0]->type==INPUT)
        return;

    //
    if(w->value){
    switch (w->inode[0]->type) {
      case AND:
      case NOR:
        for(int i=0;i<w->inode[0]->iwire.size();i++)
            ssf_DFS(w->inode[0]->iwire[i]);
        break;

      case OR:
        for(int i=0;i<w->inode[0]->iwire.size();i++){
            if(w->inode[0]->iwire[i]->value==1)
                ssf_DFS(w->inode[0]->iwire[i]);
            }
            break;

      case NAND:
        for(int i=0;i<w->inode[0]->iwire.size();i++){
            if(w->inode[0]->iwire[i]->value==0)
                ssf_DFS(w->inode[0]->iwire[i]);
            }
            break;

      case BUF:
      case NOT:
        ssf_DFS(w->inode[0]->iwire[0]);
        break;
      case INPUT:
      case EQV:
      case XOR:
        break;
    }
    }
    else{
    switch (w->inode[0]->type) {
      case NAND:
      case OR:
        for(int i=0;i<w->inode[0]->iwire.size();i++)
            ssf_DFS(w->inode[0]->iwire[i]);
        break;

      case AND:
        for(int i=0;i<w->inode[0]->iwire.size();i++){
            if(w->inode[0]->iwire[i]->value==0)
                ssf_DFS(w->inode[0]->iwire[i]);
            }
            break;

      case NOR:
        for(int i=0;i<w->inode[0]->iwire.size();i++){
            if(w->inode[0]->iwire[i]->value==1)
                ssf_DFS(w->inode[0]->iwire[i]);
            }
            break;

      case BUF:
      case NOT:
        ssf_DFS(w->inode[0]->iwire[0]);
        break;
      case INPUT:
      case EQV:
      case XOR:
        break;
    }
    }
    //
    
    // for(int i=0;i<w->inode[0]->iwire.size();i++)
    //     ssf_DFS(w->inode[0]->iwire[i]);
    return;
}

void ATPG::ssf_diag_sim_a_vector(const string &vec, fptr f, int vecidx) {
  wptr w, faulty_wire;
  /* array of 16 fptrs, which points to the 16 faults in a simulation packet  */
//   fptr simulated_fault_list[num_of_faults_in_parallel];
//   fptr f;

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


    /* if f is an gate output fault */
    if (f->io == GO) {

      /* if this wire is not yet marked as faulty, mark the wire as faulty
        * and insert the corresponding wire to the list of faulty wires. */
      if (!(sort_wlist[f->to_swlist]->is_faulty())) {
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
      start_wire_index = min(start_wire_index, f->to_swlist);
    }  // if gate output fault

      /* the fault is a gate input fault */
    else if (!(f->node->type == OUTPUT)) {    //output is a fanout branch

      /* if the fault is propagated, set faulty_wire equal to the faulty wire.
        * faulty_wire is the gate output of f.  */
      faulty_wire = diag_get_faulty_wire(f, fault_type);
      if (faulty_wire != nullptr) {

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
          start_wire_index = min(start_wire_index, f->to_swlist);

      }
    }




      /* starting with start_wire_index, evaulate all scheduled wires
       * start_wire_index helps to save time. */
      for (i = start_wire_index; i < nckt; i++) {
        if (sort_wlist[i]->is_scheduled()) {
          sort_wlist[i]->remove_scheduled();
          fault_sim_evaluate(sort_wlist[i]);
        }
      } /* event evaluations end here */

      while (!wlist_faulty.empty()) {
        w = wlist_faulty.front();
        wlist_faulty.pop_front();
        w->remove_faulty();
        w->remove_fault_injected();
        w->set_fault_free();

        // }
        //w->wire_value2 = w->wire_value1;  // reset to fault-free values
        /* end TODO*/
      } // pop out all faulty wires
    /// 
    unordered_map<string, bool> map_temp;
    //
      for (auto pos : sort_wlist){
          w = pos;
          if (w->is_output()) { // if primary output
            //add failingPO
            map_temp[w->name]=w->wire_value2 & Mask[0]; //ssf_diag need mod w->wire_value2 & Mask[0]
            // cout << w->name << " " << map_temp[w->name.substr(0,w->name.find("("))] << endl; //ssf_diag need mod
            //
          }
          //
          ///
          w->wire_value2 = w->wire_value1;  // reset to fault-free values
      }

      //output is a fanout branch
      if(f->node->type == OUTPUT)
        map_temp[w->name.substr(0,w->name.find("("))]=f->fault_type;



        if(!(map_temp==exact_response[vecidx])){ 
            f->diag_flag=true;
        }
        

      num_of_fault = 0;  // reset the counter of faults in a packet
      start_wire_index = 10000;  //reset this index to a very large value.
//     } // end fault sim of a packet
//   } // end loop. for f = flist

}/* end of fault_sim_a_vector */