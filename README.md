# RaidEncoding
A simple GitHub repo to store old C files pertaining to Raid encoding.

Two programs: raid.c and diar.c. Raid.c performs RAID 2 encoding (utilizing Hamming(7,4) code) of a given '.txt' file accepted as
a command-line argument. Instead of separating the 7 bits into 7 different discs, we simulate the discs as 7 different .txt output files.
Diar.c performs the decoding of the RAID 2 encoding; the program accepts the name of the original file which got split into 7 different
files, and recreates the contents of the original encoded file based on its separated counterparts.
