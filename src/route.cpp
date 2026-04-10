//
// Created by HP on 2025/11/5.
// Route registration implementation file

#include "../include/route.h"

void registerRoutes(Server &app) {
    // Public routes
    app.get("/", RouteHandlers::homeGet);
    app.post("/", RouteHandlers::homePost);

    auto download = app.group("/download");

    download.get("", RouteHandlers::downloadFile);
    download.get("/stream", RouteHandlers::downloadFileStream);

}