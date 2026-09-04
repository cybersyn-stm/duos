#include "linklisted.h"
#include <stdio.h>
#include <stdlib.h>

linklisted_t *linkedlist_create(char *name, void (*function)(void)) {
    linklisted_t *newNode = (linklisted_t *)malloc(sizeof(linklisted_t));
    if (newNode == NULL) {
        return NULL; // Memory allocation failed
    }
    newNode->name = name;
    newNode->function = function;
    newNode->next = NULL;
    newNode->prev = NULL;
    return newNode;
}

linklisted_t *linklisted_add(linklisted_t *head, char *name,
                             void (*function)(void)) {
    linklisted_t *newNode = linkedlist_create(name, function);
    if (newNode == NULL) {
        return head; // Memory allocation failed, return original head
    }
    if (head == NULL) {
        return newNode; // List was empty, new node is now the head
    }

    linklisted_t *traversal = head;
    while (traversal->next != NULL) {
        traversal = traversal->next;
    }

    newNode->prev = traversal;
    traversal->next = newNode;
    newNode->next = NULL;
    return head;
}

linklisted_t *linklisted_slide(linklisted_t *head, int step) {
    linklisted_t *current = head;
    return current;
}
