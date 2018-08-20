#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <iostream>
#include <fstream>
using namespace std;

int compressionVbytes(std::vector<int>& input, std::vector<unsigned char>& compressedList);
int decompressionVbytes(char* input, int* output, int size);
int decompressionVbytes_new(char* input, int* output, int size);

class lexInfoR{
public:
	int id;
	int count;
	double gain;
	bool indicator;
	lexInfoR(){}
	lexInfoR(int i, int c, double g){
		id = i;
		count = c;
		gain = g;
	}
	lexInfoR(int i, int c, double g, bool b){
		id = i;
		count = c;
		gain = g;
		indicator = b;
	}
};

class docInfoR{
public:
	int part;
	int did;
	double gain;
	string url;
	docInfoR(){}
	docInfoR(int d, int p, double g){
		part = p;
		did = d;
		gain = g;
	}
	docInfoR(int d, int p, double g, string u){
		part = p;
		did = d;
		gain = g;
		url = u;
	}
};

class OrderR{
public:
	/*the algorithm from the fb paper*/
	void init_fb(int MAXD, int number_of_term, long long vbyte_size, string vbyte_lex, string vbyte_index, string url_lex,
	string final_ordering);

//private:
	int threshold;
	int round;
	int current_ID;
	int term_num;
	int did_num;
	// int d, length, chunk_size;
 //  	long long offset;
 //  	docInfoR tmp;
	vector<int> final_order;

	/*for vbyte compressed forward index*/
	vector<int> lengths;
        vector<int> sizes;
        vector<long long> offsets;
        char* data;

        /*for exp_bp_ppp, the most recent did each term appears in*/
        vector<int> most_recent_did_vec;

        void update_recent_did_for_terms(int did, int recent_did);

	void increase_count_with_indicator(vector<lexInfoR>& lex, int tid);
	int get_count(vector<lexInfoR>& lex, int tid);
       
	/*for fb algo*/
	double exp_fb(int fs, int ts, int fc, int tc);
	double exp_fb_double(double fs, double ts, double fc, double tc);
	void get_gain_fb(vector<lexInfoR>& from_lex, int from_size, vector<lexInfoR>& to_lex, int to_size, docInfoR& doc);
	void get_gain_fb_scaled_diff(vector<lexInfoR>& from_lex, int from_size, vector<lexInfoR>& to_lex, int to_size, docInfoR& doc);
	void get_gain_fb_using_lm(vector<lexInfoR>& from_lex, int from_size, vector<lexInfoR>& to_lex, int to_size, docInfoR& doc);

	void partition_fb_opt(int left, int right, vector<docInfoR>& docs, vector<lexInfoR>& left_lex, vector<lexInfoR>& right_lex);
};
	
