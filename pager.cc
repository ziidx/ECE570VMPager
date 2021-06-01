#include <cstdlib>
#include <stdlib.h>
#include <queue>
#include <map>
#include <stdio.h>
#include <string.h>
#include <string>
#include <iostream>
#include "vm_pager.h"
#include <vector>
#include <new>

using namespace std;

struct page {
    unsigned int blockSize;
    pid_t processID;
    unsigned int residentbit;
    unsigned int dirtybit;
    unsigned int referencebit;
    unsigned int zerobit;
    unsigned int virtualPage;
} ;

struct process {
    pid_t id;
    page_table_t* pageTable;
    unsigned long int highestIndex;
    vector<page*> validPages;
} ;

vector<unsigned int> freeBlocks, freePages;
queue<page*> clockQueue;

unsigned int pageBaseVal;
map<pid_t, process*> processTable;

pid_t currPid;

void vm_init(unsigned int memory_pages, unsigned int disk_blocks) {
    for (unsigned int i = 0; i < disk_blocks; i++) {
        freeBlocks.push_back(i);
    }
    
    for (unsigned int i = 0; i < memory_pages; ++i) {
        freePages.push_back(i);
    }  
    pageBaseVal = ((unsigned long int)VM_ARENA_SIZE / (unsigned long int)VM_PAGESIZE) + 1;
}

void vm_create(pid_t pid) {

    unsigned int numPTE = (unsigned long int)VM_ARENA_SIZE / (unsigned long int)VM_PAGESIZE;
    
    process* p = new process;
    p->id = pid;
    p->pageTable = new page_table_t;
    page_table_entry_t* entry = new page_table_entry_t;
    
    for (unsigned int i = 0; i < numPTE; i++) {
        entry = &p->pageTable->ptes[i];
        entry->ppage = pageBaseVal;
        entry->read_enable = 0;
        entry->write_enable = 0;
    }
    p->highestIndex = (unsigned long int) VM_ARENA_BASEADDR;
    processTable.insert(pair<pid_t, process*>(p->id, p));
}

void vm_switch(pid_t pid) {
    currPid = pid;
    page_table_base_register = processTable[pid]->pageTable;
}

void* vm_extend() {
    if (freeBlocks.empty() || (processTable[currPid]->highestIndex + (unsigned long int)VM_PAGESIZE) > ((unsigned long int)VM_ARENA_BASEADDR + (unsigned long int)VM_ARENA_SIZE)) {
        return NULL;
    }

    unsigned int block = freeBlocks.front();
    freeBlocks.erase(freeBlocks.begin());
    
    unsigned long int lowestByte;
    unsigned long int arenaBase = (unsigned long int)VM_ARENA_BASEADDR;
    
    if (processTable[currPid]->highestIndex == arenaBase) {
        lowestByte = (unsigned long int)VM_ARENA_BASEADDR;
    }

    else {
        lowestByte = processTable[currPid]->highestIndex + 1;
    }

    page* p = new page;
    
    p->residentbit = 0;
    p->dirtybit = 0;
    p->referencebit = 0;
    p->zerobit = 0;
    p->processID = currPid;
    p->virtualPage = (unsigned int)((lowestByte - (unsigned long int)VM_ARENA_BASEADDR) / (unsigned long int)VM_PAGESIZE);
    p->blockSize = block;
    
    processTable[currPid]->highestIndex = lowestByte + (unsigned long int)VM_PAGESIZE - 1;
    processTable[currPid]->validPages.push_back(p);
    
    return (void*)(unsigned long int*)lowestByte;
}

int vm_fault(void *addr, bool write_flag) {

    unsigned long int hexVirtualAddress = (unsigned long int)addr;
    
    if ((unsigned long int)addr > processTable[currPid]->highestIndex || (unsigned long int)addr < (unsigned int long)VM_ARENA_BASEADDR) {
        return -1;

    }
    
    unsigned int pageTableIndex = (unsigned int)(((unsigned long int)addr - (unsigned long int)VM_ARENA_BASEADDR) / (unsigned long int)VM_PAGESIZE);
    
    page_table_entry_t* tableEntry = &processTable[currPid]->pageTable->ptes[pageTableIndex];

    if (processTable[currPid]->validPages[pageTableIndex]->residentbit == 0) {
        page* p = processTable[currPid]->validPages[pageTableIndex];
        if (freePages.empty()) {
            bool foundPageToEvict = false;
            page* clockPage;
            page_table_entry_t* auxTableEntry; 
            while (!foundPageToEvict) {                
                clockPage = clockQueue.front();
                clockQueue.pop();
                auxTableEntry = &processTable[clockPage->processID]->pageTable->ptes[clockPage->virtualPage];
                if (clockPage->referencebit == 1) {
                    auxTableEntry->read_enable = 0;
                    auxTableEntry->write_enable = 0;
                    clockPage->referencebit = 0;
                    clockQueue.push(clockPage);
                }
                else {
                    foundPageToEvict = true;
                }
            }
            if (clockPage->dirtybit == 1) {
                clockPage->dirtybit = 0;
                disk_write(clockPage->blockSize, auxTableEntry->ppage);   
            }
            tableEntry->ppage = auxTableEntry->ppage;
            auxTableEntry->ppage = pageBaseVal;
            clockPage->residentbit = 0;
            
            if (p->zerobit == 0) {           
                memset ((char*)pm_physmem + ((processTable[currPid]->pageTable->ptes[pageTableIndex].ppage) * (unsigned long int)(VM_PAGESIZE)), 0, (unsigned int long)VM_PAGESIZE);
            }
        
            else {
                disk_read(p->blockSize, processTable[currPid]->pageTable->ptes[pageTableIndex].ppage);
            }

            if (write_flag) {
                tableEntry->write_enable = 1;
                p->dirtybit = 1;
                p->zerobit = 1;
            }
            tableEntry->read_enable = 1;
            p->referencebit = 1;
            p->residentbit = 1;
        }
        
        else {
            tableEntry->ppage = freePages.front();
            freePages.erase(freePages.begin());
            
            if (p->zerobit == 0) {           
                memset ((char*)pm_physmem + ((processTable[currPid]->pageTable->ptes[pageTableIndex].ppage) * (unsigned long int)(VM_PAGESIZE)), 0, (unsigned int long)VM_PAGESIZE);
            }
        
            else {
                disk_read(p->blockSize, processTable[currPid]->pageTable->ptes[pageTableIndex].ppage);
            }

            if (write_flag) {
                tableEntry->write_enable = 1;
                p->dirtybit = 1;
                p->zerobit = 1;
            }
            tableEntry->read_enable = 1;
            p->referencebit = 1;
            p->residentbit = 1;
        }
        clockQueue.push(p);
    }
    else {   
        page* residentPage = processTable[currPid]->validPages[pageTableIndex];
        if (write_flag) {
            tableEntry->write_enable = 1;
            residentPage->dirtybit = 1;
            residentPage->zerobit = 1;
        }
        else{
            if (residentPage->dirtybit == 1) {
                tableEntry->write_enable = 1;
            }
        }
        tableEntry->read_enable = 1;
        residentPage->referencebit = 1;
    }
    return 0;
}

void vm_destroy() {
    page* p = new page;
    for (int i = 0; i < clockQueue.size(); i++) {
        p = clockQueue.front();
        clockQueue.pop();

        if (p->processID == currPid) {
            freePages.push_back(processTable[currPid]->pageTable->ptes[p->virtualPage].ppage);
        }
        else {
            clockQueue.push(p); 
        }
    }
    delete processTable[currPid]->pageTable;
    for (int i = 0; i < processTable[currPid]->validPages.size(); i++) {
        freeBlocks.push_back(processTable[currPid]->validPages[i]->blockSize);
        delete processTable[currPid]->validPages[i];
    }    
    delete processTable[currPid];
}

int vm_syslog(void *message, unsigned int len) {
    
    unsigned long int hexVirtualAddress = (unsigned long int)message;
    
    if (len <= 0 || (unsigned long int)message < (unsigned long int)VM_ARENA_BASEADDR || ((unsigned long int)message + len - 1) > processTable[currPid]->highestIndex) {  
        return -1;     
    }

    string s;

    for (unsigned int i = 0; i < len; i++) {
        unsigned int virtualPageNumber = ((unsigned long int)message - (unsigned long int)VM_ARENA_BASEADDR + i) / (unsigned long int)VM_PAGESIZE;
        unsigned int pageOffset = ((unsigned long int)message - (unsigned long int)VM_ARENA_BASEADDR + i) % (unsigned long int)VM_PAGESIZE;;

        if (processTable[currPid]->pageTable->ptes[virtualPageNumber].read_enable == 0) {
            if (vm_fault((void*) ((unsigned long int)message + i), false) == -1) {
                return -1;
            }
        }
        unsigned int pFrame = (unsigned int)(processTable[currPid]->pageTable->ptes[virtualPageNumber].ppage);
        s += ((char*)pm_physmem)[(pFrame * (unsigned int long)VM_PAGESIZE) + pageOffset];
    }
    cout << "syslog \t\t\t" << s << endl;
    return 0;
}

