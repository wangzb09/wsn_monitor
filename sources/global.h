#pragma once

#include"database.h"

#define MYDBG

#ifdef MYDBG
#define dbgprint(...) printf(__VA_ARGS__)
#else
#define dbgprint(...)
#endif  

#define BUFFLEN		4096

extern Database mydb;


