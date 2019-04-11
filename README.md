Header only, FreeRTOS based set of tools that allows running deferred tasks in a single-threaded environment.

## Usage
### FreeRTOSHelpers::setImmediate
Callback will be delayed and queued by the scheduler.

```cpp
#include <include/freertoshelpers.hpp>

int main() {
    FreeRTOSHelpers::setImmediate([](){
        printf("world\n");
    });
    
    printf("hello\n")
    
    // FreeRTOS scheduler must be run
    vTaskStartScheduler();
}
```
Output:
```
hello
world
```

### FreeRTOSHelpers::setTimeout
Callback will be called by the scheduler after a period of time.

```cpp
#include <include/freertoshelpers.hpp>

int main() {
    FreeRTOSHelpers::setTimeout(1000, [](){
        printf("brave\n");
    });
    
    FreeRTOSHelpers::setTimeout(2000, [](){
        printf("world");
    });
    
    printf("hello\n")
    
    // FreeRTOS scheduler must be run
    vTaskStartScheduler();
}
```
Output:
```
[00.000] hello
[01.000] brave
[02.000] world
```

### FreeRTOSHelpers::setInterval
Callback will be called by the scheduler every n milliseconds.

```cpp
#include <include/freertoshelpers.hpp>

int main() {
    FreeRTOSHelpers::setInterval(1000, [](){
        printf("world\n");
    });
    
    printf("hello\n")
    
    // FreeRTOS scheduler must be run
    vTaskStartScheduler();
}
```
Output:
```
[00.000] hello
[01.000] world
[02.000] world
[03.000] world
...
```

### FreeRTOSHelpers::Function
std::function replacement using local storage instead of heap allocation.

```cpp
#include <include/freertoshelpers.hpp>

int main() {
    FreeRTOSHelpers::Function f0 = [](){
        printf("hello\n");
    };
    
    FreeRTOSHelpers::Function f1 = [](){
        printf("world\n");
    };
    
    f0();
    f1();
}
```
Output:
```
hello
world
```

The maximum size of capture variables is defined by FreeRTOSHelpers::FUNCTION_MAX_SIZE.

### FreeRTOSHelpers::ConditionVariable
FreeRTOS based std::condition_variable

```cpp
#include <include/freertoshelpers.hpp>

FreeRTOSHelpers::ConditionVariable cv;
        
void otherThread( ){
    while (true) {
        cv.wait();
        printf("world\n");
    }
}

int main() {
    FreeRTOSHelpers::setInterval(1000, [](){
        printf("hello\n");
        cv.notify();
    });
    
    // FreeRTOS scheduler must be run
    vTaskStartScheduler();
}
```
Output:
```
[01.000] hello
[01.000] world
[02.000] hello
[02.000] world
[03.000] hello
[03.000] world
...
```
