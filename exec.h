// exec.h
#ifndef TEMPLATE_EXEC_H
#define TEMPLATE_EXEC_H

#include "parse.h"
#include "values.h"

#include <sstream>
#include <vector>
#include <stack>
#include <map>
#include <stdexcept>

namespace template_engine {

// 错误类型
enum ExecErrorType {
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
    Values* value;
    
    Variable() : name(""), value(NULL) {}
    
    Variable(const std::string& n, Values* v)
        : name(n), value(v) {}
        
    ~Variable() {
        // 不在这里删除value，因为它的生命周期由ExecContext管理
    }
};

// 执行错误
class ExecError : public std::runtime_error {
public:
    ExecError(ExecErrorType type, const std::string& templateName, const std::string& msg)
        : std::runtime_error(msg), type_(type), templateName_(templateName) {}
    
    virtual ~ExecError() throw() {}  // 修复异常规范
    
    ExecErrorType type() const { return type_; }
    const std::string& templateName() const { return templateName_; }
    
private:
    ExecErrorType type_;
    std::string templateName_;
};

// 函数类型定义
class TemplateFn {
public:
    virtual ~TemplateFn() {}
    virtual Values* operator()(const std::vector<Values*>& args) = 0;
};

// 函数库
class FunctionLib {
public:
    FunctionLib();
    ~FunctionLib(); // 析构函数，负责清理函数
    
    void AddFunction(const std::string& name, TemplateFn* func);
    bool HasFunction(const std::string& name) const;
    TemplateFn* GetFunction(const std::string& name) const;
    
    // 提供一个可以设置ExecContext的方法
    void SetContext(class ExecContext* ctx);
    
    // 获取上下文
    class ExecContext* GetContext() const { return ctx_; }
    
private:
    std::map<std::string, TemplateFn*> functions_;
    class ExecContext* ctx_; // 保存ExecContext指针，而不是引用
    
    // 初始化内置函数
    void initBuiltinFunctions();
};

// 执行选项
struct ExecOptions {
    bool missingKeyError;   // 是否对缺失的键报错
    int maxExecDepth;       // 最大执行深度
    
    ExecOptions() : missingKeyError(false), maxExecDepth(100) {}
};

// 执行上下文
class ExecContext {
public:
    ExecContext(
        Tree* tmpl,
        std::ostream& writer,
        Values* data,
        FunctionLib& funcs,
        const ExecOptions& options = ExecOptions());
    
    ~ExecContext(); // 析构函数，负责清理资源
    
    // 执行模板
    void Execute();
    
    // 当前模板
    Tree* GetTemplate();
    
    // 输出器
    std::ostream& GetWriter();
    
    // 变量管理
    void PushVariable(const std::string& name, Values* value);
    int MarkVariables();
    void PopVariables(int mark);
    void SetVariable(const std::string& name, Values* value);
    void SetTopVariable(int n, Values* value);
    Values* GetVariable(const std::string& name);
    
    // 错误管理
    void Error(ExecErrorType type, const std::string& format, ...);
    
    // 获取函数库
    FunctionLib& GetFunctions();
    
    // 执行深度管理
    void IncrementDepth();
    void DecrementDepth();
    
    // 从变量中查找字段
    Values* FindField(Values* value, const std::string& name);
    
    // 插入子模板
    void IncludeTemplate(const std::string& name, Values* data);
    
    // 打印值
    void PrintValue(const Node* node, Values* value);
    
    // 辅助函数
    Values* getFieldValue(Values* context, const std::string& field);
    void debugPrintValue(const char* prefix, Values* value);
    
    // 将isTrue从private移到public
    bool isTrue(Values* val);
    
private:
    Tree* tmpl_;
    std::ostream& writer_;
    const Node* currentNode_;
    std::vector<Variable> vars_;
    FunctionLib& funcs_;
    ExecOptions options_;
    int depth_;
    std::map<std::string, Tree*> templateCache_;
    
    // 核心执行函数
    void walk(Values* dot, const Node* node);
    void walkIfOrWith(NodeType type, Values* dot, const BranchNode* node);
    void walkRange(Values* dot, const RangeNode* node);
    void walkTemplate(Values* dot, const TemplateNode* node);
    
    // 求值函数
    Values* evalPipeline(Values* dot, const PipeNode* pipe);
    Values* evalCommand(Values* dot, const CommandNode* cmd, 
                                    Values* final = NULL);
    Values* evalFunction(Values* dot, const std::string& name, 
                                    const CommandNode* cmd, const std::vector<const Node*>& args, 
                                    Values* final = NULL, bool finalIsFirst = false);
    Values* evalField(Values* dot, const std::string& fieldNameInput, 
                                  const Node* node, const std::vector<const Node*>& args, 
                                  Values* final, Values* receiver);
    Values* evalChainedField(Values* dot, const ChainNode* chainNode, Values* final);
    Values* evalCall(Values* dot, TemplateFn* func, 
                                const Node* node, const std::string& name,
                                const std::vector<const Node*>& args, 
                                Values* final);

    
    // 辅助函数
    Values* evalArg(Values* dot, const Node* n);
    bool areEqual(const Values* a, const Values* b);
    void printNodeTree(const Node* node, int indent);
    
    // 禁止拷贝和赋值
    ExecContext(const ExecContext&);
    ExecContext& operator=(const ExecContext&);
};

// 执行模板函数 - 便捷API
std::string ExecuteTemplate(
    const std::string& templateName,
    const std::string& templateContent,
    Values* data,
    const std::string& leftDelim = "{{",
    const std::string& rightDelim = "}}",
    const ExecOptions& options = ExecOptions());

} // namespace template_engine

#endif // TEMPLATE_EXEC_H