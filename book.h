#include <iostream>
#include <string>
#include <cpprest/ws_client.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <sstream>
#include <algorithm>

using namespace web;
using namespace web::websockets::client;

using namespace boost::property_tree;

class book {

    private:
        std::vector<std::string> coins;
        std::string url = "wss://ws-feed.exchange.coinbase.com";

        std::string messenger(){
            std::string msg = "{\"type\":\"subscribe\",\"product_ids\":[";
            for(auto & coin : coins){
                msg += "\"" + coin + "\",";
            }
            msg.pop_back();
            msg += "],\"channels\":[\"level2_batch\"]}";
            return msg;
        }

        int xp(std::vector<double> x, double price){
            auto ii = std::find(x.begin(), x.end(), price);       
            if(ii != x.end()){
                return std::distance(x.begin(), ii);
            }
            return -1;
        }

        void Cyclone(std::string message){
            ptree df;
            std::stringstream ss(message);
            read_json(ss, df);
            bool snap = false, cid = false, l2 = false;
            std::string the_coin;
            for(ptree::const_iterator it = df.begin(); it != df.end(); ++it){
                if(l2 == true && cid == true && it->first == "changes"){
                    for(ptree::const_iterator jt = it->second.begin(); jt != it->second.end(); ++jt){
                        std::vector<std::string> temporary;
                        for(ptree::const_iterator kt = jt->second.begin(); kt != jt->second.end(); ++kt){
                            temporary.push_back(kt->second.get_value<std::string>());
                        }
                        
                        double price = atof(temporary[1].c_str());
                        double volume = atof(temporary[2].c_str());

                        if(temporary[0] == "buy"){
                            int index = xp(OrderBook[the_coin]["bidPrice"], price);
                            if(index != -1){
                                if(volume == 0){
                                    OrderBook[the_coin]["bidPrice"].erase(OrderBook[the_coin]["bidPrice"].begin() + index);
                                    OrderBook[the_coin]["bidSize"].erase(OrderBook[the_coin]["bidSize"].begin() + index);
                                } else {
                                    OrderBook[the_coin]["bidPrice"][index] = price;
                                    OrderBook[the_coin]["bidSize"][index] = volume;
                                }
                            } else {
                                OrderBook[the_coin]["bidPrice"].push_back(price);
                                OrderBook[the_coin]["bidSize"].push_back(volume);
                            }
                        }

                        if(temporary[0] == "sell"){
                            int index = xp(OrderBook[the_coin]["askPrice"], price);
                            if(index != -1){
                                if(volume == 0){
                                    OrderBook[the_coin]["askPrice"].erase(OrderBook[the_coin]["askPrice"].begin() + index);
                                    OrderBook[the_coin]["askSize"].erase(OrderBook[the_coin]["askSize"].begin() + index);
                                } else {
                                    OrderBook[the_coin]["askPrice"][index] = price;
                                    OrderBook[the_coin]["askSize"][index] = volume;
                                }
                            } else {
                                OrderBook[the_coin]["askPrice"].push_back(price);
                                OrderBook[the_coin]["askSize"].push_back(volume);
                            }
                        }
                        
                    }
                }

                if(snap == true && cid == true){
                    if(it->first == "bids"){
                        for(ptree::const_iterator jt = it->second.begin(); jt != it->second.end(); ++jt){
                            std::vector<double> temp;
                            for(ptree::const_iterator kt = jt->second.begin(); kt != jt->second.end(); ++kt){
                                temp.push_back(atof(kt->second.get_value<std::string>().c_str()));
                            }
                            OrderBook[the_coin]["bidPrice"].push_back(temp[0]);
                            OrderBook[the_coin]["bidSize"].push_back(temp[1]);
                        }
                    }
                    if(it->first == "asks"){
                        for(ptree::const_iterator jt = it->second.begin(); jt != it->second.end(); ++jt){
                            std::vector<double> temp;
                            for(ptree::const_iterator kt = jt->second.begin(); kt != jt->second.end(); ++kt){
                                temp.push_back(atof(kt->second.get_value<std::string>().c_str()));
                            }
                            OrderBook[the_coin]["askPrice"].push_back(temp[0]);
                            OrderBook[the_coin]["askSize"].push_back(temp[1]);
                        }
                    }
                }

                if(it->first == "type"){
                    if(it->second.get_value<std::string>() == "snapshot"){
                        snap = true;
                    }
                    if(it->second.get_value<std::string>() == "l2update"){
                        l2 = true;
                    }
                }
                if(it->first == "product_id"){
                    cid = true;
                    the_coin = it->second.get_value<std::string>();
                }
            }
        }

    public:
        book(std::vector<std::string> tickers){
            coins = tickers;
        }

        std::map<std::string, std::map<std::string, std::vector<double>>> OrderBook;

        static void WebSocket(book * bk){
            std::string msg = bk->messenger();
            
            websocket_client client;
            client.connect(bk->url).wait();

            websocket_outgoing_message outmsg;
            outmsg.set_utf8_message(msg);
            client.send(outmsg);

            while(true){
                client.receive().then([](websocket_incoming_message inmsg){
                    return inmsg.extract_string();
                }).then([&](std::string message){
                    std::cout << message << std::endl;
                    bk->Cyclone(message);
                }).wait();
            }

            client.close().wait();
        }

};