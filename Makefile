all:

	g++ -fopenmp verbose.cpp ASIC.cpp DAG.cpp sta_starter.cpp -o sta.o

