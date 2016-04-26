# MNLR
MNLR

Input Specifications:

Here is the process that we use when we run the program.

Format:

programName  -T  < tier_addr1   tier_addr2 ….  tier_addrn>  
		 -N  < indicator  [ IP_addr1  CIDR1  Port1  …  IP_addrn CIDRn  Portn ] > 

	Note:

-T specifies the tier address followed by atleast 1 tier address
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

            ./a.out.  -T  1.2   -N   0  10.1.1.1   29    eth2 or 
            ./a.out   -N 0   10.1.1.1   29   eth2   -T   1.2

Note:
	If you have source code and want to recompile it again you can use the below command
	gcc *.c  -lm


Examples:

Example 1:

./a.out   -T    2.3.4   -N   0   10.1.1.1   29   eth2


./a.out
-T
 Tier Address
-N
Indicator
IP Addr
CIDR
Interface 
name
program name
switch
2.3:4
switch
0
10.1.1.1
29
eth2


Example 2:

./a.out   -T  2.3.4  2.4.1  -N  1


Example 3:

./a.out input.txt

// Contents of input.txt
line 1: ./a.out   -T  2.3.4  2.4.1  -N  1

Example 4:

./a.out   -T    2.3.4   -N   0   10.1.1.1   24   eth2    10.2.1.1    24   eth4


Detailed Example


Aim:
	ping 10.1.2.3 from 10.1.3.2

Observation:
	Node 0 and Node 4 are part of IP domain only. We do not run MPLR code on them.

How to run code:
	
	At node 1:
		./a.out    -T   1.1    -N    0    10.1.3.2    24   eth4

	At node 2:
		./a.out   -T   1.2    -N    1

	At node 3:
		./a.out    -T   1.3    -N    0    10.1.2.3    24   eth4
	
