// add any other includes in the detetc_dups.h file
#include "detect_dups.h"
#include "errno.h"
// define any other global variable you may need over here
int ErrorNo = 0;
File * filetable = NULL;
int fileCount = 0;
#define MAX_GROUPS 100 // Define a maximum number of groups
#define MAX_PATHS 100 // define max paths
# // Function prototypes
void print_hardlink_group();
void print_file_info();
HardLinkGroup* createHardLinkGroup();
// open ssl, this will be used to get the hash of the file
EVP_MD_CTX *mdctx;
const EVP_MD *md = NULL; // use md5 hash!!



#define MAX_LEN 4096

int compute_file_hash(const char *path, EVP_MD_CTX *mdctx, unsigned char *md_value, unsigned int *md5_len) {
    
    FILE *fd = fopen(path, "rb");
    
    if (fd == NULL) {
        fprintf(stderr, "%s::%d::Error opening file %d: %s\n", __func__, __LINE__, errno, path);
	return -1; 
    }
    
    char buff[MAX_LEN];
    size_t n;
    EVP_DigestInit_ex(mdctx, EVP_md5(), NULL);
    while ((n = fread(buff, 1, MAX_LEN, fd))) {
        EVP_DigestUpdate(mdctx, buff, n);
    }
    EVP_DigestFinal_ex(mdctx, md_value, md5_len);
    EVP_MD_CTX_reset(mdctx);
    fclose(fd);
    return 0;
}


int main(int argc, char *argv[]) {
    // perform error handling, "exit" with failure incase an error occurs
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (nftw(argv[1], render_file_info, 20, FTW_PHYS) == -1) {
        fprintf(stderr, "Error %d: %s is not a valid directory\n", errno, argv[1]);
        exit(EXIT_FAILURE);
    }
    //printfiles();
    // initialize the other global variables you have, if any
    
    // add the nftw handler to explore the directory
    // nftw should invoke the render_file_info function
    
    //(nftw(argv[1], render_file_info, 20, 0) == -1);
    
    print_file_info();
}

// render the file information invoked by nftw
static int render_file_info(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf) {
    //printf("\nInode: %lu, Name: %s\n", (unsigned long)sb->st_ino, fpath);
    
    switch (tflag) {
    case FTW_F:
        //printf(" Regular File, Last Access: %s ", ctime(&sb->st_atime));
        if ( S_ISBLK(sb->st_mode) ) {
        //printf(" (Block Device)");
        } else if ( S_ISCHR(sb->st_mode) ) {
        //printf(" (Character Device)");    
        }
        break;
    case FTW_D:
        //printf(" (Directory) \n");
        //printf("level=%02d, size=%07ld path=%s filename=%s\n",
            //ftwbuf->level, sb->st_size, fpath, fpath + ftwbuf->base);
        return 0;
        break; 
    case FTW_SL:
    
        // Symbolic link
        struct stat target_stat;

        //printf("Symlink:      %-30s → ", fpath);

        if (stat(fpath, &target_stat) == 0) {
            //printf("Target inode: %lu\n", target_stat.st_ino);
            if (S_ISDIR(target_stat.st_mode)) {
                return 0;  // Don't hash directory symlinks
            }
        }

        else {
            //printf("Target inaccessible (%s)\n", strerror(errno));
        }


        

        break;
    }


    

    // computing file hash
    const char *filename ;
    unsigned char md5_value[EVP_MAX_MD_SIZE];
    unsigned int md5_len;
    int err; 
    EVP_MD_CTX *mdctx;
    
    md = EVP_md5();
    filename = fpath;
    /* create a new MD5 context structure */ 
    mdctx = EVP_MD_CTX_new();
    md5_len = 0;

    err = compute_file_hash(filename,mdctx,md5_value,&md5_len);
    if (err < 0) {
        fprintf(stderr, "%s::%d::Error computing MD5 hash %d\n", __func__, __LINE__, errno);
        exit(EXIT_FAILURE);    
    }
    

    // printf("\tMD5 Hash: ");
    // for (int i = 0; i < md5_len; i++) {
    //     printf("%02x", md5_value[i]);
    // }
    // printf("\n");

    EVP_MD_CTX_free(mdctx); // don't create a leak!
    
    
    
    
    // perform the inode operations over here
    unsigned long inode = (unsigned long)sb->st_ino;

    // Check if the file already exists in the hash table
    File *file = NULL;
    HASH_FIND(hh, filetable, md5_value, md5_len, file);
    
    if (file == NULL) {
        // If not found, create a new hash entry in the hash table
        file = malloc(sizeof(File));
        
        memcpy(file->hash, md5_value, md5_len);

        file->groups = malloc(sizeof(HardLinkGroup) * MAX_GROUPS);
        file->num_groups = 0;
        fileCount++;
        HASH_ADD(hh, filetable, hash, md5_len, file);
        
    }

    
    
    HardLinkGroup * targetGroup;
    // hardlink
    if (tflag == FTW_F){
        targetGroup = createHardLinkGroup(inode, file);
        // Add the current file path to the group's path list
        targetGroup->paths[targetGroup->referenceCount] = strdup(fpath);
        targetGroup->referenceCount++;

        // Optionally, print out the path that was added
        //printf("Added hardlink: %s \n", fpath);
    }
    // softlink
    
    if (tflag == FTW_SL){
        
        // Symbolic link
        struct stat target_stat;
        unsigned long targetInode;
        
        

        if (stat(fpath, &target_stat) == 0){
            targetInode = target_stat.st_ino;
        }
        targetGroup = createHardLinkGroup(targetInode, file);
        // we have a target hardlink group, but the soft link may or may not already be there
        SoftLinkGroup * targetSoftGroup = NULL;
        int i;
        for (i = 0; i < targetGroup->num_symlinks; i++) {
            if (targetGroup->symlinks[i].inode == inode) {
                targetSoftGroup = &targetGroup->symlinks[i];
                break;
            }
        }
        // create the new entry
        if (targetSoftGroup == NULL){
            
            // Initialize a new HardLinkGroup for the inode
            targetSoftGroup = &targetGroup->symlinks[targetGroup->num_symlinks];
            targetSoftGroup->inode = inode;
            targetSoftGroup->referenceCount = 0;  // No paths yet
            targetSoftGroup->paths = malloc(sizeof(char *) * MAX_PATHS);  // Allocate space for 10 paths initially
            

            // Increment the number of symlink groups
            targetGroup->num_symlinks++;
        }
        //printf("\n\n%d\n\n", targetSoftGroup->referenceCount);
        targetSoftGroup->paths[targetSoftGroup->referenceCount] = strdup(fpath);
        
        targetSoftGroup->referenceCount++;
    }

        
        


    

    return 0;
}



// Function to print file information in the required format
void print_file_info() {
    File *file = filetable;
    int fileNumber = 1;
    while (file != NULL){
            // Print the file number and MD5 hash
        printf("File %d:\n", fileNumber);
        printf("\tMD5 Hash: ");
        for (int i = 0; i < 16; i++) {
            printf("%02x", file->hash[i]);
        }
        printf("\n");

        // Iterate over the hard link groups
        for (int i = 0; i < file->num_groups; i++) {
            HardLinkGroup *group = &file->groups[i];

            // Print the hard link information
            printf("\tHard Link (%d): %lu\n", group->referenceCount, group->inode);

            // Print all paths in this hard link group
            printf("\t\t\tPaths:\t%s\n", group->paths[0]);
            for (int j = 1; j < group->referenceCount; j++) {
                printf("\t\t\t\t%s\n", group->paths[j]);
            }

            // Print all soft link groups
            for (int k = 0; k < group->num_symlinks; k++) {
                SoftLinkGroup *symlinkGroup = &group->symlinks[k];
                printf("\t\t\tSoft Link %d(%d): %lu\n", k + 1, symlinkGroup->referenceCount, symlinkGroup->inode);
                
                
                printf("\t\t\t\tPaths:\t%s\n", symlinkGroup->paths[0]);
                for (int m = 1; m < symlinkGroup->referenceCount; m++) {
                    printf("\t\t\t\t\t%s\n", symlinkGroup->paths[m]);
                }
                
            }
        }
        file = file->hh.next;
        fileNumber++;
    }  
    
}

HardLinkGroup* createHardLinkGroup(unsigned long inode, File* file){
    // creates target group hardlink entry depending given a target inode
    HardLinkGroup *targetGroup = NULL;
    for (int i = 0; i < file->num_groups; i++) {
        if (file->groups[i].inode == inode) {
            targetGroup = &file->groups[i];
            break;
        }
    }
    // create the new entry
    if (targetGroup == NULL){
        // Initialize a new HardLinkGroup for the inode
        targetGroup = &file->groups[file->num_groups];
        targetGroup->inode = inode;
        targetGroup->referenceCount = 0;  // No paths yet
        targetGroup->paths = malloc(sizeof(char *) * MAX_PATHS);  // Allocate space for 10 paths initially
        targetGroup->num_symlinks = 0;
        targetGroup->symlinks = malloc(sizeof(SoftLinkGroup) * MAX_GROUPS);

        // Increment the number of groups
        file->num_groups++;
    }
    
    return targetGroup;
}

