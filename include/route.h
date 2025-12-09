/**
 * @file route.h
 * @brief 路由注册功能头文件
 * @brief Route Registration Function Header File
 */
//
// Created by HP on 2025/11/5.
//

#ifndef CBSF_ROUTE_H
#define CBSF_ROUTE_H

#include "Server.h"
#include "Handler.h"

/**
 * @brief 注册所有应用程序路由
 * @param app 服务器实例引用
 * @brief Register all application routes
 * @param app Server instance reference
 * @note 此函数负责设置服务器的所有HTTP路由及其处理函数
 * @note This function is responsible for setting up all HTTP routes and their handler functions for the server
 */
void registerRoutes(Server &app);

#endif //CBSF_ROUTE_H
