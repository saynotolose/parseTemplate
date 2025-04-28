#include "lexer.h"
#include "parse.h"
#include "node.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <string>
#include <memory>
#include <fstream>  // 用于文件操作
#include <sstream>  // 用于字符串流 

// 辅助函数：打印抽象语法树
void printAST(const Node* node, int indent = 0) {
    if (!node) return;
    
    std::string indentStr(indent * 2, ' ');
    std::string nodeType;
    std::stringstream tempStream; // 临时流用于获取节点字符串表示
    
    switch (node->Type()) {
        case NodeType::NodeText: 
            nodeType = "文本节点";
            std::cout << indentStr << nodeType << ": \"" 
                      << dynamic_cast<const TextNode*>(node)->Text() << "\"" << std::endl;
            break;
        case NodeType::NodeAction:
            nodeType = "动作节点";
            std::cout << indentStr << nodeType << ": " 
                      << node->String() << std::endl;
            // 打印管道详情
            if (const ActionNode* action = dynamic_cast<const ActionNode*>(node)) {
                printAST(action->pipe_.get(), indent + 1);
            }
            break;
        case NodeType::NodePipe:
            nodeType = "管道节点";
            std::cout << indentStr << nodeType << std::endl;
            // 打印管道中的命令
            if (const PipeNode* pipe = dynamic_cast<const PipeNode*>(node)) {
                for (const auto& cmd : pipe->cmds_) {
                    std::cout << indentStr << "  命令: " << cmd->String() << std::endl;
                    // 打印命令参数
                    for (const auto& arg : cmd->args_) {
                        printAST(arg.get(), indent + 2);
                    }
                }
            }
            break;
        case NodeType::NodeIf:
            nodeType = "If节点";
            std::cout << indentStr << nodeType << std::endl;
            if (const IfNode* ifNode = dynamic_cast<const IfNode*>(node)) {
                std::cout << indentStr << "  条件: ";
                tempStream.str("");
                ifNode->pipe_->WriteTo(tempStream);
                std::cout << tempStream.str() << std::endl;
                
                std::cout << indentStr << "  主体:" << std::endl;
                printAST(ifNode->list_.get(), indent + 2);
                
                if (ifNode->else_list_) {
                    std::cout << indentStr << "  Else部分:" << std::endl;
                    printAST(ifNode->else_list_.get(), indent + 2);
                }
            }
            break;
        case NodeType::NodeRange:
            nodeType = "Range节点";
            std::cout << indentStr << nodeType << std::endl;
            if (const RangeNode* rangeNode = dynamic_cast<const RangeNode*>(node)) {
                std::cout << indentStr << "  范围: ";
                tempStream.str("");
                rangeNode->pipe_->WriteTo(tempStream);
                std::cout << tempStream.str() << std::endl;
                
                std::cout << indentStr << "  主体:" << std::endl;
                printAST(rangeNode->list_.get(), indent + 2);
                
                if (rangeNode->else_list_) {
                    std::cout << indentStr << "  Else部分:" << std::endl;
                    printAST(rangeNode->else_list_.get(), indent + 2);
                }
            }
            break;
        case NodeType::NodeWith:
            nodeType = "With节点";
            std::cout << indentStr << nodeType << std::endl;
            if (const WithNode* withNode = dynamic_cast<const WithNode*>(node)) {
                std::cout << indentStr << "  上下文: ";
                tempStream.str("");
                withNode->pipe_->WriteTo(tempStream);
                std::cout << tempStream.str() << std::endl;
                
                std::cout << indentStr << "  主体:" << std::endl;
                printAST(withNode->list_.get(), indent + 2);
                
                if (withNode->else_list_) {
                    std::cout << indentStr << "  Else部分:" << std::endl;
                    printAST(withNode->else_list_.get(), indent + 2);
                }
            }
            break;
        case NodeType::NodeList:
            nodeType = "列表节点";
            std::cout << indentStr << nodeType << std::endl;
            // 递归打印列表中的所有节点
            if (const ListNode* list = dynamic_cast<const ListNode*>(node)) {
                for (const auto& n : list->Nodes()) {
                    printAST(n.get(), indent + 1);
                }
            }
            break;
        case NodeType::NodeTemplate:
            nodeType = "模板节点";
            if (const TemplateNode* tmpl = dynamic_cast<const TemplateNode*>(node)) {
                std::cout << indentStr << nodeType << ": " << tmpl->name_ << std::endl;
                
                if (tmpl->pipe_) {
                    std::cout << indentStr << "  参数: ";
                    tempStream.str("");
                    tmpl->pipe_->WriteTo(tempStream);
                    std::cout << tempStream.str() << std::endl;
                }
            }
            break;
        case NodeType::NodeField:
            nodeType = "字段节点";
            std::cout << indentStr << nodeType << ": " 
                      << node->String() << std::endl;
            break;
        case NodeType::NodeVariable:
            nodeType = "变量节点";
            std::cout << indentStr << nodeType << ": " 
                      << node->String() << std::endl;
            break;
        case NodeType::NodeBool:
            nodeType = "布尔节点";
            std::cout << indentStr << nodeType << ": " 
                      << node->String() << std::endl;
            break;
        case NodeType::NodeNumber:
            nodeType = "数字节点";
            std::cout << indentStr << nodeType << ": " 
                      << node->String() << std::endl;
            break;
        case NodeType::NodeString:
            nodeType = "字符串节点";
            std::cout << indentStr << nodeType << ": " 
                      << node->String() << std::endl;
            break;
        case NodeType::NodeDot:
            nodeType = "点节点";
            std::cout << indentStr << nodeType << std::endl;
            break;
        case NodeType::NodeNil:
            nodeType = "空节点";
            std::cout << indentStr << nodeType << std::endl;
            break;
        case NodeType::NodeComment:
            nodeType = "注释节点";
            std::cout << indentStr << nodeType << ": " 
                      << node->String() << std::endl;
            break;
        case NodeType::NodeBreak:
            nodeType = "Break节点";
            std::cout << indentStr << nodeType << std::endl;
            break;
        case NodeType::NodeContinue:
            nodeType = "Continue节点";
            std::cout << indentStr << nodeType << std::endl;
            break;
        default:
            nodeType = "未知节点类型";
            std::cout << indentStr << nodeType << std::endl;
            break;
    }
}

// 展示词法分析结果的辅助函数
void printTokens(const std::vector<Item>& tokens) {
    std::cout << "┌─────────────────┬──────────────────────────────┬───────┬────────┐\n";
    std::cout << "│ 类型            │ 值                           │ 行号  │ 位置   │\n";
    std::cout << "├─────────────────┼──────────────────────────────┼───────┼────────┤\n";
    
    for (const auto& item : tokens) {
        std::string value = item.val;
        // 替换换行符为可见表示
        std::string displayValue = "";
        for (char c : value) {
            if (c == '\n') displayValue += "\\n";
            else if (c == '\r') displayValue += "\\r";
            else if (c == '\t') displayValue += "\\t";
            else displayValue += c;
        }
        
        // 截断过长的值
        if (displayValue.length() > 30) {
            displayValue = displayValue.substr(0, 27) + "...";
        }
        
        std::cout << "│ " 
                  << std::left << std::setw(15) << itemTypeToString(item.type) << " │ "
                  << std::left << std::setw(30) << displayValue << " │ "
                  << std::setw(5) << item.line << " │ "
                  << std::setw(6) << item.pos << " │\n";
    }
    
    std::cout << "└─────────────────┴──────────────────────────────┴───────┴────────┘\n";
}

// 测试Helm Chart模板解析
// void testHelmTemplateParser(const std::string& testName, const std::string& templateContent) {
//     std::cout << "==========================================\n";
//     std::cout << "测试: " << testName << "\n";
//     std::cout << "==========================================\n";
//     std::cout << "模板内容:\n" << templateContent << "\n\n";
    
//     try {
//         // 1. 词法分析
//         std::cout << "词法分析结果:\n";
//         auto lexer = createLexer(testName, templateContent);
        
//         // 设置词法分析器选项
//         LexOptions options;
//         options.emitComment = true;
//         options.breakOK = true;
//         options.continueOK = true;
//         lexer->setOptions(options);
        
//         std::vector<Item> tokens = lexer->getAllItems();
//         printTokens(tokens);
        
//         // 2. 语法分析和AST构建
//         std::cout << "\n语法分析和抽象语法树:\n";
        
//         // 创建空的函数映射
//         std::vector<std::unordered_map<std::string, std::any>> funcs;
        
//         // 解析模板
//         std::unordered_map<std::string, std::shared_ptr<Tree>> treeSet = 
//             Tree::Parse(testName, templateContent, "{{", "}}", funcs);
        
//         if (treeSet.empty()) {
//             std::cout << "解析失败，无法构建抽象语法树。\n";
//         } else {
//             // 输出抽象语法树
//             for (const auto& [name, tree] : treeSet) {
//                 std::cout << "模板 '" << name << "' 的抽象语法树:\n";
//                 if (tree->GetRoot()) {
//                     printAST(tree->GetRoot());
//                 } else {
//                     std::cout << "  空树\n";
//                 }
//             }
//         }
//     } catch (const ParseError& e) {
//         std::cout << "解析错误: " << e.what() << std::endl;
//     } catch (const std::exception& e) {
//         std::cout << "异常: " << e.what() << std::endl;
//     }
    
//     std::cout << "\n";
// }
void testHelmTemplateParser(const std::string& testName, const std::string& templateContent) {
    std::cout << "==========================================\n";
    std::cout << "测试: " << testName << "\n";
    std::cout << "==========================================\n";
    std::cout << "模板内容:\n" << templateContent << "\n\n";
    
    try {
        // 1. 词法分析
        std::cout << "词法分析结果:\n";
        auto lexer = createLexer(testName, templateContent);
        
        // 设置词法分析器选项
        LexOptions options;
        options.emitComment = true;
        options.breakOK = true;
        options.continueOK = true;
        lexer->setOptions(options);
        
        std::vector<Item> tokens = lexer->getAllItems();
        printTokens(tokens);
        
        // 2. 语法分析和AST构建
        std::cout << "\n语法分析和抽象语法树:\n";
        
        // 创建空的函数映射
        std::vector<std::unordered_map<std::string, std::any>> funcs;
        
        // 解析模板
        std::unordered_map<std::string, std::shared_ptr<Tree>> treeSet = 
            Tree::Parse(testName, templateContent, "{{", "}}", funcs);
        
        std::cout << "解析完成，treeSet大小: " << treeSet.size() << std::endl;
        
        if (treeSet.empty()) {
            std::cout << "解析失败，无法构建抽象语法树。\n";
        } else {
            // 输出抽象语法树
            for (const auto& [name, tree] : treeSet) {
                std::cout << "模板 '" << name << "' 的抽象语法树:\n";
                if (tree && tree->GetRoot()) {
                    printAST(tree->GetRoot());
                } else {
                    std::cout << "  空树\n";
                }
            }
        }
    } catch (const ParseError& e) {
        std::cout << "解析错误: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cout << "异常: " << e.what() << std::endl;
    }
    
    std::cout << "\n";
}

// int main() {
//     // 测试1: 简单变量替换
//     // testHelmTemplateParser("简单变量替换", 
//     //     "这个Chart的名称是: {{ .Chart.Name }}\n"
//     //     "版本: {{ .Chart.Version }}\n");
    
//     // // 测试2: 条件语句
//     // testHelmTemplateParser("条件语句", 
//     //     "{{ if .Values.serviceAccount.create }} 创建服务账号 {{ else }} 使用已有的服务账号{{ end }} ");

//     // "{{ if .Values.serviceAccount.create }}\n"
//     //     "创建服务账号\n"
//     //     "{{ else }}\n"
//     //     "使用已有的服务账号\n"
//     //     "{{ end }}"
    
//     // // 测试3: 循环语句
//     // testHelmTemplateParser("循环语句", 
//     //     "容器列表:\n"
//     //     "{{ range .Values.containers }}\n"
//     //     "- 名称: {{ .name }}, 镜像: {{ .image }}:{{ .tag }}\n"
//     //     "{{ end }}");
    
//     // // 测试4: 嵌套结构
//     // testHelmTemplateParser("嵌套结构", 
//     //     "{{- with .Values.resources }}\n"
//     //     "资源限制:\n"
//     //     "  {{- if .limits }}\n"
//     //     "  limits:\n"
//     //     "    {{- if .limits.cpu }}\n"
//     //     "    cpu: {{ .limits.cpu }}\n"
//     //     "    {{- end }}\n"
//     //     "    {{- if .limits.memory }}\n"
//     //     "    memory: {{ .limits.memory }}\n"
//     //     "    {{- end }}\n"
//     //     "  {{- end }}\n"
//     //     "{{- end }}");
    
//     // // 测试5: 管道和函数
//     // testHelmTemplateParser("管道和函数", 
//     //     "注解:\n"
//     //     "  checksum/config: {{ include (print $.Template.BasePath \"/configmap.yaml\") . | sha256sum }}\n"
//     //     "  {{- if .Values.podAnnotations }}\n"
//     //     "  {{- toYaml .Values.podAnnotations | nindent 4 }}\n"
//     //     "  {{- end }}");
    
//     // // 测试6: include模板
//     // testHelmTemplateParser("包含模板", 
//     //     "{{- define \"mychart.labels\" }}\n"
//     //     "app: {{ .Chart.Name }}\n"
//     //     "release: {{ .Release.Name }}\n"
//     //     "{{- end }}\n"
//     //     "\n"
//     //     "metadata:\n"
//     //     "  labels:\n"
//     //     "    {{- include \"mychart.labels\" . | nindent 4 }}");

//     // 指定文件路径
//     std::string filepath = "c:/Users/oykk/Desktop/project/code2/deployment.yaml";
//     std::string outputFilepath = "c:/Users/oykk/Desktop/project/code2/Res.yaml";

//     // 打开文件
//     std::ifstream file(filepath);
//     if (!file.is_open()) {
//         std::cerr << "无法打开文件: " << filepath << std::endl;
//         return 1;
//     }

//     // 读取文件内容
//     std::stringstream buffer;
//     buffer << file.rdbuf();
//     std::string input = buffer.str();
//     file.close();

//     // 打开输出文件
//     std::ofstream outFile(outputFilepath);
//     if (!outFile.is_open()) {
//         std::cerr << "无法打开输出文件: " << outputFilepath << std::endl;
//         return 1;
//     }

//     testHelmTemplateParser("yaml文件解析", input);

//     outFile.close();
    
//     return 0;
// }