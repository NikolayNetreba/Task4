#pragma once

#define DEBUG

typedef int elem_t;
#define ELEM_PRINT(elem) fprintf(stderr, "%lg\n", (float)elem)
#define ELEM_POISON 666777 // Im filling data with this default value
