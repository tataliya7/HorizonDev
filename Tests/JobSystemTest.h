#pragma once

#include <gtest/gtest.h>

#include "Core/CoreModule.h"
#include <windows.h>
#include <optick.h>

namespace HE
{
    struct Timer
    {
        std::string name;
        std::chrono::high_resolution_clock::time_point start;

        Timer(const std::string& name) : name(name), start(std::chrono::high_resolution_clock::now()) {}
        ~Timer()
        {
            std::chrono::steady_clock::time_point end = std::chrono::high_resolution_clock::now();
            std::cout << name << ": " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " milliseconds" << std::endl;
        }
    };

    void Spin(float milliseconds)
    {
        milliseconds /= 1000.0f;
        std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
        double ms = 0;
        while (ms < milliseconds)
        {
            std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
            ms = time_span.count();
        }
    }
    
    struct Data
    {
        float m[16];
        void Compute()
        {
            for (int i = 0; i < 16; ++i)
            {
                m[i] = float(1 + i);
            }
        }
    };

    TEST(JobSystemTest, Run)
    {
        OPTICK_FRAME("MainThread");

        uint32 jobSystemWorkerThreadCount = std::max(HE::GetNumberOfProcessors() - 2u, 1u);
        uint32 HE_JOB_SYSTEM_NUM_FIBIERS = 128;
        uint32 HE_JOB_SYSTEM_FIBER_STACK_SIZE = HE_JOB_SYSTEM_NUM_FIBIERS * 1024;
        JobSystemInit(jobSystemWorkerThreadCount, HE_JOB_SYSTEM_NUM_FIBIERS, HE_JOB_SYSTEM_FIBER_STACK_SIZE);

        const uint32 numJobs = 10000000;

        Data* jobDataST = new Data[numJobs];
        {
            Timer timer("Single Thread Loop");
            for (uint32 i = 0; i < numJobs; i++)
            {
                jobDataST[i].Compute();
            }
        }

        Data* jobDataMT = new Data[numJobs];
        {
            {
                JobSystemCounterHandle counter1 = JobSystemDispatchJob("JobA", JobSystemPriority::High, [jobDataMT](const JobSystemJobContext& context) {
                    Timer timer("JobA");
                    for (uint32 i = 0; i < numJobs / 4; i++)
                    {
                        jobDataMT[i].Compute();
                    }
                });
                JobSystemCounterHandle counter2 = JobSystemDispatchJob("JobB", JobSystemPriority::High, [jobDataMT](const JobSystemJobContext& context) {
                    Timer timer("JobB");
                    for (uint32 i = numJobs / 4; i < numJobs / 2; i++)
                    {
                        jobDataMT[i].Compute();
                    }
                });
                JobSystemCounterHandle counter3 = JobSystemDispatchJob("JobC", JobSystemPriority::High, [jobDataMT](const JobSystemJobContext& context) {
                    Timer timer("JobC");
                    for (uint32 i = numJobs / 2; i < 3 * numJobs / 4; i++)
                    {
                        jobDataMT[i].Compute();
                    }
                });
                JobSystemCounterHandle counter4 = JobSystemDispatchJob("JobD", JobSystemPriority::High, [jobDataMT](const JobSystemJobContext& context) {
                    Timer timer("JobD");
                    for (uint32 i = 3 * numJobs / 4; i < numJobs; i++)
                    {
                        jobDataMT[i].Compute();
                    }
                });

                JobSystemWaitForCounterAndFreeWithoutFiber(counter1);
                JobSystemWaitForCounterAndFreeWithoutFiber(counter2);
                JobSystemWaitForCounterAndFreeWithoutFiber(counter3);
                JobSystemWaitForCounterAndFreeWithoutFiber(counter4);
            }
        }

        for (uint32 i = 0; i < numJobs; i++)
        { 
            for (uint32 j = 0; j < 16; j++)
            {
                assert(jobDataST[i].m[j] == jobDataMT[i].m[j]);
            }
        }

        delete[] jobDataST;
        delete[] jobDataMT;

        JobSystemExit();
    }
}