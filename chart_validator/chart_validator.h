#ifndef CHART_VALIDATOR_H
#define CHART_VALIDATOR_H

#include <string>
#include <vector>

namespace chart_util {

/**
 * @brief 检查给定的路径是否代表一个结构有效的 Helm Chart (C++98, POSIX)。
 *
 * @param chartPath Chart 根目录的路径。
 * @param errors 输出参数，用于存储发现的结构错误信息。
 * @return true 如果 Chart 结构有效，否则返回 false。
 */
bool ValidateChartStructure(const std::string& chartPath, std::vector<std::string>& errors);

} // namespace chart_util

#endif // CHART_VALIDATOR_H 