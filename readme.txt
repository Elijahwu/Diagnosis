Please type "make" under directory src to generate the executable
The executable is under the directory src

The command of generating fail log is as follow:
./atpg –genFailLog <pattern file>  <circuit file> -fault <wire> <gate> <io> <fault type>

For example, if we want to generate a failog with fault(10GAT(6), g2, GO, SA1),
./atpg -genFailLog ../patterns/golden_c17.ptn ../sample_circuits/c17.ckt -fault 10GAT"(6)" g2 GO SA1
Please note that we add quotes on the parentheses to prevent syntax error.

Multiple fault are also supported,
./atpg -genFailLog ../patterns/golden_c17.ptn ../sample_circuits/c17.ckt -fault 11GAT"(5)" g1 GO SA1 -fault 16GAT"(8)" g5 GI SA1

The command of diagnosis is as follow:
./atpg -diag <pattern file>  <circuit file> <fail log file>

For example,
./atpg -diag ../patterns/golden_c17.ptn ../sample_circuits/c17.ckt  ./failLog/c17-001.failLog

The format of daignosis is as follow:
Example 1:
********
No.1 22GAT g6 GO SA1, groupID=1, perfect_match_score=7, single_match_score=12 [ equivalent faults: 10GAT g2 GO SA0, 16GAT g6 GI SA0, ]
No.2 7GAT dummy_gate5 GO SA1, groupID=2, perfect_match_score=8, single_match_score=13 [ equivalent faults: ]
No.3 2GAT dummy_gate2 GO SA1, groupID=2, perfect_match_score=8, single_match_score=13 [ equivalent faults: ]
********
The number of groupID means ranking, faults in same score arrange in same goupID. We show the top 5 groupID.

Example 2:
********
No.1 7GAT dummy_gate5 GO SA1, perfectly match!!! [ equivalent faults: ]
********
If the faulty response can perfectly explained by only one fault, we show only one fault and remark it as a perfectly match fault.

