#include <Bot.h>
#include <exception>

void TG::MovieBot::setupSSLContext()
{
    // Загружаем системные сертификаты
    ctx_.set_default_verify_paths();

    // На macOS дополнительно указываем путь к сертификатам Homebrew
#ifdef __APPLE__
    ctx_.add_verify_path("/opt/homebrew/etc/openssl@3/certs");
    ctx_.add_verify_path("/usr/local/etc/openssl@3/certs");
    ctx_.add_verify_path("/etc/ssl/certs");
#endif

    // Настраиваем верификацию сертификата
    ctx_.set_verify_mode(ssl::verify_peer);
    ctx_.set_verify_callback([](bool preverified, ssl::verify_context& ctx) {
        // Можно добавить дополнительную проверку здесь
        return preverified; // Возвращаем результат стандартной проверки
    });
}

void TG::MovieBot::run()
{
    std::cout << "STARTED!\n";
    while(true)
    {
        try
        {
            // Формируем запрос на получение обновлений
            json payload =
            {
                {"offset", last_update_id_ + 1},
                {"timeout", 30},  // Long polling на 30 секунд
                {"allowed_updates", json::array({"message"})}
            };
            auto response = this->sendRequest("getUpdates", payload);

            if (response.contains("ok") && response["ok"])
                this->processUpdate(response);
            else
                std::cerr << "API error: " << response.dump() << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Run loop error: " << e.what() << std::endl;
        }
    }
}

json TG::MovieBot::sendRequest(const std::string &method, const json &payload)
{
    try {
        // Создаем поток, если его нет или он закрыт
        if (!stream_ || !stream_->lowest_layer().is_open())
        {
            stream_ = std::make_unique<beast::ssl_stream<beast::tcp_stream>>(ioc_, ctx_);

            // Устанавливаем SNI (Server Name Indication) - обязательно для Telegram
            if(!SSL_set_tlsext_host_name(stream_->native_handle(), TELEGRAM_API.c_str()))
            {
                beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
                throw beast::system_error{ec};
            }

            // Разрешаем доменное имя
            tcp::resolver resolver(ioc_);
            auto const results = resolver.resolve(TELEGRAM_API, TELEGRAM_PORT);

            // Подключаемся к серверу
            beast::get_lowest_layer(*stream_).connect(results);

            // Выполняем SSL handshake
            stream_->handshake(ssl::stream_base::client);
        }

        // Формируем HTTP-запрос
        std::string body = payload.dump();
        http::request<http::string_body> req{http::verb::post,
            "/bot" + token_ + "/" + method, 11};
        req.set(http::field::host, TELEGRAM_API);
        req.set(http::field::user_agent, "Boost Beast Telegram Bot");
        req.set(http::field::content_type, "application/json");
        req.set(http::field::content_length, std::to_string(body.size()));
        req.body() = body;

        // Отправляем запрос
        http::write(*stream_, req);

        // Получаем ответ
        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(*stream_, buffer, res);

        // Проверяем статус ответа
        if (res.result() != http::status::ok)
        {
            throw std::runtime_error("HTTP error: " + std::to_string(res.result_int()));
        }

        // Парсим JSON
        return json::parse(res.body());

    }
    catch (const std::exception& e)
    {
        std::cerr << "Request error: " << e.what() << std::endl;
        // Пересоздаем поток при ошибке
        stream_.reset();
        throw;
    }
}

void TG::MovieBot::sendMessage(int64_t chat_id, const std::string &text)
{
    json payload = {
        {"chat_id", chat_id},
        {"text", text},
        {"parse_mode", "HTML"}
    };

    try
    {
        auto response = this->sendRequest("sendMessage", payload);
        if (!response.contains("ok") || !response["ok"])
        {
            std::cerr << "Failed to send message: " << response.dump() << std::endl;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Send message error: " << e.what() << std::endl;
    }
}

void TG::MovieBot::processMessage(const json& message)
{
    int64_t chat_id = message["chat"]["id"];
    std::string text = message.value("text", "");

    std::cout << "Received from " << chat_id << ": " << text << std::endl;

    // Обрабатываем команды
    if (text == "/start")
    {
        sendMessage(chat_id, "🤖 Привет! Я бот на Boost.Beast!\n\nДоступные команды:\n/help - показать помощь\n/start - приветствие");
    }
    else if (text == "/help")
    {
        sendMessage(chat_id, "📋 Доступные команды:\n/start - начать работу\n/help - показать это сообщение\n\nПросто напишите любое сообщение, и я его повторю!");
    }
    else if (!text.empty() && text[0] == '/')
    {
        sendMessage(chat_id, "❌ Неизвестная команда. Используйте /help для списка команд.");
    }
    else if (!text.empty())
    {
        sendMessage(chat_id, "🔄 Эхо: " + text);
    }
}

void TG::MovieBot::processUpdate(const json &updates)
{
    if (!updates.contains("result") || !updates["result"].is_array()) {
        return;
    }

    for (const auto& update : updates["result"]) {
        int64_t update_id = update["update_id"];

        // Пропускаем старые обновления
        if (update_id <= last_update_id_) {
            continue;
        }

        // Обрабатываем сообщение
        if (update.contains("message")) {
            processMessage(update["message"]);
        }

        last_update_id_ = update_id;
    }
}
