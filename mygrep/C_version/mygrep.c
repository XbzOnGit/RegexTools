#include "nfa.h"
#include<stdio.h>
#include<limits.h>
#include<stdint.h>
#include<stdlib.h>
long getfilesize(FILE* fp){
    if(fseek(fp, 0L, SEEK_END) != 0){
        fprintf(stderr,"Cannot fseek input file to end in mygrep\n");
        exit(1);
    }
    long sz = ftell(fp);
    if(sz < 0){
        fprintf(stderr,"Fail to ftell to get file size\n");
        exit(1);
    }
    rewind(fp);
    return sz;
}
#define STDIN_INPUT_BUF_LEN (8193)
#define STDIN_INPUT_EFFECTIVE_LEN_STR "8192"

int main(int argc, char** argv){
    FILE* input_file_stream = NULL;
    char* input_buf = NULL;
    size_t length_of_input_str = 0;
    if(argc <= 1 || argc > 3){
        fprintf(stderr,"Usage: ./mygrep pattern <input_file_name>\n");
        return 1;
    }
    else if(argc == 2){
        input_file_stream = stdin;
        input_buf = (char*)malloc(STDIN_INPUT_BUF_LEN);
        if(input_buf == NULL){
            fprintf(stderr,"Fail to malloc input buffer for stdin\n");
            exit(1);
        }
        int inputlen = scanf("%"STDIN_INPUT_EFFECTIVE_LEN_STR"s",input_buf);
        if(inputlen < 0){
            fprintf(stderr,"scanf in mygrep returns < 0: %d\n",inputlen);
            exit(1);
        }
        else if(inputlen >= (STDIN_INPUT_BUF_LEN-1)){
            fprintf(stderr,"Warning: Input from stdin with length > "STDIN_INPUT_EFFECTIVE_LEN_STR" are just discarded\n");
            exit(1);
        }
        length_of_input_str = (size_t)inputlen;
    }else{
        //Must be 3 arguments.
        input_file_stream = fopen(argv[2],"r");
        if(input_file_stream == NULL){
            fprintf(stderr,"Fail to fopen file: %s\n",argv[2]);
            return 1;
        }
        long fz = getfilesize(input_file_stream);
        if(fz < 0){
            fprintf(stderr,"getfilesize returns < 0\n");
            exit(1);
        }
        unsigned long long alloc_size = ((unsigned long long)fz) + 1;
        if(alloc_size > SIZE_MAX){
            fprintf(stderr,"Need alloc_size > SIZE_MAX: %llu > %zu\n",alloc_size,SIZE_MAX);
            exit(1);
        }
        input_buf = (char*)malloc((size_t)alloc_size);
        if(input_buf == NULL){
            fprintf(stderr,"Fail to malloc input_buf for files: alloc_size is %llu\n",alloc_size);
            exit(1);
        }
        if(fread(input_buf,sizeof(char),fz,input_file_stream) < fz){
            fprintf(stderr,"fread fails to read the whole file\n");
            exit(1);
        }
        length_of_input_str = (size_t)fz;
    }
    input_buf[length_of_input_str] = 0;//Buffer size has one more char.
    const char* regex_str = argv[1];
    struct NFA_oneacc* nfapt = Regex2NFA(regex_str);
    if(NFASimu(nfapt,input_buf,length_of_input_str)){
        printf("yes\n");
    }else{
        printf("no\n");
    }
    free_NFA(nfapt);
    free(input_buf);
    return 0;
}