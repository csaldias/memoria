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
        cout << "Usage: " << argv[0] << " Index_dir XML_file [XML_file ...]" << endl;
        cout << "    This program creates a compressed index for each of the" << endl;
        cout << "    XML files (VOTable formatted) passed as parameters." << endl;
        cout << "    It is intended to be used by the indexer Python script." << endl;
        return 1;
    }

    size_t post_context  = 150;
    //size_t pre_context   = 5;
    size_t step          = 3;
    size_t row_step      = 20;

    high_resolution_clock::time_point t1, t2;
    
    string dir_prefix     = argv[1];
    string index_suffix   = ".sdsl";
    string colname_suffix = ".colname.dat";
    string memory_suffix  = ".html";

    csa_wt<wt_huff<>, 16, 16> fm_index;

    //We iterate over the filenames given to us as parameters
    for(size_t index = 2; index < argc; index++) {
	    //Let's open the file
	    string file_path  = string(argv[index]);
	    string file_name  = file_path.substr(file_path.find("/")+1);
	    string index_file = dir_prefix+file_name+index_suffix;
	    string mem_file   = "HTML/"+file_name.substr(0, file_name.find(".xml"));

	    ifstream data_file;
	    data_file.open(argv[index]);
	    if (!data_file.is_open()) {
	        cout << "ERROR: File " << argv[index] << " could not be opened. Exit." << endl;
	        return 1;
	    }

	    //Lets read the file
	    string line;
	    int start_pos, end_pos, i, c_count = 0;
	    vector<string> column_names;

	    if(VERBOSE) cout << "Reading file " << argv[index] << " ..." << endl;

	    while (getline(data_file, line) && (line.find("TABLEDATA") == string::npos)) {
	        //Is this a FIELD descriptor? (i.e, the column name)
	        if (line.find("<FIELD") != string::npos) {
	            start_pos = line.find("name=")+6;
	            end_pos = line.find("\"", start_pos);
	            //HERE, we should include col_name:col_desc, for the dispatcher later on
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
	    memory_monitor::start();
	    construct_im(fm_index, bwt_intermediate.str(), 1); // generate index
	    store_to_file(fm_index, index_file); // save it
    	memory_monitor::stop();

    	if (VERBOSE) cout << "Index construction complete, index requires " << size_in_mega_bytes(fm_index) << " MB." << endl;
    	if (VERBOSE) cout << "Peak memory usage = " << memory_monitor::peak() / (1024*1024) << " MB." << std::endl;
	    if (VERBOSE) cout << "Index has size " << fm_index.size() << "." << endl;

	    ofstream cstofs(mem_file+".html");
	    memory_monitor::write_memory_log<HTML_FORMAT>(cstofs);
	    cstofs.close();
	    util::clear(fm_index);
	}
}
