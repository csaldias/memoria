all : indexer searcher
.PHONY : all

indexer : create_index.cpp
	g++ -std=c++11 -O3 -DNDEBUG -I ~/include -L ~/lib create_index.cpp -o create_index -lsdsl -ldivsufsort -ldivsufsort64

searcher : search_index.cpp
	g++ -std=c++11 -O3 -DNDEBUG -I ~/include -L ~/lib search_index.cpp -o search_index -lsdsl -ldivsufsort -ldivsufsort64

clean:
	rm search_index create_index