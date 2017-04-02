# MNLR
MNLR

Input Specifications:

Here is the process that we use when we run the program.

Format:

programName  -T  <Tier Level>     -L  <Node Label>     -N  < indicator  [ IP_addr1  CIDR1  Port1  …  IP_addrn CIDRn  Portn ] > 

	Note:

-T specifies the tier level
-L specifies the node label
-N specifies the end network information (IP Address, CIDR, interface name) followed by atleast 1 entries.
-V specifies the version number of the code
Indicator is used to denote whether this node is Edge Node or not. (possible values: 0,1)
0 indicates it is an edge node.
1 indicates it is not an edge node so we dont need to store IP address, CIDR and interface name.
./a.out -V specifies the version of code, compiler and OS.
No maximum limit on tier address and end network entries.
All input validation is done. It will throw error if input is not proper
input parameters can be specified in any order.  
            Example

            ./MNLR  -T  1 -L 1.1  -N   1				--> For nodes at Tier 1
            ./MNLR  -T  2 -N  1							--> For intermediate nodes at Tier n > 1
            ./MNLR  -T  2 -N  0  10.1.1.1   24    eth2	--> For edge nodes at Tier n > 1    

Note:

	If you have source code and want to recompile it again you can use the below command

	gcc  -o MNLR *.c -lm

Examples:

Example 1: (For nodes at Tier 1) 

./MNLR  -T  1   -L 1.1  -N   1         			


Example 2: (For intermediate nodes at Tier n > 1)

./MNLR  -T  2   -N  1 


Example 3: (For edge nodes at Tier n > 1 )

./MNLR  -T  2   -N  0    10.1.1.1     24     eth2 


Example 4:

./MNLR -T  5    -N  0    10.1.1.1     24     eth2  10.2.1.1  24  eth4


	
