#include "reorder.h"
#include <iostream>
using namespace std;

int main(int argc, char *argv[]){
    std::string command(argv[1]);
    if(command == "or_fb") {
        std::cout << "retrieving new order with fb algorithm for "<< argv[6] << " collection\n";
        std::string MAXD(argv[2]);
        std::string::size_type sz;   // alias of size_t
        int max_d = std::stoi(MAXD, &sz);
        string number_of_term_s(argv[3]);
        int number_of_term = std::stoi(number_of_term_s, &sz);
        string vbyte_size_s(argv[4]);
        long long vbyte_size = std::stoll(vbyte_size_s, &sz);
        string vbyte_lex(argv[5]);
        string vbyte_index(argv[6]);
        string url_lex(argv[7]);
        string final_ordering(argv[8]);
        OrderR r;
        r.init_fb(max_d, number_of_term, vbyte_size, vbyte_lex, vbyte_index, url_lex, final_ordering);
    }
}
