# PageRank Algorithm

## 📝 Description

C implementation of the PageRank algorithm, developed as part of the *Laboratorio 2* course at the *University of Pisa*, under the supervision of *Professor Giovanni Manzini*.  
Implemented using multithreading, producer-consumer with a shared buffer mechanism and thread-based computation. It begins by reading a directed graph from an input file, parsing nodes and edges information to build an internal graph structure. The PageRank values of the graph are computed iteratively. A set of threads is launched to independently calculate the PageRank values for different node ranges. The process continues until either convergence is reached within a specified error threshold or a maximum number of iterations is completed. The algorithm as a result shows the number of nodes and edges (included dead-end nodes), the number of iterations performed, the final sum of ranks and the top K nodes with the highest PageRank scores.  
PageRank has a wide range of real-world applications: nodes in the graph may represent webpages or data, while edges represent links, citations, or relationships between them; the algorithm was famously used by Google to rank web search results based on the importance of webpages.

---

## 📑 Index

- [Features](#features)
- [Input Format](#input-format)
- [Usage](#usage)
- [Parameters](#parameters)
- [Expected Output](#expected-output)

---

## 🚀 Features

- 🔀 Parallel computation
- 🔤 Supports space and comma delimited `.mtx` files
- 🔧 Handles dead-end nodes
- ⚙️ Configurable parameters

---

## 📥 Input Format

The program accepts directed graphs in **Matrix Market format (`.mtx`)**, supporting the following syntax:

```text
%%MatrixMarket matrix coordinate pattern general
% Comments
N N E     % nodes, nodes, edges
1 2       % edge from nodes 1 → 2
3 1       % edge from nodes 3 → 1

% or

N,N E     % nodes,nodes, edges
2,4       % edge from nodes 2 → 4
5,3       % edge from nodes 5 → 3
...
```

---

## ▶️ Usage

```bash
make
```

```bash
./pagerank [OPTIONS] <input_file>.mtx
```

---

## 🧩 Parameters

| Option | Description                          | Default Value |
|--------|--------------------------------------|----------------|
| `-k`   | Number of top nodes to display       | `3`            |
| `-m`   | Maximum number of iterations         | `100`          |
| `-d`   | Damping factor                       | `0.9`          |
| `-e`   | Convergence threshold                | `1e-7`         |
| `-t`   | Number of auxiliary threads to use   | `3`            |

---

## 📊 Expected Output

Example: 
```bash
Number of nodes: 916428
Number of dead-end nodes: 176974
Number of valid arcs: 5105039
Converged after 62 iterations / Did not converge after 62 iterations
Sum of ranks: 1.0000
Top 3 nodes:
597621 0.000915
41909 0.000912
163075 0.000895
```

---

## ⚠️ Platform Compatibility

This program was developed and used in **Linux**, may have incompatibilities on Windows or macOS.

