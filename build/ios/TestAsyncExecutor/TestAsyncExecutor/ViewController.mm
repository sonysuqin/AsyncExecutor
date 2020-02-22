//
//  ViewController.m
//  TestAsyncExecutor
//
//  Created by Mac on 2020/2/22.
//  Copyright © 2020 sohu-inc. All rights reserved.
//

#import "ViewController.h"

#include <stdio.h>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>

#include "executor.h"

#ifdef LEAK_CHECK
#ifdef WIN32
#include <vld.h>
#endif
#endif

using namespace qrtc;

Executor executor;

__uint64_t GetTid() {
    __uint64_t tid = 0;
    if (pthread_threadid_np(0, &tid)) {
        tid = pthread_mach_thread_np(pthread_self());
    }
    return tid;
}

class Test {
public:
    Test() {}
    ~Test() {}
    
    void print1() { printf("[Exector:%llu] print1\n", GetTid()); }
    
    void print2() { printf("[Exector:%llu] print2\n", GetTid()); }
    
    void fun1() {
        printf("[Exector:%llu] Test::fun1() in\n", GetTid());
        Test t;
        // Recursive test.
        executor.Invoke<void>(std::bind(&Test::print1, &t));
        // Recursive test.
        int r = executor.Invoke<int>(std::bind(&Test::fun3, &t));
        printf("[Exector:%llu] Invoke fun3() return %d\n", GetTid(), r);
        printf("[Exector:%llu] Test::fun1() out\n\n", GetTid());
    }
    
    int fun2() {
        printf("[Exector:%llu] Test::fun2() in\n", GetTid());
        Test t;
        // Recursive test.
        executor.PostTask(std::bind(&Test::print2, &t));
        printf("[Exector:%llu] Test::fun2() out\n\n", GetTid());
        return 2;
    }
    
    int fun3() { return 3; }
};

int test() {
    printf("[Main:%llu] Main thread.\n", GetTid());
    executor.Start();
    if (1) {
        Test t;
        printf("[Main:%llu] PostTask fun1().\n", GetTid());
        executor.PostTask(std::bind(&Test::fun1, &t));
        printf("[Main:%llu] Invoke fun2().\n", GetTid());
        int i = executor.Invoke<int>(std::bind(&Test::fun2, &t));
        printf("[Main:%llu] Invoke fun2() return %d\n", GetTid(), i);
    }
    executor.Stop();
    return 0;
}


@interface ViewController ()
@property (weak, nonatomic) IBOutlet UITextView *textView;
@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
    test();
    _textView.text = @"AsyncExecutor test done.\nPlease check console output.";
}

@end
