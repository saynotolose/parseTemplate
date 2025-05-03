#include "template_linter.h"
#include "values.h" 
#include "template_syntax_checker.h" 
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <vector>
#include <map>

// 辅助函数检查路径类型 (从 chart_processor 复制并重命名)
static int getPathType_Linter(const std::string& path) {
    struct stat path_stat;
    if (stat(path.c_str(), &path_stat) != 0) {
        return 0;
    }
    if (S_ISREG(path_stat.st_mode)) {
        return 1; // 文件
    }
    if (S_ISDIR(path_stat.st_mode)) {
        return 2; // 目录
    }
    return 0;
}

namespace template_linter {

bool LintChartTemplates(
    const std::string& chartPath,
    std::map<std::string, std::vector<template_engine::TemplateSyntaxError> >& lintErrors) {

    lintErrors.clear();
    bool chartHasErrors = false;

    // 1. 解析 values.yaml (用于数据兼容性检查)
    std::string valuesPath = chartPath + "/values.yaml";
    template_engine::Values* values = NULL;
    std::string valuesYamlStr = "";
    try {
        if (getPathType_Linter(valuesPath) == 1) {
            values = template_engine::ParseSimpleYAMLFile(valuesPath);
            if (values) {
                valuesYamlStr = values->ToYAML(); // 序列化以供检查器使用
            }
        } else {
            // values 文件缺失本身不是一个语法错误，只是在没有数据检查的情况下继续
            values = template_engine::Values::MakeMap(std::map<std::string, template_engine::Values*>());
            valuesYamlStr = "";
        }
    } catch (const std::exception& e) {
        // 将 values 解析错误报告为 Chart 级别的错误
        std::vector<template_engine::TemplateSyntaxError> valuesErrors;
        template_engine::TemplateSyntaxError err;
        err.type = template_engine::ErrorType_Syntax; // 使用通用的语法错误类型
        err.line = 1; // 表明错误与文件本身有关
        err.message = "Failed to parse 'values.yaml': " + std::string(e.what());
        valuesErrors.push_back(err);
        lintErrors["values.yaml"] = valuesErrors;
        chartHasErrors = true;
        // 即使 values 解析失败，也继续检查模板，只是不进行数据检查
        if (!values) { // 如果解析抛出异常，确保 values 至少是一个空 map
            values = template_engine::Values::MakeMap(std::map<std::string, template_engine::Values*>());
            valuesYamlStr = "";
        }
    }

    // 2. 遍历 templates/ 目录
    std::string templatesPath = chartPath + "/templates";
    if (getPathType_Linter(templatesPath) != 2) {
        // templates/ 目录缺失也是一个 Chart 级别的错误
        std::vector<template_engine::TemplateSyntaxError> dirErrors;
        template_engine::TemplateSyntaxError err;
        err.type = template_engine::ErrorType_Syntax;
        err.line = 1;
        err.message = "'templates/' directory not found or is not a directory.";
        dirErrors.push_back(err);
        lintErrors["templates/"] = dirErrors;
        delete values; // 清理 values 对象
        return false; // 没有 templates 目录无法继续
    }

    DIR* dir = opendir(templatesPath.c_str());
    if (!dir) {
        // 打开目录时出错
        std::vector<template_engine::TemplateSyntaxError> dirErrors;
        template_engine::TemplateSyntaxError err;
        err.type = template_engine::ErrorType_Syntax;
        err.line = 1;
        err.message = "Could not open 'templates/' directory: " + std::string(strerror(errno));
        dirErrors.push_back(err);
        lintErrors["templates/"] = dirErrors;
        delete values;
        return false;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string entryName = entry->d_name;
        if (entryName == "." || entryName == "..") continue;
        if (!entryName.empty() && entryName[0] == '_') continue; // 跳过辅助模板文件 (_*)

        std::string fullEntryPath = templatesPath + "/" + entryName;
        std::string relativeTemplatePath = "templates/" + entryName;

        // 只处理文件
        if (getPathType_Linter(fullEntryPath) == 1) {
            // 检查扩展名
            size_t nameLen = entryName.length();
            bool isYaml = (nameLen > 5 && entryName.substr(nameLen - 5) == ".yaml") ||
                          (nameLen > 4 && entryName.substr(nameLen - 4) == ".yml");

            if (isYaml) { // 只检查 YAML 文件
                // 读取模板内容
                std::ifstream tplFile(fullEntryPath.c_str());
                if (!tplFile.is_open()) {
                    std::vector<template_engine::TemplateSyntaxError> fileErrors;
                    template_engine::TemplateSyntaxError err;
                    err.type = template_engine::ErrorType_Syntax;
                    err.line = 1;
                    err.message = "Could not open template file.";
                    fileErrors.push_back(err);
                    lintErrors[relativeTemplatePath] = fileErrors;
                    chartHasErrors = true;
                    continue;
                }
                std::stringstream buffer;
                buffer << tplFile.rdbuf();
                std::string templateContent = buffer.str();
                tplFile.close();

                // 检查模板内容
                std::vector<template_engine::TemplateSyntaxError> currentTemplateErrors;
                bool syntaxOk = template_engine::CheckTemplateSyntax(
                    relativeTemplatePath,
                    templateContent,
                    "{{", // 左定界符
                    "}}", // 右定界符
                    valuesYamlStr,
                    currentTemplateErrors);

                if (!syntaxOk && !currentTemplateErrors.empty()) {
                    lintErrors[relativeTemplatePath] = currentTemplateErrors;
                    chartHasErrors = true;
                }
            }
        }
    }
    closedir(dir);

    // 清理已解析的 values 对象
    delete values;

    return !chartHasErrors;
}

} // namespace template_linter 