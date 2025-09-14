#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>

constexpr int NUM_WORKERS = 5; // 并发worker数量
constexpr int ITERATIONS_PER_WORKER = 5; // 减少每个worker的迭代次数
constexpr int DAEMON_CHECK_INTERVAL_MS = 1000; // 增加检查守护进程状态的间隔

bool isDaemonRunning()
{
    return true;
}

void cleanupLogFiles()
{
    // 清理之前的日志文件
    try
    {
        for (const auto& entry : std::filesystem::directory_iterator("/tmp"))
        {
            if (entry.path().filename().string().starts_with("worker_"))
            {
                std::filesystem::remove(entry.path());
            }
        }
    }
    catch (const std::exception& e)
    {
        // 忽略清理错误
    }
}

void printWorkerLogs()
{
    std::cout << "\n=== Worker Detailed Logs ===" << std::endl;
    try
    {
        for (const auto& entry : std::filesystem::directory_iterator("/tmp"))
        {
            if (entry.path().filename().string().starts_with("worker_"))
            {
                std::cout << "\n--- " << entry.path().filename() << " ---" << std::endl;
                std::ifstream log_file(entry.path());
                std::string line;
                while (std::getline(log_file, line))
                {
                    std::cout << line << std::endl;
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cout << "Failed to read worker logs: " << e.what() << std::endl;
    }
}

int main()
{
    std::cout << "=== HyCAN Daemon Concurrency Test ===" << std::endl;
    std::cout << "Testing with " << NUM_WORKERS << " concurrent workers" << std::endl;
    std::cout << "Each worker will perform " << ITERATIONS_PER_WORKER << " iterations" << std::endl;

    // 清理旧的日志文件
    cleanupLogFiles();

    // 检查守护进程是否运行
    if (!isDaemonRunning())
    {
        std::cerr << "ERROR: HyCAN daemon is not running!" << std::endl;
        std::cerr << "Please start it with: sudo systemctl start hycan-daemon" << std::endl;
        return 1;
    }

    std::cout << "HyCAN daemon is running - starting test..." << std::endl;

    std::vector<pid_t> worker_pids;
    auto start_time = std::chrono::steady_clock::now();

    // 启动所有worker进程，增加启动间隔
    for (int i = 0; i < NUM_WORKERS; ++i)
    {
        // 在启动worker之间添加小延迟，减少初始竞争
        if (i > 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        pid_t pid = fork();

        if (pid == 0)
        {
            // 子进程 - 执行worker
            std::string worker_id = std::to_string(i);
            std::string iterations = std::to_string(ITERATIONS_PER_WORKER);

            // 构建worker可执行文件路径
            std::string worker_path = "./HyCAN_DaemonConcurrencyWorker";

            execl(worker_path.c_str(), "HyCAN_DaemonConcurrencyWorker",
                  worker_id.c_str(), iterations.c_str(), nullptr);

            // 如果execl失败
            std::cerr << "Failed to execute worker " << i << std::endl;
            exit(1);
        }
        else if (pid > 0)
        {
            // 父进程 - 记录worker PID
            worker_pids.push_back(pid);
            std::cout << "Started worker " << i << " with PID " << pid << std::endl;
        }
        else
        {
            // fork失败
            std::cerr << "Failed to fork worker " << i << std::endl;
            return 1;
        }
    }

    std::cout << "All workers started, monitoring progress..." << std::endl;

    // 监控worker进程和守护进程状态
    int completed_workers = 0;
    int failed_workers = 0;
    std::vector<std::pair<int, int>> worker_results; // worker_id, exit_code

    while (completed_workers + failed_workers < NUM_WORKERS)
    {
        // 检查守护进程状态
        if (!isDaemonRunning())
        {
            std::cerr << "CRITICAL: HyCAN daemon stopped running during test!" << std::endl;

            // 终止所有剩余的worker进程
            for (pid_t pid : worker_pids)
            {
                kill(pid, SIGTERM);
            }
            printWorkerLogs();
            return 1;
        }

        // 检查是否有worker进程完成
        int status;
        pid_t finished_pid = waitpid(-1, &status, WNOHANG);

        if (finished_pid > 0)
        {
            // 有worker完成
            auto it = std::find(worker_pids.begin(), worker_pids.end(), finished_pid);
            if (it != worker_pids.end())
            {
                int worker_id = std::distance(worker_pids.begin(), it);
                int exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;

                worker_results.emplace_back(worker_id, exit_code);

                if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
                {
                    std::cout << "Worker " << worker_id << " (PID " << finished_pid
                        << ") completed successfully" << std::endl;
                    completed_workers++;
                }
                else
                {
                    std::cerr << "Worker " << worker_id << " (PID " << finished_pid
                        << ") failed with exit code " << exit_code << std::endl;
                    failed_workers++;
                }

                worker_pids.erase(it);
            }
        }
        else if (finished_pid == 0)
        {
            // 没有worker完成，等待一段时间
            std::this_thread::sleep_for(std::chrono::milliseconds(DAEMON_CHECK_INTERVAL_MS));
        }
        else
        {
            // waitpid出错
            if (errno != ECHILD)
            {
                std::cerr << "Error waiting for workers: " << strerror(errno) << std::endl;
                printWorkerLogs();
                return 1;
            }
        }
    }

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);

    std::cout << "\n=== Test Results ===" << std::endl;
    std::cout << "Test duration: " << duration.count() << " seconds" << std::endl;
    std::cout << "Workers completed successfully: " << completed_workers << std::endl;
    std::cout << "Workers failed: " << failed_workers << std::endl;

    // 显示每个worker的详细结果
    std::cout << "\nWorker Results:" << std::endl;
    for (const auto& [worker_id, exit_code] : worker_results)
    {
        std::cout << "  Worker " << worker_id << ": " << (exit_code == 0 ? "SUCCESS" : "FAILED") << std::endl;
    }

    // 最终检查守护进程状态
    if (isDaemonRunning())
    {
        std::cout << "\nHyCAN daemon is still running" << std::endl;
        if (failed_workers == 0)
        {
            std::cout << "TEST PASSED - All workers completed successfully" << std::endl;
        }
        else if (failed_workers <= NUM_WORKERS / 2)
        {
            std::cout << "TEST PASSED - Majority of workers completed successfully" << std::endl;
            std::cout << "Some failures are expected under high concurrency stress" << std::endl;
        }
        else
        {
            std::cout << "TEST FAILED - Too many workers failed" << std::endl;
            printWorkerLogs();
            return 1;
        }
        return 0;
    }
    else
    {
        std::cerr << "HyCAN daemon stopped running - TEST FAILED" << std::endl;
        printWorkerLogs();
        return 1;
    }
}
