#include "chart_processor.h" 
#include "template_linter.h"   
#include "template_syntax_checker.h" 
#include <iostream>          
#include <map>               
#include <vector>            
#include <string>            

// --- Chart 渲染 ---
bool runChartProcessing(const std::string& chartPath) {
    std::cout << "\n=== 开始测试 Chart 渲染: " << chartPath << " ===" << std::endl;

    // 用于存储渲染结果和错误的容器
    std::map<std::string, std::string> renderedOutputs;
    std::vector<std::string> processingErrors;

    // 调用 Chart 渲染函数
    bool success = chart_processor::ProcessChartTemplates(
        chartPath,
        renderedOutputs,
        processingErrors
    );

    // 打印渲染结果
    std::cout << "\n--- Chart 渲染结果 ---" << std::endl;
    if (success) {
        std::cout << "Chart 渲染流程完成" << std::endl;
    } else {
        std::cerr << "Chart 渲染过程中发生严重错误！" << std::endl;
    }

    // 打印渲染的模板内容
    std::cout << "\n--- 渲染的模板 --- (" << renderedOutputs.size() << " 个文件)" << std::endl;
    std::map<std::string, std::string>::const_iterator it_render;
    for (it_render = renderedOutputs.begin(); it_render != renderedOutputs.end(); ++it_render) {
        std::cout << "\n--- 文件: " << it_render->first << " ---\n";
        std::cout << it_render->second; // 渲染结果
        std::cout << "\n--- 文件结束: " << it_render->first << " ---\n";
    }

    // 打印渲染过程中的错误或警告
    if (!processingErrors.empty()) {
        std::cerr << "\n--- Chart 渲染错误/警告 --- (" << processingErrors.size() << " 条)" << std::endl;
        for (size_t i = 0; i < processingErrors.size(); ++i) {
            std::cerr << "- " << processingErrors[i] << std::endl;
        }
    } else {
        std::cout << "\n--- Chart 渲染无错误/警告 ---" << std::endl;
    }

     // 确定是否有严重错误
     bool hadFatalError = !success;
     for (size_t i = 0; i < processingErrors.size(); ++i) {
         if (processingErrors[i].rfind("错误:", 0) == 0) {
             hadFatalError = true;
             break;
         }
     }
     std::cout << "\nChart 渲染测试结束。" << (hadFatalError ? "状态：失败" : "状态：成功") << std::endl;
    return !hadFatalError; // 返回成功状态
}

// --- Chart语法检查 ---
bool runTemplateLinting(const std::string& chartPath) {
    std::cout << "\n=== 开始Chart语法校验: " << chartPath << " ===" << std::endl;

    std::map<std::string, std::vector<template_engine::TemplateSyntaxError> > lintErrors;

    bool success = template_linter::LintChartTemplates(chartPath, lintErrors);

    // 打印结果
    std::cout << "\n--- Chart包校验结果 ---" << std::endl;
    if (success) {
        std::cout << "No syntax errors found in templates." << std::endl;
    } else {
        std::cerr << "Syntax errors found in the following templates:" << std::endl;
        std::map<std::string, std::vector<template_engine::TemplateSyntaxError> >::const_iterator it_lint;
        for (it_lint = lintErrors.begin(); it_lint != lintErrors.end(); ++it_lint) {
            const std::string& templatePath = it_lint->first;
            const std::vector<template_engine::TemplateSyntaxError>& errors = it_lint->second;

            std::cerr << "\n=== Errors in: " << templatePath << " ===" << std::endl;
            for (size_t i = 0; i < errors.size(); ++i) {
                std::cerr << "- Line " << errors[i].line << ": " << errors[i].message << std::endl;
                 if (!errors[i].context.empty()) {
                     std::cerr << "    Context: " << errors[i].context << std::endl;
                 }
            }
        }
    }
    std::cout << "\n Chart包校验结束。" << (success ? "状态：成功" : "状态：失败") << std::endl;
    return success;
}

int main() {
    // --- 配置测试 Chart 路径 ---
    // 请将此路径修改为您要测试的 Chart 路径
    std::string chartToTestPath = "C:\\Users\\oykk\\Desktop\\project\\myapp\\myapp";
    // std::string chartToTestPath = "SingleTemplateAndValues"; // 备用路径
    // ---------------------------

    std::cout << "********** 开始 **********" << std::endl;

    // Chart 语法检查测试
    bool lintSuccess = runTemplateLinting(chartToTestPath);
    std::cout << "Chart 语法校验最终状态: " << (lintSuccess ? "成功" : "失败") << std::endl;


    // Chart 渲染测试
    bool processSuccess = runChartProcessing(chartToTestPath);
    std::cout << "Chart 渲染最终状态: " << (processSuccess ? "成功" : "失败") << std::endl;


    std::cout << "\n********** 结束 **********" << std::endl;
    

    // return lintSuccess ? 0 : 1;
    // return processSuccess ? 0 : 1;
    return (lintSuccess && processSuccess) ? 0 : 1;


} 