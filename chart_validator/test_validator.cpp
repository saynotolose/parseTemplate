#include "chart_validator.h" // 头文件在同一目录下
#include <iostream>
#include <vector>
#include <string>

// 定义一个运行单次测试的函数
int run_test(const std::string& testName, const std::string& chartPath) {
    std::cout << "\n--- 测试: " << testName << " (" << chartPath << ") ---" << std::endl;
    std::vector<std::string> validationErrors;
    bool isStructureValid = chart_util::ValidateChartStructure(chartPath, validationErrors);

    if (isStructureValid) {
        std::cout << "结果: Chart 结构有效。" << std::endl;
        return 0; // 测试通过
    } else {
        std::cerr << "结果: Chart 结构无效。" << std::endl;
        std::cerr << "发现以下问题:" << std::endl;
        for (std::vector<std::string>::const_iterator it = validationErrors.begin(); it != validationErrors.end(); ++it) {
            std::cerr << "- " << *it << std::endl;
        }
        return 1; // 测试失败
    }
}

int main() {
    int failedTests = 0;

    // --- 在这里添加或修改你的测试用例 --- 
    // 您需要确保这些路径对应的目录和文件真实存在，或者修改为您的测试路径

    // 示例 1: 假设这是一个结构完整的 Chart
    // 注意：下面的 Windows 路径是为了解决编译错误，但在 Linux 上运行时无法访问。
    // 请确保将其替换为指向您测试 Chart 的有效 Linux 路径！
    failedTests += run_test("有效的 Chart", "C:\\Users\\oykk\\Desktop\\project\\myapp\\myapp");

    // // 示例 2: 假设缺少 Chart.yaml
    // failedTests += run_test("缺少 Chart.yaml", "./test_charts/missing_chart_yaml");

    // // 示例 3: 假设 values.yaml 是一个目录而不是文件
    // failedTests += run_test("values.yaml 是目录", "./test_charts/values_is_dir");

    // // 示例 4: 假设 templates 目录不存在
    // failedTests += run_test("缺少 templates 目录", "./test_charts/no_templates_dir");

    // // 示例 5: 假设 charts 存在但是个文件
    // failedTests += run_test("charts 是文件", "./test_charts/charts_is_file"); 

    // // 示例 6: 假设包含一个结构无效的子 Chart (需要创建相应的目录结构)
    // // failedTests += run_test("包含无效子 Chart", "./test_charts/parent_with_invalid_sub");

    // --- 测试用例结束 --- 

    std::cout << "\n--- 测试总结 ---" << std::endl;
    if (failedTests == 0) {
        std::cout << "所有测试通过！" << std::endl;
    } else {
        std::cerr << failedTests << " 个测试失败。" << std::endl;
    }

    return failedTests; // 返回失败的测试数量，0 表示全部成功
} 