// scheduler.cc 
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would 
//	end up calling FindNextToRun(), and that would put us in an 
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "debug.h"
#include "scheduler.h"
#include "main.h"


int SRTNCompare(Thread *a, Thread *b) {
    if(a->getBurstTime() == b->getBurstTime())
        return 0;
    return a->getBurstTime() > b->getBurstTime() ? 1 : -1;
}
int PriorityCompare(Thread *a, Thread *b) {
    if(a->getPriority() == b->getPriority())
        return 0;
    return a->getPriority() > b->getPriority() ? -1 : 1;
}

//----------------------------------------------------------------------
//Scheduler::Aging
//	check the thread if it needs add priority
//----------------------------------------------------------------------
void
Scheduler::Aging()
{
    ListIterator<Thread *> *iter1 = new ListIterator<Thread *>(L1);
    ListIterator<Thread *> *iter2 = new ListIterator<Thread *>(L2);
    ListIterator<Thread *> *iter3 = new ListIterator<Thread *>(L3);

    while(!iter1->IsDone()){ 
        Thread* t = iter1->Item();
        L1->Remove(t);
        bool aging = this->CheckAge(t); //actually it always return false because L1 is the highest level
        if(!aging) L1->Insert(t);
        iter1->Next();
    }

    while(!iter2->IsDone()){                                                                                                    Thread* t = iter2->Item();
        L2->Remove(t);                                                                                       
	bool aging = this->CheckAge(t);                                          
	if(!aging) L2->Insert(t);                                                                            
	iter2->Next();                                                                                                      }
    //because  list3 does not use sortedList so do not remove the thread
    while(!iter3->IsDone()){                                                          
        Thread* t = iter3->Item();       
        this->CheckAge(t);
        iter3->Next(); 
    }	

}

bool
Scheduler::CheckAge(Thread *thread)
{ DEBUG(z,"checkage\n\n");
    //check thread in ready queue and wait for more than 1500 ticks
    int now = kernel->stats->totalTicks;
    int wait = now - thread->getReady();
    if( thread->getStatus() != READY ) return FALSE;
    DEBUG(z,wait<<"\n\n");
    if( wait < 1000 ) return FALSE;
    
    //update priority
    int oldPriority = thread->getPriority();
    int newPriority = oldPriority + 10;
    if( newPriority > 149) newPriority = 149;
    thread->setPriority(newPriority);
    if( newPriority != oldPriority){
	DEBUG(z,"Tick "<<now<<": Thread "<<thread->getID()<<" changes its priority from "<<oldPriority<<" to "<<newPriority);
    }
    
    //update queue list #L2->L1
    if( newPriority >= 100 && oldPriority < 100 ){
	   
	    L1->Insert(thread);
            DEBUG(z,"Tick "<<now<<": Thread "<<thread->getID()<<" is inserted into queue L1");
	    DEBUG(z,"Tick "<<now<<": Thread "<<thread->getID()<<" is removed from queue L2");	    
	
	thread->setReady(now);
	//in L1 need to check preempt
	if(kernel->currentThread->getPriority() >= 100 && (kernel->currentThread->getID()!=thread->getID())){ 
            double running = kernel->stats->userTicks - kernel->currentThread->getStart() + kernel->currentThread->bigT;            double predict = 0.5 * running + 0.5 * kernel->currentThread->getBurstTime();
            if(predict < thread->getBurstTime()) kernel->currentThread->Yield();
        }
        return TRUE;
    }else if( newPriority >= 50 && oldPriority < 50 ){ /* update queue list #L3->L2  */
            L3->Remove(thread);
            L2->Insert(thread);
            DEBUG(z,"Tick "<<now<<": Thread "<<thread->getID()<<" is inserted into queue L2");
            DEBUG(z,"Tick "<<now<<": Thread "<<thread->getID()<<" is removed from queue L1");
        
        thread->setReady(now);
    }
    return FALSE;
}

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads.
//	Initially, no ready threads.
//----------------------------------------------------------------------

Scheduler::Scheduler()
{	  
    L1 = new SortedList<Thread *>(SRTNCompare); 
    L2 = new SortedList<Thread *>(PriorityCompare);
    L3 = new List<Thread *>;
   // readyList = new List<Thread *>; 
    toBeDestroyed = NULL;
} 

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler()
{ 
    delete L1;
    delete L2;
    delete L3; 
} 

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void
Scheduler::ReadyToRun (Thread *thread)
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    DEBUG(dbgThread, "Putting thread on ready list: " << thread->getName());
	//cout << "Putting thread on ready list: " << thread->getName() << endl ;
    thread->setStatus(READY);
    
    //mp3
    int now = kernel->stats->totalTicks;
    if(thread->getPriority()<=49){
        DEBUG(z,"Tick " << now << ": Thread " << thread->getID() << " is inserted into queue L3");
	L3->Append(thread);
    }else if(thread->getPriority()<=99 && thread->getPriority()>=50){
	DEBUG(z,"Tick " << now << ": Thread " << thread->getID() << " is inserted into queue L2");
	L2->Insert(thread);
    }else{
	DEBUG(z,"Tick " << now << ": Thread " << thread->getID() << " is inserted into queue L1");
	L1->Insert(thread);
    }

    thread->setReady(now);
    //preempt
    if(thread->getPriority() >= 100 && kernel->currentThread->getPriority() >= 100)
    { 
	if(kernel->currentThread->getID()!=thread->getID()){  
	    double running = kernel->stats->userTicks - kernel->currentThread->getStart() + kernel->currentThread->bigT;
	    double predict = 0.5 * running + 0.5 * kernel->currentThread->getBurstTime();
            if(predict < thread->getBurstTime()) kernel->currentThread->Yield();
	}
    }                                                                       
}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

Thread *
Scheduler::FindNextToRun ()
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    
    //mp3
    int now = kernel->stats->totalTicks;    
    if (L1->IsEmpty() && L2->IsEmpty() && L3->IsEmpty()) {
		return NULL;
    } else if(!L1->IsEmpty()) {
       // if(kernel->currentThread->getBurstTime() > L2->Front()->getBurstTime() ) return kernel->currentThread;
        DEBUG(z,"Tick " << now << ": Thread " << L1->Front()->getID() << " is removed from queue L1");
    	return L1->RemoveFront();
    }else if(L1->IsEmpty() && !L2->IsEmpty()) {
       // if(kernel->currentThread->getPriority() > L2->Front()->getPriority() ) return kernel->currentThread;
	DEBUG(z,"Tick " << now << ": Thread " << L2->Front()->getID() << " is removed from queue L2");   
        return L2->RemoveFront();
    }else if(L1->IsEmpty() && L2->IsEmpty() && !L3->IsEmpty()) {
	DEBUG(z,"Tick " << now << ": Thread " << L3->Front()->getID() << " is removed from queue L3");   
	return L3->RemoveFront();
   }
}

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable kernel->currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//	"finishing" is set if the current thread is to be deleted
//		once we're no longer running on its stack
//		(when the next thread starts running)
//----------------------------------------------------------------------

void
Scheduler::Run (Thread *nextThread, bool finishing)
{
    Thread *oldThread = kernel->currentThread;
    
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    
    //mp3
    int now = kernel->stats->totalTicks;
    int program = kernel->stats->userTicks;
    int oldRun =  program - oldThread->getStart();
    nextThread->setStart(program);
    DEBUG(z,"Tick " << now << ": Thread " << nextThread->getID() <<" is now selected for execution, thread " << oldThread->getID() <<" is replaced, and it has executed " << oldRun << " ticks");
    
    
    if (finishing) {	// mark that we need to delete current thread
         ASSERT(toBeDestroyed == NULL);
	 toBeDestroyed = oldThread;
    }
    
    if (oldThread->space != NULL) {	// if this thread is a user program,
        oldThread->SaveUserState(); 	// save the user's CPU registers
	oldThread->space->SaveState();
    }
    
    oldThread->CheckOverflow();		    // check if the old thread
					    // had an undetected stack overflow

    kernel->currentThread = nextThread;  // switch to the next thread
    nextThread->setStatus(RUNNING);      // nextThread is now running
    
    DEBUG(dbgThread, "Switching from: " << oldThread->getName() << " to: " << nextThread->getName());
    
    // This is a machine-dependent assembly language routine defined 
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".

    SWITCH(oldThread, nextThread);

    // we're back, running oldThread
      
    // interrupts are off when we return from switch!
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    DEBUG(dbgThread, "Now in thread: " << oldThread->getName());

    CheckToBeDestroyed();		// check if thread we were running
					// before this one has finished
					// and needs to be cleaned up
    
    if (oldThread->space != NULL) {	    // if there is an address space
        oldThread->RestoreUserState();     // to restore, do it.
	oldThread->space->RestoreState();
    }
}

//----------------------------------------------------------------------
// Scheduler::CheckToBeDestroyed
// 	If the old thread gave up the processor because it was finishing,
// 	we need to delete its carcass.  Note we cannot delete the thread
// 	before now (for example, in Thread::Finish()), because up to this
// 	point, we were still running on the old thread's stack!
//----------------------------------------------------------------------

void
Scheduler::CheckToBeDestroyed()
{
    if (toBeDestroyed != NULL) {
        delete toBeDestroyed;
	toBeDestroyed = NULL;
    }
}
 
//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void
Scheduler::Print()
{
    cout << "Ready list contents:\n";
   // readyList->Apply(ThreadPrint);
}
