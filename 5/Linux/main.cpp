#include <iostream>
#include <dlfcn.h>

using namespace std;

int main()
{
    system("clear");

    void* loadedLibrary; 
    void (*concat)(void);

    loadedLibrary = dlopen("/home/user/CLionProjects/SPOVM5/libconcatFiles.so", RTLD_LAZY);  
    *(void **) (&concat) = dlsym(loadedLibrary, "concat"); 

    concat(); 

    dlclose(loadedLibrary);  
    return 0;
}
