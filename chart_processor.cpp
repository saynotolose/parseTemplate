#include "chart_processor.h"
#include "values.h" // 需要 Values 和 ParseSimpleYAMLFile
#include "exec.h"   // 需要 ExecuteTemplate
#include <fstream>  // C++98 文件流
#include <sstream>  // C++98 字符串流
#include <sys/stat.h> // C++98 POSIX stat
#include <dirent.h>   // C++98 POSIX 目录操作
#include <errno.h>    // errno
#include <string.h>   // strerror

// 辅助函数，检查路径类型 (来自 chart_validator.cpp，避免重复定义)
// 返回值: 0 = 不存在或错误, 1 = 文件, 2 = 目录
static int getPathType_Processor(const std::string& path) {
    struct stat path_stat;
    if (stat(path.c_str(), &path_stat) != 0) {
        return 0;
    }
    if (S_ISREG(path_stat.st_mode)) {
        return 1; // 普通文件
    }
    if (S_ISDIR(path_stat.st_mode)) {
        return 2; // 目录
    }
    return 0;
}

namespace chart_processor {

bool ProcessChartTemplates(
    const std::string& chartPath,
    std::map<std::string, std::string>& renderedResults,
    std::vector<std::string>& errors) {

    renderedResults.clear();
    errors.clear();

    // 1. 解析 values.yaml
    std::string valuesPath = chartPath + "/values.yaml";
    template_engine::Values* values = NULL;
    try {
        // 检查 values.yaml 是否存在
        if (getPathType_Processor(valuesPath) == 1) {
             values = template_engine::ParseSimpleYAMLFile(valuesPath);
        } else {
            // values.yaml 不存在或不是文件，使用空的 Values 对象
            errors.push_back("警告: '" + valuesPath + "' 未找到或不是文件，使用空 Values。 ");
            values = template_engine::Values::MakeMap(std::map<std::string, template_engine::Values*>());
        }
    } catch (const std::exception& e) {
        errors.push_back("错误: 解析 '" + valuesPath + "' 失败: " + e.what());
        return false; // Values 解析失败是严重错误
    }

    // 2. 遍历 templates/ 目录
    std::string templatesPath = chartPath + "/templates";
    if (getPathType_Processor(templatesPath) != 2) {
        errors.push_back("错误: 'templates' 目录 '" + templatesPath + "' 不存在或不是一个目录。");
        delete values; // 清理已分配的 Values 对象
        return false; // templates 目录不存在是严重错误
    }

    DIR* dir = opendir(templatesPath.c_str());
    if (!dir) {
        errors.push_back("错误: 无法打开 'templates' 目录 '" + templatesPath + "': " + strerror(errno));
        delete values;
        return false;
    }

    struct dirent* entry;
    bool hasRenderingErrors = false;
    while ((entry = readdir(dir)) != NULL) {
        std::string entryName = entry->d_name;

        // 跳过 '.' 和 '..'
        if (entryName == "." || entryName == "..") {
            continue;
        }

        // 跳过以下划线 '_' 开头的文件/目录 (Helm 约定)
        if (!entryName.empty() && entryName[0] == '_') {
            continue;
        }

        std::string fullEntryPath = templatesPath + "/" + entryName;
        std::string relativeTemplatePath = "templates/" + entryName; // 用于结果 Map 的键

        // 只处理普通文件
        if (getPathType_Processor(fullEntryPath) == 1) {

            // --- 检查文件扩展名 --- 
            size_t nameLen = entryName.length();
            bool isYaml = (nameLen > 5 && entryName.substr(nameLen - 5) == ".yaml") ||
                          (nameLen > 4 && entryName.substr(nameLen - 4) == ".yml");
            
            if (isYaml) { // 只处理 YAML 文件
                // 读取模板文件内容
                std::ifstream tplFile(fullEntryPath.c_str());
                if (!tplFile.is_open()) {
                    errors.push_back("错误: 无法打开模板文件 '" + fullEntryPath + "'");
                    hasRenderingErrors = true;
                    continue; // 继续处理下一个文件
                }
                std::stringstream buffer;
                buffer << tplFile.rdbuf();
                std::string templateContent = buffer.str();
                tplFile.close();

                if (templateContent.empty()) {
                    // Helm 对于空模板文件通常渲染为空字符串
                    renderedResults[relativeTemplatePath] = "";
                    continue;
                }

                // 创建顶层上下文，它是一个 Map
                std::map<std::string, template_engine::Values*> rootContextMap;
                // 将解析得到的 values 对象放入顶层 Map 中，键为 "Values"
                rootContextMap["Values"] = values; 
                template_engine::Values* rootContext = template_engine::Values::MakeMap(rootContextMap);

                // 渲染模板
                try {
                    // 使用 ExecuteTemplate 进行渲染
                    // 注意：模板名称可以简单地使用相对路径，或者根据需要生成更复杂的名称
                    std::string renderedOutput = template_engine::ExecuteTemplate(
                        relativeTemplatePath, // 使用相对路径作为模板名
                        templateContent,
                        rootContext, // <--- 传递封装后的 rootContext
                        "{{", // 默认左定界符
                        "}}"  // 默认右定界符
                        // 如果需要传递 ExecOptions，在这里添加
                    );
                    renderedResults[relativeTemplatePath] = renderedOutput;

                } catch (const std::exception& e) {
                    errors.push_back("错误: 渲染模板 '" + relativeTemplatePath + "' 失败: " + e.what());
                    hasRenderingErrors = true;
                    // 即使单个模板失败，也继续处理其他模板
                }
                // 清理封装对象 (假设 MakeMap 返回的是堆分配的对象)
                 if (rootContext) { // 检查指针是否有效
                     delete rootContext;
                     rootContext = NULL; // 防止悬空指针
                 }
            } // --- 结束 YAML 文件处理块 ---
        }
        // 忽略目录、非 YAML 文件和其他类型的文件
    }

    closedir(dir);

    // 3. 递归处理子 Chart (charts/ 目录下的子目录)
    std::string chartsPath = chartPath + "/charts";
    int chartsPathType = getPathType_Processor(chartsPath);
    if (chartsPathType != 0) { // 'charts' 目录或文件存在
        if (chartsPathType != 2) {
            errors.push_back("警告: 'charts' 在 '" + chartPath + "' 中存在但不是一个目录，跳过子 Chart 处理。");
        } else {
            // 是目录，遍历查找子 Chart 目录
            DIR* chartsDir = opendir(chartsPath.c_str());
            if (!chartsDir) {
                errors.push_back("错误: 无法打开 'charts' 目录 '" + chartsPath + "': " + strerror(errno));
                // 即使无法打开 charts 目录，也认为顶层处理可能部分成功，继续
                hasRenderingErrors = true; // 标记存在问题
            } else {
                struct dirent* chartsEntry;
                while ((chartsEntry = readdir(chartsDir)) != NULL) {
                    std::string subChartName = chartsEntry->d_name;
                    if (subChartName == "." || subChartName == "..") {
                        continue;
                    }

                    std::string subChartPath = chartsPath + "/" + subChartName;
                    int subChartType = getPathType_Processor(subChartPath);

                    // 只处理子目录 (未压缩的子 Chart)
                    if (subChartType == 2) {
                        std::map<std::string, std::string> subRenderedResults;
                        std::vector<std::string> subErrors;
                        std::string subChartRelativePath = "charts/" + subChartName;

                        errors.push_back("信息: 开始处理子 Chart: " + subChartRelativePath);

                        // 递归调用
                        bool subResult = ProcessChartTemplates(subChartPath, subRenderedResults, subErrors);

                        // 合并结果 (调整路径)
                        for (std::map<std::string, std::string>::const_iterator it = subRenderedResults.begin(); it != subRenderedResults.end(); ++it) {
                            renderedResults[subChartRelativePath + "/" + it->first] = it->second;
                        }

                        // 合并错误 (添加上下文)
                        for (size_t i = 0; i < subErrors.size(); ++i) {
                            errors.push_back("子 Chart[" + subChartRelativePath + "]: " + subErrors[i]);
                            // 如果子Chart处理失败，标记顶层处理也有问题
                            if (subErrors[i].rfind("错误:", 0) == 0) {
                                hasRenderingErrors = true;
                            }
                        }

                        if (!subResult) {
                             errors.push_back("错误: 子 Chart[" + subChartRelativePath + "] 处理过程中发生严重错误。 ");
                            hasRenderingErrors = true;
                        }
                         errors.push_back("信息: 完成处理子 Chart: " + subChartRelativePath);
                    }
                    // 忽略 charts/ 目录下的文件 (如 .tgz 压缩包) 和其他类型
                }
                closedir(chartsDir);
            }
        }
    }
    // 如果 'charts' 目录不存在，则忽略，是可选的

    delete values; // 清理顶层的 Values 对象

    // 返回 true 表示主流程完成，具体错误看 errors 向量
    // 如果需要更严格的成功/失败定义（例如，任何子chart失败都返回false），可以修改这里
    return true;
}

} // namespace chart_processor 