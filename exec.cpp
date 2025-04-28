// exec.cpp
#include "exec.h"
#include <stdarg.h>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <sstream>

namespace template_engine {

// 辅助函数：格式化错误消息
static std::string formatErrorMessage(const char* format, va_list args) {
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    return std::string(buffer);
}

// 工具函数：去除所有空行（C++98 兼容，直接删除所有空行）
static std::string RemoveAllEmptyLines(const std::string& input) {
    std::istringstream in(input);
    std::string line;
    std::string result;
    bool firstLine = true;
    while (std::getline(in, line)) {
        // 判断本行是否全是空白字符
        bool thisLineEmpty = true;
        for (size_t i = 0; i < line.size(); ++i) {
            if (line[i] != ' ' && line[i] != '\t' && line[i] != '\r') {
                thisLineEmpty = false;
                break;
            }
        }
        if (!thisLineEmpty) {
            if (!firstLine) {
                result += "\n";
            }
            result += line;
            firstLine = false;
        }
    }
    return result;
}

// 工具函数：去除多余空行（C++98 兼容版）
static std::string RemoveExtraEmptyLines(const std::string& input) {
    std::istringstream in(input);
    std::string line;
    std::string result;
    bool lastLineEmpty = false;
    bool firstContentLineFound = false;

    while (std::getline(in, line)) {
        // 判断本行是否全是空白字符
        bool thisLineEmpty = true;
        for (size_t i = 0; i < line.size(); ++i) {
            if (line[i] != ' ' && line[i] != '\t' && line[i] != '\r') {
                thisLineEmpty = false;
                break;
            }
        }

        if (thisLineEmpty) {
            // 如果前一行不是空行，且已经遇到过内容行，则保留一个空行
            if (!lastLineEmpty && firstContentLineFound) {
                result += "\n";
            }
            lastLineEmpty = true;
        } else {
            // 非空行，正常输出
            result += line + "\n";
            lastLineEmpty = false;
            firstContentLineFound = true;
        }
    }

    // 去除末尾多余空行
    while (!result.empty() && (result[result.size()-1] == '\n' || result[result.size()-1] == '\r')) {
        result.erase(result.size()-1, 1);
    }
    // 去除开头多余空行
    while (!result.empty() && (result[0] == '\n' || result[0] == '\r')) {
        result.erase(0, 1);
    }
    return result;
}

// 内置函数实现
// 等于函数
class EqFunction : public TemplateFn {
public:
    Values* operator()(const std::vector<Values*>& args) {
        if (args.size() < 2) {
            return Values::MakeBool(false);
        }
        
        bool result = true;
        for (size_t i = 1; i < args.size(); ++i) {
            if (args[0]->IsString() && args[i]->IsString()) {
                if (args[0]->AsString() != args[i]->AsString()) {
                    result = false;
                    break;
                }
            } else if (args[0]->IsNumber() && args[i]->IsNumber()) {
                if (fabs(args[0]->AsNumber() - args[i]->AsNumber()) > 1e-10) {
                    result = false;
                    break;
                }
            } else if (args[0]->IsBool() && args[i]->IsBool()) {
                if (args[0]->AsBool() != args[i]->AsBool()) {
                    result = false;
                    break;
                }
            } else {
                result = false;
                break;
            }
        }
        
        return Values::MakeBool(result);
    }
};

// 不等于函数
class NeFunction : public TemplateFn {
public:
    NeFunction(FunctionLib& lib) : lib_(lib) {}
    
    Values* operator()(const std::vector<Values*>& args) {
        if (args.size() < 2) {
            return Values::MakeBool(true);
        }
        
        Values* eqResult = lib_.GetFunction("eq")->operator()(args);
        Values* result = Values::MakeBool(!eqResult->AsBool());
        delete eqResult; // 释放临时结果
        return result;
    }
    
private:
    FunctionLib& lib_;
};

// 大于函数
class GtFunction : public TemplateFn {
public:
    Values* operator()(const std::vector<Values*>& args) {
        if (args.size() < 2) {
            return Values::MakeBool(false);
        }
        
        // 检查类型兼容性
        if ((args[0]->IsNumber() && args[1]->IsNumber()) || 
            (args[0]->IsString() && args[1]->IsString())) {
            
            if (args[0]->IsNumber()) {
                return Values::MakeBool(args[0]->AsNumber() > args[1]->AsNumber());
            } else { // IsString
                return Values::MakeBool(args[0]->AsString() > args[1]->AsString());
            }
        }
        
        // 类型不兼容
        return Values::MakeBool(false);
    }
};

// 小于函数
class LtFunction : public TemplateFn {
public:
    Values* operator()(const std::vector<Values*>& args) {
        if (args.size() < 2) {
            return Values::MakeBool(false);
        }
        
        // 检查类型兼容性
        if ((args[0]->IsNumber() && args[1]->IsNumber()) || 
            (args[0]->IsString() && args[1]->IsString())) {
            
            if (args[0]->IsNumber()) {
                return Values::MakeBool(args[0]->AsNumber() < args[1]->AsNumber());
            } else { // IsString
                return Values::MakeBool(args[0]->AsString() < args[1]->AsString());
            }
        }
        
        // 类型不兼容
        return Values::MakeBool(false);
    }
};

// 大于等于函数
class GeFunction : public TemplateFn {
public:
    GeFunction(FunctionLib& lib) : lib_(lib) {}
    
    Values* operator()(const std::vector<Values*>& args) {
        if (args.size() < 2) {
            return Values::MakeBool(false);
        }
        
        // 使用 lt 函数的否定
        Values* ltResult = lib_.GetFunction("lt")->operator()(args);
        Values* result = Values::MakeBool(!ltResult->AsBool());
        delete ltResult; // 释放临时结果
        return result;
    }
    
private:
    FunctionLib& lib_;
};

// 小于等于函数
class LeFunction : public TemplateFn {
public:
    LeFunction(FunctionLib& lib) : lib_(lib) {}
    
    Values* operator()(const std::vector<Values*>& args) {
        if (args.size() < 2) {
            return Values::MakeBool(false);
        }
        
        // 使用 gt 函数的否定
        Values* gtResult = lib_.GetFunction("gt")->operator()(args);
        Values* result = Values::MakeBool(!gtResult->AsBool());
        delete gtResult; // 释放临时结果
        return result;
    }
    
private:
    FunctionLib& lib_;
};

// 新增：and函数实现
class AndFunction : public TemplateFn {
public:
    AndFunction(FunctionLib& lib) : lib_(lib) {}

    Values* operator()(const std::vector<Values*>& args) {
        // 如果没有参数，返回true（空and为true）
        if (args.empty()) {
            return Values::MakeBool(true);
        }
        
        // 短路逻辑：任何一个参数为假时立即返回false
        for (size_t i = 0; i < args.size(); ++i) {
            if (lib_.GetContext() && !lib_.GetContext()->isTrue(args[i])) {
                return Values::MakeBool(false);
            }
        }
        
        // 所有参数都为真，返回true
        return Values::MakeBool(true);
    }
    
private:
    FunctionLib& lib_;
};

// 新增：or函数实现
class OrFunction : public TemplateFn {
public:
    OrFunction(FunctionLib& lib) : lib_(lib) {}

    Values* operator()(const std::vector<Values*>& args) {
        // 如果没有参数，返回false（空or为false）
        if (args.empty()) {
            return Values::MakeBool(false);
        }
        
        // 短路逻辑：任何一个参数为真时立即返回true
        for (size_t i = 0; i < args.size(); ++i) {
            if (lib_.GetContext() && lib_.GetContext()->isTrue(args[i])) {
                return Values::MakeBool(true);
            }
        }
        
        // 所有参数都为假，返回false
        return Values::MakeBool(false);
    }
    
private:
    FunctionLib& lib_;
};

// 新增：not函数实现
class NotFunction : public TemplateFn {
public:
    NotFunction(FunctionLib& lib) : lib_(lib) {}

    Values* operator()(const std::vector<Values*>& args) {
        // 如果没有参数，返回true（非空为真）
        if (args.empty()) {
            return Values::MakeBool(true);
        }
        
        // 对第一个参数取反
        bool value = lib_.GetContext() ? !lib_.GetContext()->isTrue(args[0]) : true;
        return Values::MakeBool(value);
    }
    
private:
    FunctionLib& lib_;
};

// default函数实现（支持两个参数）
class DefaultFunction : public TemplateFn {
public:
    Values* operator()(const std::vector<Values*>& args) {
        if (args.empty()) return Values::MakeNull();
        Values* value = args[0];
        Values* def = args.size() > 1 ? args[1] : Values::MakeNull();
        if (!value || value->IsNull() ||
            (value->IsString() && value->AsString().empty()) ||
            (value->IsList() && value->AsList().empty()) ||
            (value->IsMap() && value->AsMap().empty())) {
            return new Values(*def);
        }
        return new Values(*value);
    }
};

// FunctionLib实现
FunctionLib::FunctionLib() : ctx_(NULL) {
    // 延迟初始化内置函数，等到有了ExecContext
}

void FunctionLib::SetContext(ExecContext* ctx) {
    ctx_ = ctx;
    
    // 现在有了context，可以初始化内置函数
    if (ctx_) {
    initBuiltinFunctions();
    }
}

FunctionLib::~FunctionLib() {
    // 清理所有函数
    for (std::map<std::string, TemplateFn*>::iterator it = functions_.begin(); 
         it != functions_.end(); ++it) {
        delete it->second;
    }
}

void FunctionLib::AddFunction(const std::string& name, TemplateFn* func) {
    functions_[name] = func;
}

bool FunctionLib::HasFunction(const std::string& name) const {
    return functions_.find(name) != functions_.end();
}

TemplateFn* FunctionLib::GetFunction(const std::string& name) const {
    std::map<std::string, TemplateFn*>::const_iterator it = functions_.find(name);
    if (it != functions_.end()) {
        return it->second;
    }
    throw ExecError(RuntimeError, "", "function not found: " + name);
}

// 内置函数初始化
void FunctionLib::initBuiltinFunctions() {
    // 添加内置函数
    AddFunction("eq", new EqFunction());
    AddFunction("ne", new NeFunction(*this));
    AddFunction("gt", new GtFunction());
    AddFunction("lt", new LtFunction());
    AddFunction("ge", new GeFunction(*this));
    AddFunction("le", new LeFunction(*this));
    
    // 添加逻辑函数
    AddFunction("and", new AndFunction(*this));
    AddFunction("or", new OrFunction(*this));
    AddFunction("not", new NotFunction(*this));
    
    // 新增：default函数
    AddFunction("default", new DefaultFunction());
    
    // 其他内置函数...
}

// ExecContext实现
ExecContext::ExecContext(
    Tree* tmpl,
    std::ostream& writer,
    Values* data,
    FunctionLib& funcs,
    const ExecOptions& options)
    : tmpl_(tmpl), writer_(writer), funcs_(funcs), options_(options),
      currentNode_(0), depth_(0) {
    
    // 设置FunctionLib的context指针
    funcs_.SetContext(this);
    
    // 初始化顶层变量("$") - 创建 data 的深拷贝
    Values* dataCopy = data ? data->DeepCopy() : Values::MakeNull();
    PushVariable("$", dataCopy); // 推入副本，ExecContext 现在拥有 dataCopy
}

ExecContext::~ExecContext() {
    // 释放所有变量 (包括我们拷贝的 $)
    try {
        for (size_t i = 0; i < vars_.size(); ++i) {
            delete vars_[i].value;
            vars_[i].value = NULL;
        }
        vars_.clear();
        
        // 释放模板
        if (tmpl_) {
            delete tmpl_;
            tmpl_ = NULL;
        }
        
        // 释放模板缓存
        for (std::map<std::string, Tree*>::iterator it = templateCache_.begin();
             it != templateCache_.end(); ++it) {
            delete it->second;
            it->second = NULL;
        }
        templateCache_.clear();
    } catch (const std::exception& e) {
        std::cerr << "析构函数中发生异常: " << e.what() << std::endl;
        // 析构函数中不应抛出异常，只记录错误
    } catch (...) {
        std::cerr << "析构函数中发生未知异常" << std::endl;
    }
}

void ExecContext::Execute() {
    if (!tmpl_ || !tmpl_->GetRoot()) {
        Error(RuntimeError, "incomplete or empty template");
    }
    
    // 检查模板中的所有节点 - 使用新的递归打印
    std::cout << "\n=== 检查模板节点 (递归) ===" << std::endl;
    printNodeTree(tmpl_->GetRoot(), 0); // <--- 调用新的递归函数
    std::cout << "===========================\n" << std::endl;
    
    try {
        walk(GetVariable("$"), tmpl_->GetRoot());
    } catch (const ExecError& e) {
        std::cerr << "执行错误: " << e.what() << std::endl;
        throw;
    } catch (const std::exception& e) {
        std::cerr << "未处理的异常: " << e.what() << std::endl;
        Error(RuntimeError, "execution error: %s", e.what());
    } catch (...) {
        std::cerr << "未知异常" << std::endl;
        Error(RuntimeError, "unknown execution error");
    }
}

Tree* ExecContext::GetTemplate() {
    Tree* result = tmpl_;
    tmpl_ = NULL; // 转移所有权
    return result;
}

std::ostream& ExecContext::GetWriter() {
    return writer_;
}

void ExecContext::PushVariable(const std::string& name, Values* value) {
    vars_.push_back(Variable(name, value));
}

int ExecContext::MarkVariables() {
    return vars_.size();
}

void ExecContext::PopVariables(int mark) {
    if (mark >= 0 && mark <= (int)vars_.size()) {
        // 删除从mark到末尾的所有变量
        for (size_t i = mark; i < vars_.size(); ++i) {
            delete vars_[i].value;
        }
        vars_.resize(mark);
    }
}

void ExecContext::SetVariable(const std::string& name, Values* value) {
    for (int i = vars_.size() - 1; i >= 0; --i) {
        if (vars_[i].name == name) {
            delete vars_[i].value; // 释放旧值
            vars_[i].value = value;
            return;
        }
    }
    
    // 如果没找到变量，释放value并报错
    delete value;
    Error(UndefinedVariable, "undefined variable: %s", name.c_str());
}

void ExecContext::SetTopVariable(int n, Values* value) {
    if (n > 0 && vars_.size() >= (size_t)n) {
        delete vars_[vars_.size() - n].value; // 释放旧值
        vars_[vars_.size() - n].value = value;
    } else {
        delete value; // 未使用，释放
    }
}

Values* ExecContext::GetVariable(const std::string& name) {
    for (int i = vars_.size() - 1; i >= 0; --i) {
        if (vars_[i].name == name) {
            // 创建副本返回
            return new Values(*vars_[i].value);
        }
    }
    
    Error(UndefinedVariable, "undefined variable: %s", name.c_str());
    return NULL; // 不会到达这里
}

void ExecContext::Error(ExecErrorType type, const std::string& format, ...) {
    va_list args;
    va_start(args, format);
    std::string msg = formatErrorMessage(format.c_str(), args);
    va_end(args);
    
    std::string templateName = tmpl_ ? tmpl_->GetName() : "";
    std::string location;
    std::string context;
    
    if (currentNode_ && tmpl_) {
        std::pair<std::string, std::string> ctx = tmpl_->ErrorContext(currentNode_);
        location = ctx.first;
        context = ctx.second;
        
        if (!location.empty() && !context.empty()) {
            msg = "template: " + location + ": executing \"" + 
                  templateName + "\" at <" + context + ">: " + msg;
        }
    } else {
        msg = "template: " + templateName + ": " + msg;
    }
    
    throw ExecError(type, templateName, msg);
}

FunctionLib& ExecContext::GetFunctions() {
    return funcs_;
}

void ExecContext::IncrementDepth() {
    depth_++;
    if (depth_ > options_.maxExecDepth) {
        Error(RecursionLimit, "exceeded maximum template depth (%d)", options_.maxExecDepth);
    }
}

void ExecContext::DecrementDepth() {
    depth_--;
}

void ExecContext::walk(Values* dot, const Node* node) {
    // 保存当前节点
    const Node* savedNode = currentNode_;
    currentNode_ = node;
    
    try {
        // 根据节点类型进行处理
        std::cout << "walk: 处理节点类型 " << node->Type() << std::endl;
        switch (node->Type()) {
            case NodeText:
                writer_ << static_cast<const TextNode*>(node)->Text();
                break;
                
            case NodeAction: {
                const ActionNode* action = static_cast<const ActionNode*>(node);
                const PipeNode* pipe = action->Pipe();
                Values* value = evalPipeline(dot, pipe);
                PrintValue(node, value);
                delete value;
                break;
            }
                
            case NodeList: {
                const ListNode* list = static_cast<const ListNode*>(node);
                const std::vector<Node*>& nodes = list->Nodes();
                // 添加详细日志
                std::cout << "  Walking list node at " << node << " containing " << nodes.size() << " children." << std::endl;
                for (size_t i = 0; i < nodes.size(); ++i) {
                    // 添加空指针检查
                    if (!nodes[i]) { 
                         std::cout << "  WARNING: Child node at index " << i << " is NULL!" << std::endl;
                         continue;
                    }
                    // 打印正在处理的子节点信息
                    std::cout << "  Walking child " << i << " (Type " << nodes[i]->Type() << ") at " << nodes[i] << std::endl;
                    walk(dot, nodes[i]);
                }
                break;
            }
                
            case NodeIf:
                walkIfOrWith(NodeIf, dot, static_cast<const IfNode*>(node));
                break;
                
            case NodeWith:
                walkIfOrWith(NodeWith, dot, static_cast<const WithNode*>(node));
                break;
                
            case NodeRange:
                std::cout << "执行NodeRange(15)节点" << std::endl;
                walkRange(dot, static_cast<const RangeNode*>(node));
                break;
                
            case NodeTemplate:
                walkTemplate(dot, static_cast<const TemplateNode*>(node));
                break;
                
            default:
                std::cout << "未知节点类型: " << node->Type() << std::endl;
                if (node->Type() == 15) { // 再次检查是否是Range
                    std::cout << "尝试处理可能的Range节点(类型15)" << std::endl;
                    walkRange(dot, static_cast<const RangeNode*>(node));
                } else {
                    Error(RuntimeError, "unknown node type: %d", static_cast<int>(node->Type()));
                }
        }
    } catch (const std::exception& e) {
        std::cout << "walk处理节点时异常: " << e.what() << std::endl;
        // 恢复当前节点并重新抛出异常
        currentNode_ = savedNode;
        throw;
    }
    
    // 恢复当前节点
    currentNode_ = savedNode;
}

void ExecContext::walkRange(Values* dot, const RangeNode* node) {
    std::cout << "\n=== walkRange开始执行 ===" << std::endl;
    
    // 打印Range节点信息
    std::cout << "Range节点位置: " << node->Position() << std::endl;
    if (node->GetPipe()) {
        std::cout << "Range管道命令数: " << node->GetPipe()->Cmds().size() << std::endl;
    } else {
        std::cout << "Range节点没有管道!" << std::endl;
        return;
    }

    // 获取Range管道的值，这是我们要遍历的集合
    std::cout << "正在评估Range管道..." << std::endl;
    Values* items = NULL;
    
    try {
        // 首先尝试特殊处理Items
        if (node->GetPipe()->Cmds().size() == 1 && 
            node->GetPipe()->Cmds()[0]->Args().size() == 1 &&
            node->GetPipe()->Cmds()[0]->Args()[0]->Type() == NodeField) {
            
            const FieldNode* fieldNode = static_cast<const FieldNode*>(node->GetPipe()->Cmds()[0]->Args()[0]);
            std::string fieldName = fieldNode->Ident();
            if (!fieldName.empty() && fieldName[0] == '.') {
                fieldName = fieldName.substr(1);
            }
            
            std::cout << "Range字段名: " << fieldName << std::endl;
            
            // 直接从根数据查找Items
            items = dot->PathValue(fieldName);
            if (items) {
                std::cout << "直接获取Items成功, 类型: " << items->TypeName() << std::endl;
            } else {
                std::cout << "直接获取Items失败!" << std::endl;
            }
        }
        
        // 如果特殊处理失败，尝试常规管道处理
        if (!items) {
            std::cout << "尝试管道评估..." << std::endl;
            items = evalPipeline(dot, node->GetPipe());
        }
        
    } catch (const std::exception& e) {
        std::cout << "评估Range管道异常: " << e.what() << std::endl;
        return;
    }
    
    if (!items) {
        std::cout << "Range管道评估结果为NULL，将使用else分支" << std::endl;
        if (node->ElseList()) {
            walk(dot, node->ElseList());
        }
        return;
    }
    
    // 添加详细日志检查 items
    std::cout << "Range管道评估成功，检查返回的 items:" << std::endl;
    std::cout << "  items 指针: " << items << std::endl;
    std::cout << "  items 类型: " << items->TypeName() << std::endl;
    if (items->IsList()) {
        std::cout << "  items 是列表，长度: " << items->AsList().size() << std::endl;
        if (items->AsList().empty()) {
             std::cout << "  items 列表为空!" << std::endl;
        }
    } else {
        std::cout << "  items 不是列表!" << std::endl;
    }
    // 结束添加的日志
    
    std::cout << "Range值类型: " << items->TypeName() << std::endl;
    
    // 处理列表类型
    if (items->IsList()) {
        const std::vector<Values*>& list = items->AsList();
        std::cout << "处理列表: 长度=" << list.size() << std::endl;
        
        if (list.empty()) {
            std::cout << "列表为空, 使用else分支" << std::endl;
            if (node->ElseList()) {
                walk(dot, node->ElseList());
            }
        } else {
            // 有内容，遍历列表
            int mark = MarkVariables();
            // 先压入一个占位符
            PushVariable(".", Values::MakeNull()); 
            
            for (size_t i = 0; i < list.size(); ++i) {
                std::cout << "\n处理列表项 " << i << std::endl;
                
                // 创建当前项目的值
                Values* item = NULL;
                if (list[i]) {
                    item = new Values(*list[i]); // 创建副本
                    std::cout << "  项目类型: " << item->TypeName() << std::endl;
                    
                    if (item->IsMap()) {
                        const std::map<std::string, Values*>& itemMap = item->AsMap();
                        std::cout << "  项目键: ";
                        for (std::map<std::string, Values*>::const_iterator it = itemMap.begin();
                             it != itemMap.end(); ++it) {
                            std::cout << it->first << " ";
                        }
                        std::cout << std::endl;
                    }
                } else {
                    item = Values::MakeNull();
                    std::cout << "  项目为null" << std::endl;
                }
                
                // 更新栈顶的 "." 变量的值
                SetTopVariable(1, item); 
                
                // 执行range循环体
                std::cout << "  执行Range循环体:" << std::endl;
                if (node->List()) {
                    // 使用更新后的栈顶变量作为上下文
                    walk(vars_.back().value, node->List()); 
                } else {
                    std::cout << "  Range节点没有循环体!" << std::endl;
                }
                // SetTopVariable 已经处理了旧值的释放，这里不需要额外操作
            }
            
            // 恢复变量状态，弹出循环开始前添加的"."变量
            PopVariables(mark);
        }
    }
    // 处理映射类型
    else if (items->IsMap()) {
        const std::map<std::string, Values*>& mapValues = items->AsMap();
        std::cout << "处理映射: 键数=" << mapValues.size() << std::endl;
        
        if (mapValues.empty()) {
            std::cout << "映射为空, 使用else分支" << std::endl;
            if (node->ElseList()) {
                walk(dot, node->ElseList());
            }
        } else {
            // 有内容，遍历映射
            int mark = MarkVariables();
            // 先压入一个占位符
            PushVariable(".", Values::MakeNull()); 
            
            // 收集并排序所有键
            std::vector<std::string> keys;
            for (std::map<std::string, Values*>::const_iterator it = mapValues.begin(); 
                 it != mapValues.end(); ++it) {
                keys.push_back(it->first);
            }
            std::sort(keys.begin(), keys.end());
            
            // 遍历键值对
            for (size_t i = 0; i < keys.size(); ++i) {
                const std::string& key = keys[i];
                std::cout << "\n处理映射项 " << key << std::endl;
                
                // 创建包含key和value的新map
                std::map<std::string, Values*> entryMap;
                entryMap["key"] = Values::MakeString(key);
                
                Values* valueCopy = nullptr;
                auto mapIt = mapValues.find(key);
                if (mapIt != mapValues.end() && mapIt->second) {
                     valueCopy = new Values(*mapIt->second); // 创建值的副本
                } else {
                    valueCopy = Values::MakeNull();
                }
                entryMap["value"] = valueCopy;
                
                // 创建entry值
                Values* entry = Values::MakeMap(entryMap);
                // entry 现在拥有 valueCopy 的所有权, MakeMap 应该处理其内部值的生命周期 (假设是这样)
                // 如果 MakeMap 不复制传入的值，则需要调整 valueCopy 的生命周期管理
                
                // 更新栈顶的 "." 变量的值
                SetTopVariable(1, entry); // SetTopVariable 会 delete 旧值
                
                // 执行循环体
                std::cout << "  执行Range循环体:" << std::endl;
                if (node->List()) {
                    // 使用更新后的栈顶变量作为上下文
                    walk(vars_.back().value, node->List());
                } else {
                    std::cout << "  Range节点没有循环体!" << std::endl;
                }
            }
            
            // 恢复变量状态，弹出循环开始前添加的"."变量
            PopVariables(mark);
        }
    }
    // 其他类型，使用else分支
    else {
        std::cout << "Range值既不是列表也不是映射，使用else分支" << std::endl;
        if (node->ElseList()) {
            walk(dot, node->ElseList());
        }
    }
    
    delete items;
    std::cout << "=== walkRange执行完成 ===\n" << std::endl;
}

void ExecContext::walkTemplate(Values* dot, const TemplateNode* node) {
    // 获取模板名称
    const std::string& name = node->Name();
    
    // 执行管道获取数据
    Values* pipeVal = NULL;
    if (node->Pipe()) {
        pipeVal = evalPipeline(dot, node->Pipe());
    } else {
        pipeVal = new Values(*dot); // 复制dot
    }
    
    // 这里应该根据模板名称从某个存储中加载模板
    // 在这个简化版中，我们假设通过名称可以找到模板
    IncludeTemplate(name, pipeVal);
}

Values* ExecContext::evalPipeline(Values* dot, const PipeNode* pipe) {
    if (!pipe) {
        return Values::MakeNull();
    }
    
    // 记录变量状态
    int mark = MarkVariables();
    
    // 初始值为空
    Values* value = NULL;
    
    // 执行管道中的所有命令
    const std::vector<CommandNode*>& cmds = pipe->Cmds();
    std::cout << "执行管道(命令数: " << cmds.size() << ")" << std::endl;
    
    // --- 移除错误的链式字段检查逻辑 --- 
    // if (cmds.size() == 1) { ... }
    // if (cmds.size() >= 1) { ... } // 这个检查和内部逻辑都基于错误假设
    
    // --- 正确的逻辑：直接评估第一个命令 --- 
    if (!cmds.empty()) {
        value = evalCommand(dot, cmds[0], NULL); // 使用 evalCommand 处理第一个命令
    }
    
    // 处理后续命令（如果有管道符 | ）
    for (size_t i = 1; i < cmds.size(); ++i) {
        Values* cmdResult = evalCommand(dot, cmds[i], value); // 后续命令以上一个结果作为输入
        delete value; // 释放上一个结果
        value = cmdResult;
    }
    
    // 处理变量声明
    const std::vector<VariableNode*>& decls = pipe->Decl();
    for (size_t i = 0; i < decls.size(); ++i) {
        if (pipe->IsAssign()) {
            SetVariable(decls[i]->Ident(), new Values(*value));
        } else {
            PushVariable(decls[i]->Ident(), new Values(*value));
        }
    }
    
    // 如果不需要保留变量，恢复变量状态
    if (decls.empty()) {
        PopVariables(mark);
    }
    
    // 如果没有结果，返回空值
    if (!value) {
        value = Values::MakeNull();
    }
    
    return value;
}

Values* ExecContext::evalChainedField(Values* dot, const ChainNode* chainNode, Values* final) {
    // 改进：确保链式字段的基础节点被正确处理
    Node* baseNode = chainNode->GetNode();
    
    // 处理基础节点
    if (baseNode->Type() == NodeField) {
        // 基础节点是字段，获取字段标识符
        std::string basePath = static_cast<const FieldNode*>(baseNode)->Ident();
        if (!basePath.empty() && basePath[0] == '.') {
            basePath = basePath.substr(1);  // 去掉开头的点
        }
        
        // 获取完整的字段路径
        std::string fullPath = basePath;
        
        // 添加链式字段部分
        const std::vector<std::string>& fields = chainNode->Fields();
        for (size_t i = 0; i < fields.size(); ++i) {
            fullPath += "." + fields[i];
        }
        
        std::cout << "  构建链式字段路径: " << fullPath << std::endl;
        
        // 使用PathValue解析完整路径
        Values* result = dot->PathValue(fullPath);
        if (result) {
            std::cout << "成功评估链式字段: " << fullPath << " = " << result->ToString() << std::endl;
        } else {
            std::cout << "链式字段访问失败: " << fullPath << std::endl;
            result = Values::MakeNull();
        }
        
        return result;
    } else {
        // 其他情况，先处理基础节点，后面再通过链式访问
        Values* baseValue = evalArg(dot, baseNode);
        if (!baseValue) {
            return Values::MakeNull();
        }
        
        // 通过链式字段逐级访问
        Values* currentValue = baseValue;
        const std::vector<std::string>& fields = chainNode->Fields();
        
        for (size_t i = 0; i < fields.size() && currentValue; ++i) {
            if (!currentValue->IsMap()) {
                delete currentValue;
                return Values::MakeNull();
            }
            
            // 查找下一级字段
            const std::map<std::string, Values*>& map = currentValue->AsMap();
            std::map<std::string, Values*>::const_iterator it = map.find(fields[i]);
            
            if (it == map.end() || !it->second) {
                delete currentValue;
                return Values::MakeNull();
            }
            
            // 更新当前值为下一级字段
            Values* nextValue = new Values(*(it->second));
            delete currentValue;
            currentValue = nextValue;
        }
        
        return currentValue;
    }
}

Values* ExecContext::evalCommand(
    Values* dot, 
    const CommandNode* cmd, 
    Values* final) {
    
    if (cmd->Args().empty()) {
        Error(RuntimeError, "empty command");
    }
    const Node* firstArg = cmd->Args()[0];
    std::cout << "evalCommand: 第一个参数类型: " << firstArg->Type() << std::endl;
    if (firstArg->Type() == NodeIdentifier) {
        std::string funcName = static_cast<const IdentifierNode*>(firstArg)->Ident();
        std::cout << "  函数调用: " << funcName << std::endl;
        std::vector<const Node*> funcArgs;
        for (size_t i = 1; i < cmd->Args().size(); ++i) {
            funcArgs.push_back(cmd->Args()[i]);
        }
        // 管道左值final作为第一个参数
        return evalFunction(dot, funcName, cmd, funcArgs, final, true);
    }
    
    // 准备空的参数列表 (evalField 可能需要，暂时保留)
    std::vector<const Node*> emptyArgs;
    
    // 处理不同类型的第一个参数
    switch (firstArg->Type()) {
        case NodeField: {
            // 处理字段访问
            const FieldNode* fieldNode = static_cast<const FieldNode*>(firstArg);
            std::string fieldPath = fieldNode->Ident();
            std::cout << "  字段访问 (NodeField): " << fieldPath << std::endl;
            return evalField(dot, fieldPath, firstArg, emptyArgs, final, NULL);
        }
        
        case NodeChain: { // <--- 添加处理 ChainNode (假设类型 3)
            const ChainNode* chainNode = static_cast<const ChainNode*>(firstArg);
            std::cout << "  链式字段访问 (NodeChain)" << std::endl;
            // 使用 evalChainedField 处理
            return evalChainedField(dot, chainNode, final); 
        }
        
        case NodeDot:
            // 如果命令仅是'.'，返回当前管道值(final)或dot
            std::cout << "  点访问 (.)" << std::endl;
            return final ? new Values(*final) : new Values(*dot);
        
        case NodePipe: {
            // 新增：支持PipeNode参数，递归求值
            const PipeNode* pipeNode = static_cast<const PipeNode*>(firstArg);
            std::cout << "  递归求值PipeNode参数 (括号表达式)" << std::endl;
            return evalPipeline(dot, pipeNode);
        }
        
        default: // 处理其他简单参数类型 (Bool, Number, String etc.)
             std::cout << "  评估简单参数 (Type: " << firstArg->Type() << ")" << std::endl;
            return evalArg(dot, firstArg);
    }
}

bool ExecContext::areEqual(const Values* a, const Values* b) {
    if (!a || !b) {
        return (!a && !b);
    }
    
    if (a->IsNull() && b->IsNull()) {
        return true;
    }
    
    if (a->IsBool() && b->IsBool()) {
        return a->AsBool() == b->AsBool();
    }
    
    if (a->IsNumber() && b->IsNumber()) {
        return fabs(a->AsNumber() - b->AsNumber()) < 1e-10;
    }
    
    if (a->IsString() && b->IsString()) {
        return a->AsString() == b->AsString();
    }
    
    // Map 和 List 比较需要更复杂的逻辑，此处简化处理
    return false;
}

Values* ExecContext::evalFunction(
    Values* dot, 
    const std::string& name, 
    const CommandNode* cmd, 
    const std::vector<const Node*>& args, 
    Values* final,
    bool finalIsFirst) {
    
    if (!funcs_.HasFunction(name)) {
        Error(RuntimeError, "function not found: %s", name.c_str());
    }
    if (name == "and" || name == "or" || name == "not") {
        std::vector<Values*> funcArgs;
        if (finalIsFirst && final) {
            funcArgs.push_back(new Values(*final));
        }
        for (size_t i = 0; i < args.size(); ++i) {
            Values* argValue = evalArg(dot, args[i]);
            std::cout << "  函数参数 " << (finalIsFirst ? i+2 : i+1) << " 类型: " << (argValue ? argValue->TypeName() : "null");
            if (argValue) {
                if (argValue->IsBool()) {
                    std::cout << ", 值: " << (argValue->AsBool() ? "true" : "false");
                } else if (argValue->IsString()) {
                    std::cout << ", 值: \"" << argValue->AsString() << "\"";
                } else if (argValue->IsNumber()) {
                    std::cout << ", 值: " << argValue->AsNumber();
                }
            }
            std::cout << std::endl;
            funcArgs.push_back(argValue ? argValue : Values::MakeNull());
        }
        TemplateFn* func = funcs_.GetFunction(name);
        Values* result = func->operator()(funcArgs);
        for (size_t i = 0; i < funcArgs.size(); ++i) {
            delete funcArgs[i];
        }
        return result;
    }
    if (name == "eq") {
        if (args.size() < 2 && !(finalIsFirst && final)) {
            Error(RuntimeError, "eq function requires at least two arguments");
        }
        std::vector<Values*> evaluatedArgs;
        if (finalIsFirst && final) {
            evaluatedArgs.push_back(new Values(*final));
        }
        for (size_t i = 0; i < args.size(); ++i) {
            Values* argValue = evalArg(dot, args[i]);
            if (argValue) {
                evaluatedArgs.push_back(argValue);
            } else {
                evaluatedArgs.push_back(Values::MakeNull());
            }
        }
        bool equal = true;
        for (size_t i = 1; i < evaluatedArgs.size(); ++i) {
            if (!areEqual(evaluatedArgs[0], evaluatedArgs[i])) {
                equal = false;
                break;
            }
        }
        for (size_t i = 0; i < evaluatedArgs.size(); ++i) {
            delete evaluatedArgs[i];
        }
        return Values::MakeBool(equal);
    }
    std::vector<Values*> funcArgs;
    if (finalIsFirst && final) {
        funcArgs.push_back(new Values(*final));
    }
    for (size_t i = 0; i < args.size(); ++i) {
        Values* argValue = evalArg(dot, args[i]);
        if (argValue) {
            std::cout << "  函数参数 " << (finalIsFirst ? i+2 : i+1) << " 类型: " << argValue->TypeName();
            if (argValue->IsBool()) {
                std::cout << ", 值: " << (argValue->AsBool() ? "true" : "false");
            } else if (argValue->IsString()) {
                std::cout << ", 值: \"" << argValue->AsString() << "\"";
            } else if (argValue->IsNumber()) {
                std::cout << ", 值: " << argValue->AsNumber();
            }
            std::cout << std::endl;
        } else {
            std::cout << "  函数参数 " << (finalIsFirst ? i+2 : i+1) << " 为null" << std::endl;
        }
        funcArgs.push_back(argValue ? argValue : Values::MakeNull());
    }
    TemplateFn* func = funcs_.GetFunction(name);
    Values* result = func->operator()(funcArgs);
    for (size_t i = 0; i < funcArgs.size(); ++i) {
        delete funcArgs[i];
    }
    return result;
}

Values* ExecContext::evalField(
    Values* dot,
    const std::string& fieldNameInput, // 这个参数现在只包含单级字段名
    const Node* node, // 这个参数现在可能没用了
    const std::vector<const Node*>& args, // 这个参数现在可能没用了
    Values* final, 
    Values* receiver) {

    std::cout << "evalField: 处理单级字段 " << fieldNameInput << std::endl;
    
    // 去除前导点
    std::string fieldName = fieldNameInput;
    if (!fieldName.empty() && fieldName[0] == '.') {
        fieldName = fieldName.substr(1);
    }
    
    if (fieldName.empty()) {
        Error(RuntimeError, "empty field name");
        return Values::MakeNull();
    }

    // 确定要在哪个上下文中解析字段
    Values* context = receiver ? receiver : (final ? final : dot);
    
    if (!context) {
        std::cout << "  上下文为空" << std::endl;
        return Values::MakeNull();
    }
    
    std::cout << "  在上下文类型 " << context->TypeName() << " 中查找字段: " << fieldName << std::endl;
    
    // 单级字段访问
    if (!context->IsMap()) {
        std::cout << "  上下文不是map，无法访问字段" << std::endl;
        return Values::MakeNull();
    }
    
    const std::map<std::string, Values*>& map = context->AsMap();
    std::map<std::string, Values*>::const_iterator it = map.find(fieldName);
    
    if (it == map.end() || !it->second) {
        std::cout << "  字段 '" << fieldName << "' 未找到" << std::endl;
        return Values::MakeNull();
    }
    
    std::cout << "  找到字段 '" << fieldName << "', 类型: " << it->second->TypeName();
    if (it->second->IsString()) {
        std::cout << ", 值: \"" << it->second->AsString() << "\"";
    }
    std::cout << std::endl;
    
    // 返回找到字段的副本
    return new Values(*(it->second));
}

void ExecContext::debugPrintValue(const char* prefix, Values* value) {
    if (!value) {
        std::cout << prefix << "值为NULL" << std::endl;
        return;
    }
    
    std::cout << prefix << "类型: " << value->TypeName();
    
    if (value->IsString()) {
        std::cout << ", 字符串值: \"" << value->AsString() << "\"";
    } else if (value->IsNumber()) {
        std::cout << ", 数值: " << value->AsNumber();
    } else if (value->IsBool()) {
        std::cout << ", 布尔值: " << (value->AsBool() ? "true" : "false");
    } else if (value->IsMap()) {
        const std::map<std::string, Values*>& map = value->AsMap();
        std::cout << ", Map大小: " << map.size() << ", 键: {";
        bool first = true;
        for (std::map<std::string, Values*>::const_iterator it = map.begin(); it != map.end(); ++it) {
            if (!first) std::cout << ", ";
            first = false;
            std::cout << it->first;
        }
        std::cout << "}";
    } else if (value->IsList()) {
        std::cout << ", List大小: " << value->AsList().size();
    }
    
    std::cout << std::endl;
}

Values* ExecContext::evalCall(
    Values* dot, 
    TemplateFn* func, 
    const Node* node, 
    const std::string& name,
    const std::vector<const Node*>& args, 
    Values* final) {
    
    // 准备参数列表
    std::vector<Values*> funcArgs;
    for (size_t i = 0; i < args.size(); ++i) {
        funcArgs.push_back(evalArg(dot, args[i]));
    }
    
    // 如果需要，添加final参数
    if (final) {
        funcArgs.push_back(new Values(*final));
    }
    
    // 调用函数
    Values* result = NULL;
    try {
        result = func->operator()(funcArgs);
    } catch (const std::exception& e) {
        // 清理参数
        for (size_t i = 0; i < funcArgs.size(); ++i) {
            delete funcArgs[i];
        }
        Error(RuntimeError, "error calling %s: %s", name.c_str(), e.what());
    }
    
    // 清理参数
    for (size_t i = 0; i < funcArgs.size(); ++i) {
        delete funcArgs[i];
    }
    
    return result;
}

Values* ExecContext::evalArg(Values* dot, const Node* n) {
    if (!n) {
        return Values::MakeNull();
    }
    
    switch (n->Type()) {
        case NodeBool:
            return Values::MakeBool(static_cast<const BoolNode*>(n)->Value());
            
        case NodeNumber:
            return Values::MakeNumber(atof(static_cast<const NumberNode*>(n)->String().c_str()));
            
        case NodeString:
            return Values::MakeString(static_cast<const StringNode*>(n)->Text());
            
        case NodeField: {
            const FieldNode* fieldNode = static_cast<const FieldNode*>(n);
            std::string fieldName = fieldNode->Ident();
            return evalField(dot, fieldName, n, std::vector<const Node*>(), NULL, NULL);
        }
            
        case NodeChain: {
            // 改进：处理链式字段(比如 .Name.first 这种)
            const ChainNode* chainNode = static_cast<const ChainNode*>(n);
            return evalChainedField(dot, chainNode, NULL);
        }
            
        case NodeIdentifier:
            return Values::MakeString(static_cast<const IdentifierNode*>(n)->Ident());
            
        case NodeVariable: {
            std::string name = static_cast<const VariableNode*>(n)->Ident();
            return GetVariable(name);
        }
        
        case NodePipe: {
            // 新增：支持PipeNode参数，递归求值
            const PipeNode* pipeNode = static_cast<const PipeNode*>(n);
            return evalPipeline(dot, pipeNode);
        }
            
        default:
            std::cout << "  未处理的节点类型: " << n->Type() << std::endl;
            return Values::MakeNull();
    }
}

Values* ExecContext::FindField(Values* value, const std::string& name) {
    if (!value || !value->IsMap()) {
        return NULL;
    }
    
    // 处理链式访问 (如 "Name.first")
    std::string fieldName = name;
    if (!fieldName.empty() && fieldName[0] == '.') {
        fieldName = fieldName.substr(1);
    }
    
    size_t dotPos = fieldName.find('.');
    if (dotPos != std::string::npos) {
        std::string firstPart = fieldName.substr(0, dotPos);
        std::string remainingPart = fieldName.substr(dotPos + 1);
        
        const std::map<std::string, Values*>& map = value->AsMap();
        std::map<std::string, Values*>::const_iterator it = map.find(firstPart);
        if (it != map.end() && it->second != NULL) {
            // 递归查找剩余路径
            return FindField(it->second, remainingPart);
        }
        return NULL;
    }
    
    // 处理单一字段
    const std::map<std::string, Values*>& map = value->AsMap();
    std::map<std::string, Values*>::const_iterator it = map.find(fieldName);
    if (it != map.end() && it->second) {
        return new Values(*it->second);
    }
    
    return NULL;
}

void ExecContext::IncludeTemplate(const std::string& name, Values* data) {
    // 在真实场景中，这将从模板存储中加载模板
    // 在这个简化版中，我们假设模板已在缓存中
    std::map<std::string, Tree*>::iterator it = templateCache_.find(name);
    if (it == templateCache_.end()) {
        delete data;
        Error(RuntimeError, "template not found: %s", name.c_str());
    }
    
    // 增加执行深度
    IncrementDepth();
    
    // 创建新上下文并执行模板
    ExecContext newCtx(it->second, writer_, data, funcs_, options_);
    newCtx.Execute();
    
    // 减少执行深度
    DecrementDepth();
}

std::string ExecuteTemplate(
    const std::string& templateName,
    const std::string& templateContent,
    Values* data,
    const std::string& leftDelim,
    const std::string& rightDelim,
    const ExecOptions& options) {
    
    // 解析模板
    std::map<std::string, Tree*> templateMap;
    Tree* mainTemplate = NULL;
    std::string result;
    
    try {
        // 解析模板
        templateMap = Tree::Parse(templateName, templateContent, leftDelim, rightDelim);
        
        if (templateMap.empty()) {
            throw ExecError(RuntimeError, templateName, "failed to parse template");
        }
        
        std::map<std::string, Tree*>::iterator it = templateMap.find(templateName);
        if (it == templateMap.end() || !it->second) {
            // 清理未使用的模板
            for (std::map<std::string, Tree*>::iterator cleanup = templateMap.begin(); 
                 cleanup != templateMap.end(); ++cleanup) {
                delete cleanup->second;
            }
            throw ExecError(RuntimeError, templateName, "template not found after parsing");
        }
        
        // 设置函数库 - 不再需要在这里初始化，会在ExecContext创建时设置
        FunctionLib funcs;
        
        // 准备输出流
        std::stringstream output;
        
        // 保存主模板
        mainTemplate = it->second;
        it->second = NULL; // 防止被下面的循环删除
        
        // 从templateMap中移除主模板，以防止被下面的循环删除
        templateMap.erase(it);
        
        // 清理未使用的模板
        for (std::map<std::string, Tree*>::iterator cleanup = templateMap.begin(); 
             cleanup != templateMap.end(); ++cleanup) {
            delete cleanup->second;
        }
        templateMap.clear();
        
        // 创建执行上下文并执行模板
        {
            ExecContext ctx(mainTemplate, output, data, funcs, options);
            ctx.Execute();
            
            // 获取模板输出
            result = output.str();
            // 去除所有空行
            result = RemoveAllEmptyLines(result);
        } // ctx在这里被销毁
        
        // 不需要删除mainTemplate，因为它由ExecContext接管
        
        return result;
    } catch (const ExecError& e) {
        // 确保在发生异常时也释放资源
        std::cerr << "模板执行出错: " << e.what() << std::endl;
        
        // 清理模板Map中的所有Tree对象
        for (std::map<std::string, Tree*>::iterator cleanup = templateMap.begin(); 
             cleanup != templateMap.end(); ++cleanup) {
            delete cleanup->second;
        }
        
        // 如果mainTemplate还没被释放，则释放它
        delete mainTemplate;
        
        throw; // 重新抛出异常
    } catch (const std::exception& e) {
        std::cerr << "执行过程中发生未处理的异常: " << e.what() << std::endl;
        
        // 清理模板Map中的所有Tree对象
        for (std::map<std::string, Tree*>::iterator cleanup = templateMap.begin(); 
             cleanup != templateMap.end(); ++cleanup) {
            delete cleanup->second;
        }
        
        // 如果mainTemplate还没被释放，则释放它
        delete mainTemplate;
        
        throw; // 重新抛出异常
    }
}

Values* ExecContext::getFieldValue(Values* context, const std::string& field) {
    if (!context || !context->IsMap()) {
        return Values::MakeNull();
    }

    // 去掉可能的前导点
    std::string fieldName = field;
    if (!fieldName.empty() && fieldName[0] == '.') {
        fieldName = fieldName.substr(1);
    }

    // 直接访问字段
    const std::map<std::string, Values*>& contextMap = context->AsMap();
    std::map<std::string, Values*>::const_iterator it = contextMap.find(fieldName);
    if (it != contextMap.end() && it->second) {
        return new Values(*it->second);
    }

    return Values::MakeNull();
}

void ExecContext::PrintValue(const Node* node, Values* value) {
    try {
        debugPrintValue("打印值: ", value);
        
        if (!value) {
            writer_ << "<nil>";
            return;
        }
        
        if (value->IsNull()) {
            // 不输出任何内容
        } else if (value->IsString()) {
            writer_ << value->AsString();
        } else if (value->IsNumber()) {
            writer_ << value->AsNumber();
        } else if (value->IsBool()) {
            writer_ << (value->AsBool() ? "true" : "false");
        } else if (value->IsMap()) {
            // 如果这是一个动作节点，可能需要特殊处理
            if (node && node->Type() == NodeAction) {
                const ActionNode* action = static_cast<const ActionNode*>(node);
                const PipeNode* pipe = action->Pipe();
                
                // 输出第一个有意义的值，而不是整个map
                const std::map<std::string, Values*>& map = value->AsMap();
                if (!map.empty()) {
                    std::map<std::string, Values*>::const_iterator it = map.begin();
                    if (it->second) {
                        PrintValue(node, it->second);
                        return;
                    }
                }
            }
            
            // 如果没有特殊处理，使用普通的toString输出
            writer_ << value->ToString();
        } else if (value->IsList()) {
            writer_ << value->ToString();
        } else {
            writer_ << value->ToString();
        }
    } catch (const std::exception& e) {
        std::cerr << "打印值时发生异常: " << e.what() << std::endl;
        Error(WriteError, "error printing value: %s", e.what());
    }
}

bool ExecContext::isTrue(Values* val) {
    if (!val) {
        return false;
    }
    
    if (val->IsNull()) {
        return false;
    }
    
    if (val->IsBool()) {
        return val->AsBool();
    }
    
    if (val->IsNumber()) {
        return val->AsNumber() != 0.0;
    }
    
    if (val->IsString()) {
        return !val->AsString().empty();
    }
    
    if (val->IsMap()) {
        return !val->AsMap().empty();
    }
    
    if (val->IsList()) {
        return !val->AsList().empty();
    }
    
    return false;
}

void ExecContext::walkIfOrWith(NodeType type, Values* dot, const BranchNode* node) {
    // 评估条件
    Values* pipeValue = evalPipeline(dot, node->GetPipe());
    
    // 检查条件是否为真
    bool cond = isTrue(pipeValue);
    std::cout << "Checking if value is true: ";
    if (pipeValue->IsNull()) {
        std::cout << "null value -> false" << std::endl;
    } else if (pipeValue->IsBool()) {
        std::cout << "bool value " << (pipeValue->AsBool() ? "true" : "false") << " -> " 
                 << (pipeValue->AsBool() ? "true" : "false") << std::endl;
    } else if (pipeValue->IsNumber()) {
        bool nonZero = fabs(pipeValue->AsNumber()) > 1e-10;
        std::cout << "number value " << pipeValue->AsNumber() << " -> " 
                 << (nonZero ? "true" : "false") << std::endl;
    } else if (pipeValue->IsString()) {
        bool nonEmpty = !pipeValue->AsString().empty();
        std::cout << "string value \"" << pipeValue->AsString() << "\" -> " 
                 << (nonEmpty ? "true" : "false") << std::endl;
    } else {
        std::cout << "other type -> " << (cond ? "true" : "false") << std::endl;
    }
    
    // 根据条件执行相应分支
    if (cond) {
        if (type == NodeWith) {
            // With节点处理：创建新的变量上下文
            std::cout << "执行with节点，创建新的上下文环境" << std::endl;
            int mark = MarkVariables();
            
            // 创建pipeValue副本作为新的"."变量
            Values* newContext = new Values(*pipeValue);
            PushVariable(".", newContext);
            
            // 使用新上下文执行列表
            const ListNode* list = node->List();
            if (list) {
                walk(vars_.back().value, list);
            } else {
                std::cout << "with节点没有主体内容" << std::endl;
            }
            
            // 恢复变量状态
            PopVariables(mark);
            std::cout << "with节点执行完成，恢复原上下文" << std::endl;
        } else {
            // If节点处理：使用原始上下文
        const ListNode* list = node->List();
        walk(dot, list);
        }
    } else if (node->ElseList() != NULL) {
        // 条件为假且有else，执行else列表（使用原始上下文）
        const ListNode* elseList = node->ElseList();
        walk(dot, elseList);
    }
    
    delete pipeValue;
}

// 新增：递归打印 AST 节点的辅助函数
void ExecContext::printNodeTree(const Node* node, int indent) {
    if (!node) {
        std::cout << std::string(indent * 2, ' ') << "[NULL Node]" << std::endl;
        return;
    }

    // 打印当前节点信息
    std::cout << std::string(indent * 2, ' ') << "Node Type: " << node->Type()
              << " (Pos: " << node->Position() << ")";

    // 打印特定类型节点的额外信息（可选，增强可读性）
    if (node->Type() == NodeText) {
         std::cout << " Text: \"" << static_cast<const TextNode*>(node)->Text() << "\"";
    } else if (node->Type() == NodeField) {
         std::cout << " Field: " << static_cast<const FieldNode*>(node)->Ident();
    } else if (node->Type() == NodeIdentifier) {
         std::cout << " Ident: " << static_cast<const IdentifierNode*>(node)->Ident();
    } else if (node->Type() == NodeVariable) {
         std::cout << " Var: " << static_cast<const VariableNode*>(node)->String(); // String() includes '$'
    } else if (node->Type() == NodeString) {
        std::cout << " Str: \"" << static_cast<const StringNode*>(node)->Text() << "\"";
    } else if (node->Type() == NodeNumber) {
         std::cout << " Num: " << static_cast<const NumberNode*>(node)->String();
    } else if (node->Type() == NodeBool) {
         std::cout << " Bool: " << (static_cast<const BoolNode*>(node)->Value() ? "true" : "false");
    }
    std::cout << std::endl;


    // 递归打印子节点
    switch (node->Type()) {
        case NodeList: {
            const ListNode* listNode = static_cast<const ListNode*>(node);
            const std::vector<Node*>& children = listNode->Nodes();
            // std::cout << std::string((indent + 1) * 2, ' ') << "List Children (" << children.size() << ")" << std::endl;
            for (const Node* child : children) {
                printNodeTree(child, indent + 1);
            }
            break;
        }
        case NodeIf:
        case NodeRange:
        case NodeWith: {
            const BranchNode* branchNode = static_cast<const BranchNode*>(node);
            if (branchNode->GetPipe()) {
                 std::cout << std::string((indent + 1) * 2, ' ') << "Condition Pipe:" << std::endl;
                printNodeTree(branchNode->GetPipe(), indent + 1);
            }
            if (branchNode->List()) {
                 std::cout << std::string((indent + 1) * 2, ' ') << "Main List:" << std::endl;
                printNodeTree(branchNode->List(), indent + 1);
            }
            if (branchNode->ElseList()) {
                 std::cout << std::string((indent + 1) * 2, ' ') << "Else List:" << std::endl;
                printNodeTree(branchNode->ElseList(), indent + 1);
            }
            break;
        }
         case NodeAction: {
             const ActionNode* actionNode = static_cast<const ActionNode*>(node);
             if (actionNode->Pipe()) {
                 std::cout << std::string((indent + 1) * 2, ' ') << "Action Pipe:" << std::endl;
                printNodeTree(actionNode->Pipe(), indent + 1);
             }
             break;
         }
         case NodePipe: {
             const PipeNode* pipeNode = static_cast<const PipeNode*>(node);
             const auto& cmds = pipeNode->Cmds();
             const auto& decls = pipeNode->Decl();
             if (!decls.empty()) {
                 std::cout << std::string((indent + 1) * 2, ' ') << "Pipe Declarations (" << decls.size() << ")" << (pipeNode->IsAssign() ? " (Assign =):" : " (Declare :=):") << std::endl;
                 for (const VariableNode* decl : decls) {
                     printNodeTree(decl, indent + 1);
                 }
             }
             // std::cout << std::string((indent + 1) * 2, ' ') << "Pipe Commands (" << cmds.size() << ")" << std::endl;
             for (const CommandNode* cmd : cmds) {
                printNodeTree(cmd, indent + 1);
             }
             break;
         }
         case NodeCommand: {
             const CommandNode* cmdNode = static_cast<const CommandNode*>(node);
             const auto& args = cmdNode->Args();
              // std::cout << std::string((indent + 1) * 2, ' ') << "Command Arguments (" << args.size() << ")" << std::endl;
             for (const Node* arg : args) {
                printNodeTree(arg, indent + 1);
             }
             break;
         }
         case NodeChain: {
            const ChainNode* chainNode = static_cast<const ChainNode*>(node);
             std::cout << std::string((indent + 1) * 2, ' ') << "Chain Base:" << std::endl;
            printNodeTree(chainNode->GetNode(), indent + 1);
             std::cout << std::string((indent + 1) * 2, ' ') << "Chain Fields: ";
             const auto& fields = chainNode->Fields();
             for(size_t i=0; i< fields.size(); ++i) {
                 std::cout << fields[i] << (i == fields.size()-1 ? "" : ", ");
             }
             std::cout << std::endl;
             break;
         }
        // 其他叶子节点类型不需要递归
        default:
            break;
    }
}

} // namespace template_engine