#include "chart_processor.h" // 包含 Chart 处理器的头文件
#include <iostream>          // C++98 IO
#include <map>               // C++98 map
#include <vector>            // C++98 vector
#include <string>            // C++98 string

int main() {
    // --- 配置测试 Chart 路径 ---
    // 重要: 请将下面的路径修改为您实际包含主 Chart 和子 Chart 的测试目录路径。
    // 建议使用相对路径 (相对于可执行文件的运行位置)。
    // 例如，如果可执行文件在项目根目录，测试 Chart 在 "./test_data/chart_with_subchart/"
    // 注意：下面的 Windows 路径是为了解决编译错误，但在 Linux 上运行时无法访问。
    // 请确保将其替换为指向您测试 Chart 的有效 Linux 路径！
    std::string chartToTestPath = "C:\\Users\\oykk\\Desktop\\project\\myapp\\myapp"; 
    // ---------------------------

    std::cout << "开始测试 Chart 处理: " << chartToTestPath << std::endl;

    // 用于存储渲染结果和错误的容器
    std::map<std::string, std::string> renderedOutputs;
    std::vector<std::string> processingErrors;

    // 调用 Chart 处理函数
    bool success = chart_processor::ProcessChartTemplates(
        chartToTestPath,
        renderedOutputs,
        processingErrors
    );

    // 打印处理结果
    std::cout << "\n--- 处理结果 ---" << std::endl;
    if (success) {
        std::cout << "Chart 处理流程完成 (可能包含个别模板渲染错误)。" << std::endl;
    } else {
        std::cerr << "Chart 处理过程中发生严重错误！" << std::endl;
    }

    // 打印渲染的模板内容
    std::cout << "\n--- 渲染的模板 --- (" << renderedOutputs.size() << " 个文件)" << std::endl;
    std::map<std::string, std::string>::const_iterator it;
    for (it = renderedOutputs.begin(); it != renderedOutputs.end(); ++it) {
        std::cout << "\n--- 文件: " << it->first << " ---\n";
        std::cout << it->second; // 渲染结果
        std::cout << "\n--- 文件结束: " << it->first << " ---\n";
    }

    // 打印处理过程中的错误或警告
    if (!processingErrors.empty()) {
        std::cerr << "\n--- 处理过程中的错误/警告 --- (" << processingErrors.size() << " 条)" << std::endl;
        for (size_t i = 0; i < processingErrors.size(); ++i) {
            std::cerr << "- " << processingErrors[i] << std::endl;
        }
    } else {
        std::cout << "\n--- 处理过程中无错误/警告 ---" << std::endl;
    }

    // 根据是否有严重错误返回状态码
    bool hadFatalError = !success; // 如果 ProcessChartTemplates 返回 false，则为严重错误
     for (size_t i = 0; i < processingErrors.size(); ++i) {
         // 如果错误信息以 "错误:" 开头，也认为是严重错误
         if (processingErrors[i].rfind("错误:", 0) == 0) {
             hadFatalError = true;
             break;
         }
     }

    std::cout << "\n测试完成。" << std::endl;
    return hadFatalError ? 1 : 0; // 失败返回 1，成功返回 0
} 