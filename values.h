// values.h
#ifndef TEMPLATE_VALUES_H
#define TEMPLATE_VALUES_H

#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace template_engine {

// 前向声明
class TemplateFn;

// 错误类型定义
enum ValueErrorType {
    NoTable,     // 表不存在
    NoValue,     // 值不存在
    TypeError    // 类型错误
};

// 值错误类
class ValueError : public std::runtime_error {
public:
    ValueError(ValueErrorType type, const std::string& msg)
        : std::runtime_error(msg), type_(type) {}
    
    ValueErrorType type() const { return type_; }
    
private:
    ValueErrorType type_;
};

// 值类型定义
class Values {
public:
    // 值类型枚举
    enum Type {
        Null,
        Bool,
        Number,
        String,
        List,
        Map,
        Function  // 添加Function类型
    };


    //类型判断
    Type type_;

    // 实际值存储
    bool boolValue_;
    double numberValue_;
    std::string stringValue_;
    std::vector<Values*> listValue_;
    std::map<std::string, Values*> mapValue_;
    TemplateFn* functionValue_;  // 添加函数值


    // 构造函数
    Values();
    explicit Values(bool b);
    explicit Values(double n);
    explicit Values(const std::string& s);
    explicit Values(const std::vector<Values*>& l);
    explicit Values(const std::map<std::string, Values*>& m);
    explicit Values(TemplateFn* fn);  // 添加函数构造器

    // 析构函数 - 确保释放内存
    ~Values();
    
    // 复制构造函数和赋值运算符（防止重复删除内存）
    Values(const Values& other);
    Values& operator=(const Values& other);

    // 工厂方法
    static Values* MakeNull();
    static Values* MakeBool(bool b);
    static Values* MakeNumber(double n);
    static Values* MakeString(const std::string& s);
    static Values* MakeList(const std::vector<Values*>& l);
    static Values* MakeMap(const std::map<std::string, Values*>& m);
    static Values* MakeFunction(TemplateFn* fn);  // 添加函数工厂方法

    // 类型检查
    bool IsNull() const;
    bool IsBool() const;
    bool IsNumber() const;
    bool IsString() const;
    bool IsList() const;
    bool IsMap() const;
    bool IsFunction() const;  // 添加函数类型检查

    // 类型名称
    std::string TypeName() const;

    // 值访问
    bool AsBool() const;
    double AsNumber() const;
    const std::string& AsString() const;
    const std::vector<Values*>& AsList() const;
    const std::map<std::string, Values*>& AsMap() const;
    std::map<std::string, Values*>& AsMap();
    TemplateFn* AsFunction() const;  // 添加函数访问方法

    // 使用路径访问值 (如 foo.bar.baz)
    Values* Table(const std::string& path) const;
    Values* PathValue(const std::string& path) const;

    
    // 序列化
    std::string ToYAML() const;
    bool Encode(std::ostream& out) const;
    
    // 反序列化
    static Values* FromYAML(const std::string& yamlContent);
    static Values* FromYAMLFile(const std::string& filename);
    
    // 深拷贝
    Values* DeepCopy() const;
    
    // 判断是否包含特定键
    bool Contains(const std::string& key) const;
    
    // 类似数组访问
    Values*& operator[](const std::string& key);
    Values*& operator[](size_t index);
    
    // 调试输出
    void Print(std::ostream& os = std::cout, int indent = 0) const;
    
    // 辅助函数
    static Values* DeepCopy(const Values& value);

    // 序列化
    std::string ToString() const;


    
    
    // 分割路径为段
    static std::vector<std::string> SplitPath(const std::string& path);
    
    // 辅助函数
    void printMap(const std::map<std::string, Values*>& map, 
                  std::ostream& os, int indent) const;
    void printList(const std::vector<Values*>& list, 
                   std::ostream& os, int indent) const;
                   
                   
    // 清理资源
    void clearResources();

    // 简易YAML解析接口声明
    Values* ParseSimpleYAML(const std::string& yamlText);
    Values* ParseSimpleYAMLFile(const std::string& filename);
};

// ================== 这里是修正后的声明 ==================
// 在namespace template_engine内、class Values定义后声明
Values* ParseSimpleYAML(const std::string& yamlText);
Values* ParseSimpleYAMLFile(const std::string& filename);
// ================== 声明结束 ==================

// 用于模板渲染的选项
struct RenderOptions {
    std::string name;
    std::string nameSpace;
    int revision;
    bool isUpgrade;
    bool isInstall;
    
    RenderOptions() : revision(1), isUpgrade(false), isInstall(true) {}
};

// 合并值
Values* CoalesceValues(const Values* base, const Values* overlay);

// 准备渲染值
Values* ToRenderValues(
    const std::string& chartName,
    const std::string& chartVersion,
    const Values* chartValues,
    const RenderOptions& options);

} // namespace template_engine

#endif // TEMPLATE_VALUES_H