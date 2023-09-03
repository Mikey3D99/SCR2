//
// Created by wlodi on 16.05.2023.
//

#include <malloc.h>
#include "priority_queue.h"

// Function to create an empty priority queue
PriorityQueue* create_priority_queue() {
    PriorityQueue* pq = malloc(sizeof(PriorityQueue));
    pq->size = 0;
    return pq;
}

// Function to add a task to the queue
void add_task(PriorityQueue* pq, Task* task) {
    if (pq->size == MAX_TASKS) {
        fprintf(stderr, "Error: queue is full\n");
        return;
    }

    // If the task type is RELATIVE, adjust exec_time to an absolute timestamp
    if (task->type == RELATIVE) {
        time_t current_time;
        time(&current_time);
        task->exec_time += current_time;
    }

    // Add the task at the end of the array
    pq->tasks[pq->size] = task;
    pq->size++;

    // Heapify up
    int child_index = pq->size - 1;
    int parent_index = (child_index - 1) / 2;
    while (child_index > 0 && pq->tasks[parent_index]->exec_time > pq->tasks[child_index]->exec_time) {
        // Swap parent and child
        Task* temp = pq->tasks[parent_index];
        pq->tasks[parent_index] = pq->tasks[child_index];
        pq->tasks[child_index] = temp;

        // Update child and parent indices
        child_index = parent_index;
        parent_index = (child_index - 1) / 2;
    }
}


//helper function used when retrieving a task
void sift_down(PriorityQueue* pq, int start) {
    int parent_index = start;
    int left_child_index = 2 * parent_index + 1;
    int right_child_index = 2 * parent_index + 2;

    // Find the smallest of the parent and the children
    int smallest_index = parent_index;
    if (left_child_index < pq->size && pq->tasks[left_child_index]->exec_time < pq->tasks[smallest_index]->exec_time) {
        smallest_index = left_child_index;
    }
    if (right_child_index < pq->size && pq->tasks[right_child_index]->exec_time < pq->tasks[smallest_index]->exec_time) {
        smallest_index = right_child_index;
    }

    // If a child is smaller than the parent, swap them and sift down from the child
    if (smallest_index != parent_index) {
        Task* temp = pq->tasks[parent_index];
        pq->tasks[parent_index] = pq->tasks[smallest_index];
        pq->tasks[smallest_index] = temp;
        sift_down(pq, smallest_index);
    }
}


Task* get_next_task(PriorityQueue* pq) {
    if (pq->size == 0) {
        fprintf(stderr, "Error: queue is empty\n");
        return NULL;
    }

    Task* task = pq->tasks[0];
    pq->tasks[0] = pq->tasks[pq->size - 1];
    pq->size--;


    // Reheapify the queue
    if (pq->size > 0) {
        sift_down(pq, 0);
    }

    return task;
}

Task* peek_next_task(PriorityQueue* pq) {
    if (pq->size == 0) {
        return NULL;
    }
    return pq->tasks[0];
}


Task * find_task_by_id(PriorityQueue* pq, int task_id){
    for (int i = 0; i < pq->size; i++) {
        if(pq->tasks[i]->id == task_id){
            return pq->tasks[i];
        }
    }
    return NULL;
}

void delete_task(PriorityQueue* pq, int task_id) {

    // Find the task to delete
    int i;
    for(i = 0; i < pq->size; i++) {
        if(pq->tasks[i]->id == task_id) {
            break;
        }
    }

    // If the task was not found, just return
    if(i == pq->size) {
        return;
    }

    // Free the task memory
    free(pq->tasks[i]);

    // Swap the task with the last task in the array
    pq->tasks[i] = pq->tasks[pq->size - 1];
    pq->size--;

    // Heapify from the current position
    sift_down(pq, i);
}


char* tasks_to_string(PriorityQueue* pq) {
    // Assume MAX_TASKS is the maximum number of tasks
    // Assume TASK_STRING_SIZE is the maximum size of a task's string representation
    char* result = malloc(MAX_TASKS * TASK_STRING_SIZE * sizeof(char));

    if (result == NULL) {
        perror("Malloc failed");
        return NULL;
    }

    result[0] = '\0'; // Start with an empty string

    for (int i = 0; i < pq->size; i++) {
        char task_str[TASK_STRING_SIZE];
        task_to_string(pq->tasks[i], task_str);

        // Concatenate the task string to the result string
        strcat(result, task_str);
        strcat(result, "\n"); // Add a newline between tasks
    }

    return result;
}


void free_priority_queue(PriorityQueue* pq) {
    for (int i = 0; i < pq->size; i++) {
        free_task(pq->tasks[i]);
    }
    free(pq);
}