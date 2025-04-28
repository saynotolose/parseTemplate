// exec.h
#ifndef TEMPLATE_EXEC_H
#define TEMPLATE_EXEC_H

#include "parse.h"
#include "values.h"
#include <memory>
#include <sstream>
#include <vector>
#include <stack>
#include <functional>
#include <unordered_map>
#include <stdexcept>

namespace template_engine {

// 错误类型
enum class ExecErrorType {
    RuntimeError,
    WriteError,
    TypeMismatch,
    MissingKey,
    UndefinedVariable,
    RecursionLimit
};

// 变量栈中的变量
struct Variable {
    std::string name;
    std::shared_ptr<Values> value;
    
    // 添加默认构造函数
    Variable() : name(""), value(nullptr) {}
    
    Variable(const std::string& n, std::shared_ptr<Values> v)
        : name(n), value(std::move(v)) {}
};

// 执行错误
class ExecError : public std::runtime_error {
public:
    ExecError(ExecErrorType type, const std::string& templateName, const std::string& msg)
        : std::runtime_error(msg), type_(type), templateName_(templateName) {}
    
    ExecErrorType type() const { return type_; }
    const std::string& templateName() const { return templateName_; }
    
private:
    ExecErrorType type_;
    std::string templateName_;
};

// 函数类型定义
using TemplateFn = std::function<std::shared_ptr<Values>(const std::vector<std::shared_ptr<Values>>& args)>;

// 函数库
class FunctionLib {
public:
    FunctionLib();
    
    void AddFunction(const std::string& name, TemplateFn func);
    bool HasFunction(const std::string& name) const;
    TemplateFn GetFunction(const std::string& name) const;
    
private:
    std::unordered_map<std::string, TemplateFn> functions_;
    
    // 初始化内置函数
    void initBuiltinFunctions();
};

// 执行选项
struct ExecOptions {
    bool missingKeyError = false;   // 是否对缺失的键报错
    int maxExecDepth = 100;         // 最大执行深度
};


// 执行上下文
class ExecContext {
public:
    ExecContext(
        std::shared_ptr<Tree> tmpl,
        std::ostream& writer,
        std::shared_ptr<Values> data,
        FunctionLib& funcs,
        const ExecOptions& options = ExecOptions());
    
    // 执行模板
    void Execute();
    
    // 当前模板
    std::shared_ptr<Tree> GetTemplate() const;
    
    // 输出器
    std::ostream& GetWriter();
    
    // 变量管理
    void PushVariable(const std::string& name, std::shared_ptr<Values> value);
    int MarkVariables();
    void PopVariables(int mark);
    void SetVariable(const std::string& name, std::shared_ptr<Values> value);
    void SetTopVariable(int n, std::shared_ptr<Values> value);
    std::shared_ptr<Values> GetVariable(const std::string& name);
    
    // 错误管理
    [[noreturn]] void Error(ExecErrorType type, const std::string& format, ...);
    
    // 获取函数库
    FunctionLib& GetFunctions();
    
    // 执行深度管理
    void IncrementDepth();
    void DecrementDepth();
    
    // 从变量中查找字段
    std::shared_ptr<Values> FindField(std::shared_ptr<Values> value, const std::string& name);
    
    // 插入子模板
    void IncludeTemplate(const std::string& name, std::shared_ptr<Values> data);
    
    // 打印值
    void PrintValue(const Node* node, std::shared_ptr<Values> value);
    
private:
    std::shared_ptr<Tree> tmpl_;
    std::ostream& writer_;
    const Node* currentNode_ = nullptr;
    std::vector<Variable> vars_;
    FunctionLib& funcs_;
    ExecOptions options_;
    int depth_ = 0;
    std::unordered_map<std::string, std::shared_ptr<Tree>> templateCache_;
    
    // 核心执行函数
    void walk(std::shared_ptr<Values> dot, const Node* node);
    void walkIfOrWith(NodeType type, std::shared_ptr<Values> dot, const BranchNode* node);
    void walkRange(std::shared_ptr<Values> dot, const RangeNode* node);
    void walkTemplate(std::shared_ptr<Values> dot, const TemplateNode* node);
    
    // 求值函数
    std::shared_ptr<Values> evalPipeline(std::shared_ptr<Values> dot, const PipeNode* pipe);
    std::shared_ptr<Values> evalCommand(std::shared_ptr<Values> dot, const CommandNode* cmd, 
                                        std::shared_ptr<Values> final = nullptr);
    std::shared_ptr<Values> evalFunction(std::shared_ptr<Values> dot, const std::string& name, 
                                        const CommandNode* cmd, const std::vector<const Node*>& args, 
                                        std::shared_ptr<Values> final = nullptr);
    std::shared_ptr<Values> evalField(std::shared_ptr<Values> dot, const std::string& fieldName, 
                                    const Node* node, const std::vector<const Node*>& args, 
                                    std::shared_ptr<Values> final, std::shared_ptr<Values> receiver);
    std::shared_ptr<Values> evalFieldChain(std::shared_ptr<Values> dot, std::shared_ptr<Values> receiver,
                                          const Node* node, const std::vector<std::string>& ident,
                                          const std::vector<const Node*>& args, 
                                          std::shared_ptr<Values> final);
    std::shared_ptr<Values> evalCall(std::shared_ptr<Values> dot, TemplateFn func, 
                                    const Node* node, const std::string& name,
                                    const std::vector<const Node*>& args, 
                                    std::shared_ptr<Values> final);
    
    // 辅助函数
    bool isTrue(std::shared_ptr<Values> val);
    std::shared_ptr<Values> evalArg(std::shared_ptr<Values> dot, const Node* n);
};

// 执行模板函数 - 便捷API
std::string ExecuteTemplate(
    const std::string& templateName,
    const std::string& templateContent,
    std::shared_ptr<Values> data,
    const std::string& leftDelim = "{{",
    const std::string& rightDelim = "}}",
    const ExecOptions& options = ExecOptions());

} // namespace template_engine

#endif // TEMPLATE_EXEC_H