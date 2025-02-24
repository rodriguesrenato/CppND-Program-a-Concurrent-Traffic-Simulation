#include <iostream>
#include <random>
#include "TrafficLight.h"
#include <stdlib.h>
// #include <time.h>
#include <chrono>
#include <thread>
#include <future>
// #include <cmath>

/* Implementation of class "MessageQueue" */

template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait()
    // to wait for and receive new messages and pull them from the queue using move semantics.
    // The received object should then be returned by the receive function.

    // perform queue modification under the lock
    std::unique_lock<std::mutex> uLock(_mtx);

    // wait until new T elemente be added to _queue
    _cond.wait(uLock, [this] { return !_queue.empty(); }); // pass unique lock to condition variable

    // remove last element from _queue
    T msg = std::move(_queue.back());
    _queue.pop_back();

    return msg;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex>
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.

    // perform _queue modification under the lock
    std::lock_guard<std::mutex> uLock(_mtx);

    // Clear queue if it has accumulated previous elements
    if(!_queue.empty()) _queue.clear();

    // add T to _queue
    _queue.push_back(std::move(msg));

    // notify condition variable on receive method
    _cond.notify_one();
}

/* Implementation of class "TrafficLight" */


TrafficLight::TrafficLight()
{
    std::lock_guard<std::mutex> lck(_mutex);
    _currentPhase = TrafficLightPhase::red;

    // initialize the queue as a shared pointer to be accessed by different threads
    _traficLightPhaseQueue = std::make_shared<MessageQueue<TrafficLightPhase>>();
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.

    while(true){
        TrafficLightPhase tflCheck = _traficLightPhaseQueue->receive();
        
        // if receive a green TrafficLightPhase, then exit while loop
        if(tflCheck == TrafficLightPhase::green) break;
    }
}

// Get the current Trafic light phase safely with the lock
TrafficLightPhase TrafficLight::getCurrentPhase()
{
    std::lock_guard<std::mutex> lck(_mutex);
    return _currentPhase;
}


void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class.
    
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles
    // and toggles the current phase of the traffic light between red and green and sends an update method
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds.
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles.
    
    // initialize stop watch variable with current time
    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

    // initialize cycle duration as 4 seconds.
    int cycleDuration = 4;
    while (true)
    {
        // calculate how much time has passed
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - t1).count();
        if (duration >= cycleDuration)
        {
            // generate a new random cycle duration between 4 and 6 seconds.
            std::random_device rd;
            std::mt19937 eng(rd());
            std::uniform_int_distribution<> distr(4, 6);
            cycleDuration = distr(eng);

            // lock to toggle _currentPhase value under if context
            std::lock_guard<std::mutex> lck(_mutex);

            // toggle _currentPhase value
            _currentPhase = (_currentPhase == TrafficLightPhase::red) ? TrafficLightPhase::green : TrafficLightPhase::red;

            TrafficLightPhase t = _currentPhase;
            _traficLightPhaseQueue->send(std::move(t));

            // update stop watch time
            t1 = std::chrono::high_resolution_clock::now();
        }

        // prevent high cpu usage while waiting timeout
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
