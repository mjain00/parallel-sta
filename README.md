# parallel-sta

# Parallel Static Timing Analysis - Final Project for 15618 Spring 2025

**URL:** [https://mjain00.github.io/parallel-sta/](https://mjain00.github.io/parallel-sta/)

## Summary
Our project focuses on performing **static timing analysis** on circuits to identify if they violate **setup and hold time constraints**. In this project, we’ll leverage **task-based parallelism** to efficiently provide fast and thorough analysis on complex circuitry.

## Proposal
Here is our project [proposal](proposal.pdf).

## Background:  
Our project builds on parallelizing static time analysis to efficiently analyze any time violations in circuits. The application, STA, is used to evaluate the correctness and performance of circuits.  The tool takes as input a netlist of gates (AND, OR, NOT) and the flip-flops between them. It then uses cell delay models to calculate gate delays. These gate delays include timing constraints like clock period, setup time, and hold time. The setup time ensures that the signal arrives before the clock edge by a specified margin. The hold time ensures that the data signal remains stable after the clock edge. If the slack of a path is negative, there’s a timing violation.

The algorithm is as follows: 
1. **Parse Netlist**
   - The circuit's netlist is parsed to extract components and connections.
2. **Graph Generation**
   - A **Directed Acyclic Graph (DAG)** is generated where:  
     - Nodes represent **gates**.  
     - Edges represent **delays** between gates.
3. **Topological Sorting**  
   - The DAG is sorted in **topological order** to ensure that a gate is processed only after all its dependencies.  
   - Sorting is performed using **Kahn’s algorithm** or **DFS traversal** to process the entire circuit path.
4. **Forward Pass (Arrival Time Calculation)**  
   - The algorithm calculates the **arrival time** at each node based on gate delays.  
   - This ensures that signals propagate correctly through the circuit.
5. **Backward Pass (Required Time Calculation)**  
   - The **required time** is calculated starting from the output nodes and working backward.  
   - Ensures that each gate meets **timing constraints** relative to the clock signal.
6. **Slack Calculation**  
   - Slack is determined as:  
     ```
     Slack = Required Time - Arrival Time
     ```
   - **Negative slack** indicates a **timing violation**, meaning the circuit does not meet timing constraints.

It’s a compute intensive application, especially for a large circuitry because it requires each path to be traversed in a sequential fashion with respect to its data dependencies (components that come before it). Additionally, each path has a different quantity of combinational logic, requiring more computations depending on the traversed path. 

Although this algorithm is extremely data dependent, this algorithm can still benefit from parallelism. We can have multiple independent nodes (paths that are independent of each other) processed at the same time.

We can have task-based parallelism where we can divide the path layers into individual tasks that can be executed in parallel. Additionally, we can create a task-graph where each task is a node and edges are dependencies. This will minimize idle time and improve performance. These are all avenues of parallelization our group will look at. 

## Challenges:
### Sequential Dependencies:  
The above algorithm is difficult to parallelize due to **sequential dependencies**. In static timing analysis, the **arrival time** of a node depends on all its predecessors, meaning **slack values** cannot be determined until all arrival times are calculated. Parallelizing this requires **synchronization** between tasks.

### Workload Imbalance:  
Multiple parallel tasks read/write shared data (same paths), which can lead to **cache contention** and **race conditions**. Additionally, some paths may have significantly more gates, causing **uneven workload distribution** across parallel tasks.

### Memory and Synchronization:  
To efficiently parallelize the algorithm, we need to store task graphs in memory while maintaining **temporal** and **spatial locality**. In the sequential version of the algorithm, we traverse the graph using **Depth First Search (DFS)**. For the parallel case, we can create a task for each branch and continue executing DFS on them.

The algorithm’s **high communication-to-computation ratio** is a challenge, as much of the execution depends on task synchronization and sharing timing values across paths.  

Given that we will be using a **shared memory system**, we need to ensure that updates to shared data are **thread-safe** and free of **race conditions**. Additionally, mapping portions of the graph to specific threads or cores is a non-trivial task. We must ensure that tasks are divided so that each thread works only on its own section of the graph and does not interfere with others until absolutely necessary.

## Resources:  

- **GHC Machines** for running the code.  

- **Paper 1 (Primary Source):**  
  Tsung-Wei Huang, Boyang Zhang, Dian-Lun Lin, and Cheng-Hsiang Chiu.  
  *Parallel and Heterogeneous Timing Analysis: Partition, Algorithm, and System.*  
  In Proceedings of the 2024 International Symposium on Physical Design (ISPD '24).  
  Association for Computing Machinery, New York, NY, USA, 51–59.  
  [DOI: 10.1145/3626184.3635278](https://doi.org/10.1145/3626184.3635278)

- **Paper 2:**  
  K. E. Murray and V. Betz, *Tatum: Parallel Timing Analysis for Faster Design Cycles and Improved Optimization,* 2018 International Conference on Field-Programmable Technology (FPT), Naha, Japan, 2018, pp. 110-117.  
  [DOI: 10.1109/FPT.2018.00026](https://ieeexplore.ieee.org/document/8742253)  
  Keywords: Timing, Optimization, Static Timing Analysis (STA), Parallel Algorithms, GPU, Multi-core CPU, FPGA, Computer-Aided Design (CAD).

- **Starter Code:**  
  We will use the **OpenTimer** library as inspiration for the sequential version of the code. This open-source implementation is based on a different paper but provides a useful reference for our project.
