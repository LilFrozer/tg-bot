#include <Bot.h>
#include <memory>

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cout << "Invalid token\n";
        return EXIT_FAILURE;
    }
    try {
        // Инициализация Boost.Asio
        net::io_context ioc;

        // Настройка SSL контекста
        ssl::context ctx(ssl::context::tlsv12_client);

        // Создаем и запускаем бота
        std::string token = argv[1];
        std::shared_ptr<TG::MovieBot> movieBot = std::make_shared<TG::MovieBot>(ioc, ctx, std::move(token));
        movieBot->run();

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
