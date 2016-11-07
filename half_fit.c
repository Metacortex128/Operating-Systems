/*
MTE 241, Computer Structures and Real-time Systems Project 3
Jong Sha Han
j49han@uwaterloo.ca
*/



#include "half_fit.h"
#include <lpc17xx.h>
#include <stdio.h>
#include <stdbool.h>
#include <limits.h>
#include "uart.h"

unsigned char array[MAX_SIZE] __attribute__ ((section(".ARM.__at_0x10000000"), zero_init));
U32 *bucket_address[11]; //save the address of the bucket[num]
U32 bit_vector ;


U32 logof2( U32 n) {
	register U32 r; // result of log2(v) will go here
	register U32 shift;

	r =     (n > 0xFFFF) << 4; n >>= r;
	shift = (n > 0xFF  ) << 3; n >>= shift; r |= shift;
	shift = (n > 0xF   ) << 2; n >>= shift; r |= shift;
	shift = (n > 0x3   ) << 1; n >>= shift; r |= shift;
	r |= (n >> 1);
	return r;
}


//get_address will convert the address stored in header
//to actuall address
__inline U32 *get_address(U32 address){
	return (U32 *)(start_adr|(address<<smlst_blk));
}

//get_address_index will convert the actual address to address index
__inline U32 get_address_index(U32* address){
	return (BIT_MASK & ((U32)address>>smlst_blk));
}


//get bucket num to allocate xx chunks
U32 get_bucket_num_alloc(U16 chunks){
	return logof2(--chunks)+1;
}
//get bucket num of the chunks
U32 get_bucket_num(U16 chunks){	
	return logof2(chunks);
}

S16 find_free_bucket(U32 size){
	U32 bucket_num;
	U32 bucket_mask;

	bucket_num = get_bucket_num_alloc(bytes_to_chunk(size));
	//checking bit vector for free bucket
	while(bucket_num<=10){
		bucket_mask = 1<<bucket_num;
		
		//if bucket is available then return bucket number
		if(bucket_mask&bit_vector)
			return bucket_num;

		++bucket_num;
	}
	
	return -1; //not found	
}


//implement free_bucket_header as stack 

void push_bucket(U16 bucket_num, U32* address){
	bucket_free_header_t* bucket_header;
	bucket_free_header_t* old_bucket_header;
	
	bucket_header = (bucket_free_header_t*)(address+1);
	bucket_header->next = NULL;
	bucket_header->prev = NULL;
	
	//if there's no free block inside the bucket, set the bit that correspond to bucket num to 0
	if (!bucket_address[bucket_num])
		set_bit_vector(bucket_num);
	//else, update the old bucket header prev to point to the new one
	else{
		old_bucket_header = (bucket_free_header_t*)(bucket_address[bucket_num]+1);
		bucket_header->next = get_address_index(bucket_address[bucket_num]);
		old_bucket_header->prev = get_address_index(address);
	}
	bucket_address[bucket_num] = address;
}

void pop_bucket(U32 bucket_num){
	bucket_free_header_t* bucket_header;
	U32* next_adr;
	
	bucket_header =  (bucket_free_header_t*)(bucket_address[bucket_num]+1);
	
	//pop the old bucket from the stack
	if (!bucket_address[bucket_num]){
		next_adr = get_address(bucket_header->next);
		bucket_address[bucket_num] = next_adr;
		((bucket_free_header_t*)(next_adr+1))->prev = NULL;
	}
	else{
		set_bit_vector(bucket_num);
		bucket_address[bucket_num]= NULL;
	}
}

void pop_specific_block(U32 bucket_num, U32* address){
	bucket_free_header_t* bucket_header;
	bucket_free_header_t* next_header;
	bucket_free_header_t* prev_header;
	
	bucket_header =  (bucket_free_header_t*)(address+1);
	
	//address= top of the stack
	if(bucket_header->next != NULL && bucket_header->prev == NULL){
		pop_bucket(bucket_num);
	}
	//address= bottom of the stack
	else if(bucket_header->next == NULL && bucket_header->prev != NULL){
		prev_header = (bucket_free_header_t*)get_address(bucket_header->prev)+1;
		prev_header->next = NULL;
		bucket_header->prev = NULL; 
	}
	//address= middle of the stack
	else{
		prev_header = (bucket_free_header_t*)get_address(bucket_header->prev)+1;
		next_header = (bucket_free_header_t*)get_address(bucket_header->next)+1;
		prev_header->next = bucket_header->next;
		next_header->prev = bucket_header->prev;
		bucket_header->prev = NULL; 
		bucket_header->next = NULL; 
	}
	
	
}

//coalesce left and right free block
void coalesce(block_header_t* left,block_header_t* right){
	U32 new_size;
	U32 new_bucket_num;
	U32 left_bucket_num;
	U32 right_bucket_num;
	
	//update block header 
	left->next = right->next;

	if(right->next ^ NULL)	
		((block_header_t*)get_address(right->next))->prev = get_address_index((U32*)left);
	
	left_bucket_num = get_bucket_num((U16)(left->size));
	new_size = left->size + right->size;
	left->size = new_size;
	
	new_bucket_num = get_bucket_num((U16)new_size);
	right_bucket_num = get_bucket_num((U16)(right->size));	
	
	//pop right bucket from stack
	pop_specific_block(right_bucket_num, (U32*)right);
	
	if(left_bucket_num ^ new_bucket_num){
		//pop left bucket from the stack
		pop_specific_block(left_bucket_num, (U32*)left);
		//push the new bucket to the stack
		push_bucket((U16)new_bucket_num, (U32*)left);
	}

}


void  half_init(void){
	//initializes a block header and bucket header
	block_header_t *header;
	bucket_free_header_t *bucket_header;
	U16 i;

	//set prev and next to null
	//set size to 1024
	//set is_alloc to 0(free)
	//header = (block_header_t*)malloc(sizeof(block_header_t));
	header = (block_header_t*)&array;
	header->prev =NULL; 
	header->next =NULL; 
	header->is_alloc = 0;
	header->size = MAX_CHUNKS; 
	
	//set prev free and next free = null
	bucket_header = ((bucket_free_header_t*)&array) + 1;
	bucket_header->prev = NULL;
	bucket_header->next = NULL;
	
	bit_vector = (1<<10);
	
	for(i = 0; i<10 ; i++){
		bucket_address[i]=NULL;
	}
	bucket_address[10] = (U32*)&array;
}

void *half_alloc(U32 size){	
	S16 bucket_num;
	U16 new_bucket;
	U32 new_size;
	U32 new_adr_index;
	U32 old_adr_index;
	U32 old_size;
	U32 alloc_size;
	block_header_t *new_address;
	block_header_t *old_address;
			
	size += 4; //(add 4 bytes header size)	
	alloc_size = bytes_to_chunk(size);
	
	if(size > MAX_SIZE)
		return NULL;
	
	//find bucket number
	bucket_num = find_free_bucket(size);

	if(!(bucket_num+1)){
		//no bucket is available
		return NULL;
	}
	
	//split block & update block_header
	old_address = (block_header_t*)bucket_address[bucket_num];
	old_adr_index = get_address_index((U32*)old_address);

	//check if address->size = 0-> size = 1024
	old_size = old_address->size;
	if (!old_size)
		old_size = MAX_CHUNKS;
		
	//update size of old address
	new_size = old_size - alloc_size; 
	old_address->size = alloc_size;

	
	//if new_size > 0 -> need to divide the memory
	if(new_size>0){
		//create block header for the new divided block
		new_adr_index = old_adr_index + alloc_size;
		new_address = (block_header_t*)get_address(new_adr_index);
		
		new_address->size = new_size;
		new_address->is_alloc = 0;
		
		//update the block header next & prev pointer
		new_address->next = old_address->next;
		if(old_address->next != NULL)	
			((block_header_t*)get_address(old_address->next))->prev = new_adr_index;
		old_address->next = new_adr_index;
		new_address->prev = old_adr_index;
		
		new_bucket = get_bucket_num(new_size);
		//if new_bucket < bucket_num -> have to pop the used bucket and push a new one
		if(new_bucket < bucket_num){
			pop_bucket(bucket_num);
			push_bucket(new_bucket, (U32*)new_address);		
		}
		//else just update the bucket_address to the new address
		else
			bucket_address[bucket_num] = (U32*)new_address;
	}
	
	else
		pop_bucket(bucket_num);
		//pop used bucket from stack
	
	//change alloc state
	old_address->is_alloc = 1;

	//+1 -> dont give user access to header
	return (old_address+1);
}

void  half_free(void * address){
	block_header_t *block_h;
	block_header_t *left_block;
	block_header_t *right_block;
	U32 bucket_num;
	U32 size;

	block_h = (block_header_t*)address-1;
	
	//change is_alloc state
	block_h->is_alloc = 0;
	
	//check if left and right block is free
	right_block = (block_header_t*)get_address(block_h->next);
	left_block = (block_header_t*)get_address(block_h->prev);
	
	//if size == 0 -> size = 1024
	size = block_h->size;
	if(!size)
		size = MAX_CHUNKS;
		
	bucket_num = get_bucket_num(size);
	push_bucket((U16)bucket_num, (U32*)block_h);
	
	//coalesce block with left 	(case left is free)
	if(!left_block->is_alloc && left_block != block_h){
		coalesce(left_block, block_h);
	
		//coalesce left with right (case both are free) 
		//if right_block == start_adr ->block_h is tail
		if(!(right_block->is_alloc) && right_block !=(block_header_t*)start_adr){
			coalesce(left_block, right_block);
	}}
	
	//coalesce block with right (case right is free)
	else if(!(right_block->is_alloc) && right_block !=(block_header_t*)start_adr){
		coalesce(block_h, right_block);
	}

		
}
