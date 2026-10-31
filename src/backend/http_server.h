#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace workbench {

struct HttpRequest {
    std::string method;
    std::string path;
    std::string query_string;
    std::map<std::string, std::string> query_params;
    std::map<std::string, std::string> headers;
    std::string body;
};

struct HttpResponse {
    int status_code = 200;
    std::string content_type = "application/json";
    std::string body;
    std::map<std::string, std::string> custom_headers;

    static HttpResponse json(const std::string& json_body, int code = 200) {
        HttpResponse res;
        res.status_code = code;
        res.content_type = "application/json";
        res.body = json_body;
        return res;
    }

    static HttpResponse text(const std::string& text_body, int code = 200, const std::string& type = "text/plain") {
        HttpResponse res;
        res.status_code = code;
        res.content_type = type;
        res.body = text_body;
        return res;
    }

    static HttpResponse error(const std::string& err_msg, int code = 400) {
        HttpResponse res;
        res.status_code = code;
        res.content_type = "application/json";
        res.body = "{\"success\":false,\"error\":\"" + err_msg + "\"}";
        return res;
    }
};

using HttpHandler = std::function<HttpResponse(const HttpRequest&)>;

class HttpServer {
public:
    HttpServer(int port = 8080);
    ~HttpServer();

    void get(const std::string& path, HttpHandler handler);
    void post(const std::string& path, HttpHandler handler);

    /// Start listening (blocking call, or runs in background if threaded)
    bool start();

    /// Stop server
    void stop();

    bool is_running() const { return running_; }
    int get_port() const { return port_; }

private:
    int port_;
    bool running_ = false;
    uintptr_t server_socket_ = ~0ULL;

    std::map<std::string, HttpHandler> get_routes_;
    std::map<std::string, HttpHandler> post_routes_;

    void handle_client(uintptr_t client_socket);
    HttpResponse dispatch(const HttpRequest& req);
};

} // namespace workbench
