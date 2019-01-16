#include <iostream>
#include <fstream>
#include <math.h>
#include <string>
#include <thread>
#include <mutex>

using namespace std;

bool solution_found = false;
mutex m;
mutex l;
long double backtrackcount = 0;


struct clause{
    int condition;
    clause* next;
};

struct variable{
    bool value; //neg is false, pos is true for 0 < ID <= noliterals
            //this value is always initialized as true, therefore if it is false it means you must go back another level
                //as true has already been checked
    int order; //the value of when this variable was assigned a value
};

void print_variables(variable* variables,int noliterals);

void backtrack_soln(int noclauses, int noliterals, variable* variables, variable* variablesS, clause* clauses, bool df);

clause* get_clause();

int main(){
    int noliterals; //number of literals read into the file and to initialize the size of the literal array
    int noclauses; //same as above
    int i,j,h,varcounter;
    bool head;
    string lit;
    clause* clauses;
    variable* variablest;
    variable* variablesf;
    variable* variablesS;

    ifstream readfile;
    string filename = "dimacs.txt";

    //cout << "Enter name of file: " << endl;
    //cin >> filename;

    readfile.open(filename);

    readfile >> lit;
    readfile >> lit;
    


    readfile >> noliterals;
    readfile >> noclauses;

    //initialize clause array
    clauses = new clause[noclauses];
    for(i = 0; i < noclauses; i++){
        clauses[i].condition = 0;
        clauses[i].next = &clauses[i];
    }
    
    i = 0;
    clause* c;
    head = true;
    cout << "------------------" << endl;
    //////////this can be further optimized by threading this
    while(!readfile.eof()){
        if(head){
            readfile >> lit;
            clauses[i].condition = stoi(lit);
            head = false;
        }
        else{
            c = get_clause();
            c->condition = clauses[i].condition;
            c->next = clauses[i].next;
            readfile >> lit;
            if(lit != "0"){
                clauses[i].condition = stoi(lit);
                clauses[i].next = c;
            }
            else{
                head = true;
                i++;
            }
        }
    }
    readfile.close();
    ////////////////also by threading this
    //initialize true variable array
    variablest = new variable[noliterals];
    for(i = 0; i < noliterals; i++){
        variablest[i].value = true;
        variablest[i].order = -1;
    }
    //////////////and by threading this
    //initialize false variable array
    variablesf = new variable[noliterals];
    for(i = 0; i < noliterals; i++){
        variablesf[i].value = false;
        variablesf[i].order = -1;
    }

    variablesS = new variable[noliterals];

    varcounter = 0;
    
    /////////////////////another way to multithread and speed this up would be 
    thread threads[2];
    threads[0] = thread(backtrack_soln, noclauses, noliterals, variablest, variablesS, clauses, true);
    threads[1] = thread(backtrack_soln, noclauses, noliterals, variablesf, variablesS, clauses, false);
    //insert function////////////////////////////////////////////
    //backtrack_soln(noclauses, noliterals, variables, clauses, true);
    while(!solution_found){
        this_thread::sleep_for(chrono::seconds(2));
        cout << "Backtrack Counter: " << backtrackcount << endl;
    }
    threads[0].join();
    threads[1].join();
    cout << "-----solution-----" << endl;
    for(j = 0; j < noliterals; j++){
        cout << "x" << j+1 << ": " << variablesS[j].value << endl;
    }
    


    return 0;
}

void backtrack_soln(int noclauses, int noliterals, variable* variables, variable* variablesS, clause* clauses, bool df){
    clause* curcondition;
    int h;
    int varcounter = 0;
    for(int i = 0; i < noclauses; i++){
        if(solution_found){
            break;
        }
        //if(!df){
        //    cout << i+2 << endl;
        //}
        h = clauses[i].condition; //holder variable to identify when we have traversed each condition in the clause as each must be unique
        curcondition = &clauses[i];
        while(true){
            //cout << clauses[i].condition << endl;
            //cout << "next con: " << clauses[i].next->condition << endl;//////////////////////
            //if condition's variable is not assigned
            if(variables[abs(curcondition->condition)-1].order == -1){
                varcounter++;
                variables[abs(curcondition->condition)-1].order = varcounter;//takes note of when this var was assigned in the sequence
                variables[abs(curcondition->condition)-1].value = df;
                //if variable passes the condition as true
                if((curcondition->condition > 0 && df) || (curcondition->condition < 0 && !df)){
                    break;//variable passes and we good
                }
                //else variable does not pass the condition
                else{
                    curcondition = curcondition->next;//variable does not 
                }    
            }
            //if variable is assigned
            else if(variables[abs(curcondition->condition)-1].order != -1){
                //if variable does pass the condition
                if((curcondition->condition > 0 && variables[abs(curcondition->condition)-1].value) || (curcondition->condition < 0 && !variables[abs(curcondition->condition)-1].value)){
                    break; //we good
                }
                //else variable does NOT pass the condition
                else{                   
                    curcondition = curcondition->next;
                    //if next condition is != to first condition of clause(if we haven't explored all the conditions) do nothing
                    //if next condition is == to the first condition of clause - clause fails
                    if(curcondition->condition == h){
                        h = varcounter;
                        m.lock();
                        backtrackcount++;
                        m.unlock();
                        //find variable with order varcounter and switch, if already false, then switch previous choice as well
                        for(int j = 0; j < noliterals; j++){
                            if(variables[j].order == h){
                                if(variables[j].value == df){
                                    variables[j].value = !df;
                                }
                                //else switch variable to true and look for next most recently set variable 
                                else{
                                    variables[j].value = df;
                                    h--;
                                    j = -1;
                                }
                            }
                        }
                        i = -1;//reset i to start analysing clauses from beginning with new forced variable(s)
                        break;
                    }
                }
            }
        }
    }
    l.lock();
    if(!solution_found){
        for(int j = 0; j < noliterals; j++){
        variablesS[j] = variables[j];
        }
    }
    l.unlock();
    solution_found = true;
}

void print_variables(variable* variables,int noliterals){
    cout << "---------\n";
    for(int i = 0; i < noliterals; i++){
        
        cout << "x" << i+1 << ": " << variables[i].value << " order: " << variables[i].order << endl;
    }
}

clause* get_clause(){
    clause* c = new clause;
    return c;
}