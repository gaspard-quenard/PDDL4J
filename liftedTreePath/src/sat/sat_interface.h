
#ifndef DOMPASCH_LILOTANE_SAT_INTERFACE_H
#define DOMPASCH_LILOTANE_SAT_INTERFACE_H

#include <initializer_list>
#include <fstream>
#include <string>
#include <iostream>
#include <assert.h>
#include <vector>
#include <chrono>

#include "util/params.h"
#include "util/log.h"
#include "sat/variable_domain.h"
#include "sat/encoding_statistics.h"

extern "C" {
    #include "sat/ipasir.h"
}

class SatInterface {

private:
    Parameters& _params;
    void* _solver;
    std::ofstream _out;
    EncodingStatistics& _stats;

    const bool _print_formula;    
    bool _began_line = false;

    const bool _is_used;

    const int _seed; 

    const int _debug_level = 0;

    std::vector<int> _last_assumptions;
    std::vector<int> _no_decision_variables;

public:
    SatInterface(bool is_used, Parameters& params, EncodingStatistics& stats) : 
                _is_used(is_used), _params(params), _stats(stats), _print_formula(params.isNonzero("wf")), _seed(params.getIntParam("s")) {

        _solver = ipasir_init();

        if (!is_used) {
            _stats.is_used = false;
            return;
        }
        ipasir_set_seed(_solver, params.getIntParam("s"));
        if (true) _out.open("formula.cnf");
    }
    
    inline void addClause(int lit) {
        assert(lit != 0);
        ipasir_add(_solver, lit); ipasir_add(_solver, 0);
        if (_print_formula) _out << lit << " 0\n";

        if (_debug_level >= 1) std::cout << "SAT: " << lit << " 0" << std::endl;

        _stats._num_lits++; _stats._num_cls++;
    }
    
    inline void addClause(int lit1, int lit2) {
        assert(lit1 != 0);
        assert(lit2 != 0);
        ipasir_add(_solver, lit1); ipasir_add(_solver, lit2); ipasir_add(_solver, 0);
        if (_print_formula) _out << lit1 << " " << lit2 << " 0\n";
        if (_debug_level >= 1) std::cout << "SAT: " << lit1 << " " << lit2 << " 0" << std::endl;
        _stats._num_lits += 2; _stats._num_cls++;
    }
    inline void addClause(int lit1, int lit2, int lit3) {
        assert(lit1 != 0);
        assert(lit2 != 0);
        assert(lit3 != 0);
        ipasir_add(_solver, lit1); ipasir_add(_solver, lit2); ipasir_add(_solver, lit3); ipasir_add(_solver, 0);
        if (_print_formula) _out << lit1 << " " << lit2 << " " << lit3 << " 0\n";
        if (_debug_level >= 1) std::cout << "SAT: " << lit1 << " " << lit2 << " " << lit3 << " 0" << std::endl;
        _stats._num_lits += 3; _stats._num_cls++;
    }
    inline void addClause(const std::initializer_list<int>& lits) {
        if (_debug_level >= 1) {
            std::cout << "SAT: ";
        }
        for (int lit : lits) {
            assert(lit != 0);
            ipasir_add(_solver, lit);
            if (_print_formula) _out << lit << " ";
            if (_debug_level >= 1) std::cout << lit << " ";
        } 
        ipasir_add(_solver, 0);
        if (_print_formula) _out << "0\n";
        if (_debug_level >= 1) std::cout << "0" << std::endl;
        _stats._num_cls++;
        _stats._num_lits += lits.size();
    }
    inline void addClause(const std::vector<int>& cls) {

        if (_debug_level >= 1) {
            std::cout << "SAT: ";
        }
        for (int lit : cls) {
            assert(lit != 0);
            ipasir_add(_solver, lit);
            if (_print_formula) _out << lit << " ";
            if (_debug_level >= 1) std::cout << lit << " ";
        } 
        ipasir_add(_solver, 0);
        if (_print_formula) _out << "0\n";
        if (_debug_level >= 1) std::cout << "0" << std::endl;
        _stats._num_cls++;
        _stats._num_lits += cls.size();
    }
    inline void appendClause(int lit) {
        _began_line = true;
        assert(lit != 0);
        ipasir_add(_solver, lit);
        if (_print_formula) _out << lit << " ";
        if (_debug_level >= 1) std::cout << lit << " ";
        _stats._num_lits++;
    }
    inline void appendClause(int lit1, int lit2) {
        _began_line = true;
        assert(lit1 != 0);
        assert(lit2 != 0);
        ipasir_add(_solver, lit1); ipasir_add(_solver, lit2);
        if (_print_formula) _out << lit1 << " " << lit2 << " ";
        if (_debug_level >= 1) std::cout << lit1 << " " << lit2 << " ";
        _stats._num_lits += 2;
    }
    inline void appendClause(const std::initializer_list<int>& lits) {
        _began_line = true;
        for (int lit : lits) {
            assert(lit != 0);
            ipasir_add(_solver, lit);
            if (_print_formula) _out << lit << " ";
            if (_debug_level >= 1) std::cout << lit << " ";
            //log("%i ", lit);
        } 

        _stats._num_lits += lits.size();
    }
    inline void endClause() {
        assert(_began_line);
        ipasir_add(_solver, 0);
        if (_print_formula) _out << "0\n";
        if (_debug_level >= 1) std::cout << "0" << std::endl;
        //log("0\n");
        _began_line = false;

        _stats._num_cls++;
    }
    inline void assume(int lit) {
        if (_stats._num_asmpts == 0) _last_assumptions.clear();
        ipasir_assume(_solver, lit);
        //log("CNF !%i\n", lit);
        _last_assumptions.push_back(lit);
        _stats._num_asmpts++;
    }

    inline void reset_solver() {
        ipasir_release(_solver);

        _solver = ipasir_init();
        ipasir_set_seed(_solver, _seed);
    }

    inline bool holds(int lit) {
        return ipasir_val(_solver, lit) > 0;
    }

    inline bool didAssumptionFail(int lit) {
        return ipasir_failed(_solver, lit);
    }

    bool hasLastAssumptions() {
        return !_last_assumptions.empty();
    }

    void setTerminateCallback(void * state, int (*terminate)(void * state)) {
        ipasir_set_terminate(_solver, state, terminate);
    }

    void setLearnCallback(int maxLength, void* state, void (*learn)(void * state, int * clause)) {
        ipasir_set_learn(_solver, state, maxLength, learn);
    }

    int solve() {
        auto start = std::chrono::high_resolution_clock::now();
        int result = ipasir_solve(_solver);
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
        long int time_ms = duration.count();
        _stats.time_spend_on_solver_per_layer_ms.push_back(time_ms);
        _stats.total_time_spend_on_solver_ms += time_ms;
        if (_stats._num_asmpts == 0) _last_assumptions.clear();
        _stats._num_asmpts = 0;
        return result;
    }

    ~SatInterface() {

        if (!_is_used) {
            return;
        }

        if (_params.isNonzero("wf")) {

            for (int asmpt : _last_assumptions) {
                _out << asmpt << " 0\n";
            }
            _out.flush();
            _out.close();

            // Create final formula file
            std::ofstream ffile;
            ffile.open("f.cnf");
            
            // Append header to formula file
            ffile << "p cnf " << VariableDomain::getMaxVar() << " " << (_stats._num_cls+_last_assumptions.size()) << "\n";

            // Append main content to formula file (reading from "old" file)
            std::ifstream oldfile;
            oldfile.open("formula.cnf");
            std::string line;
            while (std::getline(oldfile, line)) {
                line = line + "\n";
                ffile.write(line.c_str(), line.size());
            }
            oldfile.close();
            // remove("formula.cnf");

            // Finish
            ffile.flush();
            ffile.close();
        }

        // Release SAT solver
        ipasir_release(_solver);
    }
};

#endif
