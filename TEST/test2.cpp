// // main.cpp
// #include "values.h"
// #include "exec.h"
// #include <iostream>
// #include <string>

// using namespace template_engine;

// // 在test2.cpp的main函数中添加测试
// void testNestedFieldAccess() {
//     using namespace template_engine;
    
//     std::cout << "\n===== 嵌套字段访问测试 =====\n";
    
//     // 创建嵌套结构
//     Values* root = Values::MakeMap(std::map<std::string, Values*>());
//     Values* person = Values::MakeMap(std::map<std::string, Values*>());
//     Values* name = Values::MakeMap(std::map<std::string, Values*>());
    
//     // 添加name.first和name.last
//     (*name)["first"] = Values::MakeString("John");
//     (*name)["last"] = Values::MakeString("Doe");
    
//     // 添加person.name
//     (*person)["name"] = name;
    
//     // 添加根级字段
//     (*root)["person"] = person;
    
//     // 打印整个结构
//     std::cout << "数据结构：\n";
//     root->Print(std::cout, 0);
    
//     // 测试嵌套访问
//     Values* nameValue = root->PathValue("person.name");
//     if (nameValue) {
//         std::cout << "person.name访问成功，类型: " << nameValue->TypeName() << std::endl;
//         delete nameValue;
//     } else {
//         std::cout << "person.name访问失败!" << std::endl;
//     }
    
//     Values* firstNameValue = root->PathValue("person.name.first");
//     if (firstNameValue && firstNameValue->IsString()) {
//         std::cout << "person.name.first访问成功: " << firstNameValue->AsString() << std::endl;
//         delete firstNameValue;
//     } else {
//         std::cout << "person.name.first访问失败!" << std::endl;
//         if (firstNameValue) delete firstNameValue;
//     }
    
//     // 测试简单模板
//     std::string simpleTemplate = "{{.person.name.first}}";
//     try {
//         std::string result = ExecuteTemplate("simple", simpleTemplate, root, "{{", "}}", ExecOptions());
//         std::cout << "简单模板结果: \"" << result << "\"" << std::endl;
//     } catch (const std::exception& e) {
//         std::cout << "简单模板失败: " << e.what() << std::endl;
//     }
    
//     // 测试完整模板
//     std::string fullTemplate = "{{.person.name.first}} {{.person.name.last}}";
//     try {
//         std::string result = ExecuteTemplate("full", fullTemplate, root, "{{", "}}", ExecOptions());
//         std::cout << "完整模板结果: \"" << result << "\"" << std::endl;
//     } catch (const std::exception& e) {
//         std::cout << "完整模板失败: " << e.what() << std::endl;
//     }
    
//     // 清理
//     delete root;
//     std::cout << "===== 测试完成 =====\n\n";
// }

// // 测试with功能
// void testWithFeature() {
//     using namespace template_engine;
    
//     std::cout << "\n===== with功能测试 =====\n";
    
//     // 创建嵌套结构
//     Values* root = Values::MakeMap(std::map<std::string, Values*>());
//     Values* person = Values::MakeMap(std::map<std::string, Values*>());
//     Values* name = Values::MakeMap(std::map<std::string, Values*>());
//     Values* address = Values::MakeMap(std::map<std::string, Values*>());
    
//     // 添加name.first和name.last
//     (*name)["first"] = Values::MakeString("John");
//     (*name)["last"] = Values::MakeString("Doe");
    
//     // 添加address字段
//     (*address)["street"] = Values::MakeString("123 Main St");
//     (*address)["city"] = Values::MakeString("Anytown");
//     (*address)["zipcode"] = Values::MakeString("12345");
    
//     // 构建person对象
//     (*person)["name"] = name;
//     (*person)["age"] = Values::MakeNumber(30);
//     (*person)["address"] = address;
    
//     // 添加根级字段
//     (*root)["person"] = person;
    
//     // 测试with的基本用法
//     std::string withTemplate = 
//         "用户信息:\n"
//         "{{with .person}}\n"
//         "  姓名: {{.name.first}} {{.name.last}}\n"
//         "  年龄: {{.age}}\n"
//         "  {{with .address}}\n"
//         "  地址: {{.street}}, {{.city}}, {{.zipcode}}\n"
//         "  {{end}}\n"
//         "{{else}}\n"
//         "  没有用户信息\n"
//         "{{end}}";
    
//     try {
//         std::string result = ExecuteTemplate("withTest", withTemplate, root, "{{", "}}", ExecOptions());
//         std::cout << "with模板结果:\n" << result << std::endl;
//     } catch (const std::exception& e) {
//         std::cout << "with模板测试失败: " << e.what() << std::endl;
//     }
    
//     // 测试with的条件判断功能 - 有效路径
//     std::string withConditionTemplate1 = 
//         "{{with .person.name}}\n"
//         "  姓: {{.last}}\n"
//         "  名: {{.first}}\n"
//         "{{else}}\n"
//         "  没有姓名信息\n"
//         "{{end}}";
    
//     try {
//         std::string result = ExecuteTemplate("withCondition1", withConditionTemplate1, root, "{{", "}}", ExecOptions());
//         std::cout << "\nwith条件测试1结果:\n" << result << std::endl;
//     } catch (const std::exception& e) {
//         std::cout << "with条件测试1失败: " << e.what() << std::endl;
//     }
    
//     // 测试with的条件判断功能 - 无效路径
//     std::string withConditionTemplate2 = 
//         "{{with .person.employment}}\n"
//         "  职位: {{.title}}\n"
//         "  公司: {{.company}}\n"
//         "{{else}}\n"
//         "  没有工作信息\n"
//         "{{end}}";
    
//     try {
//         std::string result = ExecuteTemplate("withCondition2", withConditionTemplate2, root, "{{", "}}", ExecOptions());
//         std::cout << "\nwith条件测试2结果:\n" << result << std::endl;
//     } catch (const std::exception& e) {
//         std::cout << "with条件测试2失败: " << e.what() << std::endl;
//     }
    
//     // 清理
//     delete root;
//     std::cout << "===== with测试完成 =====\n\n";
// }

// // 测试逻辑函数and、or、not
// void testLogicFunctions() {
//     using namespace template_engine;
    
//     std::cout << "\n===== 逻辑函数测试 =====\n";
    
//     // 创建测试数据
//     Values* root = Values::MakeMap(std::map<std::string, Values*>());
//     Values* flags = Values::MakeMap(std::map<std::string, Values*>());
    
//     // 添加测试标志
//     (*flags)["enabled"] = Values::MakeBool(true);
//     (*flags)["visible"] = Values::MakeBool(true);
//     (*flags)["admin"] = Values::MakeBool(false);
//     (*flags)["empty"] = Values::MakeString("");
//     (*flags)["username"] = Values::MakeString("admin");
    
//     // 添加到根对象
//     (*root)["flags"] = flags;
    
//     // 测试not函数
//     std::string notTemplate = 
//         "not测试:\n"
//         "{{if not .flags.admin}}\n"
//         "  不是管理员\n"
//         "{{else}}\n"
//         "  是管理员\n"
//         "{{end}}\n"
//         "{{if not .flags.enabled}}\n"
//         "  未启用\n"
//         "{{else}}\n"
//         "  已启用\n"
//         "{{end}}\n"
//         "{{if not .flags.empty}}\n"
//         "  empty字段不为空\n"
//         "{{else}}\n"
//         "  empty字段为空\n"
//         "{{end}}";
    
//     try {
//         std::string result = ExecuteTemplate("notTest", notTemplate, root, "{{", "}}", ExecOptions());
//         std::cout << "not函数测试结果:\n" << result << std::endl;
//     } catch (const std::exception& e) {
//         std::cout << "not函数测试失败: " << e.what() << std::endl;
//     }
    
//     // 测试and函数
//     std::string andTemplate = 
//         "and测试:\n"
//         "{{if and .flags.enabled .flags.visible}}\n"
//         "  启用且可见\n"
//         "{{else}}\n"
//         "  不满足条件\n"
//         "{{end}}\n"
//         "{{if and .flags.enabled .flags.admin}}\n"
//         "  启用且是管理员\n"
//         "{{else}}\n"
//         "  不是启用的管理员\n"
//         "{{end}}";
    
//     try {
//         std::string result = ExecuteTemplate("andTest", andTemplate, root, "{{", "}}", ExecOptions());
//         std::cout << "\nand函数测试结果:\n" << result << std::endl;
//     } catch (const std::exception& e) {
//         std::cout << "and函数测试失败: " << e.what() << std::endl;
//     }
    
//     // 测试or函数
//     std::string orTemplate = 
//         "or测试:\n"
//         "{{if or .flags.admin .flags.enabled}}\n"
//         "  是管理员或已启用\n"
//         "{{else}}\n"
//         "  既不是管理员也未启用\n"
//         "{{end}}\n"
//         "{{if or .flags.admin .flags.empty}}\n"
//         "  是管理员或empty为空\n"
//         "{{else}}\n"
//         "  不是管理员且empty不为空\n"
//         "{{end}}";
    
//     try {
//         std::string result = ExecuteTemplate("orTest", orTemplate, root, "{{", "}}", ExecOptions());
//         std::cout << "\nor函数测试结果:\n" << result << std::endl;
//     } catch (const std::exception& e) {
//         std::cout << "or函数测试失败: " << e.what() << std::endl;
//     }
    
//     // 测试嵌套逻辑函数
//     std::string nestedTemplate = 
//         "嵌套逻辑函数测试:\n"
//         "{{if and (not .flags.admin) .flags.enabled}}\n"
//         "  非管理员且已启用\n"
//         "{{end}}\n"
//         "{{if or (and .flags.enabled .flags.visible) .flags.admin}}\n"
//         "  (启用且可见)或是管理员\n"
//         "{{end}}\n"
//         "{{if and (or .flags.admin .flags.enabled) (not .flags.empty)}}\n"
//         "  (管理员或启用)且empty不为空\n"
//         "{{end}}\n"
//         "{{if not (and .flags.admin (not .flags.enabled))}}\n"
//         "  不满足(管理员且未启用)条件\n"
//         "{{end}}";
    
//     try {
//         std::string result = ExecuteTemplate("nestedTest", nestedTemplate, root, "{{", "}}", ExecOptions());
//         std::cout << "\n嵌套逻辑函数测试结果:\n" << result << std::endl;
//     } catch (const std::exception& e) {
//         std::cout << "嵌套逻辑函数测试失败: " << e.what() << std::endl;
//     }
    
//     // 清理
//     delete root;
//     std::cout << "===== 逻辑函数测试完成 =====\n\n";
// }

// void testPipelineOrder() {
//     using namespace template_engine;
//     std::cout << "\n===== 管道参数顺序测试 =====\n";
//     Values* root = Values::MakeMap(std::map<std::string, Values*>());
//     Values* flags = Values::MakeMap(std::map<std::string, Values*>());
//     (*flags)["enabled"] = Values::MakeBool(true);
//     (*flags)["admin"] = Values::MakeBool(false);
//     (*flags)["name"] = Values::MakeString("testuser");
//     (*root)["flags"] = flags;

//     // eq管道测试，左值应为第一个参数
//     std::string eqPipe = "{{ .flags.name | eq \"testuser\" }}";
//     try {
//         std::string result = ExecuteTemplate("eqPipeTest", eqPipe, root, "{{", "}}", ExecOptions());
//         std::cout << "eq管道测试结果: " << result << std::endl;
//     } catch (const std::exception& e) {
//         std::cout << "eq管道测试失败: " << e.what() << std::endl;
//     }

//     // and管道测试
//     std::string andPipe = "{{ .flags.enabled | and .flags.admin }}";
//     try {
//         std::string result = ExecuteTemplate("andPipeTest", andPipe, root, "{{", "}}", ExecOptions());
//         std::cout << "and管道测试结果: " << result << std::endl;
//     } catch (const std::exception& e) {
//         std::cout << "and管道测试失败: " << e.what() << std::endl;
//     }

//     // or管道测试
//     std::string orPipe = "{{ .flags.admin | or .flags.enabled }}";
//     try {
//         std::string result = ExecuteTemplate("orPipeTest", orPipe, root, "{{", "}}", ExecOptions());
//         std::cout << "or管道测试结果: " << result << std::endl;
//     } catch (const std::exception& e) {
//         std::cout << "or管道测试失败: " << e.what() << std::endl;
//     }

//     // not管道测试
//     std::string notPipe = "{{ .flags.admin | not }}";
//     try {
//         std::string result = ExecuteTemplate("notPipeTest", notPipe, root, "{{", "}}", ExecOptions());
//         std::cout << "not管道测试结果: " << result << std::endl;
//     } catch (const std::exception& e) {
//         std::cout << "not管道测试失败: " << e.what() << std::endl;
//     }

//     // 多级管道测试
//     std::string multiPipe = "{{ .flags.admin | not | and .flags.enabled }}";
//     try {
//         std::string result = ExecuteTemplate("multiPipeTest", multiPipe, root, "{{", "}}", ExecOptions());
//         std::cout << "多级管道测试结果: " << result << std::endl;
//     } catch (const std::exception& e) {
//         std::cout << "多级管道测试失败: " << e.what() << std::endl;
//     }

//     // default函数管道测试
//     std::cout << "\n-- default函数管道测试 --\n";
//     // 1. 字符串字段为空时兜底为\"tea\"
//     std::string defaultStrEmpty = "{{ .flags.nodrink | default \"tea\" }}";
//     try {
//         std::string result = ExecuteTemplate("defaultStrEmpty", defaultStrEmpty, root, "{{", "}}", ExecOptions());
//         std::cout << "nodrink为空时default: " << result << std::endl;
//     } catch (const std::exception& e) {
//         std::cout << "defaultStrEmpty测试失败: " << e.what() << std::endl;
//     }
//     // 2. 字符串字段有值时不兜底
//     std::string defaultStrVal = "{{ .flags.name | default \"tea\" }}";
//     try {
//         std::string result = ExecuteTemplate("defaultStrVal", defaultStrVal, root, "{{", "}}", ExecOptions());
//         std::cout << "name有值时default: " << result << std::endl;
//     } catch (const std::exception& e) {
//         std::cout << "defaultStrVal测试失败: " << e.what() << std::endl;
//     }
//     // 3. 单参数default（兜底为null）
//     std::string defaultSingle = "{{ .flags.nodrink | default }}";
//     try {
//         std::string result = ExecuteTemplate("defaultSingle", defaultSingle, root, "{{", "}}", ExecOptions());
//         std::cout << "单参数default: [" << result << "]" << std::endl;
//     } catch (const std::exception& e) {
//         std::cout << "defaultSingle测试失败: " << e.what() << std::endl;
//     }

//     delete root;
//     std::cout << "===== 管道参数顺序测试完成 =====\n\n";
// }

// int main() {
//     // 添加逻辑函数测试调用
//     testLogicFunctions();
    
//     // 添加with功能测试调用
//     // testWithFeature();

//     // testNestedFieldAccess();

//     // 创建模板渲染数据
//     Values* chartValues = Values::MakeMap(std::map<std::string, Values*>());
//     Values* nameValue = Values::MakeMap(std::map<std::string, Values*>());
    
//     // 使用operator[]添加元素
//     (*nameValue)["first"] = Values::MakeString("John.0");
//     (*nameValue)["last"] = Values::MakeString("Doe");
    
//     // 将nameValue连接到chartValues
//     (*chartValues)["Name"] = nameValue;
    
//     // 创建一个items列表
//     Values* items = Values::MakeList(std::vector<Values*>());
    
//     // 创建并添加item1
//     Values* item1 = Values::MakeMap(std::map<std::string, Values*>());
//     (*item1)["name"] = Values::MakeString("item1");
//     (*item1)["value"] = Values::MakeNumber(42);
    
//     // 注意：AsList()返回const引用，所以不能直接listValue_.push_back
//     std::vector<Values*> tempList;
//     tempList.push_back(item1);
    
//     // 创建并添加item2
//     Values* item2 = Values::MakeMap(std::map<std::string, Values*>());
//     (*item2)["name"] = Values::MakeString("item2");
//     (*item2)["value"] = Values::MakeNumber(100);
    
//     tempList.push_back(item2);
    
//     // 使用传入初始vector的MakeList创建的items
//     Values* newItems = Values::MakeList(tempList);
//     delete items; // 释放旧items
//     items = newItems;
    
//     // 将items连接到chartValues
//     (*chartValues)["Items"] = items;

//     // 打印数据结构
//     std::cout << "Data structure:" << std::endl;
//     chartValues->Print(std::cout, 0);
    
//     // 测试通过路径方式访问嵌套值
//     try {
//         Values* firstNameValue = chartValues->PathValue("Name.first");
//         if (firstNameValue != NULL) {
//             std::cout << "Successfully accessed Name.first: " 
//                      << firstNameValue->AsString() << std::endl;
//             delete firstNameValue; // 释放资源
//         } else {
//             std::cout << "Failed to access Name.first" << std::endl;
//         }
//     } catch (const std::exception& e) {
//         std::cout << "Error accessing Name.first: " << e.what() << std::endl;
//     }

//     // 深度检查数据结构中的字段
//     std::cout << "\n=== 数据结构验证 ===\n";

//     // 打印顶级键
//     std::cout << "顶级键: ";
//     if (chartValues->IsMap()) {
//         const std::map<std::string, Values*>& topMap = chartValues->AsMap();
//         std::map<std::string, Values*>::const_iterator it;
//         for (it = topMap.begin(); it != topMap.end(); ++it) {
//             std::cout << it->first << " ";
//         }
//         std::cout << std::endl;

//         // 验证 Name 字段
//         std::map<std::string, Values*>::const_iterator nameIt = topMap.find("Name");
//         if (nameIt != topMap.end() && nameIt->second != NULL) {
//             const Values* nameValue = nameIt->second;
//             if (nameValue->IsMap()) {
//                 std::cout << "Name 字段是一个 map，包含键: ";
//                 const std::map<std::string, Values*>& nameMap = nameValue->AsMap();
//                 std::map<std::string, Values*>::const_iterator nameMapIt;
//                 for (nameMapIt = nameMap.begin(); nameMapIt != nameMap.end(); ++nameMapIt) {
//                     std::cout << nameMapIt->first << " ";
//                 }
//                 std::cout << std::endl;
                
//                 // 验证 Name.first
//                 std::map<std::string, Values*>::const_iterator firstIt = nameMap.find("first");
//                 if (firstIt != nameMap.end() && firstIt->second != NULL) {
//                     const Values* firstValue = firstIt->second;
//                     if (firstValue->IsString()) {
//                         std::cout << "Name.first = " << firstValue->AsString() << std::endl;
//                     } else {
//                         std::cout << "Name.first 不是字符串类型！" << std::endl;
//                     }
//                 } else {
//                     std::cout << "Name.first 不存在或为空！" << std::endl;
//                 }
                
//                 // 验证 Name.last
//                 std::map<std::string, Values*>::const_iterator lastIt = nameMap.find("last");
//                 if (lastIt != nameMap.end() && lastIt->second != NULL) {
//                     const Values* lastValue = lastIt->second;
//                     if (lastValue->IsString()) {
//                         std::cout << "Name.last = " << lastValue->AsString() << std::endl;
//                     } else {
//                         std::cout << "Name.last 不是字符串类型！" << std::endl;
//                     }
//                 } else {
//                     std::cout << "Name.last 不存在或为空！" << std::endl;
//                 }
//             } else {
//                 std::cout << "Name 字段不是 map！" << std::endl;
//             }
//         } else {
//             std::cout << "Name 字段不存在或为空！" << std::endl;
//         }

//         // 验证 Items 字段
//         std::map<std::string, Values*>::const_iterator itemsIt = topMap.find("Items");
//         if (itemsIt != topMap.end() && itemsIt->second != NULL) {
//             const Values* itemsValue = itemsIt->second;
//             if (itemsValue->IsList()) {
//                 const std::vector<Values*>& itemsList = itemsValue->AsList();
//                 std::cout << "Items 字段是一个列表，包含 " << itemsList.size() << " 个项目" << std::endl;
                
//                 // 检查第一个项目
//                 if (!itemsList.empty() && itemsList[0] != NULL) {
//                     const Values* firstItemPtr = itemsList[0];
//                     if (firstItemPtr->IsMap()) {
//                         std::cout << "第一个项目包含键: ";
//                         const std::map<std::string, Values*>& firstItemMap = firstItemPtr->AsMap();
//                         std::map<std::string, Values*>::const_iterator itemMapIt;
//                         for (itemMapIt = firstItemMap.begin(); itemMapIt != firstItemMap.end(); ++itemMapIt) {
//                             std::cout << itemMapIt->first << " ";
//                         }
//                         std::cout << std::endl;
//                     } else {
//                         std::cout << "第一个项目不是 map！" << std::endl;
//                     }
//                 } else {
//                     std::cout << "Items 列表为空或第一项为空！" << std::endl;
//                 }
//             } else {
//                 std::cout << "Items 字段不是列表！" << std::endl;
//             }
//         } else {
//             std::cout << "Items 字段不存在或为空！" << std::endl;
//         }
//     } else {
//         std::cout << "chartValues 不是一个 map！" << std::endl;
//     }


    
//     std::cout << "=== 验证完成 ===\n\n";

    
//     std::string templateStr = 
//         "Hello {{ .Name.first }} {{ .Name.last }}!\n"
//         "\n"
//         "{{ if .Name.last }}\n"
//         "Items:\n"
//         "{{ range .Items }}\n"
//         "  - {{ .name }}: {{ .value }}\n"
//         "{{ end }}\n"
//         "{{ else }}\n"
//         "No items available.\n"
//         "{{ end }}\n"
//         "\n"
//         "{{ if gt .Name.first \"John\" }}\n"
//         "Welcome back, John!\n"
//         "{{ end }}\n";

//     try {
//         // 执行模板
//         std::string result = ExecuteTemplate("example", templateStr, chartValues, "{{", "}}", ExecOptions());
        
//         // 输出结果
//         std::cout << "Template execution result:\n" << result << std::endl;
//     } catch (const ExecError& e) {
//         std::cerr << "Template execution error: " << e.what() << std::endl;
//     } catch (const std::exception& e) {
//         std::cerr << "Error: " << e.what() << std::endl;
//     }
    
//     // 清理资源（Values对象是手动内存管理，所以需要手动删除）
//     delete chartValues;
    
//     // 添加管道参数顺序测试
//     testPipelineOrder();
    
//     return 0;
// }