#include <string>
#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>

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
	unsigned long in_length = in.length();
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
			x = (unsigned char) in[i];
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
		auto x = static_cast<unsigned char>((in[i] & 0xF0) >> 4);
		if (x > 9) {
			x += 'a' - 10;
		} else {
			x += '0';
		}
		out.push_back(x);
		x = static_cast<unsigned char>(in[i] & 0x0F);
		if (x > 9) {
			x += 'a' - 10;
		} else {
			x += '0';
		}
		out.push_back(x);
	}
	return out;
}

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*) sa)->sin_addr);
	}
	else {
		return &(((struct sockaddr_in6 *) sa)->sin6_addr);
	}
}

/**
 * Encodes an integer into the proper bencode format. An integer is encoded as i<integer encoded in base ten ASCII>e.
 * Leading zeros are not allowed (although the number zero is still represented as "0"). Negative values are encoded
 * by prefixing the number with a minus sign. The number 42 would thus be encoded as i42e, 0 as i0e, and -42 as i-42e.
 * Negative zero is not permitted.
 *
 * @param data
 * @return Bencode string
 */
std::string bencode_int(int data) {
	std::string bencoded_int = "i";
	bencoded_int += std::to_string(data);
	bencoded_int += "e";
	return bencoded_int;
}

/**
 * Encodes a string into the proper bencode format. A byte string (a sequence of bytes, not necessarily characters)
 * is encoded as <length>:<contents>. The length is encoded in base 10, like integers, but must be non-negative
 * (zero is allowed); the contents are just the bytes that make up the string. The string "spam" would be encoded as
 * 4:spam. The specification does not deal with encoding of characters outside the ASCII set; to mitigate this,
 * some BitTorrent applications explicitly communicate the encoding (most commonly UTF-8) in various non-standard
 * ways. This is identical to how netstrings work, except that netstrings additionally append a comma suffix after
 * the byte sequence.
 *
 * @param data
 * @return
 */
std::string bencode_str(std::string data) {
	std::string bencoded_str = std::to_string(data.size());
	bencoded_str += ":";
	bencoded_str += data;
	return bencoded_str;
}


/**
 * Returns a boolean for whether or not a given IPv4 address is a private/reserved address or not. A user should not
 * be able to use one of these private addresses. The list of private/reserved IP addresses that we disallow are
 * taken from https://en.wikipedia.org/wiki/IPv4#Special-use_addresses.
 * For a very good primer on what masks means for IPv4 addresses and then why we're using bitwise operations:
 * https://www.linuxquestions.org/questions/linux-networking-3/what-does-8-32-etc-mean-for-ip-240380/#post1223010
 *
 * We test for (in order):
 * 	1. 10.0.0.0/8
 * 	2. 100.64.0.0/10
 * 	3. 127.0.0.0/8
 * 	4. 169.254.0.0/16
 * 	5. 172.16.0.0/12
 * 	6. 192.168.0.0/16
 * 	7. 192.0.2.0/24
 * 	8. 198.51.100.0/24
 * 	9. 203.0.113.0/24
 * @param addr
 * @return
 */
bool private_ipv4(in_addr addr) {
	uint32_t address = ntohl(addr.s_addr);
	return (address & 0xff000000) == 0x0a000000 ||
			(address & 0xffc00000) == 0x64400000 ||
			(address & 0xff000000) == 0x7f000000 ||
			(address & 0xffff0000) == 0xa9fe0000 ||
			(address & 0xfff00000) == 0xac100000 ||
			(address & 0xffff0000) == 0xc0a80000 ||
			(address & 0xffffff00) == 0xC0000200 ||
			(address & 0xffffff00) == 0xC6336400 ||
			(address & 0xffffff00) == 0xCB007100;
}

/**
 * Returns a boolean for whether or not a given IPv6 address is a private/reserved address or not. A user should not
 * be able to use one of these private addresses. The list of these IP addresses that are disallowed are taken
 * from https://en.wikipedia.org/wiki/IPv6_address#Special_addresses.
 *
 * IPv6 is 128 bytes possible where there are 8 groups of 16 bits that are represented via 4 hexidecimal digits.
 * How this gets represented can be read more about on Wikipedia (https://en.wikipedia.org/wiki/IPv6#Address_representation)
 *
 * The in6_addr structure then has (at least on the architectures we care about):
 * 	s6_addr32[4]
 * 	s6_addr16[8]
 * 	s6_addr8[16]
 * which allow for different granularity of referencing the address space.
 *
 * We are returning true for addresses in the following spaces:
 * 	1. ::1/128
 * 	2. ::ffff/96
 *  3. fe80::/10
 *  4. fc00::/7
 *  5. fec0::/16
 *  6. 3ffe::/16
 *  7. 2001:0db8::/32
 *  8. 2001:0000::/32
 *  9. 2002::/16
 *
 * @param addr
 * @return
 */
bool private_ipv6(in6_addr addr) {
	uint32_t addr32[4];
	for (int i = 0; i < 4; i++) {
		addr32[i] = ntohl(addr.s6_addr32[i]);
	}
	return (addr32[0] == 0x00000000 && addr32[0] == addr32[1] && addr32[0] == addr32[2] && addr32[3] == 0x00000001) || //
		   true ;
}

/*
 * 	if (ntohl(addr.s6_addr32[0]) == 0x00000000) return false; // Loopback / v4 compat v6
	if (ntohs(addr.s6_addr16[0]) == 0xfe80    ) return false; // Link local
	if (ntohs(addr.s6_addr16[0]) == 0xfc00    ) return false; // Unique Local - private subnet
	if (ntohs(addr.s6_addr16[0]) == 0xfec0    ) return false; // site-local [deprecated]
	if (ntohs(addr.s6_addr16[0]) == 0x3ffe    ) return false; // 6bone [deprecated]
	if (ntohl(addr.s6_addr32[0]) == 0x20010db8) return false; // documentation examples, unroutable
	if (ntohl(addr.s6_addr32[0]) == 0x20010000) return false; // Teredo
	if (ntohs(addr.s6_addr16[0]) == 0x2002    ) return false; // 6to4
 */

