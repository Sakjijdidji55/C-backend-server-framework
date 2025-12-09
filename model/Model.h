//
// Created by HP on 2025/12/9.
//

#ifndef CBSF_MODEL_H
#define CBSF_MODEL_H
#include <string>
#include <unordered_map>
#include <iostream>

class Model {
public:
    std::string TableName;
    std::string charset;
    std::unordered_map<std::string, std::string> TableCols;
    bool isBind = false;

    explicit Model(std::string name="models", std::string cset="utf8mb4") : TableName(std::move(name)), charset(std::move(cset)) {
        TableCols["id"]="INT PRIMARY KEY AUTO_INCREMENT";
    };
    void setTableDefault(const std::string& key, std::string value) ;

    virtual void bind(const std::string& key, const std::string& value) = 0;

    virtual bool save() = 0;

    virtual bool update() = 0;

    void initDatabase() ;
};


#endif //CBSF_MODEL_H
