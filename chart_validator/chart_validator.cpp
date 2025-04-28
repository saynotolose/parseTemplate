#include "chart_validator.h"
#include <sys/stat.h> // For stat()
#include <dirent.h>   // For opendir, readdir, closedir
#include <errno.h>    // For errno
#include <string.h>   // For strerror()
#include <unistd.h>   // For S_ISREG, S_ISDIR (often included by sys/stat.h)

// 辅助函数，检查路径类型
// 返回值: 0 = 不存在或错误, 1 = 文件, 2 = 目录
static int getPathType(const std::string& path) {
    struct stat path_stat;
    if (stat(path.c_str(), &path_stat) != 0) {
        // 文件不存在或发生其他错误
        // 对于某些错误如 EACCES (无权限) 也可能需要处理，这里简化为不存在
        return 0;
    }
    if (S_ISREG(path_stat.st_mode)) {
        return 1; // 是普通文件
    }
    if (S_ISDIR(path_stat.st_mode)) {
        return 2; // 是目录
    }
    return 0; // 其他类型（符号链接、设备文件等）
}

namespace chart_util {

bool ValidateChartStructure(const std::string& chartPath, std::vector<std::string>& errors) {
    bool isValid = true; // 先假设有效

    // 1. 检查根路径
    if (getPathType(chartPath) != 2) {
        errors.push_back("错误: Chart 根路径 '" + chartPath + "' 不存在或不是一个目录。");
        return false; // 基本要求不满足
    }

    // 2. 检查必需文件/目录
    std::string chartYamlPath = chartPath + "/Chart.yaml";
    if (getPathType(chartYamlPath) != 1) {
        errors.push_back("错误: 必需文件 'Chart.yaml' 在 '" + chartPath + "' 中未找到或不是一个普通文件。");
        isValid = false;
    }

    std::string valuesYamlPath = chartPath + "/values.yaml";
    if (getPathType(valuesYamlPath) != 1) {
        errors.push_back("错误: 必需文件 'values.yaml' 在 '" + chartPath + "' 中未找到或不是一个普通文件。");
        isValid = false;
    }

    std::string templatesPath = chartPath + "/templates";
    if (getPathType(templatesPath) != 2) {
        errors.push_back("错误: 必需目录 'templates' 在 '" + chartPath + "' 中未找到或不是一个目录。");
        isValid = false;
    } else {
        // 新增检查：templates 目录必须包含至少一个 .yaml 或 .yml 文件
        bool foundYaml = false;
        DIR* tplDir = opendir(templatesPath.c_str());
        if (!tplDir) {
            errors.push_back("错误: 无法打开 'templates' 目录 '" + templatesPath + "': " + strerror(errno));
            isValid = false;
        } else {
            struct dirent* tplEntry;
            while ((tplEntry = readdir(tplDir)) != NULL) {
                std::string entryName = tplEntry->d_name;
                std::string fullEntryPath = templatesPath + "/" + entryName;

                // 检查是否是普通文件
                if (getPathType(fullEntryPath) == 1) {
                    // 检查文件扩展名
                    size_t dotPos = entryName.rfind('.');
                    if (dotPos != std::string::npos) {
                        std::string extension = entryName.substr(dotPos);
                        if (extension == ".yaml" || extension == ".yml") {
                            foundYaml = true;
                            break; // 找到一个就足够了
                        }
                    }
                }
            }
            closedir(tplDir);

            if (!foundYaml) {
                errors.push_back("错误: 'templates' 目录 '" + templatesPath + "' 中必须包含至少一个 .yaml 或 .yml 文件。");
                isValid = false;
            }
        }
    }

    // 如果基础检查已失败，无需继续检查可选部分
    if (!isValid) {
        return false;
    }

    // 3. 检查可选 'charts/' 目录
    std::string chartsPath = chartPath + "/charts";
    int chartsPathType = getPathType(chartsPath);
    if (chartsPathType != 0) { // 'charts' 目录存在
        if (chartsPathType != 2) { // 但不是目录
            errors.push_back("错误: 'charts' 在 '" + chartPath + "' 中存在但不是一个目录。");
            isValid = false;
        } else { // 是目录，需要遍历并递归检查子目录
            DIR* dir = opendir(chartsPath.c_str());
            if (!dir) {
                errors.push_back("错误: 无法打开 'charts' 目录 '" + chartsPath + "': " + strerror(errno));
                isValid = false;
            } else {
                struct dirent* entry;
                while ((entry = readdir(dir)) != NULL) {
                    std::string entryName = entry->d_name;
                    // 跳过 '.' 和 '..'
                    if (entryName == "." || entryName == "..") {
                        continue;
                    }

                    std::string subChartPath = chartsPath + "/" + entryName;
                    int subEntryType = getPathType(subChartPath);

                    // 只对子目录（未打包的 chart）进行递归检查
                    if (subEntryType == 2) {
                        std::vector<std::string> subErrors;
                        if (!ValidateChartStructure(subChartPath, subErrors)) {
                            errors.push_back("错误: 子 Chart '" + subChartPath + "' 结构无效:");
                            // 将子错误信息附加到当前错误列表
                            errors.insert(errors.end(), subErrors.begin(), subErrors.end());
                            isValid = false;
                            // 可以选择在这里 break，发现一个无效子chart就停止，或者继续检查其他子chart
                        }
                    }
                    // 忽略普通文件 (如 .tgz) 和其他类型
                }
                closedir(dir);
            }
        }
    }
     // 如果 'charts' 目录不存在，则忽略，是可选的

    // 如果检查 charts 时失败，提前返回
     if (!isValid) {
         return false;
     }

    // 4. 检查可选 'crds/' 目录
    std::string crdsPath = chartPath + "/crds";
    int crdsPathType = getPathType(crdsPath);
     if (crdsPathType != 0) { // 'crds' 目录存在
         if (crdsPathType != 2) { // 但不是目录
             errors.push_back("错误: 'crds' 在 '" + chartPath + "' 中存在但不是一个目录。");
             isValid = false;
         }
         // crds 目录存在且是目录即可，不需进一步检查
     }
     // 如果 'crds' 目录不存在，则忽略，是可选的

    return isValid;
}

} // namespace chart_util 