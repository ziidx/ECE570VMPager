#include <iostream>
#include "vm_app.h"

using namespace std;

int main(){
	char* p;

    for(int i=0;i< 2048;i++){
        p = (char*) vm_extend();
    }

    cout << "test extend and destroy" << endl;
}