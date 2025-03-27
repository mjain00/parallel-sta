# parallel-sta

# Parallel Static Timing Analysis - Final Project for 15618 Spring 2025

**URL:** [https://mjain00.github.io/parallel-sta/](https://mjain00.github.io/parallel-sta/)

## Summary
Our project focuses on performing **static timing analysis** on circuits to identify if they violate **setup and hold time constraints**. In this project, we’ll leverage **task-based parallelism** to efficiently provide fast and thorough analysis on complex circuitry.

## Proposal
Here is our project [proposal](proposal.pdf).

## BACKGROUND:  
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

Although this algorithm is extremely data dependent, this algorithm can still benefit from parallelism. We can have multiple independent nodes (paths that are independent of each other) processed at the same time. After the topological sorting, the slack calculation is embarrassingly parallel because the computation can be done completely in parallel.

We can have task-based parallelism where we can divide the path layers into individual tasks that can be executed in parallel. Additionally, we can create a task-graph where each task is a node and edges are dependencies. This will minimize idle time and improve performance. These are all avenues of parallelization our group will look at. 

