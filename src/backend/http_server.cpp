#include "http_server.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
using socket_t = SOCKET;
#define CLOSE_SOCKET closesocket
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using socket_t = int;
#define CLOSE_SOCKET close
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

namespace workbench {

namespace {

std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::string get_mime_type(const std::string& path) {
    if (path.rfind(".html") != std::string::npos) return "text/html; charset=utf-8";
    if (path.rfind(".js") != std::string::npos) return "application/javascript; charset=utf-8";
    if (path.rfind(".css") != std::string::npos) return "text/css; charset=utf-8";
    if (path.rfind(".svg") != std::string::npos) return "image/svg+xml";
    if (path.rfind(".png") != std::string::npos) return "image/png";
    if (path.rfind(".ico") != std::string::npos) return "image/x-icon";
    if (path.rfind(".json") != std::string::npos) return "application/json";
    if (path.rfind(".txt") != std::string::npos) return "text/plain";
    return "application/octet-stream";
}

bool read_file_bytes(const std::string& filepath, std::string& out_content) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) return false;
    out_content.assign((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    return true;
}

} // anonymous namespace

HttpServer::HttpServer(int port) : port_(port), running_(false) {
#ifdef _WIN32
    WSADATA wsa_data;
    WSAStartup(MAKEWORD(2, 2), &wsa_data);
#endif
}

HttpServer::~HttpServer() {
    stop();
#ifdef _WIN32
    WSACleanup();
#endif
}

void HttpServer::get(const std::string& path, HttpHandler handler) {
    get_routes_[path] = std::move(handler);
}

void HttpServer::post(const std::string& path, HttpHandler handler) {
    post_routes_[path] = std::move(handler);
}

void HttpServer::stop() {
    running_ = false;
    if (server_socket_ != ~0ULL && server_socket_ != (uintptr_t)INVALID_SOCKET) {
        CLOSE_SOCKET((socket_t)server_socket_);
        server_socket_ = ~0ULL;
    }
}

bool HttpServer::start() {
    socket_t s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        std::cerr << "Failed to create socket\n";
        return false;
    }

    int opt = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<u_short>(port_));

    if (bind(s, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "Failed to bind socket to port " << port_ << "\n";
        CLOSE_SOCKET(s);
        return false;
    }

    if (listen(s, 64) == SOCKET_ERROR) {
        std::cerr << "Failed to listen on socket\n";
        CLOSE_SOCKET(s);
        return false;
    }

    server_socket_ = (uintptr_t)s;
    running_ = true;
    std::cout << "[Workbench Backend] HTTP server listening on http://localhost:" << port_ << std::endl;

    while (running_) {
        sockaddr_in client_addr;
        int client_len = sizeof(client_addr);
        socket_t client_sock = accept(s, (sockaddr*)&client_addr, &client_len);
        if (client_sock == INVALID_SOCKET) {
            if (!running_) break;
            continue;
        }

        std::thread([this, client_sock]() {
            handle_client((uintptr_t)client_sock);
        }).detach();
    }

    return true;
}

void HttpServer::handle_client(uintptr_t client_socket) {
    socket_t sock = (socket_t)client_socket;

    std::string raw_request;
    char buffer[4096];
    size_t content_length = 0;
    bool headers_parsed = false;
    size_t header_end_pos = std::string::npos;

    while (true) {
        int bytes = recv(sock, buffer, sizeof(buffer), 0);
        if (bytes <= 0) break;
        raw_request.append(buffer, bytes);

        if (!headers_parsed) {
            header_end_pos = raw_request.find("\r\n\r\n");
            if (header_end_pos != std::string::npos) {
                headers_parsed = true;

                std::string headers_part = raw_request.substr(0, header_end_pos);
                std::istringstream hstream(headers_part);
                std::string hline;
                while (std::getline(hstream, hline)) {
                    std::string lower_line = hline;
                    std::transform(lower_line.begin(), lower_line.end(), lower_line.begin(), ::tolower);
                    if (lower_line.find("content-length:") == 0) {
                        size_t colon = hline.find(':');
                        if (colon != std::string::npos) {
                            try {
                                content_length = std::stoul(trim(hline.substr(colon + 1)));
                            } catch (...) {}
                        }
                    }
                }
            }
        }

        if (headers_parsed) {
            size_t body_bytes_read = raw_request.size() - (header_end_pos + 4);
            if (body_bytes_read >= content_length) {
                break;
            }
        }
    }

    if (raw_request.empty()) {
        CLOSE_SOCKET(sock);
        return;
    }

    HttpRequest req;
    size_t first_line_end = raw_request.find("\r\n");
    if (first_line_end == std::string::npos) {
        CLOSE_SOCKET(sock);
        return;
    }

    std::string request_line = raw_request.substr(0, first_line_end);
    std::istringstream req_stream(request_line);
    std::string raw_url;
    req_stream >> req.method >> raw_url;

    size_t qmark = raw_url.find('?');
    if (qmark != std::string::npos) {
        req.path = raw_url.substr(0, qmark);
        req.query_string = raw_url.substr(qmark + 1);

        std::istringstream qstream(req.query_string);
        std::string pair;
        while (std::getline(qstream, pair, '&')) {
            size_t eq = pair.find('=');
            if (eq != std::string::npos) {
                req.query_params[pair.substr(0, eq)] = pair.substr(eq + 1);
            } else {
                req.query_params[pair] = "";
            }
        }
    } else {
        req.path = raw_url;
    }

    if (header_end_pos != std::string::npos) {
        req.body = raw_request.substr(header_end_pos + 4);
        if (content_length > 0 && req.body.size() > content_length) {
            req.body = req.body.substr(0, content_length);
        }
    }

    HttpResponse res = dispatch(req);

    std::ostringstream resp_stream;
    resp_stream << "HTTP/1.1 " << res.status_code << " ";
    if (res.status_code == 200) resp_stream << "OK";
    else if (res.status_code == 400) resp_stream << "Bad Request";
    else if (res.status_code == 404) resp_stream << "Not Found";
    else if (res.status_code == 500) resp_stream << "Internal Server Error";
    else resp_stream << "Status";
    resp_stream << "\r\n";

    resp_stream << "Content-Type: " << res.content_type << "\r\n";
    resp_stream << "Content-Length: " << res.body.size() << "\r\n";
    resp_stream << "Access-Control-Allow-Origin: *\r\n";
    resp_stream << "Access-Control-Allow-Methods: GET, POST, OPTIONS, PUT, DELETE\r\n";
    resp_stream << "Access-Control-Allow-Headers: Content-Type, Authorization, Accept\r\n";
    resp_stream << "Connection: close\r\n";

    for (const auto& h : res.custom_headers) {
        resp_stream << h.first << ": " << h.second << "\r\n";
    }

    resp_stream << "\r\n";
    resp_stream << res.body;

    std::string response_data = resp_stream.str();
    send(sock, response_data.data(), static_cast<int>(response_data.size()), 0);

    CLOSE_SOCKET(sock);
}

HttpResponse HttpServer::dispatch(const HttpRequest& req) {
    if (req.method == "OPTIONS") {
        HttpResponse res;
        res.status_code = 200;
        res.content_type = "text/plain";
        res.body = "OK";
        return res;
    }

    if (req.method == "GET") {
        auto it = get_routes_.find(req.path);
        if (it != get_routes_.end()) {
            return it->second(req);
        }

        // Static file serving fallback
        std::vector<std::string> base_dirs = {
            "ui",
            "dist_ui",
            "out",
            "../src/frontend/out",
            "src/frontend/out"
        };

        std::string rel_path = req.path;
        if (rel_path == "/" || rel_path.empty()) {
            rel_path = "/index.html";
        }

        for (const auto& base : base_dirs) {
            std::string full_path = base + rel_path;
            std::string content;
            if (read_file_bytes(full_path, content)) {
                HttpResponse res;
                res.status_code = 200;
                res.content_type = get_mime_type(full_path);
                res.body = std::move(content);
                return res;
            }

            if (read_file_bytes(full_path + ".html", content)) {
                HttpResponse res;
                res.status_code = 200;
                res.content_type = "text/html; charset=utf-8";
                res.body = std::move(content);
                return res;
            }

            if (read_file_bytes(full_path + "/index.html", content)) {
                HttpResponse res;
                res.status_code = 200;
                res.content_type = "text/html; charset=utf-8";
                res.body = std::move(content);
                return res;
            }
        }
    } else if (req.method == "POST") {
        auto it = post_routes_.find(req.path);
        if (it != post_routes_.end()) {
            return it->second(req);
        }
    }

    return HttpResponse::error("Endpoint not found: " + req.method + " " + req.path, 404);
}

} // namespace workbench
