#ifndef CHART_PROCESSOR_H
#define CHART_PROCESSOR_H

#include <string>
#include <vector>
#include <map>

namespace chart_processor {

/**
 * @brief 处理给定 Helm Chart 目录中的所有模板文件。
 *
 * 该函数会解析 Chart 的 values.yaml 文件，然后遍历 templates/ 目录，
 * 对每个符合条件的模板文件（非下划线开头的文件）进行渲染。
 *
 * @param chartPath Chart 根目录的路径。
 * @param renderedResults 输出参数，用于存储每个模板文件渲染后的结果。
 *                      键是模板文件的相对路径 (e.g., "templates/deployment.yaml")，
 *                      值是渲染后的字符串内容。
 * @param errors 输出参数，用于存储处理过程中遇到的错误信息。
 * @return true 如果处理过程没有发生严重错误（例如 values.yaml 解析失败），否则返回 false。
 *         注意：即使返回 true，errors 中也可能包含个别模板渲染失败的信息。
 */
bool ProcessChartTemplates(
    const std::string& chartPath,
    std::map<std::string, std::string>& renderedResults,
    std::vector<std::string>& errors);

} // namespace chart_processor

#endif // CHART_PROCESSOR_H 