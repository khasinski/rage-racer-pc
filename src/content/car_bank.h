#ifndef CAR_BANK_H
#define CAR_BANK_H
#include <stddef.h>
/* Compare the serialized geometry block, ignoring images/other track blocks. */
int CarBankGeometryEqual(const void *left,size_t leftSize,const void *right,size_t rightSize,int rival);
#endif
