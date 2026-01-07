/* Author: Steven Carr
   Program Created: October 2023
A program to decode RAID 2 encoding.  It looks for seven separated files based off
the file given as a command-line argument (which represents the 7 disks normally for RAID 2)
and recreates the Hamming(7,4) code and reconstructs the original file that was encoded using RAID 2.*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>

// Function to create a byte from the 8 bits, given from two nibbles of data bits from
// reconstructed hamming code
unsigned char createByte(unsigned char byte, unsigned char givenBit[]){
    for (int i = 0; i < 8; i++){
        byte |= (givenBit[i] << (7 - i));
    }
    return byte;
}

// Function to split the given byte into 2 nibbles; I pass the nibble array into the function,
// and the function accepts it as a pointer, which I believe saves it's first address space
// so that way the array is altered based on it's memory location (I think at least, I know it works)
void splitByte(unsigned char readByte, unsigned char *nibble){
    nibble[0] = (readByte >> 4) & 0x0F;
    nibble[1] = readByte & 0x0F;
}

// Function to split the given hamming code bit nibble into their respective bits; takes both arrays
// as unsigned char pointers for the reasons stated above
// For some reason, I had to change the bitwise operations for the left nibble to be the same as the
// right nibble, even though when encoding they had opposite operations.
void splitNibble(unsigned char *nibble, unsigned char *bits){
    bits[0] = (nibble[0] >> 3) & 1;
    bits[1] = (nibble[0] >> 2) & 1;
    bits[2] = (nibble[0] >> 1) & 1;
    bits[3] = nibble[0] & 1;

    bits[4] = (nibble[1] >> 3) & 1;
    bits[5] = (nibble[1] >> 2) & 1;
    bits[6] = (nibble[1] >> 1) & 1;
    bits[7] = nibble[1] & 1;
}

void raidTwoDecoding(char *filename, FILE *encodedFiles[], int numberOfBytes){
    // Initialize the output file as (whateverfilename).txt.2 and create it
    char *newFileName = strcat(filename, ".2");
    FILE *recreatedOutputFile = fopen(newFileName, "w+");

    // Check to see if we've reached the end of the loop
    int endCheck = 0;

    // Initialize a tracker for the amount of bytes read total.  Number of bytes
    // command-line value should be the amount of bytes we are decoding.  Once
    // the amount of bytes read matches the number of bytes to decode, break
    // (unless we reach EOF beforehand)
    int bytesRead = 0;

    while (1){
        // Initialize an unsigned char for reading a byte from each of the 7 files
        uint8_t readByte[7];

        // Read a byte from each of the 7 files
        for (int i = 0; i < 7; i++){
            // If fread is 0, then there are no more bytes to read, set endCheck to 1 to break
            if (fread(&readByte[i], 1, 1, encodedFiles[i]) == 0){
                endCheck = 1;
                break;
            }
        }

        // If endCheck is flagged true, then break the while loop
        if (endCheck == 1){
            break;
        }

        // Initialize the reconstructed hamming code
        unsigned char hammingCode[7];

        // Initialize the nibbles for each byte corresponding to the respective hamming code bit
        unsigned char nibbleP1[2], nibbleP2[2], nibbleD3[2], nibbleP4[2], nibbleD5[2], nibbleD6[2], nibbleD7[2];

        // Split the byte from each file and create the respective nibbles
        splitByte(readByte[0], nibbleP1);
        splitByte(readByte[1], nibbleP2);
        splitByte(readByte[2], nibbleD3);
        splitByte(readByte[3], nibbleP4);
        splitByte(readByte[4], nibbleD5);
        splitByte(readByte[5], nibbleD6);
        splitByte(readByte[6], nibbleD7);

        // Initialize the individual bits for each hamming code bit (file)
        unsigned char bitsP1[8], bitsP2[8], bitsD3[8], bitsP4[8], bitsD5[8], bitsD6[8], bitsD7[8];

        // Split every nibble into their individual bits
        splitNibble(nibbleP1, bitsP1);
        splitNibble(nibbleP2, bitsP2);
        splitNibble(nibbleD3, bitsD3);
        splitNibble(nibbleP4, bitsP4);
        splitNibble(nibbleD5, bitsD5);
        splitNibble(nibbleD6, bitsD6);
        splitNibble(nibbleD7, bitsD7);

        // Initialize a variable to keep track of the amount of nibbles that have been processed
        // When two nibbles have been processed, we have a byte and should output to file
        int nibbleCount = 0;

        // Initialize an array of the bits we're going to output to the file; it should be two nibbles
        // of data bits, 1 byte
        unsigned char outputDataBits[8];

        // For loop to recreate the hamming code, and then extract the data bits from the hamming code
        // and recreate the original file
        for (int i = 0; i < 8; i++){
            // The recreated hamming code is of the order: P1, P2, D3, P4, D5, D6, D7; I order them with
            // respect to their bit position, so D3 is the first data bit, D5 is the second, D6 the third
            // and D7 the fourth
            hammingCode[0] = bitsP1[i];
            hammingCode[1] = bitsP2[i];
            hammingCode[2] = bitsD3[i];
            hammingCode[3] = bitsP4[i];
            hammingCode[4] = bitsD5[i];
            hammingCode[5] = bitsD6[i];
            hammingCode[6] = bitsD7[i];

            // Initialize parity bits for the error check
            unsigned char testP1, testP2, testP4;

            // To detect an error, we XOR each data bit in accordance with Hamming(7,4).  Thus, we XOR
            // P1 with D3, D5, D7; P2 with D3, D6, D7; and P4 with D5, D6, D7
            testP1 = hammingCode[0] ^ hammingCode[2] ^ hammingCode[4] ^ hammingCode[6];
            testP2 = hammingCode[1] ^ hammingCode[2] ^ hammingCode[5] ^ hammingCode[6];
            testP4 = hammingCode[3] ^ hammingCode[4] ^ hammingCode[5] ^ hammingCode[6];

            // Keep track of the amount of errors detected
            int errorTracker = 0;

            // If any of the parity bits for testing equal 1, then that means there is an error present.  For P1, add
            // 1 to the error tracker if there is an error.  For P2, add 2, and for P4, add 4. (assuming only 1 bit
            // at a time encounters an error).
            if (testP1 == 1){
                errorTracker++;
            }
            if (testP2 == 1){
                errorTracker = errorTracker + 2;
            }
            if (testP4 == 1){
                errorTracker = errorTracker + 4;
            }

            // Switch case for the error tracker; if the error tracker is 0, then there are no errors, and
            // simply break the switch cases and continue.  If it is 1, then D3 (D1) is the error and flip the
            // bit.  If it is 2, then D5(D2) is the error and flip the bit, and finally if it is 4 then D7(D4)
            // is the error and flip the bit.
            switch (errorTracker)
            {
            case 0:
                break;
            
            case 1:
                hammingCode[2] = ~hammingCode[2];
                break;

            case 2:
                hammingCode[4] = ~hammingCode[4];
                break;

            case 4:
                hammingCode[6] = ~hammingCode[6];
                break;
            }
            


            // Switch case for nibble count; if nibble count is 0, then put the first nibble (left) into the first 4 a spaces
            // of the output bit array; if its 1, then output into the second 4 spaces (right nibble)
            switch (nibbleCount)
            {
            case 0:
            // We're outputting the data bits
            // For some reason, I found the first nibble is always reversed.  I believe I tried to change my bitwise operations
            // in my splitNibble function, but that didn't fix it.  Instead, I am assigning the bits for the first nibble in the
            // reverse order to account for this issue.
                outputDataBits[0] = hammingCode[6];
                outputDataBits[1] = hammingCode[5];
                outputDataBits[2] = hammingCode[4];
                outputDataBits[3] = hammingCode[2];
                nibbleCount++;
                break;
            
            case 1:
                outputDataBits[4] = hammingCode[2];
                outputDataBits[5] = hammingCode[4];
                outputDataBits[6] = hammingCode[5];
                outputDataBits[7] = hammingCode[6];
                nibbleCount++;
                break;
            }

            // If two nibbles have been processed, we have a byte.  Output to the recreated file
            if (nibbleCount == 2){
                // Initialize a variable to recreate the byte, then create the byte
                unsigned char byte = 0;
                byte = createByte(byte, outputDataBits);

                // Output to the new recreated file
                fwrite(&byte, sizeof(unsigned char), 1, recreatedOutputFile);
                // Reset the nibble count and loop
                nibbleCount = 0;
            }
        }

        // Increment the amount of bytes read by 4.  We technically recreate the file 4 bytes at a time,
        // because all of the data bits within the 1 byte being read at a time from each of the 7 files
        // equates to 4 bytes actually being read/written to the file.
        bytesRead = bytesRead + 4;

        // If the amount of bytes read is equal to the number of bytes to decode, stop decoding and break
        if (bytesRead == numberOfBytes){
            break;
        }
    }

    // Close the recreated output file
    fclose(recreatedOutputFile);
}

int main(int argc, char* argv[]){
    // Command-line argument check for the proper arguments
    if (argc < 5){
        printf("Ensure you put a signifier, the filename, another signifier and the number of bytes to decode.\n");
        exit(1);
    }

    // Initialize the name of the file we want to recreate
    char *filename = NULL;
    int numberOfBytes = 0;

    // Make sure "-f" is present as a command-line argument followed by the filename to recreate
    if (strcmp(argv[1], "-f") == 0){
        filename = argv[2];
    } else {
        // Otherwise throw out an error
        printf("Ensure you put a '-f' before the name of the file we are recreating.\n");
        exit(1);
    }

    // Make sure "-s" is present as a command-line argument followed by the number of bytes to decode
    if (strcmp(argv[3], "-s") == 0){
        // atoi() is a function that converts a string into an integer
        numberOfBytes = atoi(argv[4]);
    } else {
        // Otherwise throw out an error
        printf("Ensure you put a '-s' before the number of bytes we will be decoding.\n");
        exit(1);
    }

    // Get the length of the filename for array initialization
    int nameLength = strlen(filename);

    // Initialize the names of the encoded files to the size of the original file's name length
    // there should be 7 separate encoded files
    char encodedFileNames[7][nameLength];
    FILE *separatedFiles[7];

    // For all of the separated encoded files (7 of them), make the name of the files we're searching for
    // the original input file's name with the extension ".part#", and then open each respective file
    for (int i = 0; i < 7; i++){
        sprintf(encodedFileNames[i], "%s.part%d", filename, i);
        separatedFiles[i] = fopen(encodedFileNames[i], "r");
    }

    // Decode the 7 separated encoded files
    raidTwoDecoding(filename, separatedFiles, numberOfBytes);

    // Close all 7 encoded files
    for (int i = 0; i < 7; i++){
        fclose(separatedFiles[i]);
    }
    
    return 0;
}