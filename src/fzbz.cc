#include "parser.h"
#include "interpreter.h"
#include <fstream>
using namespace std;

bool read_file(const char* path, vector<char>& buf) {
  ifstream ifs(path, ios::in);
  if (ifs.fail()) {
    return false;
  }
  buf.resize(static_cast<unsigned int>(ifs.seekg(0, ios::end).tellg()));
  ifs.seekg(0, ios::beg).read(&buf[0], static_cast<streamsize>(buf.size()));
  return true;
}

int main(int argc, const char** argv) {
  if (argc < 2) {
    cerr << "usage: fzbz [source file path]" << endl;
    return 1;
  }

  auto path = argv[1];

  vector<char> source;
  if (!read_file(path, source)) {
    cerr << "can't open the source file." << endl;
    return 2;
  }

  auto ast = parse(source, cerr);
  if (!ast) {
    return 3;
  }
  // cout << peg::ast_to_s(ast) << endl;

  try {
    interpret(*ast);
  } catch (const exception& e) {
    cerr << e.what() << endl;
    return 4;
  }

  return 0;
}
