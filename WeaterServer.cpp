//
// Created by Михаил Булгаков on 10/09/2018.
//

#include "WeaterServer.h"

using namespace std;
using namespace web;
using namespace utility;
using namespace http;
using namespace http::experimental::listener;
using namespace client;

WeaterServer::WeaterServer(utility::string_t url) : w_listener(url) {
    w_listener.support(methods::GET, std::bind(&WeaterServer::handle_get, this, std::placeholders::_1));
    answer_c["unit"] = json::value::string(U("celsius"));

    if (const char *env_p = std::getenv("API_ADDR")) {
        api_addr = U(env_p);
    } else {
        api_addr = U("http://api.weatherbit.io/");
    }
    if (const char *env_p = std::getenv("API_KEY")) {
        if (strlen(env_p) == 32)
            api_key = U(env_p);
        else {
            ucout << "Wrong key format" << endl;
            api_key = U("0");
            exit(1);
        }
    } else {
        ucout << "Please provide api key through env" << endl;
        api_key = U("0");
        exit(1);
    }
    client_c = std::unique_ptr<http::client::http_client>(new http_client(api_addr));
}

void WeaterServer::handle_get(http_request msg) {
    string_t key = U("city");

    auto http_get_vars = uri::split_query(msg.request_uri().query());
    if (http_get_vars[key].empty()) {
        msg.reply(status_codes::BadRequest);
        return;
    }

    uri_builder builder(U("/v2.0/current"));
    builder.append_query(U("key"), api_key);
    builder.append_query(U("city"), U(http_get_vars[key]));

    http_response response = client_c->request(methods::GET, builder.to_string()).get();

    if (response.status_code() != status_codes::OK) {
        msg.reply(status_codes::BadRequest);
        return;
    }

    json::value v1 = response.extract_json().get();

    answer_c[key] = json::value::string(http_get_vars[key]);
    answer_c["temperature"] = v1["data"][0]["temp"];

    msg.reply(status_codes::OK, answer_c);
}