#include <stdio.h>
#include <string.h>
#include "cli.h"

// TODO: Include these once the engine files are ready
// #include "shred.h"
// #include "wipe.h"

void print_help(void) {
    printf("OpenErase - Privacy & Secure Data Destruction Toolkit\n\n");
    printf("Usage:\n");
    printf("  openerase shred <file/path>     Securely overwrite and delete a target\n");
    printf("  openerase wipe-free <path>      Wipe free space on a given drive\n");
    printf("  openerase --help                Show this help message\n");
}

int parse_and_execute(int argc, char *argv[]) {
    // If the user just runs `openerase` with no arguments
    if (argc < 2) {
        print_help();
        return 1; // Return 1 to indicate an error/missing arguments
    }

    // Command: shred
    if (strcmp(argv[1], "shred") == 0) {
        if (argc < 3) {
            printf("Error: Missing target path.\n");
            printf("Usage: openerase shred <file/path>\n");
            return 1;
        }
        printf("[CLI Router] -> Sending '%s' to the shred engine...\n", argv[2]);
        // return secure_shred(argv[2]); 
        return 0;
    } 
    // Command: wipe-free
    else if (strcmp(argv[1], "wipe-free") == 0) {
        if (argc < 3) {
            printf("Error: Missing target drive/path.\n");
            printf("Usage: openerase wipe-free <path>\n");
            return 1;
        }
        printf("[CLI Router] -> Sending '%s' to the free-space wiping engine...\n", argv[2]);
        // return wipe_free_space(argv[2]);
        return 0;
    }
    // Command: --help or -h
    else if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        print_help();
        return 0; // Return 0 because asking for help is a successful operation
    } 
    // Unknown command
    else {
        printf("Error: Unknown command '%s'\n\n", argv[1]);
        print_help();
        return 1;
    }
}