// #include "values.h"
// #include <iostream>
// #include <fstream>
// #include <string>

// using namespace template_engine;

// int main() {
//     try {
//         // 解析 values.yaml 文件
//         Values* root = ParseSimpleYAMLFile("values.yaml");

//         // 打印解析后的对象树
//         if (root) {
//             std::cout << "解析结果如下：" << std::endl;
//             root->Print(std::cout, 0);
//             delete root;
//         } else {
//             std::cout << "解析结果为空" << std::endl;
//         }
//     } catch (const std::exception& e) {
//         std::cout << "解析失败: " << e.what() << std::endl;
//     }
//     return 0;
// } 