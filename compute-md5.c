/* short program to illustrate using MD5 hashing 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <openssl/evp.h>
#include "compute-md5.h"

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

// int main(int argc, char *argv[]) {
//   int j;
//   const char *filename ;
//   unsigned char md5_value[EVP_MAX_MD_SIZE];
//   unsigned int md5_len;
//   int err; 
//   EVP_MD_CTX *mdctx;
  
//   if (argc != 2) {
//         fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
//         return 1;
//   }
//   filename = argv[1];

//   /* create a new MD5 context structure */ 
//   mdctx = EVP_MD_CTX_new();
//   if (!mdctx) {
//     fprintf(stderr, "%s::%d::Error allocating MD5 context %d\n", __func__, __LINE__, errno);
//     exit(EXIT_FAILURE);
//   }

//   md5_len = 0 ;  /* the length is returned from the library */

//   err = compute_file_hash(filename,mdctx,md5_value,&md5_len);
//   if (err < 0) {
//     fprintf(stderr, "%s::%d::Error computing MD5 hash %d\n", __func__, __LINE__, errno);
//     exit(EXIT_FAILURE);    
//   }
  

//   printf("\tMD5 Hash: ");
//   for (int i = 0; i < md5_len; i++) {
//     printf("%02x", md5_value[i]);
//   }
//   printf("\n");

//   EVP_MD_CTX_free(mdctx); // don't create a leak!
  
//   return 0;
// }
