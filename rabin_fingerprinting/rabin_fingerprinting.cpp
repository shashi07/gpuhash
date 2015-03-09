#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include "rabin_polynomial.h"
#include "rabin_polynomial_constants.h"

using namespace std;

uint64_t rabin_polynomial_prime=RAB_POLYNOMIAL_REM;

unsigned int rabin_sliding_window_size=RAB_POLYNOMIAL_WIN_SIZE;

unsigned int rabin_polynomial_max_block_size=RAB_MAX_BLOCK_SIZE;
unsigned int rabin_polynomial_min_block_size=RAB_MIN_BLOCK_SIZE;

unsigned int rabin_polynomial_average_block_size=RAB_POLYNOMIAL_AVG_BLOCK_SIZE;

int rabin_poly_init_completed=0;
uint64_t *polynomial_lookup_buf;

int initialize_rabin_polynomial_defaults() {
        
    if(rabin_poly_init_completed != 0)
        return 1; //Nothing to do
    
    polynomial_lookup_buf=(uint64_t *)malloc(sizeof(uint64_t)*RAB_POLYNOMIAL_MAX_WIN_SIZE);
    
    if(polynomial_lookup_buf == NULL) {
        fprintf(stderr, "Could not initialize rabin polynomial lookaside buffer, out of memory\n");
        return 0;
    }
    
    int index=0;
    uint64_t curPower=1;
    //Initialize the lookup values we will need later
    for(index=0;index<RAB_POLYNOMIAL_MAX_WIN_SIZE;index++) {
        //TODO check if max window size is a power of 2
        //and if so use shifters instead of multiplication
        polynomial_lookup_buf[index]=curPower;
        curPower*=rabin_polynomial_prime;
    }
      
    rabin_poly_init_completed=1;
  
    return 1;
    
}



int initialize_rabin_polynomial(uint64_t prime, unsigned max_size, unsigned int min_size, unsigned int average_block_size) {
    
    rabin_polynomial_prime=prime;
    rabin_polynomial_max_block_size=max_size;
    rabin_polynomial_min_block_size=min_size;
    rabin_polynomial_average_block_size=average_block_size;
    
    return initialize_rabin_polynomial_defaults();
    
    
}



struct rabin_polynomial *gen_new_polynomial(struct rabin_polynomial *current_poly, uint64_t total_len, uint16_t length, uint64_t rab_sum) {
    
    struct rabin_polynomial *next= (rabin_polynomial*) malloc(sizeof(struct rabin_polynomial));
    
    if(next == NULL) {
        fprintf(stderr, "Could not allocate memory for rabin fingerprint record!");
        return NULL;
    }
    next->start=total_len-length;
    next->length=length;
    next->polynomial=rab_sum;
    
    return next;
    
}


struct rab_block_info *init_empty_block() {
    
    initialize_rabin_polynomial_defaults();
	struct rab_block_info *block=  (rab_block_info *) malloc(sizeof(struct rab_block_info));
    if(block == NULL) {
        fprintf(stderr,"Could not allocate rabin polynomial block, no memory left!\n");
        return NULL;
    }
	
	block->current_poly=gen_new_polynomial(NULL,0,0,0);
    
	if(block->current_poly== NULL)
        return NULL; //Couldn't allocate memory
    
	block->cur_roll_checksum=0;
	block->total_bytes_read=0;
	block->window_pos=0;
	block->current_poly_finished=0;
    
    block->current_window_data=(char*)malloc(sizeof(char)*rabin_sliding_window_size);
    
	if(block->current_window_data == NULL) {
	    fprintf(stderr,"Could not allocate buffer for sliding window data!\n");
	    free(block);
	    return NULL;
	}
    int i;
	for(i=0;i<rabin_sliding_window_size;i++) {
	    block->current_window_data[i]=0;
	}
    
    return block;
}


struct rab_block_info *read_rabin_block(void *buf, ssize_t size, struct rab_block_info *cur_block,vector<rabin_polynomial*> &poly_list) {
    struct rab_block_info *block;
    
    if(cur_block == NULL) {
        block=init_empty_block();
        if(block == NULL)
            return NULL;
    }
    
    else {
     	block=cur_block;
    }
    //We ended on a border, gen a new current_poly
    if(block->current_poly_finished) {
        struct rabin_polynomial *new_poly=gen_new_polynomial(NULL,0,0,0);
        block->current_poly=new_poly;
        block->current_poly_finished=0;
    }
   

    ssize_t i;
    for(i=0;i<size;i++) {
    	char cur_byte=*((char *)(buf+i));
        char pushed_out=block->current_window_data[block->window_pos];
        block->current_window_data[block->window_pos]=cur_byte;
        block->cur_roll_checksum=(block->cur_roll_checksum*rabin_polynomial_prime)+cur_byte;
        block->current_poly->polynomial=(block->current_poly->polynomial*rabin_polynomial_prime)+cur_byte;
        block->cur_roll_checksum-=(pushed_out*polynomial_lookup_buf[rabin_sliding_window_size]);
        
        block->window_pos++;
        block->total_bytes_read++;
        block->current_poly->length++;
        
        if(block->window_pos == rabin_sliding_window_size) //Loop back around
            block->window_pos=0;
        
        //If we hit our special value or reached the max win size create a new block
        if((block->current_poly->length >= rabin_polynomial_min_block_size && (block->cur_roll_checksum % rabin_polynomial_average_block_size) == rabin_polynomial_prime)|| block->current_poly->length == rabin_polynomial_max_block_size) {
            block->current_poly->start=block->total_bytes_read-block->current_poly->length;
            struct rabin_polynomial *new_poly=gen_new_polynomial(NULL,0,0,0);
            poly_list.push_back(block->current_poly);
            block->current_poly=new_poly;
            
            if(i==size-1)
                block->current_poly_finished=1;
        }
    }
    
    return block;
    
}


void get_file_rabin_polys(FILE *file_to_read) {
    
    initialize_rabin_polynomial_defaults();

    vector<rabin_polynomial*> poly_list;

    struct rab_block_info *block=NULL;
    char *file_data=(char*)malloc(RAB_FILE_READ_BUF_SIZE);
    
    if(file_data == NULL) {
        fprintf(stderr,"Could not allocate buffer for reading input file to rabin polynomial.\n");
    }
    
    ssize_t bytes_read=fread(file_data,1,RAB_FILE_READ_BUF_SIZE,file_to_read);
    
    while(bytes_read != 0) {
        block=read_rabin_block(file_data,bytes_read,block,poly_list);
        bytes_read=fread(file_data,1,RAB_FILE_READ_BUF_SIZE,file_to_read);
    }
    
    free(file_data);
    
    for(int i=0;i<poly_list.size();i++)
		cout<<(poly_list[i])->start<<" "<<(poly_list[i])->length<<" "<<(poly_list[i])->polynomial<<endl;
	free(block->current_window_data);
    free(block);
    free(polynomial_lookup_buf);
}



int main(int argc, char **argv){

	    FILE *input_file=fopen(argv[argc-1], "rb+");
    
    if(input_file == NULL) {
        fprintf(stderr, "Could not open file %s for reading!\n",argv[argc-1]);
        return -1;
    }

    get_file_rabin_polys(input_file);

	return 0;	
}