#include <iostream>
#include "vm_app.h"

using namespace std;

int main(){

	char* p[7];

	for(int i = 0; i < 7; i++){
		p[i] = (char *)vm_extend();
		p[i][0] = i;
		vm_syslog(p[i], 1);
	}
	p[1][0] = 'a';

	char* q = (char *)vm_extend();
	q[0] = 'b';
	for(int i = 0; i < 7; i++){
		vm_syslog(p[i], 1);
	}
}