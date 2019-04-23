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
        cout << "Usage " << argv[0] << " [XML_file] [query]" << endl;
        cout << "    This program attempts to parse the columns of a" << endl;
        cout << "    VOTable-based XML file." << endl;
        return 1;
    }

    size_t post_context  = 150;
    //size_t pre_context   = 5;
    size_t step          = 3;
    size_t row_step      = 20;

    high_resolution_clock::time_point t1, t2;
    
    string index_suffix = ".sdsl";
    string index_file   = string(argv[1])+index_suffix;
    csa_wt<wt_rlmn<>, 2048, 2048> fm_index;

    if (!load_from_file(fm_index, index_file)) {
        if (VERBOSE) cout << "No index " << index_file << " located. Building index now." << endl;

        //Let's open the file
        ifstream data_file;
        data_file.open(argv[1]);
        if (!data_file.is_open()) {
            cout << "ERROR: File " << argv[1] << " could not be opened. Exit." << endl;
            return 1;
        }

        //Lets read the file
        string line;
        int start_pos, end_pos, i, c_count = 0;
        vector<string> column_names;

        if(VERBOSE) cout << "Reading file " << argv[1] << " ..." << endl;

        while (getline(data_file, line) && (line.find("TABLEDATA") == string::npos)) {
            //Is this a FIELD descriptor? (i.e, the column name)
            if (line.find("<FIELD") != string::npos) {
                start_pos = line.find("name=")+6;
                end_pos = line.find("\"", start_pos);
                column_names.push_back(line.substr(start_pos, end_pos-start_pos));
            }
        }

        if (VERBOSE) cout << "Found " << column_names.size() << " columns." << endl;

        vector<string> lines;

        //Now, we append the corresponding column info to each data
        //For this version, we´ll be storing each line in an array, so we can
        //lexicographically sort them all later.
        if (VERBOSE) cout << "Writing intermediate to memory..." << endl;

        //First of all, we'll get the frequency of all the data contained in the table
        while (getline(data_file, line) && line.find("</TABLEDATA>") == string::npos){
            // If this is the beggining of the row, we reset the counter
            if (line.find("<TR>") != string::npos) {
            c_count = 0;
          } else if (line.find("<TD>") != string::npos) {
            size_t start_pos = line.find("<TD>")+4;
            size_t end_pos   = line.find("</TD>");
            lines.push_back(line.substr(start_pos, end_pos-start_pos)+","+column_names[c_count]);
            c_count++;
          }
        }

        data_file.close();
        sort(lines.begin(), lines.end());

        //ofstream intermediate_file;
        stringstream bwt_intermediate;
        //intermediate_file.open("intermediate.xml");
        for (size_t i = 0; i < lines.size(); i++) {
            //intermediate_file << lines[i] << "\n";
            bwt_intermediate << lines[i] << "\n";
        }
        //intermediate_file.close();

        if (VERBOSE) cout << lines.size() << " elements in memory." << endl;

        //Now, we compress the intermediate and store its compressed
        //representation into disk

        construct_im(fm_index, bwt_intermediate.str(), 1); // generate index
        store_to_file(fm_index, index_file); // save it
    }
    
    if (VERBOSE) cout << "Index construction complete, index requires " << size_in_mega_bytes(fm_index) << " MiB." << endl;
    if (VERBOSE) cout << "Index has size " << fm_index.size() << "." << endl;

    string query = argv[2];
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
            //while (s.find("\n") == string::npos) {
            //    left_border  += post_context+1;
            //    right_border += post_context+1;
            //    s.append(extract(fm_index, fm_index[pos]+left_border, fm_index[pos]+right_border));
            //}
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
            cout << "Took " << elapsed.count() << " milliseconds." << endl;
            cout << "Column names and frequencies:" << endl;
            for(auto& x : column_count) {
                cout << x.first << ": " << x.second << " hits." << endl;
            }
            cout << "Iterations:   " << iterations << endl;
            cout << "Column count: " << column_count.size() << endl;
            cout << "Ratio       : " << ratio << endl;
        } else {
            cout << elapsed.count() << endl;
        }
    }
}
