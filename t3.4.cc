#include <iostream>
#include "vm_app.h"

using namespace std;

int main() {
        char* p = (char *) vm_extend();
        char* q = (char *) vm_extend();

        p[0] = 'h';
        p[1] = 'e';
        p[2] = 'l';
        p[3] = 'l';
        p[4] = 'o';

        q[0] = 'w';
        q[1] = 'o';
        q[2] = 'r';
        q[3] = 'l';
        q[4] = 'd';

        p[8187] = p[0];
        p[8188] = p[1];
        p[8189] = p[2];
        p[8190] = p[3];
        p[8191] = p[4];


        vm_syslog(p, 4);
        vm_syslog(p, 40);
        vm_syslog(p, 400);
        vm_syslog(p, 4000);
        vm_syslog(p, 40000);

        vm_syslog(q, 4);
        vm_syslog(q, 40);
        vm_syslog(q, 400);
        vm_syslog(q, 4000);
        vm_syslog(q, 40000);

        vm_syslog(p + 8187, 10);
        vm_syslog(p + 8187, 20);
        vm_syslog(q + 8190, 10);
        vm_syslog(q + 8180, 20);
}