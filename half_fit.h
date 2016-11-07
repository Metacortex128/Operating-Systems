#ifndef HALF_FIT_H_
#define HALF_FIT_H_

#include "type.h"
/*
 * Author names:
 *   1.  uWaterloo User ID:  cmsutant@uwaterloo.ca
 *   2.  uWaterloo User ID:  j49han@uwaterloo.ca
 */

#define smlst_blk                       5 
#define smlst_blk_sz  ( 1 << smlst_blk ) 
#define lrgst_blk                       15 
#define lrgst_blk_sz    ( 1 << lrgst_blk ) 

#define onechunk 32   //1 chunk = 32 bytes
#define start_adr 0x10000000
#define BIT_MASK 	((1<<11)-1) //10 bits mask

#define MAX_SIZE 32768  //in bytes 
#define MAX_CHUNKS 1024

#define set_bit_vector(i) (bit_vector^=(1<<i))
#define bytes_to_chunk(size) (((size-1)/onechunk)+1)

typedef struct block_header{
	U32 prev:10;
	U32 next:10;
	U32 size:10; //in chunks
	U32 is_alloc:1;
}block_header_t;

typedef struct bucket_free_header{
	U32 prev:10;
	U32 next:10;
}bucket_free_header_t;

void  half_init( void );
void *half_alloc( unsigned int );
void  half_free( void * );

#endif

void *half_alloc( unsigned int );
void  half_free( void * );

#endif

