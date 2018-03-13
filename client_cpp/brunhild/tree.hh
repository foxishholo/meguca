#pragma once
#include "view.hh"

namespace brunhild {
// Initialize virtual DOM
void init();

// Generate a new unique Element ID
unsigned long new_id();

// Function to run before checking for DOM updates
extern void (*before_flush)();

// Function to run after checking for DOM updates
extern void (*after_flush)();

// Set the contents of the <body> tag
void set_body_children(Children);
}
