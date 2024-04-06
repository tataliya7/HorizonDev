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

    struct Job1Data
    {
        uint32 numJobs;
        Data* data;
    }; 

    void Job1(void* data)
    {
        Job1Data* jobData = (Job1Data*)data;
        for (uint32 i = 0; i < jobData->numJobs; i++)
        {
            (jobData->data)[i].Compute();
        }
    }

    struct Job2Data
    {
        float ms;
    };

    void Job2(void* data)
    {
        float* jobData = (float*)data;
        Spin(*jobData);
    }

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
            uint32 numGroups = 4;
            Job1Data* job1Data = new Job1Data[numGroups];
            JobSystemJobDecl* job1Decls = new JobSystemJobDecl[numGroups];
            {
                Timer timer("Job System");

                for (uint32 i = 0; i < numGroups; i++)
                {
                    job1Data[i].numJobs = numJobs / numGroups;
                    job1Data[i].data = &jobDataMT[i * (numJobs / numGroups)];
                    job1Decls[i] = JobSystemJobDecl(&job1Data[i], Job1);
                }
             
                JobSystemCounterHandle counter = JobSystemRunJobs(job1Decls, numGroups);
                JobSystemWaitForCounterAndFreeWithoutFiber(counter);
            }

            delete[] job1Data;
            delete[] job1Decls;
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

        uint32 numJobs2 = jobSystemWorkerThreadCount;
        Job2Data* job2Data = new Job2Data[numJobs2];
        JobSystemJobDecl* job2Decls = new JobSystemJobDecl[numJobs2];
        for (uint32 i = 0; i < numJobs2; i++)
        {
            job2Data[i].ms = 100.0f;
            job2Decls[i] = JobSystemJobDecl(&job2Data[i], Job2);
        }

        {
            Timer timer("Single Thread Loop");
            for (uint32 i = 0; i < numJobs2; i++)
            {
                Job2(&job2Data[i]);
            }
        }

        {
            Timer timer("Job System");
            JobSystemCounterHandle counter = JobSystemRunJobs(job2Decls, numJobs2);
            JobSystemWaitForCounterAndFreeWithoutFiber(counter);
        }

        delete[] job2Data;
        delete[] job2Decls;

        JobSystemExit();
    }
}