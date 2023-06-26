/*
	Name: Kieran Le
*/

// The MIT License (MIT)
// 
// Copyright (c) 2022 Trevor Bakker 
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "mavalloc.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

// type is whether the Node is available to do memory allocation from
enum TYPE
{
    FREE = 0,
    USED
};

// size is the length of the memory segment
// arena will point to the starting address of the memory segment
// using doubly linked list, so pointer to next Node and pointer to previous Node
struct Node {
  size_t size;
  enum TYPE type;
  void * arena; 
  struct Node * next;
  struct Node * prev;
};

// alloc_list will be the head of the linked list
// keeping track of the memory segments.
// previous_node will be keeping track of where memory allocation
// left off in the linked list in order to perform NEXT_FIT.
struct Node *alloc_list;
struct Node *previous_node;

void * arena;

// Default algorithm is FIRST_FIT.
// Algorithm can be changed later when mavalloc_init() is called
enum ALGORITHM allocation_algorithm = FIRST_FIT;

// mavalloc_init() allocates a pool of memory that is size bytes big.
int mavalloc_init( size_t size, enum ALGORITHM algorithm )
{
  // If the size parameter is less than zero then return -1.
  if( size < 0 )
  {
	return -1;  
  }
  
  // This arena will be for alloc_list, where arena
  // will point to start address of the space reserved 
  // by malloc for the memory pool.
  arena = malloc( ALIGN4( size ) );
  
  // If allocation fails return -1.
  if( arena == NULL )
  {
	return -1; 
  }
  
  allocation_algorithm = algorithm;

  // alloc_list is the head of the linked list of
  // allocated and free memory segments of the memory pool
  alloc_list = ( struct Node * )malloc( sizeof( struct Node ));

  // The starting address of the head of the linked list
  // is the same as the starting address of the memory pool.
  // The size attribute of alloc_list starts off as
  // the size of the entire memory pool because the 
  // application has not called mavalloc yet.
  // The type of the head starts off as FREE because the
  // application has not called mavalloc yet.
  // The next node and the previous node of the head is NULL
  // because the head is the only node in the linked list right now.
  alloc_list -> arena = arena; 
  alloc_list -> size  = ALIGN4(size);
  alloc_list -> type  = FREE;
  alloc_list -> next  = NULL;
  alloc_list -> prev  = NULL;

  // The first node that NEXT_FIT checks for availability
  // will be the head for the first mavalloc_alloc().
  previous_node  = alloc_list;

  // If the allocation succeeds return 0.
  return 0;
}

void mavalloc_destroy( )
{
  // Free the memory in the memory pool.
  free( arena );
  
  // Iterate over the linked list and free the nodes.
  struct Node * node = alloc_list;
  
  // Don't need to do anything if the head is already NULL.
  if( node == NULL )
  {
	return;  
  }
  // If the head isn't NULL but the next node is NULL, then
  // just free the head and then make head NULL.
  if( node != NULL && node -> next == NULL )
  {
	free( node );  
	alloc_list = NULL;
	return;
  }
  // If both head and the next node is not NULL, then start
  // by going to the next node, then freeing the previous node
  // until the next node is NULL.
  // The head is set to NULL after all the nodes got freed.
  while(node -> next)
  {
	node = node -> next;
    free(node -> prev);	  
  }
  alloc_list = NULL;
  return;
}

void * mavalloc_alloc( size_t size )
{
  // struct Node * node will be used to iterate over linked list
  // to find from which node in the linked list
  // should memory be allocated from 
  struct Node * node;

  // Iteration should start from the head of the linked list
  // in all heap allocation algorithms except NEXT_FIT.
  if( allocation_algorithm != NEXT_FIT )
  { 
    node = alloc_list;
  }
  // Iteration should start at the node left off from the
  // previous allocation for NEXT_FIT.
  else if ( allocation_algorithm == NEXT_FIT )
  {
    node = previous_node;
  }
  else
  {
    printf("ERROR: Unknown allocation algorithm!\n");
    exit(0);
  }

  // All memory allocations must be word-aligned.
  size_t aligned_size = ALIGN4( size );

  if( allocation_algorithm == FIRST_FIT )
  {
	// If node == NULL, that means all the nodes in the linked list
	// have been checked from beginning to end,
	// so the while loop ends and no memory is allocated.
	
    while( node )
    {
	  // Can only allocate memory from a node that 
	  // has enough memory AND is a hole.
      if( node -> size >= aligned_size  && node -> type == FREE )
      {
		// leftover_size is how much memory of the chosen node
		// may be unallocated after memory has been allocated from
		// the chosen node.
        int leftover_size = 0;
		// The node is now USED since memory has been allocated
		// from it for a prcoess.
		// The size of the memory of the node is reduced if
		// necessary so the unallocated memory can be used
		// by another process.
        node -> type  = USED;
        leftover_size = node -> size - aligned_size;
        node -> size =  aligned_size;
  
        // If there will be unallocated memory left from the
		// chosen node, then need to make a new node (called
		// leftover_node) for that free memory segment.
        if( leftover_size > 0 )
        {
		  // Because the node for the new free memory segment (leftover_node)
		  // is going to be between the node chosen by the algorithm and the 
		  // chosen node's next node, a reference (prevoius_node) to the
		  // chosen node's next node is needed such that the chosen node
		  // has its next node be leftover_node, and leftover_node has its
		  // next node be the previous next node of the chosen node.
          struct Node * previous_next = node -> next;
          struct Node * leftover_node = ( struct Node * ) malloc ( sizeof( struct Node ));
  
		  // The type of the leftover_node is free because leftover_node is
		  // the free memory segment resulting from allocating only some of
		  // the memory from the node chosen by the algorithm.
		  // The start address of the memory allocated from the chosen node
		  // plus the size of the memory allocated is the start address
		  // of the new free memory segment.
		  // leftover_node is added to the linked list
		  // between node and previous_next by having it
		  // point to previous_next as its next node and node
		  // as its previous node, and node points to leftover_node
		  // as its next node.
		  // If previous_next is not a NULL node, then need
		  // to maintain doubly linked list by having it point to leftover_node
		  // as its previous node.
          leftover_node -> arena = node -> arena + size;
          leftover_node -> type  = FREE;
          leftover_node -> size  = leftover_size;
          leftover_node -> next  = previous_next;
		  leftover_node -> prev = node;
		  if(previous_next != NULL)
		  {
		    previous_next -> prev = leftover_node;        
		  }
          node -> next = leftover_node;
        }
		
		// This function returns a pointer to the memory on success.
        return ( void * ) node -> arena;
      }
	  // If the node does not have free memory (or not enough free memory),
	  // then check if the next node in the linked list can be used to
	  // allocate memory from.
      node = node -> next;
    }
  }
  else if( allocation_algorithm == NEXT_FIT )
  {
	// The NEXT_FIT algorithm is the same as the FIRST_FIT algorithm except
	// the node iteration doesn't necessarily start at the head of the
	// linked list, and thus need the ability to loop back to the beginning
	// of the linked list.
	// int stop will allow us to loop back to the beginning of the linked
	// list if necessary. We break out of the while loop iterating over the
	// linked list either if a node gets chosen to allocate from, or
	// we have already looped back to the beginning of the list and
	// reached once again the start_node (the first previous_node).
	// Previous_node is updated to the node chosen for memory allocation
	// such that the next time mavalloc() is called, the NEXT_FIT
	// algorithm can start checking nodes from where the last mavalloc()
	// left off.
    int stop = 0;
	struct Node * start_node = node;
    while( node )
    {
	  if( stop == 1 && node == start_node)
	  {
		break;  
	  }
      if( node -> size >= aligned_size  && node -> type == FREE )
      {
        int leftover_size = 0;
  
        node -> type  = USED;
        leftover_size = node -> size - aligned_size;
        node -> size =  aligned_size;
    
        if( leftover_size > 0 )
        {
          struct Node * previous_next = node -> next;
          struct Node * leftover_node = ( struct Node * ) malloc ( sizeof( struct Node ));
  
          leftover_node -> arena = node -> arena + size;
          leftover_node -> type  = FREE;
          leftover_node -> size  = leftover_size;
          leftover_node -> next  = previous_next;
		  leftover_node -> prev = node;
		  if(previous_next != NULL)
		  {
		    previous_next -> prev = leftover_node;        
		  }
          node -> next = leftover_node;
        }     
		// Need to make previous_node = node so next time run NEXT_FIT,
		// it starts where the previous run of NEXT_FIT left off.
        previous_node = node; 
        return ( void * ) node -> arena;
      }
	  
	  // The NEXT_FIT algorithm has been a copy of the
	  // FIRST_FIT algorithm with the following being
	  // specific to the NEXT_FIT algorithm.
	  // This checks that if we reached the end of the
	  // linked list without choosing a node to allocate
	  // memory from, then we can start from the beginning
	  // of the linked list and stop
	  // once we reach back to the node we started from.
      else if( node -> next == NULL && stop != 1)
      {
        node = alloc_list;
        stop = 1;
      }
	  
	  // If we did not reach end of linked list, then
	  // just start checking the next node for availability.
	  else
	  {
        node = node -> next;
	  }
    }
  }
  else if( allocation_algorithm == WORST_FIT )
  {
    // WORST_FIT algorithm is a copy of the FIRST_FIT algorithm,
	// except need to iterate over the entire linked list once without 
	// allocating any memory such that the node chosen by the algorithm
	// is biggest_node, the node with the biggest size
	// that can be used for memory allocation (so also need to check if 
	// the node is indeed FREE and has enough memory). 
    long biggest_size = 0;
    struct Node * biggest_node;
    while( node )
    {
      if( node -> size >= aligned_size && node -> type == FREE && node -> size >= biggest_size )
      {
          biggest_size = node -> size; 
          biggest_node = node;
      }
      node = node -> next;
    }
    
    if( biggest_size > 0 )
    {
      int leftover_size = 0;
                
      biggest_node -> type = USED;
      leftover_size = biggest_node -> size - aligned_size; 
      biggest_node -> size = aligned_size;
                
      if( leftover_size > 0 )
      {
        struct Node * previous_next = biggest_node -> next;
        struct Node * leftover_node = ( struct Node * ) malloc ( sizeof( struct Node ) );
        leftover_node -> arena = biggest_node -> arena + size;
        leftover_node -> type  = FREE;
        leftover_node -> size  = leftover_size;
        leftover_node -> next  = previous_next;
		leftover_node -> prev = biggest_node;
		if(previous_next != NULL)
		{
		  previous_next -> prev = leftover_node;        
		}
			
        biggest_node -> next = leftover_node;
      }
		
      return ( void * ) biggest_node -> arena;          
    }
  }
  else if( allocation_algorithm == BEST_FIT )
  {
	// BEST_FIT algorithm is a copy of the FIRST_FIT algorithm,
	// except need to iterate over the entire linked list once without 
	// allocating any memory such that the node chosen by the algorithm
	// is the smallest node that fits the aligned_size such that
	// it can be used for memory allocation (so also need to check if 
	// the node is indeed FREE).
    // int available is to determine whether a FREE node that has
    // enough length has been found. If it hasn't been found,
	// then we only need to check that the node is FREE and has enough length
	// in order to pick choose the node as the current BEST_FIT.
	// If a node has already been chosen as the current BEST_FIT, then
	// to determine whether the next node should be BEST_FIT, then 
	// we also need to check that the next node is smaller than the current
	// BEST_FIT node.
	long best_size;
    struct Node * best_node;
	int available = 0;
    while( node )
    {
	  if( available == 0 )
	  {
		if( node -> size >= aligned_size && node -> type == FREE )
		{
		  best_size = node -> size; 
		  best_node = node;
		  available = 1;
		}
	  }
      else if( node -> size >= aligned_size && node -> type == FREE && node -> size <= best_size )
      {
        best_size = node -> size; 
        best_node = node;
		available = 1;
      }
      node = node -> next;
    }
	if( available == 1 )
    {
      int leftover_size = 0;
                
      best_node -> type = USED;
      leftover_size = best_node -> size - aligned_size; 
      best_node -> size = aligned_size;
                
      if( leftover_size > 0 )
      {
        struct Node * previous_next = best_node -> next;
        struct Node * leftover_node = ( struct Node * ) malloc ( sizeof( struct Node ) );
        leftover_node -> arena = best_node -> arena + size;
        leftover_node -> type  = FREE;
        leftover_node -> size  = leftover_size;
        leftover_node -> next  = previous_next;
		leftover_node -> prev = best_node;
		if(previous_next != NULL)
		{
		  previous_next -> prev = leftover_node;        
		}
			
        best_node -> next = leftover_node;
      }
		
      return ( void * ) best_node -> arena;          
    }
  }

  // Return NULL on failure.
  return NULL;
}

void mavalloc_free( void * ptr )
{
  // Need to iterate over the linked list to find the node for
  // the block we want to free back to the memory arena.
  // Start by checking the head of the linked list.
  struct Node * node = alloc_list;

  while( node )
  {
    if( node -> arena == ptr )
    {
      if( node -> type == FREE )
      {
        printf("Warning: Double free detected \n");
      }
	  // Now the node can be chosen for memory allocation when
	  // doing mavalloc_alloc().
      node -> type = FREE;
		
	  // Need to check if need to do coalescing.
	  // If both neighbors of the node that got updated to FREE
	  // are also free, then coalesce all three entries such that
      // the length of the entry closest to the head gains the
      // length of the following two entries, and the following
      // two entries can then be freed.	  
	  // Need to link the previous node to its new neigbor
	  // (node -> next -> next) before freeing node and node -> next.
	  if(node -> prev != NULL && node -> next != NULL)
	  {	 
		if(node -> prev -> type == FREE && node -> next -> type == FREE)
	 	{  
		  node -> prev -> size = node -> prev -> size + node -> size + node -> next -> size;
		  node -> prev -> next = node -> next -> next;
		  if(node -> next -> next != NULL)
		  {
		    node -> next -> next -> prev = node -> prev;
		  }
		  
		  free(node -> next);
		  free(node);
		  // This function returns no valaue.
	      return;
		}
	  }
	  // If only the previous neighbor is also FREE, then
	  // then coalesce the previous neighbor and
	  // the node that got updated to FREE such that the length
	  // of the previous neighbor gains the length of the 
	  // node that got updated to FREE, and then the node that
	  // that got updated to FREE can be freed.
	  // Need to link the previous neighbor to its new neighbor
	  // before freeing node.
	  if(node -> prev != NULL)
      {
		if(node -> prev -> type == FREE)
		{	 
		  node -> prev -> size = node -> prev -> size + node -> size;
		  node -> prev -> next = node -> next;
		  node -> next -> prev = node -> prev;
		  free(node);
		  // This function returns no valaue.
	      return;
		}
	  }
	  // If only the previous neighbor is also FREE, then
	  // then coalesce the previous neighbor and
	  // the node that got updated to FREE such that the length
	  // of the previous neighbor gains the length of the 
	  // node that got updated to FREE, and then the node that
	  // that got updated to FREE can be freed.
	  // Need to link the previous neighbor to its new neighbor
	  // before freeing node.
	  if(node -> next != NULL)
      {
	    if(node -> next -> type == FREE)
		{	  
		  node -> size = node -> size + node -> next -> size;
	      struct Node * next_node = node -> next -> next;
		  free(node -> next);
	      node -> next = next_node;
		  
		  if(node -> next != NULL)
		  {
			node -> next -> prev = node;
		  }
		  // This function returns no valaue.
		  return;
		}
	  }
	  // This function returns no valaue.
      return;
    }
	  
	// Go to the next node if the node checked doesn't
	// have the specified pointer needed to free.
    node = node -> next;
  }

  // This function returns no valaue.
  return;
}

int mavalloc_size( )
{
  int number_of_nodes = 0;
  struct Node * ptr = alloc_list;

  // counting each node that is not NULL
  while( ptr )
  {
    number_of_nodes ++;
    ptr = ptr -> next; 
  }

  return number_of_nodes;
}
