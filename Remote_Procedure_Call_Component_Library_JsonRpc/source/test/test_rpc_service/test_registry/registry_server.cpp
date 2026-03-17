#include "../../../common/Detail.hpp"
#include "../../../server/Rpc_Server.hpp"

int main()
{ 
    only_days::server::RegistryServer reg_server(8080);
    reg_server.start();
    return 0;
}