#include <iostream>
#include <thread>
#include <vector>
#include <string>

#include "book.h"

int main()
{
    std::vector<std::string> tickers = {"BTC-USD","ETH-USD","USDT-USD","DOGE-USD"};

    book client(tickers);

    std::thread feed(client.WebSocket, &client);

    

    feed.join();

    return 0;
}