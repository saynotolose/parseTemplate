#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <map>
#include <vector>
#include <memory>

// 位置类型
typedef size_t Pos;

// token类型枚举
enum ItemType {
    ItemError,        // 发生错误；值是错误文本
    ItemBool,         // 布尔常量
    ItemChar,         // 可打印的ASCII字符；逗号等的集合
    ItemCharConstant, // 字符常量
    ItemComment,      // 注释文本
    ItemComplex,      // 复数常量(1+2i)；虚数只是一个数字
    ItemAssign,       // 等号('=')引入赋值
    ItemDeclare,      // 冒号等号(':=')引入声明
    ItemEOF,          // 文件结束
    ItemField,        // 以'.'开头的字母数字标识符
    ItemIdentifier,   // 不以'.'开头的字母数字标识符
    ItemLeftDelim,    // 左动作分隔符
    ItemLeftParen,    // 动作内的'('
    ItemNumber,       // 简单数字，包括虚数
    ItemPipe,         // 管道符号
    ItemRawString,    // 原始引用字符串(包括引号)
    ItemRightDelim,   // 右动作分隔符
    ItemRightParen,   // 动作内的')'
    ItemSpace,        // 分隔参数的空格运行
    ItemString,       // 引用字符串(包括引号)
    ItemText,         // 普通文本
    ItemVariable,     // 以'$'开头的变量，如'$'或'$1'或'$hello'
    // 关键字出现在其他所有之后
    ItemKeyword,      // 仅用于分隔关键字
    ItemBlock,        // block关键字
    ItemBreak,        // break关键字
    ItemContinue,     // continue关键字
    ItemDot,          // 拼写为'.'
    ItemDefine,       // define关键字
    ItemElse,         // else关键字
    ItemEnd,          // end关键字
    ItemIf,           // if关键字
    ItemNil,          // 无类型nil常量，最容易作为关键字处理
    ItemRange,        // range关键字
    ItemTemplate,     // template关键字
    ItemWith          // with关键字
};

// 表示从扫描器返回item/token
struct Item {
    ItemType type;    // 此token的类型
    Pos pos;          // 此token在输入字符串中的起始位置
    std::string val;  // 此token的值
    int line;         // 此token开始时的行号

    Item();
    Item(ItemType t, Pos p, const std::string& v, int l);

    std::string toString() const;
};

// 词法分析器选项
struct LexOptions {
    bool emitComment;  // 发出itemComment标记
    bool breakOK;      // 允许break关键字
    bool continueOK;   // 允许continue关键字

    LexOptions() : emitComment(false), breakOK(false), continueOK(false) {}
};

// 词法分析器类
class Lexer {
public:
    Lexer(const std::string& name, const std::string& input, 
          const std::string& left, const std::string& right);
          
    // 获取下一个词法单元
    Item nextItem();
    
    // 获取所有token
    std::vector<Item> getAllItems();
    
    // 设置词法分析器选项
    void setOptions(const LexOptions& options);

private:
    // 成员变量
    std::string name_;          // 输入的名称，仅用于错误报告
    std::string input_;         // 正在扫描的字符串
    std::string leftDelim_;     // 动作标记的开始
    std::string rightDelim_;    // 动作标记的结束
    Pos pos_;                   // 输入中的当前位置
    Pos start_;                 // 此token的开始位置
    bool atEOF_;                // 我们已经到达输入结束并返回了eof
    int parenDepth_;            // () 表达式的嵌套深度
    int line_;                  // 1+看到的换行符数量
    int startLine_;             // 此token的开始行
    bool insideAction_;         // 我们是否在动作内部？
    LexOptions options_;        // 词法分析器选项
    
    Pos lastPos_;               // 上一个token的位置（用于调试）
    ItemType lastType_;         // 上一个token的类型（用于调试）
    
    std::map<std::string, ItemType> keyMap_;  // 关键字映射

    // 词法分析器方法
    int next();
    int peek() const;
    void backup();
    Item errorItem(const std::string& format, ...);
    void ignore();
    bool accept(const std::string& valid);
    void acceptRun(const std::string& valid);
    std::pair<bool, bool> atRightDelim();
    bool atTerminator();
    bool scanNumber();
    
    // 词法状态处理函数
    Item lexText();
    Item lexLeftDelim();
    Item lexComment();
    Item lexRightDelim();
    Item lexInsideAction();
    Item lexSpace();
    Item lexIdentifier();
    Item lexField();
    Item lexVariable();
    Item lexChar();
    Item lexNumber();
    Item lexQuote();
    Item lexRawQuote();
};

// 创建词法分析器的工厂函数
Lexer* createLexer(const std::string& name, const std::string& input, 
                                const std::string& left = "", const std::string& right = "");

// 将词法项类型转换为字符串的辅助函数
std::string itemTypeToString(ItemType type);

// 工具函数声明
bool isSpace(int r);
bool isAlphaNumeric(int r);
bool hasLeftTrimMarker(const std::string& s);
bool hasRightTrimMarker(const std::string& s);
Pos rightTrimLength(const std::string& s);
Pos leftTrimLength(const std::string& s);

#endif // LEXER_H