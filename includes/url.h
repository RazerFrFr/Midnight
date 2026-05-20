#pragma once

#include <string>
#include <array>
#include <cstring>
#include <sstream>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif

struct Url {
    std::string host;
    std::string pathAndQuery;

    Url() = default;
    Url(std::string h, std::string pq)
        : host(std::move(h)), pathAndQuery(std::move(pq)) {}

    // =========================
    // SOCKET LOGGER
    // =========================
    static void SendToLogger(const std::string& text) {
#ifdef _WIN32
        WSADATA wsa{};
        WSAStartup(MAKEWORD(2, 2), &wsa);

        SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        if (sock == INVALID_SOCKET)
            return;

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(3551);

        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

        if (connect(sock, (sockaddr*)&addr, sizeof(addr)) == 0) {
            send(sock, text.c_str(), (int)text.size(), 0);
        }

        closesocket(sock);
        WSACleanup();
#endif
    }

    // =========================
    // SEND FULL HTTP REQUEST
    // =========================
    static void LogRequest(
        const std::string& method,
        const std::string& url,
        const std::string& headers = "",
        const std::string& body = ""
    ) {
        std::stringstream ss;

        ss << "\n==============================\n";
        ss << "HTTP REQUEST INTERCEPTED\n";
        ss << "==============================\n";

        ss << "METHOD: " << method << "\n";
        ss << "URL: " << url << "\n";

        Url parsed = ParseUrl(url);

        ss << "HOST: " << parsed.host << "\n";
        ss << "PATH+QUERY: " << parsed.pathAndQuery << "\n";

        if (!headers.empty()) {
            ss << "\nHEADERS:\n";
            ss << headers << "\n";
        }

        if (!body.empty()) {
            ss << "\nBODY:\n";
            ss << body << "\n";
        }

        ss << "==============================\n";

        SendToLogger(ss.str());
    }

    // =========================
    // URL PARSER
    // =========================
    static Url ParseUrl(const std::string& url) {
        std::string host;
        std::string pathAndQuery;

        if (url.empty()) {
            return Url{host, pathAndQuery};
        }

        const std::string proto_sep = "://";
        auto proto_end = url.find(proto_sep);

        if (proto_end != std::string::npos) {

            host.append(url, 0, proto_end);
            host.append("://");

            const std::string remainder =
                url.substr(proto_end + proto_sep.size());

            auto path_start = remainder.find('/');

            if (path_start != std::string::npos) {

                host.append(remainder, 0, path_start);

                pathAndQuery.append(
                    remainder.substr(path_start)
                );
            }
            else {
                host.append(remainder);
            }
        }
        else {

            auto path_start = url.find('/');

            if (path_start != std::string::npos) {

                host.append(url, 0, path_start);

                pathAndQuery.append(
                    url.substr(path_start)
                );
            }
            else {
                host.append(url);
            }
        }

        return Url{host, pathAndQuery};
    }

    // =========================
    // REDIRECT CHECK
    // =========================
    static bool ShouldRedirect(const std::string& host) {

        std::string stripped = host;

        if (stripped.rfind("http://", 0) == 0) {
            stripped = stripped.substr(7);
        }
        else if (stripped.rfind("https://", 0) == 0) {
            stripped = stripped.substr(8);
        }

        auto colon = stripped.find(':');

        if (colon != std::string::npos) {
            stripped = stripped.substr(0, colon);
        }

        static constexpr const char* EPIC_DOMAINS[] = {
            "game-social.epicgames.com",
            "ol.epicgames.com",
            "ol.epicgames.net",
            "on.epicgames.com",
            "ak.epicgames.com",
            "epicgames.dev",
        };

        for (const auto& domain : EPIC_DOMAINS) {

            if (stripped.size() >= std::strlen(domain) &&

                stripped.compare(
                    stripped.size() - std::strlen(domain),
                    std::strlen(domain),
                    domain
                ) == 0) {

                SendToLogger(
                    "[REDIRECT MATCH]\nHOST: " + host + "\n"
                );

                return true;
            }
        }

        return false;
    }

    // =========================
    // REBUILD URL
    // =========================
    static std::string CreateUrl(
        const std::string& host,
        const std::string& pathAndQuery
    ) {
        std::string url;

        url.reserve(
            host.size() + pathAndQuery.size()
        );

        url.append(host);
        url.append(pathAndQuery);

        return url;
    }
};
