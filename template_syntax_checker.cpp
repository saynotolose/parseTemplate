#include "template_syntax_checker.h"
#include "parse.h"
#include "values.h"
#include <sstream>
#include <cctype>

namespace template_engine {

// 辅助：获取某一行内容
static std::string GetLineContext(const std::string& str, int line) {
    std::istringstream iss(str);
    std::string l;
    int cur = 1;
    while (std::getline(iss, l)) {
        if (cur == line) return l;
        ++cur;
    }
    return "";
}

// 辅助：根据Position推算行号
static int GetLineByPos(const std::string& str, size_t pos) {
    int line = 1;
    for (size_t i = 0; i < pos && i < str.size(); ++i) {
        if (str[i] == '\n') ++line;
    }
    return line;
}

// 辅助：检查定界符配对
static bool CheckDelimiters(const std::string& str, const std::string& left, const std::string& right, int& countL, int& countR) {
    countL = 0; countR = 0;
    size_t pos = 0;
    while ((pos = str.find(left, pos)) != std::string::npos) { ++countL; pos += left.size(); }
    pos = 0;
    while ((pos = str.find(right, pos)) != std::string::npos) { ++countR; pos += right.size(); }
    return countL == countR;
}

// 辅助：检查变量路径是否合法
static bool IsValidVarPath(const std::string& path) {
    if (path.empty() || path[0] != '.') return false;
    char prev = 0;
    for (size_t i = 0; i < path.size(); ++i) {
        char c = path[i];
        if (c == '.') {
            if (prev == '.') return false; // 连续点
        } else if (!(std::isalnum((unsigned char)c) || c == '_' || c == '-')) {
            return false;
        }
        prev = c;
    }
    return true;
}

// 递归遍历AST，检查变量路径
static void CheckVarNodes(const Node* node, std::vector<TemplateSyntaxError>& errors, const std::string& tpl, int parentLine) {
    if (!node) return;
    int line = parentLine;
    // 优先用Line()，否则用Position()
    if (node->Type() == NodeAction) {
        const ActionNode* act = static_cast<const ActionNode*>(node);
        line = act->Line();
    } else if (node->Type() == NodeIf || node->Type() == NodeRange || node->Type() == NodeWith) {
        const BranchNode* b = static_cast<const BranchNode*>(node);
        line = b->Line();
    } else if (node->Type() == NodeElse) {
        const ElseNode* e = static_cast<const ElseNode*>(node);
        line = e->Line();
    } else if (node->Type() == NodeBreak) {
        const BreakNode* b = static_cast<const BreakNode*>(node);
        line = b->Line();
    } else if (node->Type() == NodeContinue) {
        const ContinueNode* c = static_cast<const ContinueNode*>(node);
        line = c->Line();
    } else if (node->Type() == NodeTemplate) {
        const TemplateNode* t = static_cast<const TemplateNode*>(node);
        line = t->Line();
    } else {
        line = GetLineByPos(tpl, node->Position());
    }
    if (node->Type() == NodeVariable) {
        const VariableNode* v = static_cast<const VariableNode*>(node);
        std::string path = v->Ident();
        if (!IsValidVarPath(path)) {
            TemplateSyntaxError err;
            err.type = ErrorType_VariablePath;
            err.line = line;
            err.column = 1;
            err.message = "非法变量路径: " + path;
            err.context = GetLineContext(tpl, err.line);
            err.description = "变量路径只能以.开头，且只允许字母、数字、下划线、减号和点，且不能有连续点。";
            errors.push_back(err);
        }
    }
    // 递归子节点
    switch (node->Type()) {
        case NodeList: {
            const ListNode* list = static_cast<const ListNode*>(node);
            const std::vector<Node*>& nodes = list->Nodes();
            for (size_t i = 0; i < nodes.size(); ++i) {
                CheckVarNodes(nodes[i], errors, tpl, line);
            }
            break;
        }
        case NodeIf:
        case NodeRange:
        case NodeWith: {
            const BranchNode* b = static_cast<const BranchNode*>(node);
            if (b->GetPipe()) CheckVarNodes(b->GetPipe(), errors, tpl, line);
            if (b->List()) CheckVarNodes(b->List(), errors, tpl, line);
            if (b->ElseList()) CheckVarNodes(b->ElseList(), errors, tpl, line);
            break;
        }
        case NodeAction: {
            const ActionNode* act = static_cast<const ActionNode*>(node);
            if (act->Pipe()) CheckVarNodes(act->Pipe(), errors, tpl, line);
            break;
        }
        case NodeCommand: {
            const CommandNode* cmd = static_cast<const CommandNode*>(node);
            const std::vector<Node*>& args = cmd->Args();
            for (size_t i = 0; i < args.size(); ++i) {
                CheckVarNodes(args[i], errors, tpl, line);
            }
            break;
        }
        case NodePipe: {
            const PipeNode* pipe = static_cast<const PipeNode*>(node);
            const std::vector<CommandNode*>& cmds = pipe->Cmds();
            for (size_t i = 0; i < cmds.size(); ++i) {
                CheckVarNodes(cmds[i], errors, tpl, line);
            }
            break;
        }
        default:
            break;
    }
}

// 递归遍历AST，检查控制结构闭合
static void CheckControlNodes(const Node* node, std::vector<TemplateSyntaxError>& errors, const std::string& tpl, int parentLine) {
    if (!node) return;
    int line = parentLine;
    if (node->Type() == NodeIf || node->Type() == NodeRange || node->Type() == NodeWith) {
        const BranchNode* b = static_cast<const BranchNode*>(node);
        line = b->Line();
        // 检查主分支和end
        if (!b->List()) {
            TemplateSyntaxError err;
            err.type = ErrorType_Control;
            err.line = line;
            err.column = 1;
            err.message = "控制结构缺少主体";
            err.context = GetLineContext(tpl, err.line);
            err.description = "如if/range/with等语句必须有内容。";
            errors.push_back(err);
        }
        // 递归检查
        if (b->GetPipe()) CheckControlNodes(b->GetPipe(), errors, tpl, line);
        if (b->List()) CheckControlNodes(b->List(), errors, tpl, line);
        if (b->ElseList()) CheckControlNodes(b->ElseList(), errors, tpl, line);
        return;
    }
    // 递归子节点
    switch (node->Type()) {
        case NodeList: {
            const ListNode* list = static_cast<const ListNode*>(node);
            const std::vector<Node*>& nodes = list->Nodes();
            for (size_t i = 0; i < nodes.size(); ++i) {
                CheckControlNodes(nodes[i], errors, tpl, line);
            }
            break;
        }
        case NodeAction: {
            const ActionNode* act = static_cast<const ActionNode*>(node);
            if (act->Pipe()) CheckControlNodes(act->Pipe(), errors, tpl, line);
            break;
        }
        case NodeCommand: {
            const CommandNode* cmd = static_cast<const CommandNode*>(node);
            const std::vector<Node*>& args = cmd->Args();
            for (size_t i = 0; i < args.size(); ++i) {
                CheckControlNodes(args[i], errors, tpl, line);
            }
            break;
        }
        case NodePipe: {
            const PipeNode* pipe = static_cast<const PipeNode*>(node);
            const std::vector<CommandNode*>& cmds = pipe->Cmds();
            for (size_t i = 0; i < cmds.size(); ++i) {
                CheckControlNodes(cmds[i], errors, tpl, line);
            }
            break;
        }
        default:
            break;
    }
}

// 递归遍历AST，检查变量在数据中是否存在
static void CheckVarInData(const Node* node, Values* data, std::vector<TemplateSyntaxError>& errors, const std::string& tpl, int parentLine) {
    if (!node) return;
    int line = parentLine;
    if (node->Type() == NodeVariable) {
        const VariableNode* v = static_cast<const VariableNode*>(node);
        std::string path = v->Ident();
        if (data) {
            Values* cur = data;
            std::string seg;
            for (size_t i = 1; i < path.size(); ++i) {
                if (path[i] == '.' || i == path.size() - 1) {
                    if (i == path.size() - 1 && path[i] != '.') seg += path[i];
                    if (!cur->IsMap() || cur->AsMap().find(seg) == cur->AsMap().end()) {
                        TemplateSyntaxError err;
                        err.type = ErrorType_DataCompatible;
                        err.line = line;
                        err.column = 1;
                        err.message = "数据中找不到变量路径: " + path;
                        err.context = GetLineContext(tpl, err.line);
                        err.description = "模板中引用的变量在数据文件中不存在。";
                        errors.push_back(err);
                        break;
                    }
                    cur = cur->AsMap().find(seg)->second;
                    seg = "";
                } else {
                    seg += path[i];
                }
            }
        }
    }
    // 递归子节点
    switch (node->Type()) {
        case NodeList: {
            const ListNode* list = static_cast<const ListNode*>(node);
            const std::vector<Node*>& nodes = list->Nodes();
            for (size_t i = 0; i < nodes.size(); ++i) {
                CheckVarInData(nodes[i], data, errors, tpl, line);
            }
            break;
        }
        case NodeIf:
        case NodeRange:
        case NodeWith: {
            const BranchNode* b = static_cast<const BranchNode*>(node);
            if (b->GetPipe()) CheckVarInData(b->GetPipe(), data, errors, tpl, line);
            if (b->List()) CheckVarInData(b->List(), data, errors, tpl, line);
            if (b->ElseList()) CheckVarInData(b->ElseList(), data, errors, tpl, line);
            break;
        }
        case NodeAction: {
            const ActionNode* act = static_cast<const ActionNode*>(node);
            if (act->Pipe()) CheckVarInData(act->Pipe(), data, errors, tpl, line);
            break;
        }
        case NodeCommand: {
            const CommandNode* cmd = static_cast<const CommandNode*>(node);
            const std::vector<Node*>& args = cmd->Args();
            for (size_t i = 0; i < args.size(); ++i) {
                CheckVarInData(args[i], data, errors, tpl, line);
            }
            break;
        }
        case NodePipe: {
            const PipeNode* pipe = static_cast<const PipeNode*>(node);
            const std::vector<CommandNode*>& cmds = pipe->Cmds();
            for (size_t i = 0; i < cmds.size(); ++i) {
                CheckVarInData(cmds[i], data, errors, tpl, line);
            }
            break;
        }
        default:
            break;
    }
}

bool CheckTemplateSyntax(
    const std::string& templateName,
    const std::string& templateStr,
    const std::string& leftDelim,
    const std::string& rightDelim,
    const std::string& dataStr,
    std::vector<TemplateSyntaxError>& errors) {
    // 1. 空模板检查
    if (templateStr.empty()) {
        TemplateSyntaxError err;
        err.type = ErrorType_EmptyTemplate;
        err.line = 1;
        err.column = 1;
        err.message = "模板内容为空";
        err.context = "";
        err.description = "模板文件不能为空。";
        errors.push_back(err);
        return false;
    }
    // 2. 定界符检查
    int countL = 0, countR = 0;
    if (!CheckDelimiters(templateStr, leftDelim, rightDelim, countL, countR)) {
        TemplateSyntaxError err;
        err.type = ErrorType_Delimiter;
        err.line = 1;
        err.column = 1;
        err.message = "定界符数量不匹配";
        err.context = "";
        std::ostringstream oss;
        oss << "左定界符出现" << countL << "次，右定界符出现" << countR << "次。";
        err.description = oss.str();
        errors.push_back(err);
        // 继续后续检查
    }
    // 3. 解析错误检查
    Tree* mainTree = NULL;
    try {
        std::map<std::string, Tree*> trees = Tree::Parse(templateName, templateStr, leftDelim, rightDelim);
        std::map<std::string, Tree*>::iterator it = trees.find(templateName);
        if (it != trees.end()) mainTree = it->second;
    } catch (const ParseError& e) {
        TemplateSyntaxError err;
        err.type = ErrorType_Syntax;
        // 解析行号
        std::string msg = e.what();
        size_t p1 = msg.find(':');
        size_t p2 = msg.find(':', p1 + 1);
        size_t p3 = msg.find(':', p2 + 1);
        int line = 1;
        if (p1 != std::string::npos && p2 != std::string::npos && p3 != std::string::npos) {
            std::istringstream iss(msg.substr(p2 + 1, p3 - p2 - 1));
            iss >> line;
        }
        err.line = line;
        err.column = 1;
        err.message = msg;
        err.context = GetLineContext(templateStr, line);
        err.description = "模板解析器抛出语法错误。";
        errors.push_back(err);
        return false;
    }
    // 4. 控制结构、变量路径等进一步检查
    if (mainTree && mainTree->GetRoot()) {
        CheckControlNodes(mainTree->GetRoot(), errors, templateStr, 1);
        CheckVarNodes(mainTree->GetRoot(), errors, templateStr, 1);
        // 5. 数据兼容性检查
        if (!dataStr.empty()) {
            Values* data = NULL;
            try {
                data = ParseSimpleYAML(dataStr);
            } catch (...) { data = NULL; }
            if (data) {
                CheckVarInData(mainTree->GetRoot(), data, errors, templateStr, 1);
                delete data;
            }
        }
    }
    // 只要有错误就返回false
    return errors.empty();
}

} // namespace template_engine 