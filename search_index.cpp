#include <sdsl/suffix_arrays.hpp>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <vector>
#include <chrono>
#include <map>

#define VERBOSE 0

using namespace sdsl;
using namespace std;
using namespace std::chrono;

int main(int argc, char** argv) {
    if (argc <  3) {
        cout << "Usage " << argv[0] << " [query] [sdsl_file ...]" << endl;
        cout << "    This program attempts to parse the columns of a" << endl;
        cout << "    VOTable-based Compressed XML file(s)." << endl;
        return 1;
    }

    size_t post_context  = 150;
    //size_t pre_context   = 5;
    size_t step          = 3;
    size_t row_step      = 20;

    high_resolution_clock::time_point t1, t2;
    
    string index_suffix = ".sdsl";
    string index_file   = argv[2];
    string query        = argv[1];
    
    csa_wt<wt_huff<>, 16, 16> fm_index;

    load_from_file(fm_index, index_file);

    if (VERBOSE) cout << "Processing query \"" << query << "\"..." << endl;
    size_t m  = query.size();
    //Cuantas ocurrencias hay del substring?
    size_t occs = sdsl::count(fm_index, query.begin(), query.end());
    map<string, int> column_count;
    if (VERBOSE) cout << "# of occurrences: " << occs << endl;
    if (occs > 0) {
        auto start = high_resolution_clock::now();
        //Obtengamos el rango en la SA donde nuestro substring es prefijo
        auto interval = lex_interval(fm_index, query.begin(), query.begin()+m);
        if (VERBOSE) cout << "Interval of Ocurrences in SA = [" << interval[0] << ", " << interval[1] << "] (size " << interval[1]-interval[0]+1 << ")" << endl;
        if (VERBOSE) cout << "Obtaining column names..." << endl;
        size_t pos = interval[0];
        //Ahora, obtenemos los nombres de las columnas dentro del rango
        size_t iterations = 0;
        while (pos < interval[1]) {
            //Obtenemos el nombre de la columna del extremo izquierdo
            size_t right_border = post_context;
            size_t left_border  = 0;
            string s = extract(fm_index, fm_index[pos]+left_border, fm_index[pos]+right_border);
            while (s.find("\n") == string::npos) {
                left_border  += post_context+1;
                right_border += post_context+1;
                s.append(extract(fm_index, fm_index[pos]+left_border, fm_index[pos]+right_border));
            }
            size_t last_comma  = s.find_first_of('\n');
            size_t first_comma = s.find_last_of(',', last_comma-1)+1;
            string column_name = s.substr(first_comma, last_comma-first_comma);
            //cout << "Column name: " << column_name << endl;
            //En este punto, habremos obtenido un string de este estilo:
            //science observation with HIT slits.,tpl_name
            //Es este string el que usamos para afinar el rango inicial, y así
            //determinar cuántas veces ese dato aparece en la columna respectiva.
            //Dado que no existen "wildcards" para buscar por 2 prefijos (substring y columna), o una
            //función en la SDSL que me permita buscar por 2 prefijos, es la opción
            //más cercana (hasta este momento) para lograr el resultado.
            string column_query = s.substr(0, last_comma);
            //cout << "Query: " << column_query << endl;
            auto new_interval = lex_interval(fm_index, column_query.begin(), column_query.end());
            if (column_count.count(column_name) > 0) {
                column_count[column_name] += new_interval[1]-new_interval[0]+1;
            } else {
                column_count[column_name] = new_interval[1]-new_interval[0]+1;
            }
            //Continuamos iterando en la posición siguiente
            pos = new_interval[1]+1;
            iterations++;
            //cout << "New Interval = [" << new_interval[0] << ", " << new_interval[1] << "] (size " << new_interval[1]-new_interval[0]+1 << ")" << endl;
        }
        auto finish = high_resolution_clock::now();
        //Al terminar, entregamos un resumen de los resultados
        duration<double, milli> elapsed = finish - start;
        float ratio = iterations / column_count.size();
        if (VERBOSE) {
            //Show diagnostics data
            cout << "Took " << elapsed.count() << " milliseconds." << endl;
            cout << "Column names and frequencies:" << endl;
            for(auto& x : column_count) {
                cout << x.first << ": " << x.second << " hits." << endl;
            }
            cout << "Iterations:   " << iterations << endl;
            cout << "Column count: " << column_count.size() << endl;
            cout << "Ratio       : " << ratio << endl;
        } else {
            //Show results for this query/file
            stringstream output;
            output << "{";
            for(auto& x : column_count) {
                output << "\"" << x.first << "\":" << x.second << ",";
            }
            output.seekp(-1,output.cur);
            output << '}';
            cout << output.str() << endl;
        }
    } else {
        cout << "{}" << endl;
    }
}
