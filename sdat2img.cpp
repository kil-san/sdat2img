#include <iostream>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits.h>
#include <stdint.h>
using namespace std;

struct Pair {
    long block[2];
};

struct nset {
    Pair* pairs;
    int length;
};

// Function prototypes
int rangeSet(char* src, int position);
void parseTransferList(char* path);
void initOutputFile(ofstream *output_file_obj, int BLOCKSIZE);
long getFileSize();

bool isValid = false;
int isNew = -1;
nset* sets[3];                                              // Assuming only 3 possible commands, feel free to change them
                                                            // and they are referenced a lot so i made it a global variable.

int main(int argc, char* argv[]) {
    cout<<"\n.................Credits to JustusMary at valzey87@gmail.com................"<<endl<<endl;
    cout<<"   Re-written in C++ by me from xpirt - luxi78 - howellzhu work in python   \n"<<endl;
    cout<<"...........................Credits to them also.............................\n\n"<<endl;

    if (argc != 4 && argc != 5) {
        cout<<"\nsdat2img - usage is: \n\n      sdat2img <transfer_list> <system_new_file> <system_img> [-q]\n\n";
        exit(1);
    }

    bool quiet = false;
    if (argc == 5) {
        if (strcmp("-q", argv[4]) == 0) quiet = true;
        else {
            cout<<"Invalid argument: "<<argv[4]<<endl;
            cout<<"\nsdat2img - usage is: \n\n      sdat2img <transfer_list> <system_new_file> <system_img> [-q]\n\n";
            exit(1);
        }
    }

    for (int i = 0; i < 3; i++) {
        sets[i] = NULL;
    }

    char* TRANSFER_LIST_FILE = argv[1];
    char* NEW_DATA_FILE = argv[2];
    char* OUTPUT_IMAGE_FILE = argv[3];

    int BLOCK_SIZE = 4096;

    parseTransferList(TRANSFER_LIST_FILE);
    ifstream new_data_file(NEW_DATA_FILE, ios::binary);
    if (new_data_file == NULL) {
        cout<<NEW_DATA_FILE<<" not found"<<endl;
        exit(2);
    }
    ofstream output_img(OUTPUT_IMAGE_FILE, ios::binary);
    if (output_img == NULL) {
        cout<<OUTPUT_IMAGE_FILE<<" not found"<<endl;
        exit(2);
    }
    initOutputFile(&output_img, BLOCK_SIZE);

    if (quiet) {
        cout<<"converting DAT to IMG without verbose information..........................."<<endl;
        cout<<"Copying blocks of data to their respective positions........................"<<endl;
    }

    // Temp storage
    uint8_t* data;

    for (int i = 0; i < sets[isNew]->length; i++) {
        Pair* currentBlock = sets[isNew]->pairs + i;
        long stop = currentBlock->block[1];
        long start = currentBlock->block[0];

        long blockCount = stop - start;
        long long blocks = blockCount * BLOCK_SIZE;
        long long position = start * BLOCK_SIZE;
        unsigned long offset = position % ULONG_MAX;
        int cycles = position / ULONG_MAX;

        data = (uint8_t*)malloc(blocks);
        if (data == NULL) {
            cout<<"Out of memory error"<<endl;
            exit(-1);
        }
        new_data_file.read((char*)data, blocks);

        // in the case of images greater than 4GB if its even possible
        if (cycles > 0) {
            output_img.seekp(0, ios::beg);
            for (int i = 0; i < cycles; i++) {
                output_img.seekp(ULONG_MAX, ios::cur);
            }
            output_img.seekp(offset, ios::cur);
        }
        else output_img.seekp(position);

        if (!quiet) {
            cout<<"Copying "<<blockCount<<" blocks into position "<<start<<" with "<<blocks<<" bytes"<<endl;
        }
        if (quiet && i % 7 == 0) {
            cout<<"-";
        }

        output_img.write((char*)data, blocks);
        free(data);
    }

    output_img.close();
    new_data_file.close();

    cout<<"\nDone!"<<endl;

    return 0;
}


// Generate number set
int rangeSet(char* src, int position) {
    // Number of words is 1 + number of commas
    int length = 1;
    for (int i = 0, src_len = strlen(src); i < src_len; i++) {
        if (src[i] == ',') {
            length++;
        }
    }
    int ValidPairs = (length - 1) / 2;

    // Split numbers separated by commas
    int k = 0;
    long numSet[length];
    char* token = strtok(src, ",");
    while (token != NULL) {
        numSet[k] = atol(token);
        token = strtok(NULL, ",");
        k++;
    }

    if (length != numSet[0] + 1) {
        cout<<"Error on parsing following data to rangeset: "<<src<<endl;
        return -2;
    }

    // Generate block pairs
    sets[position] = (nset*)malloc(sizeof(nset));
    if (sets[position] == NULL) {
        cout<<"Out of memory error"<<endl;
        return -1;
    }
    sets[position]->pairs = (Pair*)malloc(sizeof(Pair) * ValidPairs);
    if (sets[position]->pairs == NULL) {
        cout<<"Out of memory error"<<endl;
        return -1;
    }
    sets[position]->length = ValidPairs;

    for (int i = 0, m = 0; i < length - 1; i+=2, m++) {
        (sets[position]->pairs + m)->block[0] = numSet[i + 1];
        (sets[position]->pairs + m)->block[1] = numSet[i + 2];
    }
    return 0;
}


void parseTransferList(char* path) {
    ifstream trans_list(path);
    if (trans_list == NULL) {
        cout<<path<<" not found"<<endl;
        exit(2);
    }

    string line;
    getline(trans_list, line);
    int version = atoi(line.c_str());                           // 1st line = transfer list version
    getline(trans_list, line);                                  // 2nd line = total number of blocks

    // system.transfer.list:
    // - version 1: android-5.0.0_r1
    // - version 2: android-5.1.0_r1
    // - version 3: android-6.0.0_r1

    // skip next 2 lines. we don't need this stuff now

    if (version >= 2) {
        getline(trans_list, line);                              // 3rd line = stash entries needed simultaneously
        getline(trans_list, line);                              // 4th line = number of blocks that will be stashed
    }
    char* cmd, *src;

    int i = 0;
    for (; getline(trans_list, line); i++) {                    // 5th & next lines should be only commands
        if (line.empty()) break;
        // Split command from number set
        cmd = strtok(&line[0], " ");
        src = strtok(NULL, " ");

        if (isdigit(cmd[0])) { i -= 1; continue; }
        if (strcmp(cmd, "new") == 0) { isNew = i; isValid = true;}

        int STATUS = rangeSet(src, i);
        if (STATUS != 0) {
            line.string::~string();
            exit(STATUS);
        }
    }

    // Terminate program if  there's no New command
    if (!isValid) {
        cout<<"No Valid command: "<<endl;
        line.string::~string();
        exit(3);
    }

    trans_list.close();
}


// Create empty image with Correct size;
void initOutputFile(ofstream *output_file_obj, int BLOCK_SIZE) {
    long maxBlock = getFileSize();
    long long position = maxBlock * BLOCK_SIZE - 1;
    unsigned long offset = position % ULONG_MAX;
    int cycles = position / ULONG_MAX;

    // in the case of images greater than 4GB if its even possible
    if (cycles > 0) {
        output_file_obj->seekp(0, ios::beg);
        for (int i = 0; i < cycles; i++) {
            output_file_obj->seekp(ULONG_MAX, ios::cur);
        }
        output_file_obj->seekp(offset, ios::cur);
    }
    else output_file_obj->seekp(position);

    output_file_obj->put('\0');
    output_file_obj->flush();
    return;
}


// Retrieves the Total size of the Image
long getFileSize() {
    long maxBlock = 0;
    long blockSize = 0;
    for(int m = 0; m < 3; m++) {
        if (sets[m] == NULL) break;
        for(int i = 0; i < sets[m]->length; i++) {
            blockSize = (sets[m]->pairs + i)->block[1];
            if (maxBlock < blockSize) maxBlock = blockSize;
        }
    }
    return maxBlock;
}
