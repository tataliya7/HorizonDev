#pragma once

#include <gtest/gtest.h>

#include "Core/CoreModule.h"

import HorizonEngine.Core.TaskGraph;

namespace HE
{
    class SimpleTask
    {
    public:

        SimpleTask(const char* name, float time)
            : name(name)
            , workingTime(time)
        {

        }

        void Execute()
        {
            OSSuspendCurrentThread(workingTime);
            printf("Task %s is completed.\n", name);
        }

    private:

        const char* name;
        float workingTime;
    };

    TEST(TaskGraphSystemTest, SimpleTask)
    {
        TaskGraphSystemInit(3);

        TaskGraphSystemEventRef eventA = nullptr;
        TaskGraphSystemEventRef eventB = nullptr;
        TaskGraphSystemEventRef eventC = nullptr;
        TaskGraphSystemEventRef eventD = nullptr;
        TaskGraphSystemEventRef eventE = nullptr;

        eventA = TaskGraphSystemDispatchTask(TaskGraphSystemTaskNode<SimpleTask>::Create("TaskA", 1.0f), nullptr, 0);

        std::array<TaskGraphSystemEventRef, 1> prerequisitesB = { eventA };
        eventB = TaskGraphSystemDispatchTask(TaskGraphSystemTaskNode<SimpleTask>::Create("TaskB", 1.0f), prerequisitesB.data(), (uint32)prerequisitesB.size());

        std::array<TaskGraphSystemEventRef, 1> prerequisitesC = { eventB };
        eventC = TaskGraphSystemDispatchTask(TaskGraphSystemTaskNode<SimpleTask>::Create("TaskC", 1.0f), prerequisitesC.data(), (uint32)prerequisitesC.size());

        std::array<TaskGraphSystemEventRef, 1> prerequisitesD = { eventA };
        eventD = TaskGraphSystemDispatchTask(TaskGraphSystemTaskNode<SimpleTask>::Create("TaskD", 1.0f), prerequisitesD.data(), (uint32)prerequisitesD.size());

        std::array<TaskGraphSystemEventRef, 2> prerequisitesE = { eventC, eventD };
        eventE = TaskGraphSystemDispatchTask(TaskGraphSystemTaskNode<SimpleTask>::Create("TaskE", 1.0f), prerequisitesE.data(), (uint32)prerequisitesE.size());

        TaskGraphSystemWaitForEvents(&eventE, 1);

        printf("All Tasks are completed.\n");

        TaskGraphSystemExit();
    }

    uint64 FibonacciSequence(uint32 n)
    {
        if (n <= 2)
        {
            return 1;
        }
        else
        {
            std::atomic<uint64> F1(0);
            std::atomic<uint64> F2(0);

            std::vector<TaskGraphSystemEventRef> events;
            TaskGraphSystemEventRef eventA = TaskGraphSystemDispatchTask(TaskGraphSystemLamdaTask::Create(
                [&F1, n] { F1 = FibonacciSequence(n - 1); }), nullptr, 0);
            events.push_back(eventA);
            TaskGraphSystemEventRef eventB = TaskGraphSystemDispatchTask(TaskGraphSystemLamdaTask::Create(
                [&F2, n] { F2 = FibonacciSequence(n - 2); }), nullptr, 0);
            events.push_back(eventB);
            TaskGraphSystemWaitForEvents(events.data(), (uint32)events.size());

            assert(F1 > 0 && F2 > 0);

            return F1 + F2;
        }
    }

    uint64 FibonacciSequenceSingleThread(uint32 n)
    {
        uint64 a = 0;
        uint64 b = 1;
        uint64 c = 0;
        if (n == 0)
        {
            return a;
        }
        for (uint32 i = 2; i <= n; i++)
        {
            c = a + b;
            a = b;
            b = c;
        }
        return b;
    }

    TEST(TaskGraphSystemTest, LamdaTask)
    {
        for (uint32 i = 0; i < 100; i++)
        {
            TaskGraphSystemInit(64);
            uint32 n = 10;

            uint64 result1 = FibonacciSequence(n);
            uint64 result2 = FibonacciSequenceSingleThread(n);
            assert(result1 == result2);

            printf("Fibonacci sequence: F%u is %llu.\n", n, result1);
            TaskGraphSystemExit();
        }
    }
}