// values.h
#ifndef TEMPLATE_VALUES_H
#define TEMPLATE_VALUES_H

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <variant>
#include <optional>
#include <iostream>
#include <fstream>
#include <sstream>

namespace template_engine {

// 错误类型定义
enum class ValueErrorType {
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

// 值类
class Values : public std::enable_shared_from_this<Values>  {
public:
    // 使用variant支持多种类型
    using ValueType = std::variant<
    std::nullptr_t,
    bool,
    int64_t,
    double,
    std::string,
    std::map<std::string, std::shared_ptr<Values>>,
    std::vector<std::shared_ptr<Values>>
    >;
    
    // 构造函数
    Values() = default;
    explicit Values(const ValueType& value);
    explicit Values(ValueType&& value);
    
    // 创建辅助函数
    static std::shared_ptr<Values> MakeMap();
    static std::shared_ptr<Values> MakeList();
    static std::shared_ptr<Values> MakeString(const std::string& s);
    static std::shared_ptr<Values> MakeNumber(double n);
    static std::shared_ptr<Values> MakeBool(bool b);
    static std::shared_ptr<Values> MakeNull();
    
    // 类型检查
    bool IsMap() const;
    bool IsList() const;
    bool IsString() const;
    bool IsNumber() const;
    bool IsBool() const;
    bool IsNull() const;
    
    // 访问器
    std::map<std::string, std::shared_ptr<Values>>& AsMap();
    const std::map<std::string, std::shared_ptr<Values>>& AsMap() const;
    std::vector<std::shared_ptr<Values>>& AsList();
    const std::vector<std::shared_ptr<Values>>& AsList() const;
    std::string& AsString();
    const std::string& AsString() const;
    double AsNumber() const;
    bool AsBool() const;
    
    // 使用路径访问值 (如 foo.bar.baz)
    std::shared_ptr<Values> Table(const std::string& path) const;
    std::optional<std::shared_ptr<Values>> PathValue(const std::string& path) const;
    
    // 序列化
    std::string ToYAML() const;
    bool Encode(std::ostream& out) const;
    
    // 反序列化
    static std::shared_ptr<Values> FromYAML(const std::string& yamlContent);
    static std::shared_ptr<Values> FromYAMLFile(const std::string& filename);
    
    // 深拷贝
    std::shared_ptr<Values> DeepCopy() const;
    
    // 判断是否包含特定键
    bool Contains(const std::string& key) const;
    
    // 类似数组访问
    std::shared_ptr<Values>& operator[](const std::string& key);
    std::shared_ptr<Values>& operator[](size_t index);
    
    // 调试输出
    void Print(std::ostream& os = std::cout, int indent = 0) const;
    
private:
    ValueType value_;
    
    // 分割路径为段
    static std::vector<std::string> SplitPath(const std::string& path);
    
    // 辅助函数
    void printMap(const std::map<std::string, std::shared_ptr<Values>>& map, 
                  std::ostream& os, int indent) const;
    void printList(const std::vector<std::shared_ptr<Values>>& list, 
                   std::ostream& os, int indent) const;
};

// 用于模板渲染的选项
struct RenderOptions {
    std::string name;
    std::string nameSpace;
    int revision = 1;
    bool isUpgrade = false;
    bool isInstall = true;
};

// 合并值
std::shared_ptr<Values> CoalesceValues(const std::shared_ptr<Values>& base, 
                                       const std::shared_ptr<Values>& overlay);

// 准备渲染值
std::shared_ptr<Values> ToRenderValues(
    const std::string& chartName,
    const std::string& chartVersion,
    const std::shared_ptr<Values>& chartValues,
    const RenderOptions& options);

} // namespace template_engine

#endif // TEMPLATE_VALUES_H