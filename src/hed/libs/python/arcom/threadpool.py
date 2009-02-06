import threading, traceback

from time import sleep

# Slightly modified from http://code.activestate.com/recipes/203871/

class ThreadPool:

    """Flexible thread pool class.  Creates a pool of threads, then
    accepts tasks that will be dispatched to the next available
    thread."""
    
    def __init__(self, numThreads):

        """Initialize the thread pool with numThreads workers."""
        
        self.__threads = []
        self.__resizeLock = threading.Condition(threading.Lock())
        self.__taskLock = threading.Condition(threading.Lock())
        self.__tasks = []
        self.__isJoining = False
        self.setThreadCount(numThreads)

    def setThreadCount(self, newNumThreads):

        """ External method to set the current pool size.  Acquires
        the resizing lock, then calls the internal version to do real
        work."""
        
        # Can't change the thread count if we're shutting down the pool!
        if self.__isJoining:
            return False
        
        self.__resizeLock.acquire()
        try:
            self.__setThreadCountNolock(newNumThreads)
        finally:
            self.__resizeLock.release()
        return True

    def __setThreadCountNolock(self, newNumThreads):
        
        """Set the current pool size, spawning or terminating threads
        if necessary.  Internal use only; assumes the resizing lock is
        held."""
        
        # If we need to grow the pool, do so
        while newNumThreads > len(self.__threads):
            newThread = ThreadPoolThread(self)
            self.__threads.append(newThread)
            newThread.start()
        # If we need to shrink the pool, do so
        while newNumThreads < len(self.__threads):
            self.__threads[0].goAway()
            del self.__threads[0]

    def getThreadCount(self):

        """Return the number of threads in the pool."""
        
        self.__resizeLock.acquire()
        try:
            return len(self.__threads)
        finally:
            self.__resizeLock.release()

    def queueTask(self, task, args=None, taskCallback=None):

        """Insert a task into the queue.  task must be callable;
        args and taskCallback can be None."""
        
        if self.__isJoining == True:
            return False
        if not callable(task):
            return False
        
        self.__taskLock.acquire()
        try:
            self.__tasks.append((task, args, taskCallback))
            return True
        finally:
            self.__taskLock.release()

    def getNextTask(self):

        """ Retrieve the next task from the task queue.  For use
        only by ThreadPoolThread objects contained in the pool."""
        
        self.__taskLock.acquire()
        try:
            if self.__tasks == []:
                return (None, None, None)
            else:
                return self.__tasks.pop(0)
        finally:
            self.__taskLock.release()
    
#    def joinAll(self, waitForTasks = True, waitForThreads = True):
#
#        """ Clear the task queue and terminate all pooled threads,
#        optionally allowing the tasks and threads to finish."""
#        
#        # Mark the pool as joining to prevent any more task queueing
#        self.__isJoining = True
#
#        # Wait for tasks to finish
#        if waitForTasks:
#            while self.__tasks != []:
#                sleep(.1)
#        # Tell all the threads to quit
#        self.__resizeLock.acquire()
#        try:
#            self.__setThreadCountNolock(0)
#            self.__isJoining = True
#
#            # Wait until all threads have exited
#            if waitForThreads:
#                for t in self.__threads:
#                    t.join()
#                    del t
#                # actually wait for threads to end
#                while threading.activeCount()>1:
#                    sleep(0.1)
#            # Reset the pool for potential reuse
#            self.__isJoining = False
#        finally:  
#            self.__resizeLock.release()

    def joinAll(self, waitForTasks = True, waitForThreads = True):

        """ Clear the task queue and terminate all pooled threads,
        optionally allowing the tasks and threads to finish."""

        # Mark the pool as joining to prevent any more task queueing
        self.__isJoining = True

        # Wait for tasks to finish
        if waitForTasks:
            while self.__tasks != []:
                sleep(0.1)

        # Tell all the threads to quit
        self.__resizeLock.acquire()
        try:
            # Wait until all threads have exited
            if waitForThreads:
                for t in self.__threads:
                    t.goAway()
                for t in self.__threads:
                    t.join()
                    # print t,"joined"
                    del t
            self.__setThreadCountNolock(0)
            self.__isJoining = True

            # Reset the pool for potential reuse
            self.__isJoining = False
        finally:
            self.__resizeLock.release()
        
class ThreadPoolThread(threading.Thread):

    """ Pooled thread class. """
    
    threadSleepTime = 0.1

    def __init__(self, pool):

        """ Initialize the thread and remember the pool. """
        
        threading.Thread.__init__(self)
        self.__pool = pool
        self.__isDying = False
        
    def run(self):

        """ Until told to quit, retrieve the next task and execute
        it, calling the callback if any.  """
        
        while self.__isDying == False:
            cmd, args, callback = self.__pool.getNextTask()
            # If there's nothing to do, just sleep a bit
            if cmd is None:
                sleep(ThreadPoolThread.threadSleepTime)
            elif callback is None:
                cmd(args)
            else:
                callback(cmd(args))
    
    def goAway(self):

        """ Exit the run loop next time through."""
        
        self.__isDying = True

            
# from http://my.safaribooksonline.com/0596001673/pythoncook-CHP-6-SECT-4
class ReadWriteLock:
    """ A lock object that allows many simultaneous "read locks", but
    only one "write lock." """

    def __init__(self):
        self._read_ready = threading.Condition(threading.Lock())
        self._readers = 0

    def acquire_read(self):
        """ Acquire a read lock. Blocks only if a thread has
        acquired the write lock. """
        self._read_ready.acquire()
        try:
            self._readers += 1
        finally:
            self._read_ready.release()

    def release_read(self):
        """ Release a read lock. """
        self._read_ready.acquire()
        try:
            self._readers -= 1
            if not self._readers:
                self._read_ready.notifyAll()
        finally:
            self._read_ready.release()

    def acquire_write(self):
        """ Acquire a write lock. Blocks until there are no
        acquired read or write locks. """
        self._read_ready.acquire()
        while self._readers > 0:
            self._read_ready.wait()

    def release_write(self):    
        """ Release a write lock. """
        self._read_ready.release()

COUNTER = 0

# Usage example
if __name__ == "__main__":

    from random import randrange

    # Sample task 1: given a start and end value, shuffle integers,
    # then sort them
    
    def sortTask(data):
        print "SortTask starting for ", data
        numbers = range(data[0], data[1])
        for a in numbers:
            rnd = randrange(0, len(numbers) - 1)
            a, numbers[rnd] = numbers[rnd], a
        print "SortTask sorting for ", data
        numbers.sort()
        print "SortTask done for ", data
        return "Sorter ", data

    # Sample task 2: just sleep for a number of seconds.

    def waitTask(data):
        print "WaitTask starting for ", data
        print "WaitTask sleeping for %d seconds" % data
        sleep(data)
        return "Waiter", data

    def addTask((data, locker)):
        global COUNTER
        nadds = 10000
        for i in range(nadds):
            # re-acquiring lock for every add
            # don't do this at home...
            locker.acquire_write()
            COUNTER += data
            locker.release_write()
        print "Added %d to counter, counter is now %d"%(data*nadds,COUNTER)
        return "addTask", data
    
    # Both tasks use the same callback

    def taskCallback(data):
        print "Callback called for", data
    # Create a pool with three worker threads

    pool = ThreadPool(3)

    # Insert tasks into the queue and let them run
#    pool.queueTask(sortTask, (1000, 100000), taskCallback)
#    pool.queueTask(waitTask, 5, taskCallback)
#    pool.queueTask(sortTask, (200, 200000), taskCallback)
#    pool.queueTask(waitTask, 2, taskCallback)
#    pool.queueTask(sortTask, (3, 30000), taskCallback)
#    pool.queueTask(waitTask, 7, taskCallback)

    pool.joinAll()

    pool = ThreadPool(3)

    locker = ReadWriteLock()
    pool.queueTask(addTask, (10, locker), taskCallback)
    pool.queueTask(addTask, (10, locker), taskCallback)
    pool.queueTask(addTask, (10, locker), taskCallback)
    pool.queueTask(addTask, (10, locker), taskCallback)
    
    print "before join", COUNTER
    # When all tasks are finished, allow the threads to terminate
    pool.joinAll()
    print "after join", COUNTER
