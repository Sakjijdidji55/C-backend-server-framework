/**
 * @file Model.h
 * @brief 数据模型基类头文件
 * @brief Base Model Class Header File
 * 
 * 此头文件定义了数据库模型的基类，提供了表结构定义和基本数据库操作功能
 * This header file defines the base class for database models, providing table structure definition and basic database operation functionality
 */
//
// Created by HP on 2025/12/9.
//

#ifndef CBSF_MODEL_H
#define CBSF_MODEL_H
#include <string>
#include <unordered_map>
#include <iostream>

/**
 * @class Model
 * @brief 数据库模型基类
 * @brief Database Model Base Class
 * 
 * 所有具体的数据模型类都应继承此类，提供了表结构定义和基本CRUD操作框架
 * All specific data model classes should inherit from this class, providing table structure definition and basic CRUD operation framework
 */
class Model {
public:
    /**
     * @brief 表名
     * @brief Table name
     */
    std::string TableName;
    
    /**
     * @brief 表字符集
     * @brief Table character set
     */
    std::string charset;
    
    /**
     * @brief 表字段映射（字段名->字段定义）
     * @brief Table columns mapping (column name -> column definition)
     */
    std::unordered_map<std::string, std::string> TableCols;
    
    /**
     * @brief 数据绑定状态标志
     * @brief Data binding status flag
     */
    bool isBind = false;

    /**
     * @brief 构造函数
     * @param name 表名，默认为"models"
     * @param cset 字符集，默认为"utf8mb4"
     * @brief Constructor
     * @param name Table name, default is "models"
     * @param cset Character set, default is "utf8mb4"
     * @note 初始化时会自动创建id主键字段
     * @note Automatically creates id primary key field during initialization
     */
    explicit Model(std::string name="models", std::string cset="utf8mb4") : TableName(std::move(name)), charset(std::move(cset)) {
        TableCols["id"]="INT PRIMARY KEY AUTO_INCREMENT";
    };
    
    /**
     * @brief 设置表字段默认值
     * @param key 字段名
     * @param value 字段定义
     * @brief Set table field default value
     * @param key Field name
     * @param value Field definition
     */
    void setTableDefault(const std::string& key, std::string value);

    /**
     * @brief 绑定数据到模型（纯虚函数）
     * @param key 字段名
     * @param value 字段值
     * @brief Bind data to model (pure virtual function)
     * @param key Field name
     * @param value Field value
     */
    virtual void bind(const std::string& key, const std::string& value) = 0;

    /**
     * @brief 保存模型数据到数据库（纯虚函数）
     * @return 保存是否成功
     * @brief Save model data to database (pure virtual function)
     * @return Whether save was successful
     */
    virtual bool save() = 0;

    /**
     * @brief 更新数据库中的模型数据（纯虚函数）
     * @return 更新是否成功
     * @brief Update model data in database (pure virtual function)
     * @return Whether update was successful
     */
    virtual bool update() = 0;

    /**
     * @brief 初始化数据库表结构
     * @brief Initialize database table structure
     */
    void initDatabase();
};


#endif //CBSF_MODEL_H
