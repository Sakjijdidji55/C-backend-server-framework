#include "../../include/Handler.h"

namespace RouteHandlers {
void homeGet(const Request& /*req*/, Response& res) {
    res.json(R"({"message":"Welcome to C++ Server"})");
}

void homePost(const Request& /*req*/, Response& res) {
    res.success();
}

void downloadFile(const Request& req, Response& res) {
    const std::string path = req.query_param("path");
//    std::cout<<path<<std::endl;
    if (path.empty()) {
        res.error(400, "Missing query parameter: path");
        return;
    }
    res.file(path, "application/octet-stream", true, "");
}

void downloadFileStream(const Request& req, Response& res) {
    const std::string path = req.query_param("path");
    if (path.empty()) {
        res.error(400, "Missing query parameter: path");
        return;
    }
    res.fileStream(path, "application/octet-stream", true, "", 65536);
}
}
