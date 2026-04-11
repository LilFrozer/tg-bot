#ifndef BOT_H
#define BOT_H

#include <iostream>
#include <memory.h>
#include <string>
#include "json.hpp"
#include <cstdint>
using nlohmann::json;

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;

namespace TG
{
const std::string TELEGRAM_API = "api.telegram.org";
const std::string TELEGRAM_PORT = "443";
/**
 *  Интерфейс для взаимодействия с
 *  тг-ботом
 */
class TgApi
{
protected:
    virtual json sendRequest(const std::string &method, const json &payload = json::object()) = 0;
    virtual void sendMessage(int64_t chat_id, const std::string &text) = 0;
    virtual void processMessage(const json &message) = 0;
    virtual void processUpdate(const json &updates) = 0;
public:
    virtual void run() = 0;
};

/**
 *  Класс для работы с тг-ботом
 */
class MovieBot : public TgApi
{
private:
    std::string token_ = "";
    net::io_context& ioc_;
    ssl::context& ctx_;
    std::unique_ptr<beast::ssl_stream<beast::tcp_stream>> stream_;
    int64_t last_update_id_ = 0;
protected:
    json sendRequest(const std::string &method, const json &payload = json::object()) override final;
    void sendMessage(int64_t chat_id, const std::string &text) override final;
    void processMessage(const json& message) override final;
    void processUpdate(const json &updates) override final;
public:
    MovieBot(net::io_context& ioc, ssl::context& ctx, const std::string token)
            : token_(token), ioc_(ioc), ctx_(ctx)
    {
        std::cout << "token = " << token_ << std::endl;
        this->setupSSLContext();
    }
    ~MovieBot() = default;
    void run() override final;
    void setupSSLContext();
};
}

#endif
