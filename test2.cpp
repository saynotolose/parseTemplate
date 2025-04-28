// main.cpp
#include "values.h"
#include "exec.h"
#include <iostream>
#include <string>

using namespace template_engine;

int main() {
    // 创建渲染数据
    auto chartValues = Values::MakeMap();
    auto nameValue = Values::MakeMap();
    nameValue->AsMap()["first"] = Values::MakeString("John");
    nameValue->AsMap()["last"] = Values::MakeString("Doe");
    chartValues->AsMap()["Name"] = nameValue;
    
    // 创建一个items列表
    auto items = Values::MakeList();
    auto item1 = Values::MakeMap();
    item1->AsMap()["name"] = Values::MakeString("item1");
    item1->AsMap()["value"] = Values::MakeNumber(42);
    items->AsList().push_back(item1);
    
    auto item2 = Values::MakeMap();
    item2->AsMap()["name"] = Values::MakeString("item2");
    item2->AsMap()["value"] = Values::MakeNumber(100);
    items->AsList().push_back(item2);
    
    chartValues->AsMap()["Items"] = items;




    // 打印数据结构
    std::cout << "Data structure:" << std::endl;
    chartValues->Print(std::cout, 0);
    
    // 检查能否通过编程方式访问嵌套值
    try {
        auto firstNameValue = chartValues->PathValue("Name.first");
        if (firstNameValue && *firstNameValue) {
            std::cout << "Successfully accessed Name.first: " 
                     << (*firstNameValue)->AsString() << std::endl;
        } else {
            std::cout << "Failed to access Name.first" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Error accessing Name.first: " << e.what() << std::endl;
    }


    // 在创建数据后和执行模板前
    std::cout << "\n=== 数据结构验证 ===\n";

    // 打印顶层键
    std::cout << "顶层键: ";
    for (const auto& [key, _] : chartValues->AsMap()) {
        std::cout << key << " ";
    }
    std::cout << std::endl;

    // 验证 Name 字段
    nameValue = chartValues->AsMap()["Name"];
    if (nameValue && nameValue->IsMap()) {
        std::cout << "Name 字段是一个 map，包含键: ";
        for (const auto& [key, _] : nameValue->AsMap()) {
            std::cout << key << " ";
        }
        std::cout << std::endl;
        
        // 验证 Name.first
        auto firstValue = nameValue->AsMap()["first"];
        if (firstValue) {
            std::cout << "Name.first = " << firstValue->AsString() << std::endl;
        } else {
            std::cout << "Name.first 不存在！" << std::endl;
        }
        
        // 验证 Name.last
        auto lastValue = nameValue->AsMap()["last"];
        if (lastValue) {
            std::cout << "Name.last = " << lastValue->AsString() << std::endl;
        } else {
            std::cout << "Name.last 不存在！" << std::endl;
        }
    } else {
        std::cout << "Name 字段不是 map 或不存在！" << std::endl;
    }

    // 验证 Items 字段
    auto itemsValue = chartValues->AsMap()["Items"];
    if (itemsValue && itemsValue->IsList()) {
        std::cout << "Items 字段是一个列表，包含 " << itemsValue->AsList().size() << " 个项目" << std::endl;
        
        // 检查第一个项目
        if (!itemsValue->AsList().empty()) {
            auto firstItem = itemsValue->AsList()[0];
            if (firstItem && firstItem->IsMap()) {
                std::cout << "第一个项目包含键: ";
                for (const auto& [key, _] : firstItem->AsMap()) {
                    std::cout << key << " ";
                }
                std::cout << std::endl;
            }
        }
    } else {
        std::cout << "Items 字段不是列表或不存在！" << std::endl;
    }

    std::cout << "=== 验证结束 ===\n\n";






    
    // {{ if gt (len .Items) 0 }}  {{ if eq .Name.first John }} {{ if .Name.last }} {{ if .Name.first }} {{ if gt len .Items 0 }}

    // 定义模板字符串
    std::string templateStr = R"(
Hello {{ .Name.first }} {{ .Name.last }}!

{{ if .Name.last }}
Items:
{{ range .Items }}
  - {{ .name }}: {{ .value }}
{{ end }}
{{ else }}
No items available.
{{ end }}

{{ if eq .Name.first "John" }}
Welcome back, John!
{{ end }}
)";

    try {
        // 执行模板
        std::string result = ExecuteTemplate(
            "example", templateStr, chartValues, "{{", "}}", ExecOptions{});
        
        // 输出结果
        std::cout << "Template execution result:\n" << result << std::endl;
    } catch (const ExecError& e) {
        std::cerr << "Template execution error: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    
    return 0;
}