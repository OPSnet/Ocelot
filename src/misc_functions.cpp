#include <string>
#include <iostream>
#include <sstream>
#include <netdb.h>


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

std::string hex_decode(const std::string &in) {
	std::string out;
	out.reserve(20);
	unsigned int in_length = in.length();
	for (unsigned int i = 0; i < in_length; i++) {
		unsigned char x = '0';
		if (in[i] == '%' && (i + 2) < in_length) {
			i++;
			if (in[i] >= 'a' && in[i] <= 'f') {
				x = static_cast<unsigned char>((in[i]-87) << 4);
			} else if (in[i] >= 'A' && in[i] <= 'F') {
				x = static_cast<unsigned char>((in[i]-55) << 4);
			} else if (in[i] >= '0' && in[i] <= '9') {
				x = static_cast<unsigned char>((in[i]-48) << 4);
			}

			i++;
			if (in[i] >= 'a' && in[i] <= 'f') {
				x += static_cast<unsigned char>(in[i]-87);
			} else if (in[i] >= 'A' && in[i] <= 'F') {
				x += static_cast<unsigned char>(in[i]-55);
			} else if (in[i] >= '0' && in[i] <= '9') {
				x += static_cast<unsigned char>(in[i]-48);
			}
		} else {
			x = in[i];
		}
		out.push_back(x);
	}
	return out;
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

bool ipv4_is_public(in_addr addr){
	// Match against reserved ranges
	if ((ntohl(addr.s_addr) & 0xff000000) == 0x0a000000) return false; // 10.0.0.0/8
	if ((ntohl(addr.s_addr) & 0xfff00000) == 0xac100000) return false; // 172.16.0.0/12
	if ((ntohl(addr.s_addr) & 0xffff0000) == 0xc0a80000) return false; // 192.168.0.0/16
	if ((ntohl(addr.s_addr) & 0xffff0000) == 0xa9fe0000) return false; // 169.254.0.0/16
	if ((ntohl(addr.s_addr) & 0xffc00000) == 0x64400000) return false; // 100.64.0.0/10
	if ((ntohl(addr.s_addr) & 0xff000000) == 0x7f000000) return false; // 127.0.0.0/8
	return true;
}

bool ipv6_is_public(in6_addr addr){

	// Match against reserved ranges
	if (ntohl(addr.s6_addr32[0]) == 0x00000000) return false; // Loopback / v4 compat v6
	if (ntohs(addr.s6_addr16[0]) == 0xfe80    ) return false; // Link local
	if (ntohs(addr.s6_addr16[0]) == 0xfc00    ) return false; // Unique Local - private subnet
	if (ntohs(addr.s6_addr16[0]) == 0xfec0    ) return false; // site-local [deprecated]
	if (ntohs(addr.s6_addr16[0]) == 0x3ffe    ) return false; // 6bone [deprecated]
	if (ntohl(addr.s6_addr32[0]) == 0x20010db8) return false; // documentation examples, unroutable
	if (ntohl(addr.s6_addr32[0]) == 0x20010000) return false; // Teredo
	if (ntohs(addr.s6_addr16[0]) == 0x2002    ) return false; // 6to4
	return true;
}

std::string bencode_int(int data) {
	std::string bencoded_int = "i";
	bencoded_int += inttostr(data);
	bencoded_int += "e";
	return bencoded_int;
}

std::string bencode_str(std::string data) {
	std::string bencoded_str = inttostr(data.size());
	bencoded_str += ":";
	bencoded_str += data;
	return bencoded_str;
}
