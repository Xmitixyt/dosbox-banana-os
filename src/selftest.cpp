/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <stdexcept>
#include <unistd.h>
#include <pthread.h>
static_assert(sizeof(pthread_mutex_t)==40,"GNU libstdc++ filebuf mutex ABI");
static_assert(sizeof(mbstate_t)==8,"GNU libstdc++ conversion state ABI");
namespace {
struct Base { virtual ~Base() {} };
struct Derived:Base { int value; Derived():value(42){} };
static int failures;
void check(bool ok,const char *name) {
    printf("%s %s\n",ok?"PASS":"FAIL",name);if(!ok)failures++;
}
__attribute__((noinline)) void throw_integer() {throw 73;}
}
extern "C" int dosbox_runtime_selftest(void) {
    failures=0;
    Derived d; Base *b=&d;
    check(dynamic_cast<Derived*>(b)==&d,"C++ dynamic_cast");
    bool caught=false;
    try {throw_integer();} catch(int value) {caught=value==73;}
    check(caught,"C++ exception unwinding");
    std::stringstream stream;stream<<1234<<" "<<2.5;
    int number=0;double real=0;stream>>number>>real;
    check(number==1234 && real==2.5,"C++ locale and string streams");
    std::vector<int> values(2000,42);
    check(values.at(1999)==42,"C++ allocation and vector");
    caught=false;
    try {values.at(2000);} catch(const std::out_of_range&) {caught=true;}
    check(caught,"libstdc++ exceptions");
    char path[256];snprintf(path,sizeof(path),"/tmp/dosbox-selftest-%d.txt",(int)getpid());
    {std::ofstream out(path);out<<"Banana DOSBox "<<1234<<"\n";check(out.good(),"C++ file write");}
    {std::ifstream in(path);std::string line;std::getline(in,line);check(line=="Banana DOSBox 1234","C++ file read");}
    unlink(path);
    printf("DOSBOX_RUNTIME_SELFTEST: %d failed\n",failures);
    return failures?1:0;
}
