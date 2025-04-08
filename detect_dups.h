#define _XOPEN_SOURCE 500
#include <ftw.h>
/*
    Add any other includes you may need over here...
*/
#include "compute-md5.h"
#include <openssl/evp.h>
#include "uthash.h"
#include <stdio.h>
#include <stdlib.h>
// define the structure required to store the file paths

typedef struct {
    char **paths;        // List of file paths
    unsigned long inode;     // Inode of the file the symlink points to
    int referenceCount;
} SoftLinkGroup;

typedef struct {
    unsigned long inode;         // Inode for this hard link group
    int referenceCount;   // number of hardlinks
    char **paths;        // List of file paths of hardlinks
    int pathCount;
    int num_symlinks;
    SoftLinkGroup *symlinks;  // List of symlinks pointing to this inode
} HardLinkGroup;


typedef struct {
    unsigned char hash[16];  // MD5 hash of the file
    int num_groups;
    HardLinkGroup *groups;

    UT_hash_handle hh;                    // Hash table handle
} File;


// process nftw files using this function
static int render_file_info(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf);

// add any other function you may need over here


/// im in a hash right? -> is file hard link? yes then create a new struct to hold hardink in a list of hardlinks
// is softlink? -> yes then create a softlink, ensure that hardlink count = 0 and just add softlink path
