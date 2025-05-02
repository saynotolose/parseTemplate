// #include "values.h"
// #include "exec.h"
// #include <fstream>
// #include <sstream>
// #include <iostream>
// #include <string>
// #include "template_syntax_checker.h"

// using namespace template_engine;

// int main() {
//     // 1. 解析 values0.yaml
//     Values* values = NULL;
//     try {
//         values = ParseSimpleYAMLFile("values0.yaml");
//     } catch (const std::exception& e) {
//         std::cerr << "解析 values0.yaml 失败: " << e.what() << std::endl;
//         return 1;
//     }

//     // 2. 读取 deployment0.yaml 模板内容
//     std::ifstream t("deployment0.yaml");
//     if (!t.is_open()) {
//         std::cerr << "无法打开 deployment0.yaml" << std::endl;
//         delete values;
//         return 1;
//     }
//     std::stringstream buffer;
//     buffer << t.rdbuf();
//     std::string templateStr = buffer.str();

//     // 3. 校验模板语法
//     std::vector<TemplateSyntaxError> errors;
//     std::string dataStr;
//     {
//         std::ifstream dataFile("values0.yaml");
//         if (dataFile.is_open()) {
//             std::stringstream ss;
//             ss << dataFile.rdbuf();
//             dataStr = ss.str();
//         }
//     }
//     if (!CheckTemplateSyntax("deployment0", templateStr, "{{", "}}", dataStr, errors)) {
//         for (size_t i = 0; i < errors.size(); ++i) {
//             std::cerr << "模板语法错误[" << (i+1) << "]: 类型=" << errors[i].type
//                       << " 行=" << errors[i].line
//                       << " 描述=" << errors[i].description << std::endl;
//             if (!errors[i].context.empty()) {
//                 std::cerr << "  上下文: " << errors[i].context << std::endl;
//             }
//             std::cerr << "  详细: " << errors[i].message << std::endl;
//         }
//         delete values;
//         return 1;
//     }

//     // 4. 渲染模板
//     try {
//         std::string result = ExecuteTemplate("deployment0", templateStr, values, "{{", "}}", ExecOptions());
//         std::cout << "渲染结果:\n" << result << std::endl;
//     } catch (const std::exception& e) {
//         std::cerr << "模板渲染失败: " << e.what() << std::endl;
//         delete values;
//         return 1;
//     }

//     // 5. 释放内存
//     delete values;
//     return 0;
// } 