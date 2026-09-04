#include "linklisted.h"
#include <stdio.h>
#include <stdlib.h>

linklisted_t *linklisted_create(char *name, void (*function)(MenuCtx_t *),
                                linklisted_t *sublist) {
    linklisted_t *newNode = (linklisted_t *)malloc(sizeof(linklisted_t));
    if (newNode == NULL) {
        return NULL; // Memory allocation failed
    }
    newNode->name = name;
    newNode->function = function;
    newNode->next = NULL;
    newNode->prev = NULL;
    // link sublist and parentlist
    if (sublist == NULL) {
        newNode->sublist = NULL;
        newNode->parentlist = NULL;
        return newNode;
    }
    sublist->parentlist = newNode;
    newNode->sublist = sublist;
    return newNode;
}

linklisted_t *linklisted_add(linklisted_t *head, linklisted_t *node) {
    linklisted_t *newNode = node;
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
    if (head == NULL) {
        return head;
    }
    if (head->next == NULL) {
        return head;
    }

    linklisted_t *tail = head;
    int len = 1;
    while (tail->next != NULL) {
        tail = tail->next;
        len++;
    }

    if (step >= len) {
        return head;
    }
    if (step < 0) {
        step = len + step;
    }
    linklisted_t *pointer_node = head;
    for (int i = 1; i < step; i++) {
        pointer_node = pointer_node->next;
    }
    linklisted_t *new_head = pointer_node->next;

    pointer_node->next = NULL;
    new_head->prev = NULL;

    tail->next = head;
    head->prev = tail;

    return new_head;
}
