#include <string>
#include <iostream>
#include <sstream>

int32_t strtoint32(const std::string& str) {
	std::istringstream stream(str);
	int32_t i = 0;
	stream >> i;
	return i;
}

int64_t strtoint64(const std::string& str) {
	std::istringstream stream(str);
	int64_t i = 0;
	stream >> i;
	return i;
}


std::string inttostr(const int i) {
	std::string str;
	std::stringstream out;
	out << i;
	str = out.str();
	return str;
}

std::string bintohex(const std::string &in) {
	std::string out;
	size_t length = in.length();
	out.reserve(2*length);
	for (unsigned int i = 0; i < length; i++) {
		unsigned char x = static_cast<unsigned char>((in[i] & 0xF0) >> 4);
		if (x > 9) {
			x += 'a' - 10;
		} else {
			x += '0';
		}
		out.push_back(x);
		x = in[i] & 0x0F;
		if (x > 9) {
			x += 'a' - 10;
		} else {
			x += '0';
		}
		out.push_back(x);
	}
	return out;
}
