// exec.cpp
#include "exec.h"
#include <stdarg.h>
#include <algorithm>
#include <iostream>
#include <regex>
#include <cmath>

namespace template_engine {

// 辅助函数：格式化错误消息
static std::string formatErrorMessage(const char* format, va_list args) {
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    return std::string(buffer);
}

// FunctionLib实现
FunctionLib::FunctionLib() {
    initBuiltinFunctions();
}

void FunctionLib::AddFunction(const std::string& name, TemplateFn func) {
    functions_[name] = std::move(func);
}

bool FunctionLib::HasFunction(const std::string& name) const {
    return functions_.find(name) != functions_.end();
}

TemplateFn FunctionLib::GetFunction(const std::string& name) const {
    auto it = functions_.find(name);
    if (it != functions_.end()) {
        return it->second;
    }
    throw ExecError(ExecErrorType::RuntimeError, "", "function not found: " + name);
}

// 内置函数初始化
void FunctionLib::initBuiltinFunctions() {
    // eq - 相等比较
    AddFunction("eq", [](const std::vector<std::shared_ptr<Values>>& args) -> std::shared_ptr<Values> {
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
                if (std::abs(args[0]->AsNumber() - args[i]->AsNumber()) > 1e-10) {
                    result = false;
                    break;
                }
            } else if (args[0]->IsBool() && args[i]->IsBool()) {
                if (args[0]->AsBool() != args[i]->AsBool()) {
                    result = false;
                    break;
                }
            } else {
                // 类型不同视为不等
                result = false;
                break;
            }
        }
        
        return Values::MakeBool(result);
    });
    
    // ne - 不等比较
    // AddFunction("ne", [](const std::vector<std::shared_ptr<Values>>& args) -> std::shared_ptr<Values> {
    //     if (args.size() < 2) {
    //         return Values::MakeBool(true);
    //     }
        
    //     auto eqResult = GetFunction("eq")(args);
    //     return Values::MakeBool(!eqResult->AsBool());
    // });
    // ne - 不等比较
    AddFunction("ne", [this](const std::vector<std::shared_ptr<Values>>& args) -> std::shared_ptr<Values> {
        if (args.size() < 2) {
            return Values::MakeBool(true);
        }
        
        auto eqResult = GetFunction("eq")(args);
        return Values::MakeBool(!eqResult->AsBool());
    });
    
    // lt - 小于比较
    AddFunction("lt", [](const std::vector<std::shared_ptr<Values>>& args) -> std::shared_ptr<Values> {
        if (args.size() < 2) {
            return Values::MakeBool(false);
        }
        
        for (size_t i = 0; i < args.size() - 1; ++i) {
            bool lessThan = false;
            
            if (args[i]->IsNumber() && args[i+1]->IsNumber()) {
                lessThan = args[i]->AsNumber() < args[i+1]->AsNumber();
            } else if (args[i]->IsString() && args[i+1]->IsString()) {
                lessThan = args[i]->AsString() < args[i+1]->AsString();
            } else {
                // 类型不同无法比较
                return Values::MakeBool(false);
            }
            
            if (!lessThan) {
                return Values::MakeBool(false);
            }
        }
        
        return Values::MakeBool(true);
    });
    
    // le - 小于等于比较
    AddFunction("le", [this](const std::vector<std::shared_ptr<Values>>& args) -> std::shared_ptr<Values> {
        if (args.size() < 2) {
            return Values::MakeBool(false);
        }
        
        auto ltResult = GetFunction("lt")(args);
        auto eqResult = GetFunction("eq")(args);
        
        return Values::MakeBool(ltResult->AsBool() || eqResult->AsBool());
    });
    
    // gt - 大于比较
    AddFunction("gt", [this](const std::vector<std::shared_ptr<Values>>& args) -> std::shared_ptr<Values> {
        if (args.size() < 2) {
            return Values::MakeBool(false);
        }
        
        // 反转参数顺序
        std::vector<std::shared_ptr<Values>> reversed;
        for (int i = args.size() - 1; i >= 0; --i) {
            reversed.push_back(args[i]);
        }
        
        return GetFunction("lt")(reversed);
    });
    
    // ge - 大于等于比较
    AddFunction("ge", [this](const std::vector<std::shared_ptr<Values>>& args) -> std::shared_ptr<Values> {
        if (args.size() < 2) {
            return Values::MakeBool(false);
        }
        
        // 反转参数顺序
        std::vector<std::shared_ptr<Values>> reversed;
        for (int i = args.size() - 1; i >= 0; --i) {
            reversed.push_back(args[i]);
        }
        
        return GetFunction("le")(reversed);
    });
    
    // and - 逻辑与
    AddFunction("and", [](const std::vector<std::shared_ptr<Values>>& args) -> std::shared_ptr<Values> {
        for (const auto& arg : args) {
            if (!arg->IsBool() || !arg->AsBool()) {
                return Values::MakeBool(false);
            }
        }
        return Values::MakeBool(true);
    });
    
    // or - 逻辑或
    AddFunction("or", [](const std::vector<std::shared_ptr<Values>>& args) -> std::shared_ptr<Values> {
        for (const auto& arg : args) {
            if (arg->IsBool() && arg->AsBool()) {
                return Values::MakeBool(true);
            }
        }
        return Values::MakeBool(false);
    });
    
    // not - 逻辑非
    AddFunction("not", [](const std::vector<std::shared_ptr<Values>>& args) -> std::shared_ptr<Values> {
        if (args.empty()) {
            return Values::MakeBool(true);
        }
        
        if (!args[0]->IsBool()) {
            return Values::MakeBool(false);
        }
        
        return Values::MakeBool(!args[0]->AsBool());
    });
    
    // len - 长度
    AddFunction("len", [](const std::vector<std::shared_ptr<Values>>& args) -> std::shared_ptr<Values> {
        if (args.empty()) {
            return Values::MakeNumber(0);
        }
        
        if (args[0]->IsString()) {
            return Values::MakeNumber(static_cast<double>(args[0]->AsString().length()));
        } else if (args[0]->IsList()) {
            return Values::MakeNumber(static_cast<double>(args[0]->AsList().size()));
        } else if (args[0]->IsMap()) {
            return Values::MakeNumber(static_cast<double>(args[0]->AsMap().size()));
        }
        
        return Values::MakeNumber(0);
    });
    
    // index - 获取列表或字符串中的元素
    AddFunction("index", [](const std::vector<std::shared_ptr<Values>>& args) -> std::shared_ptr<Values> {
        if (args.size() < 2) {
            return Values::MakeNull();
        }
        
        if (args[0]->IsList()) {
            if (!args[1]->IsNumber()) {
                return Values::MakeNull();
            }
            
            int index = static_cast<int>(args[1]->AsNumber());
            const auto& list = args[0]->AsList();
            
            if (index < 0 || index >= list.size()) {
                return Values::MakeNull();
            }
            
            return list[index]->DeepCopy();
        } else if (args[0]->IsString()) {
            if (!args[1]->IsNumber()) {
                return Values::MakeNull();
            }
            
            int index = static_cast<int>(args[1]->AsNumber());
            const auto& str = args[0]->AsString();
            
            if (index < 0 || index >= str.length()) {
                return Values::MakeNull();
            }
            
            return Values::MakeString(std::string(1, str[index]));
        } else if (args[0]->IsMap()) {
            if (!args[1]->IsString()) {
                return Values::MakeNull();
            }
            
            const auto& key = args[1]->AsString();
            const auto& map = args[0]->AsMap();
            
            auto it = map.find(key);
            if (it == map.end()) {
                return Values::MakeNull();
            }
            
            return it->second->DeepCopy();
        }
        
        return Values::MakeNull();
    });
    
    // print - 调试打印
    AddFunction("print", [](const std::vector<std::shared_ptr<Values>>& args) -> std::shared_ptr<Values> {
        std::stringstream result;
        
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0) {
                result << " ";
            }
            
            if (args[i]->IsString()) {
                result << args[i]->AsString();
            } else if (args[i]->IsNumber()) {
                result << args[i]->AsNumber();
            } else if (args[i]->IsBool()) {
                result << (args[i]->AsBool() ? "true" : "false");
            } else if (args[i]->IsNull()) {
                result << "null";
            } else if (args[i]->IsMap()) {
                result << "{map}";
            } else if (args[i]->IsList()) {
                result << "[list]";
            }
        }
        
        return Values::MakeString(result.str());
    });
    
    // 这里可以添加更多内置函数，如 default, empty, trim, upper, lower 等
}

// ExecContext实现
ExecContext::ExecContext(
    std::shared_ptr<Tree> tmpl,
    std::ostream& writer,
    std::shared_ptr<Values> data,
    FunctionLib& funcs,
    const ExecOptions& options)
    : tmpl_(tmpl), writer_(writer), funcs_(funcs), options_(options) {
    
    // 初始化顶层变量("$")
    PushVariable("$", data);
}

void ExecContext::Execute() {
    if (!tmpl_ || !tmpl_->GetRoot()) {
        Error(ExecErrorType::RuntimeError, "incomplete or empty template");
    }
    
    try {
        walk(GetVariable("$"), tmpl_->GetRoot());
    } catch (const ExecError& e) {
        throw; // 重新抛出执行错误
    } catch (const std::exception& e) {
        Error(ExecErrorType::RuntimeError, "execution error: %s", e.what());
    }
}

std::shared_ptr<Tree> ExecContext::GetTemplate() const {
    return tmpl_;
}

std::ostream& ExecContext::GetWriter() {
    return writer_;
}

void ExecContext::PushVariable(const std::string& name, std::shared_ptr<Values> value) {
    vars_.emplace_back(name, std::move(value));
}

int ExecContext::MarkVariables() {
    return vars_.size();
}

void ExecContext::PopVariables(int mark) {
    if (mark >= 0 && mark <= vars_.size()) {
        vars_.resize(mark);
    }
}

void ExecContext::SetVariable(const std::string& name, std::shared_ptr<Values> value) {
    for (int i = vars_.size() - 1; i >= 0; --i) {
        if (vars_[i].name == name) {
            vars_[i].value = std::move(value);
            return;
        }
    }
    
    Error(ExecErrorType::UndefinedVariable, "undefined variable: %s", name.c_str());
}

void ExecContext::SetTopVariable(int n, std::shared_ptr<Values> value) {
    if (n > 0 && vars_.size() >= n) {
        vars_[vars_.size() - n].value = std::move(value);
    }
}

std::shared_ptr<Values> ExecContext::GetVariable(const std::string& name) {
    for (int i = vars_.size() - 1; i >= 0; --i) {
        if (vars_[i].name == name) {
            return vars_[i].value;
        }
    }
    
    Error(ExecErrorType::UndefinedVariable, "undefined variable: %s", name.c_str());
    return nullptr; // 不会到达这里
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
        auto [locStr, ctxStr] = tmpl_->ErrorContext(currentNode_);
        location = locStr;
        context = ctxStr;
        
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
        Error(ExecErrorType::RecursionLimit, "exceeded maximum template depth (%d)", options_.maxExecDepth);
    }
}

void ExecContext::DecrementDepth() {
    depth_--;
}

void ExecContext::walk(std::shared_ptr<Values> dot, const Node* node) {
    if (!node) return;
    
    // 保存当前节点用于错误报告
    currentNode_ = node;
    
    try {
        switch (node->Type()) {
            case NodeType::NodeText: {
                const auto* text = static_cast<const TextNode*>(node);
                writer_ << text->Text();
                break;
            }
            case NodeType::NodeAction: {
                const auto* action = static_cast<const ActionNode*>(node);
                auto val = evalPipeline(dot, action->Pipe());
                if (val && !val->IsNull()) {
                    PrintValue(node, val);
                }
                break;
            }
            case NodeType::NodeIf: {
                const auto* ifNode = static_cast<const IfNode*>(node);
                walkIfOrWith(NodeType::NodeIf, dot, ifNode);
                break;
            }
            case NodeType::NodeWith: {
                const auto* withNode = static_cast<const WithNode*>(node);
                walkIfOrWith(NodeType::NodeWith, dot, withNode);
                break;
            }
            case NodeType::NodeRange: {
                const auto* rangeNode = static_cast<const RangeNode*>(node);
                walkRange(dot, rangeNode);
                break;
            }
            case NodeType::NodeTemplate: {
                const auto* tmplNode = static_cast<const TemplateNode*>(node);
                walkTemplate(dot, tmplNode);
                break;
            }
            case NodeType::NodeList: {
                const auto* list = static_cast<const ListNode*>(node);
                for (const auto& child : list->Nodes()) {
                    walk(dot, child.get());
                }
                break;
            }
            case NodeType::NodeComment:
                // 注释不做任何事
                break;
            default:
                Error(ExecErrorType::RuntimeError, "unknown node type: %d", 
                      static_cast<int>(node->Type()));
        }
    } catch (const ExecError&) {
        throw; // 重新抛出执行错误
    } catch (const std::exception& e) {
        Error(ExecErrorType::RuntimeError, "execution error: %s", e.what());
    }
}

void ExecContext::walkIfOrWith(NodeType type, std::shared_ptr<Values> dot, const BranchNode* node) {
    std::cout << "执行条件分支: " << (type == NodeType::NodeIf ? "if" : "with") << std::endl;
    
    // 记录变量状态
    int mark = MarkVariables();
    
    // 求值管道
    auto val = evalPipeline(dot, node->GetPipe());
    
    // 输出管道的值类型，帮助调试
    std::cout << "  管道结果类型: " << 
        (val ? (val->IsNull() ? "null" : 
               (val->IsMap() ? "map" : 
               (val->IsList() ? "list" : 
               (val->IsString() ? "string" : 
               (val->IsNumber() ? "number" : 
               (val->IsBool() ? "bool" : "unknown")))))) : "nullptr") << std::endl;
    
    // 检查真值
    bool truth = isTrue(val);
    std::cout << "  条件结果: " << (truth ? "true" : "false") << std::endl;
    
    if (truth) {
        // 如果是with，使用管道值作为新的dot；否则保持原来的dot
        if (type == NodeType::NodeWith) {
            std::cout << "  执行with分支，使用新的dot" << std::endl;
            walk(val, node->List());
        } else {
            std::cout << "  执行if分支，使用原始dot" << std::endl;
            walk(dot, node->List());
        }
    } else if (node->ElseList()) {
        // 执行else分支
        std::cout << "  执行else分支" << std::endl;
        walk(dot, node->ElseList());
    } else {
        std::cout << "  条件为false且没有else分支，不执行任何内容" << std::endl;
    }
    
    // 恢复变量状态
    PopVariables(mark);
}


bool ExecContext::isTrue(std::shared_ptr<Values> val) {
    std::cerr << "Checking if value is true: ";
    
    if (!val) {
        std::cerr << "null pointer -> false" << std::endl;
        return false;
    }
    
    if (val->IsNull()) {
        std::cerr << "null value -> false" << std::endl;
        return false;
    }
    
    if (val->IsBool()) {
        bool result = val->AsBool();
        std::cerr << "bool: " << (result ? "true" : "false") << std::endl;
        return result;
    }
    
    if (val->IsNumber()) {
        double num = val->AsNumber();
        bool result = num != 0.0;
        std::cerr << "number: " << num << " -> " << (result ? "true" : "false") << std::endl;
        return result;
    }
    
    if (val->IsString()) {
        std::string str = val->AsString();
        bool result = !str.empty();
        std::cerr << "string: \"" << str << "\" -> " << (result ? "true" : "false") << std::endl;
        return result;
    }
    
    if (val->IsMap()) {
        bool result = !val->AsMap().empty();
        std::cerr << "map (size: " << val->AsMap().size() << ") -> " << (result ? "true" : "false") << std::endl;
        return result;
    }
    
    if (val->IsList()) {
        bool result = !val->AsList().empty();
        std::cerr << "list (size: " << val->AsList().size() << ") -> " << (result ? "true" : "false") << std::endl;
        return result;
    }
    
    std::cerr << "unknown type -> false" << std::endl;
    return false;
}

void ExecContext::walkRange(std::shared_ptr<Values> dot, const RangeNode* node) {
    // 记录变量状态
    int mark = MarkVariables();
    
    // 求值管道
    auto val = evalPipeline(dot, node->GetPipe());
    
    bool rangeOver = false;
    const PipeNode* pipe = node->GetPipe();
    
    try {
        if (val->IsList()) {
            auto& list = val->AsList();
            if (list.empty()) {
                rangeOver = true;
            } else {
                // 遍历列表
                for (size_t i = 0; i < list.size(); ++i) {
                    // 为range循环设置变量
                    if (!pipe->Decl().empty()) {
                        if (pipe->IsAssign()) {
                            if (pipe->Decl().size() > 1) {
                                // 两个变量：索引和值
                                SetVariable(pipe->Decl()[0]->Ident()[0], Values::MakeNumber(i));
                                SetVariable(pipe->Decl()[1]->Ident()[0], list[i]);
                            } else {
                                // 一个变量：值
                                SetVariable(pipe->Decl()[0]->Ident()[0], list[i]);
                            }
                        } else {
                            if (pipe->Decl().size() > 1) {
                                // 两个变量
                                SetTopVariable(2, Values::MakeNumber(i));
                                SetTopVariable(1, list[i]);
                            } else {
                                // 一个变量
                                SetTopVariable(1, list[i]);
                            }
                        }
                    }
                    
                    // 执行循环体
                    walk(list[i], node->List());
                }
            }
        } else if (val->IsMap()) {
            auto& map = val->AsMap();
            if (map.empty()) {
                rangeOver = true;
            } else {
                // 对map按键排序以保持一致性
                std::vector<std::string> keys;
                for (const auto& [key, _] : map) {
                    keys.push_back(key);
                }
                std::sort(keys.begin(), keys.end());
                
                // 遍历map
                for (const auto& key : keys) {
                    // 为range循环设置变量
                    if (!pipe->Decl().empty()) {
                        if (pipe->IsAssign()) {
                            if (pipe->Decl().size() > 1) {
                                // 两个变量：键和值
                                SetVariable(pipe->Decl()[0]->Ident()[0], Values::MakeString(key));
                                SetVariable(pipe->Decl()[1]->Ident()[0], map[key]);
                            } else {
                                // 一个变量：值
                                SetVariable(pipe->Decl()[0]->Ident()[0], map[key]);
                            }
                        } else {
                            if (pipe->Decl().size() > 1) {
                                // 两个变量
                                SetTopVariable(2, Values::MakeString(key));
                                SetTopVariable(1, map[key]);
                            } else {
                                // 一个变量
                                SetTopVariable(1, map[key]);
                            }
                        }
                    }
                    
                    // 执行循环体
                    walk(map[key], node->List());
                }
            }
        } else {
            // 不是可遍历的类型
            rangeOver = true;
        }
    } catch (const std::exception& e) {
        // 捕获异常，确保在任何情况下都恢复变量状态
        PopVariables(mark);
        throw;
    }
    
    // 如果没有可遍历的内容，执行else分支
    if (rangeOver && node->ElseList()) {
        walk(dot, node->ElseList());
    }
    
    // 恢复变量状态
    PopVariables(mark);
}

void ExecContext::walkTemplate(std::shared_ptr<Values> dot, const TemplateNode* node) {
    // 获取模板名称
    std::string name = node->Name();
    
    // 计算管道值
    std::shared_ptr<Values> pipeVal = dot;
    if (node->Pipe()) {
        pipeVal = evalPipeline(dot, node->Pipe());
    }
    
    // 查找要包含的模板
    std::shared_ptr<Tree> tmpl = nullptr;
    
    // 查找缓存
    auto it = templateCache_.find(name);
    if (it != templateCache_.end()) {
        tmpl = it->second;
    } else {
        // TODO: 在实际应用中，应该从模板存储中加载模板
        // 简化版：仅支持当前模板的引用
        Error(ExecErrorType::RuntimeError, "template not found: %s", name.c_str());
    }
    
    // 递增执行深度
    IncrementDepth();
    
    // 创建新的执行上下文
    ExecContext newCtx(tmpl, writer_, pipeVal, funcs_, options_);
    newCtx.vars_ = {{"$", pipeVal}};  // 只保留顶层变量
    
    // 执行子模板
    newCtx.Execute();
    
    // 递减执行深度
    DecrementDepth();
}

std::shared_ptr<Values> ExecContext::evalPipeline(std::shared_ptr<Values> dot, const PipeNode* pipe) {
    if (!pipe) {
        return Values::MakeNull();
    }
    
    // 记录变量状态
    int mark = MarkVariables();
    
    // 初始值为空
    std::shared_ptr<Values> value = nullptr;
    
    // 执行管道中的所有命令
    for (const auto& cmd : pipe->Cmds()) {
        value = evalCommand(dot, cmd.get(), value);
    }
    
    // 处理变量声明
    for (const auto& var : pipe->Decl()) {
        if (pipe->IsAssign()) {
            SetVariable(var->Ident()[0], value);
        } else {
            PushVariable(var->Ident()[0], value);
        }
    }
    
    // 如果不需要保留变量，恢复变量状态
    if (pipe->Decl().empty()) {
        PopVariables(mark);
    }
    
    return value;
}

std::shared_ptr<Values> ExecContext::evalCommand(
    std::shared_ptr<Values> dot, 
    const CommandNode* cmd, 
    std::shared_ptr<Values> final) {
    
    // 检查参数列表
    if (cmd->Args().empty()) {
        Error(ExecErrorType::RuntimeError, "empty command");
    }
    
    // 获取第一个参数（函数名或字段名）
    const Node* firstArg = cmd->Args()[0].get();



    // 检查是否是函数调用
    if (firstArg->Type() == NodeType::NodeIdentifier) {
        std::string funcName = static_cast<const IdentifierNode*>(firstArg)->Ident();
        std::cout << "  函数调用: " << funcName << std::endl;
        
        // 创建参数列表
        std::vector<const Node*> funcArgs;
        for (size_t i = 1; i < cmd->Args().size(); ++i) {
            funcArgs.push_back(cmd->Args()[i].get());
            std::cout << "  函数参数 " << i << ": " << cmd->Args()[i]->String() << std::endl;
        }
        
        return evalFunction(dot, funcName, cmd, funcArgs, final);
    }


    
    // 准备空的参数列表
    std::vector<const Node*> emptyArgs;
    
    switch (firstArg->Type()) {
        case NodeType::NodeField:
            return evalField(dot, static_cast<const FieldNode*>(firstArg)->String(), 
                           firstArg, emptyArgs, final, dot);
        case NodeType::NodeIdentifier: {
            // 如果有参数，手动创建参数列表
            std::vector<const Node*> cmdArgs;
            for (size_t i = 1; i < cmd->Args().size(); ++i) {
                cmdArgs.push_back(cmd->Args()[i].get());
            }
            return evalFunction(dot, static_cast<const IdentifierNode*>(firstArg)->Ident(), 
                              cmd, cmdArgs, final);
        }
        case NodeType::NodeChain: {
            const auto* chain = static_cast<const ChainNode*>(firstArg);
            return evalFieldChain(dot, evalArg(dot, chain->GetNode()), 
                                chain, chain->Field(), 
                                emptyArgs, final);
        }
        case NodeType::NodeVariable: {
            const auto* var = static_cast<const VariableNode*>(firstArg);
            auto val = GetVariable(var->Ident()[0]);
            if (var->Ident().size() > 1) {
                // 处理 $x.Field 这种情况
                std::vector<std::string> fieldPath(var->Ident().begin() + 1, var->Ident().end());
                return evalFieldChain(dot, val, var, fieldPath, emptyArgs, final);
            }
            return val;
        }
        default: {
            // 如果第一个参数不是函数或字段，则计算其值
            auto val = evalArg(dot, firstArg);
            
            // 如果有附加参数，报错
            if (cmd->Args().size() > 1 || final) {
                Error(ExecErrorType::RuntimeError, "can't give argument to non-function %s", 
                      firstArg->String().c_str());
            }
            
            return val;
        }
    }
}

std::shared_ptr<Values> ExecContext::evalFunction(
    std::shared_ptr<Values> dot, 
    const std::string& name, 
    const CommandNode* cmd, 
    const std::vector<const Node*>& args, 
    std::shared_ptr<Values> final) {
    
    // 检查函数是否存在
    if (!funcs_.HasFunction(name)) {
        Error(ExecErrorType::RuntimeError, "function not found: %s", name.c_str());
    }

    // 处理eq函数
    // 在 evalFunction 方法中
    if (name == "eq") {
        if (args.size() < 2) {
            Error(ExecErrorType::RuntimeError, "eq函数需要至少两个参数");
        }
        
        // 计算第一个参数
        auto firstArg = evalArg(dot, args[0]);
        std::cout << "  第一个参数: ";
        if (firstArg) {
            if (firstArg->IsString()) std::cout << "字符串 \"" << firstArg->AsString() << "\"";
            else if (firstArg->IsNumber()) std::cout << "数字 " << firstArg->AsNumber();
            else if (firstArg->IsBool()) std::cout << "布尔 " << (firstArg->AsBool() ? "true" : "false");
            else std::cout << "其他类型";
        } else {
            std::cout << "空值";
        }
        std::cout << std::endl;
        
        // 计算第二个参数
        auto secondArg = evalArg(dot, args[1]);
        std::cout << "  第二个参数: ";
        if (secondArg) {
            if (secondArg->IsString()) std::cout << "字符串 \"" << secondArg->AsString() << "\"";
            else if (secondArg->IsNumber()) std::cout << "数字 " << secondArg->AsNumber();
            else if (secondArg->IsBool()) std::cout << "布尔 " << (secondArg->AsBool() ? "true" : "false");
            else std::cout << "其他类型";
        } else {
            std::cout << "空值";
        }
        std::cout << std::endl;
        
        // 比较参数
        bool equal = false;
        if (firstArg && secondArg) {
            if (firstArg->IsString() && secondArg->IsString()) {
                equal = (firstArg->AsString() == secondArg->AsString());
                std::cout << "  比较字符串: \"" << firstArg->AsString() << "\" == \"" << secondArg->AsString() << "\" -> " << (equal ? "true" : "false") << std::endl;
            } else if (firstArg->IsNumber() && secondArg->IsNumber()) {
                equal = (std::abs(firstArg->AsNumber() - secondArg->AsNumber()) < 1e-10);
            } else if (firstArg->IsBool() && secondArg->IsBool()) {
                equal = (firstArg->AsBool() == secondArg->AsBool());
            }
        }
        
        std::cout << "  比较结果: " << (equal ? "相等" : "不相等") << std::endl;
        return Values::MakeBool(equal);
    }
    
    TemplateFn func = funcs_.GetFunction(name);
    
    // 特殊处理 and 和 or 函数，它们需要短路评估
    if (name == "and" || name == "or") {
        std::vector<std::shared_ptr<Values>> funcArgs;
        
        // 使用原始指针而不是直接访问cmd->Args()
        for (size_t i = 1; i < cmd->Args().size(); ++i) {
            auto argVal = evalArg(dot, cmd->Args()[i].get());
            funcArgs.push_back(argVal);
            
            // 短路评估
            bool shortCircuit = (name == "and" && !isTrue(argVal)) || 
                               (name == "or" && isTrue(argVal));
            
            if (shortCircuit) {
                return name == "and" ? Values::MakeBool(false) : Values::MakeBool(true);
            }
        }
        
        if (final) {
            funcArgs.push_back(final);
        }
        
        return func(funcArgs);
    }
    
    // 处理常规函数 - 手动创建包含原始指针的vector
    std::vector<const Node*> cmdArgs;
    for (size_t i = 1; i < cmd->Args().size(); ++i) {
        cmdArgs.push_back(cmd->Args()[i].get());
    }
    
    // return evalCall(dot, func, cmd, name, cmdArgs, final);
    Error(ExecErrorType::RuntimeError, "未知函数: %s", name.c_str());
    return Values::MakeNull();
}

std::shared_ptr<Values> ExecContext::evalField(
    std::shared_ptr<Values> dot, 
    const std::string& fieldName, 
    const Node* node, 
    const std::vector<const Node*>& args, 
    std::shared_ptr<Values> final, 
    std::shared_ptr<Values> receiver) {

    std::cout << "Evaluating field: " << fieldName;
    if (receiver) {
        std::cout << " in " << (receiver->IsMap() ? "map" : (receiver->IsList() ? "list" : "other"));
    }
    std::cout << std::endl;
    
    if (!receiver || receiver->IsNull()) {
        return Values::MakeNull();
    }
    
    // 尝试查找完整的字段路径
    auto fieldValue = receiver->PathValue(fieldName);
    if (fieldValue && *fieldValue) {
        std::cout << "  找到字段: " << fieldName << " 通过PathValue" << std::endl;
        return *fieldValue;
    }
    
    // 如果使用PathValue失败，尝试使用原始的逻辑
    if (receiver->IsMap()) {
        auto& map = receiver->AsMap();
        auto it = map.find(fieldName);
        
        if (it != map.end() && it->second) {
            std::cout << "  找到字段: " << fieldName << " 在map中" << std::endl;
            return it->second;
        }
        
        std::cout << "  未找到字段: " << fieldName << std::endl;
        std::cout << "  可用的键: ";
        for (const auto& [key, _] : map) {
            std::cout << key << " ";
        }
        std::cout << std::endl;
    }
    
    return Values::MakeNull();
}

// std::shared_ptr<Values> ExecContext::evalFieldChain(
//     std::shared_ptr<Values> dot, 
//     std::shared_ptr<Values> receiver, 
//     const Node* node, 
//     const std::vector<std::string>& ident, 
//     const std::vector<const Node*>& args, 
//     std::shared_ptr<Values> final) {

//     std::cerr << "Evaluating field chain: ";
//     for (const auto& id : ident) {
//         std::cerr << id << " ";
//     }
//     std::cerr << std::endl;

    
//     // 处理字段链（如 .X.Y.Z）
//     if (ident.empty()) {
//         Error(ExecErrorType::RuntimeError, "empty field chain");
//     }
    
//     int n = ident.size();
    
//     // 遍历除最后一个字段外的所有字段
//     for (int i = 0; i < n - 1; ++i) {
//         receiver = evalField(dot, ident[i], node, std::vector<const Node*>(), 
//                             nullptr, receiver);
//     }
    
//     // 现在处理最后一个字段，可能带有参数
//     return evalField(dot, ident[n - 1], node, args, final, receiver);
// }

std::shared_ptr<Values> ExecContext::evalFieldChain(
    std::shared_ptr<Values> dot, 
    std::shared_ptr<Values> receiver, 
    const Node* node, 
    const std::vector<std::string>& ident, 
    const std::vector<const Node*>& args, 
    std::shared_ptr<Values> final) {

    std::cout << "Evaluating field chain: ";
    for (const auto& id : ident) {
        std::cout << id << " ";
    }
    std::cout << std::endl;
    
    if (ident.empty()) {
        Error(ExecErrorType::RuntimeError, "empty field chain");
    }
    
    // 处理可能包含点的字段路径，如 "Name.first"
    std::string fullPath = ident[0];
    std::cout << "  完整路径: " << fullPath << std::endl;
    
    // 直接使用完整路径访问，假设 evalField 可以处理嵌套路径
    return evalField(dot, fullPath, node, args, final, receiver);
}


std::shared_ptr<Values> ExecContext::evalCall(
    std::shared_ptr<Values> dot, 
    TemplateFn func, 
    const Node* node, 
    const std::string& name, 
    const std::vector<const Node*>& args, 
    std::shared_ptr<Values> final) {
    
    // 准备函数参数
    std::vector<std::shared_ptr<Values>> funcArgs;
    
    // 评估所有参数
    for (const auto* arg : args) {
        funcArgs.push_back(evalArg(dot, arg));
    }
    
    // 添加管道的前一个值作为最后一个参数
    if (final) {
        funcArgs.push_back(final);
    }
    
    // 调用函数
    try {
        return func(funcArgs);
    } catch (const std::exception& e) {
        Error(ExecErrorType::RuntimeError, "error calling %s: %s", name.c_str(), e.what());
    }
    
    return Values::MakeNull(); // 不会到达这里
}

std::shared_ptr<Values> ExecContext::evalArg(std::shared_ptr<Values> dot, const Node* n) {
    if (!n) {
        return Values::MakeNull();
    }
    
    switch (n->Type()) {
        case NodeType::NodeDot:
            return dot;
        
        case NodeType::NodeNil:
            return Values::MakeNull();

        case NodeType::NodeBool:
            return Values::MakeBool(static_cast<const BoolNode*>(n)->Value());

        case NodeType::NodeNumber:
            return Values::MakeNumber(static_cast<const NumberNode*>(n)->Float64());

        case NodeType::NodeString: {
            const auto* str = static_cast<const StringNode*>(n);
            std::cout << "  字符串参数: " << str->Text() << std::endl;
            return Values::MakeString(str->Text());
        }
        


        case NodeType::NodeVariable: {
            const auto* var = static_cast<const VariableNode*>(n);
            return GetVariable(var->Ident()[0]);
        }


        // case NodeType::NodeField: {
        //     const auto* field = static_cast<const FieldNode*>(n);
        //     return evalFieldChain(dot, dot, field, field->Ident(), 
        //                         std::vector<const Node*>(), nullptr);
        // }

        case NodeType::NodeField: {
            const auto* field = static_cast<const FieldNode*>(n);
            std::cout << "  字段参数: " << field->String() << std::endl;
            
            // 获取完整的字段路径
            std::string fullPath = field->String();
            
            // 使用 PathValue 访问嵌套字段
            auto result = dot->PathValue(fullPath);

            if (result.has_value()) {
                return result.value(); // 返回值
            } else {
                // 处理没有值的情况，例如返回空指针
                return nullptr;
            }
            // if (result) {
            //     // std::cout << "  字段值类型: " << 
            //     //     (result->IsNull() ? "null" : 
            //     //     (result->IsString() ? "string" : 
            //     //     (result->IsNumber() ? "number" : 
            //     //     (result->IsBool() ? "bool" : 
            //     //     (result->IsMap() ? "map" : 
            //     //     (result->IsList() ? "list" : "unknown")))))) << std::endl;
            //     return result;
            // }
            
            std::cout << "  字段不存在: " << fullPath << std::endl;
            return Values::MakeNull();
        }


        case NodeType::NodeChain: {
            const auto* chain = static_cast<const ChainNode*>(n);
            return evalFieldChain(dot, evalArg(dot, chain->GetNode()), 
                                chain, chain->Field(), 
                                std::vector<const Node*>(), nullptr);
        }
        default:
            Error(ExecErrorType::RuntimeError, "can't handle %s in argument", n->String().c_str());
            return Values::MakeNull(); // 不会到达这里
    }
}


void ExecContext::PrintValue(const Node* node, std::shared_ptr<Values> value) {
    try {
        if (!value) {
            writer_ << "<nil>";
            return;
        }
        
        if (value->IsString()) {
            writer_ << value->AsString();
        } else if (value->IsNumber()) {
            writer_ << value->AsNumber();
        } else if (value->IsBool()) {
            writer_ << (value->AsBool() ? "true" : "false");
        } else if (value->IsNull()) {
            writer_ << "<nil>";
        } else if (value->IsMap()) {
            writer_ << "map[";
            bool first = true;
            for (const auto& [key, val] : value->AsMap()) {
                if (!first) writer_ << " ";
                first = false;
                writer_ << key << ":";
                if (val) {
                    if (val->IsString()) {
                        writer_ << val->AsString();
                    } else {
                        writer_ << "...";
                    }
                } else {
                    writer_ << "<nil>";
                }
            }
            writer_ << "]";
        } else if (value->IsList()) {
            writer_ << "[";
            bool first = true;
            for (const auto& item : value->AsList()) {
                if (!first) writer_ << " ";
                first = false;
                if (item) {
                    if (item->IsString()) {
                        writer_ << item->AsString();
                    } else {
                        writer_ << "...";
                    }
                } else {
                    writer_ << "<nil>";
                }
            }
            writer_ << "]";
        }
    } catch (const std::exception& e) {
        Error(ExecErrorType::WriteError, "error printing value: %s", e.what());
    }
}

std::shared_ptr<Values> ExecContext::FindField(
    std::shared_ptr<Values> value, 
    const std::string& name) {
    
    if (!value || !value->IsMap()) {
        return nullptr;
    }
    
    auto& map = value->AsMap();
    auto it = map.find(name);
    if (it != map.end()) {
        return it->second;
    }
    
    return nullptr;
}

void ExecContext::IncludeTemplate(const std::string& name, std::shared_ptr<Values> data) {
    // 在真实场景中，这将从模板存储中加载模板
    // 在这个简化版中，我们假设模板已在缓存中
    auto it = templateCache_.find(name);
    if (it == templateCache_.end()) {
        Error(ExecErrorType::RuntimeError, "template not found: %s", name.c_str());
    }
    
    // 增加执行深度
    IncrementDepth();
    
    // 创建新上下文并执行模板
    ExecContext newCtx(it->second, writer_, data, funcs_, options_);
    newCtx.Execute();
    
    // 减少执行深度
    DecrementDepth();
}

// 执行模板的便捷函数
std::string ExecuteTemplate(
    const std::string& templateName,
    const std::string& templateContent,
    std::shared_ptr<Values> data,
    const std::string& leftDelim,
    const std::string& rightDelim,
    const ExecOptions& options) {
    
    // 解析模板
    auto templateMap = Tree::Parse(templateName, templateContent, leftDelim, rightDelim);
    
    if (templateMap.empty()) {
        throw ExecError(ExecErrorType::RuntimeError, templateName, 
                       "failed to parse template");
    }
    
    auto it = templateMap.find(templateName);
    if (it == templateMap.end() || !it->second) {
        throw ExecError(ExecErrorType::RuntimeError, templateName, 
                       "template not found after parsing");
    }
    
    // 设置函数库
    FunctionLib funcs;
    
    // 准备输出流
    std::stringstream output;
    
    // 创建执行上下文并执行模板
    ExecContext ctx(it->second, output, data, funcs, options);
    ctx.Execute();
    
    return output.str();
}

} // namespace template_engine