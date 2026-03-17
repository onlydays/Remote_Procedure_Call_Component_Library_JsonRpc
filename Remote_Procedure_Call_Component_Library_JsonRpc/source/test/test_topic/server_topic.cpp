#include "../../server/Rpc_Server.hpp"

int main()
{
    auto server = std::make_shared<only_days::server::TopicServer>(7070);
    server->start();
    return 0;
}