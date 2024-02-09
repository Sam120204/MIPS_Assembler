<div align="center">
  <img src="">
  <br>
</div>

Architecture Coursework
==========================

The code can be found  <a href="https://github.com/gioannides/EIE2-MIPS_SIMULATOR">here</a>.
-------------------------------------------------------------------------------------------

The task was to develop a MIPS CPU simulator, which can accurately execute
MIPS-1 big-endian binaries. Also a testbench was developed which is able to 
test a MIPS simulator, and try to work out whether it is correct.

Terminology
-----------

The Simulator is a single executable, and has the following behaviour:

- *Binary* : the Binary location is passed as a command-line parameter, and
  should be the path of a binary file containing MIPS-1 big-endian instructions.
  These instructions should be loaded into a fixed region of "RAM" with a known
  address, then execution should start at the first address in this region.

- *Input* : input to the simulated Binary will be passed in over the Simulator's
  standard input (`std::cin` or `stdin`), and mapped into a 32-bit memory location.
  If the Binary reads from the nominated memory location, it should be logically equivalent
  to calling `std::getchar` or `getchar` (and one approach would be for the Simulator
  to call these functions on behalf of the Binary).

- *Exceptions* : The Binary may execute instructions which are illegal, and
  so result in exceptions which should terminate execution of the Binary. To
  indicate this, the Simulator should return one of the negative exit codes
  detailed later on.
  
- *Errors* : Errors may occur within the Simulator (as opposed to exceptions
  which are due to part of the Binary's logic). Examples might include instructions
  which aren't implemented (limited functionality in the Simulator), or IO failures
  (problems which occur due to run-time interactions between the Simulator and the Environment).


Simulator build and execution
-----------------------------

The meaning of the fields is as follows:

- `TestId` : A unique identifier for the particular test. This can be composed
  of the characters `0-9`, `a-z`, `A-Z`, `-`, or `_`. So for example, ascending
  integers would be fine, or combinations of words and integers, as long as there
  are no spaces. Running the test-bench twice should produce the same set of
  test identifiers in the same order, and this reflects the order in which
  tests are executed.

- `Instruction` : This identifies the instruction which is the _primary_
  instruction being tested. Note that many (actually, most) instructions are
  impossible to test in isolation, so a given test may fail either because
  the instruction under test doesn't work, or because some other instruction
  necessary for the tests is broken. The test should be written to be particularly
  sensitive to the instruction under test, so it looks for a failure mode of
  that particular instruction.


Memory-Map
----------

The memory map of the simulated process is as follows:

```
Offset     |  Length     | Name       | R | W | X | Cached |
-----------|-------------|------------|---|---|---|--------|--------------------------------------------------------------------
0x00000000 |        0x4  | ADDR_NULL  |   |   | Y |        | Jumping to this address means the Binary has finished execution.
0x00000004 |  0xFFFFFFC  | ....       |   |   |   |        |
0x10000000 |  0x1000000  | ADDR_INSTR | Y |   | Y |      Y | Executable memory. The Binary should be loaded here.
0x11000000 |  0xF000000  | ....       |   |   |   |        |
0x20000000 |  0x4000000  | ADDR_DATA  | Y | Y |   |      Y | Read-write data area. Should be zero-initialised.
0x24000000 |  0xC000000  | ....       |   |   |   |        |
0x30000000 |        0x4  | ADDR_GETC  | Y |   |   |        | Location of memory mapped input. Read-only.
0x30000004 |        0x4  | ADDR_PUTC  |   | Y |   |        | Location of memory mapped output. Write-only.
0x30000008 | 0xCFFFFFF8  | ....       |   |   |   |        |
-----------|-------------|------------|---|---|---|--------|--------------------------------------------------------------------
```

Instructions
------------

Instructions of interest are:

Code    |   Meaning                                   | Complexity  
--------|---------------------------------------------|-----------
ADD     |  Add (with overflow)                        | 2  XX       
ADDI    |  Add immediate (with overflow)              | 2  XX              
BEQ     |  Branch on equal                            | 3  XXX          
BNE     |  Branch on not equal                        | 3  XXX      
DIV     |  Divide                                     | 4  XXXX     
DIVU    |  Divide unsigned                            | 4  XXXX     
J       |  Jump                                       | 3  XXX      
JALR    |  Jump and link register                     | 4  XXXX     
LW      |  Load word                                  | 2  XX       
MFHI    |  Move from HI                               | 3  XXX     
MFLO    |  Move from LO                               | 3  XXX        
MULT    |  Multiply                                   | 4  XXXX     
MULTU   |  Multiply unsigned                          | 4  XXXX     
OR      |  Bitwise or                                 | 1  X         
SLT     |  Set on less than (signed)                  | 2  XX          
SLTU    |  Set on less than unsigned                  | 1  X        
SUB     |  Subtract                                   | 2  XX       
SW      |  Store word                                 | 2  XX       
--------|---------------------------------------------|---------
INTERNAL|  Not associated with a specific instruction |
FUNCTION|  Testing the ability to support functions   |
STACK   |  Testing for functions using the stack      |
