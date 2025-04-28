#ifndef TEMPLATE_SYNTAX_CHECKER_H
#define TEMPLATE_SYNTAX_CHECKER_H

#include <string>
#include <vector>

namespace template_engine {

// 错误类型
enum TemplateSyntaxErrorType {
    ErrorType_Syntax,
    ErrorType_Control,
    ErrorType_VariablePath,
    ErrorType_DataCompatible,
    ErrorType_EmptyTemplate,
    ErrorType_Delimiter
};

// 语法错误信息结构体
struct TemplateSyntaxError {
    TemplateSyntaxErrorType type;
    int line;
    int column;
    std::string message;
    std::string context;
    std::string description;
};

// 检查模板语法，返回true表示语法正确，false表示有错误，错误信息写入errors
bool CheckTemplateSyntax(
    const std::string& templateName,
    const std::string& templateStr,
    const std::string& leftDelim,
    const std::string& rightDelim,
    const std::string& dataStr, // 可选，数据内容
    std::vector<TemplateSyntaxError>& errors);

}

#endif // TEMPLATE_SYNTAX_CHECKER_H 