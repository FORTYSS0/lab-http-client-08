// Copyright 2021 by FORTYSS

#include <iostream>
#include <string>
#include <cstdlib>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>


//Выполняю HTTP GET и печатает ответ
int main(int argc, char** argv)
{
  try {
    // Проверяю аргументы командной строки.
    if (argc != 5 && argc != 6) {
      std::cerr <<
                "Usage: http-client-sync <host> <port> <target> <body>"
          "[<HTTP version: 1.0 or 1.1(default)>]\n" <<
                "Example:\n" <<
                "    http-client-sync www.example.com 80 / hello\n" <<
                "    http-client-sync www.example.com 80 / hello 1.0\n";
      return EXIT_FAILURE;
    }
    auto const host = argv[1];
    auto const port = argv[2];
    auto const target = argv[3];
    auto const body = argv[4];
    int version = argc == 6 && !std::strcmp("1.0", argv[5]) ? 10 : 11;

    // io_context необходим для всех операций ввода-вывода
    boost::asio::io_context ioc;

    // Эти объекты выполняют наш ввод-вывод
    boost::asio::ip::tcp::resolver resolver(ioc);
    boost::beast::tcp_stream stream(ioc);

    // Нахожу доменное имя
    auto const results = resolver.resolve(host, port);

    // Устанавливаю соединение по IP-адресу, который я получаю из поиска
    stream.connect(results);

    // Настраиваю сообщения запроса HTTP GET
    boost::beast::http::request<boost::beast::http::string_body>
        req{boost::beast::http::verb::post, target, version};
    req.set(boost::beast::http::field::host, host);
    req.set(boost::beast::http::field::user_agent,
            BOOST_BEAST_VERSION_STRING);
    req.set(boost::beast::http::field::content_type,
            "application/json");
    req.body() = "{\n\t\"input\": \"" + std::string(body) + "\"\n}";

    req.prepare_payload();
    // Отправляю HTTP-запрос на удаленный хост
    boost::beast::http::write(stream, req);

    // Этот буфер использую для чтения и он должен сохраняться
    boost::beast::flat_buffer buffer;

    // Объявляю контейнер для хранения ответа
    boost::beast::http::response<boost::beast::http::dynamic_body> res;

    // Получаю ответ HTTP
    boost::beast::http::read(stream, buffer, res);

    // Пишу сообщение стандартного выхода
    std::cout << res << std::endl;

    // Закрываю сокет
    boost::beast::error_code ec;
    stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both,
                             ec);

    // Дисконект постоянно случается, поэтому сообщение об этом я не вывожу
    if (ec && ec != boost::beast::errc::not_connected)
      throw boost::beast::system_error{ec};

    // Если прога дойдёт сюда, то соединение будет закрыто адекватно
  } catch(std::exception const& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
