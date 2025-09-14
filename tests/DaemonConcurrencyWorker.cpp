#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <random>
#include <fstream>

#include "HyCAN/Interface/Interface.hpp"

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <worker_id> <iterations>" << std::endl;
        return 1;
    }

    int worker_id = std::stoi(argv[1]);
    int iterations = std::stoi(argv[2]);

    // 使用时间戳和PID创建更唯一的接口名称，避免冲突
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    std::string interface_name = "vcan_w" + std::to_string(worker_id) + "_" + std::to_string(getpid()) + "_" +
        std::to_string(timestamp % 10000);

    // 创建worker专用的日志文件，避免输出混乱
    std::string log_file = "/tmp/worker_" + std::to_string(worker_id) + "_" + std::to_string(getpid()) + ".log";
    std::ofstream log(log_file);

    log << "Worker " << worker_id << " (PID " << getpid() << ") starting with interface " << interface_name
        << " for " << iterations << " iterations" << std::endl;

    // 设置随机延迟生成器，增加延迟范围以减少竞争
    std::random_device rd;
    std::mt19937 gen(rd() + worker_id + getpid()); // 使用worker_id和PID作为种子增强随机性
    std::uniform_int_distribution<> delay_dist(50, 300); // 增加到50-300ms随机延迟
    std::uniform_int_distribution<> retry_delay(10, 50); // 重试延迟

    int successful_iterations = 0;

    for (int i = 0; i < iterations; ++i)
    {
        bool iteration_success = false;
        int retry_count = 0;
        const int max_retries = 3;

        while (!iteration_success && retry_count < max_retries)
        {
            try
            {
                log << "Worker " << worker_id << " - Iteration " << (i + 1) << "/" << iterations;
                if (retry_count > 0)
                {
                    log << " (retry " << retry_count << ")";
                }
                log << std::endl;

                // 在重试时稍微改变接口名以避免冲突
                std::string current_interface = interface_name;
                if (retry_count > 0)
                {
                    current_interface += "_r" + std::to_string(retry_count);
                }

                // 创建VCAN接口
                HyCAN::VCANInterface vcan(current_interface);

                // 添加小的随机延迟减少并发压力
                std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay(gen)));

                // 启动接口
                auto result = vcan.up();
                if (!result)
                {
                    log << "Worker " << worker_id << " - Failed to bring up interface " << current_interface
                        << ": " << result.error().message << std::endl;
                    retry_count++;
                    std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay(gen)));
                    continue;
                }

                log << "Worker " << worker_id << " - Interface " << current_interface << " is up" << std::endl;

                // 随机等待一段时间
                std::this_thread::sleep_for(std::chrono::milliseconds(delay_dist(gen)));

                // 关闭接口
                auto down_result = vcan.down();
                if (!down_result)
                {
                    log << "Worker " << worker_id << " - Failed to bring down interface " << current_interface
                        << ": " << down_result.error().message << std::endl;
                }
                else
                {
                    log << "Worker " << worker_id << " - Interface " << current_interface << " is down" << std::endl;
                    successful_iterations++;
                    iteration_success = true;
                }

                // 短暂等待再进行下一次迭代
                std::this_thread::sleep_for(std::chrono::milliseconds(delay_dist(gen)));
            }
            catch (const std::exception& e)
            {
                log << "Worker " << worker_id << " - Exception: " << e.what() << std::endl;
                retry_count++;
                if (retry_count < max_retries)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay(gen)));
                }
            }
        }

        if (!iteration_success)
        {
            log << "Worker " << worker_id << " - Failed iteration " << (i + 1) << " after " << max_retries << " retries"
                << std::endl;
        }
    }

    log << "Worker " << worker_id << " completed: " << successful_iterations << "/" << iterations <<
        " successful iterations" << std::endl;
    log.close();

    // 输出最终结果到stdout供主程序读取
    std::cout << "Worker " << worker_id << " (PID " << getpid() << ") completed: "
        << successful_iterations << "/" << iterations << " successful" << std::endl;

    // 如果成功率低于80%，认为失败
    return (successful_iterations >= iterations * 0.8) ? 0 : 1;
}
