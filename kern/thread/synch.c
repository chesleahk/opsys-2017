/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Synchronization primitives.
 * The specifications of the functions are in synch.h.
 */

#include <types.h>
#include <lib.h>
#include <spinlock.h>
#include <wchan.h>
#include <thread.h>
#include <current.h>
#include <synch.h>

////////////////////////////////////////////////////////////
//
// Semaphore.

struct semaphore *
sem_create(const char *name, unsigned initial_count)
{
        struct semaphore *sem;

        sem = kmalloc(sizeof(*sem));
        if (sem == NULL) {
                return NULL;
        }

        sem->sem_name = kstrdup(name);
        if (sem->sem_name == NULL) {
                kfree(sem);
                return NULL;
        }

	sem->sem_wchan = wchan_create(sem->sem_name);
	if (sem->sem_wchan == NULL) {
		kfree(sem->sem_name);
		kfree(sem);
		return NULL;
	}

	spinlock_init(&sem->sem_lock);
        sem->sem_count = initial_count;

        return sem;
}

void
sem_destroy(struct semaphore *sem)
{
        KASSERT(sem != NULL);

	/* wchan_cleanup will assert if anyone's waiting on it */
	spinlock_cleanup(&sem->sem_lock);
	wchan_destroy(sem->sem_wchan);
        kfree(sem->sem_name);
        kfree(sem);
}

void
P(struct semaphore *sem)
{
        KASSERT(sem != NULL);

        /*
         * May not block in an interrupt handler.
         *
         * For robustness, always check, even if we can actually
         * complete the P without blocking.
         */
        KASSERT(curthread->t_in_interrupt == false);

	/* Use the semaphore spinlock to protect the wchan as well. */
	spinlock_acquire(&sem->sem_lock);
        while (sem->sem_count == 0) {
		/*
		 *
		 * Note that we don't maintain strict FIFO ordering of
		 * threads going through the semaphore; that is, we
		 * might "get" it on the first try even if other
		 * threads are waiting. Apparently according to some
		 * textbooks semaphores must for some reason have
		 * strict ordering. Too bad. :-)
		 *
		 * Exercise: how would you implement strict FIFO
		 * ordering?
		 */

		wchan_sleep(sem->sem_wchan, &sem->sem_lock);// left
        }
        KASSERT(sem->sem_count > 0);
        sem->sem_count--;
	spinlock_release(&sem->sem_lock);
}

void
V(struct semaphore *sem)
{
        KASSERT(sem != NULL);

	spinlock_acquire(&sem->sem_lock);

        sem->sem_count++;
        KASSERT(sem->sem_count > 0);
	wchan_wakeone(sem->sem_wchan, &sem->sem_lock);

	spinlock_release(&sem->sem_lock);
}

////////////////////////////////////////////////////////////
//
// Lock.

struct lock *
lock_create(const char *name)
{
        struct lock *lock;

        lock = kmalloc(sizeof(*lock));
        if (lock == NULL) {
                return NULL;
        }

        lock->lk_name = kstrdup(name);
        if (lock->lk_name == NULL) {
                kfree(lock);
                return NULL;
        }

        // add stuff here as needed
	
	lock->lk_wchan = wchan_create(lock->lk_name); 
	
	if(lock->lk_wchan == NULL)
	{
		kfree(lock->lk_name); 
		kfree(lock); 
	}
	
	lock->is_locked = false; 
	lock->lk_owner = NULL; 
	spinlock_init(&lock->lk_spinlock); 
	// added up to here 
        return lock;
}

void
lock_destroy(struct lock *lock)
{
        KASSERT(lock != NULL);

        // add stuff here as needed
	// added below

	spinlock_cleanup(&lock->lk_spinlock); 
	wchan_destroy(lock->lk_wchan); 	

	//kfree(lock->lk_owner); // added 
        kfree(lock->lk_name);
        kfree(lock);
}

void
lock_acquire(struct lock *lock)
{
        /* 
		The main goal is to set the flag if no one has the lock!! 
		Steps:
		1. Acquire the spinlock for the lock before accessing the member fields 
			spinlock_acquire(&lock->lk_spinlock)???
		2. Put the thread to sleep if the lock is not available
			wchan_sleep internally releases the spinlock
			Loop over this because other threads can be competing
			so:
			if(lock is not available){
			wchan_sleep(lock->lk_wchan); }
		3. Set the lock when exiting the loop 
			lock->is_locked= true; 
			lock->lk_owner = curthread; 
		4. Release the spinlock. 
			spinlock_release(&lock->lk_spinlock); 
			

	*/


	KASSERT(lock != NULL); 
	KASSERT(curthread->t_in_interrupt == false); 
	spinlock_acquire(&lock->lk_spinlock); 
	//KASSERT(lock->lk_owner == curthread); // THIS IS THE CAUSE OF ALL MY PROBLEMS. I HATE THIS SINGLE LINE IT IS THE WORST LINE IN MY ENTIRE CODE OMG SPENT HOURS LOOKING FOR THE REASON MY CODE DIDNT WORK. OMG 


//	if(lock_do_i_hold(lock)) //  current thread doesnt have
  //      {
    //            spinlock_release(&lock->lk_spinlock);  // want the CPU to rele$
     //           return;  // return to caller
      //  }
//	else {

	while(lock->is_locked)
	{
		wchan_sleep(lock->lk_wchan, &lock->lk_spinlock);
	}

		KASSERT(!lock->is_locked); // panic if the lock isnt locked!!
		lock->is_locked = true; 
		lock->lk_owner = curthread; 
		KASSERT(lock->is_locked); 
		KASSERT(lock->lk_owner == curthread); 
	spinlock_release(&lock->lk_spinlock); // forces the CPU to release the lock because its done with the resource  

//        (void)lock;  // suppress warning until code gets written
}

void
lock_release(struct lock *lock)
{
        KASSERT(lock != NULL); 
	spinlock_acquire(&lock->lk_spinlock);  // hold so no interferences
//	KASSERT(lock != NULL);
	KASSERT(lock->lk_owner == curthread);  
//	spinlock_acquire(&lock->lk_spinlock);  // hold so no interferences
	// also to wake the thread holding the lock
	
	
	lock->is_locked = false;
	lock->lk_owner = NULL;  
	wchan_wakeone(lock->lk_wchan, &lock->lk_spinlock); 
	 // if locks owner is the current thread
	 // set the owner to null , 
	// wakes the thread of the current lock 
	
	spinlock_release(&lock->lk_spinlock); // done with the resource 
       
// (void)lock;  // suppress warning until code gets written
}

bool
lock_do_i_hold(struct lock *lock)
{
	// bool  true if thread is the current owner of the lock 
	// means i need to add to the struct lock an owner variable  
	KASSERT(lock != NULL); 

	spinlock_acquire(&lock->lk_spinlock); 
	if(lock->lk_owner == curthread)
	{
		spinlock_release(&lock->lk_spinlock); 
		return true;
	} 
	spinlock_release(&lock->lk_spinlock); 
	 return false;

	// this operation will not work !!!! SEND HELP 
	
//	  (void)lock;  // suppress warning until code gets written

  //     return true; // dummy until code gets written
}


////////////////////////////////////////////////////////////
//
// CV


struct cv *
cv_create(const char *name)
{
        struct cv *cv;

        cv = kmalloc(sizeof(*cv));
        if (cv == NULL) {
                return NULL;
        }

        cv->cv_name = kstrdup(name);
        if (cv->cv_name==NULL) {
                kfree(cv);
                return NULL;
        }

        // add stuff here as needed

	cv->cv_wchan = wchan_create(cv->cv_name); 
	if(cv->cv_wchan == NULL)
	{
		kfree(cv->cv_name); 
		kfree(cv); 
		return NULL; 
	}
	spinlock_init(&cv->cv_spinlock); 
	// added up to here 
        return cv;
}

void
cv_destroy(struct cv *cv)
{
        KASSERT(cv != NULL);

        // add stuff here as needed
	
	spinlock_cleanup(&cv->cv_spinlock); // added too 
	wchan_destroy(cv->cv_wchan); // added 
        kfree(cv->cv_name);
        kfree(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
        /*
		From Appendix: CV has to be passed in as a parameter bc C do not have implicit parent objects. 
		Goal: Atomically (without interruptions) put the thread to sleep and release the lock. 
			AND ACQUIRE THE LOCK BACK WHEN AWOKEN!!!

		Steps:
		1. Acquire the spinlock for the CV first to ensure atomicity for the following steps:
			spinlock_acquire(&cv->cv_spinlock)??
		2. Release the lock
		3. Put the thread to sleep in the CV's wchan
		4. Release the Spinlock
		5. Acquire the lock before completing the function. 
			wchan_sleep for a similar manipulation. 
	*/

	KASSERT(cv != NULL);
	KASSERT(curthread->t_in_interrupt == false); 
	 //KASSERT(curthread->t_in_interrupt == false);
	//KASSERT(lock->lk_owner == curthread);
	if(lock_do_i_hold(lock))
	{
		spinlock_acquire(&cv->cv_spinlock); 
		lock_release(lock); 
		wchan_sleep(cv->cv_wchan, &cv->cv_spinlock); 
		spinlock_release(&cv->cv_spinlock); 
		lock_acquire(lock); 
	}

        //(void)cv;    // suppress warning until code gets written
        //(void)lock;  // suppress warning until code gets written
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
        // Write this
	

	KASSERT(cv != NULL);
	KASSERT(lock != NULL);
 	
	spinlock_acquire(&cv->cv_spinlock); 
	//KASSERT(lock->lk_owner == curthread); 
	
	if(lock_do_i_hold(lock))
		wchan_wakeone(cv->cv_wchan, &cv->cv_spinlock);
	 
//	lock_acquire(lock); 
	spinlock_release(&cv->cv_spinlock); 

	//(void)cv;    // suppress warning until code gets written
//	(void)lock;  // suppress warning until code gets written
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	// Write this

	KASSERT(cv != NULL);
	KASSERT(lock != NULL); 
	spinlock_acquire(&cv->cv_spinlock); 
	//KASSERT(lock->lk_owner == curthread);
	if(lock_do_i_hold(lock))
	{
		wchan_wakeall(cv->cv_wchan, &cv->cv_spinlock);
	}
	spinlock_release(&cv->cv_spinlock); 
//	lock_acquire(lock);  
	

//	(void)cv;    // suppress warning until code gets written
//	(void)lock;  // suppress warning until code gets written
}
