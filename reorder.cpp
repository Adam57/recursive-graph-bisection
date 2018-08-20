#include "reorder.h"
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <sys/time.h>
#include <sys/stat.h>
#include <math.h>

bool sort_docs (docInfoR a, docInfoR b) { 
    if(a.part < b.part){
      return true;
    }else if(a.part > b.part){
      return false;
    }else{
      if(a.gain > b.gain){
        return true;
      }else{
        return false;
      }
    }
}

bool sort_docs_url (docInfoR a, docInfoR b) { 
    if(a.url < b.url){
      return true;
    }else{
      return false;
    }
}

void OrderR::increase_count_with_indicator(vector<lexInfoR>& lex, int tid) {
  if(lex[tid-1].id == current_ID) {
    lex[tid-1].count++;
  }else{
    lex[tid-1].id = current_ID;
    lex[tid-1].count = 1;
    lex[tid-1].gain = 0.0;
    lex[tid-1].indicator = false;
  }
}

int OrderR::get_count(vector<lexInfoR>& lex, int tid) {
  if(lex[tid-1].id == current_ID){
    return lex[tid-1].count;
  }else{
    return 0;
  }
}

double OrderR::exp_fb(int fs, int ts, int fc, int tc){
  if( (fs == 0) || (ts == 0)){
    std::cout << "fs or ts should not be zero in any case\n";
    exit(0);
  }
  if( (fc < 0) || (tc < 0) ){
    std::cout << "fc or tc should not be less than zero in any case\n";
    exit(0);
  }
  return((double)fc * log((double)fs / ((double)fc+1.0)) + (double)tc * log((double)ts / ((double)tc+1.0)));
}

void OrderR::get_gain_fb(vector<lexInfoR>& from_lex, int from_size, vector<lexInfoR>& to_lex, int to_size, docInfoR& doc){
  double doc_gain = 0.0;
  double term_gain = 0.0;
  /*  fetch terms */
  int d = doc.did;
  int chunk_size = sizes[d];
  int length = lengths[d];
  long long offset = offsets[d];
  int fc, tc;
  char d_data[chunk_size];
  int uncompressed_data[length];
  // char* d_data = new char[chunk_size];
  // int* uncompressed_data = new int[length];
  for(int j = 0; j < chunk_size; j++){
    d_data[j] = data[offset+j];
  }

  decompressionVbytes(d_data, uncompressed_data, length);

  for(int j = 1; j < length; j++){
    uncompressed_data[j]+= uncompressed_data[j-1]; 
  }

  /*  loop over all terms */
  for(int i = 0; i < length; i++){
    int tid = uncompressed_data[i];
    // cout << "tid1: " << tid << " ";
    if(from_lex[tid-1].indicator == false) {
        term_gain = 0;
        /*get count for from_lex of tid1*/
        fc = get_count(from_lex, tid); // fc is at least 1

        /*get count for to_lex of tid1*/
        tc = get_count(to_lex, tid); // tc is at least 0

        /* first add current estimated compressed size of list */
        // term_gain += fc * log(fs / (fc+1.0)) + tc * log(ts / (tc+1.0));
        term_gain += exp_fb(from_size, to_size, fc, tc);

        /* then deduct compressed size after moving document */
        // term_gain -= fc * log(fs / (fc)) + tc * log(ts / (tc+2.0));
        term_gain -= exp_fb(from_size, to_size, fc-1, tc+1);

        from_lex[tid-1].gain = term_gain;
        from_lex[tid-1].indicator = true;
    }
    doc_gain += from_lex[tid-1].gain;
  }
  doc.gain = doc_gain * (double)1000000.00;
  // delete[] d_data;
  // delete[] uncompressed_data;
}

void OrderR::init_fb(int MAXD, int number_of_term, long long vbyte_size, string vbyte_lex, string vbyte_index, string url_lex,
  string final_ordering){
    /*load forward index*/
    /*load lex*/
    // FILE* lex = fopen("/home/qi/reorder_data/forward_index_with_dups/all_nlex", "r");
    FILE* lex = fopen(vbyte_lex.c_str(), "r");
    int did, size, length;
    long long offset;

    sizes.reserve(MAXD);
    lengths.reserve(MAXD);
    offsets.reserve(MAXD);
    for(int i = 0; i < MAXD; i++){
      sizes.push_back(0);
      lengths.push_back(0);
      offsets.push_back(0);
    }

    while(fscanf(lex, "%d %d %d %lld", &did, &length, &size, &offset) == 4) {
      // cout << did << " " << size << " " << offset << "\n";
      sizes[did] = size;
      lengths[did] = length;
      offsets[did] = offset;
    }
    fclose(lex);

    // for(int i = 1; i < MAXD; i++){
    //   cout << i << " " << sizes[i] << " " << offsets[i] << "\n";
    // }
    cout << "lex loading done\n";

    /*load content*/
    long long data_size = vbyte_size;
    FILE* fp = fopen(vbyte_index.c_str(),"rb");
    data = new char[data_size];
    long long ret_code = fread(data, sizeof *data, data_size, fp); // reads an array of doubles
    if(ret_code == data_size) {
        puts("data read successfully");
    } else { // error handling
       if (feof(fp))
          printf("Error reading forward index: unexpected end of file\n");
       else if (ferror(fp)) {
           perror("Error reading forward index");
       }
    }
    fclose(fp);


    /*init doc array*/
    int total_docs = MAXD - 1;
    // int total_docs = 1000;
    vector<docInfoR> docs;
    docs.reserve(total_docs);
    std::string lex_file = url_lex.c_str();
    FILE* pfile = fopen(lex_file.c_str(), "r");
    char dname[1000];
    char url[10000];
    // int counter = 0;
    while(fscanf(pfile, "%s %d %s\n", url, &did, dname) == 3) {
      // std::cout << did << "\n";
      string s(url);
      docInfoR d(did, 0, 0.0, s);
      docs.push_back(d);
      // counter++;
      // if(counter >= 4000000) {
      // // if(counter >= 200000) {
      //   break;
      // }
    }
    fclose(pfile);

    threshold = 50;
    round = 20;
    current_ID = 0;

    /*init lex array*/
    term_num = number_of_term;
    vector<lexInfoR> left_lex;
    vector<lexInfoR> right_lex;
    left_lex.reserve(term_num);
    right_lex.reserve(term_num);

    for(int i = 0; i < term_num; i++){
      left_lex.push_back(lexInfoR(-1, 0, 0.0, false));
      right_lex.push_back(lexInfoR(-1, 0, 0.0, false));
    }

    /*recursion*/
    int left = 0;
    int right = docs.size() - 1;
    partition_fb_opt(left, right, docs, left_lex, right_lex);
    delete[] data;

    // for(int i = 0; i < final_order.size(); i++) {
    //   cout << i << ": " << final_order[i] << "\n";
    // }

    FILE* lfile = fopen(final_ordering.c_str(), "w");
    for(int i = 0; i < (int)(final_order.size()); i++) {
      // std::cout << i+1 << " " << final_order[i] << "\n";
      fprintf(lfile, "%d %d\n", i+1, final_order[i]);
    }
    fclose(lfile);
}

void OrderR::partition_fb_opt(int left, int right, vector<docInfoR>& docs, vector<lexInfoR>& left_lex, vector<lexInfoR>& right_lex){
  int d, length, chunk_size;
  long long offset;
  int size = right - left + 1;

  if(size <= threshold){
    //sort the dids here by url
    sort(docs.begin()+left, docs.begin()+left+size, sort_docs_url);

    //out put them to some global structure
    for(int i=left; i<=right; i++){
      final_order.push_back(docs[i].did);
    }
    return;
  }

  /* randomly assign half of docs to left half (part[i] = 0) or right half (part[i] = 1) */
  cout << "number of documents(left+right) at this level: " << size << "\n";
  // for (int i = left; i < left + size; i++) {
  //   docs[i].part = 1;
  //   docs[i].gain = 0.0;
  // }

  // srand (time(NULL));
  // for (int c = 0; c < size/2;){
  //   int j = rand() % size;
  //   // cout << j << "\n";
  //   if (docs[left+j].part == 1) { docs[left+j].part = 0; c++; }
  // }

  // /* next, move documents to their partitions by sorting */
  // sort(docs.begin()+left, docs.begin()+left+size, sort_docs);

  for (int i = left; i < left + size/2; i++) {
    docs[i].part = 0;
    docs[i].gain = 0.0;
  }

  /*divide the documents by sorting*/
  for (int i = left + size/2; i < left + size; i++){
    docs[i].part = 1;
    docs[i].gain = 0.0;
  }

  // for(int i = 0; i < docs.size(); i++) {
  //   std::cout << docs[i].did << " " << docs[i].part << " " << docs[i].gain << "\n";
  // }

  /* iterate 20 times as suggested in paper */
  for(int i = 0; i < round; i++){
      /* now count number of term occurrences in left partition */
      for(int k = left; k < left + size/2; k++) {
        /*fetch the terms*/
        // std::cout << k << " " << docs[k].did << "\n";
        d = docs[k].did;
        chunk_size = sizes[d];
        length = lengths[d];
        offset = offsets[d];
        // char* d_data = new char[chunk_size];
        // int* uncompressed_data = new int[length];
        char d_data[chunk_size];
        int uncompressed_data[length];
        for(int j = 0; j < chunk_size; j++){
          d_data[j] = data[offset+j];
        }

        decompressionVbytes(d_data, uncompressed_data, length);

        for(int j = 1; j < length; j++){
          uncompressed_data[j]+= uncompressed_data[j-1];
        }

        for(int j = 0; j < length; j++) {
          // cout << uncompressed_data[j] << " ";
          increase_count_with_indicator(left_lex, uncompressed_data[j]);
        }
        // cout << "\n";
        // delete[] d_data;
        // delete[] uncompressed_data;
      }
      /* now count number of term occurrences in right partition */
      for(int k = left + size/2; k < left + size; k++) {
        /*fetch the terms*/
        // std::cout << k << " " << docs[k].did << "\n";
        d = docs[k].did;
        chunk_size = sizes[d];
        length = lengths[d];
        offset = offsets[d];
        char d_data[chunk_size];
        int uncompressed_data[length];
        // char* d_data = new char[chunk_size];
        // int* uncompressed_data = new int[length];
        for(int j = 0; j < chunk_size; j++){
          d_data[j] = data[offset+j];
        }

        decompressionVbytes(d_data, uncompressed_data, length);

        for(int j = 1; j < length; j++){
          uncompressed_data[j]+= uncompressed_data[j-1];
        }

        for(int j = 0; j < length; j++) {
          // cout << uncompressed_data[j] << " ";
          increase_count_with_indicator(right_lex, uncompressed_data[j]);
        }
        // cout << "\n";
        // delete[] d_data;
        // delete[] uncompressed_data;
      }

      /* now for each document compute gain of going to other partition */
      for(int k = left; k < left + size/2; k++) {
         get_gain_fb(left_lex, size/2, right_lex, size-size/2, docs[k]);
      }
      for(int k = left + size/2; k < left + size; k++) {
         get_gain_fb(right_lex, size-size/2, left_lex, size/2, docs[k]);
      }

      /* next, need to sort docs in each partition by gain */
      sort(docs.begin()+left, docs.begin()+left+size, sort_docs);

      /* now, swap documents until gain less than zero */
      int m = 0;
      for (m = 0; (m < size/2) && (docs[left + m].gain + docs[left + size/2 + m].gain > 0); m++)
      {
        int did_tmp = docs[left + m].did;
        double gain_tmp = docs[left + m].gain;
        // string url_tmp = docs[left + k].url;

        docs[left + m].did = docs[left + size/2 + m].did;
        docs[left + m].gain = docs[left + size/2 + m].gain;
        // docs[left + k].url = docs[left + size/2 + k].url;

        docs[left + size/2 + m].did = did_tmp;
        docs[left + size/2 + m].gain = gain_tmp;
        // docs[left + size/2 + k].url = url_tmp;
        docs[left + m].url.swap(docs[left + size/2 + m].url);
      }

      /* if nothing was swapped in this iteration, no need for another one */
      if (m == 0) break;

      /* reset leftLex and rightLex lexicon structures to zero */
      current_ID++;
      // cout << "current_ID: " << current_ID << "\n";
      cout << "current_ID: " << current_ID << " number of document pairs swapped: " << m <<"\n";
  }

  // for(int i = 0; i < docs.size(); i++) {
  //     std::cout << docs[i].did << " " << docs[i].part << " " << docs[i].gain << "\n";
  // }

  /* we have fixed the partition on this level. Now recurse */
  partition_fb_opt(left, left + size/2-1, docs, left_lex, right_lex);
  partition_fb_opt(left + size/2, right, docs, left_lex, right_lex);
}


int decompressionVbytes(char* input, int* output, int size){
    char* curr_byte = input;
    unsigned n;
    for (int i = 0; i < size; ++i) {
      char b = *curr_byte;
      n = b & 0x7F;
//        cout<<"The first byte: "<<n<<endl;
//        print_binary(n);
//        cout<<endl;

      while((b & 0x80) !=0){
        n = n << 7;
        ++curr_byte;
          b = *curr_byte;
          n |= (b & 0x7F);
//          cout<<"The following byte: "<<n<<endl;
//          print_binary(n);
//          cout<<endl;
      }
      ++curr_byte;
      output[i] = n;
    }

   int num_bytes_consumed = (curr_byte - input);
   return (num_bytes_consumed >> 2) + ((num_bytes_consumed & 3) != 0 ? 1 : 0);
}

int compressionVbytes(std::vector<int>& input, std::vector<unsigned char>& compressedList){
   int compressedSize = 0;
   for (int i = 0; i < (int)(input.size()); ++i){
          unsigned char byteArr[sizeof(int)];
          bool started = false;
          int x = 0;

          for (x = 0; x < (int)(sizeof(int)); x++) {
             byteArr[x] = (input[i]%128);
             input[i] /= 128;
          }

          for (x = sizeof(int) - 1; x > 0; x--) {
            if (byteArr[x] != 0 || started == true) {
              started = true;
              byteArr[x] |= 128;
              compressedList.push_back(byteArr[x]);
              compressedSize++;
            }
          }
          compressedList.push_back(byteArr[0]);
          compressedSize++;
   }
   return compressedSize;
}

