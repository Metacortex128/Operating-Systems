/* 
MTE 241, Computer Structures and Real-time Systems Project 3 
Jong Sha Han
j49han@uwaterloo.ca
*/



#include <LPC17xx.h>
#include <RTL.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "quicksort.h"
#include "array_tools.h"

// The threshold, which will determine the point where insertion sort will be performed
// unit is number of elements in  array
#define USE_INSERTION_SORT 22

int intSemCounter = 0;
OS_SEM sort_semaphore;

typedef struct {
	array_t array;
	size_t a;
	size_t c;
} array_interval_t;

typedef struct{
	array_interval_t interval;
	unsigned char priority;
} qsort_task_parameters_t;

/*
 * Performs an in-place insertion sort on the array interval specified
 */
void insertion_sort( array_interval_t interval ) {
  // Your implementation here
	array_type intArrayValue;
	int intFCount, intBCount;
	
	if(interval.c - interval.a < 2)
	{
		return;
	}
	else
	{
		for(intFCount = interval.a + 1; intFCount < interval.c; intFCount++)
		{
			intArrayValue = interval.array.array[intFCount];
			for(intBCount = intFCount; intBCount > interval.a; intBCount--)
			{
				if(interval.array.array[intBCount - 1] < intArrayValue)
				{
					break;
				}
				interval.array.array[intBCount] = interval.array.array[intBCount - 1];
			}
			interval.array.array[intBCount] = intArrayValue;
		}
	}
}

/*
 * Finds the median of three numbers passed to the function
 */
int find_median(int intNumOne, int intNumTwo, int intNumThr)
{
	if(intNumOne > intNumTwo && intNumOne < intNumThr)
		return intNumOne;
	else if(intNumTwo > intNumOne && intNumTwo < intNumThr)
		return intNumTwo;
	else if(intNumThr > intNumTwo && intNumThr < intNumOne)
		return intNumThr;
	else if(intNumOne == intNumTwo && intNumOne == intNumThr)
		return intNumOne;
	else if(intNumTwo == intNumThr && intNumTwo == intNumOne)
		return intNumTwo;
	else if(intNumThr == intNumOne && intNumThr == intNumTwo)
		return intNumOne;
	
	//choose the first one if the above logic fails
	return intNumOne;
}

/*
 Is a task, defined with the task attr. __task.
 It finds the pivot value and finds the left and right side of the array.
 The parent task will either create more tasks or implement the insertion sort.
  */
__task void quick_sort_task( void* void_ptr){
  // Your implementation here
	qsort_task_parameters_t *varArgs = (qsort_task_parameters_t *)void_ptr;
	qsort_task_parameters_t varLeftPrms, varRightPrms;
	array_interval_t arrSortParams = varArgs->interval;
	array_interval_t arrLeftSort, arrRightSort;
	int intLeftSide = arrSortParams.a;
	int intRightSide = arrSortParams.c - 1;
	int intMidLoc = (intLeftSide + intRightSide)/2;
	array_type intArrayVal, intPivotVal;
	int intLkFwd, intLkBck;
	
	if(intRightSide - intLeftSide < USE_INSERTION_SORT)
	{
		insertion_sort(arrSortParams);
		os_tsk_delete_self();
		return;
	}
	else
	{
		//find pivot from values in the beginning, middle and end of the current array as a median-of-three
		intPivotVal = find_median(arrSortParams.array.array[intLeftSide], arrSortParams.array.array[intMidLoc], arrSortParams.array.array[intRightSide]);
		
		//insert the values that are not the pivot into the first and middle element of the array
		if(intPivotVal == arrSortParams.array.array[intLeftSide])
		{
			if(arrSortParams.array.array[intRightSide] > arrSortParams.array.array[intMidLoc])
			{
				arrSortParams.array.array[intLeftSide] = arrSortParams.array.array[intMidLoc];
				arrSortParams.array.array[intMidLoc] = arrSortParams.array.array[intRightSide];
			}
			else
			{
				arrSortParams.array.array[intLeftSide] = arrSortParams.array.array[intRightSide];
			}
		}
		else if(intPivotVal == arrSortParams.array.array[intMidLoc])
		{
			if(arrSortParams.array.array[intLeftSide] > arrSortParams.array.array[intRightSide])
			{
				arrSortParams.array.array[intMidLoc] = arrSortParams.array.array[intLeftSide];
				arrSortParams.array.array[intLeftSide] = arrSortParams.array.array[intRightSide];
			}
			else
			{
				arrSortParams.array.array[intMidLoc] = arrSortParams.array.array[intRightSide];
			}
		}
		else if(intPivotVal == arrSortParams.array.array[intRightSide])
		{
			if(arrSortParams.array.array[intLeftSide] > arrSortParams.array.array[intMidLoc])
			{
				intArrayVal = arrSortParams.array.array[intLeftSide];
				arrSortParams.array.array[intLeftSide] = arrSortParams.array.array[intMidLoc];
				arrSortParams.array.array[intMidLoc] = intArrayVal;
			}
		}
		
		//perform swapping to order array
		intLkFwd = intLeftSide;
		intLkBck = intRightSide - 1;
		
		while(intLkFwd <= intLkBck)
		{
			while(arrSortParams.array.array[intLkFwd] < intPivotVal)
			{
				intLkFwd++;
			}
			
			while(arrSortParams.array.array[intLkBck] > intPivotVal)
			{
				intLkBck--;
			}
			
			if(intLkFwd <= intLkBck)
			{
				intArrayVal = arrSortParams.array.array[intLkBck];
				arrSortParams.array.array[intLkBck] = arrSortParams.array.array[intLkFwd];
				arrSortParams.array.array[intLkFwd] = intArrayVal;
				intLkFwd++;
				intLkBck--;
			}
		}
		
		//place pivot at point of intersection where the value is
		//currently greater than the pivot, with the current value
		//being placed at the end of the array
		arrSortParams.array.array[intRightSide] = arrSortParams.array.array[intLkFwd];
		arrSortParams.array.array[intLkFwd] = intPivotVal;
		
		//set up the values to pass along to the child tasks
		//and create the child tasks
		if(intLeftSide < intLkBck)
		{
			arrLeftSort.array = arrSortParams.array;
			arrLeftSort.a = intLeftSide;
			arrLeftSort.c = intLkBck + 1;
			varLeftPrms.interval = arrLeftSort;
			varLeftPrms.priority = varArgs->priority + 1;
			
			os_tsk_create_ex(quick_sort_task, varLeftPrms.priority, &varLeftPrms);
		}
		
		if(intRightSide > intLkFwd)
		{	
			arrRightSort.array = arrSortParams.array;
			arrRightSort.a = intLkFwd + 1;
			arrRightSort.c = intRightSide + 1;
			varRightPrms.interval = arrRightSort;
			varRightPrms.priority = varArgs->priority + 1;
			
			os_tsk_create_ex(quick_sort_task, varRightPrms.priority, &varRightPrms);
		}
		
		//destroy this task
		os_tsk_delete_self();
	}
	return;
}
/*
Responsible for initializing the vars with a scheduling method.
*/
void quicksort( array_t array ) {
	array_interval_t interval;
	qsort_task_parameters_t task_param;
	
	// Based on MTE 241 course notes--you can change this if you want
	//  - in the course notes, this sorts from a to c - 1
	interval.array =  array;
	interval.a     =  0;
	interval.c     =  array.length;
	
	task_param.interval = interval;

	// If you are using priorities, you can change this
	task_param.priority = 10;
	
	//start the quick_sort threading
	os_tsk_create_ex( quick_sort_task, task_param.priority, &task_param ); 
}


//responsible for implementing quicksort using semaphores.

__task void quick_sort_sem_task( void* void_ptr){
  // Your implementation here
	qsort_task_parameters_t *varArgs = (qsort_task_parameters_t *)void_ptr;
	qsort_task_parameters_t varLeftPrms, varRightPrms;
	array_interval_t arrSortParams = varArgs->interval;
	array_interval_t arrLeftSort, arrRightSort;
	int intLeftSide = arrSortParams.a;
	int intRightSide = arrSortParams.c - 1;
	int intMidLoc = (intLeftSide + intRightSide)/2;
	array_type intArrayVal, intPivotVal;
	int intLkFwd, intLkBck;
	
	if(intRightSide - intLeftSide < USE_INSERTION_SORT)
	{
		insertion_sort(arrSortParams);
		
		os_sem_wait(&sort_semaphore, 0xFFFF);
		{
			intSemCounter--;
		}
		os_sem_send(&sort_semaphore);
		
		os_tsk_delete_self();
		return;
	}
	else
	{
		//find pivot from values in the beginning, middle and end of the current array as a median-of-three
		intPivotVal = find_median(arrSortParams.array.array[intLeftSide], arrSortParams.array.array[intMidLoc], arrSortParams.array.array[intRightSide]);
		
		//insert the values that are not the pivot into the first and middle element of the array
		if(intPivotVal == arrSortParams.array.array[intLeftSide])
		{
			if(arrSortParams.array.array[intRightSide] > arrSortParams.array.array[intMidLoc])
			{
				arrSortParams.array.array[intLeftSide] = arrSortParams.array.array[intMidLoc];
				arrSortParams.array.array[intMidLoc] = arrSortParams.array.array[intRightSide];
			}
			else
			{
				arrSortParams.array.array[intLeftSide] = arrSortParams.array.array[intRightSide];
			}
		}
		else if(intPivotVal == arrSortParams.array.array[intMidLoc])
		{
			if(arrSortParams.array.array[intLeftSide] > arrSortParams.array.array[intRightSide])
			{
				arrSortParams.array.array[intMidLoc] = arrSortParams.array.array[intLeftSide];
				arrSortParams.array.array[intLeftSide] = arrSortParams.array.array[intRightSide];
			}
			else
			{
				arrSortParams.array.array[intMidLoc] = arrSortParams.array.array[intRightSide];
			}
		}
		else if(intPivotVal == arrSortParams.array.array[intRightSide])
		{
			if(arrSortParams.array.array[intLeftSide] > arrSortParams.array.array[intMidLoc])
			{
				intArrayVal = arrSortParams.array.array[intLeftSide];
				arrSortParams.array.array[intLeftSide] = arrSortParams.array.array[intMidLoc];
				arrSortParams.array.array[intMidLoc] = intArrayVal;
			}
		}
		
		//perform swapping to order array
		intLkFwd = intLeftSide;
		intLkBck = intRightSide - 1;
		
		while(intLkFwd <= intLkBck)
		{
			while(arrSortParams.array.array[intLkFwd] < intPivotVal)
			{
				intLkFwd++;
			}
			
			while(arrSortParams.array.array[intLkBck] > intPivotVal)
			{
				intLkBck--;
			}
			
			if(intLkFwd <= intLkBck)
			{
				intArrayVal = arrSortParams.array.array[intLkBck];
				arrSortParams.array.array[intLkBck] = arrSortParams.array.array[intLkFwd];
				arrSortParams.array.array[intLkFwd] = intArrayVal;
				intLkFwd++;
				intLkBck--;
			}
		}
		
		//place pivot at point of intersection where the value is
		//currently greater than the pivot, with the current value
		//being placed at the end of the array
		arrSortParams.array.array[intRightSide] = arrSortParams.array.array[intLkFwd];
		arrSortParams.array.array[intLkFwd] = intPivotVal;
		
		//set up the values to pass along to the child tasks
		//and create the child tasks
		if(intLeftSide < intLkBck)
		{
			arrLeftSort.array = arrSortParams.array;
			arrLeftSort.a = intLeftSide;
			arrLeftSort.c = intLkBck + 1;
			varLeftPrms.interval = arrLeftSort;
			varLeftPrms.priority = varArgs->priority;
			
			os_sem_wait(&sort_semaphore, 0xFFFF);
			{
				intSemCounter++;
			}
			os_sem_send(&sort_semaphore);
			
			os_tsk_create_ex(quick_sort_task, varLeftPrms.priority, &varLeftPrms);
		}
		
		if(intRightSide > intLkFwd)
		{	
			arrRightSort.array = arrSortParams.array;
			arrRightSort.a = intLkFwd + 1;
			arrRightSort.c = intRightSide + 1;
			varRightPrms.interval = arrRightSort;
			varRightPrms.priority = varArgs->priority;
			
			os_sem_wait(&sort_semaphore, 0xFFFF);
			{
				intSemCounter++;
			}
			os_sem_send(&sort_semaphore);
			
			os_tsk_create_ex(quick_sort_task, varRightPrms.priority, &varRightPrms);
		}
		
		//destroy this task
		os_sem_wait(&sort_semaphore, 0xFFFF);
		{
			intSemCounter--;
		}
		os_sem_send(&sort_semaphore);
		
		os_tsk_delete_self();
	}
	return;
}


//responsible for implementing quicksort using semaphores and calling
//its associated task
void quicksort_sem( array_t array ) {
	array_interval_t interval;
	qsort_task_parameters_t task_param;
	intSemCounter = 1;
	os_sem_init(&sort_semaphore, 1);
	
	// Based on MTE 241 course notes--you can change this if you want
	//  - in the course notes, this sorts from a to c - 1
	interval.array =  array;
	interval.a     =  0;
	interval.c     =  array.length;
	
	task_param.interval = interval;

	// If you are using priorities, you can change this
	task_param.priority = 3;
	
	//start the quick_sort threading
	os_tsk_create_ex( quick_sort_sem_task, task_param.priority, &task_param );
	
	while(1)
	{
		os_sem_wait(&sort_semaphore, 0xFFFF); 
		{
			if(intSemCounter == 0 && interval.c - interval.a <= USE_INSERTION_SORT)
			{
				os_sem_send(&sort_semaphore);
				break;
			}
			else if(intSemCounter == 2 && interval.c - interval.a > USE_INSERTION_SORT)
			{
				os_sem_send(&sort_semaphore);
				break;
			}
		}
		os_sem_send(&sort_semaphore);
	}
	return;
}

