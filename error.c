/*
    CS140 2324A PROJECT 2 - EXT2 FILESYSTEM READER
    Ivan Cassidy Cadiang (2021-12575)
    Diego Montenejo (2020-05984)
    Rohan Solas (2021-07292)
*/

// File contains functions for error catching

#include "defs.h"

bool isAbsolutePath(char* path) {
    return path[0] == '/';
}