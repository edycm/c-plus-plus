#include "base64.h"
#include <iostream>
#include <string>
#include <fstream>
//#include <opencv2/imgproc.hpp>

using namespace std;
int main()
{
#if 0
	std::string file_path = "/home/cys/license_plate/image1/1.jpg";
	fstream f, f2;
	f.open(file_path, ios::in | ios::binary);
	f.seekg(0, std::ios_base::end);
	std::streampos sp = f.tellg();
	int size = sp;
	cout << size << endl;
	char* buffer = (char*)malloc(sizeof(char) * size);
	f.seekg(0, std::ios_base::beg);//把文件指针移到到文件头位置
	f.read(buffer, size);
	f.close();

	string imgBase64 = base64_encode(buffer, size);
	cout << "img base64 encode size:" << imgBase64.size() << endl;
	string imgdecode64 = base64_decode(imgBase64);
	cout << "img decode size:" << imgdecode64.size() << endl;

	f2.open("../out.jpg", ios::out | ios::binary);
	//f2 << imgdecode64;
	f2.write(imgdecode64.c_str(), imgdecode64.size());
	f2.close();
#endif
	std::string tr = base64_encode("123354", 7);
	std::string test = base64_decode(tr);
	std::cout << "test: " << test << std::endl;
	return 0;
}
