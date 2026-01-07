/* Author: Steven Carr
   Program Created: October 2023
   Program to perform RAID 2 encoding (which utilizes Hamming (7,4) code) of a given txt file from command-line
   Instead of separating the 7 bits into 7 different disks, we simulate the disks as 7 different output files
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Function to create a byte from the 8 bits in the given parity/data bit array
unsigned char createByte(unsigned char byte, unsigned char givenBit[]){
    for (int i = 0; i < 8; i++){
        byte |= (givenBit[i] << (7 - i));
    }
    return byte;
}

void raidTwoEncoding(FILE* incomingFile, char* fileName){
    // If the input file is NULL, then there's an error
    if (incomingFile == NULL){
        printf("There was an error with the file that was presented.\n");
        exit(1);
    }

    // Initialize 7 separate files to split our incoming file into
    FILE *separatedFiles[7];

    // These will be the names of the files; 7 for the number of files and 30 as a
    // general length for each of their names
    char namesOfTheFiles[7][30];

    // Separate the file into 7 parts using a for loop
    for (int i = 0; i < 7; i++){
        // Each separation has the extension '.part#'
        sprintf(namesOfTheFiles[i], "%s.part%d", fileName, i);
        // Create each of the files
        separatedFiles[i] = fopen(namesOfTheFiles[i], "w+");

        // If the new file is NULL then there was an issue in creating it
        if (separatedFiles[i] == NULL){
            printf("An error occurred when attempting to separate the file.\n");
            exit(1);
        }
    }

    // Initialize the byte to be read using fread
    unsigned char readByte;

    // Initialize the hamming bits
    // NOTE: I labeled the parity/data bits in order of bit place, not necessarily their parity/data bit number.
    // So, D3 is the first data bit, D5 is the second data bit, D6 is the third data bit and D7 is the fourth.
    unsigned char P1[8], P2[8], D3[8], P4[8], D5[8], D6[8], D7[8];

    // Initialize a variable to keep track of the amount of nibbles we've processed as well as
    // the amount of bytes we've processed for each respective data/parity bit
    int processedNibbles = 0;
    int processedBytes = 0;

    // While fread == 1 means that while fread is still able to read bytes from the incomingFile
    while (fread(&readByte, 1, 1, incomingFile) == 1){
        // Split the byte into two nibbles
        // Using the mask (& 0X0F) allows us to isolate a nibble (4 bits) at a time
        // Example: nibble1 is isolating left 4 bits and nibble 2 is isolating the right 4,
        // so 0x61 would be split into nibble1 = 01100000 and nibble2 = 00000001 (normally 0x61 = 01100001)
        unsigned char nibble1 = (readByte >> 4) & 0x0F;
        unsigned char nibble2 = readByte & 0x0F;

        // Process the data bits for the first nibble (left-hand side of the byte)
        // -> D3 is leftmost bit of this nibble and D7 is rightmost bit of this nibble
        D3[processedNibbles] = nibble1 & 1;
        D5[processedNibbles] = (nibble1 >> 1) & 1;
        D6[processedNibbles] = (nibble1 >> 2) & 1;
        D7[processedNibbles] = (nibble1 >> 3) & 1;

        // Calculate the parity bits - P1 should XOR every other bit starting from but not including itself,
        // P2 should XOR every other couple of bits starting from but not including itself, and P4 is the same
        // circumstance but with every quadruple of bits
        P1[processedNibbles] = D3[processedNibbles] ^ D5[processedNibbles] ^ D7[processedNibbles];
        P2[processedNibbles] = D3[processedNibbles] ^ D6[processedNibbles] ^ D7[processedNibbles];
        P4[processedNibbles] = D5[processedNibbles] ^ D6[processedNibbles] ^ D7[processedNibbles];

        // Increment the amount of nibbles processed to move the array spaces
        processedNibbles++;

        // Do the same but not for the second (right side) nibble; for some reason I'm not too sure of I've
        // found that the opposite process on the right nibble produces the wanted result; so D3 is still
        // the left most bit of the right nibble and D7 the right most bit, but the bitwise operations are
        // reversed
        D3[processedNibbles] = (nibble2 >> 3) & 1;
        D5[processedNibbles] = (nibble2 >> 2) & 1;
        D6[processedNibbles] = (nibble2 >> 1) & 1;
        D7[processedNibbles] = nibble2 & 1;

        // Do the same exact bitwise operations to caluclate the parity bits for the second nibble of this byte
        P1[processedNibbles] = D3[processedNibbles] ^ D5[processedNibbles] ^ D7[processedNibbles];
        P2[processedNibbles] = D3[processedNibbles] ^ D6[processedNibbles] ^ D7[processedNibbles];
        P4[processedNibbles] = D5[processedNibbles] ^ D6[processedNibbles] ^ D7[processedNibbles];

        // Increment the nibbles processed to move forward a space
        processedNibbles++;

        // Increment the total number of bytes processed from reading the incoming file;
        // NOTE: For each byte being read, 2 nibbles are processed; and therefore, 2 bits
        // are stored in each parity/data bit.  Thus, when the total number of processed
        // Bytes is 4, that means there are 8 bits (a byte) attached to each parity/data
        // bit, and we should then print it to the files.
        processedBytes++;

        // If the amount of processed bytes is 4, then that means there are 8 bits (1 byte)
        // in each of the parity/data bit arrays, so now we want to write to their respective files
        if (processedBytes == 4){

            // We initialize an unsigned char to hold the byte that we will be writing for each of the
            // parity data bits, and then I call the function to convert the 8 bits into a byte.  Finally,
            // I write the byte for each parity/data bit to their respective files.
            unsigned char byte = 0;
            byte = createByte(byte, P1);
            fwrite(&byte, sizeof(unsigned char), 1, separatedFiles[0]);

            // Reset the byte to 0, and restart the process for P2, D3, P4, D5, D6 and D7.
            byte = 0;
            byte = createByte(byte, P2);
            fwrite(&byte, sizeof(unsigned char), 1, separatedFiles[1]);

            byte = 0;
            byte = createByte(byte, D3);
            fwrite(&byte, sizeof(unsigned char), 1, separatedFiles[2]);

            byte = 0;
            byte = createByte(byte, P4);
            fwrite(&byte, sizeof(unsigned char), 1, separatedFiles[3]);

            byte = 0;
            byte = createByte(byte, D5);
            fwrite(&byte, sizeof(unsigned char), 1, separatedFiles[4]);

            byte = 0;
            byte = createByte(byte, D6);
            fwrite(&byte, sizeof(unsigned char), 1, separatedFiles[5]);

            byte = 0;
            byte = createByte(byte, D7);
            fwrite(&byte, sizeof(unsigned char), 1, separatedFiles[6]);

            // Reset the amount of processed bytes back to 0
            processedBytes = 0;

            // Also reset the nibble place to reset the arrays
            processedNibbles = 0;
        }
    }

    // Close all the new files
    for (int i = 0; i < 7; i++){
        fclose(separatedFiles[i]);
    }
}


int main(int argc, char* argv[]){
    if (argc < 3){
        printf("Make sure to include the proper signifier with the file you would like to encode.\n");
        exit(1);
    }
    // Initialize a file for the input file used for the encoding
    FILE *incomingFile;
    char *fileName = argv[2];

    // If -f is a recognized command-line argument, then properly open the file
    // Otherwise throw the error message
    if (strcmp(argv[1], "-f") == 0){
        incomingFile = fopen(argv[2], "r");
    } else {
        printf("Ensure you put a '-f' before the input file.\n");
        exit(1);
    }

    // Call the function to use RAID 2 encoding on the input file
    raidTwoEncoding(incomingFile, fileName);

    // Close the file when we're done
    fclose(incomingFile);

    return 0;
}