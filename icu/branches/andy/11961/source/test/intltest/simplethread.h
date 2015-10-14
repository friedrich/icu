/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2015, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#ifndef SIMPLETHREAD_H
#define SIMPLETHREAD_H

#include "mutex.h"

class U_EXPORT SimpleThread
{
  public:
    SimpleThread();
    virtual  ~SimpleThread();
    int32_t   start(void);        // start the thread. Return 0 if successfull.
    void      join();             // A thread must be joined before deleting its SimpleThread.

    virtual void run(void) = 0;   // Override this to provide the code to run
                                  //   in the thread.
  private:
    void *fImplementation;
};


class IntlTest;

class ThreadPoolBase {
  public:
    ThreadPoolBase(IntlTest *test, int32_t howMany);
    virtual ~ThreadPoolBase();
    
    void start();
    void join();

    virtual void callFn(int32_t param) = 0;
    void initThreads();

    friend class ThreadPoolThread;
  private:
    IntlTest  *fIntlTest;
    int32_t  fNumThreads;
    SimpleThread **fThreads;
};


template<class TestClass>
class ThreadPool : public ThreadPoolBase {
  private:
    TestClass *fTest;
    void (TestClass::*fRunFnPtr)(int32_t);
  public:
    ThreadPool(TestClass *test, int howMany, void (TestClass::*runFnPtr)(int32_t threadNumber)) :
        ThreadPoolBase(test, howMany), fTest(test), fRunFnPtr(runFnPtr) {initThreads(); };
    virtual ~ThreadPool() {};
  protected:
    virtual void callFn(int32_t param) {
        (fTest->*fRunFnPtr)(param);
    }
};
#endif

