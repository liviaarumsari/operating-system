#include "../../include/utility_shell.h"
#include "../../include/fat32.h"
#include "../../lib/lib-header/string.h"
#include "../../lib/lib-header/stdmem.h"

char current_directory[9] = "/\0\0\0\0\0\0\0";
uint32_t cwd_cluster_number = 2;
struct FAT32DirectoryTable cwd_table;

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    __asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));
    // Note : gcc usually use %eax as intermediate register,
    //        so it need to be the last one to mov
    __asm__ volatile("int $0x30");
}

void puts(char* buf, uint8_t color) {
    syscall(5, (uint32_t) buf, strlen(buf), color);
}

void parsePath(const char* path, char** parts, int* numParts) {
    char* token;
    int i = 0;

    token = custom_strtok((char*) path, "/");
    while (token != NULL) {
        if (*token != '\0') {
            parts[i] = token;
            i++;
        }
        // Skip over consecutive delimiter characters
        while (*token == '/' && *(token + 1) == '/') {
            token++;
        }
        token = custom_strtok(NULL, "/");
    }

    *numParts = i;
}

void mkdir(char* newDirectoryPath) {
    char* pathParts[256];
    int numParts = 0;

    char newDirectoryPathCopy[wordLen(newDirectoryPath, 0) + 1];
    getWord(newDirectoryPath, 0, newDirectoryPathCopy);
    parsePath(newDirectoryPathCopy, pathParts, &numParts);
    
    // This loop exists to check if the user put something ridiculous as the path argument.
    // For example: `//////////home/////////user//////////projects/`
    for (int i = 0; i < numParts; i++) {
        char* currentPart = pathParts[i];
        while (*currentPart != '\0') {
            if (*currentPart == '/') {
                // Found a '/' character, terminate the function
                puts("Invalid path.\n", BIOS_RED);
                return;
            }
            currentPart++;
        }
    }
    
    // Start from root
    if (newDirectoryPath[0] == '/') {

        if (numParts == 0) {
            puts("Cannot create directory.\n", BIOS_RED);
            return;
        }

        uint32_t currentClusterNumber = ROOT_CLUSTER_NUMBER;

        for (int i = 0; i < numParts; i++) {

            // Create findClusterNumber request
            struct FAT32DriverRequest findRequest = {
                .name = "\0\0\0\0\0\0\0\0",
                .ext = "\0\0\0",
                .parent_cluster_number = currentClusterNumber,
                .buffer_size = 0,
            };

            getWord(pathParts[i], 0, findRequest.name);
            
            // Find the cluster number of pathParts[i] in CWD
            // Returns 0 if it is not found
            uint32_t childClusterNumber = 0;
            syscall(9, (uint32_t) &findRequest, (uint32_t) &childClusterNumber, 0);

            if (childClusterNumber == 0) {
                // notFoundCount++;

                if (/*notFoundCount > 1 || */i != numParts - 1) {
                    puts("No such directory.\n", BIOS_RED);
                    return;
                }

                // Create write request
                struct FAT32DriverRequest writeRequest = {
                    .name = "\0\0\0\0\0\0\0\0",
                    .ext = "\0\0\0",
                    .parent_cluster_number = currentClusterNumber,
                    .buffer_size = 0,  // 0 means we're creating a directory
                };

                getWord(pathParts[i], 0, writeRequest.name);
                
                int8_t retCode = 0;
                syscall(2, (uint32_t) &writeRequest, (uint32_t) &retCode, 0);  // write

                if (retCode == 0) {
                    puts("Successfully created directory ", BIOS_GREEN);
                    puts(pathParts[i], BIOS_GRAY);
                    puts(".\n", BIOS_GREEN);
                    return;
                } else if (retCode == 1) {
                    puts("Folder ", BIOS_RED);
                    puts(pathParts[i], BIOS_GRAY);
                    puts(" already exists.\n", BIOS_RED);
                    return;
                } else if (retCode == 2) {
                    puts("Invalid parent cluster.\n", BIOS_RED);
                    return;
                } else if (retCode == -1) {
                    puts("Unknown error.\n", BIOS_RED);
                    return;
                }
            }

            if (i == numParts - 1) {

                // Create write request
                struct FAT32DriverRequest writeRequest = {
                    .name = "\0\0\0\0\0\0\0\0",
                    .ext = "\0\0\0",
                    .parent_cluster_number = currentClusterNumber,
                    .buffer_size = 0,  // 0 means we're creating a directory
                };

                getWord(pathParts[i], 0, writeRequest.name);
                
                int8_t retCode = 0;
                syscall(2, (uint32_t) &writeRequest, (uint32_t) &retCode, 0);  // write

                if (retCode == 0) {
                    puts("Successfully created directory ", BIOS_GREEN);
                    puts(pathParts[i], BIOS_GRAY);
                    puts(".\n", BIOS_GREEN);
                    return;
                } else if (retCode == 1) {
                    puts("Folder ", BIOS_RED);
                    puts(pathParts[i], BIOS_GRAY);
                    puts(" already exists.\n", BIOS_RED);
                    return;
                } else if (retCode == 2) {
                    puts("Invalid parent cluster.\n", BIOS_RED);
                    return;
                } else if (retCode == -1) {
                    puts("Unknown error.\n", BIOS_RED);
                    return;
                }
            }

            currentClusterNumber = childClusterNumber;
        }
    }
    
    else {
        
        uint32_t currentClusterNumber = cwd_cluster_number;

        // This happens if there is no slash (/) in new directory path.
        // For example: mkdir abc
        if (numParts == 0) {

            // Create write request
            struct FAT32DriverRequest writeRequest = {
                .name = "\0\0\0\0\0\0\0\0",
                .ext = "\0\0\0",
                .parent_cluster_number = currentClusterNumber,
                .buffer_size = 0,  // 0 means we're creating a directory
            };

            getWord(newDirectoryPath, 0, writeRequest.name);
            
            int8_t retCode = 0;
            syscall(2, (uint32_t) &writeRequest, (uint32_t) &retCode, 0);  // write

            if (retCode == 0) {
                puts("Successfully created directory ", BIOS_GREEN);
                puts(newDirectoryPath, BIOS_GRAY);
                puts(".\n", BIOS_GREEN);
                return;
            } else if (retCode == 1) {
                puts("Folder ", BIOS_RED);
                puts(newDirectoryPath, BIOS_GRAY);
                puts(" already exists.\n", BIOS_RED);
                return;
            } else if (retCode == 2) {
                puts("Invalid parent cluster.\n", BIOS_RED);
                return;
            } else if (retCode == -1) {
                puts("Unknown error.\n", BIOS_RED);
                return;
            }
        }

        for (int i = 0; i < numParts; i++) {

            // Create findClusterNumber request
            struct FAT32DriverRequest findRequest = {
                .name = "\0\0\0\0\0\0\0\0",
                .ext = "\0\0\0",
                .parent_cluster_number = currentClusterNumber,
                .buffer_size = 0,
            };

            getWord(pathParts[i], 0, findRequest.name);
            
            // Find the cluster number of pathParts[i] in CWD
            // Returns 0 if it is not found
            uint32_t childClusterNumber = 0;
            syscall(9, (uint32_t) &findRequest, (uint32_t) &childClusterNumber, 0);

            if (childClusterNumber == 0) {
                // notFoundCount++;

                if (/*notFoundCount > 1 || */i != numParts - 1) {
                    puts("No such directory.\n", BIOS_RED);
                    return;
                }

                // Create write request
                struct FAT32DriverRequest writeRequest = {
                    .name = "\0\0\0\0\0\0\0\0",
                    .ext = "\0\0\0",
                    .parent_cluster_number = currentClusterNumber,
                    .buffer_size = 0,  // 0 means we're creating a directory
                };

                getWord(pathParts[i], 0, writeRequest.name);
                
                int8_t retCode = 0;
                syscall(2, (uint32_t) &writeRequest, (uint32_t) &retCode, 0);  // write

                if (retCode == 0) {
                    puts("Successfully created directory ", BIOS_GREEN);
                    puts(pathParts[i], BIOS_GRAY);
                    puts(".\n", BIOS_GREEN);
                    return;
                } else if (retCode == 1) {
                    puts("Folder ", BIOS_RED);
                    puts(pathParts[i], BIOS_GRAY);
                    puts(" already exists.\n", BIOS_RED);
                    return;
                } else if (retCode == 2) {
                    puts("Invalid parent cluster.\n", BIOS_RED);
                    return;
                } else if (retCode == -1) {
                    puts("Unknown error.\n", BIOS_RED);
                    return;
                }
            }

            if (i == numParts - 1) {

                // Create write request
                struct FAT32DriverRequest writeRequest = {
                    .name = "\0\0\0\0\0\0\0\0",
                    .ext = "\0\0\0",
                    .parent_cluster_number = currentClusterNumber,
                    .buffer_size = 0,  // 0 means we're creating a directory
                };

                getWord(pathParts[i], 0, writeRequest.name);
                
                int8_t retCode = 0;
                syscall(2, (uint32_t) &writeRequest, (uint32_t) &retCode, 0);  // write

                if (retCode == 0) {
                    puts("Successfully created directory ", BIOS_GREEN);
                    puts(pathParts[i], BIOS_GRAY);
                    puts(".\n", BIOS_GREEN);
                    return;
                } else if (retCode == 1) {
                    puts("Folder ", BIOS_RED);
                    puts(pathParts[i], BIOS_GRAY);
                    puts(" already exists.\n", BIOS_RED);
                    return;
                } else if (retCode == 2) {
                    puts("Invalid parent cluster.\n", BIOS_RED);
                    return;
                } else if (retCode == -1) {
                    puts("Unknown error.\n", BIOS_RED);
                    return;
                }
            }

            currentClusterNumber = childClusterNumber;
        }
    }
}

void extractFilenameExtension(char* filename, char* name, char* extension) {
    int i = 0;
    int dot_index = -1;
    
    // find the index of the last occurrence of '.'
    while (filename[i] != '\0') {
        if (filename[i] == '.') {
            dot_index = i;
        }
        i++;
    }
    
    // if '.' is not found or is the first character or the last character
    if (dot_index == -1 || dot_index == 0 || dot_index == i-1) {
        name[0] = '\0';
        extension[0] = '\0';
        return;
    }
    
    // copy the file name and extension
    int j;
    for (j = 0; j < dot_index; j++) {
        name[j] = filename[j];
    }
    name[j] = '\0';
    
    for (j = dot_index+1; j < i; j++) {
        extension[j-dot_index-1] = filename[j];
    }
    extension[j-dot_index-1] = '\0';
}


void cat(char* filePath) {
    char* pathParts[256];
    int numParts = 0;

    char filePathCopy[wordLen(filePath, 0) + 1];
    getWord(filePath, 0, filePathCopy);
    parsePath(filePathCopy, pathParts, &numParts);
    
    // This loop exists to check if the user put something ridiculous as the path argument.
    // For example: `//////////home/////////user//////////projects/////test.txt////`
    for (int i = 0; i < numParts; i++) {
        char* currentPart = pathParts[i];
        while (*currentPart != '\0') {
            if (*currentPart == '/') {
                // Found a '/' character, terminate the function
                puts("Invalid path.\n", BIOS_RED);
                return;
            }
            currentPart++;
        }
    }
    
    // Start from root
    if (filePath[0] == '/') {

        if (numParts == 0) {
            puts("Not a file.\n", BIOS_RED);
            return;
        }

        uint32_t currentClusterNumber = ROOT_CLUSTER_NUMBER;

        for (int i = 0; i < numParts; i++) {

            // Create findClusterNumber request
            struct FAT32DriverRequest findRequest = {
                .name = "\0\0\0\0\0\0\0\0",
                .ext = "\0\0\0",
                .parent_cluster_number = currentClusterNumber,
                .buffer_size = 0,
            };

            getWord(pathParts[i], 0, findRequest.name);
            
            // Find the cluster number of pathParts[i] in CWD
            // Returns 0 if it is not found
            uint32_t childClusterNumber = 0;
            syscall(9, (uint32_t) &findRequest, (uint32_t) &childClusterNumber, 0);

            if (childClusterNumber == 0) {
                if (i != numParts - 1) {
                    puts("No such directory.\n", BIOS_RED);
                    return;
                }
            }

            if (i == numParts - 1) {
                
                struct ClusterBuffer cl[8];

                struct FAT32DriverRequest readRequest = {
                    .buf = &cl,
                    .name = "\0\0\0\0\0\0\0\0",
                    .ext = "\0\0\0",
                    .parent_cluster_number = currentClusterNumber,
                    .buffer_size = sizeof(cl), 
                };

                char name[8] = "\0\0\0\0\0\0\0\0";
                char ext[3] = "\0\0\0";

                extractFilenameExtension(pathParts[i], name, ext);

                memcpy(readRequest.name, name, 8);
                memcpy(readRequest.ext, ext, 3);

                int32_t retCode = 0;
                syscall(0, (uint32_t) &readRequest, (uint32_t) &retCode, 0);

                if (retCode == 0) {
                    int bufLen = 0;
                    while (cl->buf[bufLen] != '\0') {
                        bufLen++;
                    }

                    syscall(5, (uint32_t) cl->buf, bufLen, 0x07);
                    // struct ClusterBuffer filledBuf[5];

                    // memcpy(filledBuf, readRequest.buf, sizeof(filledBuf));

                    // for (int i = 0; i < 5; i++) {
                    //     struct ClusterBuffer item = filledBuf[i];

                    //     for (int j = 0; j < CLUSTER_SIZE; j++) {
                    //         char c = (char) item.buf[j];

                    //         if (c == 0) {
                    //             break;
                    //         }

                    //         char str[2] = {c, '\0'};
                    //         puts(str, BIOS_GRAY);
                    //     }
                    // }

                    return;
                } else if (retCode == 1) {
                    puts("Not a file.\n ", BIOS_RED);
                    return;
                } else if (retCode == 2) {
                    puts("Not enough buffer.\n", BIOS_RED);
                    return;
                } else if (retCode == 3) {
                    puts("No such file.\n", BIOS_RED);
                    return;
                } else if (retCode == -1) {
                    puts("Unknown error.\n", BIOS_RED);
                    return;
                }
            }

            currentClusterNumber = childClusterNumber;
        }
    }
    
    else {
        
        uint32_t currentClusterNumber = cwd_cluster_number;

        if (numParts == 0) {

            struct ClusterBuffer cl[8];

            struct FAT32DriverRequest readRequest = {
                .buf = &cl,
                .name = "\0\0\0\0\0\0\0\0",
                .ext = "\0\0\0",
                .parent_cluster_number = currentClusterNumber,
                .buffer_size = sizeof(cl), 
            };

            char name[8] = "\0\0\0\0\0\0\0\0";
            char ext[3] = "\0\0\0";

            extractFilenameExtension(pathParts[i], name, ext);

            memcpy(readRequest.name, name, 8);
            memcpy(readRequest.ext, ext, 3);

            int32_t retCode = 0;
            syscall(0, (uint32_t) &readRequest, (uint32_t) &retCode, 0);

            if (retCode == 0) {
                int bufLen = 0;
                while (cl->buf[bufLen] != '\0') {
                    bufLen++;
                }

                syscall(5, (uint32_t) cl->buf, bufLen, 0x07);
                // struct ClusterBuffer filledBuf[5];

                // memcpy(filledBuf, readRequest.buf, sizeof(filledBuf));

                // for (int i = 0; i < 5; i++) {
                //     struct ClusterBuffer item = filledBuf[i];

                //     for (int j = 0; j < CLUSTER_SIZE; j++) {
                //         char c = (char) item.buf[j];

                //         if (c == 0) {
                //             break;
                //         }

                //         char str[2] = {c, '\0'};
                //         puts(str, BIOS_GRAY);
                //     }
                // }
                return;
            } else if (retCode == 1) {
                puts("Not a file.\n ", BIOS_RED);
                return;
            } else if (retCode == 2) {
                puts("Not enough buffer.\n", BIOS_RED);
                return;
            } else if (retCode == 3) {
                puts("No such file.\n", BIOS_RED);
                return;
            } else if (retCode == -1) {
                puts("Unknown error.\n", BIOS_RED);
                return;
            }
        }

        for (int i = 0; i < numParts; i++) {

            // Create findClusterNumber request
            struct FAT32DriverRequest findRequest = {
                .name = "\0\0\0\0\0\0\0\0",
                .ext = "\0\0\0",
                .parent_cluster_number = currentClusterNumber,
                .buffer_size = 0,
            };

            getWord(pathParts[i], 0, findRequest.name);
            
            // Find the cluster number of pathParts[i] in CWD
            // Returns 0 if it is not found
            uint32_t childClusterNumber = 0;
            syscall(9, (uint32_t) &findRequest, (uint32_t) &childClusterNumber, 0);

            if (childClusterNumber == 0) {
                if (i != numParts - 1) {
                    puts("No such directory.\n", BIOS_RED);
                    return;
                }
            }

            if (i == numParts - 1) {

                struct ClusterBuffer cl[8];

                struct FAT32DriverRequest readRequest = {
                    .buf = &cl,
                    .name = "\0\0\0\0\0\0\0\0",
                    .ext = "\0\0\0",
                    .parent_cluster_number = currentClusterNumber,
                    .buffer_size = sizeof(cl), 
                };

                char name[8] = "\0\0\0\0\0\0\0\0";
                char ext[3] = "\0\0\0";

                extractFilenameExtension(pathParts[i], name, ext);

                memcpy(readRequest.name, name, 8);
                memcpy(readRequest.ext, ext, 3);

                int32_t retCode = 0;
                syscall(0, (uint32_t) &readRequest, (uint32_t) &retCode, 0);

                if (retCode == 0) {
                    int bufLen = 0;
                    while (cl->buf[bufLen] != '\0') {
                        bufLen++;
                    }

                    syscall(5, (uint32_t) cl->buf, bufLen, 0x07);
                    // struct ClusterBuffer filledBuf[5];

                    // memcpy(filledBuf, readRequest.buf, sizeof(filledBuf));

                    // for (int i = 0; i < 5; i++) {
                    //     struct ClusterBuffer item = filledBuf[i];

                    //     for (int j = 0; j < CLUSTER_SIZE; j++) {
                    //         char c = (char) item.buf[j];

                    //         if (c == 0) {
                    //             break;
                    //         }

                    //         char str[2] = {c, '\0'};
                    //         puts(str, BIOS_GRAY);
                    //     }
                    // }
                    return;
                } else if (retCode == 1) {
                    puts("Not a file.\n ", BIOS_RED);
                    return;
                } else if (retCode == 2) {
                    puts("Not enough buffer.\n", BIOS_RED);
                    return;
                } else if (retCode == 3) {
                    puts("No such file.\n", BIOS_RED);
                    return;
                } else if (retCode == -1) {
                    puts("Unknown error.\n", BIOS_RED);
                    return;
                }
            }

            currentClusterNumber = childClusterNumber;
        }
    }

void sleep(int microseconds) {
    int j = 0;
    for (int i=0; i < microseconds*1000; i++) {
        j++;
    }
}

void splashScreen() {
    puts("\n\n\n                  ___\n", BIOS_YELLOW);
    puts("                 (___)\n", BIOS_YELLOW);
    puts("          ____\n", BIOS_CYAN);
    puts("        _\\___ \\  |\\_/|\n", BIOS_CYAN);
    puts("       \\     \\ \\/ , , \\\n", BIOS_CYAN);
    puts("        \\__   \\ \\ =\"= //|||\\         Welcome to sOS Operating System!\n", BIOS_CYAN);
    puts("         |===  \\/____)_)||||                    Made by:\n", BIOS_CYAN);
    puts("         \\______|    | |||||          Angela Livia Arumsari 13521094\n", BIOS_CYAN);
    puts("             _/_|  | | =====          Noel Christoffel Simbolon 13521096\n", BIOS_CYAN);
    puts("            (_/  \\_)_)                Rinaldy Adin 13521134\n", BIOS_CYAN);
    puts("         _________________            Enrique Alifio Ditya 13521142\n", BIOS_CYAN);
    puts("        (                _)\n", BIOS_CYAN);
    puts("         (__    sOS      )\n", BIOS_CYAN);
    puts("           (___    _____)\n", BIOS_CYAN);
    puts("               '--'\n", BIOS_CYAN);
    puts("                                   LOADING . . .\n", BIOS_GREEN);

    puts("        ", BIOS_BLUE);
    for(int8_t i = 0; i < 60; i++) {
        puts("#", BIOS_YELLOW);
        sleep(10000);
    }

    sleep(100000);
    syscall(6,0,0,0);
}

void cd(char* command) {
    // Get the path
    int8_t retcode;
    int n_word = wordLen(command, 1);
    char path[n_word+1];
    getWord(command, 1, path);

    // If the path is empty, set the current directory to root
    if (strlen(path) == 0) {
        cwd_cluster_number = 2;
        return;
    }

    // Check if the path is valid
    struct FAT32DirectoryTable table;
    uint32_t cluster_number = cwd_cluster_number;
    char* token = strtok(path, "/");
    if (token == NULL) {
        token = path;
    }

    struct FAT32DriverRequest request = {
        .buf = &table,
        .name = "\0\0\0\0\0\0\0",
        .ext = "\0\0",
        .buffer_size = sizeof(struct FAT32DirectoryTable),
        .parent_cluster_number = cwd_cluster_number
    };

    // Iterate through the path
    while (token != NULL) {
        // For debug: check token
        puts("Token: ", BIOS_GRAY);
        puts(token, BIOS_GRAY);
        puts("\n", BIOS_GRAY);
        addTrailingNull(token, strlen(token), 8);
        memcpy(request.name, token, 8);
        request.parent_cluster_number = cluster_number;

        // Read the directory table
        syscall(1, (uint32_t) &request, (uint32_t) &retcode, 0);

        // If the token is not found, put error message and return
        if (retcode != 0) {
            puts(token, BIOS_RED);
            puts(": No such directory\n", BIOS_GRAY);
            return;
        }

        struct FAT32DirectoryEntry entry;
        char reqstr[11];
        memcpy(reqstr, token, 8);
        memcpy(reqstr + 8, "\0\0\0", 3);
        syscall(8, (uint32_t)&entry, (uint32_t)reqstr, cluster_number);
        cluster_number = (entry.cluster_high << 16) | entry.cluster_low;

        // Get the next token
        token = strtok(NULL, "/");
    }

    // If the path is valid, set the current directory to the path
    cwd_cluster_number = cluster_number;
}

void ls() {
    syscall(7, (uint32_t) &cwd_table, cwd_cluster_number, 0);
    
    // Iterate through the directory table
    for (int32_t i = 1; i < (int32_t)(CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)); i++) {
        // If the entry is empty, skip
        if (cwd_table.table[i].name[0] == 0x00)
            continue;

        // If its a folder
        if (cwd_table.table[i].attribute == ATTR_SUBDIRECTORY) {
            puts(cwd_table.table[i].name, BIOS_GREEN);
            puts("/", BIOS_GREEN);
            puts("\n", BIOS_GRAY);
        } else {
            // If its a file
            puts(cwd_table.table[i].name, BIOS_GREEN);
            puts("\n", BIOS_GRAY);
        }
    }
}

void copy(char* src_name, char* src_ext, uint32_t src_parent_number, char* target_name, char* target_ext, uint32_t target_parent_number) {
    uint32_t src_size;
    bool is_dir = 0;
    struct FAT32DirectoryTable src_table;
    uint32_t src_cluster_number;
    struct FAT32DirectoryTable src_parent_table;
    syscall(7, (uint32_t)&src_parent_table, src_parent_number, 0);
    
    for (int32_t i = 0; i < (int32_t)(CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)); i++) {
        if (memcmp(src_parent_table.table[i].name, src_name, 8) == 0 &&
            memcmp(src_parent_table.table[i].ext, src_ext, 3) == 0) {
            src_size = src_parent_table.table[i].filesize;
            src_cluster_number = (src_parent_table.table[i].cluster_high << 16) | src_parent_table.table[i].cluster_low;
            is_dir = src_parent_table.table[i].attribute == ATTR_SUBDIRECTORY;
            if (is_dir)
                src_size = sizeof(struct FAT32DirectoryTable);
            break;
        }
    }
    
    struct ClusterBuffer data_buf[(src_size + CLUSTER_SIZE - 1) / CLUSTER_SIZE];
    struct FAT32DriverRequest request = {
        .buf = &data_buf,
        .name = "\0\0\0\0\0\0\0",
        .ext = "\0\0",
        .buffer_size = src_size,
        .parent_cluster_number = src_parent_number
    };
    memcpy(request.name, src_name, 8);
    memcpy(request.ext, src_ext, 3);

    int8_t retcode;
    if (is_dir) {
        request.buf = &src_table;
        syscall(1, (uint32_t)&request, (uint32_t)&retcode, 0);
    } else
        syscall(0, (uint32_t)&request, (uint32_t)&retcode, 0);

    if (is_dir) {
        memcpy(request.name, target_name, 8);
        memcpy(request.ext, target_ext, 3);
        request.parent_cluster_number = target_parent_number;
        request.buffer_size = 0;
        syscall(2, (uint32_t)&request, (uint32_t)&retcode, 0);

        uint32_t target_cluster_number;
        struct FAT32DirectoryTable target_parent_table;

        syscall(7, (uint32_t)&target_parent_table, target_parent_number, 0);

        for (int32_t i = 0; i < (int32_t)(CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)); i++) {
            if (memcmp(target_parent_table.table[i].name, target_name, 8) == 0 &&
                memcmp(target_parent_table.table[i].ext, target_ext, 3) == 0 &&
                target_parent_table.table[i].user_attribute == UATTR_NOT_EMPTY) {
                target_cluster_number = (target_parent_table.table[i].cluster_high << 16) | target_parent_table.table[i].cluster_low;
            }
        }

        for (int32_t i = 1; i < (int32_t)(CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)); i++) {
            if (src_table.table[i].user_attribute == UATTR_NOT_EMPTY) {
                copy(src_table.table[i].name, src_table.table[i].ext, src_cluster_number, src_table.table[i].name, src_table.table[i].ext, target_cluster_number);
            }
        }
    } else {
        memcpy(request.name, target_name, 8);
        memcpy(request.ext, target_ext, 3);
        request.parent_cluster_number = target_parent_number;
        syscall(2, (uint32_t)&request, (uint32_t)&retcode, 0);
        if (retcode != 0)
            puts("Error writing to file", BIOS_GRAY);
    }
}

void cp(char* command) {
    uint16_t n_words = countWords(command);
    int16_t recursive = -1;
    int8_t retcode = 0;

    struct FAT32DirectoryTable table_buf = {0};
    struct FAT32DriverRequest request = {
        .buf = &table_buf,
        .name = "\0\0\0\0\0\0\0",
        .ext = "\0\0",
        .parent_cluster_number = cwd_cluster_number,
        .buffer_size = sizeof(struct FAT32DirectoryEntry)
    };

    for (uint16_t i = 1; i < n_words; i++) {
        uint16_t n = wordLen(command, i);
        char word[n + 1];
        getWord(command, i, word);
        if (strcmp(word, "-r"))
            recursive = i;
    }

    if ((recursive == -1 && n_words < 3) || (recursive != -1 && n_words < 4)) {
        puts(": missing file operands\n", BIOS_GRAY);
        return;
    }

    uint16_t target_idx;
    if (recursive == n_words - 1)
        target_idx = n_words - 2;
    else
        target_idx = n_words - 1;

    // check if all source exists
    for (uint16_t i = 1; i < n_words; i++) {
        if (recursive == i || target_idx == i) continue;
        char filename[12];
        getWord(command, i, filename);

        char name[9];
        char ext[4];

        // if filename is too long
        if (parseFileName(filename, name, ext)) {
            puts(filename, BIOS_GRAY);
            puts(": filename invalid, name or extension may be too long\n", BIOS_GRAY);
            return;
        }

        memcpy(request.name, name, 8);
        memcpy(request.ext, ext, 3);
        syscall(1, (uint32_t)&request, (uint32_t)&retcode, 0);
        if (retcode == 2) {
            puts(filename, BIOS_GRAY);
            puts(": file not found\n", BIOS_GRAY);
            return;
        }
        if (retcode == 0 && recursive == -1) {
            puts(filename, BIOS_GRAY);
            puts(": is a directory;  -r not specified\n", BIOS_GRAY);
            return;
        }
    }

    uint16_t target_n = wordLen(command, target_idx);
    char target_filename[target_n + 1];
    getWord(command, target_idx, target_filename);

    char target_name[9];
    char target_ext[4];

    if (parseFileName(target_filename, target_name, target_ext)) {
        puts(target_filename, BIOS_GRAY);
        puts(": filename invalid, name or extension may be too long\n", BIOS_GRAY);
        return;
    }

    memcpy(request.name, target_name, 8);
    memcpy(request.ext, target_ext, 3);
    syscall(1, (uint32_t)&request, (uint32_t)&retcode, 0);
    
    // target is an existing directory
    if (retcode == 0) {
        uint32_t target_cluster_number;
        for (int32_t i = 0; i < (int32_t)(CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)); i++) {
            if (memcmp(cwd_table.table[i].name, target_name, 8) == 0 &&
                memcmp(cwd_table.table[i].ext, target_ext, 3) == 0) {
                target_cluster_number = (cwd_table.table[i].cluster_high << 16) | cwd_table.table[i].cluster_low;
            }
        }

        for (int16_t i = 1; i < n_words; i++) {
            if (i == recursive || i == target_idx) continue;
            char filename[12];
            getWord(command, i, filename);
            char name[9];
            char ext[4];
            parseFileName(filename, name, ext);

            copy(name, ext, cwd_cluster_number, name, ext, target_cluster_number);
        }
    } else if (retcode == 1 || retcode == 2) {
        if ((recursive == -1 && n_words > 3) || (recursive != -1 && n_words > 4)) {
            puts(target_filename, BIOS_GRAY);
            puts(": is not a folder\n", BIOS_GRAY);
            return;
        } else {
            if (retcode == 1)
                syscall(3, (uint32_t)&request, (uint32_t)&retcode, 0);

            for (int16_t i = 1; i < n_words; i++) {
                if (i == recursive || i == target_idx) continue;
                char filename[12];
                getWord(command, i, filename);
                char name[9];
                char ext[4];
                parseFileName(filename, name, ext);

                copy(name, ext, cwd_cluster_number, target_name, target_ext, cwd_cluster_number);
            }
        }
    }
}

void remove(char* name, char* ext, uint32_t parent_number) {
    int8_t retcode;

    struct ClusterBuffer data_buf;
    struct FAT32DriverRequest request = {
        .buf = &data_buf,
        .name = "\0\0\0\0\0\0\0",
        .ext = "\0\0",
        .buffer_size = 0,
        .parent_cluster_number = parent_number
    };
    memcpy(request.name, name, 8);
    memcpy(request.ext, ext, 3);
    syscall(3, (uint32_t)&request, (uint32_t)&retcode, 0);

    if (retcode == 2) {
        uint32_t target_cluster_number;
        struct FAT32DirectoryTable parent_table;
        syscall(7, (uint32_t)&parent_table, parent_number, 0);

        for (int32_t i = 0; i < (int32_t)(CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)); i++) {
            if (memcmp(parent_table.table[i].name, name, 8) == 0 &&
                memcmp(parent_table.table[i].ext, ext, 3) == 0 &&
                parent_table.table[i].user_attribute == UATTR_NOT_EMPTY) {
                target_cluster_number = (parent_table.table[i].cluster_high << 16) | parent_table.table[i].cluster_low;
            }
        }

        struct FAT32DirectoryTable target_table;
        request.buf = &target_table;
        request.buffer_size = sizeof(struct FAT32DirectoryTable);
        syscall(1, (uint32_t)&request, (uint32_t)&retcode, 0);
        
        for (int32_t i = 1; i < (int32_t)(CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)); i++) {
            if (target_table.table[i].user_attribute == UATTR_NOT_EMPTY) {
                remove(target_table.table[i].name, target_table.table[i].ext, target_cluster_number);
            }
        }
    }
    memcpy(request.name, name, 8);
    memcpy(request.ext, ext, 3);
    request.parent_cluster_number = parent_number;
    syscall(3, (uint32_t)&request, (uint32_t)&retcode, 0);
}

void rm(char* command) {
    uint16_t n_words = countWords(command);
    int16_t recursive = -1;
    int8_t retcode = 0;

    struct FAT32DirectoryTable table_buf = {0};
    struct FAT32DriverRequest request = {
        .buf = &table_buf,
        .name = "\0\0\0\0\0\0\0",
        .ext = "\0\0",
        .parent_cluster_number = cwd_cluster_number,
        .buffer_size = sizeof(struct FAT32DirectoryEntry)
    };

    for (uint16_t i = 1; i < n_words; i++) {
        uint16_t n = wordLen(command, i);
        char word[n + 1];
        getWord(command, i, word);
        if (strcmp(word, "-r"))
            recursive = i;
    }

    if ((recursive == -1 && n_words < 2) || (recursive != -1 && n_words < 3)) {
        puts(": missing file operands\n", BIOS_GRAY);
        return;
    }

    // check if all files exists
    for (uint16_t i = 1; i < n_words; i++) {
        if (recursive == i) continue;
        char filename[12];
        getWord(command, i, filename);

        char name[9];
        char ext[4];

        // if filename is too long
        if (parseFileName(filename, name, ext)) {
            puts(filename, BIOS_GRAY);
            puts(": filename invalid, name or extension may be too long\n", BIOS_GRAY);
            return;
        }

        memcpy(request.name, name, 8);
        memcpy(request.ext, ext, 3);
        syscall(1, (uint32_t)&request, (uint32_t)&retcode, 0);
        if (retcode == 2) {
            puts(filename, BIOS_GRAY);
            puts(": file not found\n", BIOS_GRAY);
            return;
        }
        if (retcode == 0 && recursive == -1) {
            puts(filename, BIOS_GRAY);
            puts(": is a directory;  -r not specified\n", BIOS_GRAY);
            return;
        }
    }

    for (uint16_t i = 1; i < n_words; i++) {
        if (recursive == i) continue;
        char filename[12];
        getWord(command, i, filename);

        char name[9];
        char ext[4];

        // if filename is too long
        if (parseFileName(filename, name, ext)) {
            puts(filename, BIOS_GRAY);
            puts(": filename invalid, name or extension may be too long\n", BIOS_GRAY);
            return;
        }

        remove(name, ext, cwd_cluster_number);
    }
}

void executeCommand(char* buf) {
    uint16_t n = wordLen(buf, 0);
    char command[n + 1];
    getWord(buf, 0, command);

    if (strcmp(command, "cd")) {
        cd(buf);
    }
    else if (strcmp(command, "ls")) {
        ls();
    }
    else if (strcmp(command, "mkdir")) {
        uint16_t argumentLen = wordLen(buf, 1);
        char newDirectoryPath[argumentLen + 1];  // + 1 to compensate for null character
        getWord(buf, 1, newDirectoryPath);
        mkdir(newDirectoryPath);
    }
    else if (strcmp(command, "cat")) {
        // struct ClusterBuffer cbuf[8];

        // struct FAT32DriverRequest fileReq = {
        //     .buf = &cbuf,
        //     .name = "test",
        //     .ext = "txt",
        //     .parent_cluster_number = 2,
        //     .buffer_size = sizeof(cbuf),
        // };

        // int32_t retCode = 0;

        // syscall(2, (uint32_t) &fileReq, (uint32_t) &retCode, 0);

        uint16_t argumentLen = wordLen(buf, 1);
        char filePath[argumentLen + 1];  // + 1 to compensate for null character
        getWord(buf, 1, filePath);
        cat(filePath);
    }
    else if (strcmp(command, "cp")) {
        cp(buf);
    }
    else if (strcmp(command, "rm")) {
        rm(buf);
    }
    else if (strcmp(command, "mv")) {
        moveCommand(buf);
    }
    else if (strcmp(command, "whereis")) {
        whereisCommand(buf);
    }
    else if (strcmp(command, "clear")) {
        syscall(6, 0, 0, 0);
    }
    else {
        puts(command, BIOS_GRAY);
        puts(": command not found\n", BIOS_GRAY);
    }
}

void whereisCommand(char* buf) {
    if (countWords(buf) != 2) {
        puts("Argument is invalid\n", BIOS_GRAY);
        return;
    }

    uint16_t n = wordLen(buf, 1);
    char searchName[n + 1];
    getWord(buf, 1, searchName);
    
    char name[8];
    char ext[3];

    // if filename is too long
    if (parseFileName(searchName, name, ext)) {
        puts(searchName, BIOS_GRAY);
        puts(": filename invalid, name or extension may be too long\n", BIOS_GRAY);
        return;
    }
    puts(searchName, BIOS_GRAY);
    puts(":", BIOS_GRAY);

    if (strcmp(name, "root\0\0\0")) {
        puts("  root\n", BIOS_GRAY);
        return;
    }

    int8_t isFileFound = 0;

    DFSsearch(ROOT_CLUSTER_NUMBER, name, &isFileFound);
    if (!isFileFound) {
        puts(" file/folder is not found", BIOS_GRAY);
    }
    puts("\n", BIOS_GRAY);
}

void DFSsearch(uint32_t folderAddress, char* searchName, int8_t* isFound) {
    // struct FAT32DirectoryTable dirtable;
    // struct FAT32DriverRequest request = {
    //     .buf                   = &dirtable,
    //     .name                  = "",
    //     .ext                   = "\0\0\0",
    //     .parent_cluster_number = parentCluster,
    //     .buffer_size           = CLUSTER_SIZE
    // };
    // memcpy(request.name, folderName, 8);
    // int32_t retcode;
    // syscall(1, (uint32_t) &request, (uint32_t) &retcode, 0);
    struct FAT32DirectoryTable dirtable;
    syscall(7, (uint32_t) &dirtable, folderAddress, 0);
    for (size_t i = 1; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++) {
        uint32_t clusterAddress = (dirtable.table[i].cluster_high << 16) | dirtable.table[i].cluster_low;
        if (strcmp(dirtable.table[i].name,searchName)) {
            *isFound = 1;
            puts("  root", BIOS_GRAY);
            if (dirtable.table[i].attribute != ATTR_SUBDIRECTORY) {
                constructPath(folderAddress);
                puts("/", BIOS_GRAY);
                puts(dirtable.table[i].name, BIOS_GRAY);
                if (!strcmp(dirtable.table[i].ext, "\0\0\0")) {
                    puts(".", BIOS_GRAY);
                    puts(dirtable.table[i].ext, BIOS_GRAY);
                }
            }
            else {
                constructPath(clusterAddress);
            }
        }
        if (dirtable.table[i].attribute == ATTR_SUBDIRECTORY) {
            DFSsearch(clusterAddress, searchName, isFound);
        }
    }  
}

void constructPath(uint32_t clusterAddress) {
    struct FAT32DirectoryTable dirtable;
    syscall(7, (uint32_t) &dirtable, clusterAddress, 0);
    if (!strcmp(dirtable.table[0].name, "root\0\0\0")) {
        uint32_t parentCluster = (dirtable.table[0].cluster_high << 16) | dirtable.table[0].cluster_low;
        constructPath(parentCluster);
        puts("/", BIOS_GRAY);
        puts(dirtable.table[0].name, BIOS_GRAY);
    }
    
}

void moveCommand(char* buf){
    if (countWords(buf) != 3) {
        puts("Argument is invalid\n", BIOS_GRAY);
        return;
    }

    // Get first argument
    int16_t n_param1 = wordLen(buf, 1);
    char param1[n_param1 + 1];
    getWord(buf, 1, param1);

    // Get second argument
    int16_t n_param2 = wordLen(buf, 2);
    char param2[n_param2 + 1];
    getWord(buf, 2, param2);

    // Remove command
    char removeCommand[n_param1 + 4];
    removeCommand[0] = 'r';
    removeCommand[1] = 'm';
    removeCommand[2] = ' ';
    for (int16_t i = 0; i <= n_param1; i++) {
        removeCommand[3+i] = param1[i];
    }

    // Copy command
    char copyCommand[n_param1 + n_param2 + 5];
    memcpy(copyCommand, removeCommand, n_param1 + 3);
    copyCommand[0] = 'c';
    copyCommand[1] = 'p';
    copyCommand[n_param1+3] = ' ';
    for (int16_t i = 0; i <= n_param2; i++) {
        copyCommand[n_param1 +4+i] = param2[i];
    }

    cp(copyCommand);
    rm(removeCommand);
}