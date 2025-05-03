#ifndef TEMPLATE_LINTER_H
#define TEMPLATE_LINTER_H

#include <string>
#include <vector>
#include <map>
#include "template_syntax_checker.h" // 需要 TemplateSyntaxError 定义

namespace template_linter {

/**
 * @brief 对给定 Chart 目录中的所有有效模板文件 (.yaml/.yml) 进行语法检查。
 *
 * 它会检查语法错误、结构问题，并通过解析 Chart 的 values.yaml 来检查可选的数据兼容性问题。
 *
 * @param chartPath 要检查的 Chart 目录的根路径。
 * @param lintErrors 输出参数。一个 map，键是模板文件的相对路径（例如，"templates/deployment.yaml"），值是在该文件中找到的 TemplateSyntaxError 的向量。
 *                   该 map 将只包含有错误的文件条目。
 * @return 如果在任何模板中都没有发现错误，则返回 true，否则返回 false。
 */
bool LintChartTemplates(
    const std::string& chartPath,
    std::map<std::string, std::vector<template_engine::TemplateSyntaxError> >& lintErrors
);

} // namespace template_linter

#endif // TEMPLATE_LINTER_H 