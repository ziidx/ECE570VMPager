#include <iostream>
#include "vm_app.h"
using namespace std;

int main()
{
    char *p;
    p = (char *) vm_extend();
    p[0] = 'a';
    p[1] = 'b';
    p[2] = 'c';
    p[3] = 'd';
    p[4] = 'e';
    p[5] = 'f';
    p[6] = 'g';
    p[7] = p[6];
    p[8] = p [5];
    p[9] = p [4];
    p[10] = p [3];
    p[11] = p [2];
    p[12] = p [1];
    p[13] = p [0];

    vm_syslog(p, 0);
    vm_syslog(p, 13);
    vm_syslog(p, 9000);
    p[9000] = p[0];
    vm_syslog(p, 9000);
}