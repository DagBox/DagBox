#include <bandit/bandit.h>
#include "socket.hpp"
#include "message.hpp"


go_bandit([](){
    test_socket();
    test_message();
});


auto main(int argc, char* argv[]) -> int
{
    return bandit::run(argc, argv);
}
