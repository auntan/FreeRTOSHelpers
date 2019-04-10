#pragma once

#include <utility>
#include <cstring>
#include <cassert>

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

namespace FreeRTOSHelpers
{    

constexpr int SET_IMMEDIATE_MAX_COUNT = 100;
constexpr int SET_IMMEDIATE_LAMBDA_MAX_SIZE = 16;

constexpr int SET_TIMEOUT_MAX_COUNT = 100;
constexpr int SET_TIMEOUT_LAMBDA_MAX_SIZE = 32;

constexpr int TIMER_TICKS_TO_WAIT = pdMS_TO_TICKS(10);

constexpr int FUNCTION_MAX_SIZE = 32;

template <typename F>
void setImmediate(F f);

namespace Private
{

struct SetImmediateData
{
    char lambda[SET_IMMEDIATE_LAMBDA_MAX_SIZE]; 
    bool busy = false;
};

struct SetTimeoutData
{
    char lambda[SET_TIMEOUT_LAMBDA_MAX_SIZE];
    TimerHandle_t handle;
    StaticTimer_t data;
    bool repeat;
    bool busy = false;
};

template <typename F>
void startTimer(int interval, bool repeat, F f)
{
    if (interval == 0) {
        setImmediate(std::move(f));
        return;
    }
    
    static_assert(sizeof(F) <= SET_TIMEOUT_LAMBDA_MAX_SIZE);
    
    for (int i = 0; i < SET_TIMEOUT_MAX_COUNT; i++) {        
        static SetTimeoutData setTimeoutData[SET_TIMEOUT_MAX_COUNT];
        
        if (!setTimeoutData[i].busy) {

            setTimeoutData[i].busy = true;
            setTimeoutData[i].repeat = repeat;
            
            memcpy(setTimeoutData[i].lambda, &f, sizeof(F));
            
            setTimeoutData[i].handle = xTimerCreateStatic("FreeRTOSHelpersTimer", pdMS_TO_TICKS(interval), repeat, nullptr, [](TimerHandle_t handle) {
                for (int i = 0; i < SET_TIMEOUT_MAX_COUNT; i++) {
                    if (setTimeoutData[i].handle == handle) {
                        if (!setTimeoutData[i].repeat) {
                            setTimeoutData[i].busy = false;
                        }
                        (*(F*)&setTimeoutData[i].lambda)();
                        return;
                    }
                }
            }, &setTimeoutData[i].data);

            xTimerStart(setTimeoutData[i].handle, TIMER_TICKS_TO_WAIT);
            return;
        }
    }
    configASSERT(false);
}
    
}
    
// Perfomance depends on sizeof(F):
// <= 8: near same perfomance as direct xTimerPendFunctionCall call
// > 8: twice slower
template <typename F>
void setImmediate(F f)
{    
    if constexpr (sizeof(F) <= sizeof(uint32_t)) {
        uint32_t *buf = (uint32_t*)&f;
        xTimerPendFunctionCall([](void *part_a, uint32_t) {
            (*(F*)part_a)();
        }, buf, 0, TIMER_TICKS_TO_WAIT);	    
    } else if constexpr (sizeof(F) <= sizeof(void*) + sizeof(uint32_t)) {
        uint32_t *buf = (uint32_t*)&f;
        xTimerPendFunctionCall([](void *part_a, uint32_t part_b) {
            char binary[sizeof(F)];
            memcpy(binary, &part_a, sizeof(part_a));
            memcpy(binary + sizeof(part_a), &part_b, sizeof(F) - sizeof(part_a));
            (*(F*)binary)();
        }, (void*)buf[0], buf[1], TIMER_TICKS_TO_WAIT);
    } else {
        static_assert(sizeof(F) <= SET_IMMEDIATE_LAMBDA_MAX_SIZE);
        
        static Private::SetImmediateData setImmediateData[SET_IMMEDIATE_MAX_COUNT];
        
        for (int i = 0; i < SET_IMMEDIATE_MAX_COUNT; i++) {
            if (!setImmediateData[i].busy) {
                setImmediateData[i].busy = true;
                memcpy(setImmediateData[i].lambda, &f, sizeof(F));
                xTimerPendFunctionCall([](void *param, uint32_t) {
                    Private::SetImmediateData *set_immediate = (Private::SetImmediateData*)param;
                    set_immediate->busy = false;
                    (*(F*)set_immediate->lambda)();
                }, &setImmediateData[i], 0, TIMER_TICKS_TO_WAIT);
                return;
            }
        }
        configASSERT(false);
    }
}

template <typename F>
void setTimeout(int t, F f) 
{
   Private::startTimer(t, false, std::move(f));
}
   
template <typename F>
void setInterval(int t, F f) 
{
    Private::startTimer(t, true, std::move(f));
}
    
class Function
{
public:
    Function() = default;
    
    template<typename F>
    Function(F f) { this->operator=(std::move(f)); }
    
    template<typename F>
    void operator=(F f)
    {
        static_assert(sizeof(F) <= sizeof(_parameters));

        _function = [](void *param) {
            (*(F*)param)();
        };
        memcpy(_parameters, &f, sizeof(F));
    }

    void operator()()
    {
        assert(_function);
        
	    _function(_parameters);
    }
    
    operator bool() const { return _function; }

private:
    void(*_function)(void*) = nullptr;
    char _parameters[FUNCTION_MAX_SIZE];
};

class ConditionVariable
{
public:
    void notify() { xTaskNotify(_task, 0, eNoAction); }

    void wait()
    {
        _task = xTaskGetCurrentTaskHandle();
        xTaskNotifyWait(0, 0, nullptr, portMAX_DELAY);
    }

    bool wait_for(int ms)
    {
        _task = xTaskGetCurrentTaskHandle();
        return xTaskNotifyWait(0, 0, nullptr, pdMS_TO_TICKS(ms)) == pdTRUE;
    }

private:
    TaskHandle_t _task = nullptr;
};
    
}
